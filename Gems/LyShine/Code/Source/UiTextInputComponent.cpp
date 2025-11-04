/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiTextInputComponent.h"

#include <AzCore/Math/Crc.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Time/ITime.h>

#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>

#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiTransformBus.h>
#include <LyShine/Bus/UiVisualBus.h>
#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiTextBus.h>
#include <LyShine/ISprite.h>

#include <LyShine/IDraw2d.h>
#include <LyShine/UiSerializeHelpers.h>


#include "UiNavigationHelpers.h"
#include "UiSerialize.h"
#include "Sprite.h"
#include "StringUtfUtils.h"
#include "UiClipboard.h"

namespace
{
    // Orange color from the canvas editor style guide
    const AZ::Color defaultSelectionColor(255.0f / 255.0f, 153.0f / 255.0f, 0.0f / 255.0f, 1.0f);
    // White color from the canvas editor style guide
    const AZ::Color defaultCursorColor(238.0f / 255.0f, 238.0f / 255.0f, 238.0f / 255.0f, 1.0f);

    const uint32_t defaultReplacementChar('*');

    // Add all descendant elements that support the UiTextBus to a list of pairs of
    // entity ID and string.
    void AddDescendantTextElements(AZ::EntityId entity,
        UiTextInputComponent::EntityComboBoxVec& result)
    {
        // Get a list of all descendant elements that support the UiTextBus
        LyShine::EntityArray matchingElements;
        UiElementBus::Event(
            entity,
            &UiElementBus::Events::FindDescendantElements,
            [](const AZ::Entity* descendant)
            {
                return UiTextBus::FindFirstHandler(descendant->GetId()) != nullptr;
            },
            matchingElements);

        // add their names to the StringList and their IDs to the id list
        for (auto childEntity : matchingElements)
        {
            result.push_back(AZStd::make_pair(AZ::EntityId(childEntity->GetId()), childEntity->GetName()));
        }
    }

    //! \brief Given a UTF8 string and index, return the raw string buffer index that maps to the UTF8 index.
    int GetCharArrayIndexFromUtf8CharIndex(const AZStd::string& utf8String, const uint utf8Index)
    {
        uint utfIndexIter = 0;
        int rawIndex = 0;

        const AZStd::string::size_type stringLength = utf8String.length();
        if (stringLength > 0 && stringLength >= utf8Index)
        {
            // Iterate over the string until the given index is found.
            Utf8::Unchecked::octet_iterator pChar(utf8String.data());
            while (uint32_t ch = *pChar)
            {
                if (utf8Index == utfIndexIter)
                {
                    break;
                }
                ++utfIndexIter;

                // Add up the size of the multibyte chars along the way,
                // which will give us the "raw" string buffer index of where
                // the given index maps to.
                rawIndex += LyShine::GetMultiByteCharSize(ch);

                ++pChar;
            }
        }

        return rawIndex;
    }

    //! \brief Removes a range of UTF8 code points using the given indices.
    //! The given indices are code-point indices and not raw (byte) indices.
    void RemoveUtf8CodePointsByIndex(AZStd::string& utf8String, int index1, int index2)
    {
        const int minSelectIndex = min(index1, index2);
        const int maxSelectIndex = max(index1, index2);
        const int left = GetCharArrayIndexFromUtf8CharIndex(utf8String, minSelectIndex);
        const int right = GetCharArrayIndexFromUtf8CharIndex(utf8String, maxSelectIndex);
        utf8String.erase(left, right - left);
    }

    //! \brief Returns a UTF8 sub-string using the given indices.
    //! The given indices are code-point indices and not raw (byte) indices.
    AZStd::string Utf8SubString(const AZStd::string& utf8String, int utf8CharIndexStart, int utf8CharIndexEnd)
    {
        const int minCharIndex = min(utf8CharIndexStart, utf8CharIndexEnd);
        const int maxCharIndex = max(utf8CharIndexStart, utf8CharIndexEnd);
        const int left = GetCharArrayIndexFromUtf8CharIndex(utf8String, minCharIndex);
        const int right = GetCharArrayIndexFromUtf8CharIndex(utf8String, maxCharIndex);
        return utf8String.substr(left, right - left);
    }

    //! \brief Convenience method for erasing a range of text and updating the given selection indices accordingly.
    void EraseAndUpdateSelectionRange(AZStd::string& utf8String, int& endSelectIndex, int& startSelectIndex)
    {
        RemoveUtf8CodePointsByIndex(utf8String, endSelectIndex, startSelectIndex);
        endSelectIndex = startSelectIndex = min(endSelectIndex, startSelectIndex);
    }
}   // anonymous namespace

////////////////////////////////////////////////////////////////////////////////////////////////////
//! UiTextInputNotificationBus Behavior context handler class
class BehaviorUiTextInputNotificationBusHandler
    : public UiTextInputNotificationBus::Handler
    , public AZ::BehaviorEBusHandler
{
public:
    AZ_EBUS_BEHAVIOR_BINDER(BehaviorUiTextInputNotificationBusHandler, "{5ED20B32-95E2-4EBB-8874-7E780306F7F0}", AZ::SystemAllocator,
        OnTextInputChange, OnTextInputEndEdit, OnTextInputEnter);

    void OnTextInputChange(const AZStd::string& textString) override
    {
        Call(FN_OnTextInputChange, textString);
    }

    void OnTextInputEndEdit(const AZStd::string& textString) override
    {
        Call(FN_OnTextInputEndEdit, textString);
    }

    void OnTextInputEnter(const AZStd::string& textString) override
    {
        Call(FN_OnTextInputEnter, textString);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTextInputComponent::UiTextInputComponent()
    : m_isDragging(false)
    , m_isEditing(false)
    , m_isTextInputStarted(false)
    , m_textCursorPos(-1)
    , m_textSelectionStartPos(-1)
    , m_cursorBlinkStartTime(0.0f)
    , m_textEntity()
    , m_placeHolderTextEntity()
    , m_textSelectionColor(defaultSelectionColor)
    , m_textCursorColor(defaultCursorColor)
    , m_maxStringLength(-1)
    , m_cursorBlinkInterval(1.0f)
    , m_childTextStateDirtyFlag(true)
    , m_onChange(nullptr)
    , m_onEndEdit(nullptr)
    , m_onEnter(nullptr)
    , m_replacementCharacter(defaultReplacementChar)
    , m_isPasswordField(false)
    , m_clipInputText(true)
    , m_enableClipboard(true)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTextInputComponent::~UiTextInputComponent()
{
    if (m_isEditing)
    {
        AzFramework::InputTextEntryRequestBus::Broadcast(&AzFramework::InputTextEntryRequests::TextEntryStop);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiTextInputComponent::HandlePressed(AZ::Vector2 point, bool& shouldStayActive)
{
    bool handled = UiInteractableComponent::HandlePressed(point, shouldStayActive);

    if (handled)
    {
        // clear the dragging flag, we are not dragging until we detect a drag
        m_isDragging = false;

        // the text input field will stay active after released
        shouldStayActive = true;

        // store the character position where the press corresponds to in the text string
        UiTextBus::EventResult(m_textCursorPos, m_textEntity, &UiTextBus::Events::GetCharIndexFromPoint, point, false);
        m_textSelectionStartPos = m_textCursorPos;
    }

    ResetCursorBlink();

    return handled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiTextInputComponent::HandleReleased(AZ::Vector2 point)
{
    m_isPressed = false;
    m_isDragging = false;

    if (!m_isHandlingEvents)
    {
        return false;
    }

    if (!m_isEditing)
    {
        bool isInRect = false;
        UiTransformBus::EventResult(isInRect, GetEntityId(), &UiTransformBus::Events::IsPointInRect, point);
        if (isInRect)
        {
            BeginEditState();
        }
        else
        {
            // cancel the active status
            UiInteractableActiveNotificationBus::Event(GetEntityId(), &UiInteractableActiveNotificationBus::Events::ActiveCancelled);
        }
    }

    CheckStartTextInput();

    UiInteractableComponent::TriggerReleasedAction();

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiTextInputComponent::HandleEnterPressed(bool& shouldStayActive)
{
    bool handled = UiInteractableComponent::HandleEnterPressed(shouldStayActive);

    if (handled)
    {
        // the text input field will stay active after released
        shouldStayActive = true;

        AZStd::string textString;
        UiTextBus::EventResult(textString, m_textEntity, &UiTextBus::Events::GetText);

        // select all the text
        m_textCursorPos = 0;
        m_textSelectionStartPos = LyShine::GetUtf8StringLength(textString);
    }

    return handled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiTextInputComponent::HandleEnterReleased()
{
    m_isPressed = false;

    if (!m_isHandlingEvents)
    {
        return false;
    }

    if (!m_isEditing)
    {
        BeginEditState();
    }

    CheckStartTextInput();

    UiInteractableComponent::TriggerReleasedAction();

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiTextInputComponent::HandleAutoActivation()
{
    if (!m_isHandlingEvents)
    {
        return false;
    }

    AZStd::string textString;
    UiTextBus::EventResult(textString, m_textEntity, &UiTextBus::Events::GetText);
    m_textCursorPos = LyShine::GetUtf8StringLength(textString);
    m_textSelectionStartPos = m_textCursorPos;

    if (!m_isEditing)
    {
        BeginEditState();
    }

    CheckStartTextInput();

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiTextInputComponent::HandleTextInput(const AZStd::string& inputTextUTF8)
{
    if (!m_isHandlingEvents)
    {
        return false;
    }

    // don't accept text input while in pressed state
    if (m_isPressed)
    {
        return false;
    }

    AZStd::string currentText;
    UiTextBus::EventResult(currentText, m_textEntity, &UiTextBus::Events::GetText);

    bool changedText = false;

    if (inputTextUTF8 == "\b" || inputTextUTF8 == "\x7f")
    {
        // backspace pressed, delete character before cursor or the selected range
        if (m_textCursorPos > 0 || m_textCursorPos != m_textSelectionStartPos)
        {
            if (m_textCursorPos != m_textSelectionStartPos)
            {
                // range is selected
                EraseAndUpdateSelectionRange(currentText, m_textCursorPos, m_textSelectionStartPos);
            }
            else
            {
                // "Select" one codepoint to erase (via backspace)
                m_textSelectionStartPos = m_textCursorPos - 1;
                EraseAndUpdateSelectionRange(currentText, m_textCursorPos, m_textSelectionStartPos);
            }
            UiTextBus::Event(
                m_textEntity, &UiTextBus::Events::SetSelectionRange, m_textSelectionStartPos, m_textCursorPos, m_textCursorColor);

            changedText = true;
        }
    }
    // if inputTextUTF8 is a control character (a non printing character such as esc or tab) ignore it
    else if (inputTextUTF8.size() != 1 || !AZStd::is_cntrl(inputTextUTF8.at(0)))
    {
        // note currently we are treating the wchar passed in as a char, for localization
        // we need to use a wide string or utf8 string
        if (m_textCursorPos >= 0)
        {
            // if a range is selected then erase that first
            if (m_textCursorPos != m_textSelectionStartPos)
            {
                EraseAndUpdateSelectionRange(currentText, m_textCursorPos, m_textSelectionStartPos);
                changedText = true;
            }

            // only allow text to be added if there is no length limit or the length is under the limit
            if (m_maxStringLength < 0 || currentText.length() < m_maxStringLength)
            {
                int rawIndexPos = GetCharArrayIndexFromUtf8CharIndex(currentText, m_textCursorPos);

                if (rawIndexPos >= 0)
                {
                    currentText.insert(rawIndexPos, inputTextUTF8);

                    m_textCursorPos++;
                    m_textSelectionStartPos = m_textCursorPos;
                    UiTextBus::Event(
                        m_textEntity, &UiTextBus::Events::SetSelectionRange, m_textSelectionStartPos, m_textCursorPos, m_textCursorColor);
                    changedText = true;
                }
            }
        }
    }

    if (changedText)
    {
        ChangeText(currentText);
        ResetCursorBlink();
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiTextInputComponent::HandleKeyInputBegan(const AzFramework::InputChannel::Snapshot& inputSnapshot, AzFramework::ModifierKeyMask activeModifierKeys)
{
    if (!m_isHandlingEvents)
    {
        return false;
    }

    // don't accept character input while in pressed state
    if (m_isPressed)
    {
        return false;
    }

    bool result = true;

    int oldTextCursorPos = m_textCursorPos;
    int oldTextSelectionStartPos = m_textSelectionStartPos;

    const bool isShiftModifierActive = (static_cast<int>(activeModifierKeys) & static_cast<int>(AzFramework::ModifierKeyMask::ShiftAny)) != 0;
    const bool isLCTRLModifierActive = (static_cast<int>(activeModifierKeys) & static_cast<int>(AzFramework::ModifierKeyMask::CtrlAny)) != 0;
    const UiNavigationHelpers::Command command = UiNavigationHelpers::MapInputChannelIdToUiNavigationCommand(inputSnapshot.m_channelId, activeModifierKeys);
    if (command == UiNavigationHelpers::Command::Enter)
    {
        // enter was pressed

        AZStd::string textString;
        UiTextBus::EventResult(textString, m_textEntity, &UiTextBus::Events::GetText);

        // if a C++ callback is registered for OnEnter then call it
        if (m_onEnter)
        {
            // pass the entered text string to the C++ callback
            m_onEnter(GetEntityId(), textString);
        }

        // Tell any action listeners about the event
        if (!m_enterAction.empty())
        {
            // canvas listeners will get the action name (e.g. something like "EmailEntered") plus
            // the ID of this entity.
            AZ::EntityId canvasEntityId;
            UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
            UiCanvasNotificationBus::Event(canvasEntityId, &UiCanvasNotificationBus::Events::OnAction, GetEntityId(), m_enterAction);
        }

        UiTextInputNotificationBus::Event(GetEntityId(), &UiTextInputNotificationBus::Events::OnTextInputEnter, textString);

        // cancel the active status
        UiInteractableActiveNotificationBus::Event(GetEntityId(), &UiInteractableActiveNotificationBus::Events::ActiveCancelled);
        EndEditState();
    }
    else if (inputSnapshot.m_channelId == AzFramework::InputDeviceKeyboard::Key::NavigationDelete)
    {
        AZStd::string textString;
        UiTextBus::EventResult(textString, m_textEntity, &UiTextBus::Events::GetText);

        // Delete pressed, delete character after cursor or the selected range
        if (m_textCursorPos < LyShine::GetUtf8StringLength(textString) || m_textCursorPos != m_textSelectionStartPos)
        {
            if (m_textCursorPos != m_textSelectionStartPos)
            {
                // range is selected
                EraseAndUpdateSelectionRange(textString, m_textCursorPos, m_textSelectionStartPos);
            }
            else
            {
                // no range selected - delete character after cursor
                RemoveUtf8CodePointsByIndex(textString, m_textCursorPos, m_textCursorPos + 1);
            }

            ChangeText(textString);
        }
    }
    else if (command == UiNavigationHelpers::Command::Left || command == UiNavigationHelpers::Command::Right)
    {
        if (m_textCursorPos != m_textSelectionStartPos)
        {
            // Range is selected
            if (isShiftModifierActive)
            {
                // Move cursor to change selected range
                AZStd::string textString;
                UiTextBus::EventResult(textString, m_textEntity, &UiTextBus::Events::GetText);

                if (command == UiNavigationHelpers::Command::Left)
                {
                    if (m_textCursorPos > 0)
                    {
                        --m_textCursorPos;
                    }
                }
                else // UiNavigationHelpers::Command::Right
                {
                    if (m_textCursorPos < LyShine::GetUtf8StringLength(textString))
                    {
                        ++m_textCursorPos;
                    }
                }
            }
            else
            {
                // Place cursor at start or end of selection
                if (command == UiNavigationHelpers::Command::Left)
                {
                    m_textCursorPos = min(m_textCursorPos, m_textSelectionStartPos);
                }
                else // eKI_Right
                {
                    m_textCursorPos = max(m_textCursorPos, m_textSelectionStartPos);
                }
                m_textSelectionStartPos = m_textCursorPos;
            }
        }
        else
        {
            // No range selected, move cursor one character
            AZStd::string textString;
            UiTextBus::EventResult(textString, m_textEntity, &UiTextBus::Events::GetText);

            if (command == UiNavigationHelpers::Command::Left)
            {
                if (m_textCursorPos > 0)
                {
                    --m_textCursorPos;
                }
            }
            else // eKI_Right
            {
                if (m_textCursorPos < LyShine::GetUtf8StringLength(textString))
                {
                    ++m_textCursorPos;
                }
            }

            if (!isShiftModifierActive)
            {
                m_textSelectionStartPos = m_textCursorPos;
            }
        }
    }
    else if (command == UiNavigationHelpers::Command::Up || command == UiNavigationHelpers::Command::Down)
    {
        AZ::Vector2 currentPosition;
        UiTextBus::EventResult(currentPosition, m_textEntity, &UiTextBus::Events::GetPointFromCharIndex, m_textCursorPos);

        float fontSize;
        UiTextBus::EventResult(fontSize, m_textEntity, &UiTextBus::Events::GetFontSize);

        // To get the position of the cursor on the line above or below the
        // current cursor position, we add or subtract the font size,
        // depending on whether arrow key up or down is provided.
        if (command == UiNavigationHelpers::Command::Up)
        {
            fontSize *= -1.0f;
        }

        // Get the index that matches closest to the position directly above
        // or below the current cursor position.
        currentPosition.SetY(currentPosition.GetY() + fontSize);
        int adjustedIndex = 0;
        UiTextBus::EventResult(adjustedIndex, m_textEntity, &UiTextBus::Events::GetCharIndexFromCanvasSpacePoint, currentPosition, true);

        if (adjustedIndex != -1)
        {
            if (isShiftModifierActive)
            {
                m_textCursorPos = adjustedIndex;
            }
            else
            {
                result = m_textCursorPos != adjustedIndex;
                m_textCursorPos = m_textSelectionStartPos = adjustedIndex;
            }

            UiTextBus::Event(
                m_textEntity, &UiTextBus::Events::SetSelectionRange, m_textSelectionStartPos, m_textCursorPos, m_textCursorColor);
        }
        else
        {
            result = isShiftModifierActive;
        }
    }
    else if ((inputSnapshot.m_channelId == AzFramework::InputDeviceKeyboard::Key::AlphanumericA) &&
             (static_cast<int>(activeModifierKeys) & static_cast<int>(AzFramework::ModifierKeyMask::CtrlAny)))
    {
        // Select all
        AZStd::string textString;
        UiTextBus::EventResult(textString, m_textEntity, &UiTextBus::Events::GetText);

        m_textSelectionStartPos = 0;
        m_textCursorPos = LyShine::GetUtf8StringLength(textString);
    }
    else if (command == UiNavigationHelpers::Command::NavHome)
    {
        // Move cursor to start of text
        m_textCursorPos = 0;
        if (!isShiftModifierActive)
        {
            m_textSelectionStartPos = m_textCursorPos;
        }
    }
    else if (command == UiNavigationHelpers::Command::NavEnd)
    {
        // Move cursor to end of text
        AZStd::string textString;
        UiTextBus::EventResult(textString, m_textEntity, &UiTextBus::Events::GetText);

        m_textCursorPos = LyShine::GetUtf8StringLength(textString);
        if (!isShiftModifierActive)
        {
            m_textSelectionStartPos = m_textCursorPos;
        }
    }
    else if (m_enableClipboard && (inputSnapshot.m_channelId == AzFramework::InputDeviceKeyboard::Key::AlphanumericC) && isLCTRLModifierActive)
    {
        AZStd::string textString;
        UiTextBus::EventResult(textString, m_textEntity, &UiTextBus::Events::GetText);
        if (textString.length() > 0 && m_textCursorPos != m_textSelectionStartPos)
        {
            int left = min(m_textCursorPos, m_textSelectionStartPos);
            int right = max(m_textCursorPos, m_textSelectionStartPos);
            UiClipboard::SetText(textString.substr(left, right - left));
        }
    }
    else if (m_enableClipboard && (inputSnapshot.m_channelId == AzFramework::InputDeviceKeyboard::Key::AlphanumericX) && isLCTRLModifierActive)
    {
        AZStd::string textString;
        UiTextBus::EventResult(textString, m_textEntity, &UiTextBus::Events::GetText);
        if (textString.length() > 0 && m_textCursorPos != m_textSelectionStartPos)
        {
            int left = min(m_textCursorPos, m_textSelectionStartPos);
            int right = max(m_textCursorPos, m_textSelectionStartPos);
            UiClipboard::SetText(textString.substr(left, right - left));
            textString.erase(left, right - left);
            m_textCursorPos = m_textSelectionStartPos = left;

            ChangeText(textString);
            ResetCursorBlink();
        }
    }
    else if (m_enableClipboard && (inputSnapshot.m_channelId == AzFramework::InputDeviceKeyboard::Key::AlphanumericV) && isLCTRLModifierActive)
    {
        auto clipboardText = UiClipboard::GetText();
        if (clipboardText.length() > 0)
        {
            AZStd::string textString;
            UiTextBus::EventResult(textString, m_textEntity, &UiTextBus::Events::GetText);

            // If a range is selected then erase that first
            if (m_textCursorPos != m_textSelectionStartPos)
            {
                int left = min(m_textCursorPos, m_textSelectionStartPos);
                int right = max(m_textCursorPos, m_textSelectionStartPos);
                textString.erase(left, right - left);
                m_textCursorPos = m_textSelectionStartPos = left;
            }

            // Append text from clipboard
            textString.insert(m_textCursorPos, clipboardText);
            m_textCursorPos += static_cast<int>(clipboardText.length());
            m_textSelectionStartPos = m_textCursorPos;

            // If max length is set, remove extra characters
            if (m_maxStringLength >= 0 && textString.length() > m_maxStringLength)
            {
                textString.resize(m_maxStringLength);
            }

            ChangeText(textString);
            ResetCursorBlink();
        }
    }
    else
    {
        result = false;
    }

    if (m_textCursorPos != oldTextCursorPos || m_textSelectionStartPos != oldTextSelectionStartPos)
    {
        AZ::Color color = (m_textSelectionStartPos == m_textCursorPos) ? m_textCursorColor : m_textSelectionColor;
        UiTextBus::Event(m_textEntity, &UiTextBus::Events::SetSelectionRange, m_textSelectionStartPos, m_textCursorPos, color);
        if (m_textSelectionStartPos == m_textCursorPos)
        {
            ResetCursorBlink();
        }

        UiTextBus::Event(m_textEntity, &UiTextBus::Events::ResetCursorLineHint);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextInputComponent::InputPositionUpdate(AZ::Vector2 point)
{
    // support dragging to select text, but also support being in a parent draggable
    if (m_isPressed)
    {
        // if we are not yet in the dragging state do some tests to see if we should be
        if (!m_isDragging)
        {
            CheckForDragOrHandOffToParent(point);
        }

        if (m_isDragging)
        {
            UiTextBus::EventResult(m_textCursorPos, m_textEntity, &UiTextBus::Events::GetCharIndexFromPoint, point, false);
            AZ::Color color = (m_textSelectionStartPos == m_textCursorPos) ? m_textCursorColor : m_textSelectionColor;
            UiTextBus::Event(m_textEntity, &UiTextBus::Events::SetSelectionRange, m_textSelectionStartPos, m_textCursorPos, color);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextInputComponent::LostActiveStatus()
{
    UiInteractableComponent::LostActiveStatus();

    EndEditState();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextInputComponent::Update(float deltaTime)
{
    UiInteractableComponent::Update(deltaTime);

    // if we have not set the enable/disable status of the text and placeholder text since
    // our status changed then set it
    if (m_childTextStateDirtyFlag)
    {
        bool displayPlaceHolder = true;

        if (m_isEditing)
        {
            displayPlaceHolder = false;
        }
        else
        {
            AZStd::string text;
            UiTextBus::EventResult(text, m_textEntity, &UiTextBus::Events::GetText);
            if (!text.empty())
            {
                displayPlaceHolder = false;
            }
        }

        UiElementBus::Event(m_placeHolderTextEntity, &UiElementBus::Events::SetIsEnabled, displayPlaceHolder);
        UiElementBus::Event(m_textEntity, &UiElementBus::Events::SetIsEnabled, !displayPlaceHolder);

        m_childTextStateDirtyFlag = false;
    }

    // update cursor blinking, only if: this component is active, and blink interval set, and there is no text selection
    if (m_isEditing && m_cursorBlinkInterval > 0.0f && m_textSelectionStartPos == m_textCursorPos)
    {
        const AZ::TimeMs realTimeMs = AZ::GetRealElapsedTimeMs();
        const float currentTime = AZ::TimeMsToSeconds(realTimeMs);
        if (m_cursorBlinkStartTime == 0.0f)
        {
            m_cursorBlinkStartTime = currentTime;
        }
        else if (currentTime - m_cursorBlinkStartTime > m_cursorBlinkInterval *  0.5f)
        {
            m_textCursorColor.SetA(m_textCursorColor.GetA() ? 0.0f : 1.0f);
            m_cursorBlinkStartTime = currentTime;
            UiTextBus::Event(
                m_textEntity, &UiTextBus::Events::SetSelectionRange, m_textSelectionStartPos, m_textCursorPos, m_textCursorColor);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextInputComponent::InGamePostActivate()
{
    UpdateDisplayedTextFunction();

    if (m_clipInputText)
    {
        UiTextBus::Event(m_textEntity, &UiTextBus::Events::SetOverflowMode, UiTextInterface::OverflowMode::ClipText);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiTextInputComponent::GetIsPasswordField()
{
    return m_isPasswordField;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextInputComponent::SetIsPasswordField(bool passwordField)
{
    m_isPasswordField = passwordField;
    UpdateDisplayedTextFunction();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t UiTextInputComponent::GetReplacementCharacter()
{
    // We store our replacement character as a string due to a reflection issue
    // with chars in the editor, so as a workaround we only deal with the first
    // character of the string.
    return m_replacementCharacter;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextInputComponent::SetReplacementCharacter(uint32_t replacementChar)
{
    m_replacementCharacter = replacementChar;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Color UiTextInputComponent::GetTextSelectionColor()
{
    return m_textSelectionColor;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextInputComponent::SetTextSelectionColor(const AZ::Color& color)
{
    m_textSelectionColor = color;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Color UiTextInputComponent::GetTextCursorColor()
{
    return m_textCursorColor;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextInputComponent::SetTextCursorColor(const AZ::Color& color)
{
    m_textCursorColor = color;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float UiTextInputComponent::GetCursorBlinkInterval()
{
    return m_cursorBlinkInterval;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextInputComponent::SetCursorBlinkInterval(float interval)
{
    m_cursorBlinkInterval = interval;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int UiTextInputComponent::GetMaxStringLength()
{
    return m_maxStringLength;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextInputComponent::SetMaxStringLength(int maxCharacters)
{
    m_maxStringLength = maxCharacters;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTextInputComponent::TextInputCallback UiTextInputComponent::GetOnChangeCallback()
{
    return m_onChange;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextInputComponent::SetOnChangeCallback(TextInputCallback callbackFunction)
{
    m_onChange = callbackFunction;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTextInputComponent::TextInputCallback UiTextInputComponent::GetOnEndEditCallback()
{
    return m_onEndEdit;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextInputComponent::SetOnEndEditCallback(TextInputCallback callbackFunction)
{
    m_onEndEdit = callbackFunction;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTextInputComponent::TextInputCallback UiTextInputComponent::GetOnEnterCallback()
{
    return m_onEnter;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextInputComponent::SetOnEnterCallback(TextInputCallback callbackFunction)
{
    m_onEnter = callbackFunction;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const LyShine::ActionName& UiTextInputComponent::GetChangeAction()
{
    return m_changeAction;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextInputComponent::SetChangeAction(const LyShine::ActionName& actionName)
{
    m_changeAction = actionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const LyShine::ActionName& UiTextInputComponent::GetEndEditAction()
{
    return m_endEditAction;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextInputComponent::SetEndEditAction(const LyShine::ActionName& actionName)
{
    m_endEditAction = actionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
const LyShine::ActionName& UiTextInputComponent::GetEnterAction()
{
    return m_enterAction;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextInputComponent::SetEnterAction(const LyShine::ActionName& actionName)
{
    m_enterAction = actionName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiTextInputComponent::GetTextEntity()
{
    return m_textEntity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextInputComponent::SetTextEntity(AZ::EntityId textEntity)
{
    m_textEntity = textEntity;
    m_childTextStateDirtyFlag = true;
    UpdateDisplayedTextFunction();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZStd::string UiTextInputComponent::GetText()
{
    AZStd::string text;
    UiTextBus::EventResult(text, m_textEntity, &UiTextBus::Events::GetText);
    return text;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextInputComponent::SetText(const AZStd::string& text)
{
    UiTextBus::Event(m_textEntity, &UiTextBus::Events::SetText, text);
    m_childTextStateDirtyFlag = true;

    // Make sure cursor position and selection is in range
    if (m_textCursorPos >= 0)
    {
        int maxPos = LyShine::GetUtf8StringLength(text);
        int newTextCursorPos = AZ::GetMin(m_textCursorPos, maxPos);
        int newTextSelectionStartPos = AZ::GetMin(m_textSelectionStartPos, maxPos);

        if (newTextCursorPos != m_textCursorPos || newTextSelectionStartPos != m_textSelectionStartPos)
        {
            m_textCursorPos = newTextCursorPos;
            m_textSelectionStartPos = newTextSelectionStartPos;

            int selStartIndex, selEndIndex;
            UiTextBus::Event(m_textEntity, &UiTextBus::Events::GetSelectionRange, selStartIndex, selEndIndex);
            if (selStartIndex >= 0)
            {
                UiTextBus::Event(
                    m_textEntity, &UiTextBus::Events::SetSelectionRange, m_textSelectionStartPos, m_textCursorPos, m_textCursorColor);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::EntityId UiTextInputComponent::GetPlaceHolderTextEntity()
{
    return m_placeHolderTextEntity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextInputComponent::SetPlaceHolderTextEntity(AZ::EntityId textEntity)
{
    m_placeHolderTextEntity = textEntity;
    m_childTextStateDirtyFlag = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiTextInputComponent::GetIsClipboardEnabled()
{
    return m_enableClipboard;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextInputComponent::SetIsClipboardEnabled(bool enableClipboard)
{
    m_enableClipboard = enableClipboard;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextInputComponent::Activate()
{
    UiInteractableComponent::Activate();
    UiInitializationBus::Handler::BusConnect(m_entity->GetId());
    UiTextInputBus::Handler::BusConnect(m_entity->GetId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextInputComponent::Deactivate()
{
    UiInteractableComponent::Deactivate();
    UiInitializationBus::Handler::BusDisconnect();
    UiTextInputBus::Handler::BusDisconnect();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiTextInputComponent::IsAutoActivationSupported()
{
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextInputComponent::BeginEditState()
{
    m_isEditing = true;

    // force re-evaluation of whether text or placeholder text should be displayed
    m_childTextStateDirtyFlag = true;

    // position the cursor in the text entity
    UiTextBus::Event(m_textEntity, &UiTextBus::Events::SetSelectionRange, m_textSelectionStartPos, m_textCursorPos, m_textCursorColor);

    ResetCursorBlink();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextInputComponent::EndEditState()
{
    AZStd::string textString;
    UiTextBus::EventResult(textString, m_textEntity, &UiTextBus::Events::GetText);

    // if a C++ callback is registered for OnEndEdit then call it
    if (m_onEndEdit)
    {
        // pass the entered text string to the C++ callback
        m_onEndEdit(GetEntityId(), textString);
    }

    // Tell any action listeners that the edit ended
    if (!m_endEditAction.empty())
    {
        // canvas listeners will get the action name (e.g. something like "EmailEntered") plus
        // the ID of this entity.
        AZ::EntityId canvasEntityId;
        UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
        UiCanvasNotificationBus::Event(canvasEntityId, &UiCanvasNotificationBus::Events::OnAction, GetEntityId(), m_endEditAction);
    }

    UiTextInputNotificationBus::Event(GetEntityId(), &UiTextInputNotificationBus::Events::OnTextInputEndEdit, textString);

    // clear the selection highlight
    UiTextBus::Event(m_textEntity, &UiTextBus::Events::ClearSelectionRange);

    m_textCursorPos = m_textSelectionStartPos = -1;

    if (m_isTextInputStarted)
    {
        AzFramework::InputTextEntryRequestBus::Broadcast(&AzFramework::InputTextEntryRequests::TextEntryStop);
        m_isTextInputStarted = false;
    }

    m_isEditing = false;

    // force re-evaluation of whether text or placeholder text should be displayed
    m_childTextStateDirtyFlag = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// calculate how much we have dragged along the text
float UiTextInputComponent::GetValidDragDistanceInPixels(AZ::Vector2 startPoint, AZ::Vector2 endPoint)
{
    const float validDragRatio = 0.5f;

    // convert the drag vector to local space
    AZ::Matrix4x4 transformFromViewport;
    UiTransformBus::Event(GetEntityId(), &UiTransformBus::Events::GetTransformFromViewport, transformFromViewport);
    AZ::Vector2 dragVec = endPoint - startPoint;
    AZ::Vector3 dragVec3(dragVec.GetX(), dragVec.GetY(), 0.0f);
    AZ::Vector3 localDragVec = transformFromViewport.Multiply3x3(dragVec3);

    // the text input component only supports drag along the x axis so zero the y axis
    localDragVec.SetY(0.0f);

    // convert back to viewport space
    AZ::Matrix4x4 transformToViewport;
    UiTransformBus::Event(GetEntityId(), &UiTransformBus::Events::GetTransformToViewport, transformToViewport);
    AZ::Vector3 validDragVec = transformToViewport.Multiply3x3(localDragVec);

    float validDistance = validDragVec.GetLengthSq();
    float totalDistance = dragVec.GetLengthSq();

    // if they are not dragging mostly in a valid direction then ignore the drag
    if (validDistance / totalDistance < validDragRatio)
    {
        validDistance = 0.0f;
    }

    // return the valid drag distance
    return validDistance;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextInputComponent::CheckForDragOrHandOffToParent(AZ::Vector2 point)
{
    AZ::EntityId parentDraggable;
    UiElementBus::EventResult(parentDraggable, GetEntityId(), &UiElementBus::Events::FindParentInteractableSupportingDrag, point);

    // if this interactable is inside another interactable that supports drag then we use
    // a threshold value before starting a drag on this interactable
    const float normalDragThreshold = 0.0f;
    const float containedDragThreshold = 5.0f;

    float dragThreshold = normalDragThreshold;
    if (parentDraggable.IsValid())
    {
        dragThreshold = containedDragThreshold;
    }

    // calculate how much we have dragged along the axis of the slider
    float validDragDistance = GetValidDragDistanceInPixels(m_pressedPoint, point);

    // only enter drag mode if we dragged above the threshold AND there is something to select
    AZStd::string textString;
    UiTextBus::EventResult(textString, m_textEntity, &UiTextBus::Events::GetText);

    if (validDragDistance > dragThreshold && !textString.empty())
    {
        // we dragged above the threshold value along axis of slider
        m_isDragging = true;

        // enter editing state if we are not already in it
        if (!m_isEditing)
        {
            BeginEditState();
        }
    }
    else if (parentDraggable.IsValid())
    {
        // offer the parent draggable the chance to become the active interactable
        bool handOff = false;
        UiInteractableBus::EventResult(
            handOff,
            parentDraggable,
            &UiInteractableBus::Events::OfferDragHandOff,
            GetEntityId(),
            m_pressedPoint,
            point,
            containedDragThreshold);

        if (handOff)
        {
            // interaction has been handed off to a container entity
            m_isPressed = false;
            EndEditState();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextInputComponent::OnReplacementCharacterChange()
{
    if (m_replacementCharacter == '\0')
    {
        m_replacementCharacter = defaultReplacementChar;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextInputComponent::UpdateDisplayedTextFunction()
{
    // If we're a password input box then we need to set up a callback to allow us to change how
    // the text stored in our child component is displayed before rendering.
    if (m_isPasswordField)
    {
        // Use a lambda here so we can easily access our instance to retrieve the
        // currently configured replacement character
        UiTextBus::Event(
            m_textEntity,
            &UiTextBus::Events::SetDisplayedTextFunction,
            [this](const AZStd::string& originalText)
            {
                // NOTE: this assumes the uint32_t can be interpreted as a wchar_t, it seems to
                // work for cases tested but may not in general.
                wchar_t wcharString[2] = { static_cast<wchar_t>(this->GetReplacementCharacter()), 0 };
                AZStd::string replacementCharString;
                AZStd::to_string(replacementCharString, { wcharString, 1 });

                int numReplacementChars = LyShine::GetUtf8StringLength(originalText);

                AZStd::string replacedString;
                replacedString.reserve(numReplacementChars * replacementCharString.length());
                for (int i = 0; i < numReplacementChars; i++)
                {
                    replacedString += replacementCharString;
                }

                return replacedString;
            });
    }
    else
    {
        UiTextBus::Event(m_textEntity, &UiTextBus::Events::SetDisplayedTextFunction, nullptr);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiTextInputComponent::EntityComboBoxVec UiTextInputComponent::PopulateTextEntityList()
{
    EntityComboBoxVec result;
    AZStd::vector<AZ::EntityId> entityIdList;

    // add a first entry for "None"
    result.push_back(AZStd::make_pair(AZ::EntityId(AZ::EntityId()), "<None>"));

    // allow the destination to be the same entity as the source by
    // adding this entity (if it has a text component)
    if (UiTextBus::FindFirstHandler(GetEntityId()))
    {
        result.push_back(AZStd::make_pair(AZ::EntityId(GetEntityId()), GetEntity()->GetName()));
    }

    // Add all descendant elements that have Text components to the lists
    AddDescendantTextElements(GetEntityId(), result);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiInteractableStatesInterface::State UiTextInputComponent::ComputeInteractableState()
{
    // This currently happens every frame. Needs optimization to just happen on events

    UiInteractableStatesInterface::State state = UiInteractableStatesInterface::StateNormal;

    if (!m_isHandlingEvents)
    {
        // not handling events, use disabled state
        state = UiInteractableStatesInterface::StateDisabled;
    }
    else if (m_isPressed && m_isHover)
    {
        // We only use the pressed state when the state is pressed AND the mouse is over the rect
        state = UiInteractableStatesInterface::StatePressed;
    }
    else if (m_isHover || m_isPressed || m_isEditing)
    {
        // we use the hover state for normal hover but also if the state is pressed but
        // the mouse is outside the rect, and also if the text is being edited
        state = UiInteractableStatesInterface::StateHover;
    }

    return state;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PROTECTED STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

void UiTextInputComponent::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
    if (serializeContext)
    {
        serializeContext->Class<UiTextInputComponent, UiInteractableComponent>()
            ->Version(8, &VersionConverter)
        // Elements group
            ->Field("Text", &UiTextInputComponent::m_textEntity)
            ->Field("PlaceHolderText", &UiTextInputComponent::m_placeHolderTextEntity)
        // Text editing group
            ->Field("TextSelectionColor", &UiTextInputComponent::m_textSelectionColor)
            ->Field("TextCursorColor", &UiTextInputComponent::m_textCursorColor)
            ->Field("MaxStringLength", &UiTextInputComponent::m_maxStringLength)
            ->Field("CursorBlinkInterval", &UiTextInputComponent::m_cursorBlinkInterval)
            ->Field("IsPasswordField", &UiTextInputComponent::m_isPasswordField)
            ->Field("ReplacementCharacter", &UiTextInputComponent::m_replacementCharacter)
            ->Field("ClipInputText", &UiTextInputComponent::m_clipInputText)
            ->Field("EnableClipboard", &UiTextInputComponent::m_enableClipboard)
        // Actions group
            ->Field("ChangeAction", &UiTextInputComponent::m_changeAction)
            ->Field("EndEditAction", &UiTextInputComponent::m_endEditAction)
            ->Field("EnterAction", &UiTextInputComponent::m_enterAction);

        AZ::EditContext* ec = serializeContext->GetEditContext();
        if (ec)
        {
            auto editInfo = ec->Class<UiTextInputComponent>("TextInput", "An interactable component for editing a text string.");

            editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::Category, "UI")
                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/UiTextInput.png")
                ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/UiTextInput.png")
                ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("UI"))
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

            // Elements group
            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Elements")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiTextInputComponent::m_textEntity, "Text", "The UI element to hold the entered text.")
                    ->Attribute(AZ::Edit::Attributes::EnumValues, &UiTextInputComponent::PopulateTextEntityList);
                editInfo->DataElement(AZ::Edit::UIHandlers::ComboBox, &UiTextInputComponent::m_placeHolderTextEntity, "Placeholder text", "The UI element to display the placeholder text.")
                    ->Attribute(AZ::Edit::Attributes::EnumValues, &UiTextInputComponent::PopulateTextEntityList);
            }

            // Text Editing group
            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Text editing")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editInfo->DataElement(AZ::Edit::UIHandlers::Color, &UiTextInputComponent::m_textSelectionColor,
                    "Selection color", "The text selection color.");
                editInfo->DataElement(AZ::Edit::UIHandlers::Color, &UiTextInputComponent::m_textCursorColor,
                    "Cursor color", "The text cursor color.");
                editInfo->DataElement(AZ::Edit::UIHandlers::SpinBox, &UiTextInputComponent::m_cursorBlinkInterval,
                    "Cursor blink time", "The cursor blink interval.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.1f);
                editInfo->DataElement(AZ::Edit::UIHandlers::SpinBox, &UiTextInputComponent::m_maxStringLength,
                    "Max char count", "The maximum string length that can be entered. For unlimited enter -1.")
                    ->Attribute(AZ::Edit::Attributes::Min, -1)
                    ->Attribute(AZ::Edit::Attributes::Step, 1);

                editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiTextInputComponent::m_isPasswordField,
                    "Is password field", "A password field hides the entered text.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC_CE("RefreshEntireTree"));
                editInfo->DataElement(AZ_CRC_CE("Char"), &UiTextInputComponent::m_replacementCharacter,
                    "Replacement character", "The replacement character used to hide password text.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &UiTextInputComponent::OnReplacementCharacterChange)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &UiTextInputComponent::GetIsPasswordField);
                editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiTextInputComponent::m_clipInputText,
                    "Clip input text", "When checked, the input text is clipped to this element's rect.");
                editInfo->DataElement(AZ::Edit::UIHandlers::CheckBox, &UiTextInputComponent::m_enableClipboard,
                    "Enable clipboard", "When checked, Ctrl-C, Ctrl-X, and Ctrl-V events will be handled");
            }

            // Actions group
            {
                editInfo->ClassElement(AZ::Edit::ClassElements::Group, "Actions")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);

                editInfo->DataElement(0, &UiTextInputComponent::m_changeAction, "Change", "The action name triggered on each character typed.");
                editInfo->DataElement(0, &UiTextInputComponent::m_endEditAction, "End edit", "The action name triggered on either focus change or enter.");
                editInfo->DataElement(0, &UiTextInputComponent::m_enterAction, "Enter", "The action name triggered when enter is pressed.");
            }
        }
    }

    AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
    if (behaviorContext)
    {
        behaviorContext->EBus<UiTextInputBus>("UiTextInputBus")
            ->Event("GetTextSelectionColor", &UiTextInputBus::Events::GetTextSelectionColor)
            ->Event("SetTextSelectionColor", &UiTextInputBus::Events::SetTextSelectionColor)
            ->Event("GetTextCursorColor", &UiTextInputBus::Events::GetTextCursorColor)
            ->Event("SetTextCursorColor", &UiTextInputBus::Events::SetTextCursorColor)
            ->Event("GetCursorBlinkInterval", &UiTextInputBus::Events::GetCursorBlinkInterval)
            ->Event("SetCursorBlinkInterval", &UiTextInputBus::Events::SetCursorBlinkInterval)
            ->Event("GetMaxStringLength", &UiTextInputBus::Events::GetMaxStringLength)
            ->Event("SetMaxStringLength", &UiTextInputBus::Events::SetMaxStringLength)
            ->Event("GetChangeAction", &UiTextInputBus::Events::GetChangeAction)
            ->Event("SetChangeAction", &UiTextInputBus::Events::SetChangeAction)
            ->Event("GetEndEditAction", &UiTextInputBus::Events::GetEndEditAction)
            ->Event("SetEndEditAction", &UiTextInputBus::Events::SetEndEditAction)
            ->Event("GetEnterAction", &UiTextInputBus::Events::GetEnterAction)
            ->Event("SetEnterAction", &UiTextInputBus::Events::SetEnterAction)
            ->Event("GetTextEntity", &UiTextInputBus::Events::GetTextEntity)
            ->Event("SetTextEntity", &UiTextInputBus::Events::SetTextEntity)
            ->Event("GetText", &UiTextInputBus::Events::GetText)
            ->Event("SetText", &UiTextInputBus::Events::SetText)
            ->Event("GetPlaceHolderTextEntity", &UiTextInputBus::Events::GetPlaceHolderTextEntity)
            ->Event("SetPlaceHolderTextEntity", &UiTextInputBus::Events::SetPlaceHolderTextEntity)
            ->Event("GetIsPasswordField", &UiTextInputBus::Events::GetIsPasswordField)
            ->Event("SetIsPasswordField", &UiTextInputBus::Events::SetIsPasswordField)
            ->Event("GetReplacementCharacter", &UiTextInputBus::Events::GetReplacementCharacter)
            ->Event("SetReplacementCharacter", &UiTextInputBus::Events::SetReplacementCharacter)
            ->Event("GetIsClipboardEnabled", &UiTextInputBus::Events::GetIsClipboardEnabled)
            ->Event("SetIsClipboardEnabled", &UiTextInputBus::Events::SetIsClipboardEnabled)
            ->VirtualProperty("TextSelectionColor", "GetTextSelectionColor", "SetTextSelectionColor")
            ->VirtualProperty("TextCursorColor", "GetTextCursorColor", "SetTextCursorColor")
            ->VirtualProperty("CursorBlinkInterval", "GetCursorBlinkInterval", "SetCursorBlinkInterval")
            ->VirtualProperty("MaxStringLength", "GetMaxStringLength", "SetMaxStringLength");

        behaviorContext->Class<UiTextInputComponent>()->RequestBus("UiTextInputBus");

        behaviorContext->EBus<UiTextInputNotificationBus>("UiTextInputNotificationBus")
            ->Handler<BehaviorUiTextInputNotificationBusHandler>();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextInputComponent::ChangeText(const AZStd::string& textString)
{
    // For user-inputted text, we assume that users don't want to input
    // text as styling markup (but rather plain-text).
    UiTextBus::Event(m_textEntity, &UiTextBus::Events::SetTextWithFlags, textString, UiTextInterface::SetEscapeMarkup);

    // if a C++ callback is registered for OnChange then call it
    if (m_onChange)
    {
        // pass the entered text string to the C++ callback
        m_onChange(GetEntityId(), textString);
    }

    // Tell any action listeners about the event
    if (!m_changeAction.empty())
    {
        // canvas listeners will get the action name (e.g. something like "EmailEdited") plus
        // the ID of this entity.
        AZ::EntityId canvasEntityId;
        UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
        UiCanvasNotificationBus::Event(canvasEntityId, &UiCanvasNotificationBus::Events::OnAction, GetEntityId(), m_changeAction);
    }

    UiTextInputNotificationBus::Event(GetEntityId(), &UiTextInputNotificationBus::Events::OnTextInputChange, textString);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextInputComponent::ResetCursorBlink()
{
    m_textCursorColor.SetA(1.0f);
    m_cursorBlinkStartTime = 0.0f;
    UiTextBus::Event(m_textEntity, &UiTextBus::Events::SetSelectionRange, m_textSelectionStartPos, m_textCursorPos, m_textCursorColor);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiTextInputComponent::CheckStartTextInput()
{
    // We do not bring up the on-screen keyboard when a drag is started, only on a "click" or at
    // the end of a drag. But a drag begin can cause BeginEditState to be called. So we can begin
    // edit state before we bring up the on-screen keyboard. So here we test if it is time to bring
    // up the keyboard.
    if (m_isEditing && !m_isTextInputStarted)
    {
        // ensure the on-screen keyboard is shown on mobile and console platforms
        AzFramework::InputTextEntryRequests::VirtualKeyboardOptions options;

        AZStd::string textString;
        UiTextBus::EventResult(textString, m_textEntity, &UiTextBus::Events::GetText);
        options.m_initialText = Utf8SubString(textString, m_textCursorPos, m_textSelectionStartPos);

        AZ::EntityId canvasEntityId;
        UiElementBus::EventResult(canvasEntityId, GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);

        // Calculate height available for virtual keyboard. In game mode, canvas size is the same as viewport size
        AZ::Vector2 canvasSize;
        UiCanvasBus::EventResult(canvasSize, canvasEntityId, &UiCanvasBus::Events::GetCanvasSize);
        UiTransformInterface::RectPoints rectPoints;
        UiTransformBus::Event(GetEntityId(), &UiTransformBus::Events::GetViewportSpacePoints, rectPoints);
        const AZ::Vector2 bottomRight = rectPoints.GetAxisAlignedBottomRight();
        options.m_normalizedMinY = (canvasSize.GetY() > 0.0f) ? bottomRight.GetY() / canvasSize.GetY() : 0.0f;

        UiCanvasBus::EventResult(options.m_localUserId, canvasEntityId, &UiCanvasBus::Events::GetLocalUserIdInputFilter);

        AzFramework::InputTextEntryRequestBus::Broadcast(&AzFramework::InputTextEntryRequests::TextEntryStart, options);

        m_isTextInputStarted = true;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE STATIC MEMBER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiTextInputComponent::VersionConverter(AZ::SerializeContext& context,
    AZ::SerializeContext::DataElementNode& classElement)
{
    // conversion from version 1:
    // - Need to convert CryString elements to AZStd::string
    // - Need to convert Color to Color and Alpha
    // conversion from version 1 or 2 to current:
    // - Need to convert CryString ActionName elements to AZStd::string
    AZ_Assert(classElement.GetVersion() > 2, "Unsupported UiTextInputComponent version: %d", classElement.GetVersion());
    
    // conversion from version 1, 2 or 3 to current:
    // - Need to convert AZStd::string sprites to AzFramework::SimpleAssetReference<LmbrCentral::TextureAsset>
    if (classElement.GetVersion() <= 3)
    {
        if (!LyShine::ConvertSubElementFromAzStringToAssetRef<LmbrCentral::TextureAsset>(context, classElement, "SelectedSprite"))
        {
            return false;
        }

        if (!LyShine::ConvertSubElementFromAzStringToAssetRef<LmbrCentral::TextureAsset>(context, classElement, "PressedSprite"))
        {
            return false;
        }
    }

    // Conversion from version 4 to 5:
    if (classElement.GetVersion() < 5)
    {
        // find the base class (AZ::Component)
        // NOTE: in very old versions there may not be a base class because the base class was not serialized
        int componentBaseClassIndex = classElement.FindElement(AZ_CRC_CE("BaseClass1"));

        // If there was a base class, make a copy and remove it
        AZ::SerializeContext::DataElementNode componentBaseClassNode;
        if (componentBaseClassIndex != -1)
        {
            // make a local copy of the component base class node
            componentBaseClassNode = classElement.GetSubElement(componentBaseClassIndex);

            // remove the component base class from the button
            classElement.RemoveElement(componentBaseClassIndex);
        }

        // Add a new base class (UiInteractableComponent)
        int interactableBaseClassIndex = classElement.AddElement<UiInteractableComponent>(context, "BaseClass1");
        AZ::SerializeContext::DataElementNode& interactableBaseClassNode = classElement.GetSubElement(interactableBaseClassIndex);

        // if there was previously a base class...
        if (componentBaseClassIndex != -1)
        {
            // copy the component base class into the new interactable base class
            // Since AZ::Component is now the base class of UiInteractableComponent
            interactableBaseClassNode.AddElement(componentBaseClassNode);
        }

        // Move the selected/hover state to the base class
        if (!UiSerialize::MoveToInteractableStateActions(context, classElement, "HoverStateActions",
                "SelectedColor", "SelectedAlpha", "SelectedSprite"))
        {
            return false;
        }

        // Move the pressed state to the base class
        if (!UiSerialize::MoveToInteractableStateActions(context, classElement, "PressedStateActions",
                "PressedColor", "PressedAlpha", "PressedSprite"))
        {
            return false;
        }
    }

    // Conversion from version 5 to 6:
    if (classElement.GetVersion() < 6)
    {
        int clipTextIndex = classElement.AddElement<bool>(context, "ClipInputText");

        if (clipTextIndex == -1)
        {
            // Error adding the new sub element
            AZ_Error("Serialization", false, "Failed to create ClipInputText node");
            return false;
        }

        AZ::SerializeContext::DataElementNode& clipTextNode = classElement.GetSubElement(clipTextIndex);
        clipTextNode.SetData(context, false);
    }

    // conversion from version 6 to 7: Need to convert ColorF to AZ::Color
    if (classElement.GetVersion() < 7)
    {
        if (!LyShine::ConvertSubElementFromColorFToAzColor(context, classElement, "TextSelectionColor"))
        {
            return false;
        }

        if (!LyShine::ConvertSubElementFromColorFToAzColor(context, classElement, "TextCursorColor"))
        {
            return false;
        }
    }

    // Conversion from 7 to 8: Need to convert char to uint32_t
    if (classElement.GetVersion() < 8)
    {
        if (!LyShine::ConvertSubElementFromCharToUInt32(context, classElement, "ReplacementCharacter"))
        {
            return false;
        }
    }

    return true;
}
