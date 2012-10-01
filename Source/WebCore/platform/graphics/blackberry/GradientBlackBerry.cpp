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

// Determine the total number of stops needed, including pseudo-stops at the
// ends as necessary.
static size_t totalStopsNeeded(const Gradient::ColorStop* stopData, size_t count)
{
    // N.B.:  The tests in this function should kept in sync with the ones in
    // fillStops(), or badness happens.
    const Gradient::ColorStop* stop = stopData;
    size_t countUsed = count;
    if (count < 1 || stop->stop > 0.0)
        countUsed++;
    stop += count - 1;
    if (count < 1 || stop->stop < 1.0)
        countUsed++;
    return countUsed;
}

// Collect sorted stop position and color information into the pos and colors
// buffers, ensuring stops at both 0.0 and 1.0.  The buffers must be large
// enough to hold information for all stops, including the new endpoints if
// stops at 0.0 and 1.0 aren't already included.
static void fillStops(const Gradient::ColorStop* stopData, size_t count, float* pos, unsigned* colors)
{
    const Gradient::ColorStop* stop = stopData;
    size_t start = 0;
    if (count < 1) {
        // A gradient with no stops must be transparent black.
        pos[0] = 0.0;
        colors[0] = 0;
        start = 1;
    } else if (stop->stop > 0.0) {
        // Copy the first stop to 0.0. The first stop position may have a slight
        // rounding error, but we don't care in this float comparison, since
        // 0.0 comes through cleanly and people aren't likely to want a gradient
        // with a stop at (0 + epsilon).
        pos[0] = 0.0;
        colors[0] = Color(stop->red, stop->green, stop->blue, stop->alpha).rgb();
        start = 1;
    }

    for (size_t i = start; i < start + count; i++) {
        pos[i] = stop->stop;
        colors[i] = Color(stop->red, stop->green, stop->blue, stop->alpha).rgb();
        ++stop;
    }

    // Copy the last stop to 1.0 if needed.  See comment above about this float
    // comparison.
    if (count < 1 || (--stop)->stop < 1.0) {
        pos[start + count] = 1.0;
        colors[start + count] = colors[start + count - 1];
    }
}


BlackBerry::Platform::Graphics::Gradient* Gradient::platformGradient()
{
    if (m_gradient)
        return m_gradient;

    sortStopsIfNecessary();
    ASSERT(m_stopsSorted);

    size_t countUsed = totalStopsNeeded(m_stops.data(), m_stops.size());
    ASSERT(countUsed >= 2);
    ASSERT(countUsed >= m_stops.size());

    // FIXME: Why is all this manual pointer math needed?!
    Vector<float> pos(countUsed);
    Vector<unsigned> colors(countUsed);

    fillStops(m_stops.data(), m_stops.size(), pos.data(), colors.data());

#if 0
    if (m_radial) {
        // Since the two-point radial gradient is slower than the plain radial,
        // only use it if we have to.
        if (m_p0 == m_p1 && m_r0 <= 0.0f) {
            m_gradient = SkGradientShader::CreateRadial(m_p1, m_r1, colors, pos, static_cast<int>(countUsed), tile);
        } else {
            // The radii we give to Skia must be positive.  If we're given a
            // negative radius, ask for zero instead.
            SkScalar radius0 = m_r0 >= 0.0f ? WebCoreFloatToSkScalar(m_r0) : 0;
            SkScalar radius1 = m_r1 >= 0.0f ? WebCoreFloatToSkScalar(m_r1) : 0;
            m_gradient = SkGradientShader::CreateTwoPointRadial(m_p0, radius0, m_p1, radius1, colors, pos, static_cast<int>(countUsed), tile);
        }

        if (aspectRatio() != 1) {
            // CSS3 elliptical gradients: apply the elliptical scaling at the
            // gradient center point.
            m_gradientSpaceTransformation.translate(m_p0.x(), m_p0.y());
            m_gradientSpaceTransformation.scale(1, 1 / aspectRatio());
            m_gradientSpaceTransformation.translate(-m_p0.x(), -m_p0.y());
            ASSERT(m_p0 == m_p1);
        }
    } else {
#endif

    m_gradient = new BlackBerry::Platform::Graphics::Gradient(m_p0, m_p1, colors.data(), pos.data(), countUsed, (BlackBerry::Platform::Graphics::Gradient::SpreadMethod)m_spreadMethod);

#if 0
    if (!m_gradient)
        // use last color, since our "geometry" was degenerate (e.g. radius==0)
        m_gradient = new SkColorShader(colors[countUsed - 1]);
    else
        m_gradient->setLocalMatrix(m_gradientSpaceTransformation);
#endif

    return m_gradient;
}

void Gradient::platformDestroy()
{
    delete m_gradient;
    m_gradient = 0;
}

void Gradient::fill(GraphicsContext* context, const FloatRect& rect)
{
    sortStopsIfNecessary();
    const float EPSILON = 1.0e-6;

    // Radial Gradient
    if (m_radial) {
        //if (m_p0 == m_p1) // Hopefuly not used.
        if (m_r1 <= m_r0 || m_stops.size() <= 1) // nothing to draw?
            return;

        float r, g, b, a;
        getColor(0.0, &r, &g, &b, &a);
        unsigned c0 = Color(r, g, b, a).rgb();
        float r0 = m_r0;
        float stop0 = 0.0;
        for (unsigned int i = 0; i < m_stops.size()+1; ++i) {
            float stop1 = i < m_stops.size() ? m_stops[i].stop : 1.0;
            if (fabs(stop1 - stop0) <= EPSILON)
                continue;

            float r1 = m_r0 + stop1 * (m_r1 - m_r0);
            getColor(stop1, &r, &g, &b, &a);
            unsigned c1 = Color(r, g, b, a).rgb();

            context->platformContext()->addRadialGradient(rect, m_p0, m_p1, r0, r1, aspectRatio(), c0, c1);
            r0 = r1;
            c0 = c1;
            stop0 = stop1;
        }

        return;
    }

    // Linear Gradient
    //
    // The algorithm works by traversing the stops along the gradient
    // line and intersecting the destination rectangle by the line of
    // constant color at each stop. It then generates triangles that
    // have only two varying colors which the gpu can interpolate easily
    //
    Vector<FloatPoint> v;
    v.append(FloatPoint(rect.x(), rect.y()));
    v.append(FloatPoint(rect.maxX(), rect.y()));
    v.append(FloatPoint(rect.maxX(), rect.maxY()));
    v.append(FloatPoint(rect.x(), rect.maxY()));

    // The gradient line is from m_p0 to m_p1. The line of constant color
    // is perpendicular to this gradient line. The equation of line of
    // constant color can be written as (a,b).(x,y) + c = 0 where (a,b)
    // is the line normal, which we know to be the gradient line.
    float a = m_p1.x() - m_p0.x();
    float b = m_p1.y() - m_p0.y();
    for (unsigned int i = 0; i < m_stops.size(); ++i) {
        // The vertices on the negative side of the constant color line
        // These are all guaranteed to be using only two color and we generated
        // the resulting triangles out of those
        Vector<FloatPoint> current;
        Vector<Color> color;

        // The vertices on the positive side, these will be used in
        // the next iteration of the loop through the stops
        Vector<FloatPoint> next;

        // (a,b) is the gradient line so the current stop is at m_p0 + (a,b)*stop
        // hence from the constant color line equation we get that
        // (a,b).(m_p0 + (a,b)*stop) + c = 0
        float c = -((m_p0.x() + m_stops[i].stop * a) * a + (m_p0.y() + m_stops[i].stop * b) * b);

        for (unsigned int j0 = 0; j0 < v.size(); ++j0) {
            unsigned int j1 = j0+1 == v.size() ? 0 : j0+1;
            float t0 = a*v[j0].x() + b*v[j0].y() + c;
            float t1 = a*v[j1].x() + b*v[j1].y() + c;

            // If the current vertex is on the positive side of the line it is passed on to the
            // next stop. If it is on the negative line, we use it to generate geometry for the
            // current stop. Note that if the vertex is directly on the line it is both used in
            // this stop and passed on to the next.
            if (t0 >= -EPSILON) {
                next.append(v[j0]);
            }
            if (t0 <= EPSILON) {
                // To find out the color value of this vertex we project the vector from v[j0] to m_p0
                // onto the gradient vector (a,b). The projection of A onto B is A.B/|B|. In order to
                // find the color value we need to normalize the projection with respect to the length
                // of the gradient vector so the final value is (v[j0] - m_p0).(a,b)/|(a,b)|^2
                float proj = ((v[j0].x() - m_p0.x()) * a + (v[j0].y() - m_p0.y()) * b) / (a*a + b*b);
                // TODO: different spread methods
                proj = std::max(0.0f, std::min(proj, 1.0f));
                float r, g, b, a;
                getColor(proj, &r, &g, &b, &a);
                color.append(Color(r, g, b, a));
                current.append(v[j0]);
            }

            // If the current and the next vertex lie on different side of the constant color line
            // we calculate the intersection and add that point to both the current and next set
            if ((t0 > EPSILON && t1 < -EPSILON) || (t0 < -EPSILON && t1 > EPSILON)) {
                // The line connecting the two points is v[j0] + t*(v[j1] - v[j0). Plugging that into the constant
                // color line equation we get (a,b).(v[j0] + t*(v[j1] - v[j0])) + c = 0. Solving for t we get that
                // t = -((a,b).v[j0] + c) / ((a,b).(v[j1] - v[j0]))
                float t = -(a*v[j0].x() + b*v[j0].y() + c) / (a*(v[j1].x() - v[j0].x()) + b*(v[j1].y() - v[j0].y()));

                // Compute the intersection point and add it to both sets
                FloatPoint intersection(v[j0].x() + t*(v[j1].x() - v[j0].x()), v[j0].y() + t*(v[j1].y() - v[j0].y()));
                next.append(intersection);
                current.append(intersection);

                // The intersection is exactly at the constant color line with value of the stop
                color.append(Color(m_stops[i].red, m_stops[i].green, m_stops[i].blue, m_stops[i].alpha));
            }
        }

        // Generate all the geometry on this side of the constant color line
        for (unsigned int j = 2; j < current.size(); ++j) {
            context->platformContext()->addTriangle(current[0], color[0].rgb(),
                                                    current[j-1], color[j-1].rgb(),
                                                    current[j], color[j].rgb());
        }

        v.swap(next);
    }

    // Any remaining geometry gets the color of the last stop
    for (unsigned int j = 2; j < v.size(); ++j) {
        float r, g, b, a;
        getColor(1.0, &r, &g, &b, &a);
        Color color(r, g, b, a);
        context->platformContext()->addTriangle(v[0], color.rgb(), v[j-1], color.rgb(), v[j], color.rgb());
    }
}


}
