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
#include "HTMLNames.h"
#include "HTMLTextFormControlElement.h"
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
#include "Text.h"
#include "VisibleUnits.h"
#include "WebKitAccessibleWrapperBlackBerry.h"
#include "WebPage.h"
#include "WebPage_p.h"

#include <BlackBerryPlatformTouchEvent.h>
#include <BlackBerryPlatformViewportAccessor.h>

using namespace WebCore;

static const int s_touchMoveDelta = 60;

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
static RenderBox* checkIfPartOfScrollableNode(AccessibilityObject* accessibilityObject)
{
    RenderObject* objectRenderer = accessibilityObject->renderer();
    if (!objectRenderer)
        return 0;

    RenderBlock* objectBlock = objectRenderer->containingBlock();

    while (objectBlock && !objectBlock->isRenderView()) {
        if (checkIfScrollable(objectBlock))
            return objectBlock;
        objectBlock = objectBlock->containingBlock();
    }

    return 0;
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

    IntRect documentRect;

    // Swipe the scrollview or scrollable renderbox up in the hierarchy
    RenderBox* box = checkIfPartOfScrollableNode(coreObject);
    if (box) {
        // Contract the size of the rect by 10%. Elements sometimes won't scroll if
        // the swipe starts on the border of the element.
        documentRect = box->absoluteContentBox();
        documentRect.inflateX(-ceil(documentRect.width() * 0.05));
        documentRect.inflateY(-ceil(documentRect.height() * 0.05));
    } else
        documentRect = getScrollView(coreObject)->visibleContentRect();

    if (documentRect.isEmpty())
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    BlackBerry::Platform::ViewportAccessor* viewportAccessor = webPagePrivate->m_webPage->webkitThreadViewportAccessor();

    BlackBerry::Platform::IntRect pixelViewPortRect = viewportAccessor->roundToPixelFromDocumentContents(FloatRect(documentRect));
    pixelViewPortRect = viewportAccessor->pixelViewportFromContents(pixelViewPortRect);
    BlackBerry::Platform::IntRect screenRect = pixelViewPortRect;

    // Calculation of the screen coordinates differs between Browser and Cascade WebViews
    if (!webPagePrivate->m_client->hasView())
        webPagePrivate->m_webPage->convertToWindowCoordinates(screenRect);

    webPagePrivate->m_client->windowToScreenCoordinates(screenRect);

    // Calculate total delta to scroll
    int totalDX = 0;
    int totalDY = 0;
    int direction = 1;
    IntPoint pixelTouchPoint;
    IntPoint screenTouchPoint;

    switch (scrollType) {
    case BB_A11Y_COMPONENT_ACTIVATE_PAGE_UP:
    case BB_A11Y_COMPONENT_ACTIVATE_SWIPE_UP:
        pixelTouchPoint = pixelViewPortRect.topLeft();
        screenTouchPoint = screenRect.topLeft();
        totalDY = pixelViewPortRect.height();
        break;
    case BB_A11Y_COMPONENT_ACTIVATE_PAGE_DOWN:
    case BB_A11Y_COMPONENT_ACTIVATE_SWIPE_DOWN:
        pixelTouchPoint = pixelViewPortRect.bottomLeft();
        screenTouchPoint = screenRect.bottomLeft();
        totalDY = pixelViewPortRect.height();
        direction = -1;
        break;
    case BB_A11Y_COMPONENT_ACTIVATE_PAGE_LEFT:
    case BB_A11Y_COMPONENT_ACTIVATE_SWIPE_LEFT:
        pixelTouchPoint = pixelViewPortRect.topLeft();
        screenTouchPoint = screenRect.topLeft();
        totalDX = pixelViewPortRect.width();
        break;
    case BB_A11Y_COMPONENT_ACTIVATE_PAGE_RIGHT:
    case BB_A11Y_COMPONENT_ACTIVATE_SWIPE_RIGHT:
        pixelTouchPoint = pixelViewPortRect.topRight();
        screenTouchPoint = screenRect.topRight();
        totalDX = pixelViewPortRect.width();
        direction = -1;
        break;
    case BB_A11Y_COMPONENT_ACTIVATE_HOME:
    case BB_A11Y_COMPONENT_ACTIVATE_END:
    default:
        return BB_A11Y_STATUS_NOT_SUPPORTED;
    }

    // If WebView is in Cascades, throw it to the client to handle.
    if (!webPagePrivate->m_client->hasView()) {
        direction = -direction;
        webPagePrivate->m_client->scrollByCompositor(totalDX * direction, totalDY * direction);
        return BB_A11Y_STATUS_OK;
    }


    // Send TouchStart event
    BlackBerry::Platform::TouchEvent touchStartEvent;
    touchStartEvent.m_type = BlackBerry::Platform::TouchEvent::TouchStart;
    BlackBerry::Platform::TouchPoint pressPoint = BlackBerry::Platform::TouchPoint(0, BlackBerry::Platform::TouchPoint::TouchPressed, pixelTouchPoint, screenTouchPoint, currentTime());
    touchStartEvent.m_neverHadMultiTouch = true;
    touchStartEvent.m_hasAnyMovement = false;
    touchStartEvent.m_points.push_back(pressPoint);
    webPagePrivate->m_client->queueAccessibilityTouchEvent(touchStartEvent);

    // Send TouchMove events
    BlackBerry::Platform::TouchEvent touchMovedEvent;
    touchMovedEvent.m_type = BlackBerry::Platform::TouchEvent::TouchMove;
    touchMovedEvent.m_hasAnyMovement = true;

    int dx;
    int dy;
    while (totalDX > 0  || totalDY > 0) {
        // Clip dx or dy to make sure we don't exceed total delta
        dx = 0;
        dy = 0;

        if (totalDX > 0) {
            totalDX -= s_touchMoveDelta;
            dx = (totalDX < 0) ? s_touchMoveDelta + totalDX : s_touchMoveDelta;
        } else {
            totalDY -= s_touchMoveDelta;
            dy = (totalDY < 0) ? s_touchMoveDelta + totalDY : s_touchMoveDelta;
        }

        touchMovedEvent.m_points.clear();
        pixelTouchPoint.move(dx * direction, dy * direction);
        screenTouchPoint.move(dx * direction, dy * direction);
        BlackBerry::Platform::TouchPoint movePoint = BlackBerry::Platform::TouchPoint(0, BlackBerry::Platform::TouchPoint::TouchMoved, pixelTouchPoint, screenTouchPoint, currentTime());
        touchMovedEvent.m_points.push_back(movePoint);
        webPagePrivate->m_client->queueAccessibilityTouchEvent(touchMovedEvent);
    }

    // Send TouchEnd event
    BlackBerry::Platform::TouchEvent touchEndEvent;
    touchEndEvent.m_type = BlackBerry::Platform::TouchEvent::TouchEnd;
    touchEndEvent.m_gestures = BlackBerry::Platform::TouchEvent::TapSequenceComplete;

    BlackBerry::Platform::TouchPoint releasePoint = BlackBerry::Platform::TouchPoint(0, BlackBerry::Platform::TouchPoint::TouchReleased, pixelTouchPoint, screenTouchPoint, currentTime());
    touchEndEvent.m_points.push_back(releasePoint);
    webPagePrivate->m_client->queueAccessibilityTouchEvent(touchEndEvent);

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

    IntRect documentRect = node->pixelSnappedBoundingBox();
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

    IntPoint viewportPoint = viewportAccessor->documentViewportFromContents(BlackBerry::Platform::IntPoint(documentRect.center().x(), documentRect.center().y()));
    pressPoint.populateDocumentPosition(viewportPoint, documentRect.center());
    releasePoint.populateDocumentPosition(viewportPoint, documentRect.center());

    touchStartEvent.m_points.push_back(pressPoint);
    touchEndEvent.m_points.push_back(releasePoint);
    touchInjectedEvent.m_points.push_back(releasePoint);

    HTMLTextFormControlElement* inputElement;
    bool isTextFormControl = coreObject->isTextControl() && (inputElement = static_cast<HTMLTextFormControlElement*>(node));
    if (tapCount == 1 && (isTextFormControl || node->isContentEditable())) {
        // Dispatch Touch and Mouse events from the node since we won't be sending this event to libwebview
        webPagePrivate->m_mainFrame->eventHandler()->handleTouchEvent(PlatformTouchEvent(&touchStartEvent));
        webPagePrivate->m_mainFrame->eventHandler()->handleTouchEvent(PlatformTouchEvent(&touchEndEvent));

        PlatformMouseEvent mousePressed(viewportPoint, pressPoint.screenPosition(), PlatformEvent::MousePressed, 1, LeftButton, false, false, false, TouchScreen);
        PlatformMouseEvent mouseReleased(viewportPoint, pressPoint.screenPosition(), PlatformEvent::MouseReleased, 1, LeftButton, false, false, false, TouchScreen);
        webPagePrivate->handleMouseEvent(mousePressed);
        webPagePrivate->handleMouseEvent(mouseReleased);

        if (isTextFormControl) {
            int textLength = coreObject->textLength();
            inputElement->setSelectionRange(textLength, textLength);
        } else {
            VisiblePosition caretPosition = VisiblePosition(lastPositionInNode(node), DOWNSTREAM);
            caretPosition = endOfParagraph(caretPosition);
            webPagePrivate->m_mainFrame->selection()->setSelection(VisibleSelection(caretPosition, caretPosition), FrameSelection::CloseTyping | FrameSelection::ClearTypingStyle);
        }

        webPagePrivate->m_mainFrame->selection()->revealSelection(ScrollAlignment::alignToEdgeIfNeeded);

    } else {
        // Dispatch tap events
        for (int i = 0; i < tapCount; i++) {
            touchStartEvent.m_tapCount = i;
            webPagePrivate->m_client->queueAccessibilityTouchEvent(touchStartEvent);
            touchEndEvent.m_tapCount = i + 1;
            webPagePrivate->m_client->queueAccessibilityTouchEvent(touchEndEvent);
        }

        webPagePrivate->m_client->queueAccessibilityTouchEvent(touchInjectedEvent);
    }

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
