/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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

#include "config.h"

#if ENABLE(VIDEO)
#include "MediaDocument.h"

#include "DocumentLoader.h"
#include "EventNames.h"
#include "Frame.h"
#include "FrameLoaderClient.h"
#if PLATFORM(BLACKBERRY)
#include "FrameLoaderClientBlackBerry.h"
#include "FrameView.h"
#endif
#include "HTMLEmbedElement.h"
#include "HTMLHtmlElement.h"
#include "HTMLNames.h"
#include "HTMLSourceElement.h"
#include "HTMLVideoElement.h"
#include "KeyboardEvent.h"
#include "MainResourceLoader.h"
#include "NodeList.h"
#include "RawDataDocumentParser.h"
#include "ScriptController.h"

namespace WebCore {

using namespace HTMLNames;

// FIXME: Share more code with PluginDocumentParser.
class MediaDocumentParser : public RawDataDocumentParser {
public:
    static PassRefPtr<MediaDocumentParser> create(MediaDocument* document)
    {
        return adoptRef(new MediaDocumentParser(document));
    }

private:
    MediaDocumentParser(Document* document)
        : RawDataDocumentParser(document)
        , m_mediaElement(0)
    {
    }

    virtual void appendBytes(DocumentWriter*, const char*, size_t);

    void createDocumentStructure();

    HTMLMediaElement* m_mediaElement;
};

void MediaDocumentParser::createDocumentStructure()
{
    ExceptionCode ec;
    RefPtr<Element> rootElement = document()->createElement(htmlTag, false);
    document()->appendChild(rootElement, ec);
    document()->setCSSTarget(rootElement.get());
    static_cast<HTMLHtmlElement*>(rootElement.get())->insertedByParser();

    if (document()->frame())
        document()->frame()->loader()->dispatchDocumentElementAvailable();

    RefPtr<Element> body = document()->createElement(bodyTag, false);
    rootElement->appendChild(body, ec);

    RefPtr<Element> mediaElement;

#if PLATFORM(BLACKBERRY)
    const KURL& url = document()->url();

    String mimeType;
    if (DocumentLoader* loader = document()->loader())
        mimeType = loader->responseMIMEType();

    // Note that this check is not 100% reliable - some audio MIME types are not distinguishable
    // from video types. But if we misidentify the type, the worst that happens is we create a
    // video element instead of an audio element. In this case, the media will still play properly
    // but the wrong controls (the ones for embedded video content) will be displayed.
    // "audio/mpegurl" and "audio/x-mpegurl" can contain videos, be safe and treat them as videos.
    if (!mimeType.startsWith("audio") || mimeType.contains("mpegurl"))
        mediaElement = document()->createElement(videoTag, false);
    else
        mediaElement = document()->createElement(audioTag, false);

    m_mediaElement = static_cast<HTMLMediaElement*>(mediaElement.get());

    RefPtr<Element> headElement = document()->createElement(headTag, false);
    rootElement->appendChild(headElement);
    RefPtr<Element> meta = document()->createElement(metaTag, false);
    meta->setAttribute(nameAttr, "viewport");
    meta->setAttribute(contentAttr, "width=device-width, user-scalable=no");
    headElement->appendChild(meta, ec);

    Frame* frameForWidth = document()->frame();
    if (!frameForWidth)
        return;
    int layoutWidth = frameForWidth->view()->layoutWidth();
    char elementWidthAttr[12];
    sprintf(elementWidthAttr, "%d", layoutWidth * 3 / 4); // Make the default width 3/4 of the frame width.
    m_mediaElement->setAttribute(widthAttr, elementWidthAttr);
    m_mediaElement->setNeedsSourceWidthUpdate(300, 150); // Set to default size so we know to update the element width if we're playing a video and the size comes in with the metadata.

    // Use the name of the media file as the document title
    if (document()->title().isEmpty())
        if (url.isValid())
            document()->setTitle(decodeURLEscapeSequences(url.lastPathComponent()));
#else // !PLATFORM(BLACKBERRY)
    mediaElement = document()->createElement(videoTag, false);
    m_mediaElement = static_cast<HTMLMediaElement*>(mediaElement.get());
#endif


    m_mediaElement->setAttribute(controlsAttr, "");
    m_mediaElement->setAttribute(autoplayAttr, "");

    m_mediaElement->setAttribute(styleAttr, "margin: auto; position: absolute; top: 0; right: 0; bottom: 0; left: 0;");

    m_mediaElement->setAttribute(nameAttr, "media");

    RefPtr<Element> sourceElement = document()->createElement(sourceTag, false);
    HTMLSourceElement* source = static_cast<HTMLSourceElement*>(sourceElement.get());
    source->setSrc(document()->url());

    if (DocumentLoader* loader = document()->loader())
        source->setType(loader->responseMIMEType());

    m_mediaElement->appendChild(sourceElement, ec);
    body->appendChild(mediaElement, ec);

    Frame* frame = document()->frame();
    if (!frame)
        return;

    frame->loader()->activeDocumentLoader()->mainResourceLoader()->setShouldBufferData(DoNotBufferData);
}

void MediaDocumentParser::appendBytes(DocumentWriter*, const char*, size_t)
{
    if (m_mediaElement)
        return;

    createDocumentStructure();
    finish();

#if PLATFORM(BLACKBERRY)
    Document* doc = document();
    if (!doc)
        return;

    Frame* frame = doc->frame();
    if (!frame)
        return;

    FrameLoader* frameLoader = frame->loader();
    if (!frameLoader)
        return;

    FrameLoaderClientBlackBerry* client = static_cast<FrameLoaderClientBlackBerry*>(frameLoader->client());
    if (!client)
        return;

    // We want to cancel the loading of data now that the media element has been created and the renderer is loading
    // the data to play. WebKit no longer needs to load any more data here.
    client->setCancelLoadOnNextData();
#endif
}

MediaDocument::MediaDocument(Frame* frame, const KURL& url)
    : HTMLDocument(frame, url)
    , m_replaceMediaElementTimer(this, &MediaDocument::replaceMediaElementTimerFired)
{
    setCompatibilityMode(QuirksMode);
    lockCompatibilityMode();
}

MediaDocument::~MediaDocument()
{
    ASSERT(!m_replaceMediaElementTimer.isActive());
}

PassRefPtr<DocumentParser> MediaDocument::createParser()
{
    return MediaDocumentParser::create(this);
}

#if PLATFORM(BLACKBERRY)
static inline HTMLMediaElement* descendentMediaElement(Node* node)
{
    ASSERT(node);

    if (node->hasTagName(videoTag))
        return static_cast<HTMLMediaElement*>(node);

    if (node->hasTagName(audioTag))
        return static_cast<HTMLMediaElement*>(node);

    RefPtr<NodeList> nodeList = node->getElementsByTagNameNS(videoTag.namespaceURI(), videoTag.localName());

    if (nodeList.get()->length() > 0)
        return static_cast<HTMLMediaElement*>(nodeList.get()->item(0));

    nodeList = node->getElementsByTagNameNS(audioTag.namespaceURI(), audioTag.localName());

    if (nodeList.get()->length() > 0)
        return static_cast<HTMLMediaElement*>(nodeList.get()->item(0));

    return 0;
}
#endif

static inline HTMLVideoElement* descendentVideoElement(Node* node)
{
    ASSERT(node);

    if (node->hasTagName(videoTag))
        return static_cast<HTMLVideoElement*>(node);

    RefPtr<NodeList> nodeList = node->getElementsByTagNameNS(videoTag.namespaceURI(), videoTag.localName());

    if (nodeList.get()->length() > 0)
        return static_cast<HTMLVideoElement*>(nodeList.get()->item(0));

    return 0;
}

static inline HTMLVideoElement* ancestorVideoElement(Node* node)
{
    while (node && !node->hasTagName(videoTag))
        node = node->parentOrHostNode();

    return static_cast<HTMLVideoElement*>(node);
}

void MediaDocument::defaultEventHandler(Event* event)
{
#if !PLATFORM(BLACKBERRY)
    // Match the default Quicktime plugin behavior to allow 
    // clicking and double-clicking to pause and play the media.
    Node* targetNode = event->target()->toNode();
    if (!targetNode)
        return;

#if !PLATFORM(CHROMIUM)
    if (HTMLVideoElement* video = ancestorVideoElement(targetNode)) {
        if (event->type() == eventNames().clickEvent) {
            if (!video->canPlay()) {
                video->pause();
                event->setDefaultHandled();
            }
        } else if (event->type() == eventNames().dblclickEvent) {
            if (video->canPlay()) {
                video->play();
                event->setDefaultHandled();
            }
        }
    }
#endif

    if (event->type() == eventNames().keydownEvent && event->isKeyboardEvent()) {
        HTMLVideoElement* video = descendentVideoElement(targetNode);
        if (!video)
            return;

        KeyboardEvent* keyboardEvent = static_cast<KeyboardEvent*>(event);
        if (keyboardEvent->keyIdentifier() == "U+0020") { // space
            if (video->paused()) {
                if (video->canPlay())
                    video->play();
            } else
                video->pause();
            event->setDefaultHandled();
        }
    }
#endif
}

void MediaDocument::mediaElementSawUnsupportedTracks()
{
    // The HTMLMediaElement was told it has something that the underlying 
    // MediaPlayer cannot handle so we should switch from <video> to <embed> 
    // and let the plugin handle this. Don't do it immediately as this 
    // function may be called directly from a media engine callback, and 
    // replaceChild will destroy the element, media player, and media engine.
    m_replaceMediaElementTimer.startOneShot(0);
}

void MediaDocument::replaceMediaElementTimerFired(Timer<MediaDocument>*)
{
    HTMLElement* htmlBody = body();
    if (!htmlBody)
        return;

    // Set body margin width and height to 0 as that is what a PluginDocument uses.
    htmlBody->setAttribute(marginwidthAttr, "0");
    htmlBody->setAttribute(marginheightAttr, "0");

#if PLATFORM(BLACKBERRY)
    if (HTMLMediaElement* mediaElement = descendentMediaElement(htmlBody)) {
        RefPtr<Element> element = Document::createElement(embedTag, false);
        HTMLEmbedElement* embedElement = static_cast<HTMLEmbedElement*>(element.get());

        embedElement->setAttribute(widthAttr, "100%");
        embedElement->setAttribute(heightAttr, "100%");
        embedElement->setAttribute(nameAttr, "plugin");
        embedElement->setAttribute(srcAttr, url().string());

        DocumentLoader* documentLoader = loader();
        ASSERT(documentLoader);
        if (documentLoader)
            embedElement->setAttribute(typeAttr, documentLoader->writer()->mimeType());

        ExceptionCode ec;
        mediaElement->parentNode()->replaceChild(embedElement, mediaElement, ec);
    }
#else
    if (HTMLVideoElement* videoElement = descendentVideoElement(htmlBody)) {
        RefPtr<Element> element = Document::createElement(embedTag, false);
        HTMLEmbedElement* embedElement = static_cast<HTMLEmbedElement*>(element.get());

        embedElement->setAttribute(widthAttr, "100%");
        embedElement->setAttribute(heightAttr, "100%");
        embedElement->setAttribute(nameAttr, "plugin");
        embedElement->setAttribute(srcAttr, url().string());

        DocumentLoader* documentLoader = loader();
        ASSERT(documentLoader);
        if (documentLoader)
            embedElement->setAttribute(typeAttr, documentLoader->writer()->mimeType());

        ExceptionCode ec;
        videoElement->parentNode()->replaceChild(embedElement, videoElement, ec);
    }
#endif
}

}
#endif
