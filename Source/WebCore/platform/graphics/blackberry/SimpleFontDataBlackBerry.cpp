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
#include "SimpleFontData.h"

#include "Font.h"
#include "FontCache.h"
#include "FloatRect.h"
#include "FontDescription.h"

namespace WebCore {

// Smallcaps versions of fonts are 70% the size of the normal font.
static const float smallCapsFraction = 0.7f;
static const float emphasisMarkFraction = .5;

void SimpleFontData::platformInit()
{
    TsFontStyle* font = m_platformData.font()->unscaledFont();
    float ascender = TsFontStyle_getAscender(font) / 65536.0;
    float descender = TsFontStyle_getDescender(font) / 65536.0;
    float leading = TsFontStyle_getLeading(font) / 65536.0;
    m_fontMetrics.setAscent(ascender);
    m_fontMetrics.setDescent(descender);
    m_fontMetrics.setLineGap(leading);
    m_fontMetrics.setLineSpacing(lroundf(ascender) + lroundf(descender) + lroundf(leading));
    //m_fontMetrics.setUnitsPerEm(unitsPerEm);
}

void SimpleFontData::platformCharWidthInit()
{
    TsRect bbox;
    TsFontStyle_getBoundingBox(m_platformData.font()->unscaledFont(), &bbox);
    m_maxCharWidth = bbox.x2 - bbox.x1;
    m_avgCharWidth = 0;
    initCharWidths();
}

void SimpleFontData::platformDestroy()
{
}

PassOwnPtr<SimpleFontData> SimpleFontData::createScaledFontData(const FontDescription& fontDescription, float scaleFactor) const
{
    TsFontParam param = m_platformData.font()->param();
    param.size = static_cast<TsFixed>(param.size * scaleFactor);
    return adoptPtr(new SimpleFontData(FontPlatformData(MonotypeFont::create(m_platformData.font()->font(), param),
                                                        m_platformData.size() * scaleFactor,
                                                        m_platformData.syntheticBold(),
                                                        m_platformData.syntheticOblique(),
                                                        m_platformData.orientation(),
                                                        m_platformData.textOrientation(),
                                                        m_platformData.widthVariant()), isCustomFont(), false));
}

SimpleFontData* SimpleFontData::smallCapsFontData(const FontDescription& fontDescription) const
{
    if (!m_derivedFontData)
        m_derivedFontData = DerivedFontData::create(isCustomFont());
    if (!m_derivedFontData->smallCaps)
        m_derivedFontData->smallCaps = createScaledFontData(fontDescription, smallCapsFraction);

    return m_derivedFontData->smallCaps.get();
}

SimpleFontData* SimpleFontData::emphasisMarkFontData(const FontDescription& fontDescription) const
{
    if (!m_derivedFontData)
        m_derivedFontData = DerivedFontData::create(isCustomFont());
    if (!m_derivedFontData->emphasisMark)
        m_derivedFontData->emphasisMark = createScaledFontData(fontDescription, emphasisMarkFraction);

    return m_derivedFontData->emphasisMark.get();
}

bool SimpleFontData::containsCharacters(const UChar* characters, int length) const
{
    for (int i = 0; i < length; ++i) {
        TsInt32 glyph;
        TsResult res = TsFontStyle_mapChar(m_platformData.font()->unscaledFont(), characters[i], &glyph);
        if (res != TS_OK || glyph == TsGlyphID_NOP || glyph == TsGlyphID_MissingGlyph)
            return false;
    }
    return true;
}

void SimpleFontData::determinePitch()
{
    //m_treatAsFixedPitch = platformData().isFixedPitch();
}

FloatRect SimpleFontData::platformBoundsForGlyph(Glyph glyph) const
{
    TsGlyphMetrics metrics;
    TsFontStyle_getGlyphMetrics(m_platformData.font()->unscaledFont(), glyph, &metrics);
    TsRect& bb = metrics.boundingBox;
    return FloatRect(bb.x1, bb.y1, bb.x2 - bb.x1, bb.y2 - bb.y1);
}

float SimpleFontData::platformWidthForGlyph(Glyph glyph) const
{
    TsGlyphMetrics metrics;
    TsFontStyle_getGlyphMetrics(m_platformData.font()->unscaledFont(), glyph, &metrics);
    return metrics.advance.x / 65536.0;
}

}  // namespace WebCore
