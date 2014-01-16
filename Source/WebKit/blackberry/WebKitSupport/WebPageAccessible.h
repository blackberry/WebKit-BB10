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

#ifndef WebPageAccessible_h
#define WebPageAccessible_h

#include <bb_a11y/rt.h>

namespace BlackBerry {
namespace WebKit {

class WebPage;

class WebPageAccessible {
public:
    WebPageAccessible(bb_a11y_accessible_t /*accessible*/, WebPage* /*webPage*/);
    ~WebPageAccessible();

    bb_a11y_accessible_t accessible();

    bb_a11y_status_t getName(char* name, int nameMaxSize, char** nameOverflow);
    bb_a11y_status_t getDescription(char* description, int descriptionMaxSize, char** descriptionOverflow);
    bb_a11y_status_t getParent(bb_a11y_accessible_t* parent);
    bb_a11y_status_t getIndexInParent(int* index);
    bb_a11y_status_t getChildCount(int* childCount);
    bb_a11y_status_t getChildAtIndex(unsigned index, bb_a11y_accessible_t* child);
    bb_a11y_status_t getRole(bb_a11y_role_t* /*role*/, char* extendedRole, int extendedRoleMaxSize, char** extendedRoleOverflow);
    bb_a11y_status_t getState(_Uint64t* state);
    bb_a11y_status_t getRelations(bb_a11y_relation_t /*relation*/, bb_a11y_accessible_t** relations, int* numRelations);

private:
    bb_a11y_accessible_t m_accessible;
    WebPage* m_webPage;
};

} // namespace WebKit
} // namespace BlackBerry

#endif // WebPageAccessible_h
