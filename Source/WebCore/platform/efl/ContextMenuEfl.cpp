/*
 *  Copyright (C) 2007 Holger Hans Peter Freyther
 *  Copyright (C) 2008 INdT - Instituto Nokia de Tecnologia
 *  Copyright (C) 2009-2010 ProFUSION embedded systems
 *  Copyright (C) 2011 Samsung Electronics
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"

#if ENABLE(CONTEXT_MENUS)

#include "ContextMenu.h"

#include "NotImplemented.h"
#include "PlatformMenuDescription.h"

namespace WebCore {

ContextMenu::ContextMenu(PlatformContextMenu menu)
{
    getContextMenuItems(menu, m_items);
}

void ContextMenu::getContextMenuItems(PlatformContextMenu, Vector<ContextMenuItem>&)
{
    notImplemented();
}

PlatformContextMenu ContextMenu::createPlatformContextMenuFromItems(const Vector<ContextMenuItem>&)
{
    notImplemented();
    return 0;
}

PlatformContextMenu ContextMenu::platformContextMenu() const
{
    return createPlatformContextMenuFromItems(m_items);
}

}
#endif // ENABLE(CONTEXT_MENUS)
