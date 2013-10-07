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

#include "AccessibilityObject.h"
#include "AccessibilityScrollView.h"
#include "Chrome.h"
#include "ChromeClientBlackBerry.h"
#include "DOMSupport.h"
#include "Document.h"
#include "Frame.h"
#include "FrameView.h"
#include "IntRect.h"
#include "Node.h"
#include "NotImplemented.h"
#include "Page.h"
#include "PlatformMouseEvent.h"
#include "PlatformTouchEvent.h"
#include "RenderBlock.h"
#include "RenderLayer.h"
#include "RenderObject.h"
#include "ScrollView.h"
#include "WebKitAccessibleWrapperBlackBerry.h"
#include "WebPage.h"
#include "WebPage_p.h"

#include <BlackBerryPlatformTouchEvent.h>
#include <BlackBerryPlatformViewportAccessor.h>

using namespace WebCore;

IntPoint WebKitAccessibleWrapperBlackBerry::pointForCoordinateType(AccessibilityObject* coreObject, bb_a11y_coordinate_type_t coordinateType, int x, int y)
{
    IntPoint point(x, y);
    FrameView* frameView = coreObject->documentFrameView();
    if (frameView) {
        switch (coordinateType) {
        case BB_A11Y_COORDINATE_TYPE_SCREEN:
            return frameView->screenToContents(point);
        case BB_A11Y_COORDINATE_TYPE_BUFFER:
            return frameView->windowToContents(point);
        }
    }
    return point;
}

IntRect WebKitAccessibleWrapperBlackBerry::rectForCoordinateType(AccessibilityObject* coreObject, bb_a11y_coordinate_type_t coordinateType, IntRect rect)
{
    FrameView* frameView = coreObject->documentFrameView();
    if (frameView) {
        switch (coordinateType) {
        case BB_A11Y_COORDINATE_TYPE_SCREEN:
            return frameView->contentsToScreen(rect);
        case BB_A11Y_COORDINATE_TYPE_BUFFER:
            return frameView->contentsToWindow(rect);
        }
    }
    return rect;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::isComponent(const bb_a11y_accessible_t accessible, bool* result)
{
    if (getAccessibilityObject(accessible))
        *result = true;
    else
        *result = false;

    return BB_A11Y_STATUS_OK;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::grabFocus(const bb_a11y_accessible_t accessible)
{
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    if (!coreObject)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    coreObject->setFocused(true);
    return BB_A11Y_STATUS_OK;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::getAccessibleAtPoint(const bb_a11y_accessible_t accessible, int x, int y, bb_a11y_coordinate_type_t coordinateType, bb_a11y_accessible_t* result)
{
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);

    if (!coreObject)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    IntPoint point = pointForCoordinateType(coreObject, coordinateType, x, y);
    AccessibilityObject* target = coreObject->accessibilityHitTest(point);
    *result = target ? getAccessible(target) : 0;

    return BB_A11Y_STATUS_OK;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::containsPoint(const bb_a11y_accessible_t accessible, int x, int y, bb_a11y_coordinate_type_t coordinateType, bool* result)
{
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    if (!coreObject)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;
    IntRect rect = rectForCoordinateType(coreObject, coordinateType, coreObject->pixelSnappedElementRect());
    *result = rect.contains(x, y);
    return BB_A11Y_STATUS_OK;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::getExtents(const bb_a11y_accessible_t accessible, bb_a11y_coordinate_type_t coordinateType, int* x, int* y, int* width, int* height)
{
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    if (!coreObject)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    IntRect rect = rectForCoordinateType(coreObject, coordinateType, coreObject->pixelSnappedElementRect());
    *x = rect.x();
    *y = rect.y();
    *width = rect.width();
    *height = rect.height();
    return BB_A11Y_STATUS_OK;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::getZOrder(const bb_a11y_accessible_t accessible, int* result)
{
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    if (!coreObject)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    RenderObject* renderer = coreObject->renderer();
    *result = renderer ? renderer->style()->zIndex() : 0;
    return BB_A11Y_STATUS_OK;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::getAttributes(const bb_a11y_accessible_t accessible, char* axAttributes, int maxSize, char** overflow)
{
    // This should be made up of name:value pairs and is a means by which to
    // expose things for which there is no other suitable API in the framework.
    String attributesString;
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    if (!coreObject)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    int headingLevel = coreObject->headingLevel();
    if (headingLevel)
        attributesString.append(String::format("level:%i;", headingLevel));

    return static_cast<bb_a11y_status_t>(bb_a11y_bridge_copy_string(attributesString.utf8().data(), axAttributes, maxSize, overflow));
}

static ScrollView* getScrollView(AccessibilityObject* coreObject)
{
    if (!coreObject)
        return 0;

    AccessibilityObject* obj = coreObject;
    while (obj && !obj->isAccessibilityScrollView())
        obj = obj->parentObject();

    if (!obj || !obj->isAccessibilityScrollView())
        return 0;

    return toAccessibilityScrollView(obj)->scrollView();
}

// The RenderBox::canbeScrolledAndHasScrollableArea method returns true for the
// following scenario, for example:
// (1) a div that has a vertical overflow but no horizontal overflow
//     with overflow-y: hidden and overflow-x: auto set.
// The version below fixes it.
// FIXME: Fix RenderBox::canBeScrolledAndHasScrollableArea method instead.
static bool checkIfScrollable(const RenderBox* box)
{
    if (!box)
        return false;

    // We use this to make non-overflown contents layers to actually
    // be overscrollable.
    if (box->layer() && box->layer()->usesCompositedScrolling()) {
        if (box->style()->overflowScrolling() == OSBlackberryTouch)
            return true;
    }

    if (!box->hasOverflowClip())
        return false;

    if (box->scrollHeight() == box->clientHeight() && box->scrollWidth() == box->clientWidth())
        return false;

    if ((box->scrollsOverflowX() && (box->scrollWidth() != box->clientWidth()))
        || (box->scrollsOverflowY() && (box->scrollHeight() != box->clientHeight())))
        return true;

    Node* node = box->node();
    return node && (BlackBerry::WebKit::DOMSupport::isShadowHostTextInputElement(node) || node->isDocumentNode());
}

// Traverse up the RenderBlock tree to find a block that can be scrolled
// FIXME: Ideally we should use the RenderLayer for better performance, but this will do
// as a temporary fix. In the next patch, we will try to send the scroll events to libwebview instead.
static bool checkIfPartOfScrollableNode(RenderBox* box)
{
    RenderBlock* b = box->containingBlock();
    while (b && !b->isRenderView()) {
        if (checkIfScrollable(b))
            return true;

        b = b->containingBlock();
    }

    return false;
}

static bb_a11y_status_t scroll(AccessibilityObject* coreObject, bb_a11y_activate_type_t scrollType)
{
    Page* page = coreObject->page();
    if (!page)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    ChromeClientBlackBerry* client = static_cast<ChromeClientBlackBerry*>(page->chrome()->client());
    if (!client)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    BlackBerry::WebKit::WebPagePrivate* webPagePrivate = client->webPagePrivate();
    if (!webPagePrivate)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    // Attempt to scroll the enclosing box that wraps the node
    if (Node* node = coreObject->node()) {
        if (RenderObject* renderer = node->renderer()) {
            if (RenderBox* box = renderer->enclosingBox()) {
                if (checkIfPartOfScrollableNode(box)) {
                    switch (scrollType) {
                    case BB_A11Y_COMPONENT_ACTIVATE_PAGE_UP:
                    case BB_A11Y_COMPONENT_ACTIVATE_SWIPE_UP:
                        box->scroll(ScrollUp, ScrollByPage);
                        break;
                    case BB_A11Y_COMPONENT_ACTIVATE_PAGE_DOWN:
                    case BB_A11Y_COMPONENT_ACTIVATE_SWIPE_DOWN:
                        box->scroll(ScrollDown, ScrollByPage);
                        break;
                    case BB_A11Y_COMPONENT_ACTIVATE_PAGE_LEFT:
                    case BB_A11Y_COMPONENT_ACTIVATE_SWIPE_LEFT:
                        box->scroll(ScrollLeft, ScrollByPage);
                        break;
                    case BB_A11Y_COMPONENT_ACTIVATE_PAGE_RIGHT:
                    case BB_A11Y_COMPONENT_ACTIVATE_SWIPE_RIGHT:
                        box->scroll(ScrollRight, ScrollByPage);
                        break;
                    default:
                        break;
                    }

                    return BB_A11Y_STATUS_OK;
                }
            }
        }
    }

    ScrollView* scrollView = getScrollView(coreObject);
    if (!scrollView)
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    IntPoint delta;
    switch (scrollType) {
    case BB_A11Y_COMPONENT_ACTIVATE_PAGE_UP:
    case BB_A11Y_COMPONENT_ACTIVATE_SWIPE_UP:
        delta = IntPoint(0, -scrollView->visibleHeight());
        break;
    case BB_A11Y_COMPONENT_ACTIVATE_PAGE_DOWN:
    case BB_A11Y_COMPONENT_ACTIVATE_SWIPE_DOWN:
        delta = IntPoint(0, scrollView->visibleHeight());
        break;
    case BB_A11Y_COMPONENT_ACTIVATE_PAGE_LEFT:
    case BB_A11Y_COMPONENT_ACTIVATE_SWIPE_LEFT:
        delta = IntPoint(-scrollView->visibleWidth(), 0);
        break;
    case BB_A11Y_COMPONENT_ACTIVATE_PAGE_RIGHT:
    case BB_A11Y_COMPONENT_ACTIVATE_SWIPE_RIGHT:
        delta = IntPoint(scrollView->visibleWidth(), 0);
        break;
    case BB_A11Y_COMPONENT_ACTIVATE_HOME:
        delta = IntPoint(scrollView->minimumScrollPosition().x(), scrollView->minimumScrollPosition().y());
        break;
    case BB_A11Y_COMPONENT_ACTIVATE_END:
        delta = IntPoint(scrollView->maximumScrollPosition().x(), scrollView->maximumScrollPosition().y());
        break;
    default:
        return BB_A11Y_STATUS_NOT_SUPPORTED;
    }

    // If we have a backingstore to render to, scroll it using the scrollview. If not, we dispatch to the client to handle the scroll.
    if (webPagePrivate->m_client->hasView()) {
        IntPoint newPosition = scrollView->scrollPosition() + delta;
        newPosition.setX(std::max(scrollView->minimumScrollPosition().x(), std::min(newPosition.x(), scrollView->maximumScrollPosition().x())));
        newPosition.setY(std::max(scrollView->minimumScrollPosition().y(), std::min(newPosition.y(), scrollView->maximumScrollPosition().y())));
        scrollView->setScrollPosition(newPosition);
    } else
        webPagePrivate->m_client->scrollByCompositor(delta.x(), delta.y());

    return BB_A11Y_STATUS_OK;
}

static bb_a11y_status_t tap(AccessibilityObject* coreObject, int tapCount)
{
    Page* page = coreObject->page();
    if (!page)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    ChromeClientBlackBerry* client = static_cast<ChromeClientBlackBerry*>(page->chrome()->client());
    if (!client)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    BlackBerry::WebKit::WebPagePrivate* webPagePrivate = client->webPagePrivate();
    if (!webPagePrivate)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    Node* node = coreObject->node();
    if (!node)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    // Generate Touch Events
    BlackBerry::Platform::TouchEvent touchStartEvent;
    touchStartEvent.m_type = BlackBerry::Platform::TouchEvent::TouchStart;
    touchStartEvent.m_neverHadMultiTouch = true;
    touchStartEvent.m_hasAnyMovement = false;

    BlackBerry::Platform::TouchEvent touchEndEvent = touchStartEvent;
    touchEndEvent.m_type = BlackBerry::Platform::TouchEvent::TouchEnd;
    touchEndEvent.m_gestures = BlackBerry::Platform::TouchEvent::Tap;

    BlackBerry::Platform::TouchEvent touchInjectedEvent = touchEndEvent;
    touchInjectedEvent.m_type = BlackBerry::Platform::TouchEvent::TouchInjected;
    touchInjectedEvent.m_gestures = BlackBerry::Platform::TouchEvent::TapSequenceComplete;
    touchInjectedEvent.m_tapCount = tapCount;

    IntRect documentRect = coreObject->pixelSnappedBoundingBoxRect();
    BlackBerry::Platform::IntRect pixelViewPortRect = BlackBerry::Platform::IntRect(documentRect.x(), documentRect.y(), documentRect.width(), documentRect.height());
    BlackBerry::Platform::IntRect screenRect;
    BlackBerry::Platform::ViewportAccessor* viewportAccessor = webPagePrivate->m_webPage->webkitThreadViewportAccessor();

    pixelViewPortRect = viewportAccessor->roundToPixelFromDocumentContents(FloatRect(documentRect));
    pixelViewPortRect = viewportAccessor->pixelViewportFromContents(pixelViewPortRect);
    screenRect = pixelViewPortRect;

    // Calculation of the screen coordinates differs between Browser and Cascade WebViews
    if (!webPagePrivate->m_client->hasView()) {
        webPagePrivate->m_webPage->convertToWindowCoordinates(screenRect);
        webPagePrivate->m_client->windowToScreenCoordinates(screenRect);
    }

    IntPoint pixelViewPortCenter = pixelViewPortRect.center();
    IntPoint screenRectCenter = screenRect.center();
    BlackBerry::Platform::TouchPoint pressPoint = BlackBerry::Platform::TouchPoint(0, BlackBerry::Platform::TouchPoint::TouchPressed, pixelViewPortCenter, screenRectCenter, currentTime());
    BlackBerry::Platform::TouchPoint releasePoint = BlackBerry::Platform::TouchPoint(0, BlackBerry::Platform::TouchPoint::TouchReleased, pixelViewPortCenter, screenRectCenter, currentTime());

    touchStartEvent.m_points.push_back(pressPoint);
    touchEndEvent.m_points.push_back(releasePoint);
    touchInjectedEvent.m_points.push_back(releasePoint);

    // Dispatch tap events
    for (int i = 0; i < tapCount; i++) {
        touchStartEvent.m_tapCount = i;
        webPagePrivate->m_client->queueAccessibilityTouchEvent(touchStartEvent);
        touchEndEvent.m_tapCount = i + 1;
        webPagePrivate->m_client->queueAccessibilityTouchEvent(touchEndEvent);
    }

    webPagePrivate->m_client->queueAccessibilityTouchEvent(touchInjectedEvent);
    return BB_A11Y_STATUS_OK;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::activate(const bb_a11y_accessible_t accessible, bb_a11y_activate_type_t activateType)
{
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    if (!coreObject)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    // TODO: Implement this.
    switch (activateType) {
    case BB_A11Y_COMPONENT_ACTIVATE_TAP:
        return tap(coreObject, 1);
    case BB_A11Y_COMPONENT_ACTIVATE_DOUBLE_TAP:
        return tap(coreObject, 2);
    case BB_A11Y_COMPONENT_ACTIVATE_SWIPE_UP:
    case BB_A11Y_COMPONENT_ACTIVATE_SWIPE_DOWN:
    case BB_A11Y_COMPONENT_ACTIVATE_SWIPE_LEFT:
    case BB_A11Y_COMPONENT_ACTIVATE_SWIPE_RIGHT:
    case BB_A11Y_COMPONENT_ACTIVATE_HOME:
    case BB_A11Y_COMPONENT_ACTIVATE_END:
    case BB_A11Y_COMPONENT_ACTIVATE_PAGE_UP:
    case BB_A11Y_COMPONENT_ACTIVATE_PAGE_DOWN:
    case BB_A11Y_COMPONENT_ACTIVATE_PAGE_LEFT:
    case BB_A11Y_COMPONENT_ACTIVATE_PAGE_RIGHT:
        return scroll(coreObject, activateType);
    case BB_A11Y_COMPONENT_ACTIVATE_LONG_PRESS:
    case BB_A11Y_COMPONENT_ACTIVATE_RELEASE:
    case BB_A11Y_COMPONENT_ACTIVATE_FORWARD:
    case BB_A11Y_COMPONENT_ACTIVATE_BACKWARD:
    case BB_A11Y_COMPONENT_ACTIVATE_DEFAULT:
        break;
    default:
        break;
    }

    return BB_A11Y_STATUS_OK;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::highlight(bb_a11y_accessible_t /*accessible*/, bool /*b*/)
{
    notImplemented();

    return BB_A11Y_STATUS_NOT_SUPPORTED;
}

#endif // HAVE(ACCESSIBILITY)
