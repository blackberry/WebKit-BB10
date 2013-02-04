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

#include "config.h"
#include "FontPlatformData.h"
#include "ITypeUtils.h"

#if USE(HARFBUZZ_NG)
#include "HarfBuzzNGFace.h"
#else
#include "WorldTypeFace.h"
#endif

#include <fs_api.h>
#include <BlackBerryPlatformGraphicsContext.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

FontPlatformData::FontPlatformData(FILECHAR* name, float size, bool syntheticBold, bool syntheticOblique, FontOrientation orientation,
        TextOrientation textOrientation, FontWidthVariant widthVariant)
    : m_syntheticBold(syntheticBold)
    , m_syntheticOblique(syntheticOblique)
    , m_orientation(orientation)
    , m_textOrientation(textOrientation)
    , m_size(size)
    , m_widthVariant(widthVariant)
    , m_font(0)
    , m_name(fastStrDup(name))
    , m_scaledFont(0)
#if USE(HARFBUZZ_NG)
    , m_harfbuzzFace()
#else
    , m_worldtypeFace()
#endif
    , m_isColorBitmapFont(false)
{
    ASSERT(name);
    m_font = FS_new_client(BlackBerry::Platform::Graphics::getIType(), 0);
    ASSERT(m_font);
    applyState(m_font);
}

FontPlatformData::~FontPlatformData()
{
    if (m_scaledFont) {
        BlackBerry::Platform::Graphics::clearGlyphCacheForFont(m_scaledFont);
        FS_end_client(m_scaledFont);
    }
    if (m_font && m_font != hashTableDeletedFontValue())
        FS_end_client(m_font);
    fastFree(m_name);
}

const char* FontPlatformData::name() const
{
    return m_name;
}

bool FontPlatformData::applyState(FS_STATE* font, float scale) const
{
    ASSERT(font);
    ASSERT(m_name);
    if (FS_set_font(font, m_name) != SUCCESS)
        return false;

    if (FS_set_cmap(font, 3, 10) != SUCCESS) // First try Windows Unicode with surrogates...
        if (FS_set_cmap(font, 3, 1) != SUCCESS) // try normal Windows Unicode
            if (FS_set_cmap(font, 1, 0) != SUCCESS)
                return false;

    if (m_syntheticBold) {
        if (FS_set_bold_pct(font, floatToITypeFixed(0.06)) != SUCCESS) // 6% pseudo bold
            return false;
    } else {
        if (FS_set_bold_pct(font, 0) != SUCCESS)
            return false;
    }

    FS_FIXED skew = 0;
    if (m_syntheticOblique)
        skew = 13930; // 12 degrees

    FS_FIXED fixedScale = std::min(FixMul(floatToITypeFixed(scale), floatToITypeFixed(m_size)), MAXITYPEFONTSCALE);
    if (FS_set_scale(font, fixedScale, FixMul(fixedScale, skew), 0, fixedScale) != SUCCESS)
        return false;

    if (FS_set_flags(font, FLAGS_CMAP_OFF) != SUCCESS)
        return false;

    if (FS_set_flags(font, FLAGS_HINTS_OFF) != SUCCESS)
        return false;

    if (FS_set_flags(font, FLAGS_DEFAULT_CSM_OFF) != SUCCESS)
        return false;

    if (m_orientation == Vertical) {
        if (FS_set_flags(font, (m_textOrientation == TextOrientationVerticalRight) ? FLAGS_VERTICAL_ROTATE_LEFT_ON : FLAGS_VERTICAL_ON) != SUCCESS)
            return false;
    }
    return true;
}

void FontPlatformData::platformDataInit(const FontPlatformData& source)
{
#if USE(HARFBUZZ_NG)
    m_harfbuzzFace = source.m_harfbuzzFace;
#else
    m_worldtypeFace = source.m_worldtypeFace;
#endif
    m_scaledFont = 0;
    if (source.m_font && source.m_font != hashTableDeletedFontValue()) {
        m_font = FS_new_client(source.m_font, 0);
        m_name = fastStrDup(source.name());
        bool ret = applyState(m_font);
        ASSERT_UNUSED(ret, ret);
    } else
        m_font = source.m_font;
}

const FontPlatformData& FontPlatformData::platformDataAssign(const FontPlatformData& other)
{
#if USE(HARFBUZZ_NG)
    m_harfbuzzFace = other.m_harfbuzzFace;
#else
    m_worldtypeFace = other.m_worldtypeFace;
#endif
    m_scaledFont = 0;
    if (other.m_font && other.m_font != hashTableDeletedFontValue()) {
        m_font = FS_new_client(other.m_font, 0);
        fastFree(m_name);
        m_name = fastStrDup(other.name());
        bool ret = applyState(m_font);
        ASSERT_UNUSED(ret, ret);
    } else
        m_font = other.m_font;

    return *this;
}

bool FontPlatformData::platformIsEqual(const FontPlatformData& other) const
{
    if (m_font == other.m_font)
        return true;

    if (m_font == 0 || m_font == hashTableDeletedFontValue() || other.m_font == 0 || other.m_font == hashTableDeletedFontValue())
        return false;

    return m_font->cur_sfnt == other.m_font->cur_sfnt;
}

#ifndef NDEBUG
String FontPlatformData::description() const
{
    return "iType Font Data";
}
#endif

#if USE(HARFBUZZ_NG)
HarfBuzzNGFace* FontPlatformData::harfbuzzFace()
{
    if (!m_harfbuzzFace) {
        uint64_t uniqueID = reinterpret_cast<uintptr_t>(m_font);
        m_harfbuzzFace = HarfBuzzNGFace::create(const_cast<FontPlatformData*>(this), uniqueID);
    }

    return m_harfbuzzFace.get();
}
#else
WorldTypeFace* FontPlatformData::worldtypeFace()
{
    if (!m_worldtypeFace) {
        ASSERT(m_font && m_font != hashTableDeletedFontValue());
        uint64_t uniqueID = reinterpret_cast<uintptr_t>(m_font);
        m_worldtypeFace = WorldTypeFace::create(const_cast<FontPlatformData*>(this), uniqueID);
    }

    return m_worldtypeFace.get();
}
#endif

FS_STATE* FontPlatformData::scaledFont(float scale) const
{
    ASSERT(scale > 0.0);
    if (m_scale == scale && m_scaledFont)
        return m_scaledFont;

    if (m_scaledFont) {
        BlackBerry::Platform::Graphics::clearGlyphCacheForFont(m_scaledFont);
        if (FS_end_client(m_scaledFont) != SUCCESS)
            CRASH();
    }

    m_scaledFont = FS_new_client(m_font, 0);
    if (!applyState(m_scaledFont, scale)) {
        if (FS_end_client(m_scaledFont) != SUCCESS)
            CRASH();
        m_scaledFont = 0;
        return 0;
    }

    m_scale = scale;

    return m_scaledFont;
}

const void* FontPlatformData::cur_sfnt() const
{
    ASSERT(m_font);
    return m_font->cur_sfnt;
}

bool FontPlatformData::isFixedPitch() const
{
    TTF_POST post;
    if (m_font && FS_get_table_structure(m_font, TAG_post, &post) == SUCCESS)
        return post.isFixedPitch;
    return false;
}

}
