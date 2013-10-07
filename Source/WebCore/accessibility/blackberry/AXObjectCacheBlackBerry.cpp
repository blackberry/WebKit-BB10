/*
 * Copyright (C) 2013 Research In Motion Limited. All rights reserved.
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

#if HAVE(ACCESSIBILITY)

#include "AXObjectCache.h"

#include "AccessibilityObject.h"
#include "Chrome.h"
#include "ChromeClientBlackBerry.h"
#include "FrameView.h"
#include "MouseEvent.h"
#include "NotImplemented.h"
#include "Page.h"
#include "PlatformTouchEvent.h"
#include "TouchEvent.h"
#include "WebAccessibilityFunctionHandler_p.h"
#include "WebPage_p.h"

#include <bb_a11y/rt.h>

namespace WebCore {

BlackBerry::WebKit::WebAccessibilityFunctionHandlerPrivate* accessibilityHandler(AccessibilityObject* obj)
{
    Page* page = obj->page();
    if (!page)
        return 0;

    ChromeClientBlackBerry* client = static_cast<ChromeClientBlackBerry*>(page->chrome()->client());
    if (!client)
        return 0;

    return client->webPagePrivate()->accessibilityFunctionHandlerPrivate();
}

void AXObjectCache::detachWrapper(AccessibilityObject* obj)
{
    BlackBerry::WebKit::WebAccessibilityFunctionHandlerPrivate* handler = accessibilityHandler(obj);
    if (!handler)
        return;

    handler->removeAccessibilityObject(obj);
}

void AXObjectCache::attachWrapper(AccessibilityObject* obj)
{
    BlackBerry::WebKit::WebAccessibilityFunctionHandlerPrivate* handler = accessibilityHandler(obj);
    if (!handler)
        return;

    handler->addAccessibilityObject(obj);
}

void AXObjectCache::postPlatformNotification(AccessibilityObject* coreObject, AXNotification notification)
{
    if (!coreObject)
        return;

    BlackBerry::WebKit::WebAccessibilityFunctionHandlerPrivate* handler = accessibilityHandler(coreObject);
    if (!handler)
        return;

    // WebCore seems to be a bit too enthusiastic with AXCheckedStateChanged.
    // Make sure we only notify the platform for "checkable" widgets.
    if (notification == AXCheckedStateChanged && (coreObject->isCheckboxOrRadio() || coreObject->roleValue() == ToggleButtonRole)) {
        handler->emitStateChangedEvent(coreObject, BB_A11Y_STATE_CHECKED, coreObject->isChecked());

        return;
    }

    if (notification == AXValueChanged) {
        // Text widgets notify the platform via text-changed events.
        if (!coreObject->isTextControl() && !coreObject->isPasswordField())
            handler->emitValueChangedEvent(coreObject, -1, coreObject->valueForRange());
        return;
    }

    if (notification == AXSelectedChildrenChanged || notification == AXMenuListValueChanged) {
        handler->emitUIEvent(coreObject, BB_A11Y_EVENT_SELECTION_CHANGED);
        return;
    }

    if (notification == AXSelectedTextChanged) {
        handler->emitUIEvent(coreObject, BB_A11Y_EVENT_TEXT_SELECTION_CHANGED);
        return;
    }

    if (notification == AXTextCaretMoved) {
        int offset = coreObject->indexForVisiblePosition(coreObject->selection().visibleEnd());
        handler->emitTextCaretMovedEvent(coreObject, -1, offset);
        return;
    }

    if (notification == AXChildrenChanged) {
        handler->emitUIEvent(coreObject, BB_A11Y_EVENT_CHILDREN_CHANGED);
        return;
    }
}

void AXObjectCache::nodeTextChangePlatformNotification(AccessibilityObject* coreObject, AXTextChange textChange, unsigned offset, const String& text)
{
    if (!coreObject)
        return;

    AccessibilityObject* parentObject = coreObject->parentObjectUnignored();
    if (!parentObject)
        return;

    BlackBerry::WebKit::WebAccessibilityFunctionHandlerPrivate* handler = accessibilityHandler(coreObject);
    if (!handler)
        return;

    Node* node = coreObject->node();
    if (!node)
        return;

    Document* document = node->document();
    document->updateLayout();

    String textToEmit = text;
    unsigned offsetToEmit = offset;
    AccessibilityObject* accessibleToEmit = parentObject;

    if (parentObject->isPasswordField()) {
        String maskedText = parentObject->passwordFieldValue();
        textToEmit = maskedText.substring(offset, text.length());
    } else if (parentObject->isTextControl()) {
        RefPtr<Range> range = Range::create(document, node->parentNode(), 0, node, 0);
        offsetToEmit = offset + TextIterator::rangeLength(range.get());
    } else
        accessibleToEmit = coreObject;

    if (textChange == AXObjectCache::AXTextInserted) {
        handler->emitTextChangedEvent(accessibleToEmit, BB_A11Y_EVENT_TEXT_INSERTED, offsetToEmit, textToEmit.utf8().data());
        return;
    }

    if (textChange == AXObjectCache::AXTextDeleted) {
        handler->emitTextChangedEvent(accessibleToEmit, BB_A11Y_EVENT_TEXT_REMOVED, offsetToEmit, textToEmit.utf8().data());
        return;
    }
}

void AXObjectCache::frameLoadingEventPlatformNotification(AccessibilityObject* coreObject, AXLoadingEvent loadingEvent)
{
    if (!coreObject)
        return;

    BlackBerry::WebKit::WebAccessibilityFunctionHandlerPrivate* handler = accessibilityHandler(coreObject);
    if (!handler)
        return;

    if (loadingEvent == AXObjectCache::AXLoadingStarted) {
        handler->emitStateChangedEvent(coreObject, BB_A11Y_STATE_BUSY, true);
        return;
    }

    if (loadingEvent == AXObjectCache::AXLoadingReloaded) {
        handler->emitStateChangedEvent(coreObject, BB_A11Y_STATE_BUSY, true);
        handler->emitUIEvent(coreObject, BB_A11Y_EVENT_DOCUMENT_RELOAD);
        return;
    }

    if (loadingEvent == AXObjectCache::AXLoadingFailed) {
        handler->emitStateChangedEvent(coreObject, BB_A11Y_STATE_BUSY, false);
        handler->emitUIEvent(coreObject, BB_A11Y_EVENT_DOCUMENT_LOAD_STOPPED);
        return;
    }

    if (loadingEvent == AXObjectCache::AXLoadingFinished) {
        handler->emitStateChangedEvent(coreObject, BB_A11Y_STATE_BUSY, false);
        handler->emitUIEvent(coreObject, BB_A11Y_EVENT_DOCUMENT_LOAD_COMPLETE);
    }
}

void AXObjectCache::handleFocusedUIElementChanged(Node* oldFocusedNode, Node* newFocusedNode)
{
    AccessibilityObject* oldObject = getOrCreate(oldFocusedNode);
    if (oldObject) {
        BlackBerry::WebKit::WebAccessibilityFunctionHandlerPrivate* handler = accessibilityHandler(oldObject);
        if (!handler)
            return;
        handler->emitStateChangedEvent(oldObject, BB_A11Y_STATE_FOCUSED, false);
    }

    AccessibilityObject* newObject = getOrCreate(newFocusedNode);
    if (newObject) {
        BlackBerry::WebKit::WebAccessibilityFunctionHandlerPrivate* handler = accessibilityHandler(newObject);
        if (!handler)
            return;
        handler->emitStateChangedEvent(newObject, BB_A11Y_STATE_FOCUSED, true);
    }
}

void AXObjectCache::handleScrolledToAnchor(const Node*)
{
    notImplemented();
}

} // namespace WebCore

#endif // HAVE(ACCESSIBILITY)
