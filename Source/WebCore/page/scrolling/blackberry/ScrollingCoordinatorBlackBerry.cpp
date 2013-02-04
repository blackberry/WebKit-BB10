/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "ScrollingCoordinator.h"

#include "Frame.h"
#include "FrameView.h"
#include "LayerWebKitThread.h"
#include "PlatformLayer.h"
#include "RenderLayerCompositor.h"
#include "RenderView.h"

namespace WebCore {

class ScrollingCoordinatorPrivate {
};

#if !ENABLE(THREADED_SCROLLING)
PassRefPtr<ScrollingCoordinator> ScrollingCoordinator::create(Page* page)
{
    return adoptRef(new ScrollingCoordinator(page));
}

ScrollingCoordinator::~ScrollingCoordinator()
{
    ASSERT(!m_page);
}

void ScrollingCoordinator::frameViewHorizontalScrollbarLayerDidChange(FrameView*, GraphicsLayer*)
{
}

void ScrollingCoordinator::frameViewVerticalScrollbarLayerDidChange(FrameView*, GraphicsLayer*)
{
}

void ScrollingCoordinator::setScrollLayer(GraphicsLayer*)
{
}

void ScrollingCoordinator::setNonFastScrollableRegion(const Region&)
{
}

void ScrollingCoordinator::setScrollParameters(const ScrollParameters&)
{
}

void ScrollingCoordinator::setWheelEventHandlerCount(unsigned)
{
}

bool ScrollingCoordinator::supportsFixedPositionLayers() const
{
    return true;
}

void ScrollingCoordinator::setLayerIsContainerForFixedPositionLayers(GraphicsLayer* layer, bool b)
{
    if (layer->platformLayer())
        layer->platformLayer()->setIsContainerForFixedPositionLayers(b);
}

void ScrollingCoordinator::setLayerIsFixedToContainerLayer(GraphicsLayer* layer, bool b)
{
    if (layer->platformLayer())
        layer->platformLayer()->setFixedPosition(b);
}

static GraphicsLayer* scrollLayerForFrameView(FrameView* frameView)
{
#if USE(ACCELERATED_COMPOSITING)
    Frame* frame = frameView->frame();
    if (!frame)
        return 0;

    RenderView* renderView = frame->contentRenderer();
    if (!renderView)
        return 0;
    return renderView->compositor()->scrollLayer();
#else
    return 0;
#endif
}

void ScrollingCoordinator::frameViewFrameRectDidChange(FrameView* view)
{
    if (GraphicsLayer* scrollLayer = scrollLayerForFrameView(view))
        scrollLayer->platformLayer()->setFrameVisibleRect(view->visibleContentRect());
}

void ScrollingCoordinator::frameViewContentsSizeDidChange(FrameView* view)
{
    if (GraphicsLayer* scrollLayer = scrollLayerForFrameView(view))
        scrollLayer->platformLayer()->setFrameContentsSize(view->contentsSize());
}

void ScrollingCoordinator::scrollableAreaScrollLayerDidChange(ScrollableArea*, GraphicsLayer*)
{
}
#endif // !ENABLE(THREADED_SCROLLING)

}
