/*
 * Copyright (C) 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Google Inc. All rights reserved.
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(VIDEO)
#include "MediaControlsBlackBerry.h"

#include "CaptionUserPreferences.h"
#include "Chrome.h"
#include "ChromeClient.h"
#include "DOMTokenList.h"
#include "ExceptionCodePlaceholder.h"
#include "FloatConversion.h"
#include "Frame.h"
#include "HTMLMediaElement.h"
#include "HTMLNames.h"
#include "HTMLTrackElement.h"
#include "MediaControlElements.h"
#include "MediaPlayerPrivateBlackBerry.h"
#include "MouseEvent.h"
#include "Page.h"
#include "PageGroup.h"
#include "RenderDeprecatedFlexibleBox.h"
#include "RenderSlider.h"
#include "RenderTheme.h"
#include "Settings.h"
#include "Text.h"
#include "TimeRanges.h"

#if ENABLE(VIDEO_TRACK)
#include "TextTrackCue.h"
#include "TextTrackList.h"
#endif

using namespace std;

namespace WebCore {

using namespace HTMLNames;

#if ENABLE(VIDEO_TRACK)
static const char* textTracksOffAttrValue = "-1"; // This must match HTMLMediaElement::textTracksOffIndex()
#endif

static MediaPlayerPrivate* getNativeMediaPlayer(Node* node)
{
    HTMLMediaElement* element = toParentMediaElement(node);
    if (element) {
        MediaPlayer* wkPlayer = element->player();
        if (wkPlayer) {
            MediaPlayerPrivate* bbPlayer = static_cast<MediaPlayerPrivate*>(wkPlayer->implementation());
            // Reject the null media player.
            if (bbPlayer && bbPlayer->platformMedia().type == PlatformMedia::QNXMediaPlayerType)
                return bbPlayer;
        }
    }
    return 0;
}

static const double s_pollTimeout = 1.0;

inline MediaControlButtonGroupContainerElement::MediaControlButtonGroupContainerElement(Document* document)
    : MediaControlDivElement(document, MediaControlsPanel)
{
}

PassRefPtr<MediaControlButtonGroupContainerElement> MediaControlButtonGroupContainerElement::create(Document* document)
{
    RefPtr<MediaControlButtonGroupContainerElement> element = adoptRef(new MediaControlButtonGroupContainerElement(document));
    return element.release();
}

const AtomicString& MediaControlButtonGroupContainerElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-button-group-container", AtomicString::ConstructFromLiteral));
    return id;
}

inline MediaControlTimeDisplayContainerElement::MediaControlTimeDisplayContainerElement(Document* document)
    : MediaControlDivElement(document, MediaControlsPanel)
{
}

PassRefPtr<MediaControlTimeDisplayContainerElement> MediaControlTimeDisplayContainerElement::create(Document* document)
{
    RefPtr<MediaControlTimeDisplayContainerElement> element = adoptRef(new MediaControlTimeDisplayContainerElement(document));
    return element.release();
}

const AtomicString& MediaControlTimeDisplayContainerElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-time-display-container", AtomicString::ConstructFromLiteral));
    return id;
}

MediaControlEmbeddedPanelElement::MediaControlEmbeddedPanelElement(Document* document)
    : MediaControlDivElement(document, MediaControlsPanel)
    , m_canBeDragged(false)
    , m_isBeingDragged(false)
    , m_isDisplayed(false)
    , m_opaque(true)
    , m_transitionTimer(this, &MediaControlEmbeddedPanelElement::transitionTimerFired)
{
}

PassRefPtr<MediaControlEmbeddedPanelElement> MediaControlEmbeddedPanelElement::create(Document* document)
{
    return adoptRef(new MediaControlEmbeddedPanelElement(document));
}

const AtomicString& MediaControlEmbeddedPanelElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-embedded-panel", AtomicString::ConstructFromLiteral));
    return id;
}

void MediaControlEmbeddedPanelElement::startDrag(const LayoutPoint& eventLocation)
{
    if (!m_canBeDragged)
        return;

    if (m_isBeingDragged)
        return;

    RenderObject* renderer = this->renderer();
    if (!renderer || !renderer->isBox())
        return;

    Frame* frame = document()->frame();
    if (!frame)
        return;

    m_lastDragEventLocation = eventLocation;

    frame->eventHandler()->setCapturingMouseEventsNode(this);

    m_isBeingDragged = true;
}

void MediaControlEmbeddedPanelElement::continueDrag(const LayoutPoint& eventLocation)
{
    if (!m_isBeingDragged)
        return;

    LayoutSize distanceDragged = eventLocation - m_lastDragEventLocation;
    m_cumulativeDragOffset.move(distanceDragged);
    m_lastDragEventLocation = eventLocation;
    setPosition(m_cumulativeDragOffset);
}

void MediaControlEmbeddedPanelElement::endDrag()
{
    if (!m_isBeingDragged)
        return;

    m_isBeingDragged = false;

    Frame* frame = document()->frame();
    if (!frame)
        return;

    frame->eventHandler()->setCapturingMouseEventsNode(0);
}

void MediaControlEmbeddedPanelElement::startTimer(double duration)
{
    stopTimer();

    // The timer is required to set the property display:'none' on the panel,
    // such that captions are correctly displayed at the bottom of the video
    // at the end of the fadeout transition.
    m_transitionTimer.startOneShot(duration);
}

void MediaControlEmbeddedPanelElement::stopTimer()
{
    if (m_transitionTimer.isActive())
        m_transitionTimer.stop();
}

void MediaControlEmbeddedPanelElement::transitionTimerFired(Timer<MediaControlEmbeddedPanelElement>*)
{
    if (!m_opaque)
        hide();
    else if (m_isDisplayed)
        show();
    stopTimer();
}

void MediaControlEmbeddedPanelElement::setPosition(const LayoutPoint& position)
{
    double left = position.x();
    double top = position.y();

    // Set the left and top to control the panel's position; this depends on it being absolute positioned.
    // Set the margin to zero since the position passed in will already include the effect of the margin.
    setInlineStyleProperty(CSSPropertyLeft, left, CSSPrimitiveValue::CSS_PX);
    setInlineStyleProperty(CSSPropertyTop, top, CSSPrimitiveValue::CSS_PX);
    setInlineStyleProperty(CSSPropertyMarginLeft, 0.0, CSSPrimitiveValue::CSS_PX);
    setInlineStyleProperty(CSSPropertyMarginTop, 0.0, CSSPrimitiveValue::CSS_PX);

    classList()->add("dragged", IGNORE_EXCEPTION);
}

void MediaControlEmbeddedPanelElement::resetPosition()
{
    removeInlineStyleProperty(CSSPropertyLeft);
    removeInlineStyleProperty(CSSPropertyTop);
    removeInlineStyleProperty(CSSPropertyMarginLeft);
    removeInlineStyleProperty(CSSPropertyMarginTop);

    classList()->remove("dragged", IGNORE_EXCEPTION);

    m_cumulativeDragOffset.setX(0);
    m_cumulativeDragOffset.setY(0);
}

void MediaControlEmbeddedPanelElement::makeOpaque()
{
    if (m_opaque)
        return;

    double duration = document()->page() ? document()->page()->theme()->mediaControlsFadeInDuration() : 0;

    setInlineStyleProperty(CSSPropertyWebkitTransitionProperty, CSSPropertyOpacity);
    setInlineStyleProperty(CSSPropertyWebkitTransitionDuration, duration, CSSPrimitiveValue::CSS_S);
    setInlineStyleProperty(CSSPropertyOpacity, 1.0, CSSPrimitiveValue::CSS_NUMBER);

    m_opaque = true;
    startTimer(duration);
}

void MediaControlEmbeddedPanelElement::makeTransparent()
{
    if (!m_opaque)
        return;

    double duration = document()->page() ? document()->page()->theme()->mediaControlsFadeOutDuration() : 0;

    setInlineStyleProperty(CSSPropertyWebkitTransitionProperty, CSSPropertyOpacity);
    setInlineStyleProperty(CSSPropertyWebkitTransitionDuration, duration, CSSPrimitiveValue::CSS_S);
    setInlineStyleProperty(CSSPropertyOpacity, 0.0, CSSPrimitiveValue::CSS_NUMBER);

    m_opaque = false;
    startTimer(duration);
}

void MediaControlEmbeddedPanelElement::defaultEventHandler(Event* event)
{
    MediaControlDivElement::defaultEventHandler(event);

    if (event->isMouseEvent()) {
        LayoutPoint location = static_cast<MouseEvent*>(event)->absoluteLocation();
        if (event->type() == eventNames().mousedownEvent && event->target() == this) {
            startDrag(location);
            event->setDefaultHandled();
        } else if (event->type() == eventNames().mousemoveEvent && m_isBeingDragged)
            continueDrag(location);
        else if (event->type() == eventNames().mouseupEvent && m_isBeingDragged) {
            continueDrag(location);
            endDrag();
            event->setDefaultHandled();
        }
    } else if (event->type() == eventNames().touchendEvent) {
        // The MediaControlEmbeddedTimelineElement::defaultEventHandler() can
        // sometimes miss the touchendEvent, so this is a safety to make sure
        // we don't go into an unresponsive state.
        mediaController()->endScrubbing();
    }
}

void MediaControlEmbeddedPanelElement::setCanBeDragged(bool canBeDragged)
{
    if (m_canBeDragged == canBeDragged)
        return;

    m_canBeDragged = canBeDragged;

    if (!canBeDragged)
        endDrag();
}

void MediaControlEmbeddedPanelElement::setIsDisplayed(bool isDisplayed)
{
    m_isDisplayed = isDisplayed;
}

inline MediaControlFullscreenTimeDisplayContainerElement::MediaControlFullscreenTimeDisplayContainerElement(Document* document)
    : MediaControlDivElement(document, MediaControlsPanel)
{
}

PassRefPtr<MediaControlFullscreenTimeDisplayContainerElement> MediaControlFullscreenTimeDisplayContainerElement::create(Document* document)
{
    RefPtr<MediaControlFullscreenTimeDisplayContainerElement> element = adoptRef(new MediaControlFullscreenTimeDisplayContainerElement(document));
    return element.release();
}

const AtomicString& MediaControlFullscreenTimeDisplayContainerElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-fullscreen-time-display-container", AtomicString::ConstructFromLiteral));
    return id;
}

inline MediaControlFullscreenButtonContainerElement::MediaControlFullscreenButtonContainerElement(Document* document)
    : MediaControlDivElement(document, MediaControlsPanel)
{
}

PassRefPtr<MediaControlFullscreenButtonContainerElement> MediaControlFullscreenButtonContainerElement::create(Document* document)
{
    RefPtr<MediaControlFullscreenButtonContainerElement> element = adoptRef(new MediaControlFullscreenButtonContainerElement(document));
    return element.release();
}

const AtomicString& MediaControlFullscreenButtonContainerElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-fullscreen-button-container", AtomicString::ConstructFromLiteral));
    return id;
}

inline MediaControlFullscreenButtonDividerElement::MediaControlFullscreenButtonDividerElement(Document* document)
    : MediaControlDivElement(document, MediaRewindButton)
{
}

PassRefPtr<MediaControlFullscreenButtonDividerElement> MediaControlFullscreenButtonDividerElement::create(Document* document)
{
    RefPtr<MediaControlFullscreenButtonDividerElement> element = adoptRef(new MediaControlFullscreenButtonDividerElement(document));
    return element.release();
}

const AtomicString& MediaControlFullscreenButtonDividerElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-fullscreen-button-divider", AtomicString::ConstructFromLiteral));
    return id;
}

inline MediaControlPlayButtonContainerElement::MediaControlPlayButtonContainerElement(Document* document)
    : MediaControlDivElement(document, MediaControlsPanel)
{
}

PassRefPtr<MediaControlPlayButtonContainerElement> MediaControlPlayButtonContainerElement::create(Document* document)
{
    RefPtr<MediaControlPlayButtonContainerElement> element = adoptRef(new MediaControlPlayButtonContainerElement(document));
    return element.release();
}

const AtomicString& MediaControlPlayButtonContainerElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-play-button-container", AtomicString::ConstructFromLiteral));
    return id;
}

inline MediaControlPlayOnButtonContainerElement::MediaControlPlayOnButtonContainerElement(Document* document)
    : MediaControlDivElement(document, MediaControlsPanel)
{
}

PassRefPtr<MediaControlPlayOnButtonContainerElement> MediaControlPlayOnButtonContainerElement::create(Document* document)
{
    RefPtr<MediaControlPlayOnButtonContainerElement> element = adoptRef(new MediaControlPlayOnButtonContainerElement(document));
    return element.release();
}

const AtomicString& MediaControlPlayOnButtonContainerElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-playon-button-container", AtomicString::ConstructFromLiteral));
    return id;
}

inline MediaControlPlaceholderElement::MediaControlPlaceholderElement(Document* document)
    : MediaControlDivElement(document, MediaControlsPanel)
{
}

PassRefPtr<MediaControlPlaceholderElement> MediaControlPlaceholderElement::create(Document* document)
{
    RefPtr<MediaControlPlaceholderElement> element = adoptRef(new MediaControlPlaceholderElement(document));
    return element.release();
}

const AtomicString& MediaControlPlaceholderElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-placeholder", AtomicString::ConstructFromLiteral));
    return id;
}

inline MediaControlFullscreenPlayButtonElement::MediaControlFullscreenPlayButtonElement(Document* document)
    : MediaControlInputElement(document, MediaPlayButton)
{
}

PassRefPtr<MediaControlFullscreenPlayButtonElement> MediaControlFullscreenPlayButtonElement::create(Document* document)
{
    RefPtr<MediaControlFullscreenPlayButtonElement> button = adoptRef(new MediaControlFullscreenPlayButtonElement(document));
    button->ensureUserAgentShadowRoot();
    button->setType("button");
    return button.release();
}

void MediaControlFullscreenPlayButtonElement::defaultEventHandler(Event* event)
{
    if (event->type() == eventNames().clickEvent) {
        if (mediaController()->canPlay())
            mediaController()->play();
        else
            mediaController()->pause();
        updateDisplayType();
        event->setDefaultHandled();
    }
    HTMLInputElement::defaultEventHandler(event);
}

void MediaControlFullscreenPlayButtonElement::updateDisplayType()
{
    setDisplayType(mediaController()->canPlay() ? MediaPlayButton : MediaPauseButton);
}

const AtomicString& MediaControlFullscreenPlayButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-fullscreen-play-button", AtomicString::ConstructFromLiteral));
    return id;
}

inline MediaControlFullscreenPlayOnButtonElement::MediaControlFullscreenPlayOnButtonElement(Document* document, MediaControls* controls)
    : MediaControlInputElement(document, MediaOverlayPlayButton)
    , m_controls(controls)
{
}

PassRefPtr<MediaControlFullscreenPlayOnButtonElement> MediaControlFullscreenPlayOnButtonElement::create(Document* document, MediaControls* controls)
{
    RefPtr<MediaControlFullscreenPlayOnButtonElement> button = adoptRef(new MediaControlFullscreenPlayOnButtonElement(document, controls));
    button->ensureUserAgentShadowRoot();
    button->setType("button");
    return button.release();
}

void MediaControlFullscreenPlayOnButtonElement::defaultEventHandler(Event* event)
{
    if (event->type() == eventNames().clickEvent) {
        MediaPlayerPrivate* bbPlayer = getNativeMediaPlayer(this);
        if (bbPlayer) {
            bool hasRemoteDisplays = bbPlayer->hasRemoteDisplays();
            if (!bbPlayer->isPresentationModeEnabled() || !hasRemoteDisplays) {
                // We are not in presentation mode. Make sure presentation mode is enabled.
                if (hasRemoteDisplays || bbPlayer->isMiracastSupported()) {
                    bbPlayer->setPresentationModeEnabled(true);
                    if (!hasRemoteDisplays) {
                        // No secondary display - show Miracast card.
                        ChromeClient* client = document()->page()->chrome()->client();
                        client->invokeCard(BlackBerry::Platform::String::fromAscii("application/vnd.rim.miracast.playon"));
                        static_cast<MediaControlsBlackBerry*>(m_controls)->setMiracastCardInvoked(true);
                    }
                }
            } else {
                // Exit presentation mode.
                bbPlayer->setPresentationModeEnabled(false);
            }
        }
        event->setDefaultHandled();
    }
    HTMLInputElement::defaultEventHandler(event);
}

const AtomicString& MediaControlFullscreenPlayOnButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-fullscreen-playon-button", AtomicString::ConstructFromLiteral));
    return id;
}

MediaControlToggleClosedCaptionsListButtonElement::MediaControlToggleClosedCaptionsListButtonElement(Document* document, MediaControls* controls)
    : MediaControlInputElement(document, MediaShowClosedCaptionsButton)
    , m_controls(controls)
{
}

PassRefPtr<MediaControlToggleClosedCaptionsListButtonElement> MediaControlToggleClosedCaptionsListButtonElement::create(Document* document, MediaControls* controls)
{
    ASSERT(controls);

    RefPtr<MediaControlToggleClosedCaptionsListButtonElement> button = adoptRef(new MediaControlToggleClosedCaptionsListButtonElement(document, controls));
    button->ensureUserAgentShadowRoot();
    button->setType("button");
    button->hide();
    return button.release();
}

void MediaControlToggleClosedCaptionsListButtonElement::updateDisplayType()
{
    mediaController()->hasClosedCaptions() ?  enable() : disable();
}

void MediaControlToggleClosedCaptionsListButtonElement::defaultEventHandler(Event* event)
{
    if (event->type() == eventNames().clickEvent) {
        // FIXME: It's not great that the shared code is dictating behavior of platform-specific
        // UI. Not all ports may want the closed captions button to toggle a list of tracks, so
        // we have to use #if.
        // https://bugs.webkit.org/show_bug.cgi?id=101877
        m_controls->toggleClosedCaptionTrackList();
        event->setDefaultHandled();
    }

    HTMLInputElement::defaultEventHandler(event);
}

const AtomicString& MediaControlToggleClosedCaptionsListButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-toggle-closed-captions-button", AtomicString::ConstructFromLiteral));
    return id;
}

MediaControlClosedCaptionsTrackListMenuElement::MediaControlClosedCaptionsTrackListMenuElement(Document* document, MediaControls* controls)
    : MediaControlDivElement(document, MediaClosedCaptionsTrackList)
    , m_controls(controls)
    , m_trackListHasChanged(true)
{
}

PassRefPtr<MediaControlClosedCaptionsTrackListMenuElement> MediaControlClosedCaptionsTrackListMenuElement::create(Document* document, MediaControls* controls)
{
    ASSERT(controls);
    RefPtr<MediaControlClosedCaptionsTrackListMenuElement> element = adoptRef(new MediaControlClosedCaptionsTrackListMenuElement(document, controls));
    return element.release();
}

void MediaControlClosedCaptionsTrackListMenuElement::defaultEventHandler(Event* event)
{
#if ENABLE(VIDEO_TRACK)
    if (event->type() == eventNames().clickEvent) {
        Node* target = event->target()->toNode();
        if (!target || !target->isElementNode())
            return;

        // When we created the elements in the track list, we gave them a custom
        // attribute representing the index in the HTMLMediaElement's list of tracks.
        // Check if the event target has such a custom element and, if so,
        // tell the HTMLMediaElement to enable that track.

        int trackIndex = trackListIndexForElement(toElement(target));
        if (trackIndex == HTMLMediaElement::textTracksIndexNotFound())
            return;

        HTMLMediaElement* mediaElement = toParentMediaElement(this);
        if (!mediaElement)
            return;

        mediaElement->toggleTrackAtIndex(trackIndex);

        // We've selected a track to display, so we can now close the menu.
        m_controls->toggleClosedCaptionTrackList();
        updateDisplay();
    }
    MediaControlDivElement::defaultEventHandler(event);
#endif
}

const AtomicString& MediaControlClosedCaptionsTrackListMenuElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-closed-captions-track-list", AtomicString::ConstructFromLiteral));
    return id;
}

void MediaControlClosedCaptionsTrackListMenuElement::updateDisplay()
{
#if ENABLE(VIDEO_TRACK)
    DEFINE_STATIC_LOCAL(AtomicString, selectedClassValue, ("selected", AtomicString::ConstructFromLiteral));

    if (!mediaController()->hasClosedCaptions())
        return;

    HTMLMediaElement* mediaElement = toParentMediaElement(this);
    if (!mediaElement)
        return;

    TextTrackList* trackList = mediaElement->textTracks();

    if (!trackList || !trackList->length())
        return;

    if (m_trackListHasChanged)
        rebuildTrackListMenu();

    bool captionsVisible = mediaElement->closedCaptionsVisible();
    bool haveSelectedTrack = false;
    RefPtr<Element> offItem;
    for (unsigned i = 0, length = m_menuItems.size(); i < length; ++i) {
        RefPtr<Element> trackItem = m_menuItems[i];
        int trackIndex = trackListIndexForElement(trackItem.get());
        if (trackIndex != HTMLMediaElement::textTracksIndexNotFound()) {
            if (trackIndex == HTMLMediaElement::textTracksOffIndex()) {
                offItem = trackItem;
                if (captionsVisible)
                    trackItem->classList()->remove(selectedClassValue, ASSERT_NO_EXCEPTION);
                else {
                    trackItem->classList()->add(selectedClassValue, ASSERT_NO_EXCEPTION);
                    haveSelectedTrack = true;
                }
            } else {
                TextTrack* track = trackList->item(trackIndex);
                if (!track)
                    continue;
                if (captionsVisible && track->mode() == TextTrack::showingKeyword()) {
                    trackItem->classList()->add(selectedClassValue, ASSERT_NO_EXCEPTION);
                    haveSelectedTrack = true;
                } else
                    trackItem->classList()->remove(selectedClassValue, ASSERT_NO_EXCEPTION);
            }
        }
    }
    if (!haveSelectedTrack && offItem)
        offItem->classList()->add(selectedClassValue, ASSERT_NO_EXCEPTION);
#endif
}

#if ENABLE(VIDEO_TRACK)
static void insertTextTrackMenuItemIntoSortedContainer(RefPtr<Element>& item, RefPtr<Element>& container)
{
    // The container will always have the "Off" entry already present and it
    // should remain at the start of the list.
    ASSERT(container->childNodeCount() > 0);
    ASSERT(item->childNodeCount() == 1); // Each item should have a single text node child for the label.
    String itemLabel = toText(item->firstChild())->wholeText();

    // This is an insertion sort :( However, there shouldn't be a horrible number of text track items.
    for (int i = 1, numChildNodes = container->childNodeCount(); i < numChildNodes; ++i) {
        Node* child = container->childNode(i);
        ASSERT(child->childNodeCount() == 1); // Each item should have a single text node child for the label.
        Node* textNode = child->firstChild();
        ASSERT(textNode); // For Klocwork
        if (textNode) {
            String childLabel = toText(textNode)->wholeText();
            if (codePointCompareLessThan(itemLabel, childLabel)) {
                container->insertBefore(item, child);
                return;
            }
        }
    }
    container->appendChild(item);
}
#endif

void MediaControlClosedCaptionsTrackListMenuElement::rebuildTrackListMenu()
{
#if ENABLE(VIDEO_TRACK)
    // Remove any existing content.
    removeChildren();
    m_menuItems.clear();

    m_trackListHasChanged = false;

    if (!mediaController()->hasClosedCaptions())
        return;

    HTMLMediaElement* mediaElement = toParentMediaElement(this);
    if (!mediaElement)
        return;

    TextTrackList* trackList = mediaElement->textTracks();

    if (!trackList || !trackList->length())
        return;

    Document* doc = document();
    CaptionUserPreferences* captionsUserPreferences = doc->page()->group().captionPreferences();

    RefPtr<Element> captionsMenuList = doc->createElement(ulTag, ASSERT_NO_EXCEPTION);

    RefPtr<Element> trackItem;

    trackItem = doc->createElement(liTag, ASSERT_NO_EXCEPTION);
    trackItem->appendChild(doc->createTextNode(textTrackOffText()));
    trackItem->setAttribute(trackIndexAttributeName(), textTracksOffAttrValue, ASSERT_NO_EXCEPTION);
    captionsMenuList->appendChild(trackItem);
    m_menuItems.append(trackItem);

    for (unsigned i = 0, length = trackList->length(); i < length; ++i) {
        TextTrack* track = trackList->item(i);

        if (track->readinessState() == TextTrack::FailedToLoad)
            continue;

        trackItem = doc->createElement(liTag, ASSERT_NO_EXCEPTION);

        // Add a custom attribute to the <li> element which will allow
        // us to easily associate the user tapping here with the
        // track. Since this list is rebuilt if the tracks change, we
        // should always be in sync.
        trackItem->setAttribute(trackIndexAttributeName(), String::number(i), ASSERT_NO_EXCEPTION);

        if (captionsUserPreferences)
            trackItem->appendChild(doc->createTextNode(captionsUserPreferences->displayNameForTrack(track)));
        else
            trackItem->appendChild(doc->createTextNode(track->label()));

        insertTextTrackMenuItemIntoSortedContainer(trackItem, captionsMenuList);
        m_menuItems.append(trackItem);
    }

    appendChild(captionsMenuList);

    updateDisplay();
#endif
}

inline MediaControlFullscreenFullscreenButtonElement::MediaControlFullscreenFullscreenButtonElement(Document* document)
    : MediaControlInputElement(document, MediaExitFullscreenButton)
{
}

PassRefPtr<MediaControlFullscreenFullscreenButtonElement> MediaControlFullscreenFullscreenButtonElement::create(Document* document)
{
    RefPtr<MediaControlFullscreenFullscreenButtonElement> button = adoptRef(new MediaControlFullscreenFullscreenButtonElement(document));
    button->ensureUserAgentShadowRoot();
    button->setType("button");
    button->hide();
    return button.release();
}

void MediaControlFullscreenFullscreenButtonElement::defaultEventHandler(Event* event)
{
    if (event->type() == eventNames().clickEvent) {
#if ENABLE(FULLSCREEN_API)
        // Only use the new full screen API if the fullScreenEnabled setting has 
        // been explicitly enabled. Otherwise, use the old fullscreen API. This
        // allows apps which embed a WebView to retain the existing full screen
        // video implementation without requiring them to implement their own full 
        // screen behavior.
        Settings* settings = document()->settings();
        if (settings && settings->fullScreenEnabled()) {
            if (document()->webkitIsFullScreen() && document()->webkitCurrentFullScreenElement() == toParentMediaElement(this))
                toParentMediaElement(this)->exitFullscreen();
            else
                document()->requestFullScreenForElement(toParentMediaElement(this), 0, Document::ExemptIFrameAllowFullScreenRequirement);
        } else
#endif
            mediaController()->enterFullscreen();
        event->setDefaultHandled();
    }
    HTMLInputElement::defaultEventHandler(event);
}

const AtomicString& MediaControlFullscreenFullscreenButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-fullscreen-fullscreen-button", AtomicString::ConstructFromLiteral));
    return id;
}

void MediaControlFullscreenFullscreenButtonElement::setIsFullscreen(bool)
{
    setDisplayType(MediaExitFullscreenButton);
}

inline MediaControlFullscreenTimelineContainerElement::MediaControlFullscreenTimelineContainerElement(Document* document)
    : MediaControlDivElement(document, MediaTimelineContainer)
{
}

PassRefPtr<MediaControlFullscreenTimelineContainerElement> MediaControlFullscreenTimelineContainerElement::create(Document* document)
{
    RefPtr<MediaControlFullscreenTimelineContainerElement> element = adoptRef(new MediaControlFullscreenTimelineContainerElement(document));
    element->hide();
    return element.release();
}

const AtomicString& MediaControlFullscreenTimelineContainerElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-fullscreen-timeline-container", AtomicString::ConstructFromLiteral));
    return id;
}

MediaControlEmbeddedTimelineElement::MediaControlEmbeddedTimelineElement(Document* document, MediaControls* controls)
    : MediaControlInputElement(document, MediaSlider)
    , m_controls(controls)
{
}

PassRefPtr<MediaControlEmbeddedTimelineElement> MediaControlEmbeddedTimelineElement::create(Document* document, MediaControls* controls)
{
    ASSERT(controls);

    RefPtr<MediaControlEmbeddedTimelineElement> timeline = adoptRef(new MediaControlEmbeddedTimelineElement(document, controls));
    timeline->ensureUserAgentShadowRoot();
    timeline->setType("range");
    timeline->setAttribute(precisionAttr, "float");
    return timeline.release();
}

void MediaControlEmbeddedTimelineElement::defaultEventHandler(Event* event)
{
    // Left button is 0. Rejects mouse events not from left button.
    if (event->isMouseEvent() && static_cast<MouseEvent*>(event)->button())
        return;

    if (!attached())
        return;

    if (hasAttribute(readonlyAttr))
        return;

    if (event->type() == eventNames().touchstartEvent || event->type() == eventNames().mousedownEvent)
        mediaController()->beginScrubbing();

    MediaControlInputElement::defaultEventHandler(event);

    if (event->type() == eventNames().mouseoverEvent || event->type() == eventNames().mouseoutEvent || event->type() == eventNames().mousemoveEvent)
        return;

    float time = narrowPrecisionToFloat(value().toDouble());
    if (time != mediaController()->currentTime() && (event->type() == eventNames().touchmoveEvent || event->type() == eventNames().touchendEvent || event->type() == eventNames().inputEvent || event->type() == eventNames().mouseupEvent))
        mediaController()->setCurrentTime(time, IGNORE_EXCEPTION);

    if (event->type() == eventNames().touchendEvent || event->type() == eventNames().mouseupEvent)
        mediaController()->endScrubbing();

    RenderSlider* slider = toRenderSlider(renderer());
    if (slider && slider->inDragMode())
        m_controls->updateCurrentTimeDisplay();
}

bool MediaControlEmbeddedTimelineElement::willRespondToMouseClickEvents()
{
    if (!attached())
        return false;

    return true;
}

void MediaControlEmbeddedTimelineElement::setPosition(float currentTime)
{
    setValue(String::number(currentTime));
}

void MediaControlEmbeddedTimelineElement::setDuration(float duration)
{
    setAttribute(maxAttr, String::number(isfinite(duration) ? duration : 0));
}

const AtomicString& MediaControlEmbeddedTimelineElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-timeline", AtomicString::ConstructFromLiteral));
    return id;
}

MediaControlFullscreenTimelineElement::MediaControlFullscreenTimelineElement(Document* document, MediaControls* controls)
    : MediaControlInputElement(document, MediaSlider)
    , m_controls(controls)
{
}

PassRefPtr<MediaControlFullscreenTimelineElement> MediaControlFullscreenTimelineElement::create(Document* document, MediaControls* controls)
{
    ASSERT(controls);

    RefPtr<MediaControlFullscreenTimelineElement> timeline = adoptRef(new MediaControlFullscreenTimelineElement(document, controls));
    timeline->ensureUserAgentShadowRoot();
    timeline->setType("range");
    timeline->setAttribute(precisionAttr, "float");
    return timeline.release();
}

void MediaControlFullscreenTimelineElement::defaultEventHandler(Event* event)
{
    // Left button is 0. Rejects mouse events not from left button.
    if (event->isMouseEvent() && static_cast<MouseEvent*>(event)->button())
        return;

    if (!attached())
        return;

    if (hasAttribute(readonlyAttr))
        return;

    if (event->type() == eventNames().touchstartEvent || event->type() == eventNames().mousedownEvent)
        mediaController()->beginScrubbing();

    MediaControlInputElement::defaultEventHandler(event);

    if (event->type() == eventNames().mouseoverEvent || event->type() == eventNames().mouseoutEvent || event->type() == eventNames().mousemoveEvent)
        return;

    float time = narrowPrecisionToFloat(value().toDouble());
    if (time != mediaController()->currentTime() && (event->type() == eventNames().touchmoveEvent || event->type() == eventNames().touchendEvent || event->type() == eventNames().inputEvent || event->type() == eventNames().mouseupEvent))
        mediaController()->setCurrentTime(time, IGNORE_EXCEPTION);

    if (event->type() == eventNames().touchendEvent || event->type() == eventNames().mouseupEvent)
        mediaController()->endScrubbing();

    RenderSlider* slider = toRenderSlider(renderer());
    if (slider && slider->inDragMode())
        m_controls->updateCurrentTimeDisplay();
}

bool MediaControlFullscreenTimelineElement::willRespondToMouseClickEvents()
{
    if (!attached())
        return false;

    return true;
}

void MediaControlFullscreenTimelineElement::setPosition(float currentTime)
{
    setValue(String::number(currentTime));
}

void MediaControlFullscreenTimelineElement::setDuration(float duration)
{
    setAttribute(maxAttr, String::number(std::isfinite(duration) ? duration : 0));
}

const AtomicString& MediaControlFullscreenTimelineElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-fullscreen-timeline", AtomicString::ConstructFromLiteral));
    return id;
}

PassRefPtr<MediaControlFullscreenTimeRemainingDisplayElement> MediaControlFullscreenTimeRemainingDisplayElement::create(Document* document)
{
    return adoptRef(new MediaControlFullscreenTimeRemainingDisplayElement(document));
}

MediaControlFullscreenTimeRemainingDisplayElement::MediaControlFullscreenTimeRemainingDisplayElement(Document* document)
    : MediaControlTimeDisplayElement(document, MediaTimeRemainingDisplay)
{
}

const AtomicString& MediaControlFullscreenTimeRemainingDisplayElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-fullscreen-time-remaining-display", AtomicString::ConstructFromLiteral));
    return id;
}

PassRefPtr<MediaControlFullscreenCurrentTimeDisplayElement> MediaControlFullscreenCurrentTimeDisplayElement::create(Document* document)
{
    return adoptRef(new MediaControlFullscreenCurrentTimeDisplayElement(document));
}

MediaControlFullscreenCurrentTimeDisplayElement::MediaControlFullscreenCurrentTimeDisplayElement(Document* document)
    : MediaControlTimeDisplayElement(document, MediaCurrentTimeDisplay)
{
}

const AtomicString& MediaControlFullscreenCurrentTimeDisplayElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-fullscreen-current-time-display", AtomicString::ConstructFromLiteral));
    return id;
}

MediaControlAudioMuteButtonElement::MediaControlAudioMuteButtonElement(Document* document, MediaControls* controls)
    : MediaControlMuteButtonElement(document, MediaMuteButton)
    , m_controls(controls)
{
}

PassRefPtr<MediaControlAudioMuteButtonElement> MediaControlAudioMuteButtonElement::create(Document* document, MediaControls* controls)
{
    ASSERT(controls);

    RefPtr<MediaControlAudioMuteButtonElement> button = adoptRef(new MediaControlAudioMuteButtonElement(document, controls));
    button->ensureUserAgentShadowRoot();
    button->setType("button");
    return button.release();
}

void MediaControlAudioMuteButtonElement::defaultEventHandler(Event* event)
{
    if (event->type() == eventNames().mousedownEvent) {
        // We do not mute when the media player volume/mute control is touched.
        // Instead we show/hide the volume slider.
        static_cast<MediaControlsBlackBerry*>(m_controls)->toggleVolumeSlider();
        event->setDefaultHandled();
    }
}

const AtomicString& MediaControlAudioMuteButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-audio-mute-button", AtomicString::ConstructFromLiteral));
    return id;
}

MediaControlsBlackBerry::MediaControlsBlackBerry(Document* document)
    : MediaControls(document)
    , m_buttonContainer(0)
    , m_timeDisplayContainer(0)
    , m_fullscreenTimeDisplayContainer(0)
    , m_fullscreenPlayButton(0)
    , m_fullscreenCurrentTimeDisplay(0)
    , m_fullscreenTimeline(0)
    , m_timeRemainingDisplay(0)
    , m_fullscreenTimeRemainingDisplay(0)
    , m_timelineContainer(0)
    , m_fullscreenTimelineContainer(0)
    , m_fullScreenDivider(0)
    , m_fullscreenFullScreenButton(0)
    , m_muteButton(0)
    , m_volumeSliderContainer(0)
    , m_embeddedPanel(0)
    , m_fullScreenButtonContainer(0)
    , m_playButtonContainer(0)
    , m_placeholder(0)
    , m_toggleClosedCaptionsMenuButton(0)
    , m_closedCaptionsContainer(0)
    , m_closedCaptionsTrackList(0)
    , m_hidden(false)
    , m_playingOnRemoteDisplay(false)
    , m_miracastCardWasInvoked(false)
    , m_presentationModePollTimer(this, &MediaControlsBlackBerry::pollTimerFired)
{
}

MediaControlsBlackBerry::~MediaControlsBlackBerry()
{
    // You can't call virtual functions here.
    m_hideFullscreenControlsTimer.stop();
}

PassRefPtr<MediaControls> MediaControls::create(Document* document)
{
    return MediaControlsBlackBerry::createControls(document);
}

PassRefPtr<MediaControlsBlackBerry> MediaControlsBlackBerry::createControls(Document* document)
{
    if (!document->page())
        return 0;

    RefPtr<MediaControlsBlackBerry> controls = adoptRef(new MediaControlsBlackBerry(document));

    RefPtr<MediaControlPanelElement> panel = MediaControlPanelElement::create(document);
    RefPtr<MediaControlEmbeddedPanelElement> embedPanel = MediaControlEmbeddedPanelElement::create(document);

    ExceptionCode ec;

    RefPtr<MediaControlPlayButtonElement> playButton = MediaControlPlayButtonElement::create(document);
    controls->m_playButton = playButton.get();
    embedPanel->appendChild(playButton.release(), ec, AttachLazily);
    if (ec)
        return 0;

    RefPtr<MediaControlTimelineContainerElement> timelineContainer = MediaControlTimelineContainerElement::create(document);

    RefPtr<MediaControlEmbeddedTimelineElement> timeline = MediaControlEmbeddedTimelineElement::create(document, controls.get());
    controls->m_embeddedTimeline = timeline.get();
    timelineContainer->appendChild(timeline.release(), ec, AttachLazily);
    if (ec)
        return 0;

    RefPtr<MediaControlTimeDisplayContainerElement> timeDisplayContainer = MediaControlTimeDisplayContainerElement::create(document);

    RefPtr<MediaControlCurrentTimeDisplayElement> currentTimeDisplay = MediaControlCurrentTimeDisplayElement::create(document);
    controls->m_currentTimeDisplay = currentTimeDisplay.get();
    timeDisplayContainer->appendChild(currentTimeDisplay.release(), ec, AttachLazily);
    if (ec)
        return 0;

    RefPtr<MediaControlTimeRemainingDisplayElement> timeRemainingDisplay = MediaControlTimeRemainingDisplayElement::create(document);
    controls->m_timeRemainingDisplay = timeRemainingDisplay.get();
    timeDisplayContainer->appendChild(timeRemainingDisplay.release(), ec, AttachLazily);
    if (ec)
        return 0;

    controls->m_timeDisplayContainer = timeDisplayContainer.get();
    timelineContainer->appendChild(timeDisplayContainer.release(), ec, AttachLazily);
    if (ec)
        return 0;

    controls->m_timelineContainer = timelineContainer.get();
    embedPanel->appendChild(timelineContainer.release(), ec, AttachLazily);
    if (ec)
        return 0;

    RefPtr<MediaControlFullscreenButtonElement> fullScreenButton = MediaControlFullscreenButtonElement::create(document);
    controls->m_fullScreenButton = fullScreenButton.get();
    embedPanel->appendChild(fullScreenButton.release(), ec, AttachLazily);
    if (ec)
        return 0;

    if (document->page()->theme()->usesMediaControlVolumeSlider()) {
        // The mute button and the slider element should be in the same div.
        RefPtr<HTMLDivElement> volumeControlContainer = HTMLDivElement::create(document);

        RefPtr<MediaControlVolumeSliderContainerElement> volumeSliderContainer = MediaControlVolumeSliderContainerElement::create(document);

        RefPtr<MediaControlPanelVolumeSliderElement> slider = MediaControlPanelVolumeSliderElement::create(document);
        controls->m_volumeSlider = slider.get();
        volumeSliderContainer->appendChild(slider.release(), ec, AttachLazily);
        if (ec)
            return 0;

        controls->m_volumeSliderContainer = volumeSliderContainer.get();
        volumeControlContainer->appendChild(volumeSliderContainer.release(), ec, AttachLazily);
        if (ec)
            return 0;
        RefPtr<MediaControlAudioMuteButtonElement> muteButton = MediaControlAudioMuteButtonElement::create(document, controls.get());
        controls->m_muteButton = muteButton.get();
        volumeControlContainer->appendChild(muteButton.release(), ec, AttachLazily);
        if (ec)
            return 0;

        embedPanel->appendChild(volumeControlContainer.release(), ec, AttachLazily);
        if (ec)
            return 0;
    }

    RefPtr<MediaControlFullscreenTimelineContainerElement> fullscreenTimelineContainer = MediaControlFullscreenTimelineContainerElement::create(document);

    RefPtr<MediaControlFullscreenTimelineElement> fullscreenTimeline = MediaControlFullscreenTimelineElement::create(document, controls.get());
    controls->m_fullscreenTimeline = fullscreenTimeline.get();
    fullscreenTimelineContainer->appendChild(fullscreenTimeline.release(), ec, AttachLazily);
    if (ec)
        return 0;

    RefPtr<MediaControlFullscreenTimeDisplayContainerElement> fullscreenTimeDisplayContainer = MediaControlFullscreenTimeDisplayContainerElement::create(document);

    RefPtr<MediaControlFullscreenCurrentTimeDisplayElement> fullscreenCurrentTimeDisplay = MediaControlFullscreenCurrentTimeDisplayElement::create(document);
    controls->m_fullscreenCurrentTimeDisplay = fullscreenCurrentTimeDisplay.get();
    fullscreenTimeDisplayContainer->appendChild(fullscreenCurrentTimeDisplay.release(), ec, AttachLazily);
    if (ec)
        return 0;

    RefPtr<MediaControlFullscreenTimeRemainingDisplayElement> fullscreenTimeRemainingDisplay = MediaControlFullscreenTimeRemainingDisplayElement::create(document);
    controls->m_fullscreenTimeRemainingDisplay = fullscreenTimeRemainingDisplay.get();
    fullscreenTimeDisplayContainer->appendChild(fullscreenTimeRemainingDisplay.release(), ec, AttachLazily);
    if (ec)
        return 0;

    controls->m_fullscreenTimeDisplayContainer = fullscreenTimeDisplayContainer.get();
    fullscreenTimelineContainer->appendChild(fullscreenTimeDisplayContainer.release(), ec, AttachLazily);
    if (ec)
        return 0;

    controls->m_fullscreenTimelineContainer = fullscreenTimelineContainer.get();
    panel->appendChild(fullscreenTimelineContainer.release(), ec, AttachLazily);
    if (ec)
        return 0;

    RefPtr<MediaControlButtonGroupContainerElement> buttonGroupContainer = MediaControlButtonGroupContainerElement::create(document);

    // FIXME: Only create when needed <http://webkit.org/b/57163>
    RefPtr<MediaControlFullscreenButtonContainerElement> fullScreenButtonContainer = MediaControlFullscreenButtonContainerElement::create(document);
    controls->m_fullScreenButtonContainer = fullScreenButtonContainer.get();
    RefPtr<MediaControlFullscreenFullscreenButtonElement> fullscreenFullScreenButton = MediaControlFullscreenFullscreenButtonElement::create(document);
    controls->m_fullscreenFullScreenButton = fullscreenFullScreenButton.get();
    fullScreenButtonContainer->appendChild(fullscreenFullScreenButton.release(), ec, AttachLazily);
    if (ec)
        return 0;
    RefPtr<MediaControlFullscreenButtonDividerElement> fullScreenDivider = MediaControlFullscreenButtonDividerElement::create(document);
    controls->m_fullScreenDivider = fullScreenDivider.get();
    fullScreenButtonContainer->appendChild(fullScreenDivider.release(), ec, AttachLazily);
    if (ec)
        return 0;

    RefPtr<MediaControlPlaceholderElement> placeholder = MediaControlPlaceholderElement::create(document);
    fullScreenButtonContainer->appendChild(placeholder.release(), ec, AttachLazily);
    if (ec)
        return 0;

    buttonGroupContainer->appendChild(fullScreenButtonContainer.release(), ec, AttachLazily);
    if (ec)
        return 0;

    RefPtr<MediaControlPlayButtonContainerElement> playButtonContainer = MediaControlPlayButtonContainerElement::create(document);
    controls->m_playButtonContainer = playButtonContainer.get();
    RefPtr<MediaControlFullscreenPlayButtonElement> fullscreenPlayButton = MediaControlFullscreenPlayButtonElement::create(document);
    controls->m_fullscreenPlayButton = fullscreenPlayButton.get();
    playButtonContainer->appendChild(fullscreenPlayButton.release(), ec, AttachLazily);
    if (ec)
        return 0;
    buttonGroupContainer->appendChild(playButtonContainer.release(), ec, AttachLazily);
    if (ec)
        return 0;


    RefPtr<MediaControlPlayOnButtonContainerElement> playOnButtonContainer = MediaControlPlayOnButtonContainerElement::create(document);
    controls->m_playOnButtonContainer = playOnButtonContainer.get();

    RefPtr<MediaControlFullscreenPlayOnButtonElement> fullscreenPlayOnButton = MediaControlFullscreenPlayOnButtonElement::create(document, controls.get());
    controls->m_fullscreenPlayOnButton = fullscreenPlayOnButton.get();
    playOnButtonContainer->appendChild(fullscreenPlayOnButton.release(), ec, AttachLazily);
    if (ec)
        return 0;

    if (document->page()->theme()->supportsClosedCaptioning()) {
        RefPtr<MediaControlClosedCaptionsContainerElement> closedCaptionsContainer = MediaControlClosedCaptionsContainerElement::create(document);

        RefPtr<MediaControlClosedCaptionsTrackListMenuElement> closedCaptionsTrackList = MediaControlClosedCaptionsTrackListMenuElement::create(document, controls.get());
        controls->m_closedCaptionsTrackList = closedCaptionsTrackList.get();
        closedCaptionsContainer->appendChild(closedCaptionsTrackList.release(), ec, AttachLazily);
        if (ec)
            return 0;

        RefPtr<MediaControlToggleClosedCaptionsListButtonElement> toggleClosedCaptionsButton = MediaControlToggleClosedCaptionsListButtonElement::create(document, controls.get());
        controls->m_toggleClosedCaptionsMenuButton = toggleClosedCaptionsButton.get();
        playOnButtonContainer->appendChild(toggleClosedCaptionsButton.release(), ec, AttachLazily);
        if (ec)
            return 0;

        controls->m_closedCaptionsContainer = closedCaptionsContainer.get();
        controls->appendChild(closedCaptionsContainer.release(), ec, AttachLazily);
        if (ec)
            return 0;
    }

    buttonGroupContainer->appendChild(playOnButtonContainer.release(), ec, AttachLazily);
    if (ec)
        return 0;

    controls->m_buttonContainer = buttonGroupContainer.get();
    panel->appendChild(buttonGroupContainer.release(), ec, AttachLazily);
    if (ec)
        return 0;

    controls->m_panel = panel.get();
    controls->appendChild(panel.release(), ec, AttachLazily);
    if (ec)
        return 0;

    controls->m_embeddedPanel = embedPanel.get();
    controls->appendChild(embedPanel.release(), ec, AttachLazily);
    if (ec)
        return 0;

    return controls.release();
}

void MediaControlsBlackBerry::setMediaController(MediaControllerInterface* controller)
{
    if (m_mediaController == controller)
        return;

    MediaControls::setMediaController(controller);

    if (m_buttonContainer)
        m_buttonContainer->setMediaController(controller);
    if (m_timeDisplayContainer)
        m_timeDisplayContainer->setMediaController(controller);
    if (m_fullscreenTimeDisplayContainer)
        m_fullscreenTimeDisplayContainer->setMediaController(controller);
    if (m_fullscreenPlayButton)
        m_fullscreenPlayButton->setMediaController(controller);
    if (m_fullscreenCurrentTimeDisplay)
        m_fullscreenCurrentTimeDisplay->setMediaController(controller);
    if (m_fullscreenTimeline)
        m_fullscreenTimeline->setMediaController(controller);
    if (m_embeddedTimeline)
        m_embeddedTimeline->setMediaController(controller);
    if (m_timeRemainingDisplay)
        m_timeRemainingDisplay->setMediaController(controller);
    if (m_fullscreenTimeRemainingDisplay)
        m_fullscreenTimeRemainingDisplay->setMediaController(controller);
    if (m_timelineContainer)
        m_timelineContainer->setMediaController(controller);
    if (m_fullscreenTimelineContainer)
        m_fullscreenTimelineContainer->setMediaController(controller);
    if (m_fullscreenFullScreenButton)
        m_fullscreenFullScreenButton->setMediaController(controller);
    if (m_muteButton)
        m_muteButton->setMediaController(controller);
    if (m_volumeSliderContainer)
        m_volumeSliderContainer->setMediaController(controller);
    if (m_embeddedPanel)
        m_embeddedPanel->setMediaController(controller);
    if (m_toggleClosedCaptionsMenuButton)
        m_toggleClosedCaptionsMenuButton->setMediaController(controller);
    if (m_closedCaptionsTrackList)
        m_closedCaptionsTrackList->setMediaController(controller);
    if (m_closedCaptionsContainer)
        m_closedCaptionsContainer->setMediaController(controller);
    reset();
}

void MediaControlsBlackBerry::show()
{
    if (m_isFullscreen) {
        m_panel->setIsDisplayed(true);
        m_panel->show();
    } else {
        m_embeddedPanel->setIsDisplayed(true);
        m_embeddedPanel->show();
    }
}

void MediaControlsBlackBerry::hide()
{
    if (m_isFullscreen) {
        if (m_closedCaptionsContainer)
            m_closedCaptionsContainer->hide();
        m_panel->setIsDisplayed(false);
        m_panel->hide();
    } else {
        m_embeddedPanel->setIsDisplayed(false);
        m_embeddedPanel->hide();
        m_volumeSliderContainer->hide();
    }
}

void MediaControlsBlackBerry::makeOpaque()
{
    if (m_isFullscreen) {
        if (m_textDisplayContainer)
            m_textDisplayContainer->setInlineStyleProperty(CSSPropertyMarginBottom, 15.0, CSSPrimitiveValue::CSS_VH);
        m_panel->makeOpaque();
    } else {
        if (m_textDisplayContainer)
            m_textDisplayContainer->setInlineStyleProperty(CSSPropertyMarginBottom, 36.0, CSSPrimitiveValue::CSS_PX);
        m_embeddedPanel->makeOpaque();
    }
    m_hidden = false;
}

void MediaControlsBlackBerry::makeTransparent()
{
    if (m_isFullscreen) {
        m_panel->makeTransparent();
        if (m_closedCaptionsContainer)
            m_closedCaptionsContainer->hide();
        if (m_textDisplayContainer)
            m_textDisplayContainer->setInlineStyleProperty(CSSPropertyMarginBottom, 1.0, CSSPrimitiveValue::CSS_VH);
    } else {
        m_embeddedPanel->makeTransparent();
        m_volumeSliderContainer->hide();
        if (m_textDisplayContainer)
            m_textDisplayContainer->setInlineStyleProperty(CSSPropertyMarginBottom, 1.0, CSSPrimitiveValue::CSS_PX);
    }
    m_hidden = true;
}

void MediaControlsBlackBerry::updateTimelines()
{
    float duration = m_mediaController->duration();
    float now = m_mediaController->currentTime();
    m_embeddedTimeline->setDuration(duration);
    m_fullscreenTimeline->setDuration(duration);
    m_embeddedTimeline->setPosition(now);
    m_fullscreenTimeline->setPosition(now);
}

void MediaControlsBlackBerry::changedClosedCaptionsVisibility()
{
    refreshClosedCaptionsButtonVisibility();
    if (m_closedCaptionsTrackList)
        m_closedCaptionsTrackList->updateDisplay();
}

void MediaControlsBlackBerry::refreshClosedCaptionsButtonVisibility()
{
    if (m_toggleClosedCaptionsMenuButton)
        m_toggleClosedCaptionsMenuButton->updateDisplayType();
}

void MediaControlsBlackBerry::closedCaptionTracksChanged()
{
    if (m_closedCaptionsTrackList)
        m_closedCaptionsTrackList->resetTrackListMenu();
    refreshClosedCaptionsButtonVisibility();
}

#if ENABLE(VIDEO_TRACK)
void MediaControlsBlackBerry::createTextTrackDisplay()
{
    if (m_textDisplayContainer)
        return;

    RefPtr<MediaControlTextTrackContainerElement> textDisplayContainer = MediaControlTextTrackContainerElement::create(document());
    m_textDisplayContainer = textDisplayContainer.get();

    if (m_mediaController)
        m_textDisplayContainer->setMediaController(m_mediaController);

    if (m_hidden)
        if (m_isFullscreen)
            m_textDisplayContainer->setInlineStyleProperty(CSSPropertyMarginBottom, 1.0, CSSPrimitiveValue::CSS_VH);
        else
            m_textDisplayContainer->setInlineStyleProperty(CSSPropertyMarginBottom, 1.0, CSSPrimitiveValue::CSS_PX);
    else
        if (m_isFullscreen)
            m_textDisplayContainer->setInlineStyleProperty(CSSPropertyMarginBottom, 15.0, CSSPrimitiveValue::CSS_VH);
        else
            m_textDisplayContainer->setInlineStyleProperty(CSSPropertyMarginBottom, 36.0, CSSPrimitiveValue::CSS_PX);

    // Insert it before the first controller element so it always displays behind the controls.
    insertBefore(textDisplayContainer.release(), m_panel, IGNORE_EXCEPTION, AttachLazily);
}
#endif
void MediaControlsBlackBerry::reset()
{
    Page* page = document()->page();
    if (!page)
        return;

    updateStatusDisplay();

    if (m_fullScreenButton) {
        if (m_mediaController->supportsFullscreen())
            m_fullScreenButton->show();
        else
            m_fullScreenButton->hide();
    }
    if (m_fullScreenDivider) {
        if (m_mediaController->supportsFullscreen())
            m_fullScreenDivider->show();
        else
            m_fullScreenDivider->hide();
    }
    if (m_fullscreenFullScreenButton) {
        if (m_mediaController->supportsFullscreen())
            m_fullscreenFullScreenButton->show();
        else
            m_fullscreenFullScreenButton->hide();
    }
    float duration = m_mediaController->duration();
    if (std::isfinite(duration) || page->theme()->hasOwnDisabledStateHandlingFor(MediaSliderPart)) {
        updateTimelines();
        m_timelineContainer->show();
        m_fullscreenTimelineContainer->show();
        updateCurrentTimeDisplay();
    } else {
        m_timelineContainer->hide();
        m_fullscreenTimelineContainer->hide();
    }

    if (m_mediaController->hasAudio() || page->theme()->hasOwnDisabledStateHandlingFor(MediaMuteButtonPart))
        m_muteButton->show();
    else
        m_muteButton->hide();

    if (m_volumeSlider)
        m_volumeSlider->setVolume(m_mediaController->volume());

    if (m_toggleClosedCaptionsMenuButton) {
        m_toggleClosedCaptionsMenuButton->show();
        if (m_mediaController->hasClosedCaptions()) {
            m_toggleClosedCaptionsMenuButton->enable();
            if (m_closedCaptionsTrackList)
                m_closedCaptionsTrackList->resetTrackListMenu();
        } else
            m_toggleClosedCaptionsMenuButton->disable();
    }

    if (m_playButton)
        m_playButton->updateDisplayType();

    if (m_fullscreenPlayButton)
        m_fullscreenPlayButton->updateDisplayType();

    if (m_mediaController->seekable()->length()) {
        m_embeddedTimeline->setBooleanAttribute(readonlyAttr, false);
        m_fullscreenTimeline->setBooleanAttribute(readonlyAttr, false);
    } else {
        m_embeddedTimeline->setBooleanAttribute(readonlyAttr, true);
        m_fullscreenTimeline->setBooleanAttribute(readonlyAttr, true);
    }
    makeOpaque();
    stopHideFullscreenControlsTimer();
}

void MediaControlsBlackBerry::bufferingProgressed()
{
    // We only need to update buffering progress when paused, during normal
    // playback playbackProgressed() will take care of it.
    if (m_mediaController->paused())
        updateTimelines();
}

void MediaControlsBlackBerry::playbackStarted()
{
    m_playButton->updateDisplayType();
    m_fullscreenPlayButton->updateDisplayType();
    updateTimelines();
    updateCurrentTimeDisplay();

    if (m_isFullscreen)
        startHideFullscreenControlsTimer();
}

void MediaControlsBlackBerry::playbackProgressed()
{
    updateTimelines();
    updateCurrentTimeDisplay();
    
    if (!m_isMouseOverControls && !m_isFullscreen && m_mediaController->hasVideo())
        makeTransparent();
}

void MediaControlsBlackBerry::playbackStopped()
{
    m_playButton->updateDisplayType();
    m_fullscreenPlayButton->updateDisplayType();
    updateTimelines();
    updateCurrentTimeDisplay();
    makeOpaque();
    stopHideFullscreenControlsTimer();
}

void MediaControlsBlackBerry::updateCurrentTimeDisplay()
{
    float now = m_mediaController->currentTime();
    float duration = m_mediaController->duration();

    Page* page = document()->page();
    if (!page)
        return;

    // Allow the theme to format the time.
    m_currentTimeDisplay->setInnerText(page->theme()->formatMediaControlsCurrentTime(now, duration), IGNORE_EXCEPTION);
    m_currentTimeDisplay->setCurrentValue(now);
    m_fullscreenCurrentTimeDisplay->setInnerText(page->theme()->formatMediaControlsCurrentTime(now, duration), IGNORE_EXCEPTION);
    m_fullscreenCurrentTimeDisplay->setCurrentValue(now);
    m_timeRemainingDisplay->setInnerText(page->theme()->formatMediaControlsRemainingTime(now, duration), IGNORE_EXCEPTION);
    m_timeRemainingDisplay->setCurrentValue(now - duration);
    m_fullscreenTimeRemainingDisplay->setInnerText(page->theme()->formatMediaControlsRemainingTime(now, duration), IGNORE_EXCEPTION);
    m_fullscreenTimeRemainingDisplay->setCurrentValue(now - duration);
}

void MediaControlsBlackBerry::reportedError()
{
    Page* page = document()->page();
    if (!page)
        return;

    if (!page->theme()->hasOwnDisabledStateHandlingFor(MediaSliderPart)) {
        m_timelineContainer->hide();
        m_fullscreenTimelineContainer->hide();
    }

    if (!page->theme()->hasOwnDisabledStateHandlingFor(MediaMuteButtonPart))
        m_muteButton->hide();

    // Note: We don't hide m_fullScreenDivider or m_fullscreenFullScreenButton
    // because the user always needs these to exit fullscreen.
    if (m_fullScreenButton)
        m_fullScreenButton->hide();

    if (m_volumeSliderContainer)
        m_volumeSliderContainer->hide();

    if (m_closedCaptionsContainer && m_closedCaptionsContainer->isShowing())
        m_closedCaptionsContainer->hide();

    if (m_toggleClosedCaptionsMenuButton && !page->theme()->hasOwnDisabledStateHandlingFor(MediaToggleClosedCaptionsButtonPart))
        m_toggleClosedCaptionsMenuButton->hide();
}

void MediaControlsBlackBerry::changedMute()
{
    m_muteButton->changedMute();
}

void MediaControlsBlackBerry::enteredFullscreen()
{
    MediaControls::enteredFullscreen(); // Sets m_isFullscreen = true
    makeOpaque();

    m_panel->setCanBeDragged(true);
    m_embeddedPanel->setCanBeDragged(true);

    if (m_fullscreenFullScreenButton)
        m_fullscreenFullScreenButton->setIsFullscreen(true);

    if (m_textDisplayContainer)
        m_textDisplayContainer->setInlineStyleProperty(CSSPropertyMarginBottom, 15.0, CSSPrimitiveValue::CSS_VH);

    // Check once per second for presentation mode transitions.
    m_presentationModePollTimer.startOneShot(s_pollTimeout);
    m_playingOnRemoteDisplay = false;
    m_miracastCardWasInvoked = false;
}

void MediaControlsBlackBerry::exitedFullscreen()
{
    m_panel->setCanBeDragged(false);
    m_embeddedPanel->setCanBeDragged(false);

    if (m_fullscreenFullScreenButton)
        m_fullscreenFullScreenButton->setIsFullscreen(false);

    if (m_fullScreenButton)
        m_fullScreenButton->setIsFullscreen(false);

    if (m_closedCaptionsContainer && m_closedCaptionsContainer->isShowing())
        m_closedCaptionsContainer->hide();

    if (m_textDisplayContainer)
        m_textDisplayContainer->setInlineStyleProperty(CSSPropertyMarginBottom, 36.0, CSSPrimitiveValue::CSS_PX);

    // We will keep using the panel, but we want it to go back to the standard position.
    // This will matter right away because we use the panel even when not fullscreen.
    // And if we reenter fullscreen we also want the panel in the standard position.
    m_panel->resetPosition();
    m_embeddedPanel->resetPosition();

    m_isFullscreen = false;
    stopHideFullscreenControlsTimer();
    makeOpaque();
    m_presentationModePollTimer.stop();

    m_playingOnRemoteDisplay = false;
    m_miracastCardWasInvoked = false;
    MediaPlayerPrivate* bbPlayer = getNativeMediaPlayer(this);
    if (bbPlayer && bbPlayer->isPresentationModeEnabled()) {
        if (!bbPlayer->hasRemoteDisplays()) {
            // We are exiting fullscreen. If the user attempted to enter
            // presentation mode but there is no secondary display attached
            // then we assume that attaching to a remote display failed. Disable
            // presentation mode to tidy up loose ends. This is not crucial.
            bbPlayer->setPresentationModeEnabled(false);
        }
    }
}

void MediaControlsBlackBerry::hideFullscreenControlsTimerFired(Timer<MediaControls>*)
{
    if (m_mediaController->paused())
        return;

    if (!m_mediaController->hasVideo())
        return;

    if (m_playingOnRemoteDisplay)
        return;

    if (m_closedCaptionsContainer && m_closedCaptionsContainer->isShowing()) {
        startHideFullscreenControlsTimer();
        return;
    }

    if (Page* page = document()->page())
        page->chrome()->setCursorHiddenUntilMouseMoves(true);

    makeTransparent();
}

void MediaControlsBlackBerry::startHideFullscreenControlsTimer()
{
    if (!m_mediaController->hasVideo())
        return;

    Page* page = document()->page();
    if (!page)
        return;

    if (m_playingOnRemoteDisplay)
        return;

    m_hideFullscreenControlsTimer.startOneShot(page->settings()->timeWithoutMouseMovementBeforeHidingControls());
}

void MediaControlsBlackBerry::defaultEventHandler(Event* event)
{
    m_miracastCardWasInvoked = false; // An input event here means the miracast card is no longer up.
    HTMLDivElement::defaultEventHandler(event);

    if (event->type() == eventNames().clickEvent) {
        if (m_closedCaptionsContainer && m_closedCaptionsContainer->isShowing()) {
            m_closedCaptionsContainer->hide();
            event->setDefaultHandled();
        }
    }

    if (event->type() == eventNames().mouseoverEvent) {
        if (!containsRelatedTarget(event)) {
            m_isMouseOverControls = true;
            if (!m_mediaController->canPlay()) {
                makeOpaque();
                if (m_isFullscreen)
                    startHideFullscreenControlsTimer();
            }
        }
        return;
    }

    if (event->type() == eventNames().mouseoutEvent) {
        if (!containsRelatedTarget(event)) {
            m_isMouseOverControls = false;
            stopHideFullscreenControlsTimer();
        }
        return;
    }
    if (event->type() == eventNames().mousemoveEvent) {
        makeOpaque();
        if (m_isFullscreen)
            startHideFullscreenControlsTimer();
        return;
    }
}

void MediaControlsBlackBerry::showVolumeSlider()
{
    if (!m_mediaController->hasAudio())
        return;

    if (m_volumeSliderContainer)
        m_volumeSliderContainer->show();
}

void MediaControlsBlackBerry::toggleVolumeSlider()
{
    if (!m_mediaController->hasAudio())
        return;

    if (m_volumeSliderContainer) {
        if (m_volumeSliderContainer->renderer() && m_volumeSliderContainer->renderer()->visibleToHitTesting())
            m_volumeSliderContainer->hide();
        else
            m_volumeSliderContainer->show();
    }
}

void MediaControlsBlackBerry::toggleClosedCaptionTrackList()
{
    if (!m_mediaController->hasClosedCaptions())
        return;

    if (m_closedCaptionsContainer) {
        if (m_closedCaptionsContainer->isShowing())
            m_closedCaptionsContainer->hide();
        else {
            if (m_closedCaptionsTrackList)
                m_closedCaptionsTrackList->updateDisplay();
            m_closedCaptionsContainer->show();
        }
    }
}

void MediaControlsBlackBerry::setMiracastCardInvoked(bool enabled)
{
    m_miracastCardWasInvoked = enabled;
}

bool MediaControlsBlackBerry::shouldHideControls()
{
    return true;
}

// This timer fires once per second (s_pollTimeout) during fullscreen mode
// to detect transitions relevant to the PlayOn button.
void MediaControlsBlackBerry::pollTimerFired(Timer<MediaControlsBlackBerry>*)
{
    if (!m_isFullscreen)
        return;

    MediaPlayerPrivate* bbPlayer = getNativeMediaPlayer(this);
    if (bbPlayer) {
        if (!bbPlayer->isPresentationModeEnabled() || !bbPlayer->hasRemoteDisplays()) {
            if (m_playingOnRemoteDisplay) {
                // Detected transition out of presentation mode.
                // Hide fullscreen media controls as usual.
                m_playingOnRemoteDisplay = false;
                startHideFullscreenControlsTimer();
                if (m_fullscreenPlayOnButton->renderer())
                    m_fullscreenPlayOnButton->renderer()->repaint();
            } else if (!bbPlayer->isMiracastSupported() && !m_hidden) {
                // On devices that don't support Miracast we repaint the PlayOn
                // button upon every timer fire while not in presentation mode
                // and the controls are not hidden. This is needed to
                // enable/disable the PlayOn button when the HDMI cable is
                // connected/disconnected and is worth the slight performance
                // hit because it simplifies the logic quite a bit, compared
                // with detecting hasRemoteDisplays() transitions.
                if (m_fullscreenPlayOnButton->renderer())
                    m_fullscreenPlayOnButton->renderer()->repaint();
            }
        } else {
            if (!m_playingOnRemoteDisplay) {
                // Detected transition into presentation mode.
                // Prevent hiding of fullscreen media controls.
                m_playingOnRemoteDisplay = true;
                makeOpaque();
                stopHideFullscreenControlsTimer();
                if (m_fullscreenPlayOnButton->renderer())
                    m_fullscreenPlayOnButton->renderer()->repaint();
                Page* page = document()->page();
                if (m_miracastCardWasInvoked && page) {
                    // Close the Miracast card - it's not needed anymore.
                    // Passing an empty string to invokeCard() will close the
                    // current card if its target is on a whitelist allowing
                    // it to be closed. See closeChildCard() in invoke.utils.js.
                    // The value of m_miracastCardWasInvoked can sometimes be
                    // true when the card isn't actually up any more but this is
                    // not crucial since then invokeCard() won't close anything.
                    ChromeClient* client = page->chrome()->client();
                    client->invokeCard(BlackBerry::Platform::String::fromAscii(""));
                    m_miracastCardWasInvoked = false;
                }
            }
        }
    }
    m_presentationModePollTimer.startOneShot(s_pollTimeout);
}

}

#endif
