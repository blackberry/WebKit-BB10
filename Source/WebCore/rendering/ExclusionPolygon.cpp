/*
 * Copyright (C) 2012 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ExclusionPolygon.h"

#include <wtf/MathExtras.h>

namespace WebCore {

enum EdgeIntersectionType {
    Normal,
    VertexMinY,
    VertexMaxY,
    VertexYBoth
};

struct EdgeIntersection {
    const FloatPolygonEdge* edge;
    FloatPoint point;
    EdgeIntersectionType type;
};

static inline float leftSide(const FloatPoint& vertex1, const FloatPoint& vertex2, const FloatPoint& point)
{
    return ((point.x() - vertex1.x()) * (vertex2.y() - vertex1.y())) - ((vertex2.x() - vertex1.x()) * (point.y() - vertex1.y()));
}

static bool computeXIntersection(const FloatPolygonEdge* edgePointer, float y, EdgeIntersection& result)
{
    const FloatPolygonEdge& edge = *edgePointer;

    if (edge.minY() > y || edge.maxY() < y)
        return false;

    const FloatPoint& vertex1 = edge.vertex1();
    const FloatPoint& vertex2 = edge.vertex2();
    float dy = vertex2.y() - vertex1.y();

    float intersectionX;
    EdgeIntersectionType intersectionType;

    if (!dy) {
        intersectionType = VertexYBoth;
        intersectionX = edge.minX();
    } else if (y == edge.minY()) {
        intersectionType = VertexMinY;
        intersectionX = (vertex1.y() < vertex2.y()) ? vertex1.x() : vertex2.x();
    } else if (y == edge.maxY()) {
        intersectionType = VertexMaxY;
        intersectionX = (vertex1.y() > vertex2.y()) ? vertex1.x() : vertex2.x();
    } else {
        intersectionType = Normal;
        intersectionX = ((y - vertex1.y()) * (vertex2.x() - vertex1.x()) / dy) + vertex1.x();
    }

    result.edge = edgePointer;
    result.type = intersectionType;
    result.point.set(intersectionX, y);

    return true;
}

static inline bool getVertexIntersectionVertices(const EdgeIntersection& intersection, FloatPoint& prevVertex, FloatPoint& thisVertex, FloatPoint& nextVertex)
{
    if (intersection.type != VertexMinY && intersection.type != VertexMaxY)
        return false;

    ASSERT(intersection.edge && intersection.edge->polygon());
    const FloatPolygon& polygon = *(intersection.edge->polygon());
    const FloatPolygonEdge& thisEdge = *(intersection.edge);

    if ((intersection.type == VertexMinY && (thisEdge.vertex1().y() < thisEdge.vertex2().y()))
        || (intersection.type == VertexMaxY && (thisEdge.vertex1().y() > thisEdge.vertex2().y()))) {
        prevVertex = polygon.vertexAt(thisEdge.previousEdge().vertexIndex1());
        thisVertex = polygon.vertexAt(thisEdge.vertexIndex1());
        nextVertex = polygon.vertexAt(thisEdge.vertexIndex2());
    } else {
        prevVertex = polygon.vertexAt(thisEdge.vertexIndex1());
        thisVertex = polygon.vertexAt(thisEdge.vertexIndex2());
        nextVertex = polygon.vertexAt(thisEdge.nextEdge().vertexIndex2());
    }

    return true;
}

static inline bool appendIntervalX(float x, bool inside, Vector<ExclusionInterval>& result)
{
    if (!inside)
        result.append(ExclusionInterval(x));
    else
        result[result.size() - 1].x2 = x;

    return !inside;
}

static bool compareEdgeIntersectionX(const EdgeIntersection& intersection1, const EdgeIntersection& intersection2)
{
    float x1 = intersection1.point.x();
    float x2 = intersection2.point.x();
    return (x1 == x2) ? intersection1.type < intersection2.type : x1 < x2;
}

static void computeXIntersections(const FloatPolygon& polygon, float y, bool isMinY, Vector<ExclusionInterval>& result)
{
    Vector<const FloatPolygonEdge*> edges;
    if (!polygon.overlappingEdges(y, y, edges))
        return;

    Vector<EdgeIntersection> intersections;
    EdgeIntersection intersection;
    for (unsigned i = 0; i < edges.size(); ++i) {
        if (computeXIntersection(edges[i], y, intersection) && intersection.type != VertexYBoth)
            intersections.append(intersection);
    }

    if (intersections.size() < 2)
        return;

    std::sort(intersections.begin(), intersections.end(), WebCore::compareEdgeIntersectionX);

    unsigned index = 0;
    int windCount = 0;
    bool inside = false;

    while (index < intersections.size()) {
        const EdgeIntersection& thisIntersection = intersections[index];
        if (index + 1 < intersections.size()) {
            const EdgeIntersection& nextIntersection = intersections[index + 1];
            if ((thisIntersection.point.x() == nextIntersection.point.x()) && (thisIntersection.type == VertexMinY || thisIntersection.type == VertexMaxY)) {
                if (thisIntersection.type == nextIntersection.type) {
                    // Skip pairs of intersections whose types are VertexMaxY,VertexMaxY and VertexMinY,VertexMinY.
                    index += 2;
                } else {
                    // Replace pairs of intersections whose types are VertexMinY,VertexMaxY or VertexMaxY,VertexMinY with one intersection.
                    ++index;
                }
                continue;
            }
        }

        const FloatPolygonEdge& thisEdge = *thisIntersection.edge;
        bool evenOddCrossing = !windCount;

        if (polygon.fillRule() == RULE_EVENODD) {
            windCount += (thisEdge.vertex2().y() > thisEdge.vertex1().y()) ? 1 : -1;
            evenOddCrossing = evenOddCrossing || !windCount;
        }

        if (evenOddCrossing) {
            bool edgeCrossing = thisIntersection.type == Normal;
            if (!edgeCrossing) {
                FloatPoint prevVertex;
                FloatPoint thisVertex;
                FloatPoint nextVertex;

                if (getVertexIntersectionVertices(thisIntersection, prevVertex, thisVertex, nextVertex)) {
                    if (nextVertex.y() == y)
                        edgeCrossing = (isMinY) ? prevVertex.y() > y : prevVertex.y() < y;
                    else if (prevVertex.y() == y)
                        edgeCrossing = (isMinY) ? nextVertex.y() > y : nextVertex.y() < y;
                    else
                        edgeCrossing = true;
                }
            }
            if (edgeCrossing)
                inside = appendIntervalX(thisIntersection.point.x(), inside, result);
        }

        ++index;
    }
}

static void computeOverlappingEdgeXProjections(const FloatPolygon& polygon, float y1, float y2, Vector<ExclusionInterval>& result)
{
    Vector<const FloatPolygonEdge*> edges;
    if (!polygon.overlappingEdges(y1, y2, edges))
        return;

    EdgeIntersection intersection;
    for (unsigned i = 0; i < edges.size(); ++i) {
        const FloatPolygonEdge *edge = edges[i];
        float x1;
        float x2;

        if (edge->minY() < y1) {
            computeXIntersection(edge, y1, intersection);
            x1 = intersection.point.x();
        } else
            x1 = (edge->vertex1().y() < edge->vertex2().y()) ? edge->vertex1().x() : edge->vertex2().x();

        if (edge->maxY() > y2) {
            computeXIntersection(edge, y2, intersection);
            x2 = intersection.point.x();
        } else
            x2 = (edge->vertex1().y() > edge->vertex2().y()) ? edge->vertex1().x() : edge->vertex2().x();

        if (x1 > x2)
            std::swap(x1, x2);

        if (x2 > x1)
            result.append(ExclusionInterval(x1, x2));
    }

    sortExclusionIntervals(result);
}

void ExclusionPolygon::getExcludedIntervals(float logicalTop, float logicalHeight, SegmentList& result) const
{
    if (isEmpty())
        return;

    float y1 = logicalTop;
    float y2 = y1 + logicalHeight;

    Vector<ExclusionInterval> y1XIntervals, y2XIntervals;
    computeXIntersections(m_polygon, y1, true, y1XIntervals);
    computeXIntersections(m_polygon, y2, false, y2XIntervals);

    Vector<ExclusionInterval> mergedIntervals;
    mergeExclusionIntervals(y1XIntervals, y2XIntervals, mergedIntervals);

    Vector<ExclusionInterval> edgeIntervals;
    computeOverlappingEdgeXProjections(m_polygon, y1, y2, edgeIntervals);

    Vector<ExclusionInterval> excludedIntervals;
    mergeExclusionIntervals(mergedIntervals, edgeIntervals, excludedIntervals);

    for (unsigned i = 0; i < excludedIntervals.size(); ++i) {
        ExclusionInterval interval = excludedIntervals[i];
        result.append(LineSegment(interval.x1, interval.x2));
    }
}

void ExclusionPolygon::getIncludedIntervals(float logicalTop, float logicalHeight, SegmentList& result) const
{
    if (isEmpty())
        return;

    float y1 = logicalTop;
    float y2 = y1 + logicalHeight;

    Vector<ExclusionInterval> y1XIntervals, y2XIntervals;
    computeXIntersections(m_polygon, y1, true, y1XIntervals);
    computeXIntersections(m_polygon, y2, false, y2XIntervals);

    Vector<ExclusionInterval> commonIntervals;
    intersectExclusionIntervals(y1XIntervals, y2XIntervals, commonIntervals);

    Vector<ExclusionInterval> edgeIntervals;
    computeOverlappingEdgeXProjections(m_polygon, y1, y2, edgeIntervals);

    Vector<ExclusionInterval> includedIntervals;
    subtractExclusionIntervals(commonIntervals, edgeIntervals, includedIntervals);

    for (unsigned i = 0; i < includedIntervals.size(); ++i) {
        ExclusionInterval interval = includedIntervals[i];
        result.append(LineSegment(interval.x1, interval.x2));
    }
}

static inline bool isReflexVertex(const FloatPoint& prevVertex, const FloatPoint& vertex, const FloatPoint& nextVertex)
{
    return leftSide(prevVertex, nextVertex, vertex) < 0;
}

static inline bool firstFitRectInPolygon(const FloatPolygon& polygon, const FloatRect& rect, unsigned offsetEdgeIndex1, unsigned offsetEdgeIndex2)
{
    Vector<const FloatPolygonEdge*> edges;
    if (!polygon.overlappingEdges(rect.y(), rect.maxY(), edges))
        return true;

    for (unsigned i = 0; i < edges.size(); ++i) {
        const FloatPolygonEdge* edge = edges[i];
        if (edge->edgeIndex() != offsetEdgeIndex1 && edge->edgeIndex() != offsetEdgeIndex2 && edge->overlapsRect(rect))
            return false;
    }

    return true;
}

static inline bool aboveOrToTheLeft(const FloatRect& r1, const FloatRect& r2)
{
    if (r1.y() < r2.y())
        return true;
    if (r1.y() == r2.y())
        return r1.x() < r2.x();
    return false;
}

bool ExclusionPolygon::firstIncludedIntervalLogicalTop(float minLogicalIntervalTop, const FloatSize& minLogicalIntervalSize, float& result) const
{
    const FloatRect boundingBox = m_polygon.boundingBox();
    if (minLogicalIntervalSize.width() > boundingBox.width())
        return false;

    float minY = std::max(boundingBox.y(), minLogicalIntervalTop);
    float maxY = minY + minLogicalIntervalSize.height();

    if (maxY > boundingBox.maxY())
        return false;

    Vector<const FloatPolygonEdge*> edges;
    m_polygon.overlappingEdges(minLogicalIntervalTop, boundingBox.maxY(), edges);

    float dx = minLogicalIntervalSize.width() / 2;
    float dy = minLogicalIntervalSize.height() / 2;
    Vector<OffsetPolygonEdge> offsetEdges;

    for (unsigned i = 0; i < edges.size(); ++i) {
        const FloatPolygonEdge& edge = *(edges[i]);
        const FloatPoint& vertex0 = edge.previousEdge().vertex1();
        const FloatPoint& vertex1 = edge.vertex1();
        const FloatPoint& vertex2 = edge.vertex2();
        Vector<OffsetPolygonEdge> offsetEdgeBuffer;

        if (vertex2.y() > vertex1.y() ? vertex2.x() >= vertex1.x() : vertex1.x() >= vertex2.x()) {
            offsetEdgeBuffer.append(OffsetPolygonEdge(edge, FloatSize(dx, -dy)));
            offsetEdgeBuffer.append(OffsetPolygonEdge(edge, FloatSize(-dx, dy)));
        } else {
            offsetEdgeBuffer.append(OffsetPolygonEdge(edge, FloatSize(dx, dy)));
            offsetEdgeBuffer.append(OffsetPolygonEdge(edge, FloatSize(-dx, -dy)));
        }

        if (isReflexVertex(vertex0, vertex1, vertex2)) {
            if (vertex2.x() <= vertex1.x() && vertex0.x() <= vertex1.x())
                offsetEdgeBuffer.append(OffsetPolygonEdge(vertex1, FloatSize(dx, -dy), FloatSize(dx, dy)));
            else if (vertex2.x() >= vertex1.x() && vertex0.x() >= vertex1.x())
                offsetEdgeBuffer.append(OffsetPolygonEdge(vertex1, FloatSize(-dx, -dy), FloatSize(-dx, dy)));
            if (vertex2.y() <= vertex1.y() && vertex0.y() <= vertex1.y())
                offsetEdgeBuffer.append(OffsetPolygonEdge(vertex1, FloatSize(-dx, dy), FloatSize(dx, dy)));
            else if (vertex2.y() >= vertex1.y() && vertex0.y() >= vertex1.y())
                offsetEdgeBuffer.append(OffsetPolygonEdge(vertex1, FloatSize(-dx, -dy), FloatSize(dx, -dy)));
        }

        for (unsigned j = 0; j < offsetEdgeBuffer.size(); ++j)
            if (offsetEdgeBuffer[j].maxY() >= minY)
                offsetEdges.append(offsetEdgeBuffer[j]);
    }

    offsetEdges.append(OffsetPolygonEdge(m_polygon, minLogicalIntervalTop, FloatSize(0, dy)));

    FloatPoint offsetEdgesIntersection;
    FloatRect firstFitRect;
    bool firstFitFound = false;

    for (unsigned i = 0; i < offsetEdges.size() - 1; ++i) {
        for (unsigned j = i + 1; j < offsetEdges.size(); ++j) {
            if (offsetEdges[i].intersection(offsetEdges[j], offsetEdgesIntersection)) {
                FloatPoint potentialFirstFitLocation(offsetEdgesIntersection.x() - dx, offsetEdgesIntersection.y() - dy);
                FloatRect potentialFirstFitRect(potentialFirstFitLocation, minLogicalIntervalSize);
                if ((potentialFirstFitLocation.y() >= minLogicalIntervalTop)
                    && (!firstFitFound || aboveOrToTheLeft(potentialFirstFitRect, firstFitRect))
                    && m_polygon.contains(offsetEdgesIntersection)
                    && firstFitRectInPolygon(m_polygon, potentialFirstFitRect, offsetEdges[i].edgeIndex(), offsetEdges[j].edgeIndex())) {
                    firstFitFound = true;
                    firstFitRect = potentialFirstFitRect;
                }
            }
        }
    }

    if (firstFitFound)
        result = firstFitRect.y();
    return firstFitFound;
}

} // namespace WebCore
