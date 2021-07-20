/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/Buffer.h>

namespace AZ
{
    namespace Metal
    {
        RHI::Ptr<Buffer> Buffer::Create()
        {
            return aznew Buffer();
        }

        const MemoryView& Buffer::GetMemoryView() const
        {
            return m_memoryView;
        }
        
        MemoryView& Buffer::GetMemoryView()
        {
            return m_memoryView;
        }
        
        void Buffer::SetNameInternal(const AZStd::string_view& name)
        {
            if (m_memoryView.GetType() == BufferMemoryType::Unique)
            {
                m_memoryView.SetName(name);
            }
        }

        void Buffer::SetMapRequestOffset(const uint32_t mapRequestOffset)
        {
            m_mapRequestOffset = mapRequestOffset;
        }
    
        const uint32_t Buffer::GetMapRequestOffset() const
        {
            return m_mapRequestOffset;
        }
    
        void Buffer::ReportMemoryUsage(RHI::MemoryStatisticsBuilder& builder) const
        {
            //[GFX TODO][ATOM-493] - Report memory usage support
        }
    }
}
