/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
