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

#include <AzFramework/Input/Channels/InputChannelDigital.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelDigital::InputChannelDigital(const InputChannelId& inputChannelId,
                                             const InputDevice& inputDevice)
        : InputChannel(inputChannelId, inputDevice)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    float InputChannelDigital::GetValue() const
    {
        return IsActive() ? 1.0f : 0.0f;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    float InputChannelDigital::GetDelta() const
    {
        switch (GetState())
        {
            case State::Idle: return 0.0f;
            case State::Began: return 1.0f;
            case State::Updated: return 0.0f;
            case State::Ended: return -1.0f;
        }
        return 0.0f;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelDigital::SimulateRawInput(float rawValue)
    {
        ProcessRawInputEvent(rawValue != 0.0f ? true : false);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelDigital::ProcessRawInputEvent(bool rawValue)
    {
        UpdateState(rawValue);
    }
} // namespace AzFramework
