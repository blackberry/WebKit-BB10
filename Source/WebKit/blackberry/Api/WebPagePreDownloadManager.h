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

#ifndef WebPagePreDownloadManager_h
#define WebPagePreDownloadManager_h

#include "ResourceHandleClient.h"

#include <BlackBerryPlatformMisc.h>
#include <wtf/PassRefPtr.h>

namespace WebCore {
class NetworkJob;
class ResourceError;
class ResourceHandle;
class ResourceRequest;
class ResourceResponse;
}

namespace BlackBerry {
namespace WebKit {

class WebPageClient;
class WebPagePreDownloadClient;
class WebPagePreDownloadClientPrivate;
class WebPagePreDownloadManagerPrivate;

class WebPagePreDownloadListener {
public:
    virtual void notifyDownloadCreated(unsigned jodId) = 0;
    virtual void notifyDownloadFailed(unsigned jobId, int error) = 0;
};

class WebPagePreDownloadManager {
public:
    WebPagePreDownloadManager(WebPageClient*);
    ~WebPagePreDownloadManager();

    void addPreDownloadClient(WebPagePreDownloadClient*);
    void removePreDownloadClient(WebPagePreDownloadClient*);

    void clearAll();
    WebPageClient* pageClient();

private:
    WebPagePreDownloadManagerPrivate* d;

    DISABLE_COPY(WebPagePreDownloadManager)
};

class WebPagePreDownloadClient : public WebCore::ResourceHandleClient {
public:
    WebPagePreDownloadClient(WebPagePreDownloadManager*, WebPagePreDownloadListener*, unsigned parentDownloadId, bool confirmed);
    ~WebPagePreDownloadClient();

    void setResourceHandle(PassRefPtr<WebCore::ResourceHandle>);

    virtual void didReceiveResponse(WebCore::ResourceHandle*, const WebCore::ResourceResponse&) OVERRIDE;
    virtual void didReceiveData(WebCore::ResourceHandle*, const char*, int, int /*encodedDataLength*/) OVERRIDE;
    virtual void didFinishLoading(WebCore::ResourceHandle*, double /*finishTime*/) OVERRIDE;
    virtual void didFail(WebCore::ResourceHandle*, const WebCore::ResourceError&) OVERRIDE;

private:
    WebPagePreDownloadClientPrivate* d;

    DISABLE_COPY(WebPagePreDownloadClient)
};

} // namespace WebKit
} // namespace BlackBerry

#endif // WebPagePreDownloadManager_h
