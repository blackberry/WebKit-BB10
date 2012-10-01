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
#include "Font.h"

#include "FloatPoint.h"
#include "FloatRect.h"
#include "FontCache.h"
#include "GraphicsContext.h"
#include "AffineTransform.h"
#include "TextRun.h"

#include <BlackBerryPlatformGraphicsContext.h>

#include <wtf/MathExtras.h>

namespace WebCore {

TsLayout* gLayout;

static TsLayout* getLayout()
{
    if (!gLayout) {
        TsLayoutOptions* layoutOptions = TsLayoutOptions_new();
        TsLayoutControl* layoutControl = TsLayoutControl_new();
        gLayout = TsLayout_new(NULL, layoutControl, layoutOptions);
    }
    return gLayout;
}

class BlackBerryDC : public TsDC {
public:
    TsFontStyle* scaledFont;
    PlatformGraphicsContext* context;

    static BlackBerryDC* instance();

private:
    BlackBerryDC();
    static BlackBerryDC* s_instance;
};

BlackBerryDC* BlackBerryDC::s_instance;

BlackBerryDC* BlackBerryDC::instance()
{
    if (!s_instance) {
        s_instance = new BlackBerryDC;
    }
    return s_instance;
}

static TsResult getPixel(TsDC *dc, TsInt32 x, TsInt32 y, TsColor *color)
{
    return TS_OK;
}
TsResult setPixel(TsDC *dc, TsInt32 x, TsInt32 y, TsColor color)
{
    return TS_OK;
}
TsResult getDeviceClip(TsDC *dc, TsRect *clip, TsBool *haveClip)
{
    return TS_OK;
}
TsResult setDeviceClip(TsDC *dc, const TsRect *clip)
{
    return TS_OK;
}
TsResult fillRect(TsDC *dc, const TsRect *rect, TsColor color)
{
    return TS_OK;
}
TsResult highlightRect(TsDC *dc, const TsRect *rect)
{
    return TS_OK;
}
TsResult drawLine(TsDC *dc, TsInt32 x1, TsInt32 y1, TsInt32 x2, TsInt32 y2, TsPen *pen)
{
    return TS_OK;
}
TsResult drawImage(TsDC *dc, TsImageType type, const void *imageData, TsInt32 x, TsInt32 y)
{
    return TS_OK;
}

TsResult drawGlyphs(TsDC *dc, TsDisplayListEntry *entry, TsInt32 numGlyphs, TsFontStyle *style, TsGlyphLayer layer, TsColor color, TsInt32 x, TsInt32 y, TsRect *clipRect)
{
    BlackBerryDC* bdc = static_cast<BlackBerryDC*>(dc);
    for (int i = 0; i < numGlyphs; ++i) {
        unsigned glyph = entry[i].glyphID;
        BlackBerry::Platform::FloatSize advance(0, 0);
        FloatPoint pt((x + entry[i].glyphX) / 65536.0, (y + entry[i].glyphY) / 65536.0);
        bdc->context->addGlyphs(&glyph, &advance, 1, pt, bdc->scaledFont);
    }
    return TS_OK;
}

TsResult setTransform(TsDC *dc, TsMatrix33 *m)
{
    return TS_OK;
}


BlackBerryDC::BlackBerryDC()
{
    static TsDC_Funcs funcs = {
        &getPixel,
        &setPixel,
        &getDeviceClip,
        &setDeviceClip,
        &fillRect,
        &highlightRect,
        &drawLine,
        &drawImage,
        &drawGlyphs,
        &setTransform
    };
    TsDC_init(this, &funcs, NULL);
}

void Font::drawComplexText(GraphicsContext* context, const TextRun& run, const FloatPoint& point, int from, int to) const
{
    BlackBerryDC* dc = BlackBerryDC::instance();
    dc->context = context->platformContext();
    dc->scaledFont = primaryFont()->platformData().font()->scaledFont(context->getCTM().a());

    TsFontStyle* font = primaryFont()->platformData().font()->unscaledFont();
    float ascender = TsFontStyle_getAscender(font) / 65536.0;
    TsLayout_drawString(getLayout(), const_cast<UChar*>(run.characters()), run.length(), TS_UTF16,
                        font, dc, (int)(point.x() * 65536), (int)((point.y() - ascender) * 65536), NULL);
}

float Font::floatWidthForComplexText(const TextRun& run, HashSet<const SimpleFontData*>* fallbackFonts, GlyphOverflow*) const
{
    TsInt32 width = 0;
    TsLayout_getStringWidth(getLayout(), const_cast<UChar*>(run.characters()), run.length(), TS_UTF16, primaryFont()->platformData().font()->unscaledFont(), &width);
    return width;
}

int Font::offsetForPositionForComplexText(const TextRun& run, float position, bool includePartialGlyphs) const
{
}

FloatRect Font::selectionRectForComplexText(const TextRun& run, const FloatPoint& point, int height, int from, int to) const
{
    return FloatRect();
}

void Font::drawGlyphs(GraphicsContext* context, const SimpleFontData* font, const GlyphBuffer& glyphBuffer,
                      int from, int numGlyphs, const FloatPoint& point) const
{
    context->platformContext()->addGlyphs(glyphBuffer.glyphs(from),
                                          reinterpret_cast<const BlackBerry::Platform::FloatSize*>(glyphBuffer.advances(from)),
                                          numGlyphs, point,
                                          font->platformData().font()->scaledFont(context->getCTM().a()));
}

bool Font::canReturnFallbackFontsForComplexText()
{
    return true;
}

void Font::drawEmphasisMarksForComplexText(GraphicsContext* /* context */, const TextRun& /* run */, const AtomicString& /* mark */, const FloatPoint& /* point */, int /* from */, int /* to */) const
{
    //notImplemented();
}

bool Font::canExpandAroundIdeographsInComplexText()
{
    return false;
}

}
