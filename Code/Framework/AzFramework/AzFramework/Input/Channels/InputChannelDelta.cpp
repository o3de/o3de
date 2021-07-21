/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Channels/InputChannelDelta.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelDelta::InputChannelDelta(const InputChannelId& inputChannelId,
                                         const InputDevice& inputDevice)
        : InputChannel(inputChannelId, inputDevice)
        , m_delta(0.0f)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    float InputChannelDelta::GetValue() const
    {
        return m_delta;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    float InputChannelDelta::GetDelta() const
    {
        return m_delta;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelDelta::ResetState()
    {
        m_delta = 0.0f;
        InputChannel::ResetState();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelDelta::SimulateRawInput(float rawValue)
    {
        ProcessRawInputEvent(rawValue);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelDelta::ProcessRawInputEvent(float rawValue)
    {
        m_delta = rawValue;

        const bool isChannelActive = (m_delta != 0.0f);
        UpdateState(isChannelActive);
    }
} // namespace AzFramework
