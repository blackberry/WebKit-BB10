/*
 * Copyright (C) Research In Motion Limited 2013. All rights reserved.
 */

#include "config.h"
#include "SpellingHandler.h"

#include "DOMSupport.h"
#include "Frame.h"
#include "InputHandler.h"
#include "Range.h"
#include "SpellChecker.h"
#include "VisibleUnits.h"
#include "WebPage_p.h"

#include <BlackBerryPlatformIMF.h>
#include <BlackBerryPlatformLog.h>
#include <BlackBerryPlatformStopWatch.h>

#define ENABLE_SPELLING_LOG 0

using namespace BlackBerry::Platform;
using namespace WebCore;

#if ENABLE_SPELLING_LOG
#define SpellingLog(severity, format, ...) Platform::logAlways(severity, format, ## __VA_ARGS__)
#else
#define SpellingLog(severity, format, ...)
#endif // ENABLE_SPELLING_LOG

namespace BlackBerry {
namespace WebKit {

static const int s_HalfMaxSpellCheckingStringLength = (MaxSpellCheckingStringLength - 1) / 2;

SpellingHandler::SpellingHandler(InputHandler* inputHandler)
    : m_inputHandler(inputHandler)
    , m_request(0)
    , m_processingTransactionId(-1)
    , m_minimumSpellCheckingRequestSequence(-1)
    , m_spellCheckStatusConfirmed(false)
    , m_globalSpellCheckStatus(false)
{
}

SpellingHandler::~SpellingHandler()
{
}

void SpellingHandler::spellCheckTextBlock(WebCore::Element* element)
{
    SpellingLog(Platform::LogLevelInfo, "SpellingHandler::spellCheckTextBlock");

#if ENABLE_SPELLING_LOG
    BlackBerry::Platform::StopWatch timer;
    timer.start();
#endif

    if (!(element && element->document() && element->document()->frame() && element->document()->frame()->selection()))
        return;

    VisiblePosition caretPosition = element->document()->frame()->selection()->start();
    // Expand out from the current caret position in both directions
    int offsetFromStart = DOMSupport::offsetFromStartOfBlock(caretPosition);
    int totalLength = DOMSupport::inputElementText(element).length();
    int offsetFromStartWithPadding = offsetFromStart + s_HalfMaxSpellCheckingStringLength;
    int offsetFromStartLessPadding = offsetFromStart - s_HalfMaxSpellCheckingStringLength;

    // If expanding out reaches a boundary [0 or totalLength], slide the window over to allow for more text to be checked.
    int start = std::max(0, (offsetFromStartWithPadding > totalLength) ? static_cast<int>(totalLength - MaxSpellCheckingStringLength + 1) : offsetFromStartLessPadding);
    int end = std::min(totalLength, (offsetFromStartLessPadding < 0) ? static_cast<int>(MaxSpellCheckingStringLength - 1) : offsetFromStartWithPadding);

    SpellingLog(Platform::LogLevelInfo, "SpellingHandler::spellCheckTextBlock start %d, end %d", start, end);
    if (end <= start)
        return;

    VisibleSelection visibleSelectionForSpellChecking = DOMSupport::visibleSelectionForRangeInputElement(element, start, end);

    // Make sure we don't fall in the middle of a word when creating the range
    RefPtr<Range> rangeForSpellChecking = makeRange(endOfWord(visibleSelectionForSpellChecking.visibleStart(), LeftWordIfOnBoundary)
        , startOfWord(visibleSelectionForSpellChecking.visibleEnd(), RightWordIfOnBoundary));

    if (!(rangeForSpellChecking && rangeForSpellChecking->text().length() < MaxSpellCheckingStringLength))
        return;

    createSpellCheckingRequest(rangeForSpellChecking);

#if ENABLE_SPELLING_LOG
    SpellingLog(Platform::LogLevelInfo, "SpellingHandler::spellCheckTextBlock spellchecking took %lf seconds", timer.elapsed());
#endif
}

void SpellingHandler::createSpellCheckingRequest(const PassRefPtr<WebCore::Range> rangeForSpellCheckingPtr)
{
    SpellingLog(Platform::LogLevelInfo, "SpellingHandler::createSpellCheckingRequest length %d, '%s'", rangeForSpellCheckingPtr->text().length(), rangeForSpellCheckingPtr->text().latin1().data());
    if (rangeForSpellCheckingPtr->text().length() >= MinSpellCheckingStringLength) {
        if (SpellChecker* spellChecker = getSpellChecker()) {
            RefPtr<WebCore::Range> rangeForSpellChecking = rangeForSpellCheckingPtr;
            spellChecker->requestCheckingFor(SpellCheckRequest::create(TextCheckingTypeSpelling, TextCheckingProcessIncremental, rangeForSpellChecking, rangeForSpellChecking));
        }
    }
}

void SpellingHandler::requestCheckingOfString(PassRefPtr<WebCore::SpellCheckRequest> spellCheckRequest)
{
    SpellingLog(Platform::LogLevelInfo, "SpellingHandler::requestCheckingOfString '%s'", spellCheckRequest->data().text().latin1().data());

    if (!spellCheckRequest) {
        SpellingLog(Platform::LogLevelWarn, "SpellingHandler::requestCheckingOfString did not receive a valid request.");
        return;
    }

    if (spellCheckRequest->data().sequence() <= m_minimumSpellCheckingRequestSequence) {
        SpellingLog(Platform::LogLevelWarn, "SpellingHandler::requestCheckingOfString rejecting stale request with sequenceId=%d. Sentinal currently at %d."
            , spellCheckRequest->data().sequence(), m_minimumSpellCheckingRequestSequence);
        spellCheckRequest->didCancel();
        return;
    }

    unsigned requestLength = spellCheckRequest->data().text().length();

    // Check if the field should be spellchecked.
    if (!shouldSpellCheckElement(m_inputHandler->currentFocusElement().get()) || requestLength < MinSpellCheckingStringLength) {
        SpellingLog(Platform::LogLevelWarn, "SpellingHandler::requestCheckingOfString request cancelled");
        spellCheckRequest->didCancel();
        return;
    }

    if (requestLength >= MaxSpellCheckingStringLength) {
        // Cancel this request and send it off in newly created chunks.
        spellCheckRequest->didCancel();
        spellCheckTextBlock(m_inputHandler->currentFocusElement().get());
        return;
    }

    wchar_t* checkingString = (wchar_t*)malloc(sizeof(wchar_t) * (requestLength + 1));
    if (!checkingString) {
        Platform::logAlways(Platform::LogLevelCritical, "SpellingHandler::requestCheckingOfString Cannot allocate memory for string.");
        spellCheckRequest->didCancel();
        return;
    }

    int paragraphLength = 0;

    if (!InputHandler::convertStringToWchar(spellCheckRequest->data().text(), checkingString, requestLength + 1, &paragraphLength)) {
        Platform::logAlways(Platform::LogLevelCritical, "SpellingHandler::requestCheckingOfString Failed to convert String to wchar type.");
        free(checkingString);
        spellCheckRequest->didCancel();
        return;
    }

    m_processingTransactionId = m_inputHandler->m_webPage->m_client->checkSpellingOfStringAsync(checkingString, static_cast<unsigned>(paragraphLength));
    free(checkingString);

    // If the call to the input service did not go through, then cancel the request so we don't block endlessly.
    // This should still take transactionId as a parameter to maintain the same behavior as if InputMethodSupport
    // were to cancel a request during processing.
    if (m_processingTransactionId == -1) { // Error before sending request to input service.
        spellCheckRequest->didCancel();
        return;
    }

    m_request = spellCheckRequest;
}

void SpellingHandler::spellCheckingRequestProcessed(int32_t transactionId, spannable_string_t* spannableString)
{
#if !ENABLE_SPELLING_LOG
    UNUSED_PARAM(transactionId)
#endif

    SpellingLog(Platform::LogLevelWarn,
        "SpellingHandler::spellCheckingRequestProcessed Expected transaction id %d, received %d. %s",
        m_processingTransactionId,
        transactionId,
        transactionId == m_processingTransactionId ? "" : "We are out of sync with input service.");

    if (!spannableString
        || !m_inputHandler->isActiveTextEdit()
        || !DOMSupport::elementHasContinuousSpellCheckingEnabled(m_inputHandler->currentFocusElement().get())
        || !m_processingTransactionId
        || !m_request) {
        SpellingLog(Platform::LogLevelWarn, "SpellingHandler::spellCheckingRequestProcessed Cancelling request with transactionId %d.", transactionId);
        if (m_request) {
            m_request->didCancel();
            m_request = 0;
        }
        if (spannableString) {
            // free data that we malloc'ed in InputMethodSupport
            free(spannableString->spans);
            free(spannableString);
        }
        m_processingTransactionId = -1;
        return;
    }

    Vector<TextCheckingResult> results;

    // Convert the spannableString to TextCheckingResult then append to results vector.
    WTF::String replacement;
    TextCheckingResult textCheckingResult;
    textCheckingResult.type = TextCheckingTypeSpelling;
    textCheckingResult.replacement = replacement;
    textCheckingResult.location = 0;
    textCheckingResult.length = 0;

    span_t* span = spannableString->spans;
    for (unsigned i = 0; i < spannableString->spans_count; i++) {
        if (!span)
            break;
        if (span->end < span->start) {
            m_request->didCancel();
            m_request = 0;
            // free data that we malloc'ed in InputMethodSupport
            free(spannableString->spans);
            free(spannableString);
            return;
        }
        if (span->attributes_mask & MISSPELLED_WORD_ATTRIB) {
            textCheckingResult.location = span->start;
            // The end point includes the character that it is before. Ie, 0, 0
            // applies to the first character as the end point includes the character
            // at the position. This means the endPosition is always +1.
            textCheckingResult.length = span->end - span->start + 1;
            results.append(textCheckingResult);
        }
        span++;
    }

    // free data that we malloc'ed in InputMethodSupport
    free(spannableString->spans);
    free(spannableString);

    m_request->didSucceed(results);
    m_request = 0;
}

void SpellingHandler::stopPendingSpellCheckingRequests(bool isRestartRequired)
{
    // Prevent response from propagating through.
    m_processingTransactionId = 0;

    // Reject requests until lastRequestSequence. This helps us clear the queue of stale requests.
    if (SpellChecker* spellChecker = getSpellChecker()) {
        if (spellChecker->lastRequestSequence() != spellChecker->lastProcessedSequence()) {
            SpellingLog(LogLevelInfo, "SpellingHandler::stopPendingSpellCheckingRequests will block requests up to lastRequest=%d [lastProcessed=%d]"
                , spellChecker->lastRequestSequence(), spellChecker->lastProcessedSequence());
            // Prevent requests in queue from executing.
            m_minimumSpellCheckingRequestSequence = spellChecker->lastRequestSequence();
            // Create new spellcheck requests to replace those that were invalidated.
            if (isRestartRequired && !m_inputHandler->compositionActive())
                spellCheckTextBlock(m_inputHandler->currentFocusElement().get());
        }
    }
}

void SpellingHandler::cancelRequest()
{
    // Cancel any preexisting spellcheck requests.
    if (m_request) {
        stopPendingSpellCheckingRequests();
        m_request->didCancel();
        m_request = 0;
    }
}

bool SpellingHandler::shouldSpellCheckElement(const Element* element) const
{
    if (!element)
        return false;

    DOMSupport::AttributeState spellCheckAttr = DOMSupport::elementSupportsSpellCheck(element);

    // Explicitly set to off.
    if (spellCheckAttr == DOMSupport::Off)
        return false;

    // Undefined and part of a set of cases which we do not wish to check. This includes user names and email addresses, so we are piggybacking on NoAutocomplete cases.
    if (spellCheckAttr == DOMSupport::Default && m_inputHandler->m_currentFocusElementTextEditMask & NO_AUTO_TEXT)
        return false;

    // Check if the system spell check setting is off
    return m_spellCheckStatusConfirmed ? m_globalSpellCheckStatus : true;
}

SpellChecker* SpellingHandler::getSpellChecker()
{
    Element* element = m_inputHandler->currentFocusElement().get();
    if (!element || !element->document())
        return 0;

    if (Frame* frame = element->document()->frame())
        if (Editor* editor = frame->editor())
            return editor->spellChecker();

    return 0;
}

} // WebKit
} // BlackBerry
