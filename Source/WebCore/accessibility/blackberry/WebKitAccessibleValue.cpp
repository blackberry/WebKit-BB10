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
#include "HTMLNames.h"
#include "WebKitAccessibleWrapperBlackBerry.h"

#include <wtf/text/CString.h>

using namespace WebCore;

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::isValue(const bb_a11y_accessible_t accessible, bool* result)
{
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    *result = coreObject ? (coreObject->minValueForRange() != coreObject->maxValueForRange()) : 0;
    return BB_A11Y_STATUS_OK;
}

float getStepIncrement(AccessibilityObject* coreObject)
{
    if (!coreObject->getAttribute(HTMLNames::stepAttr).isEmpty())
        return coreObject->stepValueForRange();

    // If 'step' attribute is not defined, WebCore assumes a 5% of the
    // range between minimum and maximum values, so return that.
    float range = coreObject->maxValueForRange() - coreObject->minValueForRange();
    return range * 0.05;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::get(const bb_a11y_accessible_t accessible, bb_a11y_value_t valueType, double* number)
{
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    if (!coreObject)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    bool supportsValue;
    isValue(accessible, &supportsValue);
    if (!supportsValue)
        return BB_A11Y_STATUS_NOT_SUPPORTED;

    switch (valueType) {
    case BB_A11Y_VALUE_CURRENT:
        *number = coreObject->valueForRange();
        break;
    case BB_A11Y_VALUE_MINIMUM:
        *number = coreObject->minValueForRange();
        break;
    case BB_A11Y_VALUE_MAXIMUM:
        *number = coreObject->maxValueForRange();
        break;
    case BB_A11Y_VALUE_SINGLE_STEP_ADD:
    case BB_A11Y_VALUE_SINGLE_STEP_SUBTRACT:
        *number = getStepIncrement(getAccessibilityObject(accessible));
        break;
    case BB_A11Y_VALUE_PAGE_STEP_ADD:
    case BB_A11Y_VALUE_PAGE_STEP_SUBTRACT:
        // TODO: It appears that WebCore's accessibility support doesn't have the
        // notion of "page versus step". Verify this and also find out if WebCore's
        // non-a11y support has this.
        *number = getStepIncrement(getAccessibilityObject(accessible));
        break;
    default:
        *number = -1;
    }

    return BB_A11Y_STATUS_OK;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::set(const bb_a11y_accessible_t accessible, double number)
{
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    if (!coreObject)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    if (coreObject->canSetNumericValue())
        coreObject->setValue(number);

    return BB_A11Y_STATUS_OK;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::getText(const bb_a11y_accessible_t accessible, bb_a11y_value_t valueType, char* axValue, int maxSize, char** overflow)
{
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    if (!coreObject) {
        bb_a11y_bridge_copy_string("", axValue, maxSize, overflow);
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;
    }

    bool supportsValue;
    isValue(accessible, &supportsValue);
    if (!supportsValue) {
        bb_a11y_bridge_copy_string("", axValue, maxSize, overflow);
        return BB_A11Y_STATUS_NOT_SUPPORTED;
    }

    String valueString;

    // TODO: It appears that WebCore's accessibility support only provides a way
    // to get the description for the current value; not for other steps in the
    // continuum. Verify this and also find out if WebCore's non-a11y support has
    // a means to obtain this information.
    switch (valueType) {
    case BB_A11Y_VALUE_CURRENT:
        valueString = coreObject->valueDescription();
        break;
    case BB_A11Y_VALUE_MINIMUM:
    case BB_A11Y_VALUE_MAXIMUM:
    case BB_A11Y_VALUE_SINGLE_STEP_ADD:
    case BB_A11Y_VALUE_SINGLE_STEP_SUBTRACT:
    case BB_A11Y_VALUE_PAGE_STEP_ADD:
    case BB_A11Y_VALUE_PAGE_STEP_SUBTRACT:
    default:
        break;
    }

    // The screen reader is only presenting the value text; not the value.
    if (valueString.isEmpty()) {
        double number;
        get(accessible, valueType, &number);
        valueString = String::number(number);
    }

    return static_cast<bb_a11y_status_t>(bb_a11y_bridge_copy_string(valueString.utf8().data(), axValue, maxSize, overflow));
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::adjust(const bb_a11y_accessible_t accessible, bb_a11y_value_t valueType)
{
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    if (!coreObject)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    switch (valueType) {
    case BB_A11Y_VALUE_SINGLE_STEP_ADD:
    case BB_A11Y_VALUE_PAGE_STEP_ADD:
        coreObject->increment();
        return BB_A11Y_STATUS_OK;
    case BB_A11Y_VALUE_SINGLE_STEP_SUBTRACT:
    case BB_A11Y_VALUE_PAGE_STEP_SUBTRACT:
        coreObject->decrement();
        return BB_A11Y_STATUS_OK;
    case BB_A11Y_VALUE_CURRENT: // TODO: Will this even happen?
    case BB_A11Y_VALUE_MINIMUM: // TODO: Will this even happen?
    case BB_A11Y_VALUE_MAXIMUM: // TODO: Will this even happen?
    default:
        return BB_A11Y_STATUS_OK;
    }
}

#endif // HAVE(ACCESSIBILITY)
