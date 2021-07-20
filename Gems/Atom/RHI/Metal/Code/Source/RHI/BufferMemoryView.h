/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/MemoryView.h>

namespace AZ
{
    namespace Metal
    {
        enum class BufferMemoryType : uint32_t
        {
            /// This buffer owns its own memory resource. Attachments on the frame scheduler require
            /// this type.
            Unique,

            /// This buffer shares a memory resource with other buffers through sub-allocation.
            SubAllocated
        };

        class BufferMemoryView
            : public MemoryView
        {
        public:
            BufferMemoryView() = default;
            BufferMemoryView(const MemoryView& memoryView, BufferMemoryType memoryType);
            BufferMemoryView(MemoryView&& memoryView, BufferMemoryType memoryType);

            /// Supports copy and move construction / assignment.
            BufferMemoryView(const BufferMemoryView& rhs) = default;
            BufferMemoryView(BufferMemoryView&& rhs) = default;
            BufferMemoryView& operator=(const BufferMemoryView& rhs) = default;
            BufferMemoryView& operator=(BufferMemoryView&& rhs) = default;

            BufferMemoryType GetType() const;

        private:
            BufferMemoryType m_type = BufferMemoryType::Unique;
        };
    }
}
