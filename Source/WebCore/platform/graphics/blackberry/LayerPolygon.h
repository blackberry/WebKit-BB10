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

#ifndef LayerPolygon_h
#define LayerPolygon_h

#if USE(ACCELERATED_COMPOSITING)

#include "FloatPoint3D.h"
#include "LayerPlane.h"
#include <stdio.h>

#define MAX_VERTEX_COUNT 12

namespace WebCore {

class LayerPolygon;
class LayerCompositingThread;

// Represents a planar two-sided polygon for use with LayerBSPTree.
// LayerPolygon can be allocated by MemoryBucket, so its destructor may not run.
// If you change this class, make sure not to add any data members that require destruction.
class LayerPolygon {
public:
    LayerPolygon createEmptyPolygon() const
    {
        return LayerPolygon(m_plane, m_paintersAlgorithmIndex, m_layerPointer);
    }

    LayerPolygon() : m_vertexCount(0), m_layerPointer(0), m_paintersAlgorithmIndex(0), m_isClipped(false) { }
    LayerPolygon(const LayerPlane& plane, size_t paintersAlgorithmIndex, LayerCompositingThread* layerPointer)
        : m_vertexCount(0)
        , m_plane(plane)
        , m_layerPointer(layerPointer)
        , m_paintersAlgorithmIndex(paintersAlgorithmIndex)
        , m_isClipped(false)
    {
        ensureForwardFacingPlane();
    }

    ~LayerPolygon() { }

    size_t vertexCount() const { return m_vertexCount; }
    const FloatPoint3D& operator[](size_t i) const { return vertex[i]; }

    LayerPlane plane() const { return m_plane; }
    size_t paintersAlgorithmIndex() const { return m_paintersAlgorithmIndex; }
    LayerCompositingThread* layerPointer() const { return m_layerPointer; }

    void addVertex(const FloatPoint3D&);

    bool isClipped() const { return m_isClipped; }
    void setClipped() { m_isClipped = true; }

private:
    void ensureForwardFacingPlane();

    FloatPoint3D vertex[MAX_VERTEX_COUNT];
    size_t m_vertexCount;

    LayerPlane m_plane;

    LayerCompositingThread* m_layerPointer;
    size_t m_paintersAlgorithmIndex;
    bool m_isClipped;
};

} // namespace Webcore

#endif // USE(ACCELERATED_COMPOSITING)

#endif // LayerPolygon_h
