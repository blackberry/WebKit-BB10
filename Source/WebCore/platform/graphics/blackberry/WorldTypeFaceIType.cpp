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
#include "WorldTypeFace.h"

#include "FontPlatformData.h"
#include "GlyphBuffer.h"
#include "SimpleFontData.h"
#include "WorldTypeShaper.h"

#include <fs_api.h>
#include <tsshaperapi.h>

namespace WebCore {

// Our implementation of the callbacks which worldtype requires by using iType calls.

#define INT_TO_FIXED(a) \
    ((((FS_FIXED)(a)) < 0) ? -((FS_FIXED)(-(a)) << 16) : ((FS_FIXED)(a) << 16))

struct WorldTypeFontData {
    WorldTypeFontData(FS_STATE* iType, WTF::HashMap<uint32_t, uint16_t>* glyphCacheForFaceCacheEntry)
        : m_iType(iType)
        , m_opentypeCache(TsOtCache_new(0))
        , m_opentypeLayoutManager(TsOtLayoutManager_new(0))
        , m_glyphCacheForFaceCacheEntry(glyphCacheForFaceCacheEntry)
    { }

    ~WorldTypeFontData()
    {
        TsOtLayoutManager_delete(m_opentypeLayoutManager);
        TsOtCache_delete(m_opentypeCache);
    }

    FS_STATE* m_iType;
    TsOtCache* m_opentypeCache;
    TsOtLayoutManager* m_opentypeLayoutManager;
    WTF::HashMap<uint32_t, uint16_t>* m_glyphCacheForFaceCacheEntry;
};

// TsOtFont callback functions
static TsBool otfont_compare_equal(void *v1, void *v2)
{
    TS_UNUSED_PARAMETER(v1);
    TS_UNUSED_PARAMETER(v2);
    return TRUE;
}

static WorldTypeFontData* otfont_doNothingCopyHandle(WorldTypeFontData *face)
{
    return face;
}

static void otfont_doNothingReleaseHandle(WorldTypeFontData *face)
{
    TS_UNUSED_PARAMETER(face);
}

static void * otfont_getTable(void *data, TsTag tag)
{
    TS_ASSERT_AND_RETURN_NULL_IF_NULL(data);

    WorldTypeFontData* fontData = static_cast<WorldTypeFontData*>(data);

    TsUInt32 len;
    void* table = FS_get_table(fontData->m_iType, tag, TBL_EXTRACT, &len);

    return table;
}

static TsResult otfont_releaseTable(void *data, void *table)
{
    TS_ASSERT_AND_RETURN_ERROR_IF_NULL_ARG(data);

    if (!table)
        return TS_OK;

    WorldTypeFontData* fontData = static_cast<WorldTypeFontData*>(data);

    if (FS_free_table(fontData->m_iType, table) != SUCCESS)
        return TS_ERR_FM_RELEASE_TABLE_FAILED;

    return TS_OK;
}

// TsShaperFont callback functions
static void designUnits2pixels(void *data, TsInt16 xdu, TsInt16 ydu, TsFixed *xp, TsFixed *yp)
{
    TS_ASSERT_AND_RETURN_VOID_IF_NULL(data);

    *xp = 0; *yp = 0;

    WorldTypeFontData* fontData = static_cast<WorldTypeFontData*>(data);

    FONT_METRICS* fm = static_cast<FONT_METRICS*>(TS_MALLOC(sizeof(FONT_METRICS)));
    if (fm == NULL)
        return;

    if (FS_font_metrics(fontData->m_iType, fm) != SUCCESS) {
        TS_FREE(fm);
        return;
    }

    // legal range for units per em is 16 to 16386
    FS_SHORT upm = static_cast<FS_SHORT>(MIN(MAX(fm->unitsPerEm, 16), 16386));

    fm->unitsPerEm = static_cast<FS_USHORT>(upm);

    FS_FIXED x_scaled = FixDiv(INT_TO_FIXED(xdu), INT_TO_FIXED(upm));
    FS_FIXED y_scaled = FixDiv(INT_TO_FIXED(ydu), INT_TO_FIXED(upm));

    TS_FREE(fm);

    FS_FIXED m[4];
    if (FS_get_scale(fontData->m_iType, &m[0], &m[1], &m[2], &m[3]) != SUCCESS)
        return;

    /* see TsMatrix_transform2 */
    *xp = FixMul(m[0], x_scaled) + FixMul(m[1], y_scaled);
    *yp = FixMul(m[2], x_scaled) + FixMul(m[3], y_scaled);
}

static TsBool getGPOSpoint(void *data, TsUInt16 glyphID, TsUInt16 pointIndex, TsFixed26_6 *x, TsFixed26_6 *y)
{
    TS_ASSERT_AND_RETURN_VALUE_IF_NULL(data, FALSE);

    WorldTypeFontData* fontData = static_cast<WorldTypeFontData*>(data);

    if (FS_get_gpos_pts(fontData->m_iType, glyphID, 1, &pointIndex, static_cast<FS_LONG*>(x), static_cast<FS_LONG*>(y)) != SUCCESS)
        return FALSE;

    return TRUE;
}

static TsFixed getXSize(void *data)
{
    TS_ASSERT_AND_RETURN_VALUE_IF_NULL(data, 0);

    WorldTypeFontData* fontData = static_cast<WorldTypeFontData*>(data);

    TsMatrix m;
    if (FS_get_scale(fontData->m_iType, &m.a, &m.b, &m.c, &m.d) != SUCCESS)
        return 0;

    return m.a;
}

static TsFixed getYSize(void *data)
{
    TS_ASSERT_AND_RETURN_VALUE_IF_NULL(data, 0);

    WorldTypeFontData* fontData = static_cast<WorldTypeFontData*>(data);

    TsMatrix m;
    if (FS_get_scale(fontData->m_iType, &m.a, &m.b, &m.c, &m.d) != SUCCESS)
        return 0;

    return m.d;
}

static TsResult mapChar(void *data, TsInt32 charID, TsInt32 *glyphID)
{
    *glyphID = 0;

    TS_ASSERT_AND_RETURN_ERROR_IF_NULL_ARG(data);

    WorldTypeFontData* fontData = static_cast<WorldTypeFontData*>(data);

    if (charID < 0)
        return TS_ERR_FM_INVALID_CHARID;

    WTF::HashMap<uint32_t, uint16_t>::AddResult result = fontData->m_glyphCacheForFaceCacheEntry->add(charID, 0);
    if (result.isNewEntry) {
        *glyphID = static_cast<TsInt32>(FS_map_char(fontData->m_iType, static_cast<FS_ULONG>(charID)));
        result.iterator->second = *glyphID;
    } else
        *glyphID = result.iterator->second;

    return TS_OK;
}

TsOtCache* getOpenTypeCache(void *data)
{
    TS_ASSERT_AND_RETURN_NULL_IF_NULL(data);

    WorldTypeFontData* fontData = static_cast<WorldTypeFontData*>(data);

    return fontData->m_opentypeCache;
}

static TsOtLayout* findOtLayout(void *data, TsTag script, TsTag langSys)
{
    TS_ASSERT_AND_RETURN_NULL_IF_NULL(data);

    WorldTypeFontData* fontData = static_cast<WorldTypeFontData*>(data);

    TsOtFont otFont;
    otFont.fontData = fontData;
    otFont.holdHandle = (void (*)(void *))otfont_doNothingCopyHandle;
    otFont.releaseHandle = (void (*)(void *))otfont_doNothingReleaseHandle;
    otFont.compare = (TsBool (*)(void *, void *))otfont_compare_equal;
    otFont.getTable = (void *(*)(void *, TsUInt32))otfont_getTable;
    otFont.releaseTable = (TsResult (*)(void *, void *))otfont_releaseTable;
    return TsOtLayoutManager_findOtLayout(fontData->m_opentypeLayoutManager, &otFont, script, langSys);
}

static TsBool hasThaiPUA(void *data)
{
    TS_ASSERT_AND_RETURN_VALUE_IF_NULL(data, FALSE);

    /* This routine should only return TRUE for a Thai font that does NOT   */
    /* have OpenType tables for Thai, but which does have glyphs for Thai   */
    /* presentation forms assigned to the Unicode Private Use Area range    */
    /* U+F700..U+F71D. If you are unsure about this, return FALSE.           */

    return FALSE;
}

const TsShaperFontFuncs worldTypeITypeFontFuncs =
{
    designUnits2pixels,
    getGPOSpoint,
    getXSize,
    getYSize,
    mapChar,
    getOpenTypeCache,
    findOtLayout,
    hasThaiPUA
};

TsShaperFont* WorldTypeFace::createShaperFont()
{
    m_shaperFontData = new WorldTypeFontData(m_platformData->font(), m_glyphCacheForFaceCacheEntry);
    TsShaperFont* font = TsShaperFont_new(0, &worldTypeITypeFontFuncs, m_shaperFontData);
    ASSERT(font);

    return font;
}

void WorldTypeFace::deleteShaperFont()
{
    TsShaperFont_delete(m_shaperFont);
    delete static_cast<WorldTypeFontData*>(m_shaperFontData);
}

GlyphBufferAdvance WorldTypeShaper::createGlyphBufferAdvance(float width, float height)
{
    return GlyphBufferAdvance(width, height);
}

} // namespace WebCore
