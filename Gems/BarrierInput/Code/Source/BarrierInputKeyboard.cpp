/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <BarrierInputKeyboard.h>

#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboardWindowsScanCodes.h>

#include <AzCore/std/containers/fixed_unordered_map.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace BarrierInput
{
    using namespace AzFramework;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceKeyboardBarrier::InputDeviceKeyboardBarrier(InputDeviceKeyboard& inputDevice)
        : InputDeviceKeyboard::Implementation(inputDevice)
        , m_threadAwareRawKeyEventQueuesById()
        , m_threadAwareRawKeyEventQueuesByIdMutex()
        , m_threadAwareRawTextEventQueue()
        , m_threadAwareRawTextEventQueueMutex()
        , m_hasTextEntryStarted(false)
    {
        RawInputNotificationBusBarrier::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceKeyboardBarrier::~InputDeviceKeyboardBarrier()
    {
        RawInputNotificationBusBarrier::Handler::BusDisconnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceKeyboardBarrier::IsConnected() const
    {
        // We could check the validity of the socket connection to the Barrier server
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceKeyboardBarrier::HasTextEntryStarted() const
    {
        return m_hasTextEntryStarted;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceKeyboardBarrier::TextEntryStart(const InputTextEntryRequests::VirtualKeyboardOptions&)
    {
        m_hasTextEntryStarted = true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceKeyboardBarrier::TextEntryStop()
    {
        m_hasTextEntryStarted = false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceKeyboardBarrier::TickInputDevice()
    {
        {
            // Queue all key events that were received in the other thread
            AZStd::scoped_lock lock(m_threadAwareRawKeyEventQueuesByIdMutex);
            for (const auto& keyEventQueuesById : m_threadAwareRawKeyEventQueuesById)
            {
                const InputChannelId& inputChannelId = keyEventQueuesById.first;
                for (bool rawKeyState : keyEventQueuesById.second)
                {
                    QueueRawKeyEvent(inputChannelId, rawKeyState);
                }
            }
            m_threadAwareRawKeyEventQueuesById.clear();
        }

        {
            // Queue all text events that were received in the other thread
            AZStd::scoped_lock lock(m_threadAwareRawTextEventQueueMutex);
            for (const AZStd::string& rawTextEvent : m_threadAwareRawTextEventQueue)
            {
            #if !defined(ALWAYS_DISPATCH_KEYBOARD_TEXT_INPUT)
                if (!m_hasTextEntryStarted)
                {
                    continue;
                }
            #endif // !defined(ALWAYS_DISPATCH_KEYBOARD_TEXT_INPUT)
                QueueRawTextEvent(rawTextEvent);
            }
            m_threadAwareRawTextEventQueue.clear();
        }

        // Process raw event queues once each frame
        ProcessRawEventQueues();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceKeyboardBarrier::OnRawKeyboardKeyDownEvent(uint32_t scanCode,
                                                               ModifierMask activeModifiers)
    {
        // Queue key events and text events
        ThreadSafeQueueRawKeyEvent(scanCode, true);
        if (char asciiChar = TranslateRawKeyEventToASCIIChar(scanCode, activeModifiers))
        {
            const AZStd::string text(1, asciiChar);
            ThreadSafeQueueRawTextEvent(text);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceKeyboardBarrier::OnRawKeyboardKeyUpEvent(uint32_t scanCode,
                                                             [[maybe_unused]]ModifierMask activeModifiers)
    {
        // Queue key events, not text events
        ThreadSafeQueueRawKeyEvent(scanCode, false);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceKeyboardBarrier::OnRawKeyboardKeyRepeatEvent(uint32_t scanCode,
                                                                 ModifierMask activeModifiers)
    {
        // Don't queue key events, only text events
        if (char asciiChar = TranslateRawKeyEventToASCIIChar(scanCode, activeModifiers))
        {
            const AZStd::string text(1, asciiChar);
            ThreadSafeQueueRawTextEvent(text);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceKeyboardBarrier::ThreadSafeQueueRawKeyEvent(uint32_t scanCode, bool rawKeyState)
    {
        // From observation, Barrier scan codes in the:
        // - Range 0x0-0x7F (0-127) correspond to windows scan codes without the extended bit set
        // - Range 0x100-0x17F (256-383) correspond to windows scan codes with the extended bit set
        const InputChannelId* inputChannelId = nullptr;
        if (scanCode < InputChannelIdByScanCodeTable.size())
        {
            inputChannelId = InputChannelIdByScanCodeTable[scanCode];
        }
        else if (0x100 <= scanCode && scanCode < InputChannelIdByScanCodeWithExtendedPrefixTable.size())
        {
            inputChannelId = InputChannelIdByScanCodeWithExtendedPrefixTable[scanCode - 0x100];
        }

        if (inputChannelId)
        {
            AZStd::scoped_lock lock(m_threadAwareRawKeyEventQueuesByIdMutex);
            m_threadAwareRawKeyEventQueuesById[*inputChannelId].push_back(rawKeyState);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceKeyboardBarrier::ThreadSafeQueueRawTextEvent(const AZStd::string& textUTF8)
    {
        AZStd::scoped_lock lock(m_threadAwareRawTextEventQueueMutex);
        m_threadAwareRawTextEventQueue.push_back(textUTF8);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    char InputDeviceKeyboardBarrier::TranslateRawKeyEventToASCIIChar(uint32_t scanCode,
                                                                     ModifierMask activeModifiers)
    {
        // Map ASCII character pairs keyed by their keyboard scan code, assuming an ANSI mechanical
        // keyboard layout with a standard QWERTY key mapping. The first element of the pair is the
        // character that should be produced if the key is pressed while no shift or caps modifiers
        // are active, while the second element is the character that should be produced if the key
        // is pressed while a shift or caps modifier is active. Required because Barrier only sends
        // raw key events, not translated text input. While we would ideally support the full range
        // of UTF-8 text input, that is beyond the scope of this debug/development only class. Note
        // that this function assumes an ANSI mechanical keyboard layout with a standard QWERTY key
        // mapping, and will not produce correct results if used with other key layouts or mappings.
        static const AZStd::fixed_unordered_map<AZ::u32, AZStd::pair<char, char>, 16, 64> ScanCodeToASCIICharMap =
        {
            {   2, { '1', '!' } },
            {   3, { '2', '@' } },
            {   4, { '3', '#' } },
            {   5, { '4', '$' } },
            {   6, { '5', '%' } },
            {   7, { '6', '^' } },
            {   8, { '7', '&' } },
            {   9, { '8', '*' } },
            {  10, { '9', '(' } },
            {  11, { '0', ')' } },
            {  12, { '-', '_' } },
            {  13, { '=', '+' } },
            {  15, { '\t', '\t' } },
            {  16, { 'q', 'Q' } },
            {  17, { 'w', 'W' } },
            {  18, { 'e', 'E' } },
            {  19, { 'r', 'R' } },
            {  20, { 't', 'T' } },
            {  21, { 'y', 'Y' } },
            {  22, { 'u', 'U' } },
            {  23, { 'i', 'I' } },
            {  24, { 'o', 'O' } },
            {  25, { 'p', 'P' } },
            {  26, { '[', '{' } },
            {  27, { ']', '}' } },
            {  30, { 'a', 'A' } },
            {  31, { 's', 'S' } },
            {  32, { 'd', 'D' } },
            {  33, { 'f', 'F' } },
            {  34, { 'g', 'G' } },
            {  35, { 'h', 'H' } },
            {  36, { 'j', 'J' } },
            {  37, { 'k', 'K' } },
            {  38, { 'l', 'L' } },
            {  39, { ';', ':' } },
            {  40, { '\'', '"' } },
            {  41, { '`', '~' } },
            {  43, { '\\', '|' } },
            {  44, { 'z', 'Z' } },
            {  45, { 'x', 'X' } },
            {  46, { 'c', 'C' } },
            {  47, { 'v', 'V' } },
            {  48, { 'b', 'B' } },
            {  49, { 'n', 'N' } },
            {  50, { 'm', 'M' } },
            {  51, { ',', '<' } },
            {  52, { '.', '>' } },
            {  53, { '/', '?' } },
            {  55, { '*', '*' } },
            {  57, { ' ', ' ' } },
            {  71, { '7', '7' } },
            {  72, { '8', '8' } },
            {  73, { '9', '9' } },
            {  74, { '-', '-' } },
            {  75, { '4', '4' } },
            {  76, { '5', '5' } },
            {  77, { '6', '6' } },
            {  78, { '+', '+' } },
            {  79, { '1', '1' } },
            {  80, { '2', '2' } },
            {  81, { '3', '3' } },
            {  82, { '0', '0' } },
            {  83, { '.', '.' } },
            { 309, { '/', '/' } }
        };

        const auto& it = ScanCodeToASCIICharMap.find(scanCode);
        if (it == ScanCodeToASCIICharMap.end())
        {
            return '\0';
        }

        const bool shiftOrCapsLockActive = (activeModifiers & ModifierMask_Shift) ||
                                           (activeModifiers & ModifierMask_CapsLock);
        return shiftOrCapsLockActive ? it->second.second : it->second.first;
    }

    AZStd::unique_ptr<AzFramework::InputDeviceKeyboard::Implementation> InputDeviceKeyboardBarrierImplFactory::Create(AzFramework::InputDeviceKeyboard& inputDevice)
    {
        return AZStd::make_unique<InputDeviceKeyboardBarrier>(inputDevice);
    }

} // namespace BarrierInput
