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

#include "LayerPlane.h"

#include "LayerPolygon.h"

namespace WebCore {

LayerPlane::Classification LayerPlane::classify(const LayerPolygon& polygon) const
{
    const float epsilon = 1e-3;

    if (polygon.plane() == *this)
        return Coplanar;

    bool front = false;
    bool back = false;

    for (size_t i = 0; i < polygon.vertexCount(); i++) {
        float d = polygon[i].dot(m_normal) + m_planeConstant;

        if (d > epsilon)
            front = true;
        else if (d < -epsilon)
            back = true;
    }

    if (front && back)
        return Spanning;
    if (front)
        return Front;
    if (back)
        return Back;

    return Coplanar;
}

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)
