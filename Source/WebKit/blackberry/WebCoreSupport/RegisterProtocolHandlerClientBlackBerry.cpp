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

#include "config.h"
#include "RegisterProtocolHandlerClientBlackBerry.h"

#if ENABLE(REGISTER_PROTOCOL_HANDLER) || ENABLE(CUSTOM_SCHEME_HANDLER)

#include "WebPage_p.h"

namespace WebCore {

RegisterProtocolHandlerClientBlackBerry::RegisterProtocolHandlerClientBlackBerry(BlackBerry::WebKit::WebPagePrivate* webPagePrivate)
    : m_webPagePrivate(webPagePrivate)
{
}

#if ENABLE(REGISTER_PROTOCOL_HANDLER)
void RegisterProtocolHandlerClientBlackBerry::registerProtocolHandler(const String& scheme, const String& baseURL, const String& url, const String& title)
{
    m_webPagePrivate->m_client->registerProtocolHandler(scheme, baseURL, url, title);
}
#endif

#if ENABLE(CUSTOM_SCHEME_HANDLER)
RegisterProtocolHandlerClient::CustomHandlersState RegisterProtocolHandlerClientBlackBerry::isProtocolHandlerRegistered(const String& scheme, const String& baseURL, const String& url)
{
    return static_cast<CustomHandlersState>(m_webPagePrivate->m_client->isProtocolHandlerRegistered(scheme, baseURL, url));
}

void RegisterProtocolHandlerClientBlackBerry::unregisterProtocolHandler(const String& scheme, const String& baseURL, const String& url)
{
    m_webPagePrivate->m_client->unregisterProtocolHandler(scheme, baseURL, url);
}
#endif

}

#endif // ENABLE(REGISTER_PROTOCOL_HANDLER) || ENABLE(CUSTOM_SCHEME_HANDLER)
