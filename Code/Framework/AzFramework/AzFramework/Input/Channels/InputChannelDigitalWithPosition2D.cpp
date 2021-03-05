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
