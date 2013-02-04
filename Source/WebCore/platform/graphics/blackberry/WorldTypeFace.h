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

#ifndef WorldTypeFace_h
#define WorldTypeFace_h

#include <tsshaperapi.h>

#include <wtf/HashMap.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class FontPlatformData;

class WorldTypeFace : public RefCounted<WorldTypeFace> {
public:
    static PassRefPtr<WorldTypeFace> create(FontPlatformData* platformData, uint64_t uniqueID)
    {
        return adoptRef(new WorldTypeFace(platformData, uniqueID));
    }
    ~WorldTypeFace();

    FontPlatformData* platformData();
    TsShaper* shaper();
    TsShaperFont* shaperFont();

private:
    WorldTypeFace(FontPlatformData*, uint64_t);

    TsShaper* createShaper();
    TsShaperFont* createShaperFont();
    void deleteShaperFont();

    FontPlatformData* m_platformData;
    uint64_t m_uniqueID;
    TsShaper* m_shaper;
    TsShaperFont* m_shaperFont;
    void* m_shaperFontData;
    WTF::HashMap<uint32_t, uint16_t>* m_glyphCacheForFaceCacheEntry;
};

}

#endif // WorldTypeFace_h
