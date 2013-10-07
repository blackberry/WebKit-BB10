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


#ifndef LayerBSPTree_h
#define LayerBSPTree_h

#if USE(ACCELERATED_COMPOSITING)

#include "LayerPolygon.h"
#include "MemoryBucket.h"

#include <wtf/Vector.h>

#define ENABLE_BSP_TREE_TIMERS 0

namespace WebCore {

class LayerBSPTreeNode;

// Represents a BSP tree of LayerPolygon instances useful for z sorting.
// Since this implementation internally creates two-sided polygons, it's not suitable for backface culling.
class LayerBSPTree {
public:
    LayerBSPTree();
    ~LayerBSPTree() { }

    void prepareFrame();

    // Returns pointers to MemoryBucket-allocated LayerPolygon:s that are guaranteed to be valid until the next call
    // to prepareFrame.
    Vector<LayerPolygon*> zSort(const Vector<LayerPolygon>&);

private:
    size_t build(const Vector<LayerPolygon>&);
    void collect(Vector<LayerPolygon*>& orderedPolygons);
    LayerBSPTreeNode* createNode(const LayerPolygon&);
    void buildRecursive(const LayerPolygon&, LayerBSPTreeNode*);
    void collectRecursive(LayerBSPTreeNode*, Vector<LayerPolygon*>&);

    size_t m_nodeCount;
    LayerBSPTreeNode* m_rootNode;

    MemoryBucket m_bucket;

#if ENABLE_BSP_TREE_TIMERS
    size_t m_incomingLayerCount;
    size_t m_outgoingLayerCount;

    int m_frameCount;
    int m_buildCount;
    double m_timeSum;
#endif
};

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)

#endif // LayerBSPTree_h
