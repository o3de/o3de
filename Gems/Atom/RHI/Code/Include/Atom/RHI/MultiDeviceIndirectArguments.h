/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/MultiDeviceBuffer.h>
#include <Atom/RHI/MultiDeviceIndirectBufferView.h>
#include <Atom/RHI/IndirectArguments.h>

namespace AZ
{
    namespace RHI
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
                , m_indirectBufferView(&indirectBuffer)
                , m_indirectBufferByteOffset(indirectBufferByteOffset)
                , m_countBuffer(countBuffer)
                , m_countBufferByteOffset(countBufferByteOffset)
            {
            }

            IndirectArguments GetDeviceIndirectArguments(int deviceIndex) const
            {
                AZ_Assert(m_indirectBufferView, "No MultiDeviceIndirectBufferView available\n");
                AZ_Assert(m_countBuffer, "No MultiDeviceBuffer available\n");

                return IndirectArguments{ m_maxSequenceCount,
                                                      m_indirectBufferView->GetDeviceIndirectBufferView(deviceIndex),
                                                      m_indirectBufferByteOffset,
                                                      m_countBuffer ? m_countBuffer->GetDeviceBuffer(deviceIndex).get() : nullptr,
                                                      m_countBufferByteOffset };
            }

            //! There are two ways that m_maxSequenceCount can be specified:
            //! 1) If m_countBuffer is not NULL, then m_maxSequenceCount specifies the maximum number of operations which will be performed.
            //!    The actual number of operations to be performed are defined by the minimum of this value, and a 32 bit unsigned integer
            //!    contained in m_countBuffer(at the byte offset specified by m_countBufferByteOffset).
            //! 2) If m_countBuffer is NULL, the m_maxSequenceCount specifies the exact number of operations which will be performed.
            uint32_t m_maxSequenceCount = 0;

            //! Specifies an offset into MultiDeviceIndirectBufferView to identify the first command argument.
            uint64_t m_indirectBufferByteOffset = 0;
            //! Specifies an offset into m_countBuffer, identifying the argument count.
            uint64_t m_countBufferByteOffset = 0;

            //! View over the Indirect buffer that contains the commands.
            const MultiDeviceIndirectBufferView* m_indirectBufferView = nullptr;

            //! Optional count buffer that contains the number of indirect commands in the indirect buffer.
            const MultiDeviceBuffer* m_countBuffer = nullptr;
        };
    } // namespace RHI
} // namespace AZ
