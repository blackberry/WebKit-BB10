/*
 * Copyright (C) 2009, 2010, 2011, 2013 BlackBerry Limited. All rights reserved.
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

#ifndef AuthenticationChallenge_h
#define AuthenticationChallenge_h

#include "AuthenticationChallengeBase.h"

namespace WebCore {

class AuthenticationChallenge : public AuthenticationChallengeBase {
public:
    AuthenticationChallenge()
        : m_shouldAllowNegotiate(true)
    {
    }

    AuthenticationChallenge(const ProtectionSpace& protectionSpace, const Credential& proposedCredential, unsigned previousFailureCount, const ResourceResponse& response, const ResourceError& error, bool shouldAllowNegotiate)
        : AuthenticationChallengeBase(protectionSpace, proposedCredential, previousFailureCount, response, error)
        , m_shouldAllowNegotiate(shouldAllowNegotiate)
    {
    }

    void setShouldAllowNegotiate(bool shouldAllowNegotiate) { m_shouldAllowNegotiate = shouldAllowNegotiate; }
    bool shouldAllowNegotiate() const { return m_shouldAllowNegotiate; }

    bool hasCredentials() const
    {
        if (isNull())
            return false;
        return !proposedCredential().isEmpty();
    }

private:
    friend class AuthenticationChallengeBase;

    static bool platformCompare(const AuthenticationChallenge& a, const AuthenticationChallenge& b)
    {
        return a.m_shouldAllowNegotiate == b.m_shouldAllowNegotiate;
    }

    bool m_shouldAllowNegotiate;
};

} // namespace WebCore

#endif // AuthenticationChallenge_h
