/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/BufferMemoryView.h>

namespace AZ
{
    namespace DX12
    {
        BufferMemoryView::BufferMemoryView(
            const MemoryView& memoryView,
            BufferMemoryType memoryType)
            : MemoryView(memoryView)
            , m_type{memoryType}
        {}

        BufferMemoryView::BufferMemoryView(
            MemoryView&& memoryView,
            BufferMemoryType memoryType)
            : MemoryView(AZStd::move(memoryView))
            , m_type{memoryType}
        {}

        BufferMemoryType BufferMemoryView::GetType() const
        {
            return m_type;
        }
    }
}
