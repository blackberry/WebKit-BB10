/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
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

#ifndef RegisterProtocolHandlerClientBlackBerry_h
#define RegisterProtocolHandlerClientBlackBerry_h

#if ENABLE(REGISTER_PROTOCOL_HANDLER) || ENABLE(CUSTOM_SCHEME_HANDLER)

#include "RegisterProtocolHandlerClient.h"

namespace BlackBerry {
namespace WebKit {
class WebPagePrivate;
}
}

namespace WebCore {
class RegisterProtocolHandlerClientBlackBerry : public WebCore::RegisterProtocolHandlerClient {
public:
    explicit RegisterProtocolHandlerClientBlackBerry(BlackBerry::WebKit::WebPagePrivate*);
    ~RegisterProtocolHandlerClientBlackBerry() { }

#if ENABLE(REGISTER_PROTOCOL_HANDLER)
    virtual void registerProtocolHandler(const String& scheme, const String& baseURL, const String& url, const String& title);
#endif

#if ENABLE(CUSTOM_SCHEME_HANDLER)
    virtual CustomHandlersState isProtocolHandlerRegistered(const String& scheme, const String& baseURL, const String& url);
    virtual void unregisterProtocolHandler(const String& scheme, const String& baseURL, const String& url);
#endif

private:
    BlackBerry::WebKit::WebPagePrivate* m_webPagePrivate;
};
}

#endif
#endif // RegisterProtocolHandlerClientBlackBerry_h
