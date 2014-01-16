/*
 * Copyright (C) Research In Motion Limited 2013. All rights reserved.
 */

#ifndef SpellingHandler_h
#define SpellingHandler_h

#include "SpellChecker.h"
#include "TextIterator.h"

#include <imf/events.h>
#include <imf/input_data.h>
#include <wtf/RefPtr.h>

/**
 * SpellingHandler
 */

namespace WebCore {
class Range;
}

namespace BlackBerry {
namespace WebKit {

class InputHandler;

class SpellingHandler {
public:
    SpellingHandler(InputHandler*);
    ~SpellingHandler();

    // Called from WebCore::SpellChecker
    void requestCheckingOfString(PassRefPtr<WebCore::SpellCheckRequest>);
    void spellCheckingRequestProcessed(int32_t transactionId, spannable_string_t*);

    void spellCheckTextBlock(WebCore::Element*);
    void stopPendingSpellCheckingRequests(bool isRestartRequired = false);
    void cancelRequest();

    void setSystemSpellCheckStatus(bool enabled) { m_spellCheckStatusConfirmed = true; m_globalSpellCheckStatus = enabled; }

private:
    void createSpellCheckingRequest(PassRefPtr<WebCore::Range> rangeForSpellCheckingPtr);
    WebCore::SpellChecker* getSpellChecker();
    bool shouldSpellCheckElement(const WebCore::Element*) const;

    InputHandler* m_inputHandler;
    RefPtr<WebCore::SpellCheckRequest> m_request;
    int32_t m_processingTransactionId;
    int32_t m_minimumSpellCheckingRequestSequence;
    bool m_spellCheckStatusConfirmed;
    bool m_globalSpellCheckStatus;
};

} // WebKit
} // BlackBerry

#endif // SpellingHandler_h
