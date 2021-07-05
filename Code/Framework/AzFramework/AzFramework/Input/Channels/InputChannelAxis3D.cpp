/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Channels/InputChannelAxis3D.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelAxis3D::InputChannelAxis3D(const InputChannelId& inputChannelId,
                                           const InputDevice& inputDevice)
        : InputChannel(inputChannelId, inputDevice)
        , m_axisData3D()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    float InputChannelAxis3D::GetValue() const
    {
        return m_axisData3D.m_values.GetLength();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    float InputChannelAxis3D::GetDelta() const
    {
        return m_axisData3D.m_deltas.GetLength();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputChannel::CustomData* InputChannelAxis3D::GetCustomData() const
    {
        return &m_axisData3D;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelAxis3D::ResetState()
    {
        m_axisData3D = AxisData3D();
        InputChannel::ResetState();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelAxis3D::SimulateRawInput3D(float rawValueX, float rawValueY, float rawValueZ)
    {
        const AZ::Vector3 rawValues(rawValueX, rawValueY, rawValueZ);
        ProcessRawInputEvent(rawValues);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelAxis3D::ProcessRawInputEvent(const AZ::Vector3& rawValues)
    {
        const AZ::Vector3 oldValues = m_axisData3D.m_values;
        m_axisData3D.m_values = rawValues;
        m_axisData3D.m_deltas = rawValues - oldValues;

        const bool isChannelActive = (m_axisData3D.m_values.GetX() != 0.0f) ||
                                     (m_axisData3D.m_values.GetY() != 0.0f) ||
                                     (m_axisData3D.m_values.GetZ() != 0.0f);
        UpdateState(isChannelActive);
    }
} // namespace AzFramework
