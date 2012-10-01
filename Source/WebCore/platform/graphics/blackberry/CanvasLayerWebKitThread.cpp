/*
 * Copyright (C) 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
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
#include "CanvasLayerWebKitThread.h"
#include "LayerCompositingThread.h"

#if USE(ACCELERATED_COMPOSITING) && ENABLE(ACCELERATED_2D_CANVAS)

#if USE(SKIA)
#include "SharedGraphicsContext3D.h"
#include <GLES2/gl2.h>
#include <SkGpuDevice.h>
#endif

namespace WebCore {

void CanvasLayerWebKitThread::deleteTextures()
{
#if USE(SKIA)
    if (SharedGraphicsContext3D::get()->makeContextCurrent())
        deleteFrontBuffer();
#endif
}

#if USE(SKIA)

CanvasLayerWebKitThread::CanvasLayerWebKitThread(SkGpuDevice* device)
    : EGLImageLayerWebKitThread(CanvasLayer)
{
    setDevice(device);
}

CanvasLayerWebKitThread::~CanvasLayerWebKitThread()
{
    if (SharedGraphicsContext3D::get()->makeContextCurrent())
        deleteFrontBuffer();
}

void CanvasLayerWebKitThread::setDevice(SkGpuDevice* device)
{
    m_device = device;
    setNeedsDisplay();
}

void CanvasLayerWebKitThread::updateTextureContentsIfNeeded()
{
    if (!m_device)
        return;

    if (!SharedGraphicsContext3D::get()->makeContextCurrent())
        return;

    GrRenderTarget* renderTarget = reinterpret_cast<GrRenderTarget*>(m_device->accessRenderTarget());
    if (!renderTarget)
        return;

    GrTexture* texture = renderTarget->asTexture();
    if (!texture)
        return;

    updateFrontBuffer(IntSize(m_device->width(), m_device->height()), texture->getTextureHandle());
}

#else

class CanvasLayerCompositingThreadClient : public LayerCompositingThreadClient {
public:
    CanvasLayerCompositingThreadClient(const IntSize&);

    void layerCompositingThreadDestroyed(LayerCompositingThread*) { }
    void layerVisibilityChanged(LayerCompositingThread*, bool) { }
    void uploadTexturesIfNeeded(LayerCompositingThread*) { }

    void drawTextures(LayerCompositingThread*, double scale, int positionLocation, int texCoordLocation);
    void deleteTextures(LayerCompositingThread*);

    BlackBerry::Platform::Graphics::Buffer* buffer() const { return m_buffer; }
private:
    BlackBerry::Platform::Graphics::Buffer* m_buffer;
};

CanvasLayerCompositingThreadClient::CanvasLayerCompositingThreadClient(const IntSize& size)
{
    m_buffer = createBuffer(size, BlackBerry::Platform::Graphics::TileBuffer);
}

void CanvasLayerCompositingThreadClient::drawTextures(LayerCompositingThread* layer, double scale, int positionLocation, int texCoordLocation)
{
    if (!m_buffer)
        return;

    TransformationMatrix dt = layer->drawTransform();
    dt.translate(-layer->bounds().width() / 2.0, -layer->bounds().height() / 2.0);
    blitToBuffer(0, m_buffer, reinterpret_cast<BlackBerry::Platform::TransformationMatrix&>(dt),
                 BlackBerry::Platform::Graphics::SourceOver, static_cast<unsigned char>(layer->drawOpacity() * 255));
}

void CanvasLayerCompositingThreadClient::deleteTextures(LayerCompositingThread* layer)
{
    destroyBuffer(m_buffer);
    m_buffer = 0;
}



CanvasLayerWebKitThread::CanvasLayerWebKitThread(const IntSize& size)
    : EGLImageLayerWebKitThread(CanvasLayer)
{
    m_compositingThreadClient = new CanvasLayerCompositingThreadClient(size);
    layerCompositingThread()->setClient(m_compositingThreadClient);
}

CanvasLayerWebKitThread::~CanvasLayerWebKitThread()
{
}

void CanvasLayerWebKitThread::updateTextureContentsIfNeeded()
{
    if (m_compositingThreadClient->buffer())
        releaseBufferDrawable(m_compositingThreadClient->buffer());
}

BlackBerry::Platform::Graphics::Buffer* CanvasLayerWebKitThread::buffer()
{
    return m_compositingThreadClient->buffer();
}

#endif


}

#endif // USE(ACCELERATED_COMPOSITING) && ENABLE(ACCELERATED_2D_CANVAS)
