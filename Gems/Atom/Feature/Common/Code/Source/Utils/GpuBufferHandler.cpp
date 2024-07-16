/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/Utils/GpuBufferHandler.h>
#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RPI.Public/Buffer/BufferSystemInterface.h>
#include <Atom/Utils/Utils.h>
#include <cinttypes>

namespace AZ
{
    namespace Render
    {
        [[maybe_unused]] static const char* ClassName = "GpuBufferHandler";
        static const uint32_t BufferMinSize = 1 << 16; // Min 64Kb.

        GpuBufferHandler::GpuBufferHandler(const Descriptor& descriptor)
        {
            m_elementSize = descriptor.m_elementFormat == RHI::Format::Unknown ? descriptor.m_elementSize
                                                                               : RHI::GetFormatSize(descriptor.m_elementFormat);
            m_elementCount = 0;
            
            m_bufferIndex = descriptor.m_srgLayout->FindShaderInputBufferIndex(Name(descriptor.m_bufferSrgName));
            AZ_Error(ClassName, m_bufferIndex.IsValid(), "Unable to find %s in %s shader resource group.", descriptor.m_bufferSrgName.c_str(), descriptor.m_srgLayout->GetName().GetCStr());

            if (!descriptor.m_elementCountSrgName.empty())
            {
                m_elementCountIndex = descriptor.m_srgLayout->FindShaderInputConstantIndex(Name(descriptor.m_elementCountSrgName));
                AZ_Error(ClassName, m_elementCountIndex.IsValid(), "Unable to find %s in %s shader resource group.", descriptor.m_elementCountSrgName.c_str(), descriptor.m_srgLayout->GetName().GetCStr());
            }

            if (m_bufferIndex.IsValid())
            {
                uint32_t byteCount = RHI::NextPowerOfTwo(GetMax<uint32_t>(BufferMinSize, m_elementCount * m_elementSize));
                
                RPI::CommonBufferDescriptor desc;
                desc.m_poolType = RPI::CommonBufferPoolType::ReadOnly;
                desc.m_bufferName = descriptor.m_bufferName;
                desc.m_byteCount = byteCount;
                desc.m_elementSize = m_elementSize;
                desc.m_elementFormat = descriptor.m_elementFormat;

                m_buffer = RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
            }
        }

        bool GpuBufferHandler::IsValid() const
        {
            // Consider this is valid if the buffer is valid.
           return m_buffer;
        }

        void GpuBufferHandler::Release()
        {
            m_buffer = nullptr;
            m_bufferIndex.Reset();
            m_elementCountIndex.Reset();
        }

        bool GpuBufferHandler::UpdateBuffer(uint32_t elementCount, const void* data)
        {
            if (!IsValid())
            {
                return false;
            }

            m_elementCount = elementCount;

            AZ::u64 currentByteCount = m_buffer->GetBufferSize();
            uint32_t dataSize = elementCount * m_elementSize;

            if (dataSize > currentByteCount)
            {
                uint32_t byteCount = RHI::NextPowerOfTwo(GetMax<uint32_t>(BufferMinSize, dataSize));
                m_buffer->Resize(byteCount);
            }

            if (dataSize > 0)
            {
                return m_buffer->UpdateData(data, dataSize, 0);
            }
            return true;
        }

        bool GpuBufferHandler::UpdateBuffer(const AZStd::unordered_map<int, const void*>& data, uint32_t elementCount)
        {
            if (!IsValid())
            {
                return false;
            }

            m_elementCount = elementCount;

            AZ::u64 currentByteCount = m_buffer->GetBufferSize();
            uint32_t dataSize = elementCount * m_elementSize;

            if (dataSize > currentByteCount)
            {
                uint32_t byteCount = RHI::NextPowerOfTwo(GetMax<uint32_t>(BufferMinSize, dataSize));
                m_buffer->Resize(byteCount);
            }

            if (dataSize > 0)
            {
                return m_buffer->UpdateData(data, dataSize, 0);
            }
            return true;
        }

        void GpuBufferHandler::UpdateSrg(RPI::ShaderResourceGroup* srg) const
        {
            if (m_bufferIndex.IsValid())
            {
                srg->SetBufferView(m_bufferIndex, m_buffer->GetBufferView());
            }
            if (m_elementCountIndex.IsValid())
            {
                srg->SetConstant<uint32_t>(m_elementCountIndex, m_elementCount);
            }
        }

    } // namespace Render
} // namespace AZ
