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
#include "GraphicsContext.h"

#include "AffineTransform.h"
#include "Gradient.h"
#include "KURL.h"
#include "NotImplemented.h"
#include "TransformationMatrix.h"
#include "GraphicsContext3D.h"

#include <BlackBerryPlatformGraphicsContext.h>

#include <wtf/Assertions.h>
#include <wtf/MathExtras.h>
#include <wtf/UnusedParam.h>
#include <wtf/Vector.h>
#include <stdio.h>

namespace WebCore {

class GraphicsContextPlatformPrivate {
public:
    GraphicsContextPlatformPrivate(PlatformGraphicsContext* platformContext)
        : m_platformContext(platformContext)
    {
    }

    PlatformGraphicsContext* m_platformContext;
};

void GraphicsContext::platformInit(PlatformGraphicsContext* platformContext)
{
    m_data = new GraphicsContextPlatformPrivate(platformContext);
}

void GraphicsContext::platformDestroy()
{
    delete m_data;
}

PlatformGraphicsContext* GraphicsContext::platformContext() const
{
    return m_data->m_platformContext;
}

void GraphicsContext::savePlatformState()
{
    platformContext()->save();
}

void GraphicsContext::restorePlatformState()
{
    platformContext()->restore();
    platformContext()->setFillColor(m_state.fillColor.rgb());
    platformContext()->setShadow(m_state.shadowOffset, m_state.shadowBlur, m_state.shadowColor.rgb());
}

bool GraphicsContext::isAcceleratedContext() const
{
    return true;
}

AffineTransform GraphicsContext::getCTM(IncludeDeviceScale) const
{
    AffineTransform result;
    platformContext()->getTransform((double*)&result);
    return result;
}

void GraphicsContext::concatCTM(const AffineTransform& transformation)
{
    AffineTransform current;
    platformContext()->getTransform((double*)&current);
    current *= transformation;
    platformContext()->setTransform((double*)&current);
}

void GraphicsContext::setCTM(const AffineTransform& transformation)
{
    platformContext()->setTransform((const double*)&transformation);
}

void GraphicsContext::scale(const FloatSize& scaleFactors)
{
    AffineTransform current;
    platformContext()->getTransform((double*)&current);
    current.scaleNonUniform(scaleFactors.width(), scaleFactors.height());
    platformContext()->setTransform((double*)&current);
}

void GraphicsContext::rotate(float radians)
{
    AffineTransform current;
    platformContext()->getTransform((double*)&current);
    current.rotate(rad2deg(radians));
    platformContext()->setTransform((double*)&current);
}

void GraphicsContext::translate(float dx, float dy)
{
    AffineTransform current;
    platformContext()->getTransform((double*)&current);
    current.translate(dx, dy);
    platformContext()->setTransform((double*)&current);
}

void GraphicsContext::drawEllipse(const IntRect& rect)
{
    platformContext()->addEllipse(FloatRect(rect));
}

void GraphicsContext::strokeArc(const IntRect& rect, int startAngle, int angleSpan)
{
    platformContext()->addArc(rect, startAngle, angleSpan);
}

void GraphicsContext::drawConvexPolygon(size_t numPoints, const FloatPoint* points, bool shouldAntialias)
{
    platformContext()->addConvexPolygon(numPoints, (const BlackBerry::Platform::FloatPoint*)points);
}

void GraphicsContext::drawRect(const IntRect& rect)
{
    platformContext()->addDrawRect(FloatRect(rect));
}

void GraphicsContext::fillRect(const FloatRect& rect)
{
    if (fillGradient())
        fillGradient()->fill(this, rect);
    else
        platformContext()->addFillRect(rect);
}

void GraphicsContext::fillRect(const FloatRect& rect, const Color& color, ColorSpace colorSpace)
{
    platformContext()->setFillColor(color.rgb());
    platformContext()->addFillRect(rect);
    platformContext()->setFillColor(m_state.fillColor.rgb());
}

void GraphicsContext::clearRect(const FloatRect& rect)
{
    platformContext()->addClearRect(rect);
}

void GraphicsContext::strokeRect(const FloatRect& rect, float lineWidth)
{
    platformContext()->addStrokeRect(rect, lineWidth);
}

void GraphicsContext::fillRoundedRect(const IntRect& rect, const IntSize& topLeft, const IntSize& topRight, const IntSize& bottomLeft, const IntSize& bottomRight, const Color& color, ColorSpace colorSpace)
{
    BlackBerry::Platform::FloatRoundedRect r = BlackBerry::Platform::FloatRoundedRect(FloatRect(rect), FloatSize(topLeft), FloatSize(topRight), FloatSize(bottomLeft), FloatSize(bottomRight));
    platformContext()->setFillColor(color.rgb());
    platformContext()->addRoundedRect(r);
    platformContext()->setFillColor(m_state.fillColor.rgb());
}

FloatRect GraphicsContext::roundToDevicePixels(const FloatRect& rect, RoundingMode roundingMode)
{
    return rect;
}

void GraphicsContext::setPlatformShadow(const FloatSize& offset, float blur, const Color& color, ColorSpace colorSpace)
{
    platformContext()->setShadow(offset, blur, color.rgb());
}

void GraphicsContext::clearPlatformShadow()
{
    platformContext()->clearShadow();
}

void GraphicsContext::beginPlatformTransparencyLayer(float opacity)
{
    platformContext()->save();
    // This is not right since we're blending individual draw calls with the background
    // rather than blending the full result all at once. Better than not doing at all though
    platformContext()->setAlpha(opacity);
}

void GraphicsContext::endPlatformTransparencyLayer()
{
    platformContext()->restore();
}

bool GraphicsContext::supportsTransparencyLayers()
{
    return true;
}

void GraphicsContext::setLineCap(LineCap lc)
{
    platformContext()->setLineCap((BlackBerry::Platform::Graphics::LineCap)lc);
}

void GraphicsContext::setLineDash(const DashArray& dashes, float dashOffset)
{
    platformContext()->setLineDash(dashes.data(), dashes.size(), dashOffset);
}

void GraphicsContext::setLineJoin(LineJoin lj)
{
    platformContext()->setLineJoin((BlackBerry::Platform::Graphics::LineJoin)lj); //naming coinsides
}

void GraphicsContext::setMiterLimit(float limit)
{
    platformContext()->setMiterLimit(limit);
}

void GraphicsContext::setAlpha(float opacity)
{
    platformContext()->setAlpha(opacity);
}


void GraphicsContext::clip(const FloatRect& rect)
{
    platformContext()->clip(rect);
}

void GraphicsContext::clipOut(const IntRect& rect)
{
    platformContext()->clipOut(FloatRect(rect));
}

void GraphicsContext::clipConvexPolygon(size_t numPoints, const FloatPoint* points, bool antialias)
{
    platformContext()->clipConvexPolygon(numPoints, (const BlackBerry::Platform::FloatPoint*)points);
}

void GraphicsContext::addRoundedRectClip(const RoundedRect& rect)
{
    if (paintingDisabled())
        return;

    BlackBerry::Platform::FloatRoundedRect r = BlackBerry::Platform::FloatRoundedRect(FloatRect(rect.rect()),
                                                                                      FloatSize(rect.radii().topLeft()),
                                                                                      FloatSize(rect.radii().topRight()),
                                                                                      FloatSize(rect.radii().bottomLeft()),
                                                                                      FloatSize(rect.radii().bottomRight()));
    platformContext()->clipRoundedRect(r);
}

void GraphicsContext::clipOutRoundedRect(const RoundedRect& rect)
{
    if (paintingDisabled())
        return;

    BlackBerry::Platform::FloatRoundedRect r = BlackBerry::Platform::FloatRoundedRect(FloatRect(rect.rect()),
                                                                                      FloatSize(rect.radii().topLeft()),
                                                                                      FloatSize(rect.radii().topRight()),
                                                                                      FloatSize(rect.radii().bottomLeft()),
                                                                                      FloatSize(rect.radii().bottomRight()));
    platformContext()->clipOutRoundedRect(r);
}


void GraphicsContext::addInnerRoundedRectClip(const IntRect& rect, int thickness)
{
    fprintf(stderr, "clip rounded rect %d,%d,%d,%d - %d\n", rect.x(), rect.y(), rect.width(), rect.height(), thickness);
}

void GraphicsContext::setURLForRect(const KURL& link, const IntRect& destRect)
{
}

void GraphicsContext::setPlatformStrokeColor(const Color& color, ColorSpace colorSpace)
{
    platformContext()->setStrokeColor(color.rgb());
}

void GraphicsContext::setPlatformStrokeStyle(StrokeStyle stroke)
{
    platformContext()->setStrokeStyle((BlackBerry::Platform::Graphics::StrokeStyle)stroke);
}

void GraphicsContext::setPlatformStrokeThickness(float thickness)
{
    platformContext()->setStrokeThickness(thickness);
}

void GraphicsContext::setPlatformFillColor(const Color& color, ColorSpace colorSpace)
{
    platformContext()->setFillColor(color.rgb());
}

void GraphicsContext::setPlatformCompositeOperation(CompositeOperator op)
{
    platformContext()->setCompositeOperation(static_cast<BlackBerry::Platform::Graphics::CompositeOperator>(op));
}

void GraphicsContext::setPlatformShouldAntialias(bool enable)
{
    platformContext()->setUseAntialiasing(enable);
}

void GraphicsContext::setImageInterpolationQuality(InterpolationQuality)
{
}

InterpolationQuality GraphicsContext::imageInterpolationQuality() const
{
    return InterpolationDefault;
}

bool GraphicsContext3D::getImageData(Image* image,
                                     GC3Denum format,
                                     GC3Denum type,
                                     bool premultiplyAlpha,
                                     bool ignoreGammaAndColorProfile,
                                     Vector<uint8_t>& outputVector)
{
    return false;
}

}
