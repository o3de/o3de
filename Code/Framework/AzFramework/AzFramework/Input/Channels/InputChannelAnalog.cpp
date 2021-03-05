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
