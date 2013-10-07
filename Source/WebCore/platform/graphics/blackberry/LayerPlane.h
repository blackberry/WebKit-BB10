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

#ifndef LayerPlane_h
#define LayerPlane_h

#if USE(ACCELERATED_COMPOSITING)

#include <FloatPoint3D.h>

namespace WebCore {

class LayerPolygon;

// Specifies a clip plane with normal n and containing point p_0
// as p * n + d = 0, d = -p_0 * n. The asterisk is dot product.
class LayerPlane {
public:
    LayerPlane()
        : m_planeConstant(0)
    {
    }

    LayerPlane(const FloatPoint3D& normal, float planeConstant)
        : m_normal(normal)
        , m_planeConstant(planeConstant)
    {
    }

    FloatPoint3D normal() const { return m_normal; }
    float planeConstant() const { return m_planeConstant; }

    inline bool isPointInside(const FloatPoint3D& p) const
    {
        return p * m_normal + m_planeConstant > 0;
    }

    inline bool arePointsOnSameSide(const FloatPoint3D& p1, const FloatPoint3D& p2) const
    {
        return isPointInside(p1) == isPointInside(p2);
    }

    inline FloatPoint3D computeIntersection(const FloatPoint3D& p1, const FloatPoint3D& p2) const
    {
        float u = (-m_planeConstant - p1 * m_normal) / ((p2 - p1) * m_normal);
        return p1 + u * (p2 - p1);
    }

    float findZ(const FloatPoint3D& xyPoint)
    {
        // Solving for z in the plane equation
        if (m_normal.z())
            return (-m_planeConstant - m_normal.x() * xyPoint.x() - m_normal.y() * xyPoint.y()) / m_normal.z();

        return 0;
    }

    // Assumes the camera is facing along the z axis towards negative z.
    void ensureForwardFacing()
    {
        if (m_normal.z() < 0) {
            m_normal.scale(-1, -1, -1);
            m_planeConstant = -m_planeConstant;
        }
    }

    enum Classification {
        Coplanar,
        Back,
        Front,
        Spanning,
    };

    Classification classify(const LayerPolygon&) const;

    bool operator==(const LayerPlane& other) const
    {
        return m_normal == other.m_normal && m_planeConstant == other.m_planeConstant;
    }

protected:
    FloatPoint3D m_normal;
    float m_planeConstant;
};

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)

#endif // LayerPlane_h
