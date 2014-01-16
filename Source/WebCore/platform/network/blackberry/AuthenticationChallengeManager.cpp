/*
 * Copyright (C) 2012, 2013 BlackBerry Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "AuthenticationChallengeManager.h"

#include "CredMgrUtils.h"
#include "Credential.h"
#include "CredentialTransformData.h"
#include "HTMLCollection.h"
#include "HTMLFormElement.h"
#include "KURL.h"
#include "Node.h"
#include "PageClientBlackBerry.h"
#include "ProtectionSpace.h"

#include <BlackBerryPlatformExecutableMessage.h>
#include <BlackBerryPlatformGuardedPointer.h>
#include <BlackBerryPlatformLog.h>
#include <BlackBerryPlatformMessageClient.h>
#include <set>
#include <sys/credmgr_dialog.h>
#include <sys/credmgr_return.h>
#include <wtf/Assertions.h>
#include <wtf/HashMap.h>
#include <wtf/Vector.h>
#include <wtf/text/CString.h>

using BlackBerry::Platform::createFunctionCallMessage;
using BlackBerry::Platform::createMethodCallMessage;
using BlackBerry::Platform::GuardedPointer;
using BlackBerry::Platform::GuardedPointerBase;
using BlackBerry::Platform::GuardedPointerLocker;
using BlackBerry::Platform::logAlways;
using BlackBerry::Platform::LogLevelCritical;
using BlackBerry::Platform::LogLevelWarn;
using BlackBerry::Platform::MessageClient;
using BlackBerry::Platform::SharedGuardedPointer;
using BlackBerry::Platform::Singleton;
using BlackBerry::Platform::webKitThreadMessageClient;

namespace WebCore {

typedef HashMap<PageClientBlackBerry*, bool> PageVisibilityMap;

class ChallengeInfo : public GuardedPointerBase {
public:
    ChallengeInfo(const KURL&, const ProtectionSpace&, const CredMgrServerQuery&, AuthenticationChallengeClient*, PageClientBlackBerry*);

    KURL url;
    ProtectionSpace space;
    CredMgrServerQuery query;
    AuthenticationChallengeClient* authClient;
    PageClientBlackBerry* pageClient;
    bool blocked;

private:
    ~ChallengeInfo();
};

ChallengeInfo::ChallengeInfo(const KURL& aUrl, const ProtectionSpace& aSpace, const CredMgrServerQuery& aQuery, AuthenticationChallengeClient* anAuthClient, PageClientBlackBerry* aPageClient)
    : url(aUrl)
    , space(aSpace)
    , query(aQuery)
    , authClient(anAuthClient)
    , pageClient(aPageClient)
    , blocked(false)
{
}

ChallengeInfo::~ChallengeInfo()
{
}

// Since the CredMgr API is synchronous, if it shows a prompt we need to do it in a background
// thread instead of blocking the webkit thread.
class CredMgrPromptThread : public MessageClient, public Singleton<CredMgrPromptThread> {
    SINGLETON_DEFINITION_THREADSAFE(CredMgrPromptThread)

private:
    CredMgrPromptThread()
    {
        createThread("credmgr_prompt");
    }

    virtual ~CredMgrPromptThread()
    {
    }
};

SINGLETON_INITIALIZER_THREADSAFE(CredMgrPromptThread)

// A comparison function to use ProtectionSpace with an STL set
// FIXME: we should be using WTF::HashSet, but it is locking up on add for some reason.
struct ProtectionSpaceCompare : std::binary_function<ProtectionSpace, ProtectionSpace, bool> {
    bool operator()(const ProtectionSpace& a, const ProtectionSpace& b) const
    {
        int hostCmp = strcmp(a.host().utf8().data(), b.host().utf8().data());
        if (hostCmp < 0) return true;
        if (hostCmp > 0) return false;

        if (a.port() < b.port()) return true;
        if (a.port() > b.port()) return false;

        if (a.serverType() < b.serverType()) return true;
        if (a.serverType() > b.serverType()) return false;

        int realmCmp = strcmp(a.realm().utf8().data(), b.realm().utf8().data());
        if (realmCmp < 0) return true;
        if (realmCmp > 0) return false;

        if (a.authenticationScheme() < b.authenticationScheme()) return true;
        if (a.authenticationScheme() > b.authenticationScheme()) return false;

        // a == b, so it is not less.
        return false;
    }
};

class AuthenticationChallengeManagerPrivate : public GuardedPointerBase {
public:
    AuthenticationChallengeManagerPrivate();

    bool resumeAuthenticationChallenge(PageClientBlackBerry*);
    void startAuthenticationChallenge(ChallengeInfo*);
    bool pageExists(PageClientBlackBerry*);
    AuthenticationChallengeResult runAuthenticationQuery(const CredMgrServerQuery&, bool allowPrompts, const ProtectionSpace&, Credential&);
    void runAuthenticationPrompt(const GuardedPointer<ChallengeInfo>&);
    void notifyChallengeResult(const KURL&, const ProtectionSpace&, AuthenticationChallengeResult, const Credential&);
    void notifyChallengeResultForClient(AuthenticationChallengeClient*, const KURL&, const ProtectionSpace&, AuthenticationChallengeResult, const Credential&);

    bool shouldSaveCredentials() const;

    // Pass the exact Query subclass as a template parameter because the query will be passed through an
    // ExecutableMessage, which cannot take a class with pure virtuals like CredMgrQuery.
    template<typename Query>
    bool createAndSaveItem(PageClientBlackBerry*, const Query&, const String& username, const String& password, const String& url, BlackBerry::Platform::String* handle = 0);

    ChallengeInfo* m_activeChallenge;
    PageVisibilityMap m_pageVisibilityMap;
    Vector<SharedGuardedPointer<ChallengeInfo> > m_challenges;

    // We need to distinguish between Negotiate failures caused by a bad password
    // (CREDMGR_AUTH_RESULT_FAIL returned immediately from credmgr_ItemFind because it could not
    // initialize a TGT) and failures caused by a bad Kerberos setup (the attempt to use the TGT
    // fails and the mechanism calls credmgr_ItemSetAuthResult(CREDMGR_AUTH_RESULT_FAIL), which is
    // then returned the next time we call credmgr_ItemFind). So, we will keep track of which hosts
    // we have tried Negotiate with already in this session. If CREDMGR_AUTH_RESULT_FAIL is returned
    // for a host we have not tried, we ask for a new password. Once CREDMGR_AUTH_RESULT_SUCCESS is
    // returned, we add the host to this set, and then the next time CREDMGR_AUTH_RESULT_FAIL is
    // returned for that host we know that the TGT has failed despite having the correct password.
    //
    // Once the TGT has failed, we disable Negotiate auth for that host. We also disable it as soon
    // as CREDMGR_AUTH_RESULT_AUTH_MECH_NOT_AVAIL is returned, which is a situation when CredMgr
    // itself has detected that Negotiate is unusable.
    //
    // FIXME: all this logic should be in CredMgr!
    std::set<ProtectionSpace, ProtectionSpaceCompare> m_triedNegotiateSet;

    // The current store will be m_privateCredentialStore if it exists, or CREDMGR_STORE_DEFAULT otherwise.
    credmgr_Store m_currentCredentialStore;

    // A temporary credential store used only in private mode.
    credmgr_Store m_privateCredentialStore;

    // There might be an error creating m_privateCredentialStore, so we also need to record whether
    // we are in private mode - we can't just check if m_privateCredentialStore exists.
    bool m_privateMode;

private:
    ~AuthenticationChallengeManagerPrivate();
};

#define ABORT_ON_ERROR(func, ...) do { \
    int rc = func(__VA_ARGS__); \
    if (rc != CREDMGR_SUCCESS) { \
        logAlways(LogLevelCritical, "%s failed: error 0x%04X", #func, rc); \
        return; \
    } \
} while (false)

#define ABORT_ON_ERROR_RET(ret, func, ...) do { \
    int rc = func(__VA_ARGS__); \
    if (rc != CREDMGR_SUCCESS) { \
        logAlways(LogLevelCritical, "%s failed: error 0x%04X", #func, rc); \
        return ret; \
    } \
} while (false)

#define WARN_ON_ERROR(func, ...) do { \
    int rc = func(__VA_ARGS__); \
    if (rc != CREDMGR_SUCCESS) \
        logAlways(LogLevelWarn, "%s failed: error 0x%04X", #func, rc); \
} while (false)

static Credential handleToCredential(const String& handle)
{
    // Store the handle in the "user" field of Credential, with nothing in the password. (This will
    // make Credential::hasPassword return false, but that's ok since we don't use it.)
    Credential credential(handle, WTF::emptyString(), CredentialPersistenceForSession);
    return credential;
}

AuthenticationChallengeResult AuthenticationChallengeManagerPrivate::runAuthenticationQuery(const CredMgrServerQuery& query, bool allowPrompts, const ProtectionSpace& space, Credential& credential)
{
    // This function is called once with allowPrompts set to false, so that if there is an existing
    // credential it will be returned immediately without blocking. If it returns
    // AuthenticationChallengeCancelled from that first call, we open a dialog to let the user enter
    // credentials. (This will either be done by calling pageClient->authenticationChallenge or by
    // calling this function again with allowPrompts set to true, so the the CredMgr library will
    // open a system dialog.) The AuthenticationChallengeCancelled code will only be sent through
    // notifyChallengeResult if the user hits cancel in the dialog (such as when this function is
    // called with allowPrompts set to true and returns AuthenticationChallengeCancelled anyway).

    // Prompts should only be shown in the prompt thread.
    ASSERT(!allowPrompts || CredMgrPromptThread::instance()->isCurrentThread());

    CredMgrObject<credmgr_Item> item;
    CredMgrQuery::FindResult findResult;
    do {
        // If prompts are allowed, keep rerunning the query (and showing a prompt) every time a
        // failed credential is returned.
        findResult = query.findOneItem(item, allowPrompts);
    } while (allowPrompts && findResult == CredMgrQuery::FoundFailedItem);

    AuthenticationChallengeResult result = AuthenticationChallengeCancelled;
    switch (findResult) {
    case CredMgrQuery::FoundNothing:
        result = AuthenticationChallengeCancelled;
        break;
    case CredMgrQuery::FoundMultipleItems:
        // Cancel this challenge so that it can be tried again with prompts. (This should not happen
        // if prompts are allowed, because a prompt would be used to narrow it down.)
        ASSERT(!allowPrompts);
        result = AuthenticationChallengeCancelled;
        break;
    case CredMgrQuery::FoundSuccessfulItem:
        result = AuthenticationChallengeSuccess;
        break;
    case CredMgrQuery::FoundFailedItem:
        ASSERT(!allowPrompts);
        // If this is the first time we're trying Negotiate with this host, treat this as a bad
        // password - return Cancelled so that the query will be run again with prompts allowed (to
        // let the user type a new one). Otherwise, disable Negotiate for this host.
        if (space.authenticationScheme() == ProtectionSpaceAuthenticationSchemeNegotiate && m_triedNegotiateSet.find(space) != m_triedNegotiateSet.end())
            result = AuthenticationChallengeRetryWithoutNegotiate;
        else
            result = AuthenticationChallengeCancelled;
        break;
    case CredMgrQuery::FoundMechNotAvail:
        // Disable Negotiate for this host.
        if (space.authenticationScheme() == ProtectionSpaceAuthenticationSchemeNegotiate)
            result = AuthenticationChallengeRetryWithoutNegotiate;
        else
            result = AuthenticationChallengeCancelled;
        break;
    case CredMgrQuery::FoundError:
        result = AuthenticationChallengeCancelled;
        break;
    }

    if (result == AuthenticationChallengeSuccess || result == AuthenticationChallengeRetryWithoutNegotiate) {
        BlackBerry::Platform::String handle;
        if (!getHandleFromCredMgrItem(item.ref(), handle))
            return AuthenticationChallengeCancelled;
        credential = handleToCredential(handle);
    }

    return result;
}

AuthenticationChallengeManagerPrivate::AuthenticationChallengeManagerPrivate()
    : m_activeChallenge(0)
    , m_currentCredentialStore(CREDMGR_STORE_DEFAULT)
    , m_privateCredentialStore(0)
    , m_privateMode(false)
{
}

AuthenticationChallengeManagerPrivate::~AuthenticationChallengeManagerPrivate()
{
    if (m_privateCredentialStore)
        credmgr_StoreClose(&m_privateCredentialStore);
}

bool AuthenticationChallengeManagerPrivate::resumeAuthenticationChallenge(PageClientBlackBerry* client)
{
    ASSERT(!m_activeChallenge);

    for (size_t i = 0; i < m_challenges.size(); ++i) {
        if (m_challenges[i]->pageClient == client && m_challenges[i]->blocked) {
            startAuthenticationChallenge(m_challenges[i].get());
            return true;
        }
    }

    return false;
}

void AuthenticationChallengeManagerPrivate::runAuthenticationPrompt(const GuardedPointer<ChallengeInfo>& infoPtr)
{
    ASSERT(CredMgrPromptThread::instance()->isCurrentThread());
    GuardedPointerLocker<ChallengeInfo> info(infoPtr);
    if (!info)
        return;

    // Run another CredMgr query, this time allowing prompts. (But do not open a prompt if we're not
    // allowed to save credentials, because CredMgr will automatically save credentials entered at
    // the prompt.)
    Credential credential;
    AuthenticationChallengeResult result = runAuthenticationQuery(info->query, shouldSaveCredentials(), info->space, credential);
    webKitThreadMessageClient()->dispatchMessage(createMethodCallMessage(&AuthenticationChallengeManagerPrivate::notifyChallengeResult, this, info->url, info->space, result, credential));
}

void AuthenticationChallengeManagerPrivate::startAuthenticationChallenge(ChallengeInfo* info)
{
    ASSERT(webKitThreadMessageClient()->isCurrentThread());
    m_activeChallenge = info;
    m_activeChallenge->blocked = false;

    BlackBerry::Platform::String handle;

    // First call pageClient->authenticationChallenge to give the app a chance to show its own prompt (for password credentials only).

    if (info->query.isPasswordQuery()) {
        Credential passwordCredential; // This will store a username/password, not a CredMgr handle
        switch (info->pageClient->authenticationChallenge(info->url, info->space, passwordCredential)) {
        case PageClientBlackBerry::AuthDialogCancelled:
            // No prompt, no query.
            notifyChallengeResult(info->url, info->space, AuthenticationChallengeCancelled, Credential());
            return;
        case PageClientBlackBerry::AuthDialogConfirmed:
            // Add the passwordCredential returned to CredMgr so we can use it.
            if (createAndSaveItem(info->pageClient, info->query, passwordCredential.user(), passwordCredential.password(), info->url.string(), &handle))
                notifyChallengeResult(info->url, info->space, AuthenticationChallengeSuccess, handleToCredential(handle));
            else
                notifyChallengeResult(info->url, info->space, AuthenticationChallengeCancelled, Credential());
            return;
        case PageClientBlackBerry::AuthDialogNotShown:
            // Do nothing; fall through to next step.
            break;
        }
    }

    // Run another CredMgr query, this time allowing prompts.
    CredMgrPromptThread::instance()->dispatchMessage(createMethodCallMessage(&AuthenticationChallengeManagerPrivate::runAuthenticationPrompt, this, info));
}

template<typename Query>
void showCredMgrSaveDialogForItem(const Query& query, CredMgrQuery::CreatedItemStatus status, credmgr_Item& item, const String& url, const credmgr_Store& store)
{
    ASSERT(CredMgrPromptThread::instance()->isCurrentThread());

    CredMgrObject<credmgr_Item> scopedItem(item);

    credmgr_DialogConfirmSaveAction action;
    ABORT_ON_ERROR(credmgr_DialogConfirmSaveCredentialForUrl, store, url.utf8().data(), &action);
    if (action == CREDMGR_DIALOG_CONFIRM_SAVE_ACTION_OK)
        query.saveItem(status, scopedItem.ref());
}

template<typename Query>
bool AuthenticationChallengeManagerPrivate::createAndSaveItem(PageClientBlackBerry* pageClient, const Query& query, const String& username, const String& password, const String& url, BlackBerry::Platform::String* handle)
{
    CredMgrObject<credmgr_Item> item;
    CredMgrQuery::CreatedItemStatus status = query.createItem(item, username, password, url);
    if (status == CredMgrQuery::ItemHasError)
        return false;

    if (handle) {
        if (!getHandleFromCredMgrItem(item.ref(), *handle)) {
            // Return immediately since there is no point in saving an item which is giving an error
            return false;
        }
    }

    if (status != CredMgrQuery::ItemIsUnchanged && shouldSaveCredentials()) {
        // Give the app a chance to open its own "Save Credentials" dialog.
        switch (pageClient->notifyShouldSaveCredential(status == CredMgrQuery::ItemIsNew)) {
        case PageClientBlackBerry::SaveCredentialDialogNotShown:
            // Detach the item so it can be passed to another thread and not released at end of
            // scope.
            item.setValid(false);

            // Show the credmgr save dialog since no user dialog was shown. We can continue to use
            // this credential while the user is saving it.
            CredMgrPromptThread::instance()->dispatchMessage(createFunctionCallMessage(&showCredMgrSaveDialogForItem<Query>, query, status, item.ref(), url, m_currentCredentialStore));
            break;
        case PageClientBlackBerry::SaveCredentialDialogNeverForThisSite:
        case PageClientBlackBerry::SaveCredentialDialogNotNow:
            // do nothing
            break;
        case PageClientBlackBerry::SaveCredentialDialogYes:
            query.saveItem(status, item.ref());
            break;
        }
    }

    return true;
}

bool AuthenticationChallengeManagerPrivate::pageExists(PageClientBlackBerry* client)
{
    return m_pageVisibilityMap.find(client) != m_pageVisibilityMap.end();
}

SINGLETON_INITIALIZER_THREADUNSAFE(AuthenticationChallengeManager)

AuthenticationChallengeManager::AuthenticationChallengeManager()
    : d(new AuthenticationChallengeManagerPrivate)
{
}

AuthenticationChallengeManager::~AuthenticationChallengeManager()
{
    deleteGuardedObject(d);
}

void AuthenticationChallengeManager::pageCreated(PageClientBlackBerry* client)
{
    d->m_pageVisibilityMap.add(client, false);
}

void AuthenticationChallengeManager::pageDeleted(PageClientBlackBerry* client)
{
    d->m_pageVisibilityMap.remove(client);

    if (d->m_activeChallenge && d->m_activeChallenge->pageClient == client)
        d->m_activeChallenge = 0;

    Vector<SharedGuardedPointer<ChallengeInfo> > existing;
    d->m_challenges.swap(existing);

    for (size_t i = 0; i < existing.size(); ++i) {
        if (existing[i]->pageClient != client)
            d->m_challenges.append(existing[i]);
    }
}

void AuthenticationChallengeManager::pageVisibilityChanged(PageClientBlackBerry* client, bool visible)
{
    PageVisibilityMap::iterator iter = d->m_pageVisibilityMap.find(client);

    ASSERT(iter != d->m_pageVisibilityMap.end());
    if (iter == d->m_pageVisibilityMap.end()) {
        d->m_pageVisibilityMap.add(client, visible);
        return;
    }

    if (iter->value == visible)
        return;

    iter->value = visible;
    if (!visible)
        return;

    if (d->m_activeChallenge)
        return;

    d->resumeAuthenticationChallenge(client);
}

void AuthenticationChallengeManager::authenticationChallenge(const KURL& url, const ProtectionSpace& space,
    AuthenticationChallengeClient* authClient, PageClientBlackBerry* pageClient, bool allowDialog)
{
    BLACKBERRY_ASSERT(authClient);
    BLACKBERRY_ASSERT(pageClient);

    CredMgrServerQuery query(space, d->m_currentCredentialStore);

    // Try it without allowing a prompt (will not block)
    Credential credential;
    AuthenticationChallengeResult result = d->runAuthenticationQuery(query, false, space, credential);
    if (result == AuthenticationChallengeSuccess || result == AuthenticationChallengeRetryWithoutNegotiate || !allowDialog) {
        // Handle this result immediately since no prompt dialog will be needed.
        d->notifyChallengeResultForClient(authClient, url, space, result, credential);
        return;
    }

    // Try again, this time allowing prompts. (But queue it to run later when the page needing the
    // prompt is guaranteed to be visible.)
    ChallengeInfo* info = new ChallengeInfo(url, space, query, authClient, pageClient);
    d->m_challenges.append(info);

    if (d->m_activeChallenge || !pageClient->isVisible()) {
        info->blocked = true;
        return;
    }

    d->startAuthenticationChallenge(info);
}

void AuthenticationChallengeManager::cancelAuthenticationChallenge(AuthenticationChallengeClient* client)
{
    BLACKBERRY_ASSERT(client);

    if (d->m_activeChallenge && d->m_activeChallenge->authClient == client)
        d->m_activeChallenge = 0;

    Vector<SharedGuardedPointer<ChallengeInfo> > existing;
    d->m_challenges.swap(existing);

    ChallengeInfo* next = 0;
    PageClientBlackBerry* page = 0;

    for (size_t i = 0; i < existing.size(); ++i) {
        if (existing[i]->authClient != client) {
            if (page && !next && existing[i]->pageClient == page)
                next = existing[i].get();
            d->m_challenges.append(existing[i]);
        } else if (d->m_activeChallenge == existing[i].get())
            page = existing[i]->pageClient;
    }

    if (next)
        d->startAuthenticationChallenge(next);
}

void AuthenticationChallengeManagerPrivate::notifyChallengeResult(const KURL&, const ProtectionSpace& space,
    AuthenticationChallengeResult result, const Credential& credential)
{
    m_activeChallenge = 0;

    Vector<SharedGuardedPointer<ChallengeInfo> > existing;
    m_challenges.swap(existing);

    ChallengeInfo* next = 0;
    PageClientBlackBerry* page = 0;

    for (size_t i = 0; i < existing.size(); ++i) {
        if (existing[i]->space != space) {
            if (page && !next && existing[i]->pageClient == page)
                next = existing[i].get();
            m_challenges.append(existing[i]);
        } else {
            page = existing[i]->pageClient;
            notifyChallengeResultForClient(existing[i]->authClient, existing[i]->url, space, result, credential);

            // After calling notifyChallengeResult(), page could be destroyed or something.
            if (!pageExists(page) || !page->isVisible())
                page = 0;
        }
    }

    if (next)
        startAuthenticationChallenge(next);
}

void AuthenticationChallengeManagerPrivate::notifyChallengeResultForClient(AuthenticationChallengeClient* client, const KURL& url, const ProtectionSpace& space,
    AuthenticationChallengeResult result, const Credential& credential)
{
    if (result == AuthenticationChallengeSuccess && space.authenticationScheme() == ProtectionSpaceAuthenticationSchemeNegotiate)
        m_triedNegotiateSet.insert(space);

    client->notifyChallengeResult(url, space, result, credential);
}

void AuthenticationChallengeManager::setPrivateMode(bool mode)
{
    if (d->m_privateMode == mode)
        return;

    d->m_privateMode = mode;

    if (d->m_privateMode) {
        // Open a new private credential store.
        ASSERT(!d->m_privateCredentialStore);
        if (d->m_privateCredentialStore)
            return;

        // It's safe to continue to use m_currentCredentialStore on failure because we will not
        // let the user save credentials, only look them up
        ABORT_ON_ERROR(credmgr_StoreCreateTemporary, CREDMGR_STORE_DEFAULT, &d->m_privateCredentialStore);
        d->m_currentCredentialStore = d->m_privateCredentialStore;
    } else {
        // Close the private credential store (discarding all credentials in it).
        d->m_currentCredentialStore = CREDMGR_STORE_DEFAULT;
        if (d->m_privateCredentialStore) {
            credmgr_StoreClose(&d->m_privateCredentialStore);
            d->m_privateCredentialStore = 0;
        }
    }
}

bool AuthenticationChallengeManagerPrivate::shouldSaveCredentials() const
{
    // We do not want to store any credentials if the temporary store failed to be created.
    return !m_privateMode || m_currentCredentialStore != CREDMGR_STORE_DEFAULT;
}

void AuthenticationChallengeManager::autofillPasswordForms(PageClientBlackBerry* pageClient, const HTMLCollection& forms)
{
    ASSERT(pageClient);

    // Forms can be filled even if the page isn't visible, because it doesn't involve showing any
    // dialogs.

    if (!pageClient->isFormAutofillEnabled()
        || !pageClient->isCredentialAutofillEnabled()
        || pageClient->isPrivateBrowsingEnabled())
        return;

    size_t sourceLength = forms.length();
    for (size_t i = 0; i < sourceLength; ++i) {
        Node* node = forms.item(i);
        if (node && node->isHTMLElement()) {
            CredentialTransformData data(static_cast<HTMLFormElement*>(node));
            if (!data.isValid())
                continue;

            CredMgrServerQuery query(data.protectionSpace(), d->m_currentCredentialStore);
            CredMgrObject<credmgr_Item> item;
            CredMgrQuery::FindResult result = query.findOneItem(item, /*allowPrompts*/ false);
            // If multiple items are found, just use the first, since there is no good UI
            // implemented to choose between them here. (We don't want to open a dialog when just
            // showing a form.)
            if (result != CredMgrQuery::FoundSuccessfulItem && result != CredMgrQuery::FoundMultipleItems)
                continue;

            BlackBerry::Platform::String username, password;
            if (!getCredentialsFromCredMgrItem(item.ref(), username, password))
                continue;

            Credential savedCredential = Credential(username, password, CredentialPersistencePermanent);
            data.setCredential(savedCredential);
        }
    }
}

void AuthenticationChallengeManager::saveCredentialIfConfirmed(PageClientBlackBerry* pageClient, const CredentialTransformData& data)
{
    ASSERT(pageClient);

    // FIXME: this should queue the dialog until the page is in the foreground.

    if (!pageClient->isFormAutofillEnabled()
        || !pageClient->isCredentialAutofillEnabled()
        || pageClient->isPrivateBrowsingEnabled())
        return;

    if (!data.isValid() || data.credential().isEmpty())
        return;

    CredMgrServerQuery query(data.protectionSpace(), d->m_currentCredentialStore);
    d->createAndSaveItem(pageClient, query, data.credential().user(), data.credential().password(), data.url().string());
}

void AuthenticationChallengeManager::clearAllCredentials()
{
    CredMgrObject<credmgr_AccessSubject> subject;
    ABORT_ON_ERROR(credmgr_AccessSubjectCreateForCurrentApp, subject.ptr());
    subject.setValid(true);

    if (d->m_currentCredentialStore != CREDMGR_STORE_DEFAULT)
        WARN_ON_ERROR(credmgr_ItemDeleteAllForAccessSubject, d->m_currentCredentialStore, subject.ref());

    WARN_ON_ERROR(credmgr_ItemDeleteAllForAccessSubject, CREDMGR_STORE_DEFAULT, subject.ref());
}

} // namespace WebCore
