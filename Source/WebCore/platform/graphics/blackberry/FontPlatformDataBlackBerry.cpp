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
#include "FontPlatformData.h"
#include "PlatformString.h"

namespace WebCore {

FontPlatformData::~FontPlatformData()
{
}

void FontPlatformData::platformDataInit(const FontPlatformData& source)
{
    m_font = source.m_font;
}

const FontPlatformData& FontPlatformData::platformDataAssign(const FontPlatformData& other)
{
    m_font = other.m_font;
    return *this;
}

bool FontPlatformData::platformIsEqual(const FontPlatformData& other) const
{
    return m_font == other.m_font;
}

#ifndef NDEBUG
String FontPlatformData::description() const
{
    return "WTLE Font Data";
}
#endif


}
