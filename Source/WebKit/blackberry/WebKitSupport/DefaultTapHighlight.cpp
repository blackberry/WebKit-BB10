/*
 * Copyright (C) 2012, 2013 Research In Motion Limited. All rights reserved.
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

#include "DefaultTapHighlight.h"

#include "GraphicsContext.h"
#include "GraphicsLayer.h"
#include "LayerWebKitThread.h"
#include "Path.h"
#include "WebAnimation.h"
#include "WebAnimation_p.h"
#include "WebKitThreadViewportAccessor.h"
#include "WebPage_p.h"

#include <BlackBerryPlatformGraphicsContext.h>
#include <BlackBerryPlatformMessageClient.h>
#include <BlackBerryPlatformPath.h>
#include <BlackBerryPlatformSettings.h>
#include <BlackBerryPlatformViewportAccessor.h>

using namespace WebCore;

namespace BlackBerry {
namespace WebKit {

const double ActiveTextFadeAnimationDuration = 0.3;
const double OverlayShrinkAnimationDuration = 0.5;
const double OverlayInitialScale = 2.0;

DEFINE_STATIC_LOCAL(String, s_fadeAnimationName, (ASCIILiteral("fade")));
DEFINE_STATIC_LOCAL(String, s_shrinkAnimationName, (ASCIILiteral("shrink")));

DefaultTapHighlight::DefaultTapHighlight(WebPagePrivate* page)
    : m_page(page)
    , m_visible(false)
    , m_shouldHideAfterScroll(false)
{
}

DefaultTapHighlight::~DefaultTapHighlight()
{
}

void DefaultTapHighlight::draw(const Platform::IntRectRegion& region, int red, int green, int blue, int alpha, bool hideAfterScroll, bool isStartOfSelection)
{
    ASSERT(BlackBerry::Platform::webKitThreadMessageClient()->isCurrentThread());

    m_region = region;
    m_color = Color(red, green, blue, std::min(128, alpha));
    m_shouldHideAfterScroll = hideAfterScroll;
    double overlayScale = OverlayInitialScale;
    FloatRect rect = IntRect(m_region.extents());
    if (rect.isEmpty())
        return;

    // Transparent color means disable the tap highlight.
    if (!m_color.alpha()) {
        hide();
        return;
    }

    {
        MutexLocker lock(m_mutex);
        m_visible = true;
    }

    if (!m_overlay) {
        m_overlay = GraphicsLayer::create(m_page->graphicsLayerFactory(), this);
        m_page->overlayLayer()->platformLayer()->addOverlay(m_overlay->platformLayer());
    }

    m_overlay->removeAnimation(s_shrinkAnimationName);
    m_overlay->setPosition(rect.location());
    m_overlay->setSize(rect.size());
    m_overlay->setDrawsContent(true);
    m_overlay->removeAnimation(s_fadeAnimationName);
    m_overlay->setOpacity(1.0);
    m_overlay->setNeedsDisplay();

    // Get current text selections pixels from document numbers.
    const Platform::ViewportAccessor* viewportAccessor = m_page->m_webPage->webkitThreadViewportAccessor();
    IntRect textRect = viewportAccessor->roundToPixelFromDocumentContents(rect);

    IntSize thumbSize = BlackBerry::Platform::Settings::instance()->minSelectionThumbSize();

    // Check if text is too small to see rectangle highlight animation, if it is, make Animation overlay's Initial scale bigger.
    if (textRect.width() < thumbSize.width() || textRect.height() < thumbSize.height())
        overlayScale = max(((double)thumbSize.width() * 2) / textRect.width(), ((double)thumbSize.height() * 2) / textRect.height());

    // Animate overlay scale to indicate selection is started.
    if (isStartOfSelection) {
        WebAnimation shrinkAnimation = WebAnimation::shrinkAnimation(BlackBerry::Platform::String::emptyString(), overlayScale, 1, OverlayShrinkAnimationDuration);
        m_overlay->addAnimation(shrinkAnimation.d->keyframes, flooredIntSize(m_overlay->size()), shrinkAnimation.d->animation.get(), s_shrinkAnimationName, 0);
    }
}

void DefaultTapHighlight::hide()
{
    if (!m_overlay)
        return;

    {
        MutexLocker lock(m_mutex);
        if (!m_visible)
            return;
        m_visible = false;
    }

    // This method has to be called on the WebKit thread.
    ASSERT(BlackBerry::Platform::webKitThreadMessageClient()->isCurrentThread());
    if (!BlackBerry::Platform::webKitThreadMessageClient()->isCurrentThread())
        return;

    WebAnimation fadeAnimation = WebAnimation::fadeAnimation(BlackBerry::Platform::String::emptyString(), 1.0, 0.0, ActiveTextFadeAnimationDuration);
    m_overlay->addAnimation(fadeAnimation.d->keyframes, flooredIntSize(m_overlay->size()), fadeAnimation.d->animation.get(), s_fadeAnimationName, 0);
}

void DefaultTapHighlight::notifyFlushRequired(const GraphicsLayer* layer)
{
    m_page->notifyFlushRequired(layer);
}

void DefaultTapHighlight::paintContents(const GraphicsLayer*, GraphicsContext& c, GraphicsLayerPaintingPhase, const IntRect& /*inClip*/)
{
    if (!m_region.numRects())
        return;

    Path path(m_region.boundaryPath());

    c.save();
    const Platform::IntRect& rect = m_region.extents();
    c.translate(-rect.x(), -rect.y());

    // Draw tap highlight
    c.setFillColor(m_color, ColorSpaceDeviceRGB);
    c.fillPath(path);
    Color darker = Color(m_color.red(), m_color.green(), m_color.blue()); // Get rid of alpha.
    c.setStrokeColor(darker, ColorSpaceDeviceRGB);
    c.setStrokeThickness(1);
    c.strokePath(path);
    c.restore();
}

} // namespace WebKit
} // namespace BlackBerry

#endif // USE(ACCELERATED_COMPOSITING)
