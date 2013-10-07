/*
 * Copyright (C) 2013 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"

#if HAVE(ACCESSIBILITY)

#include "AccessibilityObject.h"

#include "Node.h"
#include "RenderObject.h"
#include "RenderText.h"
#include "WebKitAccessibleWrapperBlackBerry.h"

#include <BlackBerryPlatformLog.h>

namespace WebCore {

bool AccessibilityObject::accessibilityIgnoreAttachment() const
{
    return false;
}

AccessibilityObjectInclusion AccessibilityObject::accessibilityPlatformIncludesObject() const
{
    // TODO: We need feedback regarding what the expected accessible tree should
    // contain. For now, we will defer almost 100% to WebCore Accessibility.

    if (roleValue() == UnknownRole)
        return IgnoreObject;

    AccessibilityObject* parent = parentObject();
    if (!parent)
        return DefaultBehavior;

    // Text widgets have extraneous StaticTextRole children. We want to expose the
    // text as being in the widget itself.
    if (parent->isPasswordField() || parent->isTextControl())
        return IgnoreObject;

    // We expose the slider as a single accessible widget.
    if (roleValue() == SliderThumbRole)
        return IgnoreObject;

    return DefaultBehavior;
}

bool AccessibilityObject::allowsTextRanges() const
{
    return WebKitAccessibleWrapperBlackBerry::isTextObject(this);
}

unsigned AccessibilityObject::getLengthForTextRange() const
{
    unsigned textLength = text().length();
    if (textLength)
        return textLength;

    Node* node = this->node();
    RenderObject* renderer = node ? node->renderer() : 0;
    if (renderer && renderer->isText()) {
        RenderText* renderText = toRenderText(renderer);
        textLength = renderText ? renderText->textLength() : 0;
    }

    if (!textLength && allowsTextRanges())
        textLength = textUnderElement().length();

    return textLength;
}

} // namespace WebCore

#endif // HAVE(ACCESSIBILITY)
