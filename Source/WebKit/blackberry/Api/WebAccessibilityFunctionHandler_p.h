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

#ifndef WebAccessibilityFunctionHandler_p_h
#define WebAccessibilityFunctionHandler_p_h

#include "AccessibilityObject.h"
#include "WebKitAccessibleWrapperBlackBerry.h"
#include "WebPageAccessible.h"
#include "WebPage_p.h"

#include <BlackBerryPlatformMisc.h>
#include <bb_a11y/rt.h>
#include <screen/screen.h>
#include <wtf/HashMap.h>
#include <wtf/RefPtr.h>

namespace BlackBerry {
namespace WebKit {

class WebAccessibilityFunctionHandlerPrivate {
public:
    WebAccessibilityFunctionHandlerPrivate(WebPagePrivate*);
    ~WebAccessibilityFunctionHandlerPrivate();

    bb_a11y_bridge_t accessibilityBridge();
    bb_a11y_accessible_t rootAccessible();
    void addAccessibilityObject(WebCore::AccessibilityObject* /*obj*/);
    void removeAccessibilityObject(WebCore::AccessibilityObject* /*obj*/);
    WebCore::AccessibilityObject* getAccessibilityObject(bb_a11y_accessible_t /*accessible*/);
    bb_a11y_accessible_t getAccessible(WebCore::AccessibilityObject* /*obj*/);

    // Events

    void emitUIEvent(WebCore::AccessibilityObject* /*obj*/, bb_a11y_ui_event_type_t /*eventType*/);
    void emitStateChangedEvent(WebCore::AccessibilityObject* /*obj*/, bb_a11y_state_t /*state*/, bool value);
    void emitTextChangedEvent(WebCore::AccessibilityObject* /*obj*/, bb_a11y_ui_event_type_t change, unsigned offset, const char* text);
    void emitValueChangedEvent(WebCore::AccessibilityObject* /*obj*/, int oldValue, int newValue);
    bool emitInputEvent(WebCore::AccessibilityObject* /*obj*/, screen_event_t /*screenEvent*/);
    void emitTextCaretMovedEvent(WebCore::AccessibilityObject* /*obj*/, int oldOffset, int newOffset);

private:
    friend class WebAccessibilityFunctionHandler;
    WebPagePrivate* m_webPage;
    WebPageAccessible* m_rootAccessible;
    WebCore::WebKitAccessibleWrapperBlackBerry* m_wrapperBlackBerry;

    typedef HashMap<RefPtr<WebCore::AccessibilityObject>, bb_a11y_accessible_t> AccessibleMap;
    AccessibleMap m_accessibleMap;
    typedef HashMap<bb_a11y_accessible_t, RefPtr<WebCore::AccessibilityObject> > AccessibilityObjectMap;
    AccessibilityObjectMap m_accessibilityObjectMap;

    DISABLE_COPY(WebAccessibilityFunctionHandlerPrivate)
};

} // namespace WebKit
} // namespace BlackBerry

#endif // WebAccessibilityFunctionHandler_p_h
