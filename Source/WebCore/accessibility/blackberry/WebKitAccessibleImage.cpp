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
#include "NotImplemented.h"
#include "WebKitAccessibleWrapperBlackBerry.h"

using namespace WebCore;

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::isImage(const bb_a11y_accessible_t accessible, bool* result)
{
    AccessibilityObject* coreObject = getAccessibilityObject(accessible);
    *result = coreObject ? coreObject->isImage() : 0;
    return BB_A11Y_STATUS_OK;
}

bb_a11y_status_t WebKitAccessibleWrapperBlackBerry::getUuid(const bb_a11y_accessible_t /*accessible*/, char* axUUID, int maxSize, char** overflow)
{
    notImplemented();

    bb_a11y_bridge_copy_string("", axUUID, maxSize, overflow);

    return BB_A11Y_STATUS_NOT_SUPPORTED;
}

#endif // HAVE(ACCESSIBILITY)
