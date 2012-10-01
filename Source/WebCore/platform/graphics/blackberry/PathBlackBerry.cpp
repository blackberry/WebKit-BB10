/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
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
#include "Path.h"

#include "AffineTransform.h"
#include "FloatRect.h"
#include "GraphicsContext.h"
#include "NotImplemented.h"
#include "PlatformString.h"
#include "StrokeStyleApplier.h"

#include <BlackBerryPlatformGraphics.h>
#include <BlackBerryPlatformGraphicsContext.h>

#include <wtf/MathExtras.h>
#include <stdio.h>

namespace WebCore {

struct BlackBerryPath {
    BlackBerryPath()
        : m_isRoundedRect(true)
        , m_rectCount(0)
    { }

    bool m_isRoundedRect;
    int m_rectCount;
    BlackBerry::Platform::FloatRoundedRect m_rect[2];
    BlackBerry::Platform::Graphics::Path m_path;
};

Path::Path()
{
    m_path = new BlackBerryPath;
}

Path::~Path()
{
    if (m_path) {
        delete m_path;
        m_path = 0;
    }
}

Path::Path(const Path& other)
{
    m_path = new BlackBerryPath(*other.m_path);
}

Path& Path::operator=(const Path& other)
{
    if (m_path) {
        delete m_path;
        m_path = 0;
    }
    m_path = new BlackBerryPath(*other.m_path);
    return *this;
}

FloatPoint Path::currentPoint() const
{
    return FloatPoint(0, 0);
}


bool Path::contains(const FloatPoint& point, WindRule rule) const
{
    notImplemented();
    return boundingRect().contains(point);
}

bool Path::strokeContains(StrokeStyleApplier* applier, const FloatPoint& point) const
{
    notImplemented();
    return (const_cast<Path*>(this))->strokeBoundingRect().contains(point);
}

void Path::translate(const FloatSize& size)
{
    AffineTransform transformation;
    transformation.translate(size.width(), size.height());
    transform(transformation);
}

FloatRect Path::boundingRect() const
{
    if (m_path->m_isRoundedRect && m_path->m_rectCount) {
        return m_path->m_rect[0].rect();
    } else {
        return m_path->m_path.getBounds();
    }
}

static GraphicsContext* scratchContext()
{
    static BlackBerry::Platform::Graphics::Buffer* buffer = createBuffer(IntSize(0,0), BlackBerry::Platform::Graphics::TileBuffer);
    static PlatformGraphicsContext* pgc = lockBufferDrawable(buffer);
    static GraphicsContext gc(pgc);
    return &gc;
}

FloatRect Path::strokeBoundingRect(StrokeStyleApplier* applier) const
{
    GraphicsContext* scratch = scratchContext();
    scratch->save();

    if (applier)
        applier->strokeStyle(scratch);

    FloatRect result;
    if (m_path->m_isRoundedRect && m_path->m_rectCount) {
        result = m_path->m_rect[0].rect();
        result.inflate(scratch->strokeThickness());
    } else {
        result = scratch->platformContext()->getStrokeBounds(m_path->m_path);
    }

    scratch->restore();
    return result;
}

void Path::moveTo(const FloatPoint& point)
{
    m_path->m_path.moveTo(point);
}

void Path::addLineTo(const FloatPoint& point)
{
    m_path->m_isRoundedRect = false;
    m_path->m_path.addLineTo(point);
}

void Path::addQuadCurveTo(const FloatPoint& controlPoint, const FloatPoint& endPoint)
{
    m_path->m_isRoundedRect = false;
    m_path->m_path.addQuadCurveTo(controlPoint, endPoint);
}

void Path::addBezierCurveTo(const FloatPoint& controlPoint1, const FloatPoint& controlPoint2, const FloatPoint& endPoint)
{
    m_path->m_isRoundedRect = false;
    m_path->m_path.addBezierCurveTo(controlPoint1, controlPoint2, endPoint);
}

void Path::addArcTo(const FloatPoint& point1, const FloatPoint& point2, float radius)
{
    m_path->m_isRoundedRect = false;
    m_path->m_path.addArcTo(point1, point2, radius);
}

void Path::closeSubpath()
{
    m_path->m_path.closeSubpath();
}

void Path::addArc(const FloatPoint& center, float radius, float startAngle, float endAngle, bool anticlockwise)
{
    if (m_path->m_isRoundedRect && m_path->m_rectCount < 2 && fabs(fmod(endAngle - startAngle, 2*M_PI)) < 0.01) {
        FloatRect r(center.x() - radius, center.y() - radius, 2*radius, 2*radius);
        FloatSize rad(radius, radius);
        m_path->m_rect[m_path->m_rectCount] = BlackBerry::Platform::FloatRoundedRect(r, rad, rad, rad, rad);
        ++m_path->m_rectCount;
    } else
        m_path->m_isRoundedRect = false;
    m_path->m_path.addArc(center, radius, startAngle, endAngle, anticlockwise);
}

void Path::addRect(const FloatRect& rect)
{
    if (m_path->m_isRoundedRect && m_path->m_rectCount < 2) {
        m_path->m_rect[m_path->m_rectCount] = BlackBerry::Platform::FloatRoundedRect(rect, FloatSize(0,0), FloatSize(0,0), FloatSize(0,0), FloatSize(0,0));
        ++m_path->m_rectCount;
    } else {
        m_path->m_isRoundedRect = false;
    }

    // Make sure the generic path reflects the contents of the rounded rects
    m_path->m_path.addRect(rect);
}

void Path::addEllipse(const FloatRect& rect)
{
    // TODO: this can be a rounded rect
    platformPath()->m_isRoundedRect = false;
    platformPath()->m_path.addEllipse(rect);
}

void Path::platformAddPathForRoundedRect(const FloatRect& rect, const FloatSize& topLeftRadius, const FloatSize& topRightRadius, const FloatSize& bottomLeftRadius, const FloatSize& bottomRightRadius)
{
    if (rect.isEmpty())
        return;

    if (rect.width() < topLeftRadius.width() + topRightRadius.width()
            || rect.width() < bottomLeftRadius.width() + bottomRightRadius.width()
            || rect.height() < topLeftRadius.height() + bottomLeftRadius.height()
            || rect.height() < topRightRadius.height() + bottomRightRadius.height()) {
        // If all the radii cannot be accommodated, return a rect.
        addRect(rect);
        return;
    }

    BlackBerry::Platform::FloatRoundedRect rr(rect, topLeftRadius, topRightRadius, bottomLeftRadius, bottomRightRadius);
    if (m_path->m_isRoundedRect && m_path->m_rectCount < 2) {
        m_path->m_rect[m_path->m_rectCount] = rr;
        ++m_path->m_rectCount;
    } else {
        m_path->m_isRoundedRect = false;
    }

    // Make sure the generic path reflects the contents of the rounded rects
    m_path->m_path.addRoundedRect(rr);
}


void Path::clear()
{
    m_path->m_rectCount = 0;
    m_path->m_isRoundedRect = true;
    platformPath()->m_path.reset();
}

bool Path::isEmpty() const
{
    return m_path->m_path.isEmpty() && m_path->m_rectCount == 0;
}

bool Path::hasCurrentPoint() const
{
    return m_path->m_path.hasCurrentPoint();
}

void Path::apply(void* info, PathApplierFunction function) const
{
    m_path->m_path.apply(info, (void*)function);
}

void Path::transform(const AffineTransform& transformation)
{
    for (int i = 0; i < m_path->m_rectCount; ++i) {
        BlackBerry::Platform::FloatRoundedRect transformed(transformation.mapRect(m_path->m_rect[i].rect()),
                                                           transformation.mapSize(m_path->m_rect[i].radii().topLeft()),
                                                           transformation.mapSize(m_path->m_rect[i].radii().topRight()),
                                                           transformation.mapSize(m_path->m_rect[i].radii().bottomLeft()),
                                                           transformation.mapSize(m_path->m_rect[i].radii().bottomRight()));
        m_path->m_rect[i] = transformed;
    }
    m_path->m_path.transform(reinterpret_cast<const double*>(&transformation));
}

void GraphicsContext::fillPath(const Path& path)
{
    BlackBerryPath* pp = path.platformPath();
    if (pp->m_isRoundedRect && pp->m_rectCount && !fillGradient()) {
        if (pp->m_rectCount == 1)
            platformContext()->addRoundedRect(pp->m_rect[0]);
        else
            platformContext()->addRoundedRectOutline(pp->m_rect[0], pp->m_rect[1]);
    } else if (!pp->m_path.isEmpty()) {
        BlackBerry::Platform::Graphics::Gradient* gradient = fillGradient() ? fillGradient()->platformGradient() : 0;
        platformContext()->addFillPath(pp->m_path, (BlackBerry::Platform::Graphics::WindRule)m_state.fillRule, gradient);
    }
}

static BlackBerry::Platform::FloatRoundedRect inflate(const BlackBerry::Platform::FloatRoundedRect& rr, float delta)
{
    FloatRect r = rr.rect();
    r.inflate(delta);
    FloatSize tl(rr.radii().topLeft().width() + delta, rr.radii().topLeft().height() + delta);
    FloatSize tr(rr.radii().topRight().width() + delta, rr.radii().topRight().height() + delta);
    FloatSize bl(rr.radii().bottomLeft().width() + delta, rr.radii().bottomLeft().height() + delta);
    FloatSize br(rr.radii().bottomRight().width() + delta, rr.radii().bottomRight().height() + delta);
    return BlackBerry::Platform::FloatRoundedRect(r, tl, tr, bl, br);
}

void GraphicsContext::strokePath(const Path& path)
{
    BlackBerryPath* pp = path.platformPath();
    if (pp->m_isRoundedRect && pp->m_rectCount && strokeStyle() == SolidStroke && !strokeGradient()) {
        Color oldFill = fillColor();
        platformContext()->setFillColor(strokeColor().rgb());
        for (int i = 0; i < pp->m_rectCount; ++i) {
            platformContext()->addRoundedRectOutline(inflate(pp->m_rect[i], strokeThickness() / 2.0),
                                                     inflate(pp->m_rect[i], -strokeThickness() / 2.0));
        }
        platformContext()->setFillColor(oldFill.rgb());
    } else if (!pp->m_path.isEmpty()) {
        BlackBerry::Platform::Graphics::Gradient* gradient = strokeGradient() ? strokeGradient()->platformGradient() : 0;
        platformContext()->addStrokePath(pp->m_path, gradient);
    }
}

void GraphicsContext::drawFocusRing(const Vector<IntRect>& rects, int width, int offset, const Color& color)
{
    notImplemented();
}

void GraphicsContext::drawFocusRing(const Path& path, int width, int offset, const Color& color)
{
    notImplemented();
}

void GraphicsContext::drawLine(const IntPoint& from, const IntPoint& to)
{
    platformContext()->addDrawLine(from, to);
}

void GraphicsContext::drawLineForDocumentMarker(const FloatPoint& pt, float width, DocumentMarkerLineStyle style)
{
    platformContext()->addDrawLineForDocumentMarker(pt, width, (BlackBerry::Platform::Graphics::DocumentMarkerLineStyle)style);
}

void GraphicsContext::drawLineForText(const FloatPoint& pt, float width, bool printing)
{
    platformContext()->addDrawLineForText(pt, width, printing);
}

void GraphicsContext::clip(const Path& path)
{
    BlackBerryPath* pp = path.platformPath();
    if (pp->m_isRoundedRect && pp->m_rectCount == 1)
        platformContext()->clipRoundedRect(pp->m_rect[0]);
    else if (pp->m_isRoundedRect && pp->m_rectCount == 2) {
        platformContext()->clipRoundedRect(pp->m_rect[0]);
        platformContext()->clipOutRoundedRect(pp->m_rect[1]);
    } else
        fprintf(stderr, "clip path\n");
}

void GraphicsContext::clipPath(const Path& path, WindRule clipRule)
{
    notImplemented();
}

void GraphicsContext::canvasClip(const Path& path)
{
    notImplemented();
}

void GraphicsContext::clipOut(const Path& path)
{
    BlackBerryPath* pp = path.platformPath();
    if (pp->m_isRoundedRect && pp->m_rectCount == 1)
        platformContext()->clipOutRoundedRect(path.platformPath()->m_rect[0]);
    else
        fprintf(stderr, "clip path out\n");
}

}
