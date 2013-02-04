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

#include <BlackBerryPlatformGLES2Program.h>

#if USE(SKIA)
#include "SharedGraphicsContext3D.h"
#include <GLES2/gl2.h>
#include <SkGpuDevice.h>
#endif

using BlackBerry::Platform::Graphics::GLES2Program;

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
    CanvasLayerCompositingThreadClient(BlackBerry::Platform::Graphics::Buffer*);

    void layerCompositingThreadDestroyed(LayerCompositingThread*) { }
    void layerVisibilityChanged(LayerCompositingThread*, bool) { }
    void uploadTexturesIfNeeded(LayerCompositingThread*) { }

    void drawTextures(LayerCompositingThread*, double scale, const GLES2Program&);
    void deleteTextures(LayerCompositingThread*);

    void commitPendingTextureUploads(LayerCompositingThread*);

    void clearBuffer() { m_buffer = 0; }

private:
    BlackBerry::Platform::Graphics::Buffer* m_buffer;
};

CanvasLayerCompositingThreadClient::CanvasLayerCompositingThreadClient(BlackBerry::Platform::Graphics::Buffer* buffer)
    : m_buffer(buffer)
{
}

void CanvasLayerCompositingThreadClient::drawTextures(LayerCompositingThread* layer, double scale, const GLES2Program&)
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
    // Nothing to do here, the buffer is not owned by us.
}

void CanvasLayerCompositingThreadClient::commitPendingTextureUploads(LayerCompositingThread* layer)
{
    if (!m_buffer)
        return;

    // This method is called during a synchronization point between WebKit and compositing thread.
    // This is our only chance to transfer the back display list to the front display list without
    // race conditions.
    // 1. Flush back display list to front
    BlackBerry::Platform::Graphics::releaseBufferDrawable(m_buffer);
    // 2. Draw front display list to FBO
    BlackBerry::Platform::Graphics::updateBufferBackingSurface(m_buffer);
}


CanvasLayerWebKitThread::CanvasLayerWebKitThread(BlackBerry::Platform::Graphics::Buffer* buffer)
    : LayerWebKitThread(CanvasLayer, 0)
{
    m_compositingThreadClient = new CanvasLayerCompositingThreadClient(buffer);
    layerCompositingThread()->setClient(m_compositingThreadClient);
}

CanvasLayerWebKitThread::~CanvasLayerWebKitThread()
{
    layerCompositingThread()->setClient(0);
    delete m_compositingThreadClient;
}

void CanvasLayerWebKitThread::clearBuffer(CanvasLayerWebKitThread* layer)
{
    layer->m_compositingThreadClient->clearBuffer();
}

#endif


}

#endif // USE(ACCELERATED_COMPOSITING) && ENABLE(ACCELERATED_2D_CANVAS)
