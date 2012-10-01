/*
 * Copyright (C) 2011, 2012 Research In Motion Limited. All rights reserved.
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
#include "PixelDumpSupportBlackBerry.h"

#include "DumpRenderTreeBlackBerry.h"
#include "PNGImageEncoder.h"
#include "PixelDumpSupport.h"
#include "WebPage.h"
#include "WebPageClient.h"

#include <BlackBerryPlatformWindow.h>
#include <wtf/MD5.h>
#include <wtf/Vector.h>
#include <string.h>

using namespace BlackBerry::WebKit;
using namespace BlackBerry;
using namespace WTF;

PassRefPtr<BitmapContext> createBitmapContextFromWebView(bool /*onscreen*/, bool /*incrementalRepaint*/, bool /*sweepHorizontally*/, bool /*drawSelectionRect*/)
{
    Platform::Graphics::Window* window = DumpRenderTree::currentInstance()->page()->client()->window();
    ASSERT(window);

    const Platform::IntRect& windowRect = window->viewportRect();
    unsigned char* data = new unsigned char[windowRect.width() * windowRect.height() * 4];

    // We need to force a synchronous update to the window or we may get an empty bitmap.
    // For example, running DRT with one test case that finishes before the screen is updated.
    window->post(windowRect);

#if USE(SKIA)
    const SkBitmap* windowBitmap = lockBufferBackingImage(window->buffer(), Platform::Graphics::ReadAccess);
    SkAutoLockPixels lock(*windowBitmap);
    const unsigned char* windowPixels = static_cast<const unsigned char*>(windowBitmap->getPixels());
#else
    const unsigned char* windowPixels = lockBufferBackingImage(window->buffer(), Platform::Graphics::ReadAccess);
#endif
    memcpy(data, windowPixels, windowRect.width() * windowRect.height() * 4);
    Platform::Graphics::releaseBufferBackingImage(window->buffer());
    return BitmapContext::createByAdoptingData(data, windowRect.width(), windowRect.height());
}

void computeMD5HashStringForBitmapContext(BitmapContext* context, char hashString[33])
{
    int pixelsWide = context->m_width;
    int pixelsHigh = context->m_height;
    int bytesPerRow = context->m_width * 4;
    unsigned char* pixelData = context->m_data;

    MD5 md5;
    for (int i = 0; i < pixelsHigh; ++i) {
        md5.addBytes(pixelData, 4 * pixelsWide);
        pixelData += bytesPerRow;
    }

    Vector<uint8_t, 16> hash;
    md5.checksum(hash);

    hashString[0] = '\0';
    for (int i = 0; i < 16; ++i)
        snprintf(hashString, 33, "%s%02x", hashString, hash[i]);
}

static void printPNG(BitmapContext* context, const char* checksum)
{
    Vector<unsigned char> pngData;
    encodeBitmapToPNG(context->m_data, context->m_width, context->m_height, &pngData);
    printPNG(pngData.data(), pngData.size(), checksum);
}

void dumpBitmap(BitmapContext* context, const char* checksum)
{
    printPNG(context, checksum);
}
