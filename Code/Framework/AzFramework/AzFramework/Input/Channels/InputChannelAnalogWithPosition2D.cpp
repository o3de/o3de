/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Channels/InputChannelAnalogWithPosition2D.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelAnalogWithPosition2D::InputChannelAnalogWithPosition2D(
        const InputChannelId& inputChannelId,
        const InputDevice& inputDevice)
    : InputChannelAnalog(inputChannelId, inputDevice)
    , m_positionData()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputChannel::CustomData* InputChannelAnalogWithPosition2D::GetCustomData() const
    {
        return &m_positionData;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelAnalogWithPosition2D::ResetState()
    {
        m_positionData = PositionData2D();
        InputChannelAnalog::ResetState();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelAnalogWithPosition2D::SimulateRawInputWithPosition2D(float rawValue,
                                                                          float normalizedX,
                                                                          float normalizedY)
    {
        const RawInputEvent rawValues(normalizedX, normalizedY, rawValue);
        ProcessRawInputEvent(rawValues);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelAnalogWithPosition2D::ProcessRawInputEvent(const RawInputEvent& rawValues)
    {
        const AZ::Vector2 newPosition = AZ::Vector2(rawValues.m_normalizedX, rawValues.m_normalizedY);
        m_positionData.UpdateNormalizedPositionAndDelta(newPosition);

        InputChannelAnalog::ProcessRawInputEvent(rawValues.m_analogValue);
    }
} // namespace AzFramework
