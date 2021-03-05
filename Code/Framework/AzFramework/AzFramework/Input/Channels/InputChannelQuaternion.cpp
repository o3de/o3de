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

#include <AzFramework/Input/Channels/InputChannelQuaternion.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelQuaternion::InputChannelQuaternion(const InputChannelId& inputChannelId,
                                                   const InputDevice& inputDevice)
        : InputChannel(inputChannelId, inputDevice)
        , m_quaternionData()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputChannel::CustomData* InputChannelQuaternion::GetCustomData() const
    {
        return &m_quaternionData;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelQuaternion::ResetState()
    {
        m_quaternionData = QuaternionData();
        InputChannel::ResetState();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelQuaternion::ProcessRawInputEvent(const AZ::Quaternion& rawValue)
    {
        const AZ::Quaternion oldValue = m_quaternionData.m_value;
        m_quaternionData.m_value = rawValue;
        m_quaternionData.m_delta = oldValue.GetInverseFast() * rawValue;

        const bool isChannelActive = !rawValue.IsIdentity();
        UpdateState(isChannelActive);
    }
} // namespace AzFramework
