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

#ifndef CredMgrUtils_h
#define CredMgrUtils_h

#include <BlackBerryPlatformMisc.h>
#include <BlackBerryPlatformString.h>
#include <sys/credmgr_authdomain_network.h>
#include <sys/credmgr_policy.h>
#include <sys/credmgr_store.h>

namespace WebCore {

class ProtectionSpace;

// RAII wrapper that calls the appropriate CredMgr release function when an object goes out of
// scope.
template<typename T>
class CredMgrObject {
public:
    CredMgrObject()
        : m_isValid(false)
    {
    }

    CredMgrObject(const T& object)
        : m_object(object)
        , m_isValid(true)
    {
    }

    ~CredMgrObject()
    {
        release();
    }

    void release()
    {
        if (m_isValid)
            release(&m_object);
        m_isValid = false;
    }

    T& ref()
    {
        return m_object;
    }

    T* ptr()
    {
        return &m_object;
    }

    void setValid(bool value)
    {
        m_isValid = value;
    }

    bool isValid() const
    {
        return m_isValid;
    }

private:
    DISABLE_COPY(CredMgrObject<T>)

    T m_object;
    bool m_isValid;

    static void release(credmgr_AccessSubject* subject)
    {
        credmgr_AccessSubjectRelease(subject);
    }

    static void release(credmgr_Item* item)
    {
        credmgr_ItemRelease(item);
    }

    static void release(credmgr_ItemIterator* iterator)
    {
        credmgr_ItemIteratorRelease(iterator);
    }

    static void release(credmgr_ItemQuery* query)
    {
        credmgr_ItemQueryRelease(query);
    }

    static void release(credmgr_ItemQueryResult* result)
    {
        credmgr_ItemQueryResultRelease(result);
    }

    static void release(credmgr_NetworkAuthDomain* domain)
    {
        credmgr_NetworkAuthDomainRelease(domain);
    }
};

// An abstract base class holding the parameters for a CredMgr query, either by url or by
// host+port+protocol.
class CredMgrQuery {
public:
    CredMgrQuery(credmgr_NetworkHostType hostType, credmgr_NetworkAuthScheme authScheme, const credmgr_Store& store)
        : m_hostType(hostType)
        , m_authScheme(authScheme)
        , m_store(store)
    {
    }

    virtual ~CredMgrQuery()
    {
    }

    credmgr_NetworkHostType hostType() const { return m_hostType; }
    credmgr_NetworkAuthScheme authScheme() const { return m_authScheme; }
    credmgr_Store store() const { return m_store; }

    bool isPasswordQuery() const { return m_authScheme != CREDMGR_NETWORK_AUTH_SCHEME_CLIENT_CERT; }

    // Possible results of findOneItem in order of precedence.
    enum FindResult {
        FoundError, // An error occurred; items may or may not exist.
        FoundMultipleItems, // More than one successful item was found; only the first was returned.
        FoundSuccessfulItem, // A single successful item was returned; there may or may not be items with other status codes as well.
        FoundMechNotAvail, // An item with MechNotAvailable was returned; there may or may not be failed items as well.
        FoundFailedItem, // Only failed items were returned.
        FoundNothing, // No items returned.
    };

    // Loop through all items returned by the query, and return the most specific status code
    // possible. Fill in the item parameter with the first successful item found (if the result is
    // not FoundSuccessfulItem or FoundMultipleItems, the item is not changed.)
    FindResult findOneItem(CredMgrObject<credmgr_Item>&, bool allowPrompts) const;

    enum CreatedItemStatus {
        ItemIsUnchanged,
        ItemIsUpdated,
        ItemIsNew,
        ItemHasError,
    };

    // Create a temporary item in CredMgr with the given username and password, that would be
    // returned by this query. If the changes are to be persisted, call saveItem afterward.
    CreatedItemStatus createItem(CredMgrObject<credmgr_Item>&, const BlackBerry::Platform::String& username, const BlackBerry::Platform::String& password, const BlackBerry::Platform::String& url) const;

    // Persist changes that were made to this item by createItem.
    void saveItem(CreatedItemStatus, const credmgr_Item&) const;

protected:

    // Create a credmgr_ItemQuery object with the parameters for this query.
    virtual bool createQuery(CredMgrObject<credmgr_ItemQuery>&) const = 0;

    // Call credmgr_NetworkAuthDomainCreateAndSave or a similar function to bind a new item to a domain.
    virtual bool bindItemToDomain(const credmgr_Item&) const = 0;

    // Common parameters used by all queries.
    credmgr_NetworkHostType m_hostType;
    credmgr_NetworkAuthScheme m_authScheme;
    credmgr_Store m_store;
};

// A CredMgrQuery keyed by url (queries created with credmgr_NetworkItemQueryCreateFromUrl, new
// items bound with credmgr_NetworkAuthDomainCreateFromUrlAndSave).
class CredMgrUrlQuery : public CredMgrQuery {
public:
    CredMgrUrlQuery(const BlackBerry::Platform::String& url,
                    credmgr_NetworkHostType hostType,
                    credmgr_NetworkAuthScheme authScheme,
                    const credmgr_Store& store)
        : CredMgrQuery(hostType, authScheme, store)
        , m_url(url.utf8())
    {
    }

    BlackBerry::Platform::String url() const { return m_url; }

    // CredMgrUrlQuery already contains a url, so overload createItem to use that url.
    CreatedItemStatus createItem(CredMgrObject<credmgr_Item>&, const BlackBerry::Platform::String& username, const BlackBerry::Platform::String& password) const;
    CreatedItemStatus createItem(CredMgrObject<credmgr_Item>&, const BlackBerry::Platform::String& username, const BlackBerry::Platform::String& password, const BlackBerry::Platform::String& url) const;

private:
    virtual bool createQuery(CredMgrObject<credmgr_ItemQuery>&) const;
    virtual bool bindItemToDomain(const credmgr_Item&) const;

    BlackBerry::Platform::String m_url;
};

// A CredMgrQuery keyed by protocol+server+port (queries created with
// credmgr_NetworkItemQueryCreate, new items bound with credmgr_NetworkAuthDomainCreateAndSave).
class CredMgrServerQuery : public CredMgrQuery {
public:
    CredMgrServerQuery(credmgr_NetworkProtocol protocol,
                       const BlackBerry::Platform::String& serverName,
                       int serverPort,
                       credmgr_NetworkHostType hostType,
                       credmgr_NetworkAuthScheme authScheme,
                       const credmgr_Store& store,
                       const BlackBerry::Platform::String& resourcePath = BlackBerry::Platform::String::emptyString())
        : CredMgrQuery(hostType, authScheme, store)
        , m_protocol(protocol)
        , m_serverName(serverName.utf8())
        , m_serverPort(serverPort)
        , m_resourcePath(resourcePath.utf8())
    {
    }

    CredMgrServerQuery(const ProtectionSpace&, const credmgr_Store&, const BlackBerry::Platform::String& resourcePath = BlackBerry::Platform::String::emptyString());

    credmgr_NetworkProtocol protocol() const { return m_protocol; }
    BlackBerry::Platform::String serverName() const { return m_serverName; }
    int serverPort() const { return m_serverPort; }
    BlackBerry::Platform::String resourcePath() const { return m_resourcePath; }

private:
    virtual bool createQuery(CredMgrObject<credmgr_ItemQuery>&) const;
    virtual bool bindItemToDomain(const credmgr_Item&) const;

    credmgr_NetworkProtocol m_protocol;
    BlackBerry::Platform::String m_serverName;
    int m_serverPort;
    BlackBerry::Platform::String m_resourcePath;
};

// Utility functions.

bool getCredentialsFromCredMgrHandle(const BlackBerry::Platform::String& handle, BlackBerry::Platform::String& username, BlackBerry::Platform::String& password);

bool getCredentialsFromCredMgrItem(const credmgr_Item&, BlackBerry::Platform::String& username, BlackBerry::Platform::String& password);

bool getHandleFromCredMgrItem(const credmgr_Item&, BlackBerry::Platform::String& handle);

} // namespace WebCore

#endif // CredMgrUtils_h
