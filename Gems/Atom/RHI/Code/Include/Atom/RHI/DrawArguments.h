/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Limits.h>
#include <Atom/RHI/DeviceDrawArguments.h>
#include <Atom/RHI/IndirectArguments.h>

namespace AZ::RHI
{
    using DrawIndirect = IndirectArguments;

    //! A structure used to define the type of draw that should happen, directly passed on to the device-specific DrawItems in
    //! DrawItem::SetArguments
    struct DrawArguments
    {
        AZ_TYPE_INFO(DrawArguments, "B8127BDE-513E-4D5C-98C2-027BA1DE9E6E");

        DrawArguments()
            : DrawArguments(DrawIndexed{})
        {
        }

        DrawArguments(const DrawIndexed& indexed)
            : m_type{ DrawType::Indexed }
            , m_indexed{ indexed }
        {
        }

        DrawArguments(const DrawLinear& linear)
            : m_type{ DrawType::Linear }
            , m_linear{ linear }
        {
        }

        DrawArguments(const DrawIndirect& indirect)
            : m_type{ DrawType::Indirect }
            , m_indirect{ indirect }
        {
        }

        //! Returns the device-specific DeviceDrawArguments for the given index
        DeviceDrawArguments GetDeviceDrawArguments(int deviceIndex) const
        {
            switch (m_type)
            {
            case DrawType::Indexed:
                return DeviceDrawArguments(m_indexed);
            case DrawType::Linear:
                return DeviceDrawArguments(m_linear);
            case DrawType::Indirect:
                return DeviceDrawArguments(DeviceDrawIndirect{ m_indirect.m_maxSequenceCount, m_indirect.m_indirectBufferView->GetDeviceIndirectBufferView(deviceIndex), m_indirect.m_indirectBufferByteOffset, m_indirect.m_countBuffer ? m_indirect.m_countBuffer->GetDeviceBuffer(deviceIndex).get() : nullptr, m_indirect.m_countBufferByteOffset });
            default:
                return DeviceDrawArguments();
            }
        }

        DrawType m_type;
        union {
            DrawIndexed m_indexed;
            DrawLinear m_linear;
            DrawIndirect m_indirect;
        };
    };
}
