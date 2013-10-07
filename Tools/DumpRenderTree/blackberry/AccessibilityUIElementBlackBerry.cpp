/*
 * Copyright (C) 2010, 2012, 2013 Research In Motion Limited. All rights reserved.
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

#include "config.h"
#include "AccessibilityUIElement.h"

#if HAVE(ACCESSIBILITY)

#include "NotImplemented.h"
#include <JavaScriptCore/JSStringRef.h>
#include <bb_a11y/at.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

AccessibilityUIElement::AccessibilityUIElement(PlatformUIElement element)
    : m_element(element)
{
}

AccessibilityUIElement::AccessibilityUIElement(const AccessibilityUIElement& other)
    : m_element(other.m_element)
{
}

AccessibilityUIElement::~AccessibilityUIElement()
{
}

static bool hasState(PlatformUIElement element, uint64_t state)
{
    uint64_t stateSet;
    bb_a11y_accessible_get_state(element, &stateSet);
    return stateSet & state;
}

static String roleToString(bb_a11y_role_t role)
{
    switch (role) {
    case BB_A11Y_ROLE_ACTIVITYINDICATOR:
        return "AXRole: AXBusyIndicator";
    case BB_A11Y_ROLE_ALERT:
        return "AXRole: AXApplicationAlert";
    case BB_A11Y_ROLE_ALERTDIALOG:
        return "AXRole: AXApplicationAlertDialog";
    case BB_A11Y_ROLE_APPLICATION:
        return "AXRole: AXApplication";
    case BB_A11Y_ROLE_ARTICLE:
        return "AXRole: AXDocumentArticle";
    case BB_A11Y_ROLE_BANNER:
        return "AXRole: AXLandmarkBanner";
    case BB_A11Y_ROLE_BUTTON:
        return "AXRole: AXButton";
    case BB_A11Y_ROLE_CHECKBOX:
        return "AXRole: AXCheckBox";
    case BB_A11Y_ROLE_COLUMNHEADER:
        return "AXRole: AXColumnHeader";
    case BB_A11Y_ROLE_COMBOBOX:
        return "AXRole: AXComboBox";
    case BB_A11Y_ROLE_COMPLEMENTARY:
        return "AXRole: AXLandmarkComplementary";
    case BB_A11Y_ROLE_CONTAINER:
        return "AXRole: AXGroup";
    case BB_A11Y_ROLE_CONTENTINFO:
        return "AXRole: AXLandmarkContentInfo";
    case BB_A11Y_ROLE_DEFINITION:
        return "AXRole: AXDefinition";
    case BB_A11Y_ROLE_DIRECTORY:
        return "AXRole: AXDirectory";
    case BB_A11Y_ROLE_DOCUMENT:
        return "AXRole: AXDocument";
    case BB_A11Y_ROLE_FORM:
        return "AXRole: AXForm";
    case BB_A11Y_ROLE_GRID:
        return "AXRole: AXGrid";
    case BB_A11Y_ROLE_GRIDCELL:
        return "AXRole: AXCell";
    case BB_A11Y_ROLE_GROUP:
        return "AXRole: AXGroup";
    case BB_A11Y_ROLE_HEADING:
        return "AXRole: AXHeading";
    case BB_A11Y_ROLE_IMG:
        return "AXRole: AXImage";
    case BB_A11Y_ROLE_LABEL:
        return "AXRole: AXLabel";
    case BB_A11Y_ROLE_LINK:
        return "AXRole: AXLink";
    case BB_A11Y_ROLE_LIST:
        return "AXRole: AXList";
    case BB_A11Y_ROLE_LISTBOX:
        return "AXRole: AXListBox";
    case BB_A11Y_ROLE_LISTITEM:
        return "AXRole: AXListItem";
    case BB_A11Y_ROLE_LOG:
        return "AXRole: AXApplicationLog";
    case BB_A11Y_ROLE_MAIN:
        return "AXRole: AXLandmarkMain";
    case BB_A11Y_ROLE_MARQUEE:
        return "AXRole: AXApplicationMarquee";
    case BB_A11Y_ROLE_MATH:
        return "AXRole: AXDocumentMath";
    case BB_A11Y_ROLE_NAVIGATION:
        return "AXRole: AXLandmarkNavigation";
    case BB_A11Y_ROLE_MENU:
        return "AXRole: AXMenu";
    case BB_A11Y_ROLE_MENUBAR:
        return "AXRole: AXMenuBar";
    case BB_A11Y_ROLE_MENUITEM:
        return "AXRole: AXMenuItem";
    case BB_A11Y_ROLE_NOTE:
        return "AXRole: AXAnnotation";
    case BB_A11Y_ROLE_OPTION:
        return "AXRole: AXListBoxOption";
    case BB_A11Y_ROLE_PRESENTATION:
        return "AXRole: AXPresentational";
    case BB_A11Y_ROLE_PROGRESSBAR:
        return "AXRole: AXProgressIndicator";
    case BB_A11Y_ROLE_RADIO:
        return "AXRole: AXRadioButton";
    case BB_A11Y_ROLE_RADIOGROUP:
        return "AXRole: AXRadioGroup";
    case BB_A11Y_ROLE_REGION:
        return "AXRole: AXDocumentRegion";
    case BB_A11Y_ROLE_ROW:
        return "AXRole: AXRow";
    case BB_A11Y_ROLE_ROWHEADER:
        return "AXRole: AXRowHeader";
    case BB_A11Y_ROLE_SCROLLBAR:
        return "AXRole: AXScrollBar";
    case BB_A11Y_ROLE_SCROLLVIEW:
        return "AXRole: AXScrollArea";
    case BB_A11Y_ROLE_SEARCH:
        return "AXRole: AXLandmarkSearch";
    case BB_A11Y_ROLE_SEPARATOR:
        return "AXRole: AXHorizontalRule";
    case BB_A11Y_ROLE_SLIDER:
        return "AXRole: AXSlider";
    case BB_A11Y_ROLE_SPINBUTTON:
        return "AXRole: AXSpinButton";
    case BB_A11Y_ROLE_STATUS:
        return "AXRole: AXApplicationStatus";
    case BB_A11Y_ROLE_TAB:
        return "AXRole: AXTab";
    case BB_A11Y_ROLE_TABLIST:
        return "AXRole: AXTabList";
    case BB_A11Y_ROLE_TABPANEL:
        return "AXRole: AXTabPanel";
    case BB_A11Y_ROLE_TEXTBOX:
        return "AXRole: AXTextField";
    case BB_A11Y_ROLE_TIMER:
        return "AXRole: AXApplicationTimer";
    case BB_A11Y_ROLE_TOOLBAR:
        return "AXRole: AXToolbar";
    case BB_A11Y_ROLE_TREE:
        return "AXRole: AXTree";
    case BB_A11Y_ROLE_TREEGRID:
        return "AXRole: AXTreeGrid";
    case BB_A11Y_ROLE_TREEITEM:
        return "AXRole: AXTreeItem";
    case BB_A11Y_ROLE_WEBVIEW:
        return "AXRole: AXWebAreaRole";
    default:
        return "AXRole: AXGroupRole";
    }
}

void AccessibilityUIElement::getLinkedUIElements(Vector<AccessibilityUIElement>&)
{
    notImplemented();
}

void AccessibilityUIElement::getDocumentLinks(Vector<AccessibilityUIElement>&)
{
    notImplemented();
}

void AccessibilityUIElement::getChildren(Vector<AccessibilityUIElement>& children)
{
    getChildrenWithRange(children, 0, childrenCount());
}

void AccessibilityUIElement::getChildrenWithRange(Vector<AccessibilityUIElement>& children, unsigned start, unsigned end)
{
    for (unsigned i = start; i < end; i++)
        children.append(getChildAtIndex(i));
}

int AccessibilityUIElement::childrenCount()
{
    int count;
    bb_a11y_accessible_get_child_count(m_element, &count);
    return count;
}

AccessibilityUIElement AccessibilityUIElement::elementAtPoint(int x, int y)
{
    bb_a11y_accessible_t element = 0;
    bb_a11y_component_get_accessible_at_point(m_element, x, y, BB_A11Y_COORDINATE_TYPE_BUFFER, element);
    return element;
}

AccessibilityUIElement AccessibilityUIElement::getChildAtIndex(unsigned index)
{
    bb_a11y_accessible_t child = 0;
    bb_a11y_accessible_get_child_at_index(m_element, index, child);
    return child;
}

AccessibilityUIElement AccessibilityUIElement::linkedUIElementAtIndex(unsigned)
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::allAttributes()
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::attributesOfLinkedUIElements()
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::attributesOfDocumentLinks()
{
    notImplemented();
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::titleUIElement()
{
    notImplemented();
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::parentElement()
{
    bb_a11y_accessible_t parent = 0;
    bb_a11y_accessible_get_parent(m_element, parent);
    return parent;
}

JSStringRef AccessibilityUIElement::attributesOfChildren()
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::parameterizedAttributeNames()
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::role()
{
    bb_a11y_role_t axRole;
    if (bb_a11y_accessible_get_role(m_element, &axRole, 0, 0, 0) != BB_A11Y_STATUS_OK)
        return JSStringCreateWithCharacters(0, 0);
    String roleString = roleToString(axRole);
    return JSStringCreateWithUTF8CString(roleString.utf8().data());
}

JSStringRef AccessibilityUIElement::subrole()
{
    // The BB A11y Framework doesn't have this concept.
    return 0;
}

JSStringRef AccessibilityUIElement::roleDescription()
{
    // The BB A11y Framework doesn't have this concept.
    return 0;
}

JSStringRef AccessibilityUIElement::title()
{
    char buffer[64];
    char* string = buffer;
    if (bb_a11y_accessible_get_name(m_element, buffer, 64, &string) != BB_A11Y_STATUS_OK)
        return JSStringCreateWithCharacters(0, 0);

    String result = String::format("AXTitle: %s", string);
    if (buffer != string)
        bb_a11y_at_free(reinterpret_cast<void**>(&string));

    return JSStringCreateWithUTF8CString(result.utf8().data());
}

JSStringRef AccessibilityUIElement::description()
{
    char buffer[64];
    char* string = buffer;
    if (bb_a11y_accessible_get_description(m_element, buffer, 64, &string) != BB_A11Y_STATUS_OK)
        return JSStringCreateWithCharacters(0, 0);

    String result = String::format("AXDescription: %s", string);
    if (buffer != string)
        bb_a11y_at_free(reinterpret_cast<void**>(&string));

    return JSStringCreateWithUTF8CString(result.utf8().data());
}

JSStringRef AccessibilityUIElement::stringValue()
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::language()
{
    char buffer[64];
    char* string = buffer;
    if (bb_a11y_accessible_get_language(m_element, buffer, 64, &string) != BB_A11Y_STATUS_OK)
        return JSStringCreateWithCharacters(0, 0);

    String result = String::format("AXLanguage: %s", string);
    if (buffer != string)
        bb_a11y_at_free(reinterpret_cast<void**>(&string));

    return JSStringCreateWithUTF8CString(result.utf8().data());
}

double AccessibilityUIElement::x()
{
    int x, y, width, height;
    bb_a11y_component_get_extents(m_element, BB_A11Y_COORDINATE_TYPE_BUFFER, &x, &y, &width, &height);
    return x;
}

double AccessibilityUIElement::y()
{
    int x, y, width, height;
    bb_a11y_component_get_extents(m_element, BB_A11Y_COORDINATE_TYPE_BUFFER, &x, &y, &width, &height);
    return y;
}

double AccessibilityUIElement::width()
{
    int x, y, width, height;
    bb_a11y_component_get_extents(m_element, BB_A11Y_COORDINATE_TYPE_BUFFER, &x, &y, &width, &height);
    return width;
}

double AccessibilityUIElement::height()
{
    int x, y, width, height;
    bb_a11y_component_get_extents(m_element, BB_A11Y_COORDINATE_TYPE_BUFFER, &x, &y, &width, &height);
    return height;
}

double AccessibilityUIElement::clickPointX()
{
    int x, y, width, height;
    bb_a11y_component_get_extents(m_element, BB_A11Y_COORDINATE_TYPE_BUFFER, &x, &y, &width, &height);
    return x + width / 2;
}

double AccessibilityUIElement::clickPointY()
{
    int x, y, width, height;
    bb_a11y_component_get_extents(m_element, BB_A11Y_COORDINATE_TYPE_BUFFER, &x, &y, &width, &height);
    return y + height / 2;
}

JSStringRef AccessibilityUIElement::orientation() const
{
    if (hasState(m_element, BB_A11Y_STATE_ORIENTATION_HORIZONTAL))
        return JSStringCreateWithUTF8CString("AXOrientation: AXHorizontalOrientation");
    if (hasState(m_element, BB_A11Y_STATE_ORIENTATION_VERTICAL))
        return JSStringCreateWithUTF8CString("AXOrientation: AXVerticalOrientation");

    return JSStringCreateWithCharacters(0, 0);
}

double AccessibilityUIElement::minValue()
{
    double minimum;
    bb_a11y_value_get(m_element, BB_A11Y_VALUE_MINIMUM, &minimum);
    return minimum;
}

double AccessibilityUIElement::maxValue()
{
    double maximum;
    bb_a11y_value_get(m_element, BB_A11Y_VALUE_MINIMUM, &maximum);
    return maximum;
}

JSStringRef AccessibilityUIElement::valueDescription()
{
    char buffer[64];
    char* string = buffer;
    if (bb_a11y_value_get_text(m_element, BB_A11Y_VALUE_CURRENT, buffer, 64, &string) != BB_A11Y_STATUS_OK)
        return JSStringCreateWithCharacters(0, 0);

    String result = buffer;
    if (buffer != string)
        bb_a11y_at_free(reinterpret_cast<void**>(&string));

    return JSStringCreateWithUTF8CString(result.utf8().data());
}

bool AccessibilityUIElement::isEnabled()
{
    return !hasState(m_element, BB_A11Y_STATE_DISABLED);
}

int AccessibilityUIElement::insertionPointLineNumber()
{
    notImplemented();
    return 0;
}

bool AccessibilityUIElement::isPressActionSupported()
{
    notImplemented();
    return 0;
}

bool AccessibilityUIElement::isIncrementActionSupported()
{
    notImplemented();
    return 0;
}

bool AccessibilityUIElement::isDecrementActionSupported()
{
    notImplemented();
    return 0;
}

bool AccessibilityUIElement::isRequired() const
{
    return hasState(m_element, BB_A11Y_STATE_REQUIRED);
}

bool AccessibilityUIElement::isSelected() const
{
    return hasState(m_element, BB_A11Y_STATE_SELECTED);
}

int AccessibilityUIElement::hierarchicalLevel() const
{
    notImplemented();
    return 0;
}

bool AccessibilityUIElement::ariaIsGrabbed() const
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::ariaDropEffects() const
{
    notImplemented();
    return 0;
}

bool AccessibilityUIElement::isExpanded() const
{
    return hasState(m_element, BB_A11Y_STATE_EXPANDED);
}

JSStringRef AccessibilityUIElement::attributesOfColumnHeaders()
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::attributesOfRowHeaders()
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::attributesOfColumns()
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::attributesOfRows()
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::attributesOfVisibleCells()
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::attributesOfHeader()
{
    notImplemented();
    return 0;
}

int AccessibilityUIElement::indexInTable()
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::rowIndexRange()
{
    int row, rowSpan, column, columnSpan;
    bb_a11y_table_cell_get_extents(m_element, &row, &rowSpan, &column, &columnSpan);
    String string = String::format("{%d, %d}", row, row + rowSpan);
    return JSStringCreateWithUTF8CString(string.utf8().data());
}

JSStringRef AccessibilityUIElement::columnIndexRange()
{
    int row, rowSpan, column, columnSpan;
    bb_a11y_table_cell_get_extents(m_element, &row, &rowSpan, &column, &columnSpan);
    String string = String::format("{%d, %d}", column, column + columnSpan);
    return JSStringCreateWithUTF8CString(string.utf8().data());
}

int AccessibilityUIElement::lineForIndex(int)
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::boundsForRange(unsigned start, unsigned end)
{
    int x, y, width, height;
    bb_a11y_text_get_extents(m_element, start, end, BB_A11Y_COORDINATE_TYPE_BUFFER, &x, &y, &width, &height);
    String string = String::format("{{%f, %f}, {%f, %f}}", static_cast<double>(x), static_cast<double>(y), static_cast<double>(width), static_cast<double>(height));
    return JSStringCreateWithUTF8CString(string.utf8().data());
}

JSStringRef AccessibilityUIElement::stringForRange(unsigned start, unsigned end)
{
    char buffer[64];
    char* string = buffer;
    if (bb_a11y_text_get_text(m_element, start, end, buffer, 64, &string) != BB_A11Y_STATUS_OK)
        return JSStringCreateWithCharacters(0, 0);

    String result = string;
    if (buffer != string)
        bb_a11y_at_free(reinterpret_cast<void**>(&string));

    return JSStringCreateWithUTF8CString(result.utf8().data());
}

AccessibilityUIElement AccessibilityUIElement::uiElementForSearchPredicate(AccessibilityUIElement*, bool, JSStringRef, JSStringRef)
{
    notImplemented();
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::cellForColumnAndRow(unsigned column, unsigned row)
{
    bool isTable;
    if (!bb_a11y_accessible_is_table(m_element, &isTable))
        return 0;

    bb_a11y_accessible_t cell = 0;
    bb_a11y_table_get_cell(m_element, row, column, cell);
    return cell;
}

JSStringRef AccessibilityUIElement::selectedTextRange()
{
    int start, end;
    bb_a11y_text_get_selection(m_element, &start, &end);
    String string = String::format("{%d, %d}", start, end-start);
    return JSStringCreateWithUTF8CString(string.utf8().data());
}

void AccessibilityUIElement::setSelectedTextRange(unsigned start, unsigned length)
{
    bb_a11y_text_set_selection(m_element, start, start+length);
}

bool AccessibilityUIElement::isAttributeSettable(JSStringRef)
{
    notImplemented();
    return 0;
}

bool AccessibilityUIElement::isAttributeSupported(JSStringRef)
{
    notImplemented();
    return 0;
}

void AccessibilityUIElement::increment()
{
    bb_a11y_value_adjust(m_element, BB_A11Y_VALUE_SINGLE_STEP_ADD);
}

void AccessibilityUIElement::decrement()
{
    bb_a11y_value_adjust(m_element, BB_A11Y_VALUE_SINGLE_STEP_SUBTRACT);
}

void AccessibilityUIElement::showMenu()
{
    notImplemented();
}

AccessibilityUIElement AccessibilityUIElement::disclosedRowAtIndex(unsigned)
{
    notImplemented();
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::ariaOwnsElementAtIndex(unsigned)
{
    notImplemented();
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::ariaFlowToElementAtIndex(unsigned)
{
    notImplemented();
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::selectedRowAtIndex(unsigned)
{
    notImplemented();
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::rowAtIndex(unsigned)
{
    notImplemented();
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::disclosedByRow()
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::accessibilityValue() const
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::documentEncoding()
{
    // The BB A11y Framework doesn't have API for runtimes to expose this.
    return 0;
}

JSStringRef AccessibilityUIElement::documentURI()
{
    // The BB A11y Framework doesn't have API for runtimes to expose this.
    return 0;
}

unsigned AccessibilityUIElement::indexOfChild(AccessibilityUIElement*)
{
    int index = 0;
    bb_a11y_accessible_get_index_in_parent(m_element, &index);
    return index;
}

double AccessibilityUIElement::numberAttributeValue(JSStringRef)
{
    notImplemented();
    return 0;
}

bool AccessibilityUIElement::boolAttributeValue(JSStringRef)
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::stringAttributeValue(JSStringRef)
{
    notImplemented();
    return 0;
}

double AccessibilityUIElement::intValue() const
{
    double value;
    bb_a11y_value_get(m_element, BB_A11Y_VALUE_CURRENT, &value);
    return value;
}

bool AccessibilityUIElement::isChecked() const
{
    return hasState(m_element, BB_A11Y_STATE_CHECKED);
}

JSStringRef AccessibilityUIElement::url()
{
    // The BB A11y Framework doesn't have API for runtimes to expose this.
    return 0;
}

bool AccessibilityUIElement::addNotificationListener(JSObjectRef)
{
    notImplemented();
    return 0;
}

bool AccessibilityUIElement::isSelectable() const
{
    return hasState(m_element, BB_A11Y_STATE_SELECTABLE);
}

bool AccessibilityUIElement::isMultiSelectable() const
{
    return hasState(m_element, BB_A11Y_STATE_MULTISELECTABLE);
}

bool AccessibilityUIElement::isSelectedOptionActive() const
{
    notImplemented();
    return false;
}

bool AccessibilityUIElement::isVisible() const
{
    return !hasState(m_element, BB_A11Y_STATE_HIDDEN);
}

bool AccessibilityUIElement::isOffScreen() const
{
    return hasState(m_element, BB_A11Y_STATE_OFFSCREEN);
}

bool AccessibilityUIElement::isCollapsed() const
{
    if (!hasState(m_element, BB_A11Y_STATE_EXPANDABLE))
        return false;

    return !hasState(m_element, BB_A11Y_STATE_EXPANDED);
}

bool AccessibilityUIElement::hasPopup() const
{
    return hasState(m_element, BB_A11Y_STATE_HASPOPUP);
}

void AccessibilityUIElement::scrollToMakeVisible()
{
    notImplemented();
}

void AccessibilityUIElement::scrollToMakeVisibleWithSubFocus(int, int, int, int)
{
    notImplemented();
}

void AccessibilityUIElement::scrollToGlobalPoint(int /*x*/, int /*y*/)
{
    notImplemented();
}

void AccessibilityUIElement::takeFocus()
{
    bb_a11y_component_grab_focus(m_element);
}

void AccessibilityUIElement::takeSelection()
{
    notImplemented();
}

void AccessibilityUIElement::addSelection()
{
    notImplemented();
}

void AccessibilityUIElement::removeSelection()
{
    notImplemented();
}

int AccessibilityUIElement::columnCount()
{
    int rowCount, columnCount;
    bb_a11y_table_get_extents(m_element, &rowCount, &columnCount);
    return columnCount;
}

void AccessibilityUIElement::removeNotificationListener()
{
    notImplemented();
}

void AccessibilityUIElement::press()
{
    notImplemented();
}

int AccessibilityUIElement::rowCount()
{
    int rowCount, columnCount;
    bb_a11y_table_get_extents(m_element, &rowCount, &columnCount);
    return rowCount;
}

JSStringRef AccessibilityUIElement::helpText() const
{
    notImplemented();
    return 0;
}

JSStringRef AccessibilityUIElement::attributedStringForRange(unsigned, unsigned)
{
    notImplemented();
    return 0;
}

bool AccessibilityUIElement::attributedStringRangeIsMisspelled(unsigned, unsigned)
{
    notImplemented();
    return 0;
}

bool AccessibilityUIElement::isIgnored() const
{
    notImplemented();
    return false;
}

bool AccessibilityUIElement::isFocused() const
{
    return hasState(m_element, BB_A11Y_STATE_FOCUSED);
}

bool AccessibilityUIElement::isFocusable() const
{
    return hasState(m_element, BB_A11Y_STATE_FOCUSABLE);
}

#endif // HAVE(ACCESSIBILITY)
