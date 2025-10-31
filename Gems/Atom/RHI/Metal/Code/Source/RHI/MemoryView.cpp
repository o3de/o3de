/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/string/string.h>
#include <AzCore/std/string/conversions.h>
#include <RHI/MemoryView.h>

namespace AZ
{
    namespace Metal
    {
        MemoryView::MemoryView(RHI::Ptr<Memory> memory, size_t offset, size_t size, size_t alignment)
            : MemoryView(MemoryAllocation(memory, offset, size, alignment))
        {
        }

        MemoryView::MemoryView(const MemoryAllocation& memAllocation)
            : m_memoryAllocation(memAllocation)
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
        
        void MemoryView::SetName(const AZStd::string_view& name)
        {
            if (RHI::Validation::IsEnabled() && m_memoryAllocation.m_memory)
            {
                m_memoryAllocation.m_memory->SetName(Name(name.data()));
                
                //Set the label for GPU Capture related debugging
                NSString* labelName = [NSString stringWithCString:name.data() encoding:NSUTF8StringEncoding];
                switch(m_memoryAllocation.m_memory->GetResourceType())
                {
                    case ResourceType::MtlTextureType:
                    {
                        m_memoryAllocation.m_memory->GetGpuAddress<id<MTLTexture>>().label = labelName;
                        break;
                    }
                    case ResourceType::MtlBufferType:
                    {
                        m_memoryAllocation.m_memory->GetGpuAddress<id<MTLBuffer>>().label = labelName;
                        break;
                    }
                    default:
                    {
                        AZ_Assert(false, "Undefined type");
                    }
                }
            }
        }
    
        MTLStorageMode MemoryView::GetStorageMode()
        {
            return m_memoryAllocation.m_memory->GetGpuAddress<id<MTLResource>>().storageMode;
        }
    }
}
