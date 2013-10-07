/*
 * Copyright (C) 2013 Research In Motion Limited. All rights reserved.
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
 *     * Neither the name of Research In Motion Limited nor the names of its
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
#include "ITypeState.h"

#include "ITypeUtils.h"

namespace WebCore {

static FS_STATE* s_deletedValue = reinterpret_cast<FS_STATE*>(-1);

ITypeState::ITypeState()
    : m_parent(0)
    , m_state(0)
{ }

ITypeState::ITypeState(FS_STATE* parent)
    : m_parent(parent)
    , m_state(parent ? FS_new_client(parent, 0) : 0)
{ }

ITypeState::~ITypeState()
{
    if (m_state && m_state != s_deletedValue)
        FS_end_client(m_state);
}

ITypeState& ITypeState::operator=(const ITypeState& other)
{
    if (m_state == other.m_state)
        return *this;

    if (m_state && m_state != s_deletedValue)
        FS_end_client(m_state);
    m_state = other.m_parent ? FS_new_client(other.m_parent, 0) : 0;
    m_parent = other.m_parent;
    return *this;
}

bool ITypeState::operator==(const ITypeState& other) const
{
    if (m_state == other.m_state)
        return true;

    if (!m_state || m_state == s_deletedValue || !other.m_state || other.m_state == s_deletedValue)
        return false;

    return m_state->cur_sfnt == other.m_state->cur_sfnt;
}

ITypeState::operator FS_STATE*() const
{
    return m_state;
}

void ITypeState::clear()
{
    if (m_state && m_state != s_deletedValue) {
        FS_end_client(m_state);
        m_state = 0;
    }
    m_parent = 0;
}

bool ITypeState::isValid() const
{
    return m_state && m_state != s_deletedValue;
}

FS_STATE* ITypeState::state() const
{
    return m_state;
}

const ITypeState& ITypeState::deletedValue()
{
    static ITypeState s_deletedState;
    static bool s_initialized = 0;

    if (!s_initialized) {
        s_deletedState.m_parent = s_deletedValue;
        s_deletedState.m_state = s_deletedValue;
        s_initialized = 1;
    }

    return s_deletedState;
}

}
