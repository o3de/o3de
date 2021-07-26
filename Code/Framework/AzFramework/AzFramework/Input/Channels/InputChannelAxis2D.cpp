/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Channels/InputChannelAxis2D.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelAxis2D::InputChannelAxis2D(const InputChannelId& inputChannelId,
                                           const InputDevice& inputDevice)
        : InputChannel(inputChannelId, inputDevice)
        , m_axisData2D()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    float InputChannelAxis2D::GetValue() const
    {
        return m_axisData2D.m_values.GetLength();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    float InputChannelAxis2D::GetDelta() const
    {
        return m_axisData2D.m_deltas.GetLength();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputChannel::CustomData* InputChannelAxis2D::GetCustomData() const
    {
        return &m_axisData2D;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelAxis2D::ResetState()
    {
        m_axisData2D = AxisData2D();
        InputChannel::ResetState();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelAxis2D::SimulateRawInput2D(float rawValueX, float rawValueY)
    {
        const AZ::Vector2 rawValues(rawValueX, rawValueY);
        ProcessRawInputEvent(rawValues);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelAxis2D::ProcessRawInputEvent(const AZ::Vector2& rawValues,
                                                  const AZ::Vector2* rawValuesPreDeadZone) // = nullptr
    {
        const AZ::Vector2 oldValues = m_axisData2D.m_values;
        m_axisData2D.m_values = rawValues;
        m_axisData2D.m_deltas = rawValues - oldValues;
        m_axisData2D.m_preDeadZoneValues = rawValuesPreDeadZone ? *rawValuesPreDeadZone : rawValues;

        // The modification of this check to use m_preDeadZoneValues instead of m_values will
        // possibly result in events being sent out even while the thumbstick is still idling,
        // (depending on the physical hardware and the platform). Although this change to the
        // existing behavior could potentially impact other systems, it is simply unavoidable
        // if we want any system to be able to access the pre dead-zone thumbstick values, so
        // we'll just have to be aware and ensure that any other systems using these specific
        // InputChannelAxis2D input events (of which there are few) are checking the m_values
        // instead of doing something based simply off receiving the event in the first place.
        const bool isChannelActive = (m_axisData2D.m_preDeadZoneValues.GetX() != 0.0f) ||
                                     (m_axisData2D.m_preDeadZoneValues.GetY() != 0.0f);
        UpdateState(isChannelActive);
    }
} // namespace AzFramework
