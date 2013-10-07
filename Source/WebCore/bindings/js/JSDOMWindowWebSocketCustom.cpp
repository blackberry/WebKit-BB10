/*
 * Copyright (C) 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Google Inc. All rights reserved.
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 *
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "JSDOMWindowCustom.h"

#if ENABLE(WEB_SOCKETS)
#include "Frame.h"
#include "JSWebSocket.h"
#include "Settings.h"

#if PLATFORM(BLACKBERRY)
#include "WebSocket.h"
#endif

using namespace JSC;

namespace WebCore {

static Settings* settingsForWindowWebSocket(const JSDOMWindow* window)
{
    ASSERT(window);
    if (Frame* frame = window->impl()->frame())
        return frame->settings();
    return 0;
}

JSValue JSDOMWindow::webSocket(ExecState* exec) const
{
    if (!settingsForWindowWebSocket(this))
        return jsUndefined();
#if PLATFORM(BLACKBERRY)
    // FIXME: returning undefined is not the same things as disabling the
    // WebSocket object completely. But, this is the best we can do without
    // changing the code generation to check RuntimeEnabledFeatures, like V8
    // does, which is too big a job at the moment. Remove this check when
    // https://bugs.webkit.org/show_bug.cgi?id=52011 is fixed.
    if (!WebSocket::isAvailable())
        return jsUndefined();
#endif
    return getDOMConstructor<JSWebSocketConstructor>(exec, this);
}

} // namespace WebCore

#endif
