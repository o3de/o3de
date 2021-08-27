/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Channels/InputChannelDigitalWithPosition2D.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelDigitalWithPosition2D::InputChannelDigitalWithPosition2D(
        const InputChannelId& inputChannelId,
        const InputDevice& inputDevice)
    : InputChannelDigital(inputChannelId, inputDevice)
    , m_positionData()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputChannel::CustomData* InputChannelDigitalWithPosition2D::GetCustomData() const
    {
        return &m_positionData;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelDigitalWithPosition2D::ResetState()
    {
        m_positionData = PositionData2D();
        InputChannelDigital::ResetState();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelDigitalWithPosition2D::SimulateRawInputWithPosition2D(float rawValue,
                                                                          float normalizedX,
                                                                          float normalizedY)
    {
        const RawInputEvent rawValues(normalizedX, normalizedY, rawValue != 0.0f ? true : false);
        ProcessRawInputEvent(rawValues);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelDigitalWithPosition2D::ProcessRawInputEvent(const RawInputEvent& rawValues)
    {
        const AZ::Vector2 newPosition = AZ::Vector2(rawValues.m_normalizedX, rawValues.m_normalizedY);
        m_positionData.UpdateNormalizedPositionAndDelta(newPosition);

        InputChannelDigital::ProcessRawInputEvent(rawValues.m_digitalValue);
    }
} // namespace AzFramework
