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

#include "AccessibilityTable.h"
#include "AccessibilityTableCell.h"
#include "WebKitAccessibleWrapperBlackBerry.h"

using namespace WebCore;

AccessibilityTable* WebKitAccessibleWrapperBlackBerry::coreTable(const bb_a11y_accessible_t accessible)
{
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    if (coreObject && coreObject->isAccessibilityRenderObject())
        return static_cast<AccessibilityTable*>(coreObject);

    return 0;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::isTable(const bb_a11y_accessible_t accessible, bool* result)
{
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    if (!coreObject)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    *result = coreObject->isAccessibilityTable();
    return BB_A11Y_STATUS_OK;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::getCell(const bb_a11y_accessible_t accessible, int row, int column, bb_a11y_accessible_t* cell)
{
    if (!getAccessibilityObject(accessible))
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    AccessibilityTable* axTable = coreTable(accessible);
    if (!axTable)
        return BB_A11Y_STATUS_NOT_FOUND;

    *cell = axTable ? getAccessible(axTable->cellForColumnAndRow(column, row)) : 0;
    return BB_A11Y_STATUS_OK;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::getExtents(const bb_a11y_accessible_t accessible, int* rowCount, int* columnCount)
{
    if (!getAccessibilityObject(accessible))
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    AccessibilityTable* axTable = coreTable(accessible);
    if (!axTable)
        return BB_A11Y_STATUS_NOT_FOUND;

    *rowCount = axTable ? axTable->rowCount() : 0;
    *columnCount = axTable ? axTable->columnCount() : 0;
    return BB_A11Y_STATUS_OK;
}

#endif // HAVE(ACCESSIBILITY)
