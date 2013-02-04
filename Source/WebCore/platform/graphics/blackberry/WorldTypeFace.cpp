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
#include <wtf/HashMap.h>

namespace WebCore {

// Though we have FontCache class, which provides the cache mechanism for
// WebKit's font objects, we also need additional caching layer for WorldType
// to reduce the memory consumption because TsShaperFont should be associated with
// underling font data (e.g. CTFontRef, FTFace).

class FaceCacheEntry : public RefCounted<FaceCacheEntry> {
public:
    static PassRefPtr<FaceCacheEntry> create(TsShaper* face)
    {
        ASSERT(face);
        return adoptRef(new FaceCacheEntry(face));
    }
    ~FaceCacheEntry()
    {
        TsShaper_delete(m_face);
    }

    TsShaper* face() { return m_face; }
    HashMap<uint32_t, uint16_t>* glyphCache() { return &m_glyphCache; }

private:
    explicit FaceCacheEntry(TsShaper* face)
        : m_face(face)
    { }

    TsShaper* m_face;
    HashMap<uint32_t, uint16_t> m_glyphCache;
};

typedef HashMap<uint64_t, RefPtr<FaceCacheEntry>, WTF::IntHash<uint64_t>, WTF::UnsignedWithZeroKeyHashTraits<uint64_t> > WorldTypeFaceCache;

static WorldTypeFaceCache* worldTypeFaceCache()
{
    DEFINE_STATIC_LOCAL(WorldTypeFaceCache, s_worldTypeFaceCache, ());
    return &s_worldTypeFaceCache;
}

WorldTypeFace::WorldTypeFace(FontPlatformData* platformData, uint64_t uniqueID)
    : m_platformData(platformData)
    , m_uniqueID(uniqueID)
{
    WorldTypeFaceCache::AddResult result = worldTypeFaceCache()->add(m_uniqueID, 0);
    if (result.isNewEntry)
        result.iterator->second = FaceCacheEntry::create(createShaper());
    result.iterator->second->ref();
    m_shaper = result.iterator->second->face();
    m_glyphCacheForFaceCacheEntry = result.iterator->second->glyphCache();
    m_shaperFont = createShaperFont();
}

WorldTypeFace::~WorldTypeFace()
{
    deleteShaperFont();

    WorldTypeFaceCache::iterator result = worldTypeFaceCache()->find(m_uniqueID);
    ASSERT(result != worldTypeFaceCache()->end());
    ASSERT(result.get()->second->refCount() > 1);
    result.get()->second->deref();
    if (result.get()->second->refCount() == 1)
        worldTypeFaceCache()->remove(m_uniqueID);
}

FontPlatformData* WorldTypeFace::platformData()
{
    return m_platformData;
}

TsShaper* WorldTypeFace::createShaper()
{
    TsShaper* shaper = TsShaper_new(0);
    ASSERT(shaper);
    TsShaper_bidiInit(shaper);
    return shaper;
}

TsShaper* WorldTypeFace::shaper()
{
    return m_shaper;
}

TsShaperFont* WorldTypeFace::shaperFont()
{
    return TsShaperFont_copyHandle(m_shaperFont);

}

} // namespace WebCore
