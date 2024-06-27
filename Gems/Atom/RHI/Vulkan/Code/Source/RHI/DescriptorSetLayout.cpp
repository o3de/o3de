/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/ShaderResourceGroupLayout.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>
#include <AzCore/Utils/TypeHash.h>
#include <AzCore/std/createdestroy.h>
#include <AzCore/std/parallel/lock.h>
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <RHI/DescriptorSetLayout.h>
#include <RHI/Device.h>
#include <Atom/RHI.Reflect/VkAllocator.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace AZ
{
    namespace Vulkan
    {
        const uint32_t DescriptorSetLayout::InvalidLayoutIndex = static_cast<uint32_t>(-1);

        AZ::HashValue64 DescriptorSetLayout::Descriptor::GetHash() const
        {
            return m_shaderResouceGroupLayout->GetHash();
        }

        RHI::Ptr<DescriptorSetLayout> DescriptorSetLayout::Create()
        {
            return aznew DescriptorSetLayout();
        }

        DescriptorSetLayout::~DescriptorSetLayout()
        {
            // do nothing.
        }

        VkDescriptorSetLayout DescriptorSetLayout::GetNativeDescriptorSetLayout() const
        {
            return m_nativeDescriptorSetLayout;
        }

        const AZStd::vector<VkDescriptorSetLayoutBinding>& DescriptorSetLayout::GetNativeLayoutBindings() const
        {
            return m_layoutBindingInfo.GetLayoutBindings();
        }

        const AZStd::vector<VkDescriptorBindingFlags>& DescriptorSetLayout::GetNativeBindingFlags() const
        {
            return m_layoutBindingInfo.GetBindingFlags();
        }

        size_t DescriptorSetLayout::GetDescriptorSetLayoutBindingsCount() const
        {
            return GetNativeBindingFlags().size();
        }

        VkDescriptorType DescriptorSetLayout::GetDescriptorType(size_t index) const
        {
            return GetNativeLayoutBindings()[index].descriptorType;
        }

        uint32_t DescriptorSetLayout::GetDescriptorCount(size_t index) const
        {
            return GetNativeLayoutBindings()[index].descriptorCount;
        }

        uint32_t DescriptorSetLayout::GetConstantDataSize() const
        {
            return m_constantDataSize;
        }

        uint32_t DescriptorSetLayout::GetBindingIndex(uint32_t index) const
        {
            return GetNativeLayoutBindings()[index].binding;
        }

        uint32_t DescriptorSetLayout::GetLayoutIndexFromGroupIndex(uint32_t groupIndex, ResourceType type) const
        {
            switch (type)
            {
            case ResourceType::ConstantData:
                return m_layoutIndexOffset[static_cast<uint32_t>(type)];
            case ResourceType::BufferView:
            case ResourceType::ImageView:
            case ResourceType::BufferViewUnboundedArray:
            case ResourceType::ImageViewUnboundedArray:
            case ResourceType::Sampler:
                return m_layoutIndexOffset[static_cast<uint32_t>(type)] + groupIndex;
            default:
                AZ_Assert(false, "Invalid type %d", static_cast<uint32_t>(type));
                return InvalidLayoutIndex;
            }
        }

        RHI::ResultCode DescriptorSetLayout::Init(const Descriptor& descriptor)
        {
            AZ_Assert(descriptor.m_device, "Device is null.");
            Base::Init(*descriptor.m_device);
            m_shaderResourceGroupLayout = descriptor.m_shaderResouceGroupLayout;

            m_layoutIndexOffset.fill(InvalidLayoutIndex);
            const RHI::ResultCode result = BuildNativeDescriptorSetLayout();
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            // Name the descriptor layout from the SRG layout
            SetName(m_shaderResourceGroupLayout->GetName());
            return result;
        }

        void DescriptorSetLayout::SetNameInternal(const AZStd::string_view& name)
        {
            if (IsInitialized() && !name.empty())
            {
                Debug::SetNameToObject(reinterpret_cast<uint64_t>(m_nativeDescriptorSetLayout), name.data(), VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, static_cast<Device&>(GetDevice()));
            }
        }

        void DescriptorSetLayout::Shutdown()
        {
            if (m_nativeDescriptorSetLayout != VK_NULL_HANDLE)
            {
                auto& device = static_cast<Device&>(GetDevice());
                device.GetContext().DestroyDescriptorSetLayout(
                    device.GetNativeDevice(), m_nativeDescriptorSetLayout, VkSystemAllocator::Get());
                m_nativeDescriptorSetLayout = VK_NULL_HANDLE;
            }
            m_shaderResourceGroupLayout = nullptr;
            Base::Shutdown();
        }

        RHI::ResultCode DescriptorSetLayout::BuildNativeDescriptorSetLayout()
        {
            const RHI::ResultCode buildResult = BuildDescriptorSetLayoutBindings();
            RETURN_RESULT_IF_UNSUCCESSFUL(buildResult);

            VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsCreateInfo{};
            bindingFlagsCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
            bindingFlagsCreateInfo.bindingCount = aznumeric_cast<uint32_t>(GetNativeBindingFlags().size());
            bindingFlagsCreateInfo.pBindingFlags = GetNativeBindingFlags().data();

            VkDescriptorSetLayoutCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            createInfo.pNext = &bindingFlagsCreateInfo;
            createInfo.flags = 0;
            if (IsBindlessSRGLayout())
            {
                // This flag is needed as we are using VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT for descriptors within unbounded arrays
                createInfo.flags |= VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
            }
            createInfo.bindingCount = static_cast<uint32_t>(GetNativeLayoutBindings().size());
            createInfo.pBindings = GetNativeLayoutBindings().size() ? GetNativeLayoutBindings().data() : nullptr;

            auto& device = static_cast<Device&>(GetDevice());
            const VkResult result = device.GetContext().CreateDescriptorSetLayout(
                device.GetNativeDevice(), &createInfo, VkSystemAllocator::Get(), &m_nativeDescriptorSetLayout);

            return ConvertResult(result);
        }

        bool IsUsingDepthFormat(const RHI::ShaderInputImageDescriptor& input)
        {
            // [GFX_TODO] Use Azslc or reflection to properly check if a texture may use a depth format.
            // This is needed when using the NullDescriptorManager and the texture to use as "null" needs a depth format (if not it generates a validation error)
            return AzFramework::StringFunc::Contains(input.m_name.GetStringView(), "Shadow") ||
                AzFramework::StringFunc::Contains(input.m_name.GetStringView(), "Depth");
        }

        RHI::ResultCode DescriptorSetLayout::BuildDescriptorSetLayoutBindings()
        {
            const AZStd::span<const RHI::ShaderInputBufferDescriptor> bufferDescs = m_shaderResourceGroupLayout->GetShaderInputListForBuffers();
            const AZStd::span<const RHI::ShaderInputImageDescriptor> imageDescs = m_shaderResourceGroupLayout->GetShaderInputListForImages();
            const AZStd::span<const RHI::ShaderInputBufferUnboundedArrayDescriptor> bufferUnboundedArrayDescs = m_shaderResourceGroupLayout->GetShaderInputListForBufferUnboundedArrays();
            const AZStd::span<const RHI::ShaderInputImageUnboundedArrayDescriptor> imageUnboundedArrayDescs = m_shaderResourceGroupLayout->GetShaderInputListForImageUnboundedArrays();
            const AZStd::span<const RHI::ShaderInputSamplerDescriptor> samplerDescs = m_shaderResourceGroupLayout->GetShaderInputListForSamplers();
            const AZStd::span<const RHI::ShaderInputStaticSamplerDescriptor>& staticSamplerDescs = m_shaderResourceGroupLayout->GetStaticSamplers();

            // About VK_SHADER_STAGE_ALL...
            // We attempted to configure the descriptor set with the actual resource visibility but it was problematic...
            // Vulkan requires that the visibility flags used to create the VkDescriptorSet (RHI::DeviceShaderResourceGroup) must exactly match the ones
            // used to create the VkPipelineLayout (RHI::DevicePipelineState).
            // See https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VUID-vkCmdBindDescriptorSets-pDescriptorSets-00358
            // But Atom currently expects to be able to use certain ShaderResourceGroup instances with many different pipeline states regardless of visibility.
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
            // It is possible that even with the proposed solution above we could run into other issues not foreseen here. Instead a better path forward would be to
            // see if Khronos can address the API requirement itself. A ticket has been opened with Khronos here: KhronosGroup/Vulkan-Docs#1790. For context the driver
            // error when the visibility bit from PSO does not match the one from Descriptor set is as follows
            // vkDebugMessage: [ERROR][Validation] Validation Error: [ VUID-vkCmdBindDescriptorSets-pDescriptorSets-00358 ] Object 0: handle = 0x59ffe0000000003d, type = VK_OBJECT_TYPE_DESCRIPTOR_SET; | MessageID = 0xe1b89b63 | vkCmdBindDescriptorSets(): descriptorSet #1 being bound is not compatible with overlapping descriptorSetLayout at index 1 of VkPipelineLayout 0x3cc1f30000000840[] due to: Binding 23 for VkDescriptorSetLayout 0xbd2b7d000000083e[] from pipeline layout has stageFlags VK_SHADER_STAGE_VERTEX_BIT but binding 23 for VkDescriptorSetLayout 0x4fac1c0000000032[], which is bound, has stageFlags Unhandled VkShaderStageFlagBits. The Vulkan spec states: Each element of pDescriptorSets must have been allocated with a VkDescriptorSetLayout that matches (is the same as, or identically defined as) the VkDescriptorSetLayout at set n in layout, where n is the sum of firstSet and the index into pDescriptorSets (https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#VUID-vkCmdBindDescriptorSets-pDescriptorSets-00358)
            static const VkShaderStageFlags DefaultShaderStageVisibility = VK_SHADER_STAGE_ALL;

            // The + 1 is for Constant Data.
            m_layoutBindingInfo = LayoutBindingInfo(
                1 +
                bufferDescs.size() +
                imageDescs.size() +
                bufferUnboundedArrayDescs.size() +
                imageUnboundedArrayDescs.size() +
                samplerDescs.size() +
                staticSamplerDescs.size());

            m_constantDataSize = m_shaderResourceGroupLayout->GetConstantDataSize();
            if (m_constantDataSize)
            {
                AZStd::span<const RHI::ShaderInputConstantDescriptor> inputListForConstants = m_shaderResourceGroupLayout->GetShaderInputListForConstants();
                AZ_Assert(!inputListForConstants.empty(), "Empty constant input list");
                auto [vbinding, _1, _2] = m_layoutBindingInfo.Add();

                // All constant data of the SRG have the same binding.
                vbinding.binding = inputListForConstants[0].m_registerId;
                vbinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                vbinding.descriptorCount = 1;
                vbinding.stageFlags = DefaultShaderStageVisibility;
                vbinding.pImmutableSamplers = nullptr;
                m_layoutIndexOffset[static_cast<uint32_t>(ResourceType::ConstantData)] = 0;
            }

            // buffers
            m_layoutIndexOffset[static_cast<uint32_t>(ResourceType::BufferView)] =
                bufferDescs.empty() ? InvalidLayoutIndex : static_cast<uint32_t>(m_layoutBindingInfo.GetSize());

            for (uint32_t index = 0; index < bufferDescs.size(); ++index)
            {
                const RHI::ShaderInputBufferDescriptor& desc = bufferDescs[index];
                auto [vbinding, _1, _2] = m_layoutBindingInfo.Add();
                vbinding.binding = desc.m_registerId;
                switch (desc.m_access)
                {
                case RHI::ShaderInputBufferAccess::Constant:
                    vbinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    if (!ValidateUniformBufferDeviceLimits(desc))
                    {
                        return RHI::ResultCode::OutOfMemory;
                    }

                    break;
                case RHI::ShaderInputBufferAccess::Read:
                    switch (desc.m_type)
                    {
                    case RHI::ShaderInputBufferType::Typed:
                        vbinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
                        break;
                    case RHI::ShaderInputBufferType::AccelerationStructure:
                        vbinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
                        break;
                    default:
                        vbinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                        break;
                    }
                    break;
                case RHI::ShaderInputBufferAccess::ReadWrite:
                    vbinding.descriptorType = desc.m_type == RHI::ShaderInputBufferType::Typed ? 
                        VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    break;
                default:
                    AZ_Assert(false, "ShaderInputBufferAccess is illegal.");
                    return RHI::ResultCode::InvalidArgument;
                }
                vbinding.descriptorCount = desc.m_count;
                vbinding.stageFlags = DefaultShaderStageVisibility;
                vbinding.pImmutableSamplers = nullptr;
            }

            // images
            m_layoutIndexOffset[static_cast<uint32_t>(ResourceType::ImageView)] =
                imageDescs.empty() ? InvalidLayoutIndex : static_cast<uint32_t>(m_layoutBindingInfo.GetSize());

            for (uint32_t index = 0; index < imageDescs.size(); ++index)
            {
                const RHI::ShaderInputImageDescriptor& desc = imageDescs[index];
                auto [vbinding, _, usesDepthFormat] = m_layoutBindingInfo.Add();
                usesDepthFormat = IsUsingDepthFormat(desc);
                vbinding.binding = desc.m_registerId;
                if (desc.m_type == RHI::ShaderInputImageType::SubpassInput)
                {
                    vbinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
                    vbinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                }
                else
                {
                    switch (desc.m_access)
                    {
                    case RHI::ShaderInputImageAccess::Read:
                        vbinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                        break;
                    case RHI::ShaderInputImageAccess::ReadWrite:
                        vbinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                        break;
                    default:
                        AZ_Assert(false, "ShaderInputImageAccess is illegal.");
                        return RHI::ResultCode::InvalidArgument;
                    }
                    vbinding.stageFlags = DefaultShaderStageVisibility;
                }
               
                vbinding.descriptorCount = desc.m_count;
                vbinding.pImmutableSamplers = nullptr;
            }

            // samplers
            m_layoutIndexOffset[static_cast<uint32_t>(ResourceType::Sampler)] =
                samplerDescs.empty() ? InvalidLayoutIndex : static_cast<uint32_t>(m_layoutBindingInfo.GetSize());

            for (uint32_t index = 0; index < samplerDescs.size(); ++index)
            {
                const RHI::ShaderInputSamplerDescriptor& desc = samplerDescs[index];
                auto [vbinding, _1, _2] = m_layoutBindingInfo.Add();
                vbinding.binding = desc.m_registerId;
                vbinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
                vbinding.descriptorCount = desc.m_count;
                vbinding.stageFlags = DefaultShaderStageVisibility;
                vbinding.pImmutableSamplers = nullptr;
            }

            if (!staticSamplerDescs.empty())
            {
                auto& device = static_cast<Device&>(GetDevice());
                m_nativeSamplers.resize(staticSamplerDescs.size(), VK_NULL_HANDLE);
                for (int index = 0; index < staticSamplerDescs.size(); ++index)
                {
                    const RHI::ShaderInputStaticSamplerDescriptor& staticSamplerInput = staticSamplerDescs[index];
                    Sampler::Descriptor samplerDesc;
                    samplerDesc.m_device = &device;
                    samplerDesc.m_samplerState = staticSamplerInput.m_samplerState;
                    m_nativeSamplers[index] = device.AcquireSampler(samplerDesc)->GetNativeSampler();

                    auto [vbinding, _1, _2] = m_layoutBindingInfo.Add();
                    vbinding.binding = staticSamplerInput.m_registerId;
                    vbinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
                    vbinding.descriptorCount = 1;
                    vbinding.stageFlags = DefaultShaderStageVisibility;
                    vbinding.pImmutableSamplers = &m_nativeSamplers[index];
                }
            }

            // buffer unbounded arrays
            m_layoutIndexOffset[static_cast<uint32_t>(ResourceType::BufferViewUnboundedArray)] =
                bufferUnboundedArrayDescs.empty() ? InvalidLayoutIndex : static_cast<uint32_t>(m_layoutBindingInfo.GetSize());

            for (const RHI::ShaderInputBufferUnboundedArrayDescriptor& desc : bufferUnboundedArrayDescs)
            {
                auto [vbinding, bindingFlags, _] = m_layoutBindingInfo.Add();
                VkDescriptorBindingFlags descriptorBindingFlag = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
                if (IsBindlessSRGLayout())
                {
                    descriptorBindingFlag |= VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
                }
                else
                {
                    descriptorBindingFlag |= VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;
                }
                bindingFlags = descriptorBindingFlag;

                vbinding.binding = desc.m_registerId;
                switch (desc.m_access)
                {
                case RHI::ShaderInputBufferAccess::Read:
                    switch (desc.m_type)
                    {
                    case RHI::ShaderInputBufferType::Typed:
                        vbinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
                        break;
                    case RHI::ShaderInputBufferType::AccelerationStructure:
                        vbinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
                        break;
                    default:
                        vbinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                        break;
                    }
                    break;
                case RHI::ShaderInputBufferAccess::ReadWrite:
                    vbinding.descriptorType = desc.m_type == RHI::ShaderInputBufferType::Typed ?
                        VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    break;
                default:
                    AZ_Assert(false, "ShaderInputBufferAccess is illegal.");
                    return RHI::ResultCode::InvalidArgument;
                }
                vbinding.descriptorCount = MaxUnboundedArrayDescriptors;
                vbinding.stageFlags = DefaultShaderStageVisibility;
                vbinding.pImmutableSamplers = nullptr;

                m_hasUnboundedArray = true;
            }

            // image unbounded arrays
            m_layoutIndexOffset[static_cast<uint32_t>(ResourceType::ImageViewUnboundedArray)] =
                imageUnboundedArrayDescs.empty() ? InvalidLayoutIndex : static_cast<uint32_t>(m_layoutBindingInfo.GetSize());

            for (const RHI::ShaderInputImageUnboundedArrayDescriptor& desc : imageUnboundedArrayDescs)
            {

                auto [vbinding, bindingFlags, _] = m_layoutBindingInfo.Add();
                VkDescriptorBindingFlags descriptorBindingFlag = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
                if (IsBindlessSRGLayout())
                {
                    descriptorBindingFlag |= VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
                }
                else
                {
                    descriptorBindingFlag |= VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;
                }
                bindingFlags = descriptorBindingFlag;

                vbinding.binding = desc.m_registerId;
                switch (desc.m_access)
                {
                case RHI::ShaderInputImageAccess::Read:
                    vbinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                    break;
                case RHI::ShaderInputImageAccess::ReadWrite:
                    vbinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                    break;
                default:
                    AZ_Assert(false, "ShaderInputImageAccess is illegal.");
                    return RHI::ResultCode::InvalidArgument;
                }
                vbinding.stageFlags = DefaultShaderStageVisibility;
                vbinding.descriptorCount = MaxUnboundedArrayDescriptors;
                vbinding.pImmutableSamplers = nullptr;

                m_hasUnboundedArray = true;
            }

            return RHI::ResultCode::Success;
        }

        bool DescriptorSetLayout::ValidateUniformBufferDeviceLimits([[maybe_unused]] const RHI::ShaderInputBufferDescriptor& desc)
        {
#if defined (AZ_RHI_ENABLE_VALIDATION)       
            const auto& physicalDevice = static_cast<const PhysicalDevice&>(GetDevice().GetPhysicalDevice());
            const VkPhysicalDeviceLimits deviceLimits = physicalDevice.GetDeviceLimits();
            if (desc.m_count > deviceLimits.maxPerStageDescriptorUniformBuffers)
            {
                AZ_Assert(false, "Maximum number of uniform buffer exceeded(%d), needed %d", deviceLimits.maxPerStageDescriptorUniformBuffers, desc.m_count);
                return false;
            }       
#endif // AZ_RHI_ENABLE_VALIDATION

            return true;
        }

        const RHI::ShaderResourceGroupLayout* DescriptorSetLayout::GetShaderResourceGroupLayout() const
        {
            return m_shaderResourceGroupLayout.get();
        }

        bool DescriptorSetLayout::UsesDepthFormat(uint32_t layoutIndex) const
        {
            return m_layoutBindingInfo.GetUseDepthFormat()[layoutIndex];
        }

        bool DescriptorSetLayout::IsBindlessSRGLayout()
        {
            auto& device = static_cast<Device&>(GetDevice());
            return m_shaderResourceGroupLayout->GetBindingSlot() == device.GetBindlessDescriptorPool().GetBindlessSrgBindingSlot();
        }

        DescriptorSetLayout::LayoutBindingInfo::LayoutBindingInfo(size_t size)
        {
            m_descriptorSetLayoutBindings.reserve(size);
            m_descriptorBindingFlags.reserve(size);
            m_useDepthFormat.reserve(size);
        }

        DescriptorSetLayout::LayoutBindingInfo::Element DescriptorSetLayout::LayoutBindingInfo::Add()
        {
            m_descriptorSetLayoutBindings.emplace_back(VkDescriptorSetLayoutBinding{});
            m_descriptorBindingFlags.emplace_back(VkDescriptorBindingFlags{});
            m_useDepthFormat.emplace_back(false);

            return LayoutBindingInfo::Element(
                m_descriptorSetLayoutBindings.back(), m_descriptorBindingFlags.back(), m_useDepthFormat.back());
        }

        AZStd::size_t DescriptorSetLayout::LayoutBindingInfo::GetSize() const
        {
            return m_descriptorSetLayoutBindings.size();
        }

        const AZStd::vector<VkDescriptorSetLayoutBinding>& DescriptorSetLayout::LayoutBindingInfo::GetLayoutBindings() const
        {
            return m_descriptorSetLayoutBindings;
        }

        const AZStd::vector<VkDescriptorBindingFlags>& DescriptorSetLayout::LayoutBindingInfo::GetBindingFlags() const
        {
            return m_descriptorBindingFlags;
        }

        const AZStd::vector<bool>& DescriptorSetLayout::LayoutBindingInfo::GetUseDepthFormat() const
        {
            return m_useDepthFormat;
        }
    } // namespace Vulkan
}
