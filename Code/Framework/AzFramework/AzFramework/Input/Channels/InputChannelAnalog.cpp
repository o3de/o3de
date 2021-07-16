/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Channels/InputChannelAnalog.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelAnalog::InputChannelAnalog(const InputChannelId& inputChannelId,
                                           const InputDevice& inputDevice)
        : InputChannel(inputChannelId, inputDevice)
        , m_value(0.0f)
        , m_delta(0.0f)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    float InputChannelAnalog::GetValue() const
    {
        return m_value;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    float InputChannelAnalog::GetDelta() const
    {
        return m_delta;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelAnalog::ResetState()
    {
        m_value = 0.0f;
        m_delta = 0.0f;
        InputChannel::ResetState();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelAnalog::SimulateRawInput(float rawValue)
    {
        ProcessRawInputEvent(rawValue);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelAnalog::ProcessRawInputEvent(float rawValue)
    {
        const float oldValue = m_value;
        m_value = rawValue;
        m_delta = rawValue - oldValue;

        const bool isChannelActive = (rawValue != 0.0f);
        UpdateState(isChannelActive);
    }
} // namespace AzFramework
