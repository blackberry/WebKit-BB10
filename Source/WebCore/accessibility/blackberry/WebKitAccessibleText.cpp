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
#include "Document.h"
#include "Editor.h"
#include "ExceptionCodePlaceholder.h"
#include "Frame.h"
#include "FrameView.h"
#include "IntRect.h"
#include "Node.h"
#include "NotImplemented.h"
#include "RenderListMarker.h"
#include "RenderObject.h"
#include "TextIterator.h"
#include "VisiblePosition.h"
#include "VisibleUnits.h"
#include "WebKitAccessibleWrapperBlackBerry.h"

#include <wtf/text/WTFString.h>

using namespace WebCore;

bool WebKitAccessibleWrapperBlackBerry::isTextObject(const AccessibilityObject* coreObject)
{
    if (!coreObject)
        return false;

    if (coreObject->isTextControl()
        || coreObject->isWebArea()
        || coreObject->isGroup()
        || coreObject->isListItem()
        || coreObject->isTableCell()
        || coreObject->isStaticText())
        return true;

    AccessibilityRole role = coreObject->roleValue();
    return role == ParagraphRole || role == LabelRole || role == DivRole || role == FormRole || role == ListMarkerRole;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::isText(const bb_a11y_accessible_t accessible, bool* result)
{
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    *result = isTextObject(coreObject);
    return BB_A11Y_STATUS_OK;
}

static String substring(AccessibilityObject* coreObject, int startOffset, int endOffset)
{
    String text = coreObject->stringValue();
    if (text.isEmpty())
        text = coreObject->textUnderElement();

    if (coreObject->roleValue() == ListMarkerRole) {
        RenderObject* renderObject = coreObject->renderer();
        text.append(toRenderListMarker(renderObject)->suffix());
    }

    int end = endOffset == -1 ? text.length() : endOffset;
    return text.substring(startOffset, end - startOffset);
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::getNumCharacters(const bb_a11y_accessible_t accessible, int* count)
{
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    if (!coreObject)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    String string = substring(coreObject, 0, -1);
    *count = string.length();
    return BB_A11Y_STATUS_OK;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::getText(const bb_a11y_accessible_t accessible, int startOffset, int endOffset, char* axText, int maxSize, char** overflow)
{
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    if (!coreObject)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    // AccessibilityObject::stringForVisiblePositionRange() is failure prone.
    String textString = substring(coreObject, startOffset, endOffset);
    if (textString.isEmpty())
        return BB_A11Y_STATUS_NOT_FOUND;

    return static_cast<bb_a11y_status_t>(bb_a11y_bridge_copy_string(textString.utf8().data(), axText, maxSize, overflow));
}

static IntRect textRect(AccessibilityObject *coreObject, int startOffset, int endOffset, bb_a11y_coordinate_type_t coordinateType)
{
    String text = substring(coreObject, startOffset, endOffset);
    IntRect rect = coreObject->doAXBoundsForRange(PlainTextRange(startOffset, text.length()));
    if (coordinateType == BB_A11Y_COORDINATE_TYPE_SCREEN) {
        if (Document* document = coreObject->document())
            rect = document->view()->contentsToScreen(rect);
    }

    return rect;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::getExtents(const bb_a11y_accessible_t accessible, int startOffset, int endOffset, bb_a11y_coordinate_type_t coordinateType, int* x, int* y, int* width, int* height)
{
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    if (!coreObject)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    IntRect rect = textRect(coreObject, startOffset, endOffset, coordinateType);
    *x = rect.x();
    *y = rect.y();
    *width = rect.width();
    *height = rect.height();
    return BB_A11Y_STATUS_OK;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::scrollSubstringTo(const bb_a11y_accessible_t accessible, int startOffset, int endOffset, bb_a11y_scroll_type_t scrollType)
{
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    if (!coreObject)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    IntRect rect = textRect(coreObject, startOffset, endOffset, BB_A11Y_COORDINATE_TYPE_BUFFER);
    switch (scrollType) {
    case BB_A11Y_SCROLL_TYPE_TOP_EDGE:
        rect.setHeight(1);
        break;
    case BB_A11Y_SCROLL_TYPE_LEFT_EDGE:
        rect.setWidth(1);
        break;
    case BB_A11Y_SCROLL_TYPE_TOP_LEFT:
        rect.setHeight(1);
        rect.setWidth(1);
        break;
    case BB_A11Y_SCROLL_TYPE_RIGHT_EDGE:
        rect.setX(rect.x() + rect.width());
        rect.setWidth(1);
        break;
    case BB_A11Y_SCROLL_TYPE_BOTTOM_EDGE:
        rect.setY(rect.y() + rect.height());
        rect.setHeight(1);
        break;
    case BB_A11Y_SCROLL_TYPE_BOTTOM_RIGHT:
        rect.setX(rect.x() + rect.width());
        rect.setY(rect.y() + rect.height());
        rect.setWidth(1);
        rect.setHeight(1);
        break;
    default:
        break;
    }

    coreObject->scrollToMakeVisibleWithSubFocus(rect);
    return BB_A11Y_STATUS_OK;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::scrollSubstringToPoint(const bb_a11y_accessible_t /*accessible*/, int /*startOffset*/, int /*endOffset*/, bb_a11y_coordinate_type_t /*coordinateType*/ , int /*x*/ , int /*y*/)
{
    notImplemented();
    return BB_A11Y_STATUS_NOT_SUPPORTED;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::getCaretOffset(const bb_a11y_accessible_t accessible, int* offset)
{
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    if (!coreObject)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    *offset = coreObject->selectionStart();
    return BB_A11Y_STATUS_OK;
}

bb_a11y_status_t setCaretOffsetOnCoreObject(AccessibilityObject* coreObject, int offset)
{
    PlainTextRange textRange(offset, 0);
    VisiblePositionRange range = coreObject->visiblePositionRangeForRange(textRange);
    if (range.isNull())
        return BB_A11Y_STATUS_NOT_FOUND;

    coreObject->setSelectedVisiblePositionRange(range);
    return BB_A11Y_STATUS_OK;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::setCaretOffset(const bb_a11y_accessible_t accessible, int offset)
{
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    if (!coreObject)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    return setCaretOffsetOnCoreObject(coreObject, offset);
}

void selectTextRange(AccessibilityObject* coreObject, int startOffset, int endOffset)
{
    PlainTextRange textRange(startOffset, endOffset);
    coreObject->setSelectedTextRange(textRange);
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::getSelection(const bb_a11y_accessible_t accessible, int* startOffset, int* endOffset)
{
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    if (!coreObject)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    if (coreObject->isTextControl()) {
        PlainTextRange textRange = coreObject->selectedTextRange();
        *startOffset = textRange.start;
        *endOffset = textRange.start + textRange.length - 1;
        return BB_A11Y_STATUS_OK;
    }

    // TODO: Implement this for non-text controls.
    // NOTE: WebKitAccessibleInterfaceText.cpp's getSelectionOffsetsForObject()
    // *may* do what we need done here. In which case it should be moved to
    // WebCore Accessibility.

    return BB_A11Y_STATUS_NOT_FOUND;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::setSelection(const bb_a11y_accessible_t accessible, int startOffset, int endOffset)
{
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    if (!coreObject)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    selectTextRange(coreObject, startOffset, endOffset);
    return BB_A11Y_STATUS_OK;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::clearSelection(const bb_a11y_accessible_t accessible)
{
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    if (!coreObject)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    RenderObject* renderObject = coreObject->renderer();
    if (!renderObject)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    renderObject->frame()->selection()->clear();
    return BB_A11Y_STATUS_OK;
}

Editor* WebKitAccessibleWrapperBlackBerry::editorForAccessible(AccessibilityObject* coreObject)
{
    Node* node = coreObject->node();
    if (!node)
        return 0;

    Document* document = node->document();
    if (!document)
        return 0;

    Frame* frame = document->frame();
    if (!frame)
        return 0;

    return frame->editor();
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::copy(const bb_a11y_accessible_t accessible)
{
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    if (!coreObject)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    Editor* editor = editorForAccessible(coreObject);
    if (!editor || !editor->canCopy())
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    editor->copy();
    return BB_A11Y_STATUS_OK;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::getAttributes(const bb_a11y_accessible_t /*accessible*/, int /*offset*/, int* /*startOffset*/, int* /*endOffset*/, char* /*textAttributes*/, int /*maxSize*/, char** /*overflow*/)
{
    notImplemented();
    return BB_A11Y_STATUS_NOT_SUPPORTED;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::getDefaultAttributes(const bb_a11y_accessible_t /*accessible*/, char* /*textAttributes*/, int /*maxSize*/, char** /*overflow*/)
{
    notImplemented();
    return BB_A11Y_STATUS_NOT_SUPPORTED;
}

VisiblePositionRange characterRange(const VisiblePosition& position)
{
    return VisiblePositionRange(position, position.next());
}

VisiblePositionRange wordRange(const VisiblePosition& position)
{
    VisiblePosition start = startOfWord(position);
    VisiblePosition end = endOfWord(position);

    // Unlike the other boundaries, characters go missing from the word
    // VisibleUnits. I suspect this may be a bug in WebCore, but in the
    // meantime, this little hack seems to solve nearly all the missing
    // characters.
    if (nextWordPosition(end).isNotNull())
        end = startOfWord(end.next());

    return VisiblePositionRange(start, end);
}

VisiblePositionRange lineRange(const VisiblePosition& position)
{
    VisiblePosition start = startOfLine(position);
    VisiblePosition end = endOfLine(position);
    return VisiblePositionRange(start, end);
}

VisiblePositionRange sentenceRange(const VisiblePosition& position)
{
    VisiblePosition start = startOfSentence(position);
    VisiblePosition end = endOfSentence(position);
    return VisiblePositionRange(start, end);
}

VisiblePositionRange paragraphRange(const VisiblePosition& position)
{
    VisiblePosition start = startOfParagraph(position);
    VisiblePosition end = endOfParagraph(position);
    return VisiblePositionRange(start, end);
}

int indexForVisiblePosition(AccessibilityObject* coreObject, const VisiblePosition& position)
{
    // TODO: We're duplicating code here because the WebCore Accessibility code
    // seems to restrict this to text controls; we need it for more than that.
    // It would be nice to move this code there.
    if (coreObject->isNativeTextControl() || coreObject->isTextControl())
        return coreObject->indexForVisiblePosition(position);

    if (!coreObject->isAccessibilityRenderObject())
        return 0;

    Node* node = coreObject->renderer()->node();
    if (!node)
        return 0;

    Position indexPosition = position.deepEquivalent();
    if (indexPosition.isNull())
        return 0;

    RefPtr<Range> range = Range::create(coreObject->renderer()->document());

    range->setStart(node, 0, IGNORE_EXCEPTION);
    range->setEnd(indexPosition, IGNORE_EXCEPTION);
    return TextIterator::rangeLength(range.get());
}

void boundaryForPosition(AccessibilityObject* coreObject, bb_a11y_text_boundary_t boundary, VisiblePosition position, int* startOffset, int* endOffset)
{
    VisiblePositionRange range;
    switch (boundary) {
    case BB_A11Y_TEXT_BOUNDARY_CHARACTER:
        range = characterRange(position);
        break;
    case BB_A11Y_TEXT_BOUNDARY_WORD:
        range = wordRange(position);
        break;
    case BB_A11Y_TEXT_BOUNDARY_LINE:
        range = lineRange(position);
        break;
    case BB_A11Y_TEXT_BOUNDARY_SENTENCE:
        range = sentenceRange(position);
        break;
    case BB_A11Y_TEXT_BOUNDARY_PARAGRAPH:
        range = paragraphRange(position);
        break;
    }

    *startOffset = indexForVisiblePosition(coreObject, range.start);
    *endOffset = indexForVisiblePosition(coreObject, range.end);
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::getBoundaryAtOffset(const bb_a11y_accessible_t accessible, bb_a11y_text_boundary_t boundary, int offset, int* startOffset, int* endOffset)
{
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    if (!coreObject)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    VisiblePosition position = coreObject->visiblePositionForIndex(offset);
    boundaryForPosition(coreObject, boundary, position, startOffset, endOffset);
    return BB_A11Y_STATUS_OK;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::getOffsetAtPoint(const bb_a11y_accessible_t accessible, bb_a11y_coordinate_type_t coordinateType, int x, int y, int* startOffset)
{
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    if (!coreObject)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    IntPoint point = pointForCoordinateType(coreObject, coordinateType, x, y);
    VisiblePosition position = coreObject->visiblePositionForPoint(point);
    int endOffset;
    boundaryForPosition(coreObject, BB_A11Y_TEXT_BOUNDARY_CHARACTER, position, startOffset, &endOffset);
    return BB_A11Y_STATUS_OK;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::highlight(bb_a11y_accessible_t /*accessible*/, int /*startOffset*/, int /*endOffset*/, bool /*b*/)
{
    notImplemented();

    return BB_A11Y_STATUS_NOT_SUPPORTED;
}

#endif // HAVE(ACCESSIBILITY)
