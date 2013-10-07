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

#include "WebPageAccessible.h"

#include "AXObjectCache.h"
#include "Frame.h"
#include "WebAccessibilityFunctionHandler.h"
#include "WebAccessibilityFunctionHandler_p.h"
#include "WebPage_p.h"

#include <BlackBerryPlatformLog.h>

namespace BlackBerry {
namespace WebKit {

WebPageAccessible::WebPageAccessible(bb_a11y_accessible_t accessible, WebPage* webPage)
    : m_accessible(accessible)
    , m_webPage(webPage)
{
}

WebPageAccessible::~WebPageAccessible()
{
}

bb_a11y_accessible_t WebPageAccessible::accessible()
{
    return m_accessible;
}

bb_a11y_status_t WebPageAccessible::getName(char* /*name*/, int /*nameMaxSize*/, char** /*nameOverflow*/)
{
    return BB_A11Y_STATUS_NOT_SUPPORTED;
}

bb_a11y_status_t WebPageAccessible::getDescription(char* /*description*/, int /*descriptionMaxSize*/, char** /*descriptionOverflow*/)
{
    return BB_A11Y_STATUS_NOT_SUPPORTED;
}

bb_a11y_status_t WebPageAccessible::getParent(bb_a11y_accessible_t* parent)
{
    return BB_A11Y_STATUS_NOT_SUPPORTED;
}

bb_a11y_status_t WebPageAccessible::getIndexInParent(int* /*index*/)
{
    return BB_A11Y_STATUS_NOT_SUPPORTED;
}

bb_a11y_status_t WebPageAccessible::getChildCount(int* childCount)
{
    *childCount = 0;

    WebCore::Frame *coreFrame = m_webPage->d->mainFrame();
    if (!coreFrame || !coreFrame->document())
        return BB_A11Y_STATUS_OK;

    WebCore::AccessibilityObject* coreRootObject = coreFrame->document()->axObjectCache()->rootObject();

    if (coreRootObject)
        *childCount = 1;

    return BB_A11Y_STATUS_OK;
}

bb_a11y_status_t WebPageAccessible::getChildAtIndex(unsigned index, bb_a11y_accessible_t* child)
{
    WebCore::Frame *coreFrame = m_webPage->d->mainFrame();
    if (!coreFrame || !coreFrame->document())
        return BB_A11Y_STATUS_INVALID_INDEX;

    WebCore::AccessibilityObject* coreRootObject = coreFrame->document()->axObjectCache()->rootObject();

    if (coreRootObject && !index) {
        WebAccessibilityFunctionHandler* handler = m_webPage->accessibilityFunctionHandler();

        *child = handler->d->getAccessible(coreRootObject);

        return BB_A11Y_STATUS_OK;
    }

    return BB_A11Y_STATUS_INVALID_INDEX;
}

bb_a11y_status_t WebPageAccessible::getRole(bb_a11y_role_t* role, char* /*extendedRole*/, int /*extendedRoleMaxSize*/, char** /*extendedRoleOverflow*/)
{
    *role = BB_A11Y_ROLE_PLUG;
    return BB_A11Y_STATUS_OK;
}

bb_a11y_status_t WebPageAccessible::getState(_Uint64t* state)
{
    *state = 0;
    return BB_A11Y_STATUS_OK;
}

bb_a11y_status_t WebPageAccessible::getRelations(bb_a11y_relation_t /*relation*/, bb_a11y_accessible_t** /*relations*/, int* /*numRelations*/)
{
    return BB_A11Y_STATUS_NOT_SUPPORTED;
}

}
}
