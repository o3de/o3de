/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Channels/InputChannelDigitalWithSharedModifierKeyStates.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool ModifierKeyStates::IsActive(ModifierKeyMask modifierKey) const
    {
        return (static_cast<int>(m_activeModifierKeys) & static_cast<int>(modifierKey)) != 0;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ModifierKeyStates::SetActive(ModifierKeyMask modifierKey, bool active)
    {
        const int newState = active ?
                             static_cast<int>(m_activeModifierKeys) | static_cast<int>(modifierKey) :
                             static_cast<int>(m_activeModifierKeys) & ~(static_cast<int>(modifierKey));
        m_activeModifierKeys = static_cast<ModifierKeyMask>(newState);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelDigitalWithSharedModifierKeyStates::InputChannelDigitalWithSharedModifierKeyStates(
        const InputChannelId& inputChannelId,
        const InputDevice& inputDevice,
        SharedModifierKeyStates& sharedModifierKeyStates,
        ModifierKeyMask correspondingModifierKey) // = ModifierKeyMask::None
    : InputChannelDigital(inputChannelId, inputDevice)
    , m_sharedModifierKeyStates(sharedModifierKeyStates)
    , m_correspondingModifierKey(correspondingModifierKey)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputChannel::CustomData* InputChannelDigitalWithSharedModifierKeyStates::GetCustomData() const
    {
        return m_sharedModifierKeyStates.get();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelDigitalWithSharedModifierKeyStates::ResetState()
    {
        if (m_correspondingModifierKey != ModifierKeyMask::None)
        {
            m_sharedModifierKeyStates->SetActive(m_correspondingModifierKey, false);
        }
        InputChannelDigital::ResetState();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelDigitalWithSharedModifierKeyStates::ProcessRawInputEvent(bool rawValue)
    {
        if (m_correspondingModifierKey != ModifierKeyMask::None)
        {
            m_sharedModifierKeyStates->SetActive(m_correspondingModifierKey, rawValue);
        }
        InputChannelDigital::ProcessRawInputEvent(rawValue);
    }
} // namespace AzFramework
