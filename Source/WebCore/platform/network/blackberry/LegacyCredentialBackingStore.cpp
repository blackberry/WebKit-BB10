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

#include "config.h"
#include "LegacyCredentialBackingStore.h"

#if ENABLE(BLACKBERRY_CREDENTIAL_PERSIST)

#include "CredMgrUtils.h"
#include "CredentialStorage.h"
#include "FileSystem.h"
#include "KURL.h"
#include "ProtectionSpaceHash.h"
#include "SQLiteStatement.h"
#include "SQLiteTransaction.h"

#include <BlackBerryPlatformEncryptor.h>
#include <BlackBerryPlatformLog.h>
#include <BlackBerryPlatformSettings.h>
#include <CertMgrWrapper.h>
#include <unistd.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/text/Base64.h>
#include <wtf/text/StringBuilder.h>

using BlackBerry::Platform::logAlways;
using BlackBerry::Platform::LogLevelCritical;
using BlackBerry::Platform::LogLevelInfo;
using BlackBerry::Platform::LogLevelWarn;

#define HANDLE_SQL_EXEC_FAILURE(statement, returnValue, ...) \
    if (statement) { \
        logAlways(LogLevelWarn, __VA_ARGS__); \
        return returnValue; \
    }

namespace WebCore {

SINGLETON_INITIALIZER_THREADSAFE(LegacyCredentialBackingStore);

static unsigned hashCredentialInfoV1(const String& url, const ProtectionSpace& space, const String& username)
{
    String hashString = String::format("%s@%s@%s@%d@%d@%s@%d",
        username.utf8().data(), url.utf8().data(),
        space.host().utf8().data(), space.port(),
        static_cast<int>(space.serverType()),
        space.realm().utf8().data(),
        static_cast<int>(space.authenticationScheme()));
    return StringHasher::computeHashAndMaskTop8Bits(hashString.characters(), hashString.length());
}

static unsigned hashCredentialInfo(const ProtectionSpace& space, const String& username)
{
    String hashString = String::format("%s@%s@%d@%d@%s@%d",
        username.utf8().data(),
        space.host().utf8().data(), space.port(),
        static_cast<int>(space.serverType()),
        space.realm().utf8().data(),
        static_cast<int>(space.authenticationScheme()));
    return StringHasher::computeHashAndMaskTop8Bits(hashString.characters(), hashString.length());
}

static String databasePath()
{
    static String path = pathByAppendingComponent(BlackBerry::Platform::Settings::instance()->applicationDataDirectory().c_str(), "/credentials.db");
    return path;
}

static String archivedDatabasePath()
{
    static String path = pathByAppendingComponent(BlackBerry::Platform::Settings::instance()->applicationDataDirectory().c_str(), "/credentials.db.archived");
    return path;
}

static void deleteDBFile(const String& path)
{
    if (!::unlink(path.utf8().data()))
        logAlways(LogLevelInfo, "Deleted legacy credential db at %s", path.utf8().data());
    else if (errno == ENOENT)
        logAlways(LogLevelInfo, "Legacy credential db does not exist at %s", path.utf8().data());
    else
        logAlways(LogLevelWarn, "Error deleting legacy credential db at %s: %d (%s)", path.utf8().data(), errno, strerror(errno));
}

LegacyCredentialBackingStore::LegacyCredentialBackingStore()
{
}

bool LegacyCredentialBackingStore::migrateCredentials()
{
    ASSERT(!m_database.isOpen());

    String dbPath = databasePath();
    errno = 0;
    if (::access(dbPath.utf8().data(), R_OK)) {
        logAlways(LogLevelInfo, "Not migrating credentials because legacy credential db %s does not exist", dbPath.utf8().data());
        return true;
    }

    HANDLE_SQL_EXEC_FAILURE(!m_database.open(dbPath), false,
        "Failed to open database file %s for legacy login database", dbPath.utf8().data());

    bool success;
    if (!m_database.tableExists("logins")) {
        logAlways(LogLevelInfo, "Legacy credentials db at %s is empty - nothing to migrate", dbPath.utf8().data());
        success = true;
    } else
        success = migrateLoginsTable(existingDatabaseVersion());

    m_database.close();

    if (success) {
        String backupPath = archivedDatabasePath();
        if (!::rename(dbPath.utf8().data(), backupPath.utf8().data()))
            logAlways(LogLevelInfo, "Archived legacy credentials db to %s", backupPath.utf8().data());
        else
            logAlways(LogLevelWarn, "Unable to archive legacy credentials db to %s: %d (%s)", backupPath.utf8().data(), errno, strerror(errno));
    }

    return success;
}

void LegacyCredentialBackingStore::deleteCredentials()
{
    deleteDBFile(databasePath());
    deleteDBFile(archivedDatabasePath());
}

unsigned LegacyCredentialBackingStore::existingDatabaseVersion()
{
    ASSERT(m_database.isOpen());
    ASSERT(m_database.tableExists("logins"));

    // For SQLite, pragma table_info should return data like following.
    //
    // sqlite> pragma table_info("cookies");
    // cid         name        type        notnull     dflt_value  pk
    // ----------  ----------  ----------  ----------  ----------  ----------
    // 0           name        TEXT        0                       1
    // 1           value       TEXT        0                       0

    SQLiteStatement tableinfo(m_database, "PRAGMA table_info('logins');");
    int result = tableinfo.step();
    ASSERT(SQLResultOk == result);
    if (SQLResultOk != result) {
        logAlways(LogLevelWarn, "LegacyCredentialBackingStore error reading table_info(logins): %d.", result);
        return 0;
    }

    String name = tableinfo.getColumnText(1);
    if (name == "origin_url")
        return 1;

    return 2;
}

bool LegacyCredentialBackingStore::migrateLoginsTable(unsigned version)
{
    ASSERT(m_database.isOpen());
    ASSERT(m_database.tableExists("logins"));

    // V1: origin_url, host, port, service_type, realm, auth_scheme, username, password
    // V2: host, port, service_type, realm, auth_scheme, username, password

    int hostOffset;
    switch (version) {
    case 1:
        hostOffset = 1;
        break;
    case 2:
        hostOffset = 0;
        break;
    default:
        logAlways(LogLevelCritical, "Unrecognized LegacyCredentialBackingStore version %u: not migrating credentials", version);
        return false;
    }

    SQLiteStatement numlogins(m_database, "SELECT COUNT(*) from logins;");
    if (numlogins.prepareAndStep() != SQLResultRow) {
        logAlways(LogLevelWarn, "Unable to count number of legacy credentials to migrate; not migrating any");
        return false;
    }
    int totalLogins = numlogins.getColumnInt(0);
    if (!totalLogins) {
        logAlways(LogLevelCritical, "LegacyCredentialBackingStore is empty: not migrating credentials");
        return true;
    }

    SQLiteStatement alllogins(m_database, "SELECT * from logins;");

    int result;
    int currentLogin = 0;
    while ((result = alllogins.step()) == SQLResultOk || result == SQLResultRow) {
        currentLogin++;

        String host = alllogins.getColumnText(hostOffset);

        // Sometimes SQLite will return empty values. I've only seen this happen if the COUNT
        // statement above returned 0 - I don't know why it's not returning SQLResultDone
        // immediately in that case. But just in case it can happen in another case, skip it.
        if (host.isEmpty()) {
            logAlways(LogLevelInfo, "Migrated credential %d of %d had an empty host - skipping", currentLogin, totalLogins);
            continue;
        }

        int port = alllogins.getColumnInt(hostOffset + 1);
        int serviceType = alllogins.getColumnInt(hostOffset + 2);
        String realm = alllogins.getColumnText(hostOffset + 3);
        int authScheme = alllogins.getColumnInt(hostOffset + 4);

        ProtectionSpace space(host, port, static_cast<ProtectionSpaceServerType>(serviceType), realm, static_cast<ProtectionSpaceAuthenticationScheme>(authScheme));

        String username = alllogins.getColumnText(hostOffset + 5);

        String url;

        String encryptedPassword;
        BlackBerry::Platform::CertMgrWrapper certMgr;
        if (!certMgr.isReady())
            encryptedPassword = alllogins.getColumnBlobAsString(hostOffset + 6);
        else {
            unsigned hash;
            if (version == 1) {
                url = alllogins.getColumnText(0);
                hash = hashCredentialInfoV1(url, space, username);
            } else
                hash = hashCredentialInfo(space, username);

            std::string passwordBlob;
            if (certMgr.getPassword(hash, passwordBlob))
                encryptedPassword = String(passwordBlob.data(), passwordBlob.length());
        }

        String password = decryptedString(encryptedPassword);

        if (url.isEmpty()) {
            // In a V1 table, the url will already be read. In a V2 table there is no url, so just
            // use the root url for the host.
            StringBuilder urlBuilder;
            switch (space.serverType()) {
            case ProtectionSpaceServerHTTP:
            case ProtectionSpaceProxyHTTP:
                urlBuilder.append("http://");
                break;
            case ProtectionSpaceServerHTTPS:
            case ProtectionSpaceProxyHTTPS:
                urlBuilder.append("https://");
                break;
            case ProtectionSpaceServerFTP:
            case ProtectionSpaceProxyFTP:
                urlBuilder.append("ftp://");
                break;
            case ProtectionSpaceServerFTPS:
                urlBuilder.append("ftps://");
                break;
            case ProtectionSpaceProxySOCKS:
                // SOCKS proxies not supported by credmgr
                logAlways(LogLevelInfo, "Migrated credential %d of %d is for a SOCKS proxy, which is not supported by CredMgr - skipping", currentLogin, totalLogins);
                continue;
            }

            urlBuilder.append(space.host());
            if (space.port())
                urlBuilder.append(space.port());

            url = urlBuilder.toString();
        }

        CredMgrServerQuery query(space, CREDMGR_STORE_DEFAULT);

        // Check if a credential already exists in CredMgr for this item
        CredMgrObject<credmgr_Item> item;
        switch (query.findOneItem(item, /*allowPrompts*/ false)) {
        case CredMgrQuery::FoundError:
            logAlways(LogLevelWarn, "Error searching CredMgr for migrated credential %d of %d - skipping", currentLogin, totalLogins);
            continue;
        case CredMgrQuery::FoundSuccessfulItem:
        case CredMgrQuery::FoundMultipleItems:
            logAlways(LogLevelInfo, "Migrated credential %d of %d already exists in CredMgr - not overwriting", currentLogin, totalLogins);
            continue;
        default:
            // Do nothing
            break;
        }

        CredMgrQuery::CreatedItemStatus status = query.createItem(item, username, password, url);
        switch (status) {
        case CredMgrQuery::ItemIsUnchanged:
            logAlways(LogLevelInfo, "Migrated credential %d of %d is unchanged in CredMgr", currentLogin, totalLogins);
            continue;
        case CredMgrQuery::ItemHasError:
            logAlways(LogLevelWarn, "Migrated credential %d of %d could not be added to CredMgr - skipping", currentLogin, totalLogins);
            continue;
        default:
            query.saveItem(status, item.ref());
            logAlways(LogLevelInfo, "Successfully migrated credential %d of %d to CredMgr", currentLogin, totalLogins);
            break;
        }
    }

    if (result != SQLResultDone) {
        logAlways(LogLevelCritical, "Error reading from LegacyCredentialBackingStore after migrating %d of %d credentials", currentLogin, totalLogins);
        return false;
    }

    logAlways(LogLevelCritical, "Migrated %d credentials from LegacyCredentialBackingStore", currentLogin);
    return true;
}

String LegacyCredentialBackingStore::decryptedString(const String& cipherText) const
{
    if (cipherText.isEmpty())
        return emptyString();

    std::string text = cipherText.is8Bit() ?
        std::string(reinterpret_cast<const char*>(cipherText.characters8()), cipherText.length() * sizeof(LChar)) :
        std::string(reinterpret_cast<const char*>(cipherText.characters16()), cipherText.length() * sizeof(UChar));

    std::string plainText;
    BlackBerry::Platform::Encryptor::decryptString(text, &plainText);

    return String::fromUTF8(plainText.data(), plainText.length());
}

} // namespace WebCore

#endif // ENABLE(BLACKBERRY_CREDENTIAL_PERSIST)
