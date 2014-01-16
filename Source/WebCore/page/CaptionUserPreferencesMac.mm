/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#import "config.h"

#if ENABLE(VIDEO_TRACK)

#import "CaptionUserPreferencesMac.h"

#import "ColorMac.h"
#import "CoreText/CoreText.h"
#import "DOMWrapperWorld.h"
#import "FloatConversion.h"
#import "KURL.h"
#import "Language.h"
#import "LocalizedStrings.h"
#import "Logging.h"
#import "MediaControlElements.h"
#import "PageGroup.h"
#import "SoftLinking.h"
#import "TextTrackCue.h"
#import "UserStyleSheetTypes.h"
#import <wtf/RetainPtr.h>
#import <wtf/text/StringBuilder.h>

#if PLATFORM(IOS)
#import "WebCoreThreadRun.h"
#endif

#if HAVE(MEDIA_ACCESSIBILITY_FRAMEWORK)
#import "MediaAccessibility/MediaAccessibility.h"
#endif

#if HAVE(MEDIA_ACCESSIBILITY_FRAMEWORK)

SOFT_LINK_FRAMEWORK_OPTIONAL(MediaAccessibility)

SOFT_LINK(MediaAccessibility, MACaptionAppearanceGetShowCaptions, bool, (MACaptionAppearanceDomain domain), (domain))
SOFT_LINK(MediaAccessibility, MACaptionAppearanceSetShowCaptions, void, (MACaptionAppearanceDomain domain, bool showCaptions), (domain, showCaptions))
SOFT_LINK(MediaAccessibility, MACaptionAppearanceCopyForegroundColor, CGColorRef, (MACaptionAppearanceDomain domain, MACaptionAppearanceBehavior *behavior), (domain, behavior))
SOFT_LINK(MediaAccessibility, MACaptionAppearanceCopyBackgroundColor, CGColorRef, (MACaptionAppearanceDomain domain, MACaptionAppearanceBehavior *behavior), (domain, behavior))
SOFT_LINK(MediaAccessibility, MACaptionAppearanceCopyWindowColor, CGColorRef, (MACaptionAppearanceDomain domain, MACaptionAppearanceBehavior *behavior), (domain, behavior))
SOFT_LINK(MediaAccessibility, MACaptionAppearanceGetForegroundOpacity, CGFloat, (MACaptionAppearanceDomain domain, MACaptionAppearanceBehavior *behavior), (domain, behavior))
SOFT_LINK(MediaAccessibility, MACaptionAppearanceGetBackgroundOpacity, CGFloat, (MACaptionAppearanceDomain domain, MACaptionAppearanceBehavior *behavior), (domain, behavior))
SOFT_LINK(MediaAccessibility, MACaptionAppearanceGetWindowOpacity, CGFloat, (MACaptionAppearanceDomain domain, MACaptionAppearanceBehavior *behavior), (domain, behavior))
SOFT_LINK(MediaAccessibility, MACaptionAppearanceGetWindowRoundedCornerRadius, CGFloat, (MACaptionAppearanceDomain domain, MACaptionAppearanceBehavior *behavior), (domain, behavior))
SOFT_LINK(MediaAccessibility, MACaptionAppearanceCopyFontDescriptorForStyle, CTFontDescriptorRef, (MACaptionAppearanceDomain domain,  MACaptionAppearanceBehavior *behavior, MACaptionAppearanceFontStyle fontStyle), (domain, behavior, fontStyle))
SOFT_LINK(MediaAccessibility, MACaptionAppearanceGetRelativeCharacterSize, CGFloat, (MACaptionAppearanceDomain domain, MACaptionAppearanceBehavior *behavior), (domain, behavior))
SOFT_LINK(MediaAccessibility, MACaptionAppearanceGetTextEdgeStyle, MACaptionAppearanceTextEdgeStyle, (MACaptionAppearanceDomain domain, MACaptionAppearanceBehavior *behavior), (domain, behavior))
SOFT_LINK(MediaAccessibility, MACaptionAppearanceAddSelectedLanguage, bool, (MACaptionAppearanceDomain domain, CFStringRef language), (domain, language));
SOFT_LINK(MediaAccessibility, MACaptionAppearanceCopySelectedLanguages, CFArrayRef, (MACaptionAppearanceDomain domain), (domain));

SOFT_LINK_POINTER(MediaAccessibility, kMAXCaptionAppearanceSettingsChangedNotification, CFStringRef)
#define kMAXCaptionAppearanceSettingsChangedNotification getkMAXCaptionAppearanceSettingsChangedNotification()

#endif

using namespace std;

namespace WebCore {

#if HAVE(MEDIA_ACCESSIBILITY_FRAMEWORK)
static void userCaptionPreferencesChangedNotificationCallback(CFNotificationCenterRef, void* observer, CFStringRef, const void *, CFDictionaryRef)
{
#if !PLATFORM(IOS)
    static_cast<CaptionUserPreferencesMac*>(observer)->captionPreferencesChanged();
#else
    WebThreadRun(^{
        static_cast<CaptionUserPreferencesMac*>(observer)->captionPreferencesChanged();
    });
#endif
}
#endif

CaptionUserPreferencesMac::CaptionUserPreferencesMac(PageGroup* group)
    : CaptionUserPreferences(group)
#if HAVE(MEDIA_ACCESSIBILITY_FRAMEWORK)
    , m_listeningForPreferenceChanges(false)
#endif
{
}

CaptionUserPreferencesMac::~CaptionUserPreferencesMac()
{
#if HAVE(MEDIA_ACCESSIBILITY_FRAMEWORK)
    if (kMAXCaptionAppearanceSettingsChangedNotification)
        CFNotificationCenterRemoveObserver(CFNotificationCenterGetLocalCenter(), this, kMAXCaptionAppearanceSettingsChangedNotification, NULL);
#endif
}

#if HAVE(MEDIA_ACCESSIBILITY_FRAMEWORK)
bool CaptionUserPreferencesMac::userPrefersCaptions() const
{
    if (testingMode() || !MediaAccessibilityLibrary())
        return CaptionUserPreferences::userPrefersCaptions();

    return MACaptionAppearanceGetShowCaptions(kMACaptionAppearanceDomainUser);
}

void CaptionUserPreferencesMac::setUserPrefersCaptions(bool preference)
{
    if (testingMode() || !MediaAccessibilityLibrary()) {
        CaptionUserPreferences::setUserPrefersCaptions(preference);
        return;
    }

    MACaptionAppearanceSetShowCaptions(kMACaptionAppearanceDomainUser, preference);
}

bool CaptionUserPreferencesMac::userHasCaptionPreferences() const
{
    if (testingMode() || !MediaAccessibilityLibrary())
        return CaptionUserPreferences::userHasCaptionPreferences();

    return true;
}

void CaptionUserPreferencesMac::setInterestedInCaptionPreferenceChanges()
{
    if (!MediaAccessibilityLibrary())
        return;

    if (!kMAXCaptionAppearanceSettingsChangedNotification)
        return;

    if (!m_listeningForPreferenceChanges) {
        m_listeningForPreferenceChanges = true;
        CFNotificationCenterAddObserver(CFNotificationCenterGetLocalCenter(), this, userCaptionPreferencesChangedNotificationCallback, kMAXCaptionAppearanceSettingsChangedNotification, NULL, CFNotificationSuspensionBehaviorCoalesce);
    }
    
    updateCaptionStyleSheetOveride();
}

void CaptionUserPreferencesMac::captionPreferencesChanged()
{
    if (m_listeningForPreferenceChanges)
        updateCaptionStyleSheetOveride();

    CaptionUserPreferences::captionPreferencesChanged();
}

String CaptionUserPreferencesMac::captionsWindowCSS() const
{
    MACaptionAppearanceBehavior behavior;
    RetainPtr<CGColorRef> color(AdoptCF, MACaptionAppearanceCopyWindowColor(kMACaptionAppearanceDomainUser, &behavior));

    Color windowColor(color.get());
    if (!windowColor.isValid())
        windowColor = Color::transparent;

    bool important = behavior == kMACaptionAppearanceBehaviorUseValue;
    CGFloat opacity = MACaptionAppearanceGetWindowOpacity(kMACaptionAppearanceDomainUser, &behavior);
    if (!important)
        important = behavior == kMACaptionAppearanceBehaviorUseValue;
    String windowStyle = colorPropertyCSS(CSSPropertyBackgroundColor, Color(windowColor.red(), windowColor.green(), windowColor.blue(), static_cast<int>(opacity * 255)), important);

    if (!opacity)
        return windowStyle;

    StringBuilder builder;
    builder.append(windowStyle);
    builder.append(getPropertyNameString(CSSPropertyPadding));
    builder.append(": .4em !important;");
    
    return builder.toString();
}

String CaptionUserPreferencesMac::captionsBackgroundCSS() const
{
    // This default value must be the same as the one specified in mediaControls.css for -webkit-media-text-track-past-nodes
    // and webkit-media-text-track-future-nodes.
    DEFINE_STATIC_LOCAL(Color, defaultBackgroundColor, (Color(0, 0, 0, 0.8 * 255)));

    MACaptionAppearanceBehavior behavior;

    RetainPtr<CGColorRef> color(AdoptCF, MACaptionAppearanceCopyBackgroundColor(kMACaptionAppearanceDomainUser, &behavior));
    Color backgroundColor(color.get());
    if (!backgroundColor.isValid())
        backgroundColor = defaultBackgroundColor;

    bool important = behavior == kMACaptionAppearanceBehaviorUseValue;
    CGFloat opacity = MACaptionAppearanceGetBackgroundOpacity(kMACaptionAppearanceDomainUser, 0);
    if (!important)
        important = behavior == kMACaptionAppearanceBehaviorUseValue;
    String backgroundStyle = colorPropertyCSS(CSSPropertyBackgroundColor, Color(backgroundColor.red(), backgroundColor.green(), backgroundColor.blue(), static_cast<int>(opacity * 255)), important);

    if (!opacity)
        return backgroundStyle;
    
    StringBuilder builder;
    builder.append(backgroundStyle);
    builder.append(getPropertyNameString(CSSPropertyPadding));
    builder.append(": 0px");
    if (behavior == kMACaptionAppearanceBehaviorUseValue)
        builder.append(" !important");
    builder.append(';');
    
    return builder.toString();
}

Color CaptionUserPreferencesMac::captionsTextColor(bool& important) const
{
    MACaptionAppearanceBehavior behavior;
    RetainPtr<CGColorRef> color(AdoptCF, MACaptionAppearanceCopyForegroundColor(kMACaptionAppearanceDomainUser, &behavior));
    Color textColor(color.get());
    if (!textColor.isValid())
        // This default value must be the same as the one specified in mediaControls.css for -webkit-media-text-track-container.
        textColor = Color::white;
    
    important = behavior == kMACaptionAppearanceBehaviorUseValue;
    CGFloat opacity = MACaptionAppearanceGetForegroundOpacity(kMACaptionAppearanceDomainUser, &behavior);
    if (!important)
        important = behavior == kMACaptionAppearanceBehaviorUseValue;
    return Color(textColor.red(), textColor.green(), textColor.blue(), static_cast<int>(opacity * 255));
}
    
String CaptionUserPreferencesMac::captionsTextColorCSS() const
{
    bool important;
    Color textColor = captionsTextColor(important);

    if (!textColor.isValid())
        return emptyString();

    return colorPropertyCSS(CSSPropertyColor, textColor, important);
}
    
String CaptionUserPreferencesMac::windowRoundedCornerRadiusCSS() const
{
    MACaptionAppearanceBehavior behavior;
    CGFloat radius = MACaptionAppearanceGetWindowRoundedCornerRadius(kMACaptionAppearanceDomainUser, &behavior);
    if (!radius)
        return emptyString();

    StringBuilder builder;
    builder.append(getPropertyNameString(CSSPropertyBorderRadius));
    builder.append(String::format(":%.02fpx", radius));
    if (behavior == kMACaptionAppearanceBehaviorUseValue)
        builder.append(" !important");
    builder.append(';');

    return builder.toString();
}
    
Color CaptionUserPreferencesMac::captionsEdgeColorForTextColor(const Color& textColor) const
{
    int distanceFromWhite = differenceSquared(textColor, Color::white);
    int distanceFromBlack = differenceSquared(textColor, Color::black);
    
    if (distanceFromWhite < distanceFromBlack)
        return textColor.dark();
    
    return textColor.light();
}

String CaptionUserPreferencesMac::cssPropertyWithTextEdgeColor(CSSPropertyID id, const String& value, const Color& textColor, bool important) const
{
    StringBuilder builder;
    
    builder.append(getPropertyNameString(id));
    builder.append(':');
    builder.append(value);
    builder.append(' ');
    builder.append(captionsEdgeColorForTextColor(textColor).serialized());
    if (important)
        builder.append(" !important");
    builder.append(';');
    
    return builder.toString();
}

String CaptionUserPreferencesMac::colorPropertyCSS(CSSPropertyID id, const Color& color, bool important) const
{
    StringBuilder builder;
    
    builder.append(getPropertyNameString(id));
    builder.append(':');
    builder.append(color.serialized());
    if (important)
        builder.append(" !important");
    builder.append(';');
    
    return builder.toString();
}

String CaptionUserPreferencesMac::captionsTextEdgeCSS() const
{
    DEFINE_STATIC_LOCAL(const String, edgeStyleRaised, (" -.05em -.05em 0 ", String::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(const String, edgeStyleDepressed, (" .05em .05em 0 ", String::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(const String, edgeStyleDropShadow, (" .075em .075em 0 ", String::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(const String, edgeStyleUniform, (" .03em ", String::ConstructFromLiteral));

    bool unused;
    Color color = captionsTextColor(unused);
    if (!color.isValid())
        color.setNamedColor("black");
    color = captionsEdgeColorForTextColor(color);

    MACaptionAppearanceBehavior behavior;
    MACaptionAppearanceTextEdgeStyle textEdgeStyle = MACaptionAppearanceGetTextEdgeStyle(kMACaptionAppearanceDomainUser, &behavior);
    switch (textEdgeStyle) {
        case kMACaptionAppearanceTextEdgeStyleUndefined:
        case kMACaptionAppearanceTextEdgeStyleNone:
            return emptyString();
            
        case kMACaptionAppearanceTextEdgeStyleRaised:
            return cssPropertyWithTextEdgeColor(CSSPropertyTextShadow, edgeStyleRaised, color, behavior == kMACaptionAppearanceBehaviorUseValue);
        case kMACaptionAppearanceTextEdgeStyleDepressed:
            return cssPropertyWithTextEdgeColor(CSSPropertyTextShadow, edgeStyleDepressed, color, behavior == kMACaptionAppearanceBehaviorUseValue);
        case kMACaptionAppearanceTextEdgeStyleDropShadow:
            return cssPropertyWithTextEdgeColor(CSSPropertyTextShadow, edgeStyleDropShadow, color, behavior == kMACaptionAppearanceBehaviorUseValue);
        case kMACaptionAppearanceTextEdgeStyleUniform:
            return cssPropertyWithTextEdgeColor(CSSPropertyWebkitTextStroke, edgeStyleUniform, color, behavior == kMACaptionAppearanceBehaviorUseValue);
            
        default:
            ASSERT_NOT_REACHED();
            break;
    }
    
    return emptyString();
}

String CaptionUserPreferencesMac::captionsDefaultFontCSS() const
{
    MACaptionAppearanceBehavior behavior;
    
    RetainPtr<CTFontDescriptorRef> font(AdoptCF, MACaptionAppearanceCopyFontDescriptorForStyle(kMACaptionAppearanceDomainUser, &behavior, kMACaptionAppearanceFontStyleDefault));
    if (!font)
        return emptyString();

    RetainPtr<CFTypeRef> name(AdoptCF, CTFontDescriptorCopyAttribute(font.get(), kCTFontNameAttribute));
    if (!name)
        return emptyString();
    
    StringBuilder builder;
    
    builder.append(getPropertyNameString(CSSPropertyFontFamily));
    builder.append(": \"");
    builder.append(static_cast<CFStringRef>(name.get()));
    builder.append('"');
    if (behavior == kMACaptionAppearanceBehaviorUseValue)
        builder.append(" !important");
    builder.append(';');
    
    return builder.toString();
}

String CaptionUserPreferencesMac::captionsStyleSheetOverride() const
{
    if (testingMode() || !MediaAccessibilityLibrary())
        return CaptionUserPreferences::captionsStyleSheetOverride();

    StringBuilder captionsOverrideStyleSheet;

    String background = captionsBackgroundCSS();
    if (!background.isEmpty()) {
        captionsOverrideStyleSheet.append(" video::");
        captionsOverrideStyleSheet.append(TextTrackCue::cueShadowPseudoId());
        captionsOverrideStyleSheet.append('{');
        captionsOverrideStyleSheet.append(background);
        captionsOverrideStyleSheet.append('}');
    }

    String windowColor = captionsWindowCSS();
    String windowCornerRadius = windowRoundedCornerRadiusCSS();
    if (!windowColor.isEmpty() || !windowCornerRadius.isEmpty()) {
        captionsOverrideStyleSheet.append(" video::");
        captionsOverrideStyleSheet.append(TextTrackCueBox::textTrackCueBoxShadowPseudoId());
        captionsOverrideStyleSheet.append('{');

        if (!windowColor.isEmpty())
            captionsOverrideStyleSheet.append(windowColor);
        if (!windowCornerRadius.isEmpty())
            captionsOverrideStyleSheet.append(windowCornerRadius);

        captionsOverrideStyleSheet.append('}');
    }
    
    String captionsColor = captionsTextColorCSS();
    String edgeStyle = captionsTextEdgeCSS();
    String fontName = captionsDefaultFontCSS();
    if (!captionsColor.isEmpty() || !edgeStyle.isEmpty() || !fontName.isEmpty()) {
        captionsOverrideStyleSheet.append(" video::");
        captionsOverrideStyleSheet.append(MediaControlTextTrackContainerElement::textTrackContainerElementShadowPseudoId());
        captionsOverrideStyleSheet.append('{');

        if (!captionsColor.isEmpty())
            captionsOverrideStyleSheet.append(captionsColor);
        if (!edgeStyle.isEmpty())
            captionsOverrideStyleSheet.append(edgeStyle);
        if (!fontName.isEmpty())
            captionsOverrideStyleSheet.append(fontName);

        captionsOverrideStyleSheet.append('}');
    }
    
    LOG(Media, "CaptionUserPreferencesMac::captionsStyleSheetOverrideSetting sytle to:\n%s", captionsOverrideStyleSheet.toString().utf8().data());

    return captionsOverrideStyleSheet.toString();
}

float CaptionUserPreferencesMac::captionFontSizeScale(bool& important) const
{
    if (testingMode() || !MediaAccessibilityLibrary())
        return CaptionUserPreferences::captionFontSizeScale(important);

    MACaptionAppearanceBehavior behavior;
    CGFloat characterScale = CaptionUserPreferences::captionFontSizeScale(important);
    CGFloat scaleAdjustment = MACaptionAppearanceGetRelativeCharacterSize(kMACaptionAppearanceDomainUser, &behavior);

    if (!scaleAdjustment)
        return characterScale;

    important = behavior == kMACaptionAppearanceBehaviorUseValue;
#if defined(__LP64__) && __LP64__
    return narrowPrecisionToFloat(scaleAdjustment * characterScale);
#else
    return scaleAdjustment * characterScale;
#endif
}

void CaptionUserPreferencesMac::updateCaptionStyleSheetOveride()
{
    // Identify our override style sheet with a unique URL - a new scheme and a UUID.
    DEFINE_STATIC_LOCAL(KURL, captionsStyleSheetURL, (ParsedURLString, "user-captions-override:01F6AF12-C3B0-4F70-AF5E-A3E00234DC23"));
    
    pageGroup()->removeUserStyleSheetFromWorld(mainThreadNormalWorld(), captionsStyleSheetURL);
    
    String captionsOverrideStyleSheet = captionsStyleSheetOverride();
    if (captionsOverrideStyleSheet.isEmpty())
        return;
    
    pageGroup()->addUserStyleSheetToWorld(mainThreadNormalWorld(), captionsOverrideStyleSheet, captionsStyleSheetURL, Vector<String>(),
             Vector<String>(), InjectInAllFrames, UserStyleAuthorLevel, InjectInExistingDocuments);
}

void CaptionUserPreferencesMac::setPreferredLanguage(String language)
{
    if (testingMode() || !MediaAccessibilityLibrary()) {
        CaptionUserPreferences::setPreferredLanguage(language);
        return;
    }

    MACaptionAppearanceAddSelectedLanguage(kMACaptionAppearanceDomainUser, language.createCFString().get());
}

Vector<String> CaptionUserPreferencesMac::preferredLanguages() const
{
    if (testingMode() || !MediaAccessibilityLibrary())
        return CaptionUserPreferences::preferredLanguages();

    Vector<String> platformLanguages = platformUserPreferredLanguages();
    Vector<String> override = userPreferredLanguagesOverride();
    if (!override.isEmpty()) {
        if (platformLanguages.size() != override.size())
            return override;
        for (size_t i = 0; i < override.size(); i++) {
            if (override[i] != platformLanguages[i])
                return override;
        }
    }

    CFIndex languageCount = 0;
    RetainPtr<CFArrayRef> languages(AdoptCF, MACaptionAppearanceCopySelectedLanguages(kMACaptionAppearanceDomainUser));
    if (languages)
        languageCount = CFArrayGetCount(languages.get());

    if (!languageCount)
        return CaptionUserPreferences::preferredLanguages();

    Vector<String> userPreferredLanguages;
    userPreferredLanguages.reserveCapacity(languageCount + platformLanguages.size());
    for (CFIndex i = 0; i < languageCount; i++)
        userPreferredLanguages.append(static_cast<CFStringRef>(CFArrayGetValueAtIndex(languages.get(), i)));

    userPreferredLanguages.append(platformLanguages);

    return userPreferredLanguages;
}
#endif

String CaptionUserPreferencesMac::displayNameForTrack(TextTrack* track) const
{
    String label = track->label();
    String language = track->language();
    String preferredLanguage = defaultLanguage();
    StringBuilder displayName;

    if (label.isEmpty() && language.isEmpty()) {
        displayName.append(textTrackNoLabelText());
        return displayName.toString();
    }

    if (!label.isEmpty())
        displayName.append(label);

    AtomicString localeDisplayName = displayNameForLanguageLocale(language);
    if (!label.contains(localeDisplayName)) {
        if (displayName.length() > 0)
            displayName.append(" ");
        displayName.append(localeDisplayName);
    }

    if (track->kind() == track->captionsKeyword()) {
        if (track->isClosedCaptions())
            displayName.append(" CC");
        else
            displayName.append(" SDH");
    }

    return displayName.toString();
}

}

#endif // ENABLE(VIDEO_TRACK)
