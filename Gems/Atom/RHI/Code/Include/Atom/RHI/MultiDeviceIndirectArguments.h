/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/IndirectArguments.h>
#include <Atom/RHI/MultiDeviceBuffer.h>
#include <Atom/RHI/MultiDeviceIndirectBufferView.h>

namespace AZ::RHI
{
    //! Encapsulates the arguments needed when doing an indirect call
    //! (draw or dispatch) into a command list.
    struct MultiDeviceIndirectArguments
    {
        MultiDeviceIndirectArguments() = default;

        MultiDeviceIndirectArguments(
            uint32_t maxSequenceCount, const MultiDeviceIndirectBufferView& indirectBuffer, uint64_t indirectBufferByteOffset)
            : MultiDeviceIndirectArguments(maxSequenceCount, indirectBuffer, indirectBufferByteOffset, nullptr, 0)
        {
        }

        MultiDeviceIndirectArguments(
            uint32_t maxSequenceCount,
            const MultiDeviceIndirectBufferView& indirectBuffer,
            uint64_t indirectBufferByteOffset,
            const MultiDeviceBuffer* countBuffer,
            uint64_t countBufferByteOffset)
            : m_maxSequenceCount(maxSequenceCount)
            , m_mdIndirectBufferView(&indirectBuffer)
            , m_indirectBufferByteOffset(indirectBufferByteOffset)
            , m_mdCountBuffer(countBuffer)
            , m_countBufferByteOffset(countBufferByteOffset)
        {
        }

        //! Returns the device-specific IndirectArguments for the given index
        IndirectArguments GetDeviceIndirectArguments(int deviceIndex) const
        {
            AZ_Assert(m_mdIndirectBufferView, "No MultiDeviceIndirectBufferView available\n");
            AZ_Assert(m_mdCountBuffer, "No MultiDeviceBuffer available\n");

            return IndirectArguments{ m_maxSequenceCount,
                                      m_mdIndirectBufferView->GetDeviceIndirectBufferView(deviceIndex),
                                      m_indirectBufferByteOffset,
                                      m_mdCountBuffer ? m_mdCountBuffer->GetDeviceBuffer(deviceIndex).get() : nullptr,
                                      m_countBufferByteOffset };
        }

        //! There are two ways that m_maxSequenceCount can be specified:
        //! 1) If m_mdCountBuffer is not NULL, then m_maxSequenceCount specifies the maximum number of operations which will be performed.
        //!    The actual number of operations to be performed are defined by the minimum of this value, and a 32 bit unsigned integer
        //!    contained in m_mdCountBuffer(at the byte offset specified by m_countBufferByteOffset).
        //! 2) If m_mdCountBuffer is NULL, the m_maxSequenceCount specifies the exact number of operations which will be performed.
        uint32_t m_maxSequenceCount = 0;

        //! Specifies an offset into MultiDeviceIndirectBufferView to identify the first command argument.
        uint64_t m_indirectBufferByteOffset = 0;
        //! Specifies an offset into m_mdCountBuffer, identifying the argument count.
        uint64_t m_countBufferByteOffset = 0;

        //! View over the Indirect buffer that contains the commands.
        const MultiDeviceIndirectBufferView* m_mdIndirectBufferView = nullptr;

        //! Optional count buffer that contains the number of indirect commands in the indirect buffer.
        const MultiDeviceBuffer* m_mdCountBuffer = nullptr;
    };
} // namespace AZ::RHI
