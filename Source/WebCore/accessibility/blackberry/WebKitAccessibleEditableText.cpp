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
#include "Editor.h"
#include "NotImplemented.h"
#include "WebKitAccessibleWrapperBlackBerry.h"

#include <wtf/text/WTFString.h>

using namespace WebCore;

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::isEditableText(const bb_a11y_accessible_t accessible, bool* result)
{
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    *result = isTextObject(coreObject) && !coreObject->isReadOnly();
    return BB_A11Y_STATUS_OK;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::cut(const bb_a11y_accessible_t accessible)
{
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    if (!coreObject)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    Editor* editor = editorForAccessible(coreObject);
    if (!editor || !editor->canCut())
        return BB_A11Y_STATUS_NOT_FOUND;

    editor->cut();
    return BB_A11Y_STATUS_OK;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::paste(const bb_a11y_accessible_t accessible)
{
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    if (!coreObject)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    Editor* editor = editorForAccessible(coreObject);
    if (!editor || !editor->canPaste())
        return BB_A11Y_STATUS_NOT_FOUND;

    editor->paste();
    return BB_A11Y_STATUS_OK;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::replace(const bb_a11y_accessible_t accessible, const char* text)
{
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    if (!coreObject)
        return BB_A11Y_STATUS_INVALID_ACCESSIBLE;

    Editor* editor = editorForAccessible(coreObject);
    if (!editor || !editor->canDelete() || !editor->canEdit())
        return BB_A11Y_STATUS_NOT_FOUND;

    editor->replaceSelectionWithText(String(text), false, true);
    return BB_A11Y_STATUS_OK;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::setAttributes(const bb_a11y_accessible_t /*accessible*/, const char* /*attributes*/ )
{
    notImplemented();
    return BB_A11Y_STATUS_NOT_SUPPORTED;
}

#endif // HAVE(ACCESSIBILITY)
