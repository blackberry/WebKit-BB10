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
#include "WebPageSaver.h"

#include "CachedResourceStream.h"
#include "Frame.h"
#include "PageSerializer.h"
#include "ResourceBuffer.h"
#include "SharedBuffer.h"
#include "WebPageClient.h"
#include "WebPage_p.h"

#include <BlackBerryPlatformString.h>
#include <network/DownloadManager.h>
#include <wtf/text/StringBuilder.h>

namespace BlackBerry {

using WebCore::CachedResourceStream;
using WebCore::PageSerializer;
using WebCore::ResourceBuffer;
using WebCore::SharedBuffer;

namespace WebKit {

struct FrameData {
    FrameData(unsigned, PassRefPtr<SharedBuffer>);

    String data;

    unsigned index;
    CachedResourceStream* stream;
    bool changed;
};

FrameData::FrameData(unsigned i, PassRefPtr<SharedBuffer> buffer)
    : index(i)
    , stream(0)
    , changed(false)
{
    StringBuilder sb;
    const char* segment;
    unsigned pos = 0;
    while (unsigned length = buffer->getSomeData(segment, pos)) {
        sb.append(segment, length);
        pos += length;
    }

    data = sb.toString();
}

static bool shouldReplaceUrl(const String& data, const String& url, unsigned pos)
{
    BLACKBERRY_ASSERT(data.length() && url.length() && data.length() >= url.length() + pos);

    // And the data should at least have one extra space after the matching url
    // to contain the matching '"' character.
    if (data.length() - pos < url.length() + 1)
        return false;

    if (data[pos + url.length()] != '"')
        return false;

    if (pos >= 5 && equalIgnoringCase(data.substring(pos - 5, 5), "src=\""))
        return true;
    if (pos >= 6 && equalIgnoringCase(data.substring(pos - 6, 6), "href=\""))
        return true;

    return false;
}

static int replaceURLWithFilename(String& data, const String& url, const String& filename)
{
    BLACKBERRY_ASSERT(data.length() && url.length() && filename.length());

    int count = 0;
    unsigned pos, start = 0;
    StringBuilder sb;

    while ((pos = data.find(url, start)) != notFound) {
        if (shouldReplaceUrl(data, url, pos)) {
            sb.append(data.substring(start, pos - start));
            sb.append(filename);
            ++count;
        } else
            sb.append(data.substring(start, pos - start + url.length()));

        start = pos + url.length();
    }

    if (count) {
        sb.append(data.substring(start, data.length() - start));
        data = sb.toString();
    }

    return count;
}

static bool isHTMLFrame(const String& mimeType)
{
    if (mimeType == "application/xhtml+xml")
        return true;
    if (mimeType == "text/html")
        return true;

    return false;
}

unsigned WebPageSaver::saveCurrentPage(WebPagePrivate* pageData, const Platform::String& filename)
{
    Vector<PageSerializer::Resource> resources;
    PageSerializer serializer(&resources);
    serializer.serialize(pageData->mainFrame()->page());

    BLACKBERRY_ASSERT(resources.size());
    if (!resources.size())
        return 0;

    CachedResourceStream* mainStream = new CachedResourceStream(resources[0].url, resources[0].mimeType, ResourceBuffer::adoptSharedBuffer(resources[0].data));

    unsigned jobid = pageData->client()->downloadRequested(mainStream, filename, 0 /* parentDownloadId */, true /* confirmed */);
    if (resources.size() == 1) {
        mainStream->streamOpen();
        return jobid;
    }

    Vector<FrameData> htmlFrames;

    htmlFrames.append(FrameData(0, resources[0].data));
    htmlFrames[0].stream = mainStream;

    for (unsigned ri = 1; ri < resources.size(); ++ri) {
        if (isHTMLFrame(resources[ri].mimeType))
            htmlFrames.append(FrameData(ri, resources[ri].data));
    }

    for (unsigned ri = 1; ri < resources.size(); ++ri) {
        String urlStr(resources[ri].url.string());
        Platform::String name = Platform::DownloadManager::instance()->filenameForChildDownload(jobid, Platform::String(urlStr));
        String ucs2name = String::fromUTF8(name.data(), name.length());
        String encodedName = WebCore::encodeWithURLEscapeSequences(ucs2name);

        CachedResourceStream* stream = new CachedResourceStream(resources[ri].url, resources[ri].mimeType, ResourceBuffer::adoptSharedBuffer(resources[ri].data));
        pageData->client()->downloadRequested(stream, name, jobid, true /* confirmed */);

        bool isFrame = false;
        for (unsigned fi = 0; fi < htmlFrames.size(); ++fi) {
            if (ri == htmlFrames[fi].index) {
                htmlFrames[fi].stream = stream;
                isFrame = true;
                continue;
            }

            if (replaceURLWithFilename(htmlFrames[fi].data, urlStr, encodedName))
                htmlFrames[fi].changed = true;
        }

        if (!isFrame)
            stream->streamOpen();
    }

    for (unsigned fi = 0; fi < htmlFrames.size(); ++fi) {
        if (htmlFrames[fi].changed) {
            const char* data = htmlFrames[fi].data.is8Bit() ? reinterpret_cast<const char*>(htmlFrames[fi].data.characters8()) : reinterpret_cast<const char*>(htmlFrames[fi].data.characters16());
            RefPtr<ResourceBuffer> newbuffer = ResourceBuffer::create(data, htmlFrames[fi].data.sizeInBytes());
            htmlFrames[fi].stream->updateData(newbuffer);
        }

        htmlFrames[fi].stream->streamOpen();
    }

    return jobid;
}

} // WebKit
} // BlackBerry
