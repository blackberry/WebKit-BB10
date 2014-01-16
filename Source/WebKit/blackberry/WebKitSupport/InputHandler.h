/*
 * Copyright (C) 2009, 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
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

#ifndef InputHandler_h
#define InputHandler_h

#include "FloatPoint.h"
#include "Timer.h"

#include <BlackBerryPlatformInputEvents.h>
#include <BlackBerryPlatformMisc.h>
#include <BlackBerryPlatformSettings.h>

#include <imf/events.h>
#include <imf/input_data.h>
#include <map>
#include <pthread.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

namespace WebCore {
class AttributeTextStyle;
class Element;
class Frame;
class HTMLInputElement;
class HTMLSelectElement;
class IntRect;
class Node;
class Range;
class RenderObject;
class SuggestionBoxHandler;
}

namespace BlackBerry {

namespace Platform {
class IntPoint;
class KeyboardEvent;
}

namespace WebKit {

class SpellingHandler;
class WebPagePrivate;

class InputHandler {
public:
    InputHandler(WebPagePrivate*);
    ~InputHandler();

    enum FocusElementType { TextEdit, TextPopup /* Date/Time & Color */, SelectPopup, Plugin };
    enum CaretScrollType {
        CenterAlways = BlackBerry::Platform::Settings::ScrollAdjustmentCenterAlways,
        CenterIfNeeded = BlackBerry::Platform::Settings::ScrollAdjustmentCenterIfNeeded,
        EdgeIfNeeded = BlackBerry::Platform::Settings::ScrollAdjustmentEdgeIfNeeded
    };

    struct SpannableAttributeDetails {
        int start;
        int end;
        uint64_t attribute;
    };

    bool isInputModeEnabled() const;
    void setInputModeEnabled(bool active = true);

    void focusedNodeChanged();
    void nodeTextChanged(const WebCore::Node*);
    void selectionChanged();
    void frameUnloaded(const WebCore::Frame*);

    bool handleKeyboardInput(const BlackBerry::Platform::KeyboardEvent&, bool changeIsPartOfComposition = false);

    bool deleteSelection();
    void insertText(const WTF::String&);
    void clearField();

    void cut();
    void copy();
    void paste();
    void selectAll();

    void cancelSelection();

    void setInputValue(const WTF::String&);

    void focusNextField();
    void focusPreviousField();

    void setDelayKeyboardVisibilityChange(bool value);
    void processPendingKeyboardVisibilityChange();

    void notifyClientOfKeyboardVisibilityChange(bool visible, bool triggeredByFocusChange = false);
    void sendInputStateNotifications(bool sendFormStateNotification = false, bool sendOnlyIfSessionDisconnected = false);

    bool isInputMode() const { return isActiveTextEdit(); }
    bool isMultilineInputMode() const { return isActiveTextEdit() && elementType(m_currentFocusElement.get()) == BlackBerry::Platform::InputTypeTextArea; }
    bool isPasswordType() const { return isActiveTextEdit() && elementType(m_currentFocusElement.get()) == BlackBerry::Platform::InputTypePassword; }
    PassRefPtr<WebCore::Element> currentFocusElement() const { return m_currentFocusElement; }

    void ensureFocusElementVisible(bool centerFieldInDisplay = true);

    // PopupMenu methods.
    bool willOpenPopupForNode(WebCore::Node*);
    bool didNodeOpenPopup(WebCore::Node*);
    bool openLineInputPopup(WebCore::HTMLInputElement*);
    bool openSelectPopup(WebCore::HTMLSelectElement*);
    void setPopupListIndex(int);
    void setPopupListIndexes(int size, const bool* selecteds);

    bool processingChange() const { return m_processingChange; }
    void setProcessingChange(bool);

    WTF::String elementText();

    WebCore::RenderObject* rendererForInputField(const bool excludeContentEditable = false);
    WebCore::IntRect boundingBoxForInputField();

    bool isCaretAtEndOfText();

    // IMF driven calls.
    bool setBatchEditingActive(bool);
    bool getSelection(int* start, int* end);
    bool setSelection(int start, int end, bool changeIsPartOfComposition = false);
    int caretPosition() const;
    bool deleteTextRelativeToCursor(int leftOffset, int rightOffset);

    spannable_string_t* selectedText(int32_t flags);
    spannable_string_t* textBeforeCursor(int32_t length, int32_t flags);
    spannable_string_t* textAfterCursor(int32_t length, int32_t flags);
    extracted_text_t* extractedTextRequest(extracted_text_request_t*, int32_t flags);

    int32_t setComposingRegion(int32_t start, int32_t end);
    int32_t finishComposition();
    int32_t setComposingText(spannable_string_t*, int32_t relativeCursorPosition);
    int32_t commitText(spannable_string_t*, int32_t relativeCursorPosition);

    void inputSessionDisconnected();

    bool isTextSelected();
    bool isAllTextSelected();

    SpellingHandler* spellingHandler() { return m_spellingHandler; }
    bool shouldRequestSpellCheckingOptionsForPoint(const Platform::IntPoint& documentContentPosition, const WebCore::Element*, imf_sp_text_t&);
    void requestSpellCheckingOptions(imf_sp_text_t&, bool shouldMoveDialog = false);

    // Used for redrawing the spellcheck dialog on rotation
    void clearDidSpellCheckState() { m_didSpellCheckWord = false; }
    void redrawSpellCheckDialogIfRequired(bool shouldMoveDialog = true);

    void elementTouched(WebCore::Element*);
    void restoreViewState();

    static bool convertStringToWchar(const WTF::String&, wchar_t* dest, int destCapacity, int* destLength);
    static bool convertStringToWcharVector(const WTF::String&, WTF::Vector<wchar_t>& wcharString);
    static WTF::String convertSpannableStringToString(spannable_string_t* src);

private:
    friend class SpellingHandler;

    enum PendingKeyboardStateChange { NoChange, Visible, NotVisible };

    void setElementFocused(WebCore::Element*);
    void setPluginFocused(WebCore::Element*);
    void setElementUnfocused(bool refocusOccuring = false);

    void ensureFocusTextElementVisible(CaretScrollType = CenterAlways);
    void ensureFocusPluginElementVisible();

    void clearCurrentFocusElement();

    bool selectionAtStartOfElement();
    bool selectionAtEndOfElement();

    WebCore::IntRect rectForCaret(int index);

    bool isActiveTextEdit() const { return m_currentFocusElement && m_currentFocusElementType == TextEdit; }
    bool isActiveTextPopup() const { return m_currentFocusElement && m_currentFocusElementType == TextPopup; }
    bool isActiveSelectPopup() const { return m_currentFocusElement && m_currentFocusElementType == SelectPopup; }
    bool isActivePlugin() const { return m_currentFocusElement && m_currentFocusElementType == Plugin; }

    bool openDatePopup(WebCore::HTMLInputElement*, BlackBerry::Platform::BlackBerryInputType);
    bool openColorPopup(WebCore::HTMLInputElement*);

    bool executeTextEditCommand(const WTF::String&);

    BlackBerry::Platform::BlackBerryInputType elementType(WebCore::Element*) const;

    int selectionStart() const;
    int selectionEnd() const;
    int selectionPosition(bool start) const;
    bool selectionActive() const { return selectionStart() != selectionEnd(); }

    bool compositionActive() const { return compositionLength(); }
    unsigned compositionLength() const { return m_composingTextEnd - m_composingTextStart; }

    spannable_string_t* spannableTextInRange(int start, int end, int32_t flags);

    void addAttributedTextMarker(int start, int end, const WebCore::AttributeTextStyle&);
    void removeAttributedTextMarker(bool clearComposingRegion = true);

    bool deleteText(int start, int end);
    bool setTextAttributes(int insertionPoint, spannable_string_t*);
    bool setText(spannable_string_t*);
    bool setSpannableTextAndRelativeCursor(spannable_string_t*, int relativeCursorPosition, bool markTextAsComposing);
    bool removeComposedText();
    bool setRelativeCursorPosition(int insertionPoint, int relativeCursorPosition);
    bool setCursorPosition(int location);

    span_t* firstSpanInString(spannable_string_t*, uint64_t);
    bool isTrailingSingleCharacter(span_t*, unsigned, unsigned);

    void learnText();
    void sendLearnTextDetails(const WTF::String&);
    bool didSpellCheckWord() const { return m_didSpellCheckWord; }

    void updateFormState();

    bool shouldNotifyWebView(const Platform::KeyboardEvent&);
    void attributeTimerFired(WebCore::Timer<InputHandler> *);

    void showTextInputTypeSuggestionBox(bool allowEmptyPrefix = false);
    void hideTextInputTypeSuggestionBox();

    bool isNavigationKey(unsigned character) const;

    WebPagePrivate* m_webPage;

    RefPtr<WebCore::Element> m_currentFocusElement;
    RefPtr<WebCore::Element> m_previousFocusableTextElement;
    RefPtr<WebCore::Element> m_nextFocusableTextElement;

    bool m_inputModeEnabled;

    bool m_processingChange;
    bool m_shouldEnsureFocusTextElementVisibleOnSelectionChanged;

    WebCore::Timer<InputHandler> m_attributeTimer;

    FocusElementType m_currentFocusElementType;
    int64_t m_currentFocusElementTextEditMask;
    BlackBerry::Platform::VirtualKeyboardType m_keyboardType;
    BlackBerry::Platform::VirtualKeyboardEnterKeyType m_enterKeyType;

    bool m_sessionDisconnected;

    int m_composingTextStart;
    int m_composingTextEnd;

    PendingKeyboardStateChange m_pendingKeyboardVisibilityChange;
    bool m_delayKeyboardVisibilityChange;
    bool m_sendFormStateOnNextKeyboardRequest;

    bool m_shouldNotifyWebView;
    unsigned m_expectedKeyUpChar;

    OwnPtr<WebCore::SuggestionBoxHandler> m_suggestionDropdownBoxHandler;

    imf_sp_text_t m_spellCheckingOptionsRequest;
    bool m_didSpellCheckWord;
    SpellingHandler* m_spellingHandler;
    bool m_isFocusChange;

    WTF::Vector<SpannableAttributeDetails> m_cachedSpannableAttributes;
    bool m_elementTouchedIsCrossFrame;

    DISABLE_COPY(InputHandler);
};

}
}

#endif // InputHandler_h
