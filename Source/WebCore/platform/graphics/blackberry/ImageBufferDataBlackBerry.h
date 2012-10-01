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

#ifndef ImageBufferData_h
#define ImageBufferData_h

#include <BlackBerryPlatformGuardedPointer.h>
#include <BlackBerryPlatformGraphics.h>
#include "CanvasLayerWebKitThread.h"
#include "GraphicsContext.h"
#include "wtf/Uint8ClampedArray.h"

namespace WebCore {

class GraphicsContext;
class IntRect;

class ImageBufferData {
public:
    void getImageData(const IntRect& rect, Uint8ClampedArray* result, bool unmultiply);
    void draw(GraphicsContext* context, ColorSpace styleColorSpace, const FloatRect& destRect, const FloatRect& srcRect,
              CompositeOperator op, bool useLowQualityScale);

    BlackBerry::Platform::Graphics::Buffer* m_buffer;
    GraphicsContext* m_context;
    RefPtr<CanvasLayerWebKitThread> m_platformLayer;
};

}  // namespace WebCore

#endif  // ImageBufferData_h
