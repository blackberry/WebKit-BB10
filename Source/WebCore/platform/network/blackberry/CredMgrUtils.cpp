/*
 * Copyright (C) 2013 BlackBerry Limited. All rights reserved.
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
#include "CredMgrUtils.h"

#include "ProtectionSpace.h"

#include <BlackBerryPlatformLog.h>
#include <sys/credmgr_auth_result.h>
#include <sys/credmgr_clientcert.h>
#include <sys/credmgr_dialog.h>
#include <sys/credmgr_pwd.h>
#include <sys/credmgr_return.h>

using BlackBerry::Platform::logAlways;
using BlackBerry::Platform::LogLevelCritical;
using BlackBerry::Platform::LogLevelWarn;

// ProtectionSpace.h includes the WTF String class, so we can't also include the platform String
// class without a namespace. But at least we can typedef it to a shorter name to save typing.
typedef BlackBerry::Platform::String BBString;

namespace WebCore {

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

static bool itemIsPasswordCredential(const credmgr_Item& item)
{
    credmgr_ItemType type;
    ABORT_ON_ERROR_RET(false, credmgr_ItemGetType, item, &type);
    return type == CREDMGR_ITEM_TYPE_PASSWORD_CRED;
}

CredMgrQuery::FindResult CredMgrQuery::findOneItem(CredMgrObject<credmgr_Item>& resultItem, bool allowPrompts) const
{
    credmgr_UserPromptPolicyFlags promptPolicy = CREDMGR_USER_PROMPT_POLICY_DISALLOW_ALL;
    if (allowPrompts)
        promptPolicy = CREDMGR_USER_PROMPT_POLICY_ALLOW_SOLICIT | CREDMGR_USER_PROMPT_POLICY_ALLOW_VERIFY | CREDMGR_USER_PROMPT_POLICY_ALLOW_RESOLVE;

    CredMgrObject<credmgr_ItemQuery> query;
    if (!createQuery(query))
        return FoundError;

    CredMgrObject<credmgr_ItemQuery>* trueQuery = &query;

    CredMgrObject<credmgr_ItemQuery> certQuery;
    if (!isPasswordQuery()) {
        // For certificate queries, a base query must be wrapped with a cert query for some reason.
        // Not sure why this detail isn't hidden behind the API.
        ABORT_ON_ERROR_RET(FoundError, credmgr_ClientCertQueryCreate, certQuery.ptr());
        certQuery.setValid(true);
        ABORT_ON_ERROR_RET(FoundError, credmgr_ClientCertQuerySetAuthDomainQuery, certQuery.ref(), query.ref());

        trueQuery = &certQuery;
    }

    ABORT_ON_ERROR_RET(FoundError, credmgr_ItemQuerySetUserPromptPolicy, trueQuery->ref(), promptPolicy);

    CredMgrObject<credmgr_ItemQueryResult> result;
    ABORT_ON_ERROR_RET(FoundError, credmgr_ItemFind, m_store, trueQuery->ref(), result.ptr());
    result.setValid(true);

    if (credmgr_ItemQueryResultGetStatus(result.ref()) != CREDMGR_ITEM_QUERY_STATUS_COMPLETED)
        return FoundNothing;

    CredMgrObject<credmgr_ItemIterator> iterator;
    ABORT_ON_ERROR_RET(FoundError, credmgr_ItemQueryResultGetItemIterator, result.ref(), iterator.ptr());
    iterator.setValid(true);

    // If we find no credentials with a more specific result code, we will return FoundNothing.
    FindResult findResult = FoundNothing;

    // We will return the first successful item found. If there are none, we return the first item
    // with MechNotAvail, and if there are none of those, we return the first failed item.
    //
    // Since the item goes out of scope each time we call credmgr_ItemIteratorHasNext, we must clone
    // each item that may be returned into resultItem (replacing the previous clone whenever a
    // higher priority item is found).
    //
    // We can stop iterating as soon as we find more than one successful item FoundMultipleItems is
    // the highest priority return code, so it will not be replaced.
    while (findResult != FoundMultipleItems && credmgr_ItemIteratorHasNext(iterator.ref())) {
        credmgr_Item item;

        ABORT_ON_ERROR_RET(FoundError, credmgr_ItemIteratorNext, iterator.ref(), &item);

        credmgr_AuthResultType authResult;
        ABORT_ON_ERROR_RET(FoundError, credmgr_ItemGetAuthResult, item, &authResult);

        bool needClone = false;
        FindResult newFindResult = findResult;

        // Should have returned immediately on FoundError or FoundMultipleItems.
        BLACKBERRY_ASSERT(findResult != FoundError && findResult != FoundMultipleItems);

        switch (authResult) {
        case CREDMGR_AUTH_RESULT_SUCCESS:
            // Only clone the first successful item found.
            if (findResult != FoundSuccessfulItem) {
                needClone = true;
                newFindResult = FoundSuccessfulItem;
            } else {
                newFindResult = FoundMultipleItems;
            }
            break;
        case CREDMGR_AUTH_RESULT_FAIL:
            // Found at least one failed credential; unless we find a successful or MechNotAvail
            // credential, we will return FoundFailedItem.
            if (findResult != FoundSuccessfulItem && findResult != FoundMechNotAvail) {
                // Only clone the first failed item found.
                if (findResult != FoundFailedItem)
                    needClone = true;
                newFindResult = FoundFailedItem;
            }
            break;
        case CREDMGR_AUTH_RESULT_AUTH_MECH_NOT_AVAIL:
            // Found at least one credential with MechNotAvail; unless we find a successful
            // credential, we will return FoundMechNotAvail.
            if (findResult != FoundSuccessfulItem) {
                // Only clone the first mech not avail item found.
                if (findResult != FoundMechNotAvail)
                    needClone = true;
                newFindResult = FoundMechNotAvail;
            }
            break;
        }

        if (needClone) {
            // FIXME: In theory we should be able to use empty credentials, but right now our network layer
            // won't handle them. So skip them for now.
            if (isPasswordQuery()) {
                BLACKBERRY_ASSERT(itemIsPasswordCredential(item));
                BBString username, password;
                if (!getCredentialsFromCredMgrItem(item, username, password))
                    return FoundError;
                if (username.empty() || password.empty()) {
                    // skip this item; do not update findResult
                    continue;
                }
            }

            resultItem.release();
            ABORT_ON_ERROR_RET(FoundError, credmgr_ItemClone, item, resultItem.ptr());
            resultItem.setValid(true);
        }

        findResult = newFindResult;
    }

    return findResult;
}

CredMgrQuery::CreatedItemStatus CredMgrQuery::createItem(CredMgrObject<credmgr_Item>& returnItem, const BBString& username, const BBString& password, const BBString& url) const
{
    BLACKBERRY_ASSERT(isPasswordQuery());

    // Query to see if an item exists.
    FindResult result = findOneItem(returnItem, /*allowPrompts*/ false);
    if (result == FoundSuccessfulItem || result == FoundMultipleItems) {
        BLACKBERRY_ASSERT(itemIsPasswordCredential(returnItem.ref()));
        // Update the item's username and password.
        // FIXME: this only updates the first item. We should update all of them.
        BBString existingUsername, existingPassword;
        if (!getCredentialsFromCredMgrItem(returnItem.ref(), existingUsername, existingPassword))
            return ItemHasError;

        CreatedItemStatus status = ItemIsUnchanged;

        if (username != existingUsername) {
            status = ItemIsUpdated;
            ABORT_ON_ERROR_RET(ItemHasError, credmgr_PasswordCredentialSetAccountName, returnItem.ref(), username.utf8().c_str());
        }
        if (password != existingPassword) {
            status = ItemIsUpdated;
            ABORT_ON_ERROR_RET(ItemHasError, credmgr_PasswordCredentialSetPassword, returnItem.ref(), password.utf8().c_str());
        }

        return status;
    }

    if (result == FoundError)
        return ItemHasError;

    // Nothing found. Create a new item.
    returnItem.release();
    ABORT_ON_ERROR_RET(ItemHasError, credmgr_PasswordCredentialCreateAndSave, url.utf8().c_str(), username.utf8().c_str(), password.utf8().c_str(), m_store, returnItem.ptr());
    returnItem.setValid(true);

    return ItemIsNew;
}

void CredMgrQuery::saveItem(CreatedItemStatus status, const credmgr_Item& item) const
{
    BLACKBERRY_ASSERT(isPasswordQuery());
    BLACKBERRY_ASSERT(itemIsPasswordCredential(item));

    switch (status) {
    case ItemHasError:
    case ItemIsUnchanged:
        // Nothing to do.
        return;
    case ItemIsUpdated:
        WARN_ON_ERROR(credmgr_PasswordCredentialUpdate, item);
        return;
    case ItemIsNew:
        {
            if (!bindItemToDomain(item))
                WARN_ON_ERROR(credmgr_ItemDelete, item);
        }
        return;
    }

    ASSERT_NOT_REACHED();
}

CredMgrQuery::CreatedItemStatus CredMgrUrlQuery::createItem(CredMgrObject<credmgr_Item>& returnItem, const BBString& username, const BBString& password) const
{
    return CredMgrQuery::createItem(returnItem, username, password, m_url);
}

CredMgrQuery::CreatedItemStatus CredMgrUrlQuery::createItem(CredMgrObject<credmgr_Item>& returnItem, const BBString& username, const BBString& password, const BBString& url) const
{
    ASSERT(url == m_url);
    return CredMgrQuery::createItem(returnItem, username, password, url);
}

bool CredMgrUrlQuery::createQuery(CredMgrObject<credmgr_ItemQuery>& query) const
{
    query.release();
    ABORT_ON_ERROR_RET(false,
                       credmgr_NetworkItemQueryCreateFromUrl,
                       m_url.c_str(),
                       m_hostType,
                       m_authScheme,
                       isPasswordQuery() ? CREDMGR_ITEM_TYPE_PASSWORD_CRED : CREDMGR_ITEM_TYPE_CLIENT_CERT,
                       query.ptr());
    query.setValid(true);
    return true;
}

bool CredMgrUrlQuery::bindItemToDomain(const credmgr_Item& item) const
{
    CredMgrObject<credmgr_NetworkAuthDomain> authDomain;
    ABORT_ON_ERROR_RET(false,
                       credmgr_NetworkAuthDomainCreateFromUrlAndSave,
                       m_url.c_str(),
                       m_hostType,
                       m_authScheme,
                       item,
                       authDomain.ptr());
    authDomain.setValid(true);
    return true;
}

static credmgr_NetworkProtocol protocolFromProtectionSpace(const ProtectionSpace& space)
{
    switch (space.serverType()) {
    case ProtectionSpaceServerHTTP:
    case ProtectionSpaceProxyHTTP:
        return CREDMGR_NETWORK_PROTOCOL_HTTP;
    case ProtectionSpaceServerHTTPS:
    case ProtectionSpaceProxyHTTPS:
        return CREDMGR_NETWORK_PROTOCOL_HTTPS;
    case ProtectionSpaceServerFTP:
    case ProtectionSpaceProxyFTP:
        return CREDMGR_NETWORK_PROTOCOL_FTP;
    case ProtectionSpaceServerFTPS:
        return CREDMGR_NETWORK_PROTOCOL_FTPS;
    default:
        ASSERT_NOT_REACHED();
        return CREDMGR_NETWORK_PROTOCOL_ANY;
    }
}

static credmgr_NetworkAuthScheme authSchemeFromProtectionSpace(const ProtectionSpace& space)
{
    switch (space.authenticationScheme()) {
    case ProtectionSpaceAuthenticationSchemeDefault:
        return CREDMGR_NETWORK_AUTH_SCHEME_ANY;
    case ProtectionSpaceAuthenticationSchemeHTTPBasic:
        return CREDMGR_NETWORK_AUTH_SCHEME_BASIC;
    case ProtectionSpaceAuthenticationSchemeHTTPDigest:
        return CREDMGR_NETWORK_AUTH_SCHEME_DIGEST;
    case ProtectionSpaceAuthenticationSchemeNTLM:
        return CREDMGR_NETWORK_AUTH_SCHEME_NTLM;
    case ProtectionSpaceAuthenticationSchemeNegotiate:
        return CREDMGR_NETWORK_AUTH_SCHEME_NEGOTIATE;
    case ProtectionSpaceAuthenticationSchemeHTMLForm:
        return CREDMGR_NETWORK_AUTH_SCHEME_HTML_FORM;
    case ProtectionSpaceAuthenticationSchemeClientCertificateRequested:
        return CREDMGR_NETWORK_AUTH_SCHEME_CLIENT_CERT;
    default:
        ASSERT_NOT_REACHED();
        return CREDMGR_NETWORK_AUTH_SCHEME_ANY;
    }
}

CredMgrServerQuery::CredMgrServerQuery(const ProtectionSpace& space, const credmgr_Store& store, const BBString& resourcePath)
    : CredMgrQuery(space.isProxy() ? CREDMGR_NETWORK_HOST_PROXY : CREDMGR_NETWORK_HOST_SERVER, authSchemeFromProtectionSpace(space), store)
    , m_protocol(protocolFromProtectionSpace(space))
    , m_serverName(space.host())
    , m_serverPort(space.port() ? space.port() : CREDMGR_NETWORK_PORT_DEFAULT)
    , m_resourcePath(resourcePath.utf8())
{
}

bool CredMgrServerQuery::createQuery(CredMgrObject<credmgr_ItemQuery>& query) const
{
    query.release();
    ABORT_ON_ERROR_RET(false,
                       credmgr_NetworkItemQueryCreate,
                       m_protocol,
                       m_serverName.c_str(),
                       m_serverPort,
                       m_hostType,
                       m_authScheme,
                       isPasswordQuery() ? CREDMGR_ITEM_TYPE_PASSWORD_CRED : CREDMGR_ITEM_TYPE_CLIENT_CERT,
                       query.ptr());
    query.setValid(true);
    if (!m_resourcePath.empty())
        ABORT_ON_ERROR_RET(false, credmgr_NetworkItemQuerySetResourcePath, query.ref(), m_resourcePath.c_str());
    return true;
}

bool CredMgrServerQuery::bindItemToDomain(const credmgr_Item& item) const
{
    CredMgrObject<credmgr_NetworkAuthDomain> authDomain;
    ABORT_ON_ERROR_RET(false,
                       credmgr_NetworkAuthDomainCreateAndSave,
                       m_protocol,
                       m_serverName.c_str(),
                       m_serverPort,
                       m_resourcePath.empty() ? 0 : m_resourcePath.c_str(),
                       m_hostType,
                       m_authScheme,
                       item,
                       authDomain.ptr());
    authDomain.setValid(true);
    return true;
}

bool getCredentialsFromCredMgrHandle(const BBString& handle, BBString& username, BBString& password)
{
    if (handle.empty())
        return false;

    CredMgrObject<credmgr_Item> item;
    ABORT_ON_ERROR_RET(false, credmgr_ItemFromHandleString, handle.utf8().c_str(), item.ptr());
    item.setValid(true);

    return getCredentialsFromCredMgrItem(item.ref(), username, password);
}

bool getCredentialsFromCredMgrItem(const credmgr_Item& item, BBString& username, BBString& password)
{
    BLACKBERRY_ASSERT(itemIsPasswordCredential(item));

    char* buffer = 0;
    ABORT_ON_ERROR_RET(false, credmgr_PasswordCredentialGetAccountName, item, &buffer);
    ASSERT(buffer);
    username = BBString::fromUtf8(buffer);
    free(buffer);

    buffer = 0;
    ABORT_ON_ERROR_RET(false, credmgr_PasswordCredentialGetPassword, item, &buffer);
    ASSERT(buffer);
    password = BBString::fromUtf8(buffer);
    free(buffer);

    return true;
}

bool getHandleFromCredMgrItem(const credmgr_Item& item, BBString& handle)
{
    char* buffer = 0;
    ABORT_ON_ERROR_RET(false, credmgr_ItemToHandleString, item, &buffer);
    handle = BBString::fromUtf8(buffer);
    free(buffer);

    return true;
}

} // namespace WebCore
