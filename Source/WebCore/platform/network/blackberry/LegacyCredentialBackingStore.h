/*
 * Copyright (C) 2011, 2012, 2013 BlackBerry Limited. All rights reserved.
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

#ifndef LegacyCredentialBackingStore_h
#define LegacyCredentialBackingStore_h

#if ENABLE(BLACKBERRY_CREDENTIAL_PERSIST)

#include "Credential.h"
#include "SQLiteDatabase.h"

#include <BlackBerryPlatformMisc.h>
#include <BlackBerryPlatformSingleton.h>
#include <wtf/OwnPtr.h>

namespace WebCore {

class ProtectionSpace;

class LegacyCredentialBackingStore : public BlackBerry::Platform::Singleton<LegacyCredentialBackingStore> {
public:
    bool migrateCredentials();
    void deleteCredentials();

private:
    LegacyCredentialBackingStore();

    bool migrateLoginsTable(unsigned version);

    String decryptedString(const String& cipherText) const;

    unsigned existingDatabaseVersion();

    SQLiteDatabase m_database;

    SINGLETON_DEFINITION_THREADSAFE(LegacyCredentialBackingStore);
};

} // namespace WebCore

#endif // ENABLE(BLACKBERRY_CREDENTIAL_PERSIST)

#endif // LegacyCredentialBackingStore_h
