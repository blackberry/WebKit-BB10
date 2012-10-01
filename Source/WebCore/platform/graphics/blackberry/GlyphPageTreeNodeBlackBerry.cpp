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
#include "GlyphPageTreeNode.h"

#include "SimpleFontData.h"

namespace WebCore {

bool GlyphPage::fill(unsigned offset, unsigned length, UChar* buffer, unsigned bufferLength, const SimpleFontData* fontData)
{
    TsFont* font = fontData->platformData().font()->font();

    bool haveGlyphs = false;
    for (unsigned i = 0; i < length; ++i) {
        TsInt32 glyph;
        // TODO: surrogate pairs
        TsResult result = TsFont_mapChar(font, buffer[i], &glyph);
        if (result != TS_OK || glyph == TsGlyphID_NOP || glyph == TsGlyphID_MissingGlyph)
            setGlyphDataForIndex(offset + i, 0, 0);
        else {
            // TODO: more than 16-bit glyphs?
            setGlyphDataForIndex(offset + i, static_cast<Glyph>(glyph), fontData);
            haveGlyphs = true;
        }
    }
    return haveGlyphs;
}

}
