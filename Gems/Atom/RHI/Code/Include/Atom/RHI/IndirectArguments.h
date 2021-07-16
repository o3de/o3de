/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/IndirectBufferView.h>

namespace AZ
{
    namespace RHI
    {
        //! Encapsulates the arguments needed when doing an indirect call
        //! (draw or dispatch) into a command list.
        struct IndirectArguments
        {
            IndirectArguments() = default;

            IndirectArguments(
                uint32_t maxSequenceCount,
                const IndirectBufferView& indirectBuffer,
                uint64_t indirectBufferByteOffset)
                : IndirectArguments(
                    maxSequenceCount,
                    indirectBuffer,
                    indirectBufferByteOffset,
                    nullptr,
                    0)
            {}

            IndirectArguments(
                uint32_t maxSequenceCount,
                const IndirectBufferView& indirectBuffer,
                uint64_t indirectBufferByteOffset,
                const Buffer* countBuffer,
                uint64_t countBufferByteOffset)
                : m_maxSequenceCount(maxSequenceCount)
                , m_indirectBufferView(&indirectBuffer)
                , m_indirectBufferByteOffset(indirectBufferByteOffset)
                , m_countBuffer(countBuffer)
                , m_countBufferByteOffset(countBufferByteOffset)
            {}

            //! There are two ways that m_maxSequenceCount can be specified:
            //! 1) If m_countBuffer is not NULL, then m_maxSequenceCount specifies the maximum number of operations which will be performed.
            //!    The actual number of operations to be performed are defined by the minimum of this value, and a 32 bit unsigned integer
            //!    contained in m_countBuffer(at the byte offset specified by m_countBufferByteOffset).
            //! 2) If m_countBuffer is NULL, the m_maxSequenceCount specifies the exact number of operations which will be performed.
            uint32_t m_maxSequenceCount = 0;

            //! Specifies an offset into IndirectBufferView to identify the first command argument.
            uint64_t m_indirectBufferByteOffset = 0;
            //! Specifies an offset into m_countBuffer, identifying the argument count.
            uint64_t m_countBufferByteOffset = 0;

            //! View over the Indirect buffer that contains the commands.
            const IndirectBufferView* m_indirectBufferView = nullptr;

            //! Optional count buffer that contains the number of indirect commands in the indirect buffer.
            const Buffer* m_countBuffer = nullptr;
        };
    }
}
