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
#include "AccessibilityTable.h"
#include "AccessibilityTableCell.h"
#include "RenderObject.h"
#include "RenderTableCell.h"
#include "WebKitAccessibleWrapperBlackBerry.h"

using namespace WebCore;

AccessibilityTableCell* WebKitAccessibleWrapperBlackBerry::coreTableCell(const bb_a11y_accessible_t accessible)
{
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    if (coreObject && coreObject->isAccessibilityRenderObject())
        return static_cast<AccessibilityTableCell*>(coreObject);

    return 0;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::isTableCell(const bb_a11y_accessible_t accessible, bool* result)
{
    AccessibilityTableCell* axTableCell = coreTableCell(accessible);

    *result = axTableCell ? axTableCell->isTableCell() : 0;
    return BB_A11Y_STATUS_OK;
}

static AccessibilityTable* parentTable(AccessibilityTableCell* axTableCell)
{
    RenderObject* renderObject = axTableCell->renderer();
    if (!renderObject || !renderObject->isTableCell())
        return 0;

    AccessibilityObject* coreObject = axTableCell->axObjectCache()->get(toRenderTableCell(renderObject)->table());
    if (coreObject)
        return static_cast<AccessibilityTable*>(coreObject);

    return 0;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::getExtents(const bb_a11y_accessible_t accessible, int* row, int* rowSpan, int* column, int* columnSpan)
{
    AccessibilityTableCell* axTableCell = coreTableCell(accessible);
    if (!axTableCell)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    pair<unsigned, unsigned> range;
    axTableCell->rowIndexRange(range);
    *row = range.first;
    *rowSpan = range.second;

    axTableCell->columnIndexRange(range);
    *column = range.first;
    *columnSpan = range.second;

    return BB_A11Y_STATUS_OK;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::getTable(const bb_a11y_accessible_t accessible, bb_a11y_accessible_t* axTable)
{
    if (!getAccessibilityObject(accessible))
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    AccessibilityTableCell* axTableCell = coreTableCell(accessible);
    if (!axTableCell)
        return BB_A11Y_STATUS_NOT_FOUND;

    AccessibilityTable* axParentTable = parentTable(axTableCell);
    if (!axParentTable)
        return BB_A11Y_STATUS_NOT_FOUND;

    *axTable = getAccessible(axParentTable);
    return BB_A11Y_STATUS_OK;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::getNumColumnHeaders(const bb_a11y_accessible_t accessible, int* count)
{
    if (!getAccessibilityObject(accessible))
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    AccessibilityTableCell* axTableCell = coreTableCell(accessible);
    if (!axTableCell) {
        *count = -1;
        return BB_A11Y_STATUS_NOT_FOUND;
    }

    AccessibilityTable* axTable = parentTable(axTableCell);
    if (!axTable) {
        *count = -1;
        return BB_A11Y_STATUS_NOT_FOUND;
    }

    AccessibilityObject::AccessibilityChildrenVector allHeaders;
    axTable->columnHeaders(allHeaders);
    *count = allHeaders.size();
    return BB_A11Y_STATUS_OK;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::getNumRowHeaders(const bb_a11y_accessible_t accessible, int* count)
{
    if (!getAccessibilityObject(accessible))
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    AccessibilityTableCell* axTableCell = coreTableCell(accessible);
    if (!axTableCell) {
        *count = -1;
        return BB_A11Y_STATUS_NOT_FOUND;
    }
    AccessibilityTable* axTable = parentTable(axTableCell);
    if (!axTable) {
        *count = -1;
        return BB_A11Y_STATUS_NOT_FOUND;
    }

    AccessibilityObject::AccessibilityChildrenVector allHeaders;
    axTable->rowHeaders(allHeaders);
    *count = allHeaders.size();
    return BB_A11Y_STATUS_OK;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::getColumnHeader(const bb_a11y_accessible_t accessible, int index, bb_a11y_accessible_t* header)
{
    if (!getAccessibilityObject(accessible))
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    *header = 0;

    AccessibilityTableCell* axTableCell = coreTableCell(accessible);
    if (!axTableCell)
        return BB_A11Y_STATUS_NOT_FOUND;
    AccessibilityTable* axTable = parentTable(axTableCell);
    if (!axTable)
        return BB_A11Y_STATUS_NOT_FOUND;

    AccessibilityObject::AccessibilityChildrenVector allHeaders;
    axTable->columnHeaders(allHeaders);
    if (static_cast<unsigned>(index) >= allHeaders.size())
        return BB_A11Y_STATUS_NOT_FOUND;

    pair<unsigned, unsigned> range;
    axTableCell->columnIndexRange(range);
    *header = getAccessible(allHeaders.at(range.first).get());
    return BB_A11Y_STATUS_OK;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::getRowHeader(const bb_a11y_accessible_t accessible, int index, bb_a11y_accessible_t* header)
{
    if (!getAccessibilityObject(accessible))
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    *header = 0;

    AccessibilityTableCell* axTableCell = coreTableCell(accessible);
    if (!axTableCell)
        return BB_A11Y_STATUS_NOT_FOUND;
    AccessibilityTable* axTable = parentTable(axTableCell);
    if (!axTable)
        return BB_A11Y_STATUS_NOT_FOUND;

    AccessibilityObject::AccessibilityChildrenVector allHeaders;
    axTable->rowHeaders(allHeaders);
    if (static_cast<unsigned>(index) >= allHeaders.size())
        return BB_A11Y_STATUS_NOT_FOUND;

    pair<unsigned, unsigned> range;
    axTableCell->rowIndexRange(range);
    *header = getAccessible(allHeaders.at(range.first).get());
    return BB_A11Y_STATUS_OK;
}

#endif // HAVE(ACCESSIBILITY)
