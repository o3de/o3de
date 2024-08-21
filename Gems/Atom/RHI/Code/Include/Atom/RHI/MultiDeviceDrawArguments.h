/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Limits.h>
#include <Atom/RHI/DrawArguments.h>
#include <Atom/RHI/IndirectArguments.h>
#include <Atom/RHI/MultiDeviceIndirectArguments.h>

namespace AZ::RHI
{
    using MultiDeviceDrawIndirect = MultiDeviceIndirectArguments;

    //! A structure used to define the type of draw that should happen, directly passed on to the device-specific DrawItems in
    //! MultiDeviceDrawItem::SetArguments
    struct MultiDeviceDrawArguments
    {
        AZ_TYPE_INFO(MultiDeviceDrawArguments, "B8127BDE-513E-4D5C-98C2-027BA1DE9E6E");

        MultiDeviceDrawArguments()
            : MultiDeviceDrawArguments(DrawIndexed{})
        {
        }

        MultiDeviceDrawArguments(const DrawIndexed& indexed)
            : m_type{ DrawType::Indexed }
            , m_indexed{ indexed }
        {
        }

        MultiDeviceDrawArguments(const DrawLinear& linear)
            : m_type{ DrawType::Linear }
            , m_linear{ linear }
        {
        }

        MultiDeviceDrawArguments(const MultiDeviceDrawIndirect& indirect)
            : m_type{ DrawType::Indirect }
            , m_mdIndirect{ indirect }
        {
        }

        //! Returns the device-specific DrawArguments for the given index
        DrawArguments GetDeviceDrawArguments(int deviceIndex) const
        {
            switch (m_type)
            {
            case DrawType::Indexed:
                return DrawArguments(m_indexed);
            case DrawType::Linear:
                return DrawArguments(m_linear);
            case DrawType::Indirect:
                return DrawArguments(DrawIndirect{ m_mdIndirect.m_maxSequenceCount, m_mdIndirect.m_indirectBufferView->GetDeviceIndirectBufferView(deviceIndex), m_mdIndirect.m_indirectBufferByteOffset, m_mdIndirect.m_countBuffer->GetDeviceBuffer(deviceIndex).get(), m_mdIndirect.m_countBufferByteOffset });
            default:
                return DrawArguments();
            }
        }

        DrawType m_type;
        union {
            DrawIndexed m_indexed;
            DrawLinear m_linear;
            MultiDeviceDrawIndirect m_mdIndirect;
        };
    };
}
