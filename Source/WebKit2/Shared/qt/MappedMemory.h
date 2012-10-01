/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2010 University of Szeged
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef MappedMemory_h
#define MappedMemory_h

#include <QCoreApplication>
#include <QFile>
#include <wtf/Vector.h>

namespace WebKit {
struct MappedMemory {
    QFile* file;
    struct Data {
        uint32_t isFree; // keep bytes aligned
        uchar bytes;
    };
    union {
        uchar* mappedBytes;
        Data* dataPtr;
    };
    size_t dataSize;

    size_t mapSize() const { return dataSize + sizeof(Data); }
    void markUsed() { dataPtr->isFree = false; }
    void markFree() { dataPtr->isFree = true; }
    bool isFree() const { return dataPtr->isFree; }
    uchar* data() const { return &dataPtr->bytes; }
};

class MappedMemoryPool : public QObject {
    Q_OBJECT
public:
    static MappedMemoryPool* instance();
    size_t size() const;
    MappedMemory& at(size_t i);
    MappedMemory& append(const MappedMemory&);

private:
    MappedMemoryPool();
    Q_SLOT void cleanUp();

    Vector<MappedMemory> m_pool;
};
} // namespace WebKit
#endif // MappedMemory_h
