/*
 * Copyright (C) 2013 BlackBerry Limited. All rights reserved.
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

#ifndef MemoryBucket_h
#define MemoryBucket_h

#if USE(ACCELERATED_COMPOSITING)

#include <wtf/FastMalloc.h>
#include <wtf/Vector.h>

namespace WebCore {

// A memory bucket is created to work as a secondary stack. Allowing (near) instantaneous allocation
// of objects. The benefit of using a MemoryBucket over normal stack allocation is that the contents of the
// bucket is not removed when it goes out of scope! Note that MemoryBucket never runs any destructors, so
// the objects created using MemoryBucket must not require destructors.
class MemoryBucket {
public:
    MemoryBucket(size_t memoryToAllocate = 0)
        : m_size(memoryToAllocate)
    {
        m_allocatedMemoryStartPointer = (char*)fastMalloc(m_size);
        m_currentPositionPointer = m_allocatedMemoryStartPointer;
        m_allocatedMemoryEndPointer = m_allocatedMemoryStartPointer + m_size;
    }

    ~MemoryBucket()
    {
        clear();
        fastFree(m_allocatedMemoryStartPointer);
    }

    void reserveCapacity(size_t memoryToAllocate)
    {
        grow(memoryToAllocate);
    }

    template<class T>
    T* createObject()
    {
        growIfNeeded(sizeof(T));

        T* retPtr = new(m_currentPositionPointer) T();
        m_currentPositionPointer += sizeof(T);
        return retPtr;
    }

    template<class T>
    T* createObject(const T* object)
    {
        growIfNeeded(sizeof(T));

        T* retPtr = new(m_currentPositionPointer) T(*object);
        m_currentPositionPointer += sizeof(T);
        return retPtr;
    }

    template<class T>
    T* createObject(const T& object)
    {
        growIfNeeded(sizeof(T));

        T* retPtr = new(m_currentPositionPointer) T(object);
        m_currentPositionPointer += sizeof(T);
        return retPtr;
    }

    template<class T, typename InParameter>
    T* createObject(const InParameter& argument)
    {
        growIfNeeded(sizeof(T));

        T* retPtr = new(m_currentPositionPointer) T(argument);
        m_currentPositionPointer += sizeof(T);
        return retPtr;
    }

    template<class T>
    T* createArray(size_t arraySize)
    {
        growIfNeeded(sizeof(T) * arraySize);

        T* retPtr = new(m_currentPositionPointer) T[arraySize]();
        m_currentPositionPointer += sizeof(T) * arraySize;
        return retPtr;
    }

    void clear()
    {
        m_currentPositionPointer = m_allocatedMemoryStartPointer;
        for (size_t i = 0; i < m_oldBuckets.size(); ++i)
            fastFree(m_oldBuckets[i]);
        m_oldBuckets.clear();
    }

private:
    void growIfNeeded(size_t requiredSize)
    {
        // We create the new area (hopefully) large enough so we avoid "deleting" the buckets unnecessarily.
        if (m_currentPositionPointer + requiredSize > m_allocatedMemoryEndPointer)
            grow(m_size * 2 + requiredSize);
    }

    void grow(size_t memoryToAllocate)
    {
        // Add the "old" bucket. Note that we don't need to copy the contents over, the contents of old buckets
        // remain valid until the next clear().
        m_oldBuckets.append(m_allocatedMemoryStartPointer);

        m_size = memoryToAllocate;
        m_allocatedMemoryStartPointer = (char*)fastMalloc(m_size);
        m_currentPositionPointer = m_allocatedMemoryStartPointer;
        m_allocatedMemoryEndPointer = m_allocatedMemoryStartPointer + m_size;
    }

    char* m_allocatedMemoryStartPointer;
    char* m_currentPositionPointer;
    char* m_allocatedMemoryEndPointer;
    size_t m_size;

    Vector<char*> m_oldBuckets;
};

} // Webcore

#endif // USE(ACCELERATED_COMPOSITING)

#endif // MemoryBucket_h
