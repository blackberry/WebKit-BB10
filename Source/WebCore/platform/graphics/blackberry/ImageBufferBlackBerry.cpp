/*
 * Copyright (C) 2009, 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
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
#include "ImageBuffer.h"

#include "Base64.h"
#include "BitmapImage.h"
#include "ImageData.h"
#include "JPEGImageEncoder.h"
#include "LayerMessage.h"
#include "PNGImageEncoder.h"
#include <wtf/text/WTFString.h>

#include <BlackBerryPlatformGraphics.h>
#include <BlackBerryPlatformGraphicsContext.h>

using namespace std;

namespace WebCore {

static void getImageDataInternal(ImageBufferData* object, const IntRect& rect, Uint8ClampedArray* result, bool unmultiply)
{
    object->m_context->platformContext()->readPixels(rect, result->data(), unmultiply);
}

void ImageBufferData::getImageData(const IntRect& rect, Uint8ClampedArray* result, bool unmultiply)
{
    if (!isCompositingThread()) {
        // Use createFunctionCallMessage instead of createMethodCallMessage to avoid deadlock
        // with Guarded pointers.
        dispatchSyncCompositingMessage(BlackBerry::Platform::createFunctionCallMessage(
            &getImageDataInternal, this, rect, result, unmultiply));
        return;
    }

    getImageDataInternal(this, rect, result, unmultiply);
}

static void flushAndDraw(ImageBufferData* object, GraphicsContext* context, ColorSpace styleColorSpace, const FloatRect& destRect, const FloatRect& srcRect,
                         CompositeOperator op, bool useLowQualityScale)
{
    context->platformContext()->flushAndDrawBuffer(object->m_buffer, destRect, srcRect);
}

void ImageBufferData::draw(GraphicsContext* context, ColorSpace styleColorSpace, const FloatRect& destRect, const FloatRect& srcRect,
                           CompositeOperator op, bool useLowQualityScale)
{
    if (m_context->platformContext()->isEmpty()) {
        context->platformContext()->drawBuffer(m_buffer, destRect, srcRect);
    }
    else {
        dispatchSyncCompositingMessage(BlackBerry::Platform::createFunctionCallMessage(
            &flushAndDraw, this, context, styleColorSpace, destRect, srcRect, op, useLowQualityScale));
    }
}

ImageBuffer::ImageBuffer(const IntSize& size, float resolutionScale, ColorSpace colorSpace, RenderingMode renderingMode, DeferralMode deferralMode, bool& success)
    : m_size(size)
    , m_logicalSize(size)
    , m_resolutionScale(1)
{
    m_data.m_platformLayer = CanvasLayerWebKitThread::create(m_size);
    m_data.m_buffer = m_data.m_platformLayer->buffer();
    m_data.m_context = new GraphicsContext(lockBufferDrawable(m_data.m_buffer));
    success = true;
}

ImageBuffer::~ImageBuffer()
{
}

GraphicsContext* ImageBuffer::context() const
{
    return m_data.m_context;
}

PlatformLayer* ImageBuffer::platformLayer() const
{
    return m_data.m_platformLayer.get();
}

#if 0
size_t ImageBuffer::dataSize() const
{
    return m_size.width() * m_size.height() * 4;
}
#endif

PassRefPtr<Image> ImageBuffer::copyImage(BackingStoreCopy copyBehavior) const
{
    return 0;
}

void ImageBuffer::clip(GraphicsContext* context, const FloatRect& rect) const
{
}

void ImageBuffer::draw(GraphicsContext* context, ColorSpace styleColorSpace, const FloatRect& destRect, const FloatRect& srcRect,
                       CompositeOperator op, bool useLowQualityScale)
{
    m_data.draw(context, styleColorSpace, destRect, srcRect, op, useLowQualityScale);
}

void ImageBuffer::drawPattern(GraphicsContext* context, const FloatRect& srcRect, const AffineTransform& patternTransform,
                              const FloatPoint& phase, ColorSpace styleColorSpace, CompositeOperator op, const FloatRect& destRect)
{
}

void ImageBuffer::platformTransformColorSpace(const Vector<int>& lookUpTable)
{
}

PassRefPtr<Uint8ClampedArray> ImageBuffer::getUnmultipliedImageData(const IntRect& rect, CoordinateSystem) const
{
    RefPtr<Uint8ClampedArray> result = Uint8ClampedArray::createUninitialized(rect.width() * rect.height() * 4);
    const_cast<ImageBufferData&>(m_data).getImageData(rect, result.get(), true);
    return result;
}

PassRefPtr<Uint8ClampedArray> ImageBuffer::getPremultipliedImageData(const IntRect& rect, CoordinateSystem) const
{
    RefPtr<Uint8ClampedArray> result = Uint8ClampedArray::createUninitialized(rect.width() * rect.height() * 4);
    const_cast<ImageBufferData&>(m_data).getImageData(rect, result.get(), false);
    return result;
}

void ImageBuffer::putByteArray(Multiply multiplied, Uint8ClampedArray* source, const IntSize& sourceSize, const IntRect& sourceRect, const IntPoint& destPoint, CoordinateSystem)
{
    BlackBerry::Platform::Graphics::TiledImage image(sourceSize, (unsigned*)source->data(), false /* this data is RGBA, not BGRA */);
    m_data.m_context->platformContext()->addImage(FloatRect(destPoint, sourceRect.size()), FloatRect(sourceRect), &image);
}

String ImageBuffer::toDataURL(const String& mimeType, const double* quality, CoordinateSystem) const
{
    if (m_size.isEmpty())
        return "data:,";

    enum {
        EncodeJPEG,
        EncodePNG,
    } encodeType = mimeType.lower() == "image/png" ? EncodePNG : EncodeJPEG;

    // According to http://www.w3.org/TR/html5/the-canvas-element.html,
    // "For image types that do not support an alpha channel, the image must be"
    // "composited onto a solid black background using the source-over operator,"
    // "and the resulting image must be the one used to create the data: URL."
    // JPEG doesn't have alpha channel, so we need premultiplied data.
    RefPtr<Uint8ClampedArray> imageData = encodeType == EncodePNG
        ? getUnmultipliedImageData(IntRect(IntPoint(0, 0), m_size))
        : getPremultipliedImageData(IntRect(IntPoint(0, 0), m_size));

    Vector<char> output;
    const char* header;
    if (encodeType == EncodePNG) {
        if (!compressRGBABigEndianToPNG(imageData->data(), m_size, output))
            return "data:,";
        header = "data:image/png;base64,";
    } else {
        if (!compressRGBABigEndianToJPEG(imageData->data(), m_size, output))
            return "data:,";
        header = "data:image/jpeg;base64,";
    }

    Vector<char> base64;
    base64Encode(output, base64);

    output.clear();

    Vector<char> url;
    url.append(header, strlen(header));
    url.append(base64);

    return String(url.data(), url.size());
}

} // namespace WebCore
