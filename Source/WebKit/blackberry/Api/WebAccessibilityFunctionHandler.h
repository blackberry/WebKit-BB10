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

#ifndef WebAccessibilityFunctionHandler_h
#define WebAccessibilityFunctionHandler_h

#include "BlackBerryGlobal.h"

#include <BlackBerryPlatformMisc.h>
#include <bb_a11y/rt.h>

namespace BlackBerry {
namespace WebKit {

class WebAccessibilityFunctionHandlerPrivate;
class WebPagePrivate;

class BLACKBERRY_EXPORT WebAccessibilityFunctionHandler {
public:
    WebAccessibilityFunctionHandler(WebPagePrivate*);
    ~WebAccessibilityFunctionHandler();

    // Accessible

    bb_a11y_status_t getLanguage(bb_a11y_accessible_t /*accessible*/, char* language, int languageMaxSize, char** languageOverflow);
    bb_a11y_status_t getId(bb_a11y_accessible_t /*accessible*/, char* id, int idMaxSize, char** idOverflow);
    bb_a11y_status_t findId(bb_a11y_accessible_t parent, const char* id, bb_a11y_accessible_t* /*accessible*/);
    bb_a11y_status_t getName(bb_a11y_accessible_t /*accessible*/, char* name, int nameMaxSize, char** nameOverflow);
    bb_a11y_status_t getDescription(bb_a11y_accessible_t /*accessible*/, char* description, int descriptionMaxSize, char** descriptionOverflow);
    bb_a11y_status_t getParent(bb_a11y_accessible_t child, bb_a11y_accessible_t* parent);
    bb_a11y_status_t getIndexInParent(bb_a11y_accessible_t child, int* index);
    bb_a11y_status_t getChildCount(bb_a11y_accessible_t parent, int* childCount);
    bb_a11y_status_t getChildAtIndex(bb_a11y_accessible_t parent, unsigned index, bb_a11y_accessible_t* child);
    bb_a11y_status_t getRole(bb_a11y_accessible_t /*accessible*/, bb_a11y_role_t* /*role*/, char* extendedRole, int extendedRoleMaxSize, char** extendedRoleOverflow);
    bb_a11y_status_t getState(bb_a11y_accessible_t /*accessible*/, _Uint64t* state);
    bb_a11y_status_t getRelations(bb_a11y_accessible_t /*accessible*/, bb_a11y_relation_t /*relation*/, bb_a11y_accessible_t** relations, int* numRelations);

    // Component

    bb_a11y_status_t isComponent(bb_a11y_accessible_t /*accessible*/, bool* result);
    bb_a11y_status_t getAccessibleAtPoint(bb_a11y_accessible_t parent, int x, int y, bb_a11y_coordinate_type_t /*coordinateType*/, bb_a11y_accessible_t* /*accessible*/);
    bb_a11y_status_t containsPoint(bb_a11y_accessible_t /*accessible*/, int x, int y, bb_a11y_coordinate_type_t /*coordinateType*/, bool* result);
    bb_a11y_status_t getAttributes(bb_a11y_accessible_t /*accessible*/, char* attributes, int attributesMaxSize, char** attributesOverflow);
    bb_a11y_status_t getExtents(bb_a11y_accessible_t /*accessible*/, bb_a11y_coordinate_type_t /*coordinateType*/, int* x, int* y, int* width, int* height);
    bb_a11y_status_t getZOrder(bb_a11y_accessible_t /*accessible*/, int* zOrder);
    bb_a11y_status_t grabFocus(bb_a11y_accessible_t /*accessible*/);
    bb_a11y_status_t activate(bb_a11y_accessible_t /*accessible*/, bb_a11y_activate_type_t /*activateType*/);
    bb_a11y_status_t highlight(bb_a11y_accessible_t /*accessible*/, bool /*b*/);

    // Text

    bb_a11y_status_t isText(bb_a11y_accessible_t /*accessible*/, bool* result);
    bb_a11y_status_t getDefaultAttributes(bb_a11y_accessible_t /*accessible*/, char* defaultAttributes, int defaultAttributesMaxSize, char** defaultAttributesOverflow);
    bb_a11y_status_t getAttributes(bb_a11y_accessible_t /*accessible*/, int offset, int* startOffset, int* endOffset, char* attributes, int attributesMaxSize, char** attributesOverflow);
    bb_a11y_status_t getText(bb_a11y_accessible_t /*accessible*/, int startOffset, int endOffset, char* text, int textMaxSize, char** textOverflow);
    bb_a11y_status_t getExtents(bb_a11y_accessible_t /*accessible*/, int startOffset, int endOffset, bb_a11y_coordinate_type_t /*coordinateType*/, int* x, int* y, int* width, int* height);
    bb_a11y_status_t getCaretOffset(bb_a11y_accessible_t /*accessible*/, int* caretOffset);
    bb_a11y_status_t setCaretOffset(bb_a11y_accessible_t /*accessible*/, int offset);
    bb_a11y_status_t getOffsetAtPoint(bb_a11y_accessible_t /*accessible*/, bb_a11y_coordinate_type_t /*coordinateType*/, int x, int y, int* offset);
    bb_a11y_status_t getBoundaryAtOffset(bb_a11y_accessible_t /*accessible*/, bb_a11y_text_boundary_t /*boundary*/, int offset, int* startOffset, int* endOffset);
    bb_a11y_status_t getNumCharacters(bb_a11y_accessible_t /*accessible*/, int* numCharacters);
    bb_a11y_status_t scrollSubstringTo(bb_a11y_accessible_t /*accessible*/, int startOffset, int endOffset, bb_a11y_scroll_type_t /*scrollType*/);
    bb_a11y_status_t scrollSubstringToPoint(bb_a11y_accessible_t /*accessible*/, int startOffset, int endOffset, bb_a11y_coordinate_type_t /*coordinateType*/, int x, int y);
    bb_a11y_status_t getSelection(bb_a11y_accessible_t /*accessible*/, int* startOffset, int* endOffset);
    bb_a11y_status_t setSelection(bb_a11y_accessible_t /*accessible*/, int startOffset, int endOffset);
    bb_a11y_status_t clearSelection(bb_a11y_accessible_t /*accessible*/);
    bb_a11y_status_t copy(bb_a11y_accessible_t /*accessible*/);
    bb_a11y_status_t highlight(bb_a11y_accessible_t /*accessible*/, int startOffset, int endOffset, bool /*b*/);

    // Editable Text

    bb_a11y_status_t isEditableText(bb_a11y_accessible_t /*accessible*/, bool* result);
    bb_a11y_status_t cut(bb_a11y_accessible_t /*accessible*/);
    bb_a11y_status_t paste(bb_a11y_accessible_t /*accessible*/);
    bb_a11y_status_t replace(bb_a11y_accessible_t /*accessible*/, const char* text);
    bb_a11y_status_t setAttributes(bb_a11y_accessible_t /*accessible*/, const char* attributes);

    // Image

    bb_a11y_status_t isImage(bb_a11y_accessible_t /*accessible*/, bool* result);
    bb_a11y_status_t getUuid(bb_a11y_accessible_t /*accessible*/, char* uuid, int uuidMaxSize, char** uuidOverflow);

    // Table

    bb_a11y_status_t isTable(bb_a11y_accessible_t /*accessible*/, bool* result);
    bb_a11y_status_t getExtents(bb_a11y_accessible_t /*accessible*/, int* rows, int* columns);
    bb_a11y_status_t getCell(bb_a11y_accessible_t /*accessible*/, int row, int column, bb_a11y_accessible_t* cell);

    // Table Cell

    bb_a11y_status_t isTableCell(bb_a11y_accessible_t /*accessible*/, bool* result);
    bb_a11y_status_t getTable(bb_a11y_accessible_t /*accessible*/, bb_a11y_accessible_t* table);
    bb_a11y_status_t getExtents(bb_a11y_accessible_t /*accessible*/, int* row, int* rowSpan, int* column, int* columnSpan);
    bb_a11y_status_t getNumRowHeaders(bb_a11y_accessible_t /*accessible*/, int* numCells);
    bb_a11y_status_t getRowHeader(bb_a11y_accessible_t /*accessible*/, int index, bb_a11y_accessible_t* rowHeader);
    bb_a11y_status_t getNumColumnHeaders(bb_a11y_accessible_t /*accessible*/, int* numCells);
    bb_a11y_status_t getColumnHeader(bb_a11y_accessible_t /*accessible*/, int index, bb_a11y_accessible_t* columnHeader);

    // Value

    bb_a11y_status_t isValue(bb_a11y_accessible_t /*accessible*/, bool* result);
    bb_a11y_status_t get(bb_a11y_accessible_t /*accessible*/, bb_a11y_value_t /*value*/, double* number);
    bb_a11y_status_t getText(bb_a11y_accessible_t /*accessible*/, bb_a11y_value_t /*value*/, char* text, int textMaxSize, char** textOverflow);
    bb_a11y_status_t adjust(bb_a11y_accessible_t /*accessible*/, bb_a11y_value_t /*value*/);
    bb_a11y_status_t set(bb_a11y_accessible_t /*accessible*/, double number);

private:
    friend class WebPagePrivate;
    friend class WebPageAccessible;
    WebAccessibilityFunctionHandlerPrivate* d;
    DISABLE_COPY(WebAccessibilityFunctionHandler)
};

} // namespace WebKit
} // namespace BlackBerry

#endif /* WebAccessibilityFunctionHandler_h */
