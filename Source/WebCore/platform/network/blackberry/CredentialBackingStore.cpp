/*
 * Copyright (C) 2011 Research In Motion Limited. All rights reserved.
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
#include "CredentialBackingStore.h"

#if ENABLE(BLACKBERRY_CREDENTIAL_PERSIST)
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

#include <wtf/PassOwnPtr.h>
#include <wtf/text/Base64.h>

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

CredentialBackingStore& credentialBackingStore()
{
    DEFINE_STATIC_LOCAL(CredentialBackingStore, backingStore, ());
    if (!backingStore.m_database.isOpen())
        backingStore.open(pathByAppendingComponent(BlackBerry::Platform::Settings::instance()->applicationDataDirectory().c_str(), "/credentials.db"));
    return backingStore;
}

CredentialBackingStore::CredentialBackingStore()
{
}

CredentialBackingStore::~CredentialBackingStore()
{
    if (m_database.isOpen())
        m_database.close();
}

static bool createLoginsTables(SQLiteDatabase& database)
{
    ASSERT(database.isOpen());

    if (!database.executeCommand("CREATE TABLE logins (host VARCHAR NOT NULL, port INTEGER, service_type INTEGER NOT NULL, realm VARCHAR, auth_scheme INTEGER NOT NULL, username VARCHAR, password BLOB)")) {
        logAlways(LogLevelWarn, "CredentialBackingStore failed to create logins: %d.", database.lastError());
        return false;
    }

    if (!database.executeCommand("CREATE INDEX logins_index ON logins (host)")) {
        logAlways(LogLevelWarn, "CredentialBackingStore failed to create logins_index: %d.", database.lastError());
        return false;
    }

    return true;
}

static bool deleteLoginsTables(SQLiteDatabase& database)
{
    ASSERT(database.isOpen());

    if (!database.executeCommand("DROP INDEX logins_index")) {
        logAlways(LogLevelWarn, "CredentialBackingStore failed to drop logins_index: %d.", database.lastError());
        return false;
    }

    if (!database.executeCommand("DROP TABLE logins")) {
        logAlways(LogLevelWarn, "CredentialBackingStore failed to drop logins: %d.", database.lastError());
        return false;
    }

    return true;
}

static bool createNeverRememberTables(SQLiteDatabase& database)
{
    ASSERT(database.isOpen());

    if (!database.executeCommand("CREATE TABLE never_remember (host VARCHAR NOT NULL, port INTEGER, service_type INTEGER NOT NULL, realm VARCHAR, auth_scheme INTEGER NOT NULL)")) {
        logAlways(LogLevelWarn, "CredentialBackingStore failed to create never_remember: %d.", database.lastError());
        return false;
    }

    if (!database.executeCommand("CREATE INDEX never_remember_index ON never_remember (host)")) {
        logAlways(LogLevelWarn, "CredentialBackingStore failed to create never_remember_index: %d.", database.lastError());
        return false;
    }

    return true;
}

static bool deleteNeverRememberTables(SQLiteDatabase& database)
{
    ASSERT(database.isOpen());

    if (!database.executeCommand("DROP INDEX never_remember_index")) {
        logAlways(LogLevelWarn, "CredentialBackingStore failed to drop never_remember_index: %d.", database.lastError());
        return false;
    }

    if (!database.executeCommand("DROP TABLE never_remember")) {
        logAlways(LogLevelWarn, "CredentialBackingStore failed to drop never_remember: %d.", database.lastError());
        return false;
    }

    return true;
}

bool CredentialBackingStore::open(const String& dbPath)
{
    ASSERT(!m_database.isOpen());

    HANDLE_SQL_EXEC_FAILURE(!m_database.open(dbPath), false,
        "Failed to open database file %s for login database", dbPath.utf8().data());

    if (!m_database.tableExists("logins") && !createLoginsTables(m_database))
        return false;

    if (!m_database.tableExists("never_remember") && !createNeverRememberTables(m_database))
        return false;

    // Prepare the statements.
    m_addLoginStatement = adoptPtr(new SQLiteStatement(m_database, "INSERT OR REPLACE INTO logins (host, port, service_type, realm, auth_scheme, username, password) VALUES (?, ?, ?, ?, ?, ?, ?)"));
    HANDLE_SQL_EXEC_FAILURE(m_addLoginStatement->prepare() != SQLResultOk,
        false, "Failed to prepare addLogin statement");

    m_updateLoginStatement = adoptPtr(new SQLiteStatement(m_database, "UPDATE logins SET username = ?, password = ? WHERE host = ? AND port = ? AND service_type = ? AND realm = ? AND auth_scheme = ?"));
    HANDLE_SQL_EXEC_FAILURE(m_updateLoginStatement->prepare() != SQLResultOk,
        false, "Failed to prepare updateLogin statement");

    m_hasLoginStatement = adoptPtr(new SQLiteStatement(m_database, "SELECT COUNT(*) FROM logins WHERE host = ? AND port = ? AND service_type = ? AND realm = ? AND auth_scheme = ?"));
    HANDLE_SQL_EXEC_FAILURE(m_hasLoginStatement->prepare() != SQLResultOk,
        false, "Failed to prepare hasLogin statement");

    m_getLoginStatement = adoptPtr(new SQLiteStatement(m_database, "SELECT username, password FROM logins WHERE host = ? AND port = ? AND service_type = ? AND realm = ? AND auth_scheme = ?"));
    HANDLE_SQL_EXEC_FAILURE(m_getLoginStatement->prepare() != SQLResultOk,
        false, "Failed to prepare getLogin statement");

    m_removeLoginStatement = adoptPtr(new SQLiteStatement(m_database, "DELETE FROM logins WHERE host = ? AND port = ? AND service_type = ? AND realm = ? AND auth_scheme = ?"));
    HANDLE_SQL_EXEC_FAILURE(m_removeLoginStatement->prepare() != SQLResultOk,
        false, "Failed to prepare removeLogin statement");

    m_addNeverRememberStatement = adoptPtr(new SQLiteStatement(m_database, "INSERT OR REPLACE INTO never_remember (host, port, service_type, realm, auth_scheme) VALUES (?, ?, ?, ?, ?)"));
    HANDLE_SQL_EXEC_FAILURE(m_addNeverRememberStatement->prepare() != SQLResultOk,
        false, "Failed to prepare addNeverRemember statement");

    m_hasNeverRememberStatement = adoptPtr(new SQLiteStatement(m_database, "SELECT COUNT(*) FROM never_remember WHERE host = ? AND port = ? AND service_type = ? AND realm = ? AND auth_scheme = ?"));
    HANDLE_SQL_EXEC_FAILURE(m_hasNeverRememberStatement->prepare() != SQLResultOk,
        false, "Failed to prepare hasNeverRemember statement");

    m_removeNeverRememberStatement = adoptPtr(new SQLiteStatement(m_database, "DELETE FROM never_remember WHERE host = ? AND port = ? AND service_type = ? AND realm = ? AND auth_scheme = ?"));
    HANDLE_SQL_EXEC_FAILURE(m_removeNeverRememberStatement->prepare() != SQLResultOk,
        false, "Failed to prepare removeNeverRemember statement");

    unsigned existing = existingDatabaseVersion();
    unsigned current = currentDatabaseVersion();
    if (existing != current) {
        upgradeLoginsTable(existing, current);
        upgradeNeverRememberTable(existing, current);
    }

    return true;
}

unsigned CredentialBackingStore::currentDatabaseVersion() const
{
    // Right now there are 2 version of database used:
    // 1: the version that uses origin_url in table logins and never_remember.
    // 2: the version that removes origin_url from both table logins and never_remember.
    return 2;
}

unsigned CredentialBackingStore::existingDatabaseVersion()
{
    ASSERT(m_database.isOpen());

    // For SQLite, pragma table_info should return data like following.
    //
    // sqlite> pragma table_info("cookies");
    // cid         name        type        notnull     dflt_value  pk
    // ----------  ----------  ----------  ----------  ----------  ----------
    // 0           name        TEXT        0                       1
    // 1           value       TEXT        0                       0

    do {
        if (!m_database.tableExists("logins"))
            break;

        SQLiteStatement tableinfo(m_database, "PRAGMA table_info('logins');");
        int result = tableinfo.step();
        ASSERT(SQLResultOk == result);
        if (SQLResultOk != result) {
            logAlways(LogLevelInfo, "CredentialBackingStore tableinfo(logins): %d.", result);
            break;
        }

        String name = tableinfo.getColumnText(1);
        if (name == "origin_url")
            return 1;
    } while (false);

    do {
        if (!m_database.tableExists("never_remember"))
            break;

        SQLiteStatement tableinfo(m_database, "PRAGMA table_info('never_remember');");
        int result = tableinfo.step();
        ASSERT(SQLResultOk == result);
        if (SQLResultOk != result) {
            logAlways(LogLevelInfo, "CredentialBackingStore tableinfo(never_remember): %d.", result);
            break;
        }

        String name = tableinfo.getColumnText(1);
        if (name == "origin_url")
            return 1;
    } while (false);

    return 2;
}

bool CredentialBackingStore::getAllLoginsV1(Vector<ProtectionSpace>& protectionSpaces, Vector<Credential>& credentials)
{
    ASSERT(m_database.isOpen());
    ASSERT(m_database.tableExists("logins"));

    SQLiteStatement alllogins(m_database, "SELECT * from logins;");

    int result;
    while ((result = alllogins.step()) == SQLResultOk || result == SQLResultRow) {
        String host = alllogins.getColumnText(1);
        int port = alllogins.getColumnInt(2);
        int serviceType = alllogins.getColumnInt(3);
        String realm = alllogins.getColumnText(4);
        int authScheme = alllogins.getColumnInt(5);

        ProtectionSpace space(host, port, static_cast<ProtectionSpaceServerType>(serviceType), realm, static_cast<ProtectionSpaceAuthenticationScheme>(authScheme));
        protectionSpaces.append(space);

        String username = alllogins.getColumnText(6);
        String password;

        // Here we don't try to decrypt the password, as we are just moving it around,
        // without intention to actually use it, so there is no need to decrypt it.
        if (!certMgrWrapper()->isReady())
            password = alllogins.getColumnBlobAsString(7);
        else {
            String url = alllogins.getColumnText(0);
            unsigned hash = hashCredentialInfoV1(url, space, username);
            std::string passwordBlob;

            // Password was saved in binary format, but it is now changed to
            // base64 encoded text, so we need to convert them here.
            if (certMgrWrapper()->getPassword(hash, passwordBlob))
                password = base64Encode(passwordBlob.data(), passwordBlob.length());
        }

        Credential cred(username, password, CredentialPersistencePermanent);
        credentials.append(cred);
    }

    return true;
}

bool CredentialBackingStore::saveAllLogins(const Vector<ProtectionSpace>& protectionSpaces, const Vector<Credential>& credentials)
{
    ASSERT(protectionSpaces.size() == credentials.size());

    if (!m_addLoginStatement)
        return false;

    SQLiteTransaction transaction(m_database);
    transaction.begin();

    Vector<ProtectionSpace>::const_iterator spaceiter = protectionSpaces.begin();
    Vector<ProtectionSpace>::const_iterator spaceend = protectionSpaces.end();
    Vector<Credential>::const_iterator crediter = credentials.begin();
    for (; spaceiter != spaceend; ++spaceiter, ++crediter) {
        m_addLoginStatement->bindText(1, spaceiter->host());
        m_addLoginStatement->bindInt(2, spaceiter->port());
        m_addLoginStatement->bindInt(3, static_cast<int>(spaceiter->serverType()));
        m_addLoginStatement->bindText(4, spaceiter->realm());
        m_addLoginStatement->bindInt(5, static_cast<int>(spaceiter->authenticationScheme()));
        m_addLoginStatement->bindText(6, crediter->user());

        // Here the password is encrypted already so no need to encrypt again.
        if (!certMgrWrapper()->isReady())
            m_addLoginStatement->bindBlob(7, crediter->password());
        else {
            m_addLoginStatement->bindBlob(7, "");

            unsigned hash = hashCredentialInfo(*spaceiter, crediter->user());
            certMgrWrapper()->savePassword(hash, std::string(reinterpret_cast<const char*>(crediter->password().characters8()), crediter->password().length()));
        }

        int result = m_addLoginStatement->step();
        m_addLoginStatement->reset();

        if (result != SQLResultDone)
            logAlways(LogLevelWarn, "CredentialBackingStore::saveAllLogins() failed with error %d: %d.", result, m_database.lastError());
    }
    transaction.commit();

    return true;
}

void CredentialBackingStore::upgradeLoginsTable(unsigned existing, unsigned current)
{
    ASSERT(m_database.isOpen());

    if (existing == current)
        return;

    Vector<ProtectionSpace> protectionSpaces;
    Vector<Credential> credentials;

    if (1 == existing)
        getAllLoginsV1(protectionSpaces, credentials);

    if (!deleteLoginsTables(m_database))
        return;

    if (!createLoginsTables(m_database))
        return;

    if (2 == current)
        saveAllLogins(protectionSpaces, credentials);
}

bool CredentialBackingStore::getAllNeverRememberV1(Vector<ProtectionSpace>& protectionSpaces)
{
    ASSERT(m_database.isOpen());
    ASSERT(m_database.tableExists("never_remember"));

    SQLiteStatement allremembers(m_database, "SELECT * from never_remember;");

    int result;
    while ((result = allremembers.step()) == SQLResultRow || result == SQLResultOk) {
        String host = allremembers.getColumnText(1);
        int port = allremembers.getColumnInt(2);
        int serviceType = allremembers.getColumnInt(3);
        String realm = allremembers.getColumnText(4);
        int authScheme = allremembers.getColumnInt(5);

        ProtectionSpace space(host, port, static_cast<ProtectionSpaceServerType>(serviceType), realm, static_cast<ProtectionSpaceAuthenticationScheme>(authScheme));
        protectionSpaces.append(space);
    }

    return true;
}

bool CredentialBackingStore::saveAllNeverRemember(const Vector<ProtectionSpace>& protectionSpaces)
{
    if (!m_addNeverRememberStatement)
        return false;

    SQLiteTransaction transaction(m_database);
    transaction.begin();

    Vector<ProtectionSpace>::const_iterator spaceiter = protectionSpaces.begin();
    Vector<ProtectionSpace>::const_iterator spaceend = protectionSpaces.end();
    for (; spaceiter != spaceend; ++spaceiter) {
        m_addNeverRememberStatement->bindText(1, spaceiter->host());
        m_addNeverRememberStatement->bindInt(2, spaceiter->port());
        m_addNeverRememberStatement->bindInt(3, static_cast<int>(spaceiter->serverType()));
        m_addNeverRememberStatement->bindText(4, spaceiter->realm());
        m_addNeverRememberStatement->bindInt(5, static_cast<int>(spaceiter->authenticationScheme()));

        int result = m_addNeverRememberStatement->step();
        m_addNeverRememberStatement->reset();

        if (result != SQLResultDone)
            logAlways(LogLevelWarn, "CredentialBackingStore::saveAllNeverRemember() failed with error %d: %d.", result, m_database.lastError());
    }
    transaction.commit();

    return true;
}

void CredentialBackingStore::upgradeNeverRememberTable(unsigned existing, unsigned current)
{
    ASSERT(m_database.isOpen());

    if (existing == current)
        return;

    Vector<ProtectionSpace> protectionSpaces;

    if (1 == existing)
        getAllNeverRememberV1(protectionSpaces);

    if (!deleteNeverRememberTables(m_database))
        return;

    if (!createNeverRememberTables(m_database))
        return;

    if (2 == current)
        saveAllNeverRemember(protectionSpaces);
}

bool CredentialBackingStore::addLogin(const ProtectionSpace& protectionSpace, const Credential& credential)
{
    ASSERT(m_database.isOpen());
    ASSERT(m_database.tableExists("logins"));

    if (!m_addLoginStatement)
        return false;

    m_addLoginStatement->bindText(1, protectionSpace.host());
    m_addLoginStatement->bindInt(2, protectionSpace.port());
    m_addLoginStatement->bindInt(3, static_cast<int>(protectionSpace.serverType()));
    m_addLoginStatement->bindText(4, protectionSpace.realm());
    m_addLoginStatement->bindInt(5, static_cast<int>(protectionSpace.authenticationScheme()));
    m_addLoginStatement->bindText(6, credential.user());
    if (certMgrWrapper()->isReady())
        m_addLoginStatement->bindBlob(7, "");
    else {
        String ciphertext = encryptedString(credential.password());
        ASSERT(ciphertext.is8Bit());
        m_addLoginStatement->bindBlob(7, ciphertext.characters8(), ciphertext.length());
    }

    int result = m_addLoginStatement->step();
    m_addLoginStatement->reset();
    HANDLE_SQL_EXEC_FAILURE(result != SQLResultDone, false,
        "Failed to add login info into table logins - %i", result);

    if (!certMgrWrapper()->isReady())
        return true;

    String ciphertext = encryptedString(credential.password());
    ASSERT(ciphertext.is8Bit());
    unsigned hash = hashCredentialInfo(protectionSpace, credential.user());
    return certMgrWrapper()->savePassword(hash, std::string(reinterpret_cast<const char*>(ciphertext.characters8()), ciphertext.length()));
}

bool CredentialBackingStore::updateLogin(const ProtectionSpace& protectionSpace, const Credential& credential)
{
    ASSERT(m_database.isOpen());
    ASSERT(m_database.tableExists("logins"));

    if (!m_updateLoginStatement)
        return false;

    m_updateLoginStatement->bindText(1, credential.user());
    if (certMgrWrapper()->isReady())
        m_updateLoginStatement->bindBlob(2, "");
    else {
        String ciphertext = encryptedString(credential.password());
        ASSERT(ciphertext.is8Bit());
        m_updateLoginStatement->bindBlob(2, ciphertext.characters8(), ciphertext.length());
    }
    m_updateLoginStatement->bindText(3, protectionSpace.host());
    m_updateLoginStatement->bindInt(4, protectionSpace.port());
    m_updateLoginStatement->bindInt(5, static_cast<int>(protectionSpace.serverType()));
    m_updateLoginStatement->bindText(6, protectionSpace.realm());
    m_updateLoginStatement->bindInt(7, static_cast<int>(protectionSpace.authenticationScheme()));

    int result = m_updateLoginStatement->step();
    m_updateLoginStatement->reset();
    HANDLE_SQL_EXEC_FAILURE(result != SQLResultDone, false,
        "Failed to update login info in table logins - %i", result);

    if (!certMgrWrapper()->isReady())
        return true;

    String ciphertext = encryptedString(credential.password());
    ASSERT(ciphertext.is8Bit());
    unsigned hash = hashCredentialInfo(protectionSpace, credential.user());
    return certMgrWrapper()->savePassword(hash, std::string(reinterpret_cast<const char*>(ciphertext.characters8()), ciphertext.length()));
}

bool CredentialBackingStore::hasLogin(const ProtectionSpace& protectionSpace)
{
    ASSERT(m_database.isOpen());
    ASSERT(m_database.tableExists("logins"));

    if (!m_hasLoginStatement)
        return false;

    m_hasLoginStatement->bindText(1, protectionSpace.host());
    m_hasLoginStatement->bindInt(2, protectionSpace.port());
    m_hasLoginStatement->bindInt(3, static_cast<int>(protectionSpace.serverType()));
    m_hasLoginStatement->bindText(4, protectionSpace.realm());
    m_hasLoginStatement->bindInt(5, static_cast<int>(protectionSpace.authenticationScheme()));

    int result = m_hasLoginStatement->step();
    int numOfRow = m_hasLoginStatement->getColumnInt(0);
    m_hasLoginStatement->reset();
    HANDLE_SQL_EXEC_FAILURE(result != SQLResultRow, false,
        "Failed to execute select login info from table logins in hasLogin - %i", result);

    if (numOfRow)
        return true;
    return false;
}

Credential CredentialBackingStore::getLogin(const ProtectionSpace& protectionSpace)
{
    ASSERT(m_database.isOpen());
    ASSERT(m_database.tableExists("logins"));

    if (!m_getLoginStatement)
        return Credential();

    m_getLoginStatement->bindText(1, protectionSpace.host());
    m_getLoginStatement->bindInt(2, protectionSpace.port());
    m_getLoginStatement->bindInt(3, static_cast<int>(protectionSpace.serverType()));
    m_getLoginStatement->bindText(4, protectionSpace.realm());
    m_getLoginStatement->bindInt(5, static_cast<int>(protectionSpace.authenticationScheme()));

    int result = m_getLoginStatement->step();
    String username = m_getLoginStatement->getColumnText(0);
    String password = m_getLoginStatement->getColumnBlobAsString(1);
    m_getLoginStatement->reset();
    HANDLE_SQL_EXEC_FAILURE(result != SQLResultRow && result != SQLResultDone, Credential(),
        "Failed to execute select login info from table logins in getLogin - %i", result);

    if (result != SQLResultRow)
        return Credential();

    if (!certMgrWrapper()->isReady())
        return Credential(username, decryptedString(password), CredentialPersistencePermanent);

    unsigned hash = hashCredentialInfo(protectionSpace, username);

    std::string passwordBlob;
    if (!certMgrWrapper()->getPassword(hash, passwordBlob))
        return Credential();

    return Credential(username, decryptedString(String(passwordBlob.data(), passwordBlob.length())), CredentialPersistencePermanent);
}

bool CredentialBackingStore::removeLogin(const ProtectionSpace& protectionSpace, const String& username)
{
    ASSERT(m_database.isOpen());
    ASSERT(m_database.tableExists("logins"));

    if (!m_removeLoginStatement)
        return false;

    m_removeLoginStatement->bindText(1, protectionSpace.host());
    m_removeLoginStatement->bindInt(2, protectionSpace.port());
    m_removeLoginStatement->bindInt(3, static_cast<int>(protectionSpace.serverType()));
    m_removeLoginStatement->bindText(4, protectionSpace.realm());
    m_removeLoginStatement->bindInt(5, static_cast<int>(protectionSpace.authenticationScheme()));

    int result = m_removeLoginStatement->step();
    m_removeLoginStatement->reset();
    HANDLE_SQL_EXEC_FAILURE(result != SQLResultDone, false,
        "Failed to remove login info from table logins - %i", result);

    if (!certMgrWrapper()->isReady())
        return true;

    unsigned hash = hashCredentialInfo(protectionSpace, username);
    if (!certMgrWrapper()->removePassword(hash))
        return false;

    return true;
}

bool CredentialBackingStore::addNeverRemember(const ProtectionSpace& protectionSpace)
{
    ASSERT(m_database.isOpen());
    ASSERT(m_database.tableExists("never_remember"));

    if (!m_addNeverRememberStatement)
        return false;

    m_addNeverRememberStatement->bindText(1, protectionSpace.host());
    m_addNeverRememberStatement->bindInt(2, protectionSpace.port());
    m_addNeverRememberStatement->bindInt(3, static_cast<int>(protectionSpace.serverType()));
    m_addNeverRememberStatement->bindText(4, protectionSpace.realm());
    m_addNeverRememberStatement->bindInt(5, static_cast<int>(protectionSpace.authenticationScheme()));

    int result = m_addNeverRememberStatement->step();
    m_addNeverRememberStatement->reset();
    HANDLE_SQL_EXEC_FAILURE(result != SQLResultDone, false,
        "Failed to add naver saved item info into table never_remember - %i", result);

    return true;
}

bool CredentialBackingStore::hasNeverRemember(const ProtectionSpace& protectionSpace)
{
    ASSERT(m_database.isOpen());
    ASSERT(m_database.tableExists("never_remember"));

    if (!m_hasNeverRememberStatement)
        return false;

    m_hasNeverRememberStatement->bindText(1, protectionSpace.host());
    m_hasNeverRememberStatement->bindInt(2, protectionSpace.port());
    m_hasNeverRememberStatement->bindInt(3, static_cast<int>(protectionSpace.serverType()));
    m_hasNeverRememberStatement->bindText(4, protectionSpace.realm());
    m_hasNeverRememberStatement->bindInt(5, static_cast<int>(protectionSpace.authenticationScheme()));

    int result = m_hasNeverRememberStatement->step();
    int numOfRow = m_hasNeverRememberStatement->getColumnInt(0);
    m_hasNeverRememberStatement->reset();
    HANDLE_SQL_EXEC_FAILURE(result != SQLResultRow, false,
        "Failed to execute select to find naver saved site from table never_remember - %i", result);

    if (numOfRow)
        return true;
    return false;
}

bool CredentialBackingStore::removeNeverRemember(const ProtectionSpace& protectionSpace)
{
    ASSERT(m_database.isOpen());
    ASSERT(m_database.tableExists("never_remember"));

    if (!m_removeNeverRememberStatement)
        return false;

    m_removeNeverRememberStatement->bindText(1, protectionSpace.host());
    m_removeNeverRememberStatement->bindInt(2, protectionSpace.port());
    m_removeNeverRememberStatement->bindInt(3, static_cast<int>(protectionSpace.serverType()));
    m_removeNeverRememberStatement->bindText(4, protectionSpace.realm());
    m_removeNeverRememberStatement->bindInt(5, static_cast<int>(protectionSpace.authenticationScheme()));

    int result = m_removeNeverRememberStatement->step();
    m_removeNeverRememberStatement->reset();
    HANDLE_SQL_EXEC_FAILURE(result != SQLResultDone, false,
        "Failed to remove never saved site from table never_remember - %i", result);

    return true;
}

bool CredentialBackingStore::clearLogins()
{
    ASSERT(m_database.isOpen());
    ASSERT(m_database.tableExists("logins"));

    HANDLE_SQL_EXEC_FAILURE(!m_database.executeCommand("DELETE FROM logins"),
        false, "Failed to clear table logins");

    return true;
}

bool CredentialBackingStore::clearNeverRemember()
{
    ASSERT(m_database.isOpen());
    ASSERT(m_database.tableExists("never_remember"));

    HANDLE_SQL_EXEC_FAILURE(!m_database.executeCommand("DELETE FROM never_remember"),
        false, "Failed to clear table never_remember");

    return true;
}

String CredentialBackingStore::encryptedString(const String& plainText) const
{
    WTF::CString utf8 = plainText.utf8(String::StrictConversion);
    std::string cipherText;
    BlackBerry::Platform::Encryptor::encryptString(std::string(utf8.data(), utf8.length()), &cipherText);
    return String(cipherText.data(), cipherText.length());
}

String CredentialBackingStore::decryptedString(const String& cipherText) const
{
    std::string text = cipherText.is8Bit() ?
        std::string(reinterpret_cast<const char*>(cipherText.characters8()), cipherText.length() * sizeof(LChar)) :
        std::string(reinterpret_cast<const char*>(cipherText.characters16()), cipherText.length() * sizeof(UChar));

    std::string plainText;
    BlackBerry::Platform::Encryptor::decryptString(text, &plainText);

    return String::fromUTF8(plainText.data(), plainText.length());
}

BlackBerry::Platform::CertMgrWrapper* CredentialBackingStore::certMgrWrapper()
{
    if (!m_certMgrWrapper)
        m_certMgrWrapper = adoptPtr(new BlackBerry::Platform::CertMgrWrapper());

    return m_certMgrWrapper.get();
}


} // namespace WebCore

#endif // ENABLE(BLACKBERRY_CREDENTIAL_PERSIST)
