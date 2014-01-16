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


#ifndef CachedResourceStream_h
#define CachedResourceStream_h

#include "ResourceBuffer.h"
#include "ResourceResponse.h"

#include <BlackBerryPlatformTimer.h>
#include <network/FilterStream.h>

namespace WebCore {

// FIXME: Right now CachedResourceStream is always used for download, if
// someday it is used for non-download purpose, then we need to add a new
// parameter to its constructor to indicate its target type.
// like ResourceRequest::TargetType.

class CachedResourceStream : public BlackBerry::Platform::FilterStream {
public:
    CachedResourceStream(const String& url, const String& mimeType, PassRefPtr<ResourceBuffer>);
    virtual ~CachedResourceStream();

    // From class BlackBerry::Platform::FilterStream.
    virtual BlackBerry::Platform::String url() const OVERRIDE;
    virtual const BlackBerry::Platform::String mimeType() const OVERRIDE;
    virtual int streamOpen();

    void updateData(PassRefPtr<ResourceBuffer>);

private:
    void sendData();

    String m_url;
    String m_mimeType;
    RefPtr<ResourceBuffer> m_buff;
    BlackBerry::Platform::Timer<CachedResourceStream>* m_timer;
};

} // namespace WebCore

#endif // CachedResourceStream_h
