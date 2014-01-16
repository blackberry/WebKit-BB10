/*
 * Copyright (C) 2012, 2013 Research In Motion Limited. All rights reserved.
 */

#include "config.h"

#include "ZoomViewportAccessor.h"

#include "WebPageCompositor_p.h"
#include "WebPage_p.h"

#include <BlackBerryPlatformMessageClient.h>
#include <BlackBerryPlatformPrimitives.h>
#include <BlackBerryPlatformSettings.h>

using BlackBerry::Platform::IntPoint;
using BlackBerry::Platform::IntSize;
using BlackBerry::Platform::ViewportAccessor;

namespace BlackBerry {
namespace WebKit {

ZoomViewportAccessor::ZoomViewportAccessor(WebPagePrivate* webPagePrivate)
    : WebKitThreadViewportAccessor(webPagePrivate)
{
}

double ZoomViewportAccessor::scale() const
{
    ASSERT(Platform::webKitThreadMessageClient()->isCurrentThread());
    return m_webPagePrivate->currentScale() * m_webPagePrivate->zoomFactor();
}

IntSize ZoomViewportAccessor::pixelViewportSize() const
{
    ASSERT(Platform::webKitThreadMessageClient()->isCurrentThread());

    double zoomFactor = m_webPagePrivate->zoomFactor();

    if (zoomFactor != 1.0) {
        // Round down to avoid showing partially rendered pixels.
        IntSize size = m_webPagePrivate->transformedActualVisibleSize();
        return IntSize(
            floorf(size.width() * zoomFactor),
            floorf(size.height() * zoomFactor));
    }

    return m_webPagePrivate->transformedActualVisibleSize();
}

IntSize ZoomViewportAccessor::preferredSize() const
{
    return m_webPagePrivate->m_defaultLayoutSize;
}

} // namespace WebKit
} // namespace BlackBerry
