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

#include "WebKitAccessibleWrapperBlackBerry.h"

#include "AXObjectCache.h"
#include "AccessibilityObject.h"
#include "Chrome.h"
#include "ChromeClientBlackBerry.h"
#include "FrameView.h"
#include "HTMLNames.h"
#include "HTMLOptionElement.h"
#include "IntRect.h"
#include "NotImplemented.h"
#include "Page.h"
#include "RenderListItem.h"
#include "RenderMenuList.h"
#include "WebAccessibilityFunctionHandler_p.h"
#include "WebPage_p.h"

using namespace WebCore;

WebKitAccessibleWrapperBlackBerry::WebKitAccessibleWrapperBlackBerry(BlackBerry::WebKit::WebAccessibilityFunctionHandlerPrivate *handler)
    : m_handler(handler)
{
}

WebKitAccessibleWrapperBlackBerry::~WebKitAccessibleWrapperBlackBerry()
{
}

AccessibilityObject* WebKitAccessibleWrapperBlackBerry::getAccessibilityObject(const bb_a11y_accessible_t accessible)
{
    if (!accessible)
        return 0;

    return m_handler->getAccessibilityObject(accessible);
}

bb_a11y_accessible_t WebKitAccessibleWrapperBlackBerry::getAccessible(WebCore::AccessibilityObject* obj)
{
    if (!obj)
        return 0;

    return m_handler->getAccessible(obj);
}

bb_a11y_accessible_t WebKitAccessibleWrapperBlackBerry::focusedElement(bb_a11y_accessible_t accessible)
{
    if (!accessible)
        return 0;

    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    if (!coreObject)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    AccessibilityObject* focusedObject = getAccessibilityObject(accessible)->focusedUIElement();
    if (!focusedObject)
        return 0;

    return getAccessible(focusedObject);
}

static String bbName(AccessibilityObject* coreObject)
{
    if (!coreObject)
        return String();

    if (coreObject->isWebArea())
        return coreObject->document()->title();

    String string = coreObject->ariaLabeledByAttribute();
    if (!string.isEmpty())
        return string;

    string = coreObject->getAttribute(HTMLNames::aria_labelAttr);
    if (!string.isEmpty())
        return string;

    if (coreObject->isListItem()) {
        RenderListItem* listItem = toRenderListItem(coreObject->renderer());
        string = listItem->markerTextWithSuffix();
        string.append(coreObject->textUnderElement());
        return string;
    }

    string = coreObject->title();
    if (!string.isEmpty())
        return string;

    AccessibilityObject* label = 0;
    if (coreObject->isFieldset())
        label = coreObject->titleUIElement();
    else if (coreObject->isControl())
        label = coreObject->correspondingLabelForControlElement();
    if (label)
        return label->stringValue();

    if (coreObject->isTextControl())
        return coreObject->placeholderValue();

    if (coreObject->isMenuListOption())
        return coreObject->stringValue();

    if (coreObject->roleValue() == PopUpButtonRole) {
        // TODO: This would be nice to have in WebCore Accessibility.
        RenderMenuList* renderMenuList = toRenderMenuList(coreObject->renderer());
        if (renderMenuList)
            return renderMenuList->text();
    }

    if (coreObject->isImage() || coreObject->isInputImage() || coreObject->isNativeImage() || coreObject->isCanvas())
        return coreObject->getAttribute(HTMLNames::altAttr);

    return String();
}

static bb_a11y_role_t bbRole(AccessibilityObject* coreObject)
{
    AccessibilityRole role = coreObject ? coreObject->roleValue() : UnknownRole;
    switch (role) {
    case AnnotationRole:
        return BB_A11Y_ROLE_NOTE;
    case ApplicationRole:
        return BB_A11Y_ROLE_APPLICATION;
    case ApplicationAlertRole:
        return BB_A11Y_ROLE_ALERT;
    case ApplicationAlertDialogRole:
        return BB_A11Y_ROLE_ALERTDIALOG;
    case ApplicationDialogRole:
        return BB_A11Y_ROLE_DIALOG;
    case ApplicationLogRole:
        return BB_A11Y_ROLE_LOG;
    case ApplicationMarqueeRole:
        return BB_A11Y_ROLE_MARQUEE;
    case ApplicationStatusRole:
        return BB_A11Y_ROLE_STATUS;
    case ApplicationTimerRole:
        return BB_A11Y_ROLE_TIMER;
    case BusyIndicatorRole:
        return BB_A11Y_ROLE_ACTIVITYINDICATOR;
    case ButtonRole:
        return BB_A11Y_ROLE_BUTTON;
    case CanvasRole:
        return coreObject->canSetFocusAttribute() ? BB_A11Y_ROLE_BUTTON : BB_A11Y_ROLE_IMG;
    case CellRole:
        return BB_A11Y_ROLE_GRIDCELL;
    case CheckBoxRole:
        return BB_A11Y_ROLE_CHECKBOX;
    case ColorWellRole:
        return BB_A11Y_ROLE_BUTTON;
    case ColumnHeaderRole:
        return BB_A11Y_ROLE_COLUMNHEADER;
    case ComboBoxRole:
        return BB_A11Y_ROLE_COMBOBOX;
    case DefinitionRole:
        return BB_A11Y_ROLE_DEFINITION;
    case DirectoryRole:
        return BB_A11Y_ROLE_DIRECTORY;
    case DivRole:
    case ParagraphRole:
        return BB_A11Y_ROLE_CONTAINER;
    case DocumentRole:
        return BB_A11Y_ROLE_DOCUMENT;
    case DocumentArticleRole:
        return BB_A11Y_ROLE_ARTICLE;
    case DocumentMathRole:
        return BB_A11Y_ROLE_MATH;
    case DocumentNoteRole:
        return BB_A11Y_ROLE_NOTE;
    case DocumentRegionRole:
        return BB_A11Y_ROLE_REGION;
    case EditableTextRole:
        return BB_A11Y_ROLE_TEXTBOX;
    case FormRole:
        return BB_A11Y_ROLE_FORM;
    case GridRole:
        return BB_A11Y_ROLE_GRID;
    case GroupRole:
        return BB_A11Y_ROLE_GROUP;
    case HeadingRole:
        return BB_A11Y_ROLE_HEADING;
    case HorizontalRuleRole:
        return BB_A11Y_ROLE_SEPARATOR;
    case ImageRole:
        return BB_A11Y_ROLE_IMG;
    case LabelRole:
        return BB_A11Y_ROLE_LABEL;
    case LandmarkApplicationRole:
        return BB_A11Y_ROLE_APPLICATION;
    case LandmarkBannerRole:
        return BB_A11Y_ROLE_BANNER;
    case LandmarkComplementaryRole:
        return BB_A11Y_ROLE_COMPLEMENTARY;
    case LandmarkContentInfoRole:
        return BB_A11Y_ROLE_CONTENTINFO;
    case LandmarkMainRole:
        return BB_A11Y_ROLE_MAIN;
    case LandmarkNavigationRole:
        return BB_A11Y_ROLE_NAVIGATION;
    case LandmarkSearchRole:
        return BB_A11Y_ROLE_SEARCH;
    case LinkRole:
    case WebCoreLinkRole:
        return BB_A11Y_ROLE_LINK;
    case ListRole:
        return BB_A11Y_ROLE_LIST;
    case ListBoxOptionRole:
        return BB_A11Y_ROLE_OPTION;
    case ListBoxRole:
        return BB_A11Y_ROLE_LISTBOX;
    case ListItemRole:
        return BB_A11Y_ROLE_LISTITEM;
    case ListMarkerRole:
        return BB_A11Y_ROLE_STATICTEXT;
    case MenuRole:
    case MenuListPopupRole:
        return BB_A11Y_ROLE_MENU;
    case MenuBarRole:
        return BB_A11Y_ROLE_MENUBAR;
    case MenuItemRole:
        return BB_A11Y_ROLE_MENUITEM;
    case MenuListOptionRole:
        return BB_A11Y_ROLE_OPTION;
    case PopUpButtonRole:
        return BB_A11Y_ROLE_DROPDOWN;
    case PresentationalRole:
        return BB_A11Y_ROLE_PRESENTATION;
    case ProgressIndicatorRole:
        return BB_A11Y_ROLE_PROGRESSBAR;
    case RadioButtonRole:
        return BB_A11Y_ROLE_RADIO;
    case RadioGroupRole:
        return BB_A11Y_ROLE_RADIOGROUP;
    case RowHeaderRole:
        return BB_A11Y_ROLE_ROWHEADER;
    case RowRole:
        return BB_A11Y_ROLE_ROW;
    case ScrollAreaRole:
        return BB_A11Y_ROLE_SCROLLVIEW;
    case ScrollBarRole:
        return BB_A11Y_ROLE_SCROLLBAR;
    case SliderRole:
        return BB_A11Y_ROLE_SLIDER;
    case SpinButtonRole:
        return BB_A11Y_ROLE_SPINBUTTON;
    case StaticTextRole:
        return BB_A11Y_ROLE_STATICTEXT;
    case TabListRole:
        return BB_A11Y_ROLE_TABLIST;
    case TabPanelRole:
        return BB_A11Y_ROLE_TABPANEL;
    case TabRole:
        return BB_A11Y_ROLE_TAB;
    case TableRole:
        return BB_A11Y_ROLE_GRID;
    case TableHeaderContainerRole:
        return BB_A11Y_ROLE_CONTAINER;
    case TextAreaRole:
    case TextFieldRole:
        return BB_A11Y_ROLE_TEXTBOX;
    case ToggleButtonRole:
        return BB_A11Y_ROLE_BUTTON;
    case ToolbarRole:
        return BB_A11Y_ROLE_TOOLBAR;
    case TreeRole:
        return BB_A11Y_ROLE_TREE;
    case TreeGridRole:
        return BB_A11Y_ROLE_TREEGRID;
    case TreeItemRole:
        return BB_A11Y_ROLE_TREEITEM;
    case WebAreaRole:
        return BB_A11Y_ROLE_WEBVIEW;
    default:
        return BB_A11Y_ROLE_UNKNOWN;
    }
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::getChildAtIndex(const bb_a11y_accessible_t accessible, unsigned index, bb_a11y_accessible_t* axChild)
{
    *axChild = 0;

    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    if (!coreObject)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    AccessibilityObject::AccessibilityChildrenVector children = coreObject->children();
    if (index < children.size())
        *axChild = getAccessible(children.at(index).get());

    return BB_A11Y_STATUS_OK;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::getChildCount(const bb_a11y_accessible_t accessible, int* count)
{
    *count = 0;

    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    if (!coreObject)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    *count = coreObject->children().size();
    return BB_A11Y_STATUS_OK;
}

String elementID(AccessibilityObject* coreObject)
{
    if (!coreObject)
        return String();

    Node* node = coreObject->node();
    if (!node || !node->isElementNode())
        return String();

    return toElement(node)->getIdAttribute().string();
}

AccessibilityObject* coreDescendantWithID(AccessibilityObject* coreObject, String* id)
{
    if (id->isEmpty())
        return 0;

    if (elementID(coreObject) == *id)
        return coreObject;

    AccessibilityObject::AccessibilityChildrenVector children = coreObject->children();
    unsigned count = children.size();
    for (unsigned i = 0; i < count; i++) {
        AccessibilityObject* child = coreDescendantWithID(children.at(i).get(), id);
        if (elementID(child) == *id)
            return child;
    }

    return 0;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::findId(const bb_a11y_accessible_t parent, const char* axID, bb_a11y_accessible_t* accessible)
{
    AccessibilityObject* coreObject = getAccessibilityObject(parent);
    if (!coreObject)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    String id = String(axID);
    AccessibilityObject* descendant = coreDescendantWithID(coreObject, &id);
    *accessible = descendant ? getAccessible(descendant) : 0;
    return BB_A11Y_STATUS_OK;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::getId(const bb_a11y_accessible_t accessible, char* axID, int maxSize, char** overflow)
{
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    if (!coreObject)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    String idString = elementID(coreObject);
    bb_a11y_status_t result = static_cast<bb_a11y_status_t>(bb_a11y_bridge_copy_string(idString.utf8().data(), axID, maxSize, overflow));
    return idString.isEmpty() ? BB_A11Y_STATUS_NOT_FOUND : result;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::getIndexInParent(const bb_a11y_accessible_t accessible, int* index)
{
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    if (!coreObject) {
        *index = -1;
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;
    }

    AccessibilityObject* coreParent = coreObject->parentObjectUnignored();
    if (!coreParent) {
        *index = 0;
        return BB_A11Y_STATUS_OK;
    }

    if (coreObject->isMenuListOption()) {
        // TODO: This would be nice to have in WebCore Accessibility.
        HTMLOptionElement* optionElement = toHTMLOptionElement(coreObject->actionElement());
        if (optionElement) {
            *index =  optionElement->index();
            return BB_A11Y_STATUS_OK;
        }
    }

    AccessibilityObject::AccessibilityChildrenVector children = coreParent->children();
    size_t count = children.size();
    for (unsigned i = 0; i < count; ++i) {
        if (getAccessible(children.at(i).get()) == accessible) {
            *index = i;
            break;
        }
    }

    return BB_A11Y_STATUS_OK;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::getDescription(const bb_a11y_accessible_t accessible, char* axDescription, int maxSize, char** overflow)
{
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    // We don't return yet if coreObject is 0, because we want to fill the out parameters
    String descriptionString = coreObject? coreObject->accessibilityDescription() : String();
    if (coreObject && descriptionString.isEmpty())
        descriptionString = coreObject->getAttribute(HTMLNames::titleAttr);

    bb_a11y_status_t result = static_cast<bb_a11y_status_t>(bb_a11y_bridge_copy_string(descriptionString.utf8().data(), axDescription, maxSize, overflow));

    if (!coreObject)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    return descriptionString.isEmpty() ? BB_A11Y_STATUS_NOT_FOUND : result;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::getLanguage(const bb_a11y_accessible_t accessible, char* axLanguage, int maxSize, char** overflow)
{
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    // We don't return yet if coreObject is 0, because we want to fill the out parameters
    String languageString = coreObject ? coreObject->language() : String();
    bb_a11y_status_t result = static_cast<bb_a11y_status_t>(bb_a11y_bridge_copy_string(languageString.utf8().data(), axLanguage, maxSize, overflow));

    if (!coreObject)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    return languageString.isEmpty() ? BB_A11Y_STATUS_NOT_FOUND : result;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::getName(const bb_a11y_accessible_t accessible, char* axName, int maxSize, char** overflow)
{
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    // We don't return yet if coreObject is 0, because we want to fill the out parameters
    String nameString = bbName(coreObject);
    bb_a11y_status_t result = static_cast<bb_a11y_status_t>(bb_a11y_bridge_copy_string(nameString.utf8().data(), axName, maxSize, overflow));

    if (!coreObject)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    return nameString.isEmpty() ? BB_A11Y_STATUS_NOT_FOUND : result;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::getParent(const bb_a11y_accessible_t accessible, bb_a11y_accessible_t* axParent)
{
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    if (!coreObject) {
        *axParent = 0;
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;
    }

    AccessibilityObject* coreParent = coreObject->parentObjectUnignored();

    if (!coreParent) {
        Document* document = coreObject->document();
        if (document) {
            if (coreObject == document->axObjectCache()->rootObject()) {
                *axParent = m_handler->rootAccessible();
                return BB_A11Y_STATUS_OK;
            }

            coreParent = document->axObjectCache()->rootObject();
        }
    }

    *axParent = coreParent ? getAccessible(coreParent) : 0;

    return BB_A11Y_STATUS_OK;
}

Relation* WebKitAccessibleWrapperBlackBerry::newRelation(bb_a11y_relation_t relationType, AccessibilityObject* target)
{
    Relation* relation = new Relation;
    relation->m_type = relationType;
    relation->m_targets.append(getAccessible(target));
    return relation;
}

Relation* WebKitAccessibleWrapperBlackBerry::newRelation(bb_a11y_relation_t relationType, AccessibilityObject::AccessibilityChildrenVector& targets)
{
    Relation* relation = new Relation;
    relation->m_type = relationType;
    unsigned count = targets.size();
    for (unsigned i = 0; i < count; i++)
        relation->m_targets.append(getAccessible(targets.at(i).get()));

    return relation;
}

Vector<Relation*> WebKitAccessibleWrapperBlackBerry::relationSet(const bb_a11y_accessible_t accessible)
{
    Vector<Relation*> relations;

    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    if (!coreObject)
        return relations;

    if (coreObject->supportsARIAFlowTo()) {
        AccessibilityObject::AccessibilityChildrenVector targets;
        coreObject->ariaFlowToElements(targets);
        relations.append(newRelation(BB_A11Y_RELATION_FLOWS_TO, targets));
    }

    if (coreObject->isControl()) {
        AccessibilityObject* label = coreObject->correspondingLabelForControlElement();
        relations.append(newRelation(BB_A11Y_RELATION_LABELLED_BY, label));
        return relations;
    }

    AccessibilityObject* control = coreObject->correspondingControlForLabelElement();
    if (!control)
        return relations;

    relations.append(newRelation(BB_A11Y_RELATION_LABEL_FOR, control));
    return relations;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::getRelations(const bb_a11y_accessible_t accessible, bb_a11y_relation_t axRelation, bb_a11y_accessible_t** targets, int* targetCount)
{
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    if (!coreObject)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    Vector<Relation*> relations = relationSet(accessible);
    unsigned relationCount = relations.size();
    for (unsigned i = 0; i < relationCount; i++) {
        Relation* relation = relations[i];
        if (relation->m_type == axRelation) {
            *targetCount = relation->m_targets.size();
            *targets = new bb_a11y_accessible_t[*targetCount];
            std::copy(relation->m_targets.begin(), relation->m_targets.end(), *targets);
            return BB_A11Y_STATUS_OK;
        }
    }
    return BB_A11Y_STATUS_NOT_FOUND;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::getRole(const bb_a11y_accessible_t accessible, bb_a11y_role_t* axRole, char* /*extendedRoleType*/, int /* maxSize */, char** /*overflow*/)
{
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    // We don't return yet because bbRole also fills the proper role when we don't have coreObject
    *axRole = bbRole(coreObject);

    if (coreObject)
        return BB_A11Y_STATUS_OK;

    return BB_A11Y_STATUS_INVALID_ACCESSIBLE;
}

void addState(uint64_t* stateSet, bb_a11y_state_t state)
{
    *stateSet |= BB_A11Y_STATE_AS_BIT(state);
}

void removeState(uint64_t* stateSet, bb_a11y_state_t state)
{
    *stateSet ^= BB_A11Y_STATE_AS_BIT(state);
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::getState(const bb_a11y_accessible_t accessible, uint64_t* axStateSet)
{
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    if (!coreObject)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    AccessibilityRole role = coreObject->roleValue();

    if (coreObject->isCheckboxOrRadio() || role == ToggleButtonRole)
        addState(axStateSet, BB_A11Y_STATE_CHECKABLE);

    if (coreObject->isChecked() || coreObject->isPressed())
        addState(axStateSet, BB_A11Y_STATE_CHECKED);

    if (!coreObject->isEnabled())
        addState(axStateSet, BB_A11Y_STATE_DISABLED);

    if (coreObject->canSetExpandedAttribute() || role == PopUpButtonRole || role == ComboBoxRole)
        addState(axStateSet, BB_A11Y_STATE_EXPANDABLE);

    if (coreObject->isExpanded() || (role == PopUpButtonRole && !coreObject->isCollapsed()))
        addState(axStateSet, BB_A11Y_STATE_EXPANDED);

    // WebCore accessible sliders claim to be focusable, but on the BlackBerry
    // giving them focus does not seem possible. As a result when the user
    // interacts with a slider, the screen reader announces that it is not
    // focused. This behavior is not desirable.
    if (coreObject->canSetFocusAttribute() && !coreObject->isSlider())
        addState(axStateSet, BB_A11Y_STATE_FOCUSABLE);

    if (coreObject->isFocused())
        addState(axStateSet, BB_A11Y_STATE_FOCUSED);

    if (coreObject->ariaHasPopup())
        addState(axStateSet, BB_A11Y_STATE_HASPOPUP);

    if (!coreObject->isVisible())
        addState(axStateSet, BB_A11Y_STATE_HIDDEN);

    if (!coreObject->invalidStatus().contains("false"))
        addState(axStateSet, BB_A11Y_STATE_INVALID);

    if (coreObject->isIndeterminate())
        addState(axStateSet, BB_A11Y_STATE_MIXED);

    if (coreObject->isMultiSelectable())
        addState(axStateSet, BB_A11Y_STATE_MULTISELECTABLE);

    if (coreObject->isOffScreen())
        addState(axStateSet, BB_A11Y_STATE_OFFSCREEN);

    if (coreObject->isScrollbar() || coreObject->roleValue() == SliderRole || coreObject->roleValue() == HorizontalRuleRole) {
        if (coreObject->orientation() == AccessibilityOrientationHorizontal)
            addState(axStateSet, BB_A11Y_STATE_ORIENTATION_HORIZONTAL);
        else
            addState(axStateSet, BB_A11Y_STATE_ORIENTATION_VERTICAL);
    }

    if (coreObject->isPasswordField())
        addState(axStateSet, BB_A11Y_STATE_PROTECTED);

    if ((coreObject->isTextControl() || coreObject->isPasswordField()) && coreObject->isReadOnly())
        addState(axStateSet, BB_A11Y_STATE_READONLY);

    if (coreObject->isRequired())
        addState(axStateSet, BB_A11Y_STATE_REQUIRED);

    if (coreObject->canSetSelectedAttribute())
        addState(axStateSet, BB_A11Y_STATE_SELECTABLE);

    if (coreObject->isSelected())
        addState(axStateSet, BB_A11Y_STATE_SELECTED);

    if (coreObject->roleValue() == TextAreaRole)
        addState(axStateSet, BB_A11Y_STATE_MULTILINE);

    if (coreObject->isVisited())
        addState(axStateSet, BB_A11Y_STATE_VISITED);

    return BB_A11Y_STATUS_OK;
}

#endif // HAVE(ACCESSIBILITY)
