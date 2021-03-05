/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "RHI/Atom_RHI_DX12_precompiled.h"
#include <RHI/Buffer.h>
#include <Atom/RHI/MemoryStatisticsBuilder.h>

namespace AZ
{
    namespace DX12
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

        void Buffer::ReportMemoryUsage(RHI::MemoryStatisticsBuilder& builder) const
        {
            const RHI::BufferDescriptor& descriptor = GetDescriptor();

            RHI::MemoryStatistics::Buffer* bufferStats = builder.AddBuffer();
            bufferStats->m_name = GetName();
            bufferStats->m_bindFlags = descriptor.m_bindFlags;
            bufferStats->m_sizeInBytes = m_memoryView.GetSize();
        }
    }
}