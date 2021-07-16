/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
