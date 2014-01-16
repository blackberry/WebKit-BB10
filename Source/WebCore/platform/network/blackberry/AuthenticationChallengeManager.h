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

#ifndef AuthenticationChallengeManager_h
#define AuthenticationChallengeManager_h

#include <BlackBerryPlatformSingleton.h>

class PageClientBlackBerry;

namespace WebCore {

class AuthenticationChallengeManagerPrivate;
class Credential;
class CredentialTransformData;
class HTMLCollection;
class KURL;
class ProtectionSpace;

enum AuthenticationChallengeResult {
    AuthenticationChallengeSuccess,
    AuthenticationChallengeCancelled,
    AuthenticationChallengeRetryWithoutNegotiate,
};

class AuthenticationChallengeClient {
public:
    virtual void notifyChallengeResult(const KURL&, const ProtectionSpace&, AuthenticationChallengeResult, const Credential&) = 0;
};

class AuthenticationChallengeManager : public BlackBerry::Platform::ThreadUnsafeSingleton<AuthenticationChallengeManager> {
    SINGLETON_DEFINITION_THREADUNSAFE(AuthenticationChallengeManager)
public:
    void pageCreated(PageClientBlackBerry*);
    void pageDeleted(PageClientBlackBerry*);
    void pageVisibilityChanged(PageClientBlackBerry*, bool visible);

    void authenticationChallenge(const KURL&, const ProtectionSpace&, AuthenticationChallengeClient*, PageClientBlackBerry*, bool allowDialog);
    void cancelAuthenticationChallenge(AuthenticationChallengeClient*);

    void setPrivateMode(bool);

    void autofillPasswordForms(PageClientBlackBerry*, const HTMLCollection& forms);
    void saveCredentialIfConfirmed(PageClientBlackBerry*, const CredentialTransformData&);

    void clearAllCredentials();

private:
    AuthenticationChallengeManager();
    ~AuthenticationChallengeManager();

    AuthenticationChallengeManagerPrivate* d;
};

} // namespace WebCore

#endif // AuthenticationChallengeManager_h
