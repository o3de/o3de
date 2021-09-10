/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/MemoryView.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Debug/EventTrace.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/conversions.h>

namespace AZ
{
    namespace DX12
    {
        MemoryView::MemoryView(const MemoryAllocation& memAllocation, MemoryViewType viewType)
            : m_memoryAllocation(memAllocation)
            , m_viewType(viewType)
        {
            Construct();
        }

        MemoryView::MemoryView(RHI::Ptr<Memory> memory, size_t offset, size_t size, size_t alignment, MemoryViewType viewType)
            : MemoryView(MemoryAllocation(memory, offset, size, alignment), viewType)
        {
        }

        bool MemoryView::IsValid() const
        {
            return m_memoryAllocation.m_memory != nullptr;
        }

        Memory* MemoryView::GetMemory() const
        {
            return m_memoryAllocation.m_memory.get();
        }

        size_t MemoryView::GetOffset() const
        {
            return m_memoryAllocation.m_offset;
        }

        size_t MemoryView::GetSize() const
        {
            return m_memoryAllocation.m_size;
        }

        size_t MemoryView::GetAlignment() const
        {
            return m_memoryAllocation.m_alignment;
        }

        CpuVirtualAddress MemoryView::Map(RHI::HostMemoryAccess hostAccess) const
        {
            AZ_TRACE_METHOD();

            CpuVirtualAddress cpuAddress = nullptr;
            D3D12_RANGE readRange = {};
            if (hostAccess == RHI::HostMemoryAccess::Read)
            {
                readRange.Begin = m_memoryAllocation.m_offset;
                readRange.End = m_memoryAllocation.m_offset + m_memoryAllocation.m_size;
            }
            m_memoryAllocation.m_memory->Map(0, &readRange, reinterpret_cast<void**>(&cpuAddress));

            // Make sure we return null if the map operation failed.
            if (cpuAddress)
            {
                cpuAddress += m_memoryAllocation.m_offset;
            }
            return cpuAddress;
        }

        void MemoryView::Unmap(RHI::HostMemoryAccess hostAccess) const
        {
            D3D12_RANGE writeRange = {};
            if (hostAccess == RHI::HostMemoryAccess::Write)
            {
                writeRange.Begin = m_memoryAllocation.m_offset;
                writeRange.End = m_memoryAllocation.m_offset + m_memoryAllocation.m_size;
            }
            m_memoryAllocation.m_memory->Unmap(0, &writeRange);
        }

        GpuVirtualAddress MemoryView::GetGpuAddress() const
        {
            return m_gpuAddress;
        }

        void MemoryView::SetName(const AZStd::string_view& name)
        {
            if (m_memoryAllocation.m_memory)
            {
                AZStd::wstring wname;
                AZStd::to_wstring(wname, name);
                m_memoryAllocation.m_memory->SetPrivateData(
                    WKPDID_D3DDebugObjectNameW, aznumeric_cast<unsigned int>(wname.size() * sizeof(wchar_t)), wname.data());
            }
        }

        void MemoryView::SetName(const AZStd::wstring_view& name)
        {
            if (m_memoryAllocation.m_memory)
            {
                m_memoryAllocation.m_memory->SetPrivateData(
                    WKPDID_D3DDebugObjectNameW, aznumeric_cast<unsigned int>(name.size() * sizeof(wchar_t)), name.data());
            }
        }

        void MemoryView::Construct()
        {
            if (m_memoryAllocation.m_memory)
            {
                if (m_viewType == MemoryViewType::Image)
                {
                    // Image resources will always be zero. The address will only be valid for buffer 
                    m_gpuAddress = 0;

                    AZ_Assert(m_memoryAllocation.m_offset == 0, "Image memory does not support local offsets.");
                }
                else
                {
                    m_gpuAddress = m_memoryAllocation.m_memory->GetGPUVirtualAddress();
                }

                m_gpuAddress += m_memoryAllocation.m_offset;
            }
        }
    }
}
