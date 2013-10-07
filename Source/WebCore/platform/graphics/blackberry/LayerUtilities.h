/*
 * Copyright (C) 2013 Research In Motion Limited. All rights reserved.
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

#ifndef LayerUtilities_h
#define LayerUtilities_h

#if USE(ACCELERATED_COMPOSITING)

#include "AffineTransform.h"
#include "FloatPoint.h"
#include "FloatPoint3D.h"
#include "FloatQuad.h"
#include "FloatSize.h"
#include "LayerCompositingThread.h"
#include "TransformationMatrix.h"

#include <algorithm>
#include <wtf/Vector.h>

namespace WebCore {

// determinant of column vectors
inline float determinant(const FloatSize& a, const FloatSize& b)
{
    return a.width() * b.height() - a.height() * b.width();
}

inline float determinant(const FloatPoint& a, const FloatPoint& b)
{
    return a.x() * b.y() - a.y() * b.x();
}

inline double determinant2x2(double a, double b, double c, double d)
{
    return a * d - b * c;
}

inline double determinant3x3(double a1, double a2, double a3, double b1, double b2, double b3, double c1, double c2, double c3)
{
    return a1 * determinant2x2(b2, b3, c2, c3) - b1 * determinant2x2(a2, a3, c2, c3) + c1 * determinant2x2(a2, a3, b2, b3);
}

// Transforms the direction (0, 0, 1, 0) using the transposed inverse
inline FloatPoint3D forwardVector(const TransformationMatrix& matrix)
{
    // Assign to individual variable names to aid
    // selecting correct values
    double a1 = matrix.m11();
    double b1 = matrix.m12();
    double c1 = matrix.m13();
    double d1 = matrix.m14();

    double a2 = matrix.m21();
    double b2 = matrix.m22();
    double c2 = matrix.m23();
    double d2 = matrix.m24();

    double a4 = matrix.m41();
    double b4 = matrix.m42();
    double c4 = matrix.m43();
    double d4 = matrix.m44();

    // Row column labeling reversed since we transpose rows & columns
    FloatPoint3D result(determinant3x3(b1, b2, b4, c1, c2, c4, d1, d2, d4), -determinant3x3(a1, a2, a4, c1, c2, c4, d1, d2, d4), determinant3x3(a1, a2, a4, b1, b2, b4, d1, d2, d4));
    result.normalize();
    return result;
}

// dot product
inline float dot(const FloatSize& a, const FloatSize& b)
{
    return a.width() * b.width() + a.height() * b.height();
}

// Represents a line, not a finite line segment
class LayerClipEdge {
public:
    LayerClipEdge(const FloatPoint& first, const FloatPoint& second)
        : m_first(first)
        , m_second(second)
    {
    }

    inline bool isPointInside(const FloatPoint& p) const
    {
        // For numeric robustness, we prefer to consider a point to be inside rather than
        // clip it again.
        const float epsilon = 1e-6;
        return determinant(m_second - m_first, p - m_first) > -epsilon;
    }

    inline FloatPoint computeIntersection(const FloatPoint& p1, const FloatPoint& p2) const
    {
        const FloatPoint& p3 = m_first;
        const FloatPoint& p4 = m_second;
        float denominator = determinant(p1 - p2, p3 - p4);
        FloatSize determinants(determinant(p1, p2), determinant(p3, p4));
        FloatPoint result(
            determinant(determinants, FloatSize(p1.x() - p2.x(), p3.x() - p4.x())) / denominator,
            determinant(determinants, FloatSize(p1.y() - p2.y(), p3.y() - p4.y())) / denominator);

        return result;
    }

private:
    FloatPoint m_first;
    FloatPoint m_second;
};

// Sutherland - Hodgman, inner loop
template<typename Point, size_t inlineCapacity, typename ClipPrimitive>
inline Vector<Point, inlineCapacity> intersect(const Vector<Point, inlineCapacity>& inputList, const ClipPrimitive& clipPrimitive)
{
    Vector<Point, inlineCapacity> outputList;
    Point s;
    if (!inputList.isEmpty())
        s = inputList.last();
    for (typename Vector<Point, inlineCapacity>::const_iterator eIterator = inputList.begin(); eIterator != inputList.end(); ++eIterator) {
        const Point& e = *eIterator;
        if (clipPrimitive.isPointInside(e)) {
            if (!clipPrimitive.isPointInside(s))
                outputList.append(clipPrimitive.computeIntersection(s, e));
            outputList.append(e);
        } else {
            if (clipPrimitive.isPointInside(s))
                outputList.append(clipPrimitive.computeIntersection(s, e));
        };
        s = e;
    }
    return outputList;
}

// Sutherland - Hodgman, main driver
template<size_t inlineCapacity>
inline Vector<FloatPoint, inlineCapacity> intersectPolygonWithRect(const Vector<FloatPoint, inlineCapacity>& subjectPolygon, const FloatRect& clipRect)
{
    FloatQuad clipQuad(clipRect);
    Vector<LayerClipEdge> edges;
    edges.append(LayerClipEdge(clipQuad.p1(), clipQuad.p2()));
    edges.append(LayerClipEdge(clipQuad.p2(), clipQuad.p3()));
    edges.append(LayerClipEdge(clipQuad.p3(), clipQuad.p4()));
    edges.append(LayerClipEdge(clipQuad.p4(), clipQuad.p1()));

    Vector<FloatPoint> outputList = subjectPolygon;
    for (Vector<LayerClipEdge>::const_iterator clipEdgeIterator = edges.begin(); clipEdgeIterator != edges.end(); ++clipEdgeIterator) {
        const LayerClipEdge& clipEdge = *clipEdgeIterator;
        Vector<FloatPoint> inputList = outputList;
        outputList = intersect(inputList, clipEdge);
    }
    return outputList;
}

template<typename Point, size_t inlineCapacity>
inline FloatRect boundingBox(const Vector<Point, inlineCapacity>& points)
{
    if (points.isEmpty())
        return FloatRect();
    float xmin, xmax, ymin, ymax;
    xmin = ymin = std::numeric_limits<float>::infinity();
    xmax = ymax = -std::numeric_limits<float>::infinity();
    for (size_t i = 0; i < points.size(); ++i) {
        const Point& p = points[i];
        if (p.x() < xmin)
            xmin = p.x();
        if (p.x() > xmax)
            xmax = p.x();
        if (p.y() < ymin)
            ymin = p.y();
        if (p.y() > ymax)
            ymax = p.y();
    }
    return FloatRect(xmin, ymin, xmax - xmin, ymax - ymin);
}

inline FloatPoint3D computeBarycentricCoordinates(const FloatPoint& p, const FloatPoint& t1, const FloatPoint& t2, const FloatPoint& t3, bool& ok)
{
    // Compute vectors
    FloatSize v0 = t2 - t1;
    FloatSize v1 = t3 - t1;
    FloatSize v2 = p - t1;

    // Compute dot products
    float dot00 = dot(v0, v0);
    float dot01 = dot(v0, v1);
    float dot02 = dot(v0, v2);
    float dot11 = dot(v1, v1);
    float dot12 = dot(v1, v2);

    // Compute barycentric coordinates
    float denominator = (dot00 * dot11 - dot01 * dot01);
    ok = (denominator != 0.0);
    if (!ok)
        return FloatPoint3D();

    float v = (dot11 * dot02 - dot01 * dot12) / denominator;
    float w = (dot00 * dot12 - dot01 * dot02) / denominator;

    return FloatPoint3D(1.0f - v - w, v, w);
}

inline float manhattanDistanceToViewport(const FloatPoint& p)
{
    float d = 0;
    if (fabsf(p.x()) > 1)
        d += fabsf(p.x()) - 1;
    if (fabsf(p.y()) > 1)
        d += fabsf(p.y()) - 1;
    return d;
}

struct UnprojectionVertex {
    FloatPoint xy;
    float w;
    FloatSize uv;
};

inline bool compareManhattanDistanceToViewport(const UnprojectionVertex& a, const UnprojectionVertex& b)
{
    return manhattanDistanceToViewport(a.xy) < manhattanDistanceToViewport(b.xy);
}

template<size_t inlineCapacity>
inline Vector<FloatPoint, inlineCapacity> unproject(LayerCompositingThread* layer, const Vector<FloatPoint, inlineCapacity>& points)
{
    LayerPlane plane = layer->plane();
    Vector<FloatPoint, inlineCapacity> result;
    TransformationMatrix modelViewTransform = layer->modelViewTransform();
    TransformationMatrix viewToTextureTransform = modelViewTransform
        .scaleNonUniform(layer->bounds().width(), layer->bounds().height())
        .translate(-0.5, -0.5)
        .inverse();

    // Extract the 2D part of the projection matrix (which is always an orthographic projection)
    // The matrix could be zeroing z, so ignore the 3D bits. The 2D part is more likely to be invertible.
    const TransformationMatrix& projectionMatrix = layer->layerRenderer()->projectionMatrix();
    AffineTransform inverseProjectionMatrix(projectionMatrix.m11(), projectionMatrix.m21(), projectionMatrix.m12(), projectionMatrix.m22(), projectionMatrix.m41(), projectionMatrix.m42());
    inverseProjectionMatrix = inverseProjectionMatrix.inverse();

    for (size_t i = 0; i < points.size(); ++i) {
        FloatPoint p = inverseProjectionMatrix.mapPoint(points[i]);
        FloatPoint3D viewCoordinate(p.x(), p.y(), plane.findZ(p));
        FloatPoint3D textureCoordinate = viewToTextureTransform.mapPoint(viewCoordinate);
        result.append(FloatPoint(textureCoordinate.x(), textureCoordinate.y()));
    }

    return result;
}

inline FloatPoint3D multVecMatrix(const TransformationMatrix& matrix, const FloatPoint3D& p, float& w)
{
    FloatPoint3D result(
        matrix.m41() + p.x() * matrix.m11() + p.y() * matrix.m21() + p.z() * matrix.m31(),
        matrix.m42() + p.x() * matrix.m12() + p.y() * matrix.m22() + p.z() * matrix.m32(),
        matrix.m43() + p.x() * matrix.m13() + p.y() * matrix.m23() + p.z() * matrix.m33());
    w = matrix.m44() + p.x() * matrix.m14() + p.y() * matrix.m24() + p.z() * matrix.m34();
    return result;
}

template<typename Point, size_t inlineCapacity>
inline Vector<Point, inlineCapacity> toVector(const FloatQuad& quad)
{
    Vector<Point, inlineCapacity> result;
    result.append(quad.p1());
    result.append(quad.p2());
    result.append(quad.p3());
    result.append(quad.p4());
    return result;
}

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)

#endif // LayerUtilities_h
