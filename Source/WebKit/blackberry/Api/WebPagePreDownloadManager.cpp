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
#include "WebPagePreDownloadManager.h"

#include "NetworkJob.h"
#include "NetworkManager.h"
#include "ResourceError.h"
#include "ResourceHandle.h"
#include "ResourceResponse.h"
#include "WebPageClient.h"

#include <BlackBerryPlatformLog.h>
#include <BlackBerryPlatformTimer.h>
#include <ThreadTimerClient.h>

#include <set>

using namespace WebCore;

namespace BlackBerry {
namespace WebKit {

class WebPagePreDownloadManagerPrivate {
public:
    void deleteClients();
    void deleteFinishedClients();

    std::set<WebPagePreDownloadClient*> m_clients;
    std::set<WebPagePreDownloadClient*> m_finishedClients;
    OwnPtr<Platform::Timer<WebPagePreDownloadManagerPrivate> > m_timer;
    WebPageClient* m_pageClient;
};

static inline void deletePreDownloadClients(std::set<WebPagePreDownloadClient*>& clients)
{
    if (clients.empty())
        return;

    std::set<WebPagePreDownloadClient*> temp;
    clients.swap(temp);

    std::set<WebPagePreDownloadClient*>::const_iterator end = temp.end();
    std::set<WebPagePreDownloadClient*>::const_iterator iter = temp.begin();
    for (; iter != end; ++iter)
        delete *iter;
}

void WebPagePreDownloadManagerPrivate::deleteClients()
{
    deletePreDownloadClients(m_clients);
}

void WebPagePreDownloadManagerPrivate::deleteFinishedClients()
{
    deletePreDownloadClients(m_finishedClients);
}

WebPagePreDownloadManager::WebPagePreDownloadManager(WebPageClient* pageClient)
    : d(new WebPagePreDownloadManagerPrivate)
{
    d->m_pageClient = pageClient;
}

WebPagePreDownloadManager::~WebPagePreDownloadManager()
{
    clearAll();

    delete d;
    d = 0;
}

void WebPagePreDownloadManager::addPreDownloadClient(WebPagePreDownloadClient* client)
{
    d->m_clients.insert(client);
}

void WebPagePreDownloadManager::removePreDownloadClient(WebPagePreDownloadClient* client)
{
    d->m_finishedClients.insert(client);
    d->m_clients.erase(client);

    if (!d->m_timer)
        d->m_timer = adoptPtr(new Platform::Timer<WebPagePreDownloadManagerPrivate>(d, &WebPagePreDownloadManagerPrivate::deleteFinishedClients, Platform::webKitThreadTimerClient()));

    d->m_timer->start(0);
}

void WebPagePreDownloadManager::clearAll()
{
    d->deleteClients();
    d->deleteFinishedClients();
}

WebPageClient* WebPagePreDownloadManager::pageClient()
{
    return d->m_pageClient;
}

class WebPagePreDownloadClientPrivate {
public:
    NetworkJob* job();

    WebPagePreDownloadManager* m_manager;
    RefPtr<ResourceHandle> m_handle;
};

NetworkJob* WebPagePreDownloadClientPrivate::job()
{
    return NetworkManager::instance()->findJobForHandle(m_handle);
}


WebPagePreDownloadClient::WebPagePreDownloadClient(WebPagePreDownloadManager* manager)
    : d(new WebPagePreDownloadClientPrivate)
{
    d->m_manager = manager;
}

WebPagePreDownloadClient::~WebPagePreDownloadClient()
{
    delete d;
    d = 0;
}

void WebPagePreDownloadClient::setResourceHandle(PassRefPtr<ResourceHandle> handle)
{
    d->m_handle = handle;
}

inline static bool isRedirect(int statusCode)
{
    return 300 <= statusCode && statusCode < 400 && statusCode != 304;
}

void WebPagePreDownloadClient::didReceiveResponse(ResourceHandle* handle, const ResourceResponse& response)
{
    BLACKBERRY_ASSERT(d->m_handle.get() == handle);
    if (isRedirect(response.httpStatusCode()))
        return;

    // We need to save NetworkJob::wrappedStream() here because in downloadRequested(),
    // the wrappedStream() will be "liberated" and be wrapped by other FilterStream,
    // like DownloadFilterStream in current DownloadManager implementation, in place of
    // NetworkJob. And NetworkJob::wrappedStream() will return 0!
    NetworkJob* job = d->job();
    if (!job) {
        Platform::logAlways(Platform::LogLevelWarn, "WebPagePreDownloadClient::didReceiveResponse(): NetworkJob for handle 0x%x not found!",
            reinterpret_cast<unsigned>(handle));
        d->m_manager->removePreDownloadClient(this);
        return;
    }

    Platform::FilterStream* stream = job->wrappedStream();
    BLACKBERRY_ASSERT(stream);

    d->m_manager->pageClient()->downloadRequested(stream, response.suggestedFilename());

    // We need to notify the FilterStream that handles downloads, which in current
    // DownloadManager implementation is DownloadFilterStream, about any error in the
    // response, so it will fail gracefully, instead hanging there waiting indefinitely
    // for status that has already been sent. Ideally this notification should be done
    // in didFail(), but it is unsafe to save the stream pointer as it could become a
    // dangling pointer if something happened, like if somehow the download is cancelled
    // and that stream has been deleted. And because the NetworkJob::status() is the
    // ResourceError::errorCode(), so it is safe to do the notification here too.

    // Calling notifyClose() of the wrappedStream() will reach DownloadFilterStream, which
    // has replaced NetworkJob as the listener of this stream.
    if (NetworkJob::isError(job->status()))
        stream->notifyClose(job->status());

    d->m_manager->removePreDownloadClient(this);
}

void WebPagePreDownloadClient::didReceiveData(ResourceHandle* handle, const char* /*data*/, int /*length*/, int /*encodedLength*/)
{
    BLACKBERRY_ASSERT(d->m_handle.get() == handle);
}

void WebPagePreDownloadClient::didFinishLoading(ResourceHandle* handle, double /*finishTime*/)
{
    BLACKBERRY_ASSERT(d->m_handle.get() == handle);
}

void WebPagePreDownloadClient::didFail(ResourceHandle* handle, const ResourceError& error)
{
    BLACKBERRY_ASSERT(d->m_handle.get() == handle);

    NetworkJob* job = d->job();
    if (!job) {
        Platform::logAlways(Platform::LogLevelWarn, "WebPagePreDownloadClient::didFail(): NetworkJob for handle 0x%x not found!",
            reinterpret_cast<unsigned>(handle));
        d->m_manager->removePreDownloadClient(this);
        return;
    }

    // If NetworkJob::wrappedStream() is not empty, then it has not been transferred
    // to DownloadManager yet, we need to transfer it to DownloadManager to let user
    // know there is a failed download job.
    Platform::FilterStream* stream = job->wrappedStream();
    if (stream) {
        d->m_manager->pageClient()->downloadRequested(stream, Platform::String::emptyString());
        stream->notifyClose(error.errorCode());
    }

    d->m_manager->removePreDownloadClient(this);
}

} // namespace WebKit
} // namespace BlackBerry
