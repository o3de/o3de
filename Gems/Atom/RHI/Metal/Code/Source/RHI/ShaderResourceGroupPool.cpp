/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/ArgumentBuffer.h>
#include <RHI/Conversions.h>
#include <RHI/Device.h>
#include <RHI/ShaderResourceGroup.h>
#include <RHI/ShaderResourceGroupPool.h>

namespace AZ
{
    namespace Metal
    {
        RHI::Ptr<ShaderResourceGroupPool> ShaderResourceGroupPool::Create()
        {
            return aznew ShaderResourceGroupPool();
        }

        RHI::ResultCode ShaderResourceGroupPool::InitInternal(RHI::Device& deviceBase, const RHI::ShaderResourceGroupPoolDescriptor& descriptor)
        {
            Device& device = static_cast<Device&>(deviceBase);
            m_device = &device;
            m_srgLayout = descriptor.m_layout;
            return RHI::ResultCode::Success;
        }

        void ShaderResourceGroupPool::ShutdownInternal()
        {
            Base::ShutdownInternal();
        }

        RHI::ResultCode ShaderResourceGroupPool::InitGroupInternal(RHI::DeviceShaderResourceGroup& groupBase)
        {
            ShaderResourceGroup& group = static_cast<ShaderResourceGroup&>(groupBase);

            for (size_t i = 0; i < RHI::Limits::Device::FrameCountMax; ++i)
            {
                auto argBuffer = ArgumentBuffer::Create();
                argBuffer->Init(m_device, m_srgLayout, this);
                group.m_compiledArgBuffers[i] = argBuffer;
            }

            return RHI::ResultCode::Success;
        }

        void ShaderResourceGroupPool::ShutdownResourceInternal(RHI::DeviceResource& resourceBase)
        {
            ShaderResourceGroup& group = static_cast<ShaderResourceGroup&>(resourceBase);
            for (size_t i = 0; i < RHI::Limits::Device::FrameCountMax; ++i)
            {
                group.m_compiledArgBuffers[i] = nullptr;
            }
            Base::ShutdownResourceInternal(resourceBase);
        }

        RHI::ResultCode ShaderResourceGroupPool::CompileGroupInternal(RHI::DeviceShaderResourceGroup& groupBase, const RHI::DeviceShaderResourceGroupData& groupData)
        {
            typedef AZ::RHI::DeviceShaderResourceGroupData::ResourceTypeMask ResourceMask;
            ShaderResourceGroup& group = static_cast<ShaderResourceGroup&>(groupBase);

            group.UpdateCompiledDataIndex();
            ArgumentBuffer& argBuffer = *group.m_compiledArgBuffers[group.m_compiledDataIndex];

            auto constantData = groupData.GetConstantData();
            if (!constantData.empty() && groupBase.IsResourceTypeEnabledForCompilation(static_cast<uint32_t>(ResourceMask::ConstantDataMask)))
            {
                argBuffer.UpdateConstantBufferViews(groupData.GetConstantData());
            }

            const RHI::ShaderResourceGroupLayout* layout = groupData.GetLayout();
            uint32_t shaderInputIndex = 0;
            if (groupBase.IsResourceTypeEnabledForCompilation(static_cast<uint32_t>(ResourceMask::ImageViewMask)))
            {
                for (const RHI::ShaderInputImageDescriptor& shaderInputImage : layout->GetShaderInputListForImages())
                {
                    const RHI::ShaderInputImageIndex imageInputIndex(shaderInputIndex);
                    AZStd::span<const RHI::ConstPtr<RHI::DeviceImageView>> imageViews = groupData.GetImageViewArray(imageInputIndex);
                    argBuffer.UpdateImageViews(shaderInputImage, imageViews);
                    ++shaderInputIndex;
                }
            }

            if (groupBase.IsResourceTypeEnabledForCompilation(static_cast<uint32_t>(ResourceMask::BufferViewMask)))
            {
                shaderInputIndex = 0;
                for (const RHI::ShaderInputBufferDescriptor& shaderInputBuffer : layout->GetShaderInputListForBuffers())
                {
                    const RHI::ShaderInputBufferIndex bufferInputIndex(shaderInputIndex);
                    AZStd::span<const RHI::ConstPtr<RHI::DeviceBufferView>> bufferViews = groupData.GetBufferViewArray(bufferInputIndex);
                    argBuffer.UpdateBufferViews(shaderInputBuffer, bufferViews);
                    ++shaderInputIndex;
                }
            }
            
            if (groupBase.IsResourceTypeEnabledForCompilation(static_cast<uint32_t>(ResourceMask::SamplerMask)))
            {
                shaderInputIndex = 0;
                for (const RHI::ShaderInputSamplerDescriptor& shaderInputSampler : layout->GetShaderInputListForSamplers())
                {
                    const RHI::ShaderInputSamplerIndex samplerInputIndex(shaderInputIndex);
                    AZStd::span<const RHI::SamplerState> samplerStates = groupData.GetSamplerArray(samplerInputIndex);
                    argBuffer.UpdateSamplers(shaderInputSampler, samplerStates);
                    ++shaderInputIndex;
                }
            }
            
            return RHI::ResultCode::Success;
        }

        void ShaderResourceGroupPool::OnFrameEnd()
        {
            Base::OnFrameEnd();
        }

    }
}
