/*
 * Copyright (C) 2013 Research In Motion Limited. All rights reserved.
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
#include "CachedResourceStream.h"

#include <ThreadTimerClient.h>

using BlackBerry::Platform::FilterStream;

namespace WebCore {
CachedResourceStream::CachedResourceStream(const String& url, const String& mimeType, PassRefPtr<ResourceBuffer>buff)
    : FilterStream()
    , m_url(url)
    , m_mimeType(mimeType)
    , m_buff(buff)
    , m_timer(0)
{
    registerStreamId();
}

CachedResourceStream::~CachedResourceStream()
{
    delete m_timer;
    m_timer = 0;
}

BlackBerry::Platform::String CachedResourceStream::url() const
{
    return m_url;
}

const BlackBerry::Platform::String CachedResourceStream::mimeType() const
{
    return m_mimeType;
}

int CachedResourceStream::streamOpen()
{
    if (!m_timer)
        m_timer = new BlackBerry::Platform::Timer<CachedResourceStream>(this, &CachedResourceStream::sendData, BlackBerry::Platform::webKitThreadTimerClient());
    m_timer->start(0);
    return 0;
}

void CachedResourceStream::sendData()
{
    notifyDataReceived(BlackBerry::Platform::createNetworkBufferByWrappingData(m_buff->data(), m_buff->size()));
    notifyClose(0);
    m_buff.release();
}

} // namespace WebCore
