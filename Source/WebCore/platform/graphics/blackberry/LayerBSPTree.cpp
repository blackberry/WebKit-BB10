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

#include "LayerBSPTree.h"

#include "LayerPlane.h"

#if ENABLE_BSP_TREE_TIMERS
#include <BlackBerryPlatformStopWatch.h>
#endif

namespace WebCore {

// Don't make this too small, or the MemoryBucket will have to grow multiple times initially.
const size_t initialNodeCapacity = 100;

// LayerBSPTreeNode is allocated by MemoryBucket, so its destructor will never run.
// If you change this class, make sure not to add any data members that require destruction.
class LayerBSPTreeNode {
public:
    LayerBSPTreeNode(const LayerPolygon& polygon)
        : m_polygon(polygon)
        , m_front(0)
        , m_back(0)
    {
    }

    ~LayerBSPTreeNode() { }

    void collect(Vector<LayerPolygon*>& orderedPolygons)
    {
        // LayerBSPTreeNode is stored in MemoryBucket, so this pointer will be valid until the next frame.
        orderedPolygons.append(&m_polygon);
    }

    LayerPlane::Classification classify(const LayerPolygon&);
    void split(const LayerPolygon& originalPolygon, LayerPolygon &frontPolygon, LayerPolygon &backPolygon);

    LayerPolygon m_polygon;
    LayerBSPTreeNode* m_front;
    LayerBSPTreeNode* m_back;
};

void LayerBSPTreeNode::split(const LayerPolygon &originalPolygon, LayerPolygon &frontPolygon, LayerPolygon &backPolygon)
{
    for (size_t i = 0; i < originalPolygon.vertexCount(); i++) {
        FloatPoint3D pA = originalPolygon[i];
        FloatPoint3D pB = originalPolygon[(i + 1) % originalPolygon.vertexCount()];
        if (m_polygon.plane().arePointsOnSameSide(pA, pB)) {
            if (m_polygon.plane().isPointInside(pA))
                frontPolygon.addVertex(pA);
            else
                backPolygon.addVertex(pA);
        } else {
            FloatPoint3D intersection = m_polygon.plane().computeIntersection(pA, pB);
            if (m_polygon.plane().isPointInside(pA))
                frontPolygon.addVertex(pA);
            else
                backPolygon.addVertex(pA);
            frontPolygon.addVertex(intersection);
            backPolygon.addVertex(intersection);
        }
    }
    frontPolygon.setClipped();
    backPolygon.setClipped();
}

LayerPlane::Classification LayerBSPTreeNode::classify(const LayerPolygon& polygon)
{
    LayerPlane::Classification result = m_polygon.plane().classify(polygon);
    if (result == LayerPlane::Coplanar) {
        if (polygon.paintersAlgorithmIndex() < m_polygon.paintersAlgorithmIndex())
            return LayerPlane::Back;
        // else
        return LayerPlane::Front;
    }

    return result;
}

LayerBSPTree::LayerBSPTree()
    : m_nodeCount(0)
    , m_rootNode(0)
    , m_bucket(sizeof(LayerBSPTreeNode) * initialNodeCapacity)
#if ENABLE_BSP_TREE_TIMERS
    , m_incomingLayerCount(0)
    , m_outgoingLayerCount(0)
    , m_frameCount(0)
    , m_buildCount(0)
    , m_timeSum(0)
#endif
{
}

void LayerBSPTree::prepareFrame()
{
    // FIXME: Perhaps make it so that the memory bucket might actually get smaller if no large amounts have been thrown
    //        in for a while? Currently considered not worth the effort considering the small memory savings.
    m_bucket.clear();

#if ENABLE_BSP_TREE_TIMERS
    const int numberOfFramesBetweenPrints = 50;
    m_frameCount++;
    if (!(m_frameCount % numberOfFramesBetweenPrints)) {
        if (m_buildCount) {
            printf("Calls to LayerBSPTree::build() per frame: %f\n", static_cast<float>(m_buildCount) / numberOfFramesBetweenPrints);
            printf("    Average incoming polygon count per call to build: %f\n", static_cast<float>(m_incomingLayerCount) / m_buildCount);
            printf("    Average outgoing polygon count per call to build: %f\n", static_cast<float>(m_outgoingLayerCount) / m_buildCount);
        }

        printf("Sorting time: %f\n\n", m_timeSum / numberOfFramesBetweenPrints);
        m_timeSum = 0;
        m_buildCount = 0;
        m_incomingLayerCount = 0;
        m_outgoingLayerCount = 0;
    }
#endif
}

Vector<LayerPolygon*> LayerBSPTree::zSort(const Vector<LayerPolygon>& polygons)
{
#if ENABLE_BSP_TREE_TIMERS
    m_buildCount++;
    BlackBerry::Platform::HighPrecisionStopWatch timer;
    timer.start();
#endif

    // Build tree and calculate m_nodeCount.
    size_t nodeCount = build(polygons);

    Vector<LayerPolygon*> sortedPolygons;
    sortedPolygons.reserveInitialCapacity(nodeCount);
    collect(sortedPolygons);

#if ENABLE_BSP_TREE_TIMERS
    m_incomingLayerCount += polygons.size();
    m_timeSum += timer.elapsedMS();
    m_outgoingLayerCount += sortedPolygons.size();
#endif

    return sortedPolygons;
}

size_t LayerBSPTree::build(const Vector<LayerPolygon>& polygons)
{
    m_nodeCount = 0;

    if (polygons.isEmpty()) {
        m_rootNode = 0;
        return m_nodeCount;
    }

    m_rootNode = createNode(polygons[0]);

    // Start at 1 since we already created root node from polygons[0].
    for (size_t i = 1; i < polygons.size(); ++i)
        buildRecursive(polygons[i], m_rootNode);

    return m_nodeCount;
}

void LayerBSPTree::buildRecursive(const LayerPolygon& polygon, LayerBSPTreeNode* node)
{
    switch (node->classify(polygon)) {
    case LayerPlane::Spanning: {
        LayerPolygon frontPolygon = polygon.createEmptyPolygon();
        LayerPolygon backPolygon = polygon.createEmptyPolygon();
        node->split(polygon, frontPolygon, backPolygon);

        if (node->m_back)
            buildRecursive(backPolygon, node->m_back);
        else
            node->m_back = createNode(backPolygon);

        if (node->m_front)
            buildRecursive(frontPolygon, node->m_front);
        else
            node->m_front = createNode(frontPolygon);
        break;
    }
    case LayerPlane::Back: {
        if (node->m_back)
            buildRecursive(polygon, node->m_back);
        else
            node->m_back = createNode(polygon);
        break;
    }
    case LayerPlane::Front: {
        if (node->m_front)
            buildRecursive(polygon, node->m_front);
        else
            node->m_front = createNode(polygon);
        break;
    }
    case LayerPlane::Coplanar:
        // Handled inside LayerBSPTreeNode::split()
        ASSERT_NOT_REACHED();
        break;
    }
}

void LayerBSPTree::collect(Vector<LayerPolygon*>& orderedPolygons)
{
    if (m_rootNode)
        collectRecursive(m_rootNode, orderedPolygons);
}

void LayerBSPTree::collectRecursive(LayerBSPTreeNode* node, Vector<LayerPolygon*>& orderedPolygons)
{
    // Note that this code is hardwired to assume all objects are facing towards the camera. By modifying this code,
    // the BSP tree could be used to collect objects sorted relative to a certain camera position and direction.
    if (node->m_back)
        collectRecursive(node->m_back, orderedPolygons);
    node->collect(orderedPolygons);
    if (node->m_front)
        collectRecursive(node->m_front, orderedPolygons);
}

LayerBSPTreeNode* LayerBSPTree::createNode(const LayerPolygon& polygon)
{
    m_nodeCount++;
    return m_bucket.createObject<LayerBSPTreeNode, LayerPolygon>(polygon);
}

} // namespace Webcore

#endif // USE(ACCELERATED_COMPOSITING)
