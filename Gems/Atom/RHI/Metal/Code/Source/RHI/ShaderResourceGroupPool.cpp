/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/EventTrace.h>
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

        RHI::ResultCode ShaderResourceGroupPool::InitGroupInternal(RHI::ShaderResourceGroup& groupBase)
        {
            ShaderResourceGroup& group = static_cast<ShaderResourceGroup&>(groupBase);
            
            for (size_t i = 0; i < RHI::Limits::Device::FrameCountMax; ++i)
            {
                auto argBuffer = ArgumentBuffer::Create();
                argBuffer->Init(m_device, m_srgLayout, group, this);
                group.m_compiledArgBuffers[i] = argBuffer;
            }

            return RHI::ResultCode::Success;
        }

        void ShaderResourceGroupPool::ShutdownResourceInternal(RHI::Resource& resourceBase)
        {
            ShaderResourceGroup& group = static_cast<ShaderResourceGroup&>(resourceBase);
            for (size_t i = 0; i < RHI::Limits::Device::FrameCountMax; ++i)
            {
                group.m_compiledArgBuffers[i] = nullptr;
            }
            Base::ShutdownResourceInternal(resourceBase);
        }

        RHI::ResultCode ShaderResourceGroupPool::CompileGroupInternal(RHI::ShaderResourceGroup& groupBase, const RHI::ShaderResourceGroupData& groupData)
        {
            ShaderResourceGroup& group = static_cast<ShaderResourceGroup&>(groupBase);
            group.UpdateCompiledDataIndex();

            if (!groupData.IsAnyResourceTypeUpdated())
            {
                return RHI::ResultCode::Success;
            }

            ArgumentBuffer& argBuffer = *group.m_compiledArgBuffers[group.m_compiledDataIndex];
            argBuffer.ClearResourceTracking();

            auto constantData = groupData.GetConstantData();
            if (!constantData.empty() && groupData.IsResourceTypeEnabledForCompilation(static_cast<uint32_t>(RHI::ShaderResourceGroupData::ResourceTypeMask::ConstantDataMask)))
            {
                argBuffer.UpdateConstantBufferViews(groupData.GetConstantData());
            }

            const RHI::ShaderResourceGroupLayout* layout = groupData.GetLayout();
            uint32_t shaderInputIndex = 0;
            if (groupData.IsResourceTypeEnabledForCompilation(static_cast<uint32_t>(RHI::ShaderResourceGroupData::ResourceTypeMask::ImageViewMask)))
            {
                for (const RHI::ShaderInputImageDescriptor& shaderInputImage : layout->GetShaderInputListForImages())
                {
                    const RHI::ShaderInputImageIndex imageInputIndex(shaderInputIndex);
                    AZStd::array_view<RHI::ConstPtr<RHI::ImageView>> imageViews = groupData.GetImageViewArray(imageInputIndex);
                    argBuffer.UpdateImageViews(shaderInputImage, imageInputIndex, imageViews);
                    ++shaderInputIndex;
                }
            }

            if (groupData.IsResourceTypeEnabledForCompilation(static_cast<uint32_t>(RHI::ShaderResourceGroupData::ResourceTypeMask::SamplerMask)))
            {
                shaderInputIndex = 0;
                for (const RHI::ShaderInputSamplerDescriptor& shaderInputSampler : layout->GetShaderInputListForSamplers())
                {
                    const RHI::ShaderInputSamplerIndex samplerInputIndex(shaderInputIndex);
                    AZStd::array_view<RHI::SamplerState> samplerStates = groupData.GetSamplerArray(samplerInputIndex);
                    argBuffer.UpdateSamplers(shaderInputSampler, samplerInputIndex, samplerStates);
                    ++shaderInputIndex;
                }
            }

            if (groupData.IsResourceTypeEnabledForCompilation(static_cast<uint32_t>(RHI::ShaderResourceGroupData::ResourceTypeMask::BufferViewMask)))
            {
                shaderInputIndex = 0;
                for (const RHI::ShaderInputBufferDescriptor& shaderInputBuffer : layout->GetShaderInputListForBuffers())
                {
                    const RHI::ShaderInputBufferIndex bufferInputIndex(shaderInputIndex);
                    AZStd::array_view<RHI::ConstPtr<RHI::BufferView>> bufferViews = groupData.GetBufferViewArray(bufferInputIndex);
                    argBuffer.UpdateBufferViews(shaderInputBuffer, bufferInputIndex, bufferViews);
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
