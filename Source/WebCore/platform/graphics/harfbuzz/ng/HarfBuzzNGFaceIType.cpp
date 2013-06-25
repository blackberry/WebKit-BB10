/*
 * Copyright (c) 2012 Google Inc. All rights reserved.
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "HarfBuzzNGFace.h"

#include "FontPlatformData.h"
#include "GlyphBuffer.h"
#include "HarfBuzzShaper.h"
#include "SimpleFontData.h"

#include <fs_api.h>
#include <hb.h>

namespace WebCore {

// Our implementation of the callbacks which HarfBuzz requires by using iType calls.

#define INT_TO_FIXED(a) \
    ((((FS_FIXED)(a)) < 0) ? -((FS_FIXED)(-(a)) << 16) : ((FS_FIXED)(a) << 16))

static hb_bool_t harfbuzzGetGlyph(hb_font_t* hbFont, void* fontData, hb_codepoint_t unicode, hb_codepoint_t variationSelector, hb_codepoint_t* glyph, void* userData)
{
    FS_STATE* font = reinterpret_cast<FontPlatformData*>(fontData)->font();

    *glyph = FS_map_char_variant(font, unicode, variationSelector);

    return !!*glyph;
}

static hb_position_t harfbuzzGetGlyphHorizontalAdvance(hb_font_t* hbFont, void* fontData, hb_codepoint_t glyph, void* userData)
{
    FS_STATE* font = reinterpret_cast<FontPlatformData*>(fontData)->font();
    FS_SHORT i_dx, i_dy;
    FS_FIXED dx = 0, dy;
    if (FS_get_advance(font, glyph, FS_MAP_DISTANCEFIELD, &i_dx, &i_dy, &dx, &dy) != SUCCESS)
        return dx;

    return dx;
}

static hb_position_t harfbuzzGetGlyphHorizontalKerning(hb_font_t *hbFont, void *fontData, hb_codepoint_t leftGlyph, hb_codepoint_t rightGlyph, void *userData)
{
    FS_STATE* font = reinterpret_cast<FontPlatformData*>(fontData)->font();

    FS_FIXED dx = 0, dy;
    if (FS_get_kerning(font, leftGlyph, rightGlyph, &dx, &dy) != SUCCESS)
        return dx;

    return dx;
}

static hb_position_t harfbuzzGetGlyphVerticalKerning(hb_font_t *hbFont, void *fontData, hb_codepoint_t topGlyph, hb_codepoint_t bottomGlyph, void *userData)
{
    FS_STATE* font = reinterpret_cast<FontPlatformData*>(fontData)->font();

    FS_FIXED dx, dy = 0;
    if (FS_get_kerning(font, topGlyph, bottomGlyph, &dx, &dy) != SUCCESS)
        return dy;

    return dy;
}

static hb_bool_t harfbuzzGetGlyphHorizontalOrigin(hb_font_t* hbFont, void* fontData, hb_codepoint_t glyph, hb_position_t* x, hb_position_t* y, void* userData)
{
    return true;
}

static hb_bool_t harfbuzzGetGlyphExtents(hb_font_t* hbFont, void* fontData, hb_codepoint_t glyph, hb_glyph_extents_t* extents, void* userData)
{
    FS_STATE* font = reinterpret_cast<FontPlatformData*>(fontData)->font();

    FS_GLYPHMAP* glyphmap = FS_get_glyphmap(font, glyph, FS_MAP_DISTANCEFIELD);
    if (!glyphmap)
        return false;

    // Invert y-axis because iType is y-grows-down but we set up harfbuzz to be y-grows-up.
    extents->x_bearing = INT_TO_FIXED(glyphmap->lo_x);
    extents->y_bearing = -INT_TO_FIXED(glyphmap->hi_y);
    extents->width = INT_TO_FIXED(glyphmap->width);
    extents->height = INT_TO_FIXED(-glyphmap->height);

    if (FS_free_char(font, glyphmap) != SUCCESS)
        return false;

    return true;
}

static hb_font_funcs_t* harfbuzzITypeGetFontFuncs(const TypesettingFeatures& features)
{
    static hb_font_funcs_t* harfbuzzITypeFontFuncs = 0;
    static hb_font_funcs_t* harfbuzzITypeFontFuncsWithKerning = 0; // includes TrueType ("kern" table) kerning

    if (features & Kerning) {
        if (!harfbuzzITypeFontFuncsWithKerning) {
            harfbuzzITypeFontFuncsWithKerning = hb_font_funcs_create();
            hb_font_funcs_set_glyph_func(harfbuzzITypeFontFuncsWithKerning, harfbuzzGetGlyph, 0, 0);
            hb_font_funcs_set_glyph_h_advance_func(harfbuzzITypeFontFuncsWithKerning, harfbuzzGetGlyphHorizontalAdvance, 0, 0);
            hb_font_funcs_set_glyph_h_kerning_func(harfbuzzITypeFontFuncsWithKerning, harfbuzzGetGlyphHorizontalKerning, 0, 0);
            hb_font_funcs_set_glyph_h_origin_func(harfbuzzITypeFontFuncsWithKerning, harfbuzzGetGlyphHorizontalOrigin, 0, 0);
            hb_font_funcs_set_glyph_v_kerning_func(harfbuzzITypeFontFuncsWithKerning, harfbuzzGetGlyphVerticalKerning, 0, 0);
            hb_font_funcs_set_glyph_extents_func(harfbuzzITypeFontFuncsWithKerning, harfbuzzGetGlyphExtents, 0, 0);
            hb_font_funcs_make_immutable(harfbuzzITypeFontFuncsWithKerning);
        }
        return harfbuzzITypeFontFuncsWithKerning;
    }

    // We don't set callback functions which we can't support.
    // Harfbuzz will use the fallback implementation if they aren't set.
    if (!harfbuzzITypeFontFuncs) {
        harfbuzzITypeFontFuncs = hb_font_funcs_create();
        hb_font_funcs_set_glyph_func(harfbuzzITypeFontFuncs, harfbuzzGetGlyph, 0, 0);
        hb_font_funcs_set_glyph_h_advance_func(harfbuzzITypeFontFuncs, harfbuzzGetGlyphHorizontalAdvance, 0, 0);
        hb_font_funcs_set_glyph_h_origin_func(harfbuzzITypeFontFuncs, harfbuzzGetGlyphHorizontalOrigin, 0, 0);
        hb_font_funcs_set_glyph_extents_func(harfbuzzITypeFontFuncs, harfbuzzGetGlyphExtents, 0, 0);
        hb_font_funcs_make_immutable(harfbuzzITypeFontFuncs);
    }
    return harfbuzzITypeFontFuncs;
}

struct ITypeTableData {
    FS_STATE* font;
    FS_BYTE* data;
};

static void releaseTableData(void* userData)
{
    struct ITypeTableData* table = reinterpret_cast<struct ITypeTableData*>(userData);
    if (FS_free_table(table->font, table->data) != SUCCESS)
        CRASH();
    delete table;
}

static hb_blob_t* harfbuzzITypeGetTable(hb_face_t* face, hb_tag_t tag, void* userData)
{
    FS_STATE* font = reinterpret_cast<FS_STATE*>(userData);

    FS_ULONG length = 0;
    FS_BYTE* data = FS_get_table(font, tag, TBL_EXTRACT, &length);
    if (!data || !length)
        return 0;

    struct ITypeTableData* table = new ITypeTableData;
    table->font = font;
    table->data = data;

    return hb_blob_create(reinterpret_cast<char*>(data), length, HB_MEMORY_MODE_READONLY, table, releaseTableData);
}

hb_face_t* HarfBuzzNGFace::createFace()
{
    hb_face_t* face = hb_face_create_for_tables(harfbuzzITypeGetTable, m_platformData->font(), 0);
    ASSERT(face);
    return face;
}

hb_font_t* HarfBuzzNGFace::createFont(const TypesettingFeatures& features)
{
    hb_font_t* font = hb_font_create(m_face);
    hb_font_set_funcs(font, harfbuzzITypeGetFontFuncs(features), m_platformData, 0);
    const float size = m_platformData->size();
    hb_font_set_ppem(font, size, size);
    const int scale = (1 << 16) * static_cast<int>(size);
    hb_font_set_scale(font, scale, scale);
    hb_font_make_immutable(font);
    return font;
}

GlyphBufferAdvance HarfBuzzShaper::createGlyphBufferAdvance(float width, float height)
{
    return GlyphBufferAdvance(width, height);
}

} // namespace WebCore
