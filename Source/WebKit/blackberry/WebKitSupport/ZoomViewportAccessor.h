/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 */

#ifndef ZoomViewportAccessor_h
#define ZoomViewportAccessor_h

#include "WebKitThreadViewportAccessor.h"

#include <BlackBerryPlatformPrimitives.h>

namespace BlackBerry {
namespace WebKit {

// NOTE: Don't use pixel coordinates when computing zoom-related information.
class ZoomViewportAccessor : private WebKitThreadViewportAccessor {
public:
    ZoomViewportAccessor(WebPagePrivate*);
    virtual ~ZoomViewportAccessor() { }

    // Ideal zoom can be computed as ratio between a size in document coordinates and the preferredSize.
    // Note that except for Cascades, the preferred size is equal to the pixel viewport size.
    virtual Platform::IntSize preferredSize() const;

    virtual double scale() const;

    virtual Platform::IntSize documentContentsSize() const { return WebKitThreadViewportAccessor::documentContentsSize(); }
    Platform::IntRect documentContentsRect() const { return WebKitThreadViewportAccessor::documentContentsRect(); }
    virtual Platform::IntPoint documentScrollPosition() const { return WebKitThreadViewportAccessor::documentScrollPosition(); }
    virtual Platform::IntSize documentViewportSize() const { return WebKitThreadViewportAccessor::documentViewportSize(); }
    Platform::IntRect documentViewportRect() const { return WebKitThreadViewportAccessor::documentViewportRect(); }

    Platform::IntPoint roundedDocumentContents(const Platform::FloatPoint& p) const { return WebKitThreadViewportAccessor::roundedDocumentContents(p); }
    Platform::IntRect roundedDocumentContents(const Platform::FloatRect& r) const { return WebKitThreadViewportAccessor::roundedDocumentContents(r); }

    Platform::IntRect documentViewportFromContents(const Platform::IntRect& r) const { return WebKitThreadViewportAccessor::documentViewportFromContents(r); }
    Platform::IntPoint documentViewportFromContents(const Platform::IntPoint& p) const { return WebKitThreadViewportAccessor::documentViewportFromContents(p); }

    Platform::IntRect documentContentsFromViewport(const Platform::IntRect& r) const { return WebKitThreadViewportAccessor::documentContentsFromViewport(r); }
    Platform::IntPoint documentContentsFromViewport(const Platform::IntPoint& p) const { return WebKitThreadViewportAccessor::documentContentsFromViewport(p); }

private:
    virtual Platform::IntSize pixelViewportSize() const;
};

} // namespace WebKit
} // namespace BlackBerry

#endif
