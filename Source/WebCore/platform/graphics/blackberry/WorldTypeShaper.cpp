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
#include "WorldTypeShaper.h"

#include "Font.h"
#include "SurrogatePairAwareTextIterator.h"
#include "TextRun.h"
#include "WorldTypeFace.h"
#include <unicode/normlzr.h>
#include <unicode/uchar.h>
#include <wtf/MathExtras.h>
#include <wtf/Vector.h>
#include <wtf/unicode/Unicode.h>

namespace WebCore {

TsTag uScriptCodeToWorldTypeLanguage(UScriptCode script)
{
    switch (script) {
      case USCRIPT_INVALID_CODE:
        ASSERT_NOT_REACHED();
        return 0;
      case USCRIPT_COMMON: /* Zyyy */
        return 0;
      case USCRIPT_INHERITED: /* "Code for inherited script", for non-spacing combining marks; also Qaai */
        return 0;
      case USCRIPT_ARABIC: /* Arab */
        return TsTag_ARA;
      case USCRIPT_ARMENIAN: /* Armn */
        return TsTag_HYE;
      case USCRIPT_BENGALI: /* Beng */
        return TsTag_BEN;
      case USCRIPT_BOPOMOFO: /* Bopo */
        return TsTag_ZHP;
      case USCRIPT_CHEROKEE: /* Cher */
        return TsTag_CHR;
      case USCRIPT_COPTIC: /* Copt */
        return TsTag_COP;
      case USCRIPT_CYRILLIC: /* Cyrl */
        return TsTag_RUS;
      case USCRIPT_DEVANAGARI: /* Deva */
        return TsTag_HIN;
      case USCRIPT_GEORGIAN: /* Geor */
        return TsTag_KAT;
      case USCRIPT_GREEK: /* Grek */
        return TsTag_ELL;
      case USCRIPT_GUJARATI: /* Gujr */
        return TsTag_GUJ;
      case USCRIPT_GURMUKHI: /* Guru */
        return TsTag_PAN;
      case USCRIPT_HANGUL: /* Hang */
      case USCRIPT_KOREAN: /* Kore */
        return TsTag_KOR;
      case USCRIPT_HEBREW: /* Hebr */
        return TsTag_IWR;
      case USCRIPT_HIRAGANA: /* Hira */
      case USCRIPT_KATAKANA: /* Kana */
      case USCRIPT_KATAKANA_OR_HIRAGANA: /*Hrkt */
      case USCRIPT_JAPANESE: /* Jpan */
        return TsTag_JAN;
      case USCRIPT_KANNADA: /* Knda */
        return TsTag_KAN;
      case USCRIPT_KHMER: /* Khmr */
        return TsTag_KHM;
      case USCRIPT_LAO: /* Laoo */
        return TsTag_LAO;
      case USCRIPT_LATIN: /* Latn */
        return TsTag_ENG;
      case USCRIPT_MALAYALAM: /* Mlym */
        return TsTag_MAL;
      case USCRIPT_MONGOLIAN: /* Mong */
        return TsTag_MNG;
      case USCRIPT_ORIYA: /* Orya */
        return TsTag_ORI;
      case USCRIPT_SINHALA: /* Sinh */
        return TsTag_SNH;
      case USCRIPT_SYRIAC: /* Syrc */
        return TsTag_SYR;
      case USCRIPT_TAMIL: /* Taml */
        return TsTag_TAM;
      case USCRIPT_TELUGU: /* Telu */
        return TsTag_TEL;
      case USCRIPT_THAI: /* Thai */
        return TsTag_THA;
      case USCRIPT_TIBETAN: /* Tibt */
        return TsTag_TIB;
      case USCRIPT_YI: /* Yiii */
        return TsTag_YIM;
      case USCRIPT_HAN: /* Hani */
      case USCRIPT_SIMPLIFIED_HAN: /* Hans */
        return TsTag_ZHS;
      case USCRIPT_TRADITIONAL_HAN: /* Hant */
        return TsTag_ZHT;
      case USCRIPT_JAVANESE: /* Java */
        return TsTag_JAV;
      case USCRIPT_LATIN_GAELIC: /* Latg */
        return TsTag_GAE;
      case USCRIPT_TAI_VIET: /* Tavt */
        return TsTag_VIT;

      case USCRIPT_DESERET: /* Dsrt */
      case USCRIPT_ETHIOPIC: /* Ethi */
      case USCRIPT_GOTHIC: /* Goth */
      case USCRIPT_MYANMAR: /* Mymr */
      case USCRIPT_OGHAM: /* Ogam */
      case USCRIPT_OLD_ITALIC: /* Ital */
      case USCRIPT_RUNIC: /* Runr */
      case USCRIPT_THAANA: /* Thaa */
      case USCRIPT_CANADIAN_ABORIGINAL: /* Cans */
      case USCRIPT_TAGALOG: /* Tglg */
      case USCRIPT_HANUNOO: /* Hano */
      case USCRIPT_BUHID: /* Buhd */
      case USCRIPT_TAGBANWA: /* Tagb */
      case USCRIPT_BRAILLE: /* Brai */
      case USCRIPT_CYPRIOT: /* Cprt */
      case USCRIPT_LIMBU: /* Limb */
      case USCRIPT_LINEAR_B: /* Linb */
      case USCRIPT_OSMANYA: /* Osma */
      case USCRIPT_SHAVIAN: /* Shaw */
      case USCRIPT_TAI_LE: /* Tale */
      case USCRIPT_UGARITIC: /* Ugar */
      case USCRIPT_BUGINESE: /* Bugi */
      case USCRIPT_GLAGOLITIC: /* Glag */
      case USCRIPT_KHAROSHTHI: /* Khar */
      case USCRIPT_SYLOTI_NAGRI: /* Sylo */
      case USCRIPT_NEW_TAI_LUE: /* Talu */
      case USCRIPT_TIFINAGH: /* Tfng */
      case USCRIPT_OLD_PERSIAN: /* Xpeo */
      case USCRIPT_BALINESE: /* Bali */
      case USCRIPT_BATAK: /* Batk */
      case USCRIPT_BLISSYMBOLS: /* Blis */
      case USCRIPT_BRAHMI: /* Brah */
      case USCRIPT_CHAM: /* Cham */
      case USCRIPT_CIRTH: /* Cirt */
      case USCRIPT_OLD_CHURCH_SLAVONIC_CYRILLIC: /* Cyrs */
      case USCRIPT_DEMOTIC_EGYPTIAN: /* Egyd */
      case USCRIPT_HIERATIC_EGYPTIAN: /* Egyh */
      case USCRIPT_EGYPTIAN_HIEROGLYPHS: /* Egyp */
      case USCRIPT_KHUTSURI: /* Geok */
      case USCRIPT_PAHAWH_HMONG: /* Hmng */
      case USCRIPT_OLD_HUNGARIAN: /* Hung */
      case USCRIPT_HARAPPAN_INDUS: /* Inds */
      case USCRIPT_KAYAH_LI: /* Kali */
      case USCRIPT_LATIN_FRAKTUR: /* Latf */
      case USCRIPT_LEPCHA: /* Lepc */
      case USCRIPT_LINEAR_A: /* Lina */
      case USCRIPT_MANDAIC: /* Mand */
      case USCRIPT_MAYAN_HIEROGLYPHS: /* Maya */
      case USCRIPT_MEROITIC_HIEROGLYPHS: /* Mero */
      case USCRIPT_NKO: /* Nkoo */
      case USCRIPT_ORKHON: /* Orkh */
      case USCRIPT_OLD_PERMIC: /* Perm */
      case USCRIPT_PHAGS_PA: /* Phag */
      case USCRIPT_PHOENICIAN: /* Phnx */
      case USCRIPT_PHONETIC_POLLARD: /* Plrd */
      case USCRIPT_RONGORONGO: /* Roro */
      case USCRIPT_SARATI: /* Sara */
      case USCRIPT_ESTRANGELO_SYRIAC: /* Syre */
      case USCRIPT_WESTERN_SYRIAC: /* Syrj */
      case USCRIPT_EASTERN_SYRIAC: /* Syrn */
      case USCRIPT_TENGWAR: /* Teng */
      case USCRIPT_VAI: /* Vaii */
      case USCRIPT_VISIBLE_SPEECH: /* Visp */
      case USCRIPT_CUNEIFORM: /* Xsux */
      case USCRIPT_UNWRITTEN_LANGUAGES: /* Zxxx */
      case USCRIPT_UNKNOWN: /* Unknown="Code for uncoded script", for unassigned code points */
      case USCRIPT_CARIAN: /* Cari */
      case USCRIPT_LANNA: /* Lana */
      case USCRIPT_LYCIAN: /* Lyci */
      case USCRIPT_LYDIAN: /* Lydi */
      case USCRIPT_OL_CHIKI: /* Olck */
      case USCRIPT_REJANG: /* Rjng */
      case USCRIPT_SAURASHTRA: /* Saur */
      case USCRIPT_SIGN_WRITING: /* Sgnw */
      case USCRIPT_SUNDANESE: /* Sund */
      case USCRIPT_MOON: /* Moon */
      case USCRIPT_MEITEI_MAYEK: /* Mtei */
      case USCRIPT_IMPERIAL_ARAMAIC: /* Armi */
      case USCRIPT_AVESTAN: /* Avst */
      case USCRIPT_CHAKMA: /* Cakm */
      case USCRIPT_KAITHI: /* Kthi */
      case USCRIPT_MANICHAEAN: /* Mani */
      case USCRIPT_INSCRIPTIONAL_PAHLAVI: /* Phli */
      case USCRIPT_PSALTER_PAHLAVI: /* Phlp */
      case USCRIPT_BOOK_PAHLAVI: /* Phlv */
      case USCRIPT_INSCRIPTIONAL_PARTHIAN: /* Prti */
      case USCRIPT_SAMARITAN: /* Samr */
      case USCRIPT_MATHEMATICAL_NOTATION: /* Zmth */
      case USCRIPT_SYMBOLS: /* Zsym */
      case USCRIPT_BAMUM: /* Bamu */
      case USCRIPT_LISU: /* Lisu */
      case USCRIPT_NAKHI_GEBA: /* Nkgb */
      case USCRIPT_OLD_SOUTH_ARABIAN: /* Sarb */
      case USCRIPT_BASSA_VAH: /* Bass */
      case USCRIPT_DUPLOYAN_SHORTAND: /* Dupl */
      case USCRIPT_ELBASAN: /* Elba */
      case USCRIPT_GRANTHA: /* Gran */
      case USCRIPT_KPELLE: /* Kpel */
      case USCRIPT_LOMA: /* Loma */
      case USCRIPT_MENDE: /* Mend */
      case USCRIPT_MEROITIC_CURSIVE: /* Merc */
      case USCRIPT_OLD_NORTH_ARABIAN: /* Narb */
      case USCRIPT_NABATAEAN: /* Nbat */
      case USCRIPT_PALMYRENE: /* Palm */
      case USCRIPT_SINDHI: /* Sind */
      case USCRIPT_WARANG_CITI: /* Wara */
      case USCRIPT_AFAKA: /* Afak */
      case USCRIPT_JURCHEN: /* Jurc */
      case USCRIPT_MRO: /* Mroo */
      case USCRIPT_NUSHU: /* Nshu */
      case USCRIPT_SHARADA: /* Shrd */
      case USCRIPT_SORA_SOMPENG: /* Sora */
      case USCRIPT_TAKRI: /* Takr */
      case USCRIPT_TANGUT: /* Tang */
      case USCRIPT_WOLEAI: /* Wole */
      case USCRIPT_ANATOLIAN_HIEROGLYPHS: /* Hluw */
      case USCRIPT_KHOJKI: /* Khoj */
      case USCRIPT_TIRHUTA: /* Tirh */
      default:
        return 0;
    }
}

template<typename T>
class WorldTypeScopedPtr {
public:
    typedef void (*DestroyFunction)(T*);

    WorldTypeScopedPtr(T* ptr, DestroyFunction destroy)
        : m_ptr(ptr)
        , m_destroy(destroy)
    {
        ASSERT(m_destroy);
    }
    ~WorldTypeScopedPtr()
    {
        if (m_ptr)
            (*m_destroy)(m_ptr);
    }

    T* get() { return m_ptr; }
private:
    T* m_ptr;
    DestroyFunction m_destroy;
};

static inline float worldtypePositionToFloat(TsFixed value)
{
    return static_cast<float>(value) / (1 << 16);
}

WorldTypeShaper::WorldTypeRun::WorldTypeRun(const SimpleFontData* fontData, unsigned startIndex, unsigned numCharacters, TextDirection direction, UScriptCode script)
    : m_fontData(fontData)
    , m_startIndex(startIndex)
    , m_numCharacters(numCharacters)
    , m_direction(direction)
    , m_script(script)
{
}

void WorldTypeShaper::WorldTypeRun::applyShapeResult(TsShaperText* worldtypeBuffer)
{
    ASSERT(worldtypeBuffer);
    m_numGlyphs = worldtypeBuffer->textInfoArrayLen;
    m_glyphs.resize(m_numGlyphs);
    m_advances.resize(m_numGlyphs);
    m_glyphToCharacterIndexes.resize(m_numGlyphs);
    m_offsets.resize(m_numGlyphs);
}

void WorldTypeShaper::WorldTypeRun::setGlyphAndPositions(unsigned index, uint16_t glyphId, float advance, float offsetX, float offsetY)
{
    m_glyphs[index] = glyphId;
    m_advances[index] = advance;
    m_offsets[index] = FloatPoint(offsetX, offsetY);
}

int WorldTypeShaper::WorldTypeRun::characterIndexForXPosition(float targetX)
{
    ASSERT(targetX <= m_width);
    float currentX = 0;
    float currentAdvance = m_advances[0];
    unsigned glyphIndex = 0;

    // Sum up advances that belong to a character.
    while (glyphIndex < m_numGlyphs - 1 && m_glyphToCharacterIndexes[glyphIndex] == m_glyphToCharacterIndexes[glyphIndex + 1])
        currentAdvance += m_advances[++glyphIndex];
    currentAdvance = currentAdvance / 2.0;
    if (targetX <= currentAdvance)
        return rtl() ? m_numCharacters : 0;

    ++glyphIndex;
    while (glyphIndex < m_numGlyphs) {
        unsigned prevCharacterIndex = m_glyphToCharacterIndexes[glyphIndex - 1];
        float prevAdvance = currentAdvance;
        currentAdvance = m_advances[glyphIndex];
        while (glyphIndex < m_numGlyphs - 1 && m_glyphToCharacterIndexes[glyphIndex] == m_glyphToCharacterIndexes[glyphIndex + 1])
            currentAdvance += m_advances[++glyphIndex];
        currentAdvance = currentAdvance / 2.0;
        float nextX = currentX + prevAdvance + currentAdvance;
        if (currentX <= targetX && targetX <= nextX)
            return rtl() ? prevCharacterIndex : m_glyphToCharacterIndexes[glyphIndex];
        currentX = nextX;
        prevAdvance = currentAdvance;
        ++glyphIndex;
    }

    return rtl() ? 0 : m_numCharacters;
}

float WorldTypeShaper::WorldTypeRun::xPositionForOffset(unsigned offset)
{
    ASSERT(offset < m_numCharacters);
    unsigned glyphIndex = 0;
    float position = 0;
    if (rtl()) {
        while (glyphIndex < m_numGlyphs && m_glyphToCharacterIndexes[glyphIndex] > offset) {
            position += m_advances[glyphIndex];
            ++glyphIndex;
        }
        // For RTL, we need to return the right side boundary of the character.
        // Add advance of glyphs which are part of the character.
        while (glyphIndex < m_numGlyphs - 1 && m_glyphToCharacterIndexes[glyphIndex] == m_glyphToCharacterIndexes[glyphIndex + 1]) {
            position += m_advances[glyphIndex];
            ++glyphIndex;
        }
        position += m_advances[glyphIndex];
    } else {
        while (glyphIndex < m_numGlyphs && m_glyphToCharacterIndexes[glyphIndex] < offset) {
            position += m_advances[glyphIndex];
            ++glyphIndex;
        }
    }
    return position;
}

static void normalizeCharacters(const UChar* source, UChar* destination, int length)
{
    int position = 0;
    bool error = false;
    while (position < length) {
        UChar32 character;
        int nextPosition = position;
        U16_NEXT(source, nextPosition, length, character);
        // Don't normalize tabs as they are not treated as spaces for word-end.
        if (Font::treatAsSpace(character) && character != '\t')
            character = ' ';
        else if (Font::treatAsZeroWidthSpaceInComplexScript(character))
            character = zeroWidthSpace;
        U16_APPEND(destination, position, length, character, error);
        ASSERT_UNUSED(error, !error);
        position = nextPosition;
    }
}

WorldTypeShaper::WorldTypeShaper(const Font* font, const TextRun& run)
    : HarfBuzzShaperBase(font, run)
    , m_fromIndex(0)
    , m_toIndex(m_run.length())
{
    m_normalizedBuffer = adoptArrayPtr(new UChar[m_run.length() + 1]);
    m_normalizedBufferLength = m_run.length();
    normalizeCharacters(m_run.characters16(), m_normalizedBuffer.get(), m_normalizedBufferLength);
    setPadding(m_run.expansion());
    setFontFeatures();
}

WorldTypeShaper::~WorldTypeShaper()
{
    //TsShaper_delete(m_shaper);
}

void WorldTypeShaper::setDrawRange(int from, int to)
{
    ASSERT(from >= 0);
    ASSERT(to <= m_run.length());
    m_fromIndex = from;
    m_toIndex = to;
}

void WorldTypeShaper::setFontFeatures()
{
#if 0
    const FontDescription& description = m_font->fontDescription();
    if (description.orientation() == Vertical) {
        static hb_feature_t vert = { HarfBuzzNGFace::vertTag, 1, 0, static_cast<unsigned>(-1) };
        static hb_feature_t vrt2 = { HarfBuzzNGFace::vrt2Tag, 1, 0, static_cast<unsigned>(-1) };
        m_features.append(vert);
        m_features.append(vrt2);
    }

    FontFeatureSettings* settings = description.featureSettings();
    if (!settings)
        return;

    unsigned numFeatures = settings->size();
    for (unsigned i = 0; i < numFeatures; ++i) {
        hb_feature_t feature;
        const UChar* tag = settings->at(i).tag().characters();
        feature.tag = HB_TAG(tag[0], tag[1], tag[2], tag[3]);
        feature.value = settings->at(i).value();
        feature.start = 0;
        feature.end = static_cast<unsigned>(-1);
        m_features.append(feature);
    }
#endif
}

bool WorldTypeShaper::shape(GlyphBuffer* glyphBuffer)
{
    if (!collectWorldTypeRuns())
        return false;

    m_totalWidth = 0;
    if (!shapeWorldTypeRuns())
        return false;
    m_totalWidth = roundf(m_totalWidth);

    if (glyphBuffer)
        fillGlyphBuffer(glyphBuffer);

    return true;
}

FloatPoint WorldTypeShaper::adjustStartPoint(const FloatPoint& point)
{
    return point + m_startOffset;
}

bool WorldTypeShaper::collectWorldTypeRuns()
{
    const UChar* normalizedBufferEnd = m_normalizedBuffer.get() + m_normalizedBufferLength;
    SurrogatePairAwareTextIterator iterator(m_normalizedBuffer.get(), 0, m_normalizedBufferLength, m_normalizedBufferLength);
    UChar32 character;
    unsigned clusterLength = 0;
    unsigned startIndexOfCurrentRun = 0;
    if (!iterator.consume(character, clusterLength))
        return false;

    const SimpleFontData* nextFontData = m_font->glyphDataForCharacter(character, false).fontData;
    UErrorCode errorCode = U_ZERO_ERROR;
    UScriptCode nextScript = uscript_getScript(character, &errorCode);
    if (U_FAILURE(errorCode))
        return false;

    do {
        const UChar* currentCharacterPosition = iterator.characters();
        const SimpleFontData* currentFontData = nextFontData;
        UScriptCode currentScript = nextScript;

        for (iterator.advance(clusterLength); iterator.consume(character, clusterLength); iterator.advance(clusterLength)) {
            if (Font::treatAsZeroWidthSpace(character))
                continue;
            if (U_GET_GC_MASK(character) & U_GC_M_MASK) {
                int markLength = clusterLength;
                const UChar* markCharactersEnd = iterator.characters() + clusterLength;
                while (markCharactersEnd < normalizedBufferEnd) {
                    UChar32 nextCharacter;
                    int nextCharacterLength = 0;
                    U16_NEXT(markCharactersEnd, nextCharacterLength, normalizedBufferEnd - markCharactersEnd, nextCharacter);
                    if (!(U_GET_GC_MASK(nextCharacter) & U_GC_M_MASK))
                        break;
                    markLength += nextCharacterLength;
                    markCharactersEnd += nextCharacterLength;
                }
                if (currentFontData->canRenderCombiningCharacterSequence(currentCharacterPosition, markCharactersEnd - currentCharacterPosition)) {
                    clusterLength = markLength;
                    continue;
                }
                nextFontData = m_font->glyphDataForCharacter(character, false).fontData;
            } else
                nextFontData = m_font->glyphDataForCharacter(character, false).fontData;

            nextScript = uscript_getScript(character, &errorCode);
            if (U_FAILURE(errorCode))
                return false;
            if ((nextFontData != currentFontData) || ((currentScript != nextScript) && (nextScript != USCRIPT_INHERITED)))
                break;
            if (nextScript == USCRIPT_INHERITED)
                nextScript = currentScript;
            currentCharacterPosition = iterator.characters();
        }
        unsigned numCharactersOfCurrentRun = iterator.currentCharacter() - startIndexOfCurrentRun;
        m_worldtypeRuns.append(WorldTypeRun::create(currentFontData, startIndexOfCurrentRun, numCharactersOfCurrentRun, m_run.direction(), currentScript));
        currentFontData = nextFontData;
        startIndexOfCurrentRun = iterator.currentCharacter();
    } while (iterator.consume(character, clusterLength));

    return !m_worldtypeRuns.isEmpty();
}

bool WorldTypeShaper::shapeWorldTypeRuns()
{
    WorldTypeScopedPtr<TsShaperText> worldtypeBuffer(TsShaperText_new(0), TsShaperText_delete);

    for (unsigned i = 0; i < m_worldtypeRuns.size(); ++i) {
        unsigned runIndex = m_run.rtl() ? m_worldtypeRuns.size() - i - 1 : i;
        WorldTypeRun* currentRun = m_worldtypeRuns[runIndex].get();
        const SimpleFontData* currentFontData = currentRun->fontData();

        if (TsShaperText_init(worldtypeBuffer.get(), currentRun->numCharacters()) != TS_OK)
            return false;

        memset(worldtypeBuffer.get()->textInfoArray, 0, sizeof(TsTextProcessInfo) * currentRun->numCharacters());

        int index = 0;
        const UChar* firstCodeUnit;
        if (m_font->isSmallCaps() && u_islower(m_normalizedBuffer[currentRun->startIndex()])) {
            String upperText = String(m_normalizedBuffer.get() + currentRun->startIndex(), currentRun->numCharacters());
            upperText.makeUpper();
            currentFontData = m_font->glyphDataForCharacter(upperText[0], false, SmallCapsVariant).fontData;
            firstCodeUnit = upperText.characters();
        } else
            firstCodeUnit = m_normalizedBuffer.get() + currentRun->startIndex();

        const UChar* currentCodeUnit = firstCodeUnit;
        unsigned codeUnitsLeft = currentRun->numCharacters();
        while (codeUnitsLeft) {
            TsLength consumed;
            TsUInt32 character = TsUniEnc_decodeUTF16(const_cast<TsUInt16*>(currentCodeUnit), codeUnitsLeft, &consumed);
            currentCodeUnit += consumed;
            codeUnitsLeft -= consumed;
            if (character == 0xFFFD && consumed == 1) // not enough code points left or bad surrogate
                continue;

            TsTextProcessInfo* info = worldtypeBuffer.get()->textInfoArray + index++;
            info->character = character;
            info->sourceIndex = currentCodeUnit - firstCodeUnit - consumed;
        }

        worldtypeBuffer.get()->textInfoArrayLen = index;

        FontPlatformData* platformData = const_cast<FontPlatformData*>(&currentFontData->platformData());
        WorldTypeFace* face = platformData->worldtypeFace();
        if (!face)
            return false;

        TsBool bidiNeeded = false;
        TsUInt8 baseLevel;
        if (m_run.rtl() || m_run.directionalOverride()) {
            if (TsShaper_bidiSetLevels(face->shaper(), worldtypeBuffer.get(), 0, currentRun->numCharacters() - 1, &baseLevel, &bidiNeeded, m_run.rtl() ? TS_RTL_DIRECTION : TS_LTR_DIRECTION) != TS_OK)
                // FIXME: bidiSetOverride?
                return false;
        }

        TsInt32 growth;
        if (TsShaper_markBoundaries(face->shaper(), worldtypeBuffer.get(), 0, 0, currentRun->numCharacters() - 1, &growth) != TS_OK) // FIXME: Thai dictionary
            return false;

        if (bidiNeeded && TsShaper_bidiMirrorChars(face->shaper(), worldtypeBuffer.get(), 0, currentRun->numCharacters() + growth - 1) != TS_OK) // FIXME: should we just ASSERT(!bidiNeeded)?
            return false;

        if (TsShaper_optionsSetLanguage(face->shaper(), uScriptCodeToWorldTypeLanguage(currentRun->script())) != TS_OK)
            return false;

        WorldTypeScopedPtr<TsShaperFont> worldtypeFont(face->shaperFont(), TsShaperFont_releaseHandle);
        // FIXME: handle m_features somehow
        if (TsShaper_shape(face->shaper(), worldtypeFont.get(), worldtypeBuffer.get(), 0, currentRun->numCharacters() + growth - 1, 0, &growth) != TS_OK)
            return false;

        if (bidiNeeded && TsShaper_bidiReorder(face->shaper(), worldtypeBuffer.get(), 0, currentRun->numCharacters() + growth - 1, baseLevel) != TS_OK)
            return false;

        currentRun->applyShapeResult(worldtypeBuffer.get());
        setGlyphPositionsForWorldTypeRun(currentRun, worldtypeBuffer.get());

        TsShaperText_done(worldtypeBuffer.get());
    }
    return true;
}

void WorldTypeShaper::setGlyphPositionsForWorldTypeRun(WorldTypeRun* currentRun, TsShaperText* worldtypeBuffer)
{
    const SimpleFontData* currentFontData = currentRun->fontData();

    unsigned numGlyphs = currentRun->numGlyphs();
    uint16_t* glyphToCharacterIndexes = currentRun->glyphToCharacterIndexes();
    float totalAdvance = 0;

    // WorldType returns the shaping result in visual order. We need not to flip for RTL.
    for (size_t i = 0; i < numGlyphs; ++i) {
        bool runEnd = i + 1 == numGlyphs;
        uint16_t glyph = worldtypeBuffer->textInfoArray[i].glyphID;
        float offsetX = worldtypePositionToFloat(worldtypeBuffer->textInfoArray[i].posDelta.u.place.xPlacement);
        float offsetY = -worldtypePositionToFloat(worldtypeBuffer->textInfoArray[i].posDelta.u.place.yPlacement);
        float advance = currentFontData->platformWidthForGlyph(glyph);

        unsigned currentCharacterIndex = worldtypeBuffer->textInfoArray[i].sourceIndex;
        bool isClusterEnd = runEnd || worldtypeBuffer->textInfoArray[i].sourceIndex != worldtypeBuffer->textInfoArray[i + 1].sourceIndex;
        float spacing = 0;

        glyphToCharacterIndexes[i] = worldtypeBuffer->textInfoArray[i].sourceIndex;

        bool treatAsZeroWidthSpace = Font::treatAsZeroWidthSpace(m_normalizedBuffer[currentCharacterIndex]);
        if (treatAsZeroWidthSpace && !Font::treatAsSpace(m_normalizedBuffer[currentCharacterIndex])) {
            currentRun->setGlyphAndPositions(i, currentFontData->spaceGlyph(), 0, offsetX, offsetY);
            continue;
        }

        if (isClusterEnd && !treatAsZeroWidthSpace)
            spacing += m_letterSpacing;

        if (isClusterEnd && isWordEnd(currentCharacterIndex))
            spacing += determineWordBreakSpacing();

        if (currentFontData->isZeroWidthSpaceGlyph(glyph)) {
            currentRun->setGlyphAndPositions(i, glyph, 0, 0, 0);
            continue;
        }

        advance += spacing;
        if (m_run.rtl()) {
            // In RTL, spacing should be added to left side of glyphs.
            offsetX += spacing;
            if (!isClusterEnd)
                offsetX += m_letterSpacing;
        }

        currentRun->setGlyphAndPositions(i, glyph, advance, offsetX, offsetY);

        totalAdvance += advance;
    }
    currentRun->setWidth(totalAdvance > 0.0 ? totalAdvance : 0.0);
    m_totalWidth += currentRun->width();
}

void WorldTypeShaper::fillGlyphBufferFromWorldTypeRun(GlyphBuffer* glyphBuffer, WorldTypeRun* currentRun, FloatPoint& firstOffsetOfNextRun)
{
    FloatPoint* offsets = currentRun->offsets();
    uint16_t* glyphs = currentRun->glyphs();
    float* advances = currentRun->advances();
    unsigned numGlyphs = currentRun->numGlyphs();
    uint16_t* glyphToCharacterIndexes = currentRun->glyphToCharacterIndexes();

    for (unsigned i = 0; i < numGlyphs; ++i) {
        uint16_t currentCharacterIndex = currentRun->startIndex() + glyphToCharacterIndexes[i];
        FloatPoint& currentOffset = offsets[i];
        FloatPoint& nextOffset = (i == numGlyphs - 1) ? firstOffsetOfNextRun : offsets[i + 1];
        float glyphAdvanceX = advances[i] + nextOffset.x() - currentOffset.x();
        float glyphAdvanceY = nextOffset.y() - currentOffset.y();
        if (m_run.rtl()) {
            if (currentCharacterIndex > m_toIndex)
                m_startOffset.move(glyphAdvanceX, glyphAdvanceY);
            else if (currentCharacterIndex >= m_fromIndex)
                glyphBuffer->add(glyphs[i], currentRun->fontData(), createGlyphBufferAdvance(glyphAdvanceX, glyphAdvanceY));
        } else {
            if (currentCharacterIndex < m_fromIndex)
                m_startOffset.move(glyphAdvanceX, glyphAdvanceY);
            else if (currentCharacterIndex < m_toIndex)
                glyphBuffer->add(glyphs[i], currentRun->fontData(), createGlyphBufferAdvance(glyphAdvanceX, glyphAdvanceY));
        }
    }
}

void WorldTypeShaper::fillGlyphBuffer(GlyphBuffer* glyphBuffer)
{
    unsigned numRuns = m_worldtypeRuns.size();
    if (m_run.rtl()) {
        m_startOffset = m_worldtypeRuns.last()->offsets()[0];
        for (int runIndex = numRuns - 1; runIndex >= 0; --runIndex) {
            WorldTypeRun* currentRun = m_worldtypeRuns[runIndex].get();
            FloatPoint firstOffsetOfNextRun = !runIndex ? FloatPoint() : m_worldtypeRuns[runIndex - 1]->offsets()[0];
            fillGlyphBufferFromWorldTypeRun(glyphBuffer, currentRun, firstOffsetOfNextRun);
        }
    } else {
        m_startOffset = m_worldtypeRuns.first()->offsets()[0];
        for (unsigned runIndex = 0; runIndex < numRuns; ++runIndex) {
            WorldTypeRun* currentRun = m_worldtypeRuns[runIndex].get();
            FloatPoint firstOffsetOfNextRun = runIndex == numRuns - 1 ? FloatPoint() : m_worldtypeRuns[runIndex + 1]->offsets()[0];
            fillGlyphBufferFromWorldTypeRun(glyphBuffer, currentRun, firstOffsetOfNextRun);
        }
    }
}

int WorldTypeShaper::offsetForPosition(float targetX)
{
    int charactersSoFar = 0;
    float currentX = 0;

    if (m_run.rtl()) {
        charactersSoFar = m_normalizedBufferLength;
        for (int i = m_worldtypeRuns.size() - 1; i >= 0; --i) {
            charactersSoFar -= m_worldtypeRuns[i]->numCharacters();
            float nextX = currentX + m_worldtypeRuns[i]->width();
            float offsetForRun = targetX - currentX;
            if (offsetForRun >= 0 && offsetForRun <= m_worldtypeRuns[i]->width()) {
                // The x value in question is within this script run.
                const unsigned index = m_worldtypeRuns[i]->characterIndexForXPosition(offsetForRun);
                return charactersSoFar + index;
            }
            currentX = nextX;
        }
    } else {
        for (unsigned i = 0; i < m_worldtypeRuns.size(); ++i) {
            float nextX = currentX + m_worldtypeRuns[i]->width();
            float offsetForRun = targetX - currentX;
            if (offsetForRun >= 0 && offsetForRun <= m_worldtypeRuns[i]->width()) {
                const unsigned index = m_worldtypeRuns[i]->characterIndexForXPosition(offsetForRun);
                return charactersSoFar + index;
            }
            charactersSoFar += m_worldtypeRuns[i]->numCharacters();
            currentX = nextX;
        }
    }

    return charactersSoFar;
}

FloatRect WorldTypeShaper::selectionRect(const FloatPoint& point, int height, int from, int to)
{
    float currentX = 0;
    float fromX = 0;
    float toX = 0;
    bool foundFromX = false;
    bool foundToX = false;

    if (m_run.rtl())
        currentX = m_totalWidth;
    for (unsigned i = 0; i < m_worldtypeRuns.size(); ++i) {
        if (m_run.rtl())
            currentX -= m_worldtypeRuns[i]->width();
        int numCharacters = m_worldtypeRuns[i]->numCharacters();
        if (!foundFromX && from >= 0 && from < numCharacters) {
            fromX = m_worldtypeRuns[i]->xPositionForOffset(from) + currentX;
            foundFromX = true;
        } else
            from -= numCharacters;

        if (!foundToX && to >= 0 && to < numCharacters) {
            toX = m_worldtypeRuns[i]->xPositionForOffset(to) + currentX;
            foundToX = true;
        } else
            to -= numCharacters;

        if (foundFromX && foundToX)
            break;
        if (!m_run.rtl())
            currentX += m_worldtypeRuns[i]->width();
    }

    // The position in question might be just after the text.
    if (!foundFromX)
        fromX = 0;
    if (!foundToX)
        toX = m_run.rtl() ? 0 : m_totalWidth;

    // Using floorf() and roundf() as the same as mac port.
    if (fromX < toX)
        return FloatRect(floorf(point.x() + fromX), point.y(), roundf(toX - fromX), height);
    return FloatRect(floorf(point.x() + toX), point.y(), roundf(fromX - toX), height);
}

} // namespace WebCore
