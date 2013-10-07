/*
 * Copyright (C) 2013 BlackBerry Limited. All rights reserved.
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

#if USE(ACCELERATED_COMPOSITING)

#include "LayerPolygon.h"

#include "LayerBSPTree.h"

#include <wtf/Assertions.h>

namespace WebCore {

void LayerPolygon::ensureForwardFacingPlane()
{
    m_plane.ensureForwardFacing();
}

void LayerPolygon::addVertex(const FloatPoint3D &v)
{
    if (m_vertexCount < MAX_VERTEX_COUNT)
        vertex[m_vertexCount++] = v;
    else
        LOG_ERROR("Trying to add more than %d vertices\n", MAX_VERTEX_COUNT);
}

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)
