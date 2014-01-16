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
#include "WebAccessibilityFunctionHandler.h"

#include "WebAccessibilityFunctionHandler_p.h"
#include "WebKitAccessibleWrapperBlackBerry.h"
#include "WebPage_p.h"

#define ErrorLog(format, ...) BlackBerry::Platform::logAlways(BlackBerry::Platform::LogLevelCritical, format, ## __VA_ARGS__)

namespace BlackBerry {
namespace WebKit {

WebAccessibilityFunctionHandler::WebAccessibilityFunctionHandler(WebPagePrivate* webPagePrivate)
    : d(new WebAccessibilityFunctionHandlerPrivate(webPagePrivate))
{
    ASSERT(webPagePrivate);
}

WebAccessibilityFunctionHandler::~WebAccessibilityFunctionHandler()
{
    delete d;
    d = 0;
}

WebAccessibilityFunctionHandlerPrivate::WebAccessibilityFunctionHandlerPrivate(WebPagePrivate* webPagePrivate)
    : m_webPage(webPagePrivate)
{
    WebPageClient* client = m_webPage->m_client;
    bb_a11y_accessible_t accessible = client->createAccessible();
    m_rootAccessible = new WebPageAccessible(accessible, m_webPage->m_webPage);
    m_wrapperBlackBerry = new WebCore::WebKitAccessibleWrapperBlackBerry(this);

    client->addAccessible(accessible);
    client->registerRootAccessible(accessible);
}

WebAccessibilityFunctionHandlerPrivate::~WebAccessibilityFunctionHandlerPrivate()
{
    bb_a11y_accessible_t accessible = m_rootAccessible->accessible();
    WebPageClient* client = m_webPage->m_client;
    client->unregisterRootAccessible(accessible);
    client->removeAccessible(accessible);

    delete m_rootAccessible;
    m_rootAccessible = 0;
    delete m_wrapperBlackBerry;
    m_wrapperBlackBerry = 0;
}

bb_a11y_accessible_t WebAccessibilityFunctionHandlerPrivate::rootAccessible()
{
    return m_rootAccessible->accessible();
}

bb_a11y_bridge_t WebAccessibilityFunctionHandlerPrivate::accessibilityBridge()
{
    return m_webPage->m_client->accessibilityBridge();
}

void WebAccessibilityFunctionHandlerPrivate::addAccessibilityObject(WebCore::AccessibilityObject* obj)
{
    bb_a11y_accessible_t accessible = m_webPage->m_client->createAccessible();
    m_accessibleMap.set(obj, accessible);
    m_accessibilityObjectMap.set(accessible, obj);
    m_webPage->m_client->addAccessible(accessible);
}

void WebAccessibilityFunctionHandlerPrivate::removeAccessibilityObject(WebCore::AccessibilityObject* obj)
{
    bb_a11y_accessible_t accessible = m_accessibleMap.take(obj);
    if (!accessible)
        return;

    m_accessibilityObjectMap.remove(accessible);
    m_webPage->m_client->removeAccessible(accessible);
}

WebCore::AccessibilityObject* WebAccessibilityFunctionHandlerPrivate::getAccessibilityObject(bb_a11y_accessible_t accessible)
{
    return m_accessibilityObjectMap.get(accessible).get();
}

bb_a11y_accessible_t WebAccessibilityFunctionHandlerPrivate::getAccessible(WebCore::AccessibilityObject* obj)
{
    return m_accessibleMap.get(obj);
}

// Accessible

bb_a11y_status_t WebAccessibilityFunctionHandler::getLanguage(bb_a11y_accessible_t accessible, char* language, int languageMaxSize, char** languageOverflow)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->getLanguage(accessible, language, languageMaxSize, languageOverflow);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::getId(bb_a11y_accessible_t accessible, char* id, int idMaxSize, char** idOverflow)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->getId(accessible, id, idMaxSize, idOverflow);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::findId(bb_a11y_accessible_t parent, const char* id, bb_a11y_accessible_t* accessible)
{
    if (parent == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->findId(parent, id, accessible);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::getName(bb_a11y_accessible_t accessible, char *name, int name_max_size, char **name_overflow)
{
    if (accessible == d->m_rootAccessible->accessible())
        return d->m_rootAccessible->getName(name, name_max_size, name_overflow);

    return d->m_wrapperBlackBerry->getName(accessible, name, name_max_size, name_overflow);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::getDescription(bb_a11y_accessible_t accessible, char* description, int descriptionMaxSize, char** descriptionOverflow)
{
    if (accessible == d->m_rootAccessible->accessible())
        return d->m_rootAccessible->getDescription(description, descriptionMaxSize, descriptionOverflow);

    return d->m_wrapperBlackBerry->getDescription(accessible, description, descriptionMaxSize, descriptionOverflow);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::getParent(bb_a11y_accessible_t child, bb_a11y_accessible_t* parent)
{
    if (child == d->m_rootAccessible->accessible())
        return d->m_rootAccessible->getParent(parent);

    return d->m_wrapperBlackBerry->getParent(child, parent);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::getIndexInParent(bb_a11y_accessible_t child, int* index)
{
    if (child == d->m_rootAccessible->accessible())
        return d->m_rootAccessible->getIndexInParent(index);

    return d->m_wrapperBlackBerry->getIndexInParent(child, index);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::getChildCount(bb_a11y_accessible_t parent, int* childCount)
{
    if (parent == d->m_rootAccessible->accessible())
        return d->m_rootAccessible->getChildCount(childCount);

    return d->m_wrapperBlackBerry->getChildCount(parent, childCount);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::getChildAtIndex(bb_a11y_accessible_t parent, unsigned index, bb_a11y_accessible_t *child)
{
    if (parent == d->m_rootAccessible->accessible())
        return d->m_rootAccessible->getChildAtIndex(index, child);

    return d->m_wrapperBlackBerry->getChildAtIndex(parent, index, child);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::getRole(bb_a11y_accessible_t accessible, bb_a11y_role_t* role, char* extendedRole, int extendedRoleMaxSize, char** extendedRoleOverflow)
{
    if (accessible == d->m_rootAccessible->accessible())
        return d->m_rootAccessible->getRole(role, extendedRole, extendedRoleMaxSize, extendedRoleOverflow);

    return d->m_wrapperBlackBerry->getRole(accessible, role, extendedRole, extendedRoleMaxSize, extendedRoleOverflow);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::getState(bb_a11y_accessible_t accessible, _Uint64t* state)
{
    if (accessible == d->m_rootAccessible->accessible())
        return d->m_rootAccessible->getState(state);

    return d->m_wrapperBlackBerry->getState(accessible, state);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::getRelations(bb_a11y_accessible_t accessible, bb_a11y_relation_t relation, bb_a11y_accessible_t** relations, int* numRelations)
{
    if (accessible == d->m_rootAccessible->accessible())
        return d->m_rootAccessible->getRelations(relation, relations, numRelations);

    return d->m_wrapperBlackBerry->getRelations(accessible, relation, relations, numRelations);
}

// Component

bb_a11y_status_t WebAccessibilityFunctionHandler::isComponent(bb_a11y_accessible_t accessible, bool* result)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->isComponent(accessible, result);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::getAccessibleAtPoint(bb_a11y_accessible_t parent, int x, int y, bb_a11y_coordinate_type_t coordinateType, bb_a11y_accessible_t* accessible)
{
    if (parent == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->getAccessibleAtPoint(parent, x, y, coordinateType, accessible);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::containsPoint(bb_a11y_accessible_t accessible, int x, int y, bb_a11y_coordinate_type_t coordinateType, bool* result)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->containsPoint(accessible, x, y, coordinateType, result);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::getAttributes(bb_a11y_accessible_t accessible, char* attributes, int attributesMaxSize, char** attributesOverflow)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->getAttributes(accessible, attributes, attributesMaxSize, attributesOverflow);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::getExtents(bb_a11y_accessible_t accessible, bb_a11y_coordinate_type_t coordinateType, int* x, int* y, int* width, int* height)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->getExtents(accessible, coordinateType, x, y, width, height);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::getZOrder(bb_a11y_accessible_t accessible, int* zOrder)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->getZOrder(accessible, zOrder);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::grabFocus(bb_a11y_accessible_t accessible)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->grabFocus(accessible);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::activate(bb_a11y_accessible_t accessible, bb_a11y_activate_type_t activateType)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->activate(accessible, activateType);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::highlight(bb_a11y_accessible_t accessible, bool b)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->highlight(accessible, b);
}

// Text

bb_a11y_status_t WebAccessibilityFunctionHandler::isText(bb_a11y_accessible_t accessible, bool* result)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->isText(accessible, result);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::getDefaultAttributes(bb_a11y_accessible_t accessible, char* defaultAttributes, int defaultAttributesMaxSize, char** defaultAttributesOverflow)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->getDefaultAttributes(accessible, defaultAttributes, defaultAttributesMaxSize, defaultAttributesOverflow);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::getAttributes(bb_a11y_accessible_t accessible, int offset, int* startOffset, int* endOffset, char* attributes, int attributesMaxSize, char** attributesOverflow)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->getAttributes(accessible, offset, startOffset, endOffset, attributes, attributesMaxSize, attributesOverflow);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::getText(bb_a11y_accessible_t accessible, int startOffset, int endOffset, char* text, int textMaxSize, char **textOverflow)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->getText(accessible, startOffset, endOffset, text, textMaxSize, textOverflow);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::getExtents(bb_a11y_accessible_t accessible, int startOffset, int endOffset, bb_a11y_coordinate_type_t coordinateType, int* x, int* y, int* width, int* height)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->getExtents(accessible, startOffset, endOffset, coordinateType, x, y, width, height);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::getCaretOffset(bb_a11y_accessible_t accessible, int* caretOffset)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->getCaretOffset(accessible, caretOffset);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::setCaretOffset(bb_a11y_accessible_t accessible, int offset)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->setCaretOffset(accessible, offset);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::getOffsetAtPoint(bb_a11y_accessible_t accessible, bb_a11y_coordinate_type_t coordinateType, int x, int y, int* offset)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->getOffsetAtPoint(accessible, coordinateType, x, y, offset);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::getBoundaryAtOffset(bb_a11y_accessible_t accessible, bb_a11y_text_boundary_t boundary, int offset, int* startOffset, int* endOffset)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->getBoundaryAtOffset(accessible, boundary, offset, startOffset, endOffset);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::getNumCharacters(bb_a11y_accessible_t accessible, int* numCharacters)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->getNumCharacters(accessible, numCharacters);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::scrollSubstringTo(bb_a11y_accessible_t accessible, int startOffset, int endOffset, bb_a11y_scroll_type_t scrollType)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->scrollSubstringTo(accessible, startOffset, endOffset, scrollType);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::scrollSubstringToPoint(bb_a11y_accessible_t accessible, int startOffset, int endOffset, bb_a11y_coordinate_type_t coordinateType, int x, int y)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->scrollSubstringToPoint(accessible, startOffset, endOffset, coordinateType, x, y);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::getSelection(bb_a11y_accessible_t accessible, int* startOffset, int* endOffset)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->getSelection(accessible, startOffset, endOffset);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::setSelection(bb_a11y_accessible_t accessible, int startOffset, int endOffset)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->setSelection(accessible, startOffset, endOffset);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::clearSelection(bb_a11y_accessible_t accessible)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->clearSelection(accessible);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::copy(bb_a11y_accessible_t accessible)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->copy(accessible);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::highlight(bb_a11y_accessible_t accessible, int startOffset, int endOffset, bool b)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->highlight(accessible, startOffset, endOffset, b);
}

// Editable Text

bb_a11y_status_t WebAccessibilityFunctionHandler::isEditableText(bb_a11y_accessible_t accessible, bool* result)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->isEditableText(accessible, result);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::cut(bb_a11y_accessible_t accessible)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->cut(accessible);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::paste(bb_a11y_accessible_t accessible)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->paste(accessible);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::replace(bb_a11y_accessible_t accessible, const char* text)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->replace(accessible, text);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::setAttributes(bb_a11y_accessible_t accessible, const char* attributes)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->setAttributes(accessible, attributes);
}

// Image

bb_a11y_status_t WebAccessibilityFunctionHandler::isImage(bb_a11y_accessible_t accessible, bool* result)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->isImage(accessible, result);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::getUuid(bb_a11y_accessible_t accessible, char* uuid, int uuidMaxSize, char** uuidOverflow)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->getUuid(accessible, uuid, uuidMaxSize, uuidOverflow);
}

// Table

bb_a11y_status_t WebAccessibilityFunctionHandler::isTable(bb_a11y_accessible_t accessible, bool* result)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->isTable(accessible, result);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::getExtents(bb_a11y_accessible_t accessible, int* rows, int* columns)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->getExtents(accessible, rows, columns);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::getCell(bb_a11y_accessible_t accessible, int row, int column, bb_a11y_accessible_t* cell)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->getCell(accessible, row, column, cell);
}

// Table Cell

bb_a11y_status_t WebAccessibilityFunctionHandler::isTableCell(bb_a11y_accessible_t accessible, bool* result)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->isTableCell(accessible, result);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::getTable(bb_a11y_accessible_t accessible, bb_a11y_accessible_t* table)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->getTable(accessible, table);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::getExtents(bb_a11y_accessible_t accessible, int* row, int* rowSpan, int* column, int* columnSpan)
{
    return d->m_wrapperBlackBerry->getExtents(accessible, row, rowSpan, column, columnSpan);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::getNumRowHeaders(bb_a11y_accessible_t accessible, int* numCells)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->getNumRowHeaders(accessible, numCells);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::getRowHeader(bb_a11y_accessible_t accessible, int index, bb_a11y_accessible_t* rowHeader)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->getRowHeader(accessible, index, rowHeader);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::getNumColumnHeaders(bb_a11y_accessible_t accessible, int* numCells)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->getNumColumnHeaders(accessible, numCells);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::getColumnHeader(bb_a11y_accessible_t accessible, int index, bb_a11y_accessible_t* columnHeader)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->getColumnHeader(accessible, index, columnHeader);
}

// Value

bb_a11y_status_t WebAccessibilityFunctionHandler::isValue(bb_a11y_accessible_t accessible, bool* result)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->isValue(accessible, result);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::get(bb_a11y_accessible_t accessible, bb_a11y_value_t value, double* number)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->get(accessible, value, number);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::getText(bb_a11y_accessible_t accessible, bb_a11y_value_t value, char *text, int textMaxSize, char** textOverflow)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->getText(accessible, value, text, textMaxSize, textOverflow);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::adjust(bb_a11y_accessible_t accessible, bb_a11y_value_t value)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->adjust(accessible, value);
}

bb_a11y_status_t WebAccessibilityFunctionHandler::set(bb_a11y_accessible_t accessible, double number)
{
    if (accessible == d->m_rootAccessible->accessible())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    return d->m_wrapperBlackBerry->set(accessible, number);
}

// Events

void WebAccessibilityFunctionHandlerPrivate::emitStateChangedEvent(WebCore::AccessibilityObject* obj, bb_a11y_state_t state, bool value)
{
    bb_a11y_ui_event_t event;
    bb_a11y_bridge_t bridge = accessibilityBridge();
    event.application = m_rootAccessible->accessible();
    event.accessible = getAccessible(obj);
    event.type = BB_A11Y_EVENT_STATE_CHANGED;
    event.details.state.changed_state = state;
    event.details.state.new_value = value;
    bb_a11y_bridge_notify_ui_event(bridge, &event);
}

void WebAccessibilityFunctionHandlerPrivate::emitTextChangedEvent(WebCore::AccessibilityObject* obj, bb_a11y_ui_event_type_t change, unsigned offset, const char* text)
{
    bb_a11y_ui_event_t event;
    bb_a11y_bridge_t bridge = accessibilityBridge();
    event.application = m_rootAccessible->accessible();
    event.accessible = getAccessible(obj);
    event.type = change;
    event.details.text_contents.start_offset = offset;
    event.details.text_contents.changed_text = text;
    bb_a11y_bridge_notify_ui_event(bridge, &event);
}

void WebAccessibilityFunctionHandlerPrivate::emitTextCaretMovedEvent(WebCore::AccessibilityObject* obj, int oldOffset, int newOffset)
{
    bb_a11y_ui_event_t event;
    bb_a11y_bridge_t bridge = accessibilityBridge();
    event.application = m_rootAccessible->accessible();
    event.accessible = getAccessible(obj);
    event.type = BB_A11Y_EVENT_TEXT_CARET_MOVED;
    event.details.text_caret.old_offset = oldOffset;
    event.details.text_caret.new_offset = newOffset;
    bb_a11y_bridge_notify_ui_event(bridge, &event);
}

void WebAccessibilityFunctionHandlerPrivate::emitValueChangedEvent(WebCore::AccessibilityObject* obj, int oldValue, int newValue)
{
    bb_a11y_ui_event_t event;
    bb_a11y_bridge_t bridge = accessibilityBridge();
    event.application = m_rootAccessible->accessible();
    event.accessible = getAccessible(obj);
    event.type = BB_A11Y_EVENT_VALUE_CHANGED;
    event.details.value.old_value = oldValue;
    event.details.value.new_value = newValue;
    bb_a11y_bridge_notify_ui_event(bridge, &event);
}

bool WebAccessibilityFunctionHandlerPrivate::emitInputEvent(WebCore::AccessibilityObject *obj, screen_event_t screenEvent)
{
    bb_a11y_accessible_t accessible = 0;
    if (obj)
        accessible = getAccessible(obj);

    bb_a11y_bridge_t bridge = accessibilityBridge();
    int result = 0;
    bb_a11y_bridge_notify_input_event(bridge, accessible, screenEvent, &result);
    return static_cast<bool>(result);
}

// Use emitUIEvent to emit a ui event that doesn't need any detail.
void WebAccessibilityFunctionHandlerPrivate::emitUIEvent(WebCore::AccessibilityObject* obj, bb_a11y_ui_event_type_t eventType)
{
    bb_a11y_ui_event_t event;
    bb_a11y_bridge_t bridge = accessibilityBridge();
    event.application = m_rootAccessible->accessible();
    event.accessible = getAccessible(obj);
    event.type = eventType;
    bb_a11y_bridge_notify_ui_event(bridge, &event);
}

}
}
