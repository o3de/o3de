/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WebGPU.h>
#include <RHI/BindGroupLayout.h>
#include <RHI/Conversions.h>
#include <RHI/Device.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayout.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>
#include <AzCore/StringFunc/StringFunc.h>

namespace AZ::WebGPU
{
    AZ::HashValue64 BindGroupLayout::Descriptor::GetHash() const
    {
        return m_shaderResouceGroupLayout->GetHash();
    }

    RHI::Ptr<BindGroupLayout> BindGroupLayout::Create()
    {
        return aznew BindGroupLayout();
    }

    BindGroupLayout::~BindGroupLayout()
    {
    }

    const wgpu::BindGroupLayout& BindGroupLayout::GetNativeBindGroupLayout() const
    {
        return m_wgpuBindGroupLayout;
    }

    uint32_t BindGroupLayout::GetConstantDataSize() const
    {
        return m_constantDataSize;
    }

    const RHI::ShaderResourceGroupLayout* BindGroupLayout::GetShaderResourceGroupLayout() const
    {
        return m_shaderResourceGroupLayout.get();
    }

    const wgpu::BindGroupLayoutEntry& BindGroupLayout::GetEntry(const uint32_t index) const
    {
        return m_wgpuEntries[index];
    }

    RHI::ResultCode BindGroupLayout::Init(Device& device, const Descriptor& descriptor)
    {
        Base::Init(device);
        m_shaderResourceGroupLayout = descriptor.m_shaderResouceGroupLayout;
        m_useDynamicOffset = descriptor.m_useDynamicBuffer;

        const RHI::ResultCode result = BuildNativeDescriptorSetLayout();
        RETURN_RESULT_IF_UNSUCCESSFUL(result);

        // Name the descriptor layout from the SRG layout
        SetName(m_shaderResourceGroupLayout->GetName());
        return result;
    }

    void BindGroupLayout::SetNameInternal(const AZStd::string_view& name)
    {
        if (m_wgpuBindGroupLayout && !name.empty())
        {
            m_wgpuBindGroupLayout.SetLabel(name.data());
        }
    }

    void BindGroupLayout::Shutdown()
    {
        m_wgpuBindGroupLayout = nullptr;
        m_shaderResourceGroupLayout = nullptr;
        Base::Shutdown();
    }

    RHI::ResultCode BindGroupLayout::BuildNativeDescriptorSetLayout()
    {
        const RHI::ResultCode buildResult = BuildDescriptorSetLayoutBindings();
        RETURN_RESULT_IF_UNSUCCESSFUL(buildResult);

        wgpu::BindGroupLayoutDescriptor descriptor = {};
        descriptor.entryCount = m_wgpuEntries.size();
        descriptor.entries = m_wgpuEntries.data();
        descriptor.label = GetName().GetCStr();
        m_wgpuBindGroupLayout = static_cast<Device&>(GetDevice()).GetNativeDevice().CreateBindGroupLayout(&descriptor);
        AZ_Assert(m_wgpuBindGroupLayout, "Failed to create BindGroupLayout");
        return m_wgpuBindGroupLayout ? RHI::ResultCode::Success : RHI::ResultCode::Fail;
    }

    RHI::ResultCode BindGroupLayout::BuildDescriptorSetLayoutBindings()
    {
        const AZStd::span<const RHI::ShaderInputBufferDescriptor> bufferDescs = m_shaderResourceGroupLayout->GetShaderInputListForBuffers();
        const AZStd::span<const RHI::ShaderInputImageDescriptor> imageDescs = m_shaderResourceGroupLayout->GetShaderInputListForImages();
        const AZStd::span<const RHI::ShaderInputBufferUnboundedArrayDescriptor> bufferUnboundedArrayDescs = m_shaderResourceGroupLayout->GetShaderInputListForBufferUnboundedArrays();
        const AZStd::span<const RHI::ShaderInputImageUnboundedArrayDescriptor> imageUnboundedArrayDescs = m_shaderResourceGroupLayout->GetShaderInputListForImageUnboundedArrays();
        const AZStd::span<const RHI::ShaderInputSamplerDescriptor> samplerDescs = m_shaderResourceGroupLayout->GetShaderInputListForSamplers();
        const AZStd::span<const RHI::ShaderInputStaticSamplerDescriptor>& staticSamplerDescs = m_shaderResourceGroupLayout->GetStaticSamplers();

        // About adding all shader stages for visibility...
        // We attempted to configure the descriptor set with the actual resource visibility but it was problematic...
        // Atom currently expects to be able to use certain ShaderResourceGroup instances with many different pipeline states regardless of visibility.
        // - ShaderResourceGroupLayouts for "SceneSrg" and "ViewSrg" are defined in a special SceneAndViewSrgs.shader file. This shader has no entry points
        //   and the asset only exists to provide these SRG layouts. The runtime loads this special shader and uses it to instantiate the one
        //   SceneSrg (and ViewSrg(s)) and uses this instance for many shaders with different resource visibilities.
        // - Same for RayTracingSrgs.shader's "RayTracingSceneSrg" and "RayTracingMaterialSrg"
        // - Same for ForwardPassSrg.shader's "PassSrg". (This one is especially problematic because, unlike the above cases, we can't just add some special
        //   handling for the particular SRG name; "PassSrg" is widely used as the name for many different per-pass SRG layouts).
        // - ShaderResourceGroupPool is intentionally set up to reuse SRGs regardless of visibility, per ShaderResourceGroup::MakeInstanceId which uses
        //   the source file path in the unique ID (ex: "D:\o3de\Gems\Atom\RPI\Assets\ShaderLib\Atom\RPI\ShaderResourceGroups\DefaultDrawSrg.azsli")
        //   so that any shader that uses this azsli file will share the same pool and thus share the same PipelineLayoutDescriptor.
        // In order to address the above issues, one solution would be to update AZSLc to support some kind of attribute by which the shader-author can manually
        // override the visibility for each resource. Or we add some new metadata to the .shader files to provide explicit overrides for particular resource
        // visibilities. Either way, this would likely become error prone and difficult to maintain.
        // 
        static const wgpu::ShaderStage DefaultShaderStageVisibility =
            wgpu::ShaderStage::Fragment | wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Compute;

        m_constantDataSize = m_shaderResourceGroupLayout->GetConstantDataSize();
        if (m_constantDataSize)
        {
            AZStd::span<const RHI::ShaderInputConstantDescriptor> inputListForConstants = m_shaderResourceGroupLayout->GetShaderInputListForConstants();
            AZ_Assert(!inputListForConstants.empty(), "Empty constant input list");

            wgpu::BindGroupLayoutEntry& entry = m_wgpuEntries.emplace_back(wgpu::BindGroupLayoutEntry{});
            // All constant data of the SRG have the same binding.
            entry.binding = inputListForConstants[0].m_registerId;
            entry.visibility = DefaultShaderStageVisibility;
            entry.buffer.type = wgpu::BufferBindingType::Uniform;
            entry.buffer.hasDynamicOffset = false;
        }

        // Buffers
        for (uint32_t index = 0; index < bufferDescs.size(); ++index)
        {
            const RHI::ShaderInputBufferDescriptor& desc = bufferDescs[index];
            wgpu::BindGroupLayoutEntry entry{};
            entry.binding = desc.m_registerId;
            entry.visibility = DefaultShaderStageVisibility;
            entry.buffer.hasDynamicOffset = m_useDynamicOffset;
            entry.buffer.minBindingSize = 0;
            switch (desc.m_access)
            {
            case RHI::ShaderInputBufferAccess::Constant:
                entry.buffer.type = wgpu::BufferBindingType::Uniform;
                break;
            case RHI::ShaderInputBufferAccess::Read:
                entry.buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;
                break;
            case RHI::ShaderInputBufferAccess::ReadWrite:
                entry.buffer.type = wgpu::BufferBindingType::Storage;
                // Storage buffers cannot be accessed from the vertex stage
                entry.visibility &= ~wgpu::ShaderStage::Vertex;
                break;
            default:
                AZ_Assert(false, "Invalid ShaderInputBufferAccess.");
                return RHI::ResultCode::InvalidArgument;
            }

            // Arrays at not supported yet on WebGPU, so we add them as consecutive buffers (shaders are modified to fit this)
            for (uint32_t arrayIndex = 0; arrayIndex < desc.m_count; ++arrayIndex)
            {
                m_wgpuEntries.emplace_back(entry);
                entry.binding++;
            }
        }

        // Images
        for (uint32_t index = 0; index < imageDescs.size(); ++index)
        {
            const RHI::ShaderInputImageDescriptor& desc = imageDescs[index];
            wgpu::BindGroupLayoutEntry entry{};
            entry.binding = desc.m_registerId;
            entry.visibility = DefaultShaderStageVisibility;
            switch (desc.m_access)
            {
            case RHI::ShaderInputImageAccess::Read:
                entry.texture.multisampled = desc.m_type == RHI::ShaderInputImageType::Image2DMultisample ||
                    desc.m_type == RHI::ShaderInputImageType::Image2DMultisampleArray;
                entry.texture.sampleType =
                    entry.texture.multisampled ? wgpu::TextureSampleType::UnfilterableFloat : ConvertSampleType(desc.m_sampleType);
                entry.texture.viewDimension = ConvertImageType(desc.m_type);
                break;
            case RHI::ShaderInputImageAccess::ReadWrite:
                entry.storageTexture.access = wgpu::StorageTextureAccess::ReadWrite;
                entry.storageTexture.format = ConvertImageFormat(desc.m_format);
                entry.storageTexture.viewDimension = ConvertImageType(desc.m_type);
                break;
            default:
                AZ_Assert(false, "Invalid ShaderInputImageAccess.");
                return RHI::ResultCode::InvalidArgument;
            }

            // Arrays at not supported yet on WebGPU, so we add them as consecutive images (shaders are modified to fit this)
            for (uint32_t arrayIndex = 0; arrayIndex < desc.m_count; ++arrayIndex)
            {
                m_wgpuEntries.emplace_back(entry);
                entry.binding++;
            }
        }

        // Samplers
        for (uint32_t index = 0; index < samplerDescs.size(); ++index)
        {
            const RHI::ShaderInputSamplerDescriptor& desc = samplerDescs[index];
            wgpu::BindGroupLayoutEntry entry;
            entry.binding = desc.m_registerId;
            entry.visibility = DefaultShaderStageVisibility;
            entry.sampler.type = ConvertSamplerBindingType(desc.m_type);

            // Arrays at not supported yet on WebGPU, so we add them as consecutive samplers (shaders are modified to fit this)
            for (uint32_t arrayIndex = 0; arrayIndex < desc.m_count; ++arrayIndex)
            {
                m_wgpuEntries.emplace_back(entry);
                entry.binding++;
            }
        }

        // Static samplers (WebGPU doesn't support static samplers, so we use regular ones)
        for (int index = 0; index < staticSamplerDescs.size(); ++index)
        {
            const RHI::ShaderInputStaticSamplerDescriptor& staticSamplerInput = staticSamplerDescs[index];
            wgpu::BindGroupLayoutEntry& entry = m_wgpuEntries.emplace_back(wgpu::BindGroupLayoutEntry{});
            entry.binding = staticSamplerInput.m_registerId;
            entry.visibility = DefaultShaderStageVisibility;
            if (staticSamplerInput.m_samplerState.m_reductionType == RHI::ReductionType::Comparison)
            {
                entry.sampler.type = wgpu::SamplerBindingType::Comparison;
            }
            // Check if the sampler is doing filtering
            else if (
                !staticSamplerInput.m_samplerState.m_anisotropyEnable && // We use linear filtering when anisotropic is enabled
                staticSamplerInput.m_samplerState.m_filterMag == RHI::FilterMode::Point &&
                staticSamplerInput.m_samplerState.m_filterMin == RHI::FilterMode::Point &&
                staticSamplerInput.m_samplerState.m_filterMip == RHI::FilterMode::Point)
            {
                entry.sampler.type = wgpu::SamplerBindingType::NonFiltering;
            }
            else
            {
                entry.sampler.type = ConvertReductionType(staticSamplerInput.m_samplerState.m_reductionType);
            }
        }

        return RHI::ResultCode::Success;
    }
}
