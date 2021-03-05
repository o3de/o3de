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
