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
#include "Image.h"

#include "BitmapImage.h"
#include "ImageBuffer.h"
#include "SharedBuffer.h"

namespace WebCore {

PassRefPtr<Image> Image::loadPlatformResource(const char *name)
{
    // RESOURCE_PATH is set by CMake in OptionsBlackBerry.cmake
    String fullPath(RESOURCE_PATH);
    String extension(".png");

    fullPath += name;
    fullPath += extension;

    RefPtr<SharedBuffer> buffer = SharedBuffer::createWithContentsOfFile(fullPath);
    if (!buffer)
        return BitmapImage::nullImage();

    RefPtr<BitmapImage> img = BitmapImage::create();
    img->setData(buffer.release(), true /* allDataReceived */);
    return img.release();
}

} // namespace WebCore

#if !USE(SKIA)

#include "AffineTransform.h"
#include "FloatConversion.h"
#include "FloatRect.h"
#include "GraphicsContext.h"
#include "ImageDecoder.h"
#include "ImageObserver.h"
#include "IntSize.h"
#include "NotImplemented.h"
#include <BlackBerryPlatformGraphicsContext.h>
#include <wtf/MathExtras.h>

namespace WebCore {

NativeImagePtr ImageFrame::asNewNativeImage() const
{
    return new BlackBerry::Platform::Graphics::TiledImage(m_size, m_bytes);
}

bool FrameData::clear(bool clearMetadata)
{
    if (clearMetadata)
        m_haveMetadata = false;

    if (m_frame) {
        delete m_frame;
        m_frame = 0;
        return true;
    }
    return false;
}

void BitmapImage::checkForSolidColor()
{
    m_isSolidColor = size().width() == 1 && size().height() == 1 && frameCount() == 1;
    m_checkedForSolidColor = true;
}

void BitmapImage::invalidatePlatformData()
{
}

void BitmapImage::draw(GraphicsContext* context, const FloatRect& dst, const FloatRect& src, ColorSpace styleColorSpace, CompositeOperator op)
{
    startAnimation();

    NativeImagePtr image = nativeImageForCurrentFrame();
    if (!image)
        return;

    context->platformContext()->addImage(dst, src, image);

    if (ImageObserver* observer = imageObserver())
        observer->didDraw(this);
}

void Image::drawPattern(GraphicsContext* context, const FloatRect& src,
                        const AffineTransform& patternTransformation,
                        const FloatPoint& phase, ColorSpace styleColorSpace,
                        CompositeOperator op, const FloatRect& dst)
{
    NativeImagePtr image = nativeImageForCurrentFrame();
    if (!image)
        return;

    FloatSize tileScaleFactor = FloatSize(narrowPrecisionToFloat(patternTransformation.a()), narrowPrecisionToFloat(patternTransformation.d()));
    context->platformContext()->addPattern(image, src, tileScaleFactor, phase, dst);
}

} // namespace WebCore
#endif
