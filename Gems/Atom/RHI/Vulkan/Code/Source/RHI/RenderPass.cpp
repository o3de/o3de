/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/Conversion.h>
#include <RHI/Device.h>
#include <RHI/Framebuffer.h>
#include <RHI/RenderPass.h>
#include <AzCore/std/hash.h>
#include <memory>
#include <Atom/RHI.Reflect/VkAllocator.h>

namespace AZ
{
    namespace Vulkan
    {
        namespace Internal
        {
            enum class AttachmentType : uint32_t
            {
                Color, // Color render target attachment
                DepthStencil, // Depth stencil attachment
                Resolve, // Resolve attachment
                InputAttachment, // An input attachment that is the output of a previous subpass.
                Preserve, // An attachment that is not being used by the subpass but that it will be preserved.
                ShadingRate, // An attachment that is used for specifiying shading rate.
                Count
            };
            static const uint32_t AttachmentTypeCount = static_cast<uint32_t>(AttachmentType::Count);

            using RenderPassResult = AZStd::tuple<VkResult, VkRenderPass>;

            // Helper class to build the Vulkan native renderpass.
            // If available at runtime, we prefer to use the Renderpass2 extension (need it for other features like shading rate attachments).
            // This extension has different structures for declaring the attachments, subpasses, dependencies, etc. Although the logic is almost the same
            // as the standard declaration of a renderpass, the extension has members that do not exist in the
            // standard way. Because of this, a template approach is used to only use those members is they exist
            // in the structure.
            template<
                typename VkRenderPassCreateInfoTraits,
                typename VkAttachmentDescriptionTraits,
                typename VkAttachmentReferenceTraits,
                typename VkSubpassDescriptionTraits,
                typename VkSubpassDependencyTraits>
            struct VkRenderpassBuilder
            {
            public:

                VkRenderpassBuilder(const Device& device)
                    : m_device(device)
                {}

                // The structure for declaring the renderpass
                using VkRenderPassCreateInfoType = typename VkRenderPassCreateInfoTraits::value_type;
                // The structyure used for declaring attachments
                using VkAttachmentDescriptionType = typename VkAttachmentDescriptionTraits::value_type;
                // The structure used for referencing an attachment
                using VkAttachmentReferenceType = typename VkAttachmentReferenceTraits::value_type;
                // The structure used for describing a subpass
                using VkSubpassDescriptionType = typename VkSubpassDescriptionTraits::value_type;
                // The structure used for declaring a dependency
                using VkSubpassDependencyType = typename VkSubpassDependencyTraits::value_type;

                // Holds the information about a subpass
                struct SubpassInfo
                {
                    AZStd::array<AZStd::vector<VkAttachmentReferenceType>, AttachmentTypeCount> m_attachmentReferences;
                    AZStd::vector<uint32_t> m_preserveAttachments;
                    // Used only if the pass uses fragment rate attachment.
                    VkFragmentShadingRateAttachmentInfoKHR m_shadingRateAttachmentExtension;
                };

                RenderPassResult Build(const RenderPass::Descriptor& descriptor)
                {
                    m_descriptor = &descriptor;
                    AZStd::vector<VkAttachmentDescriptionType> attachmentDescriptions;
                    AZStd::vector<SubpassInfo> subpassInfo;
                    AZStd::vector<VkSubpassDescriptionType> subpassDescriptions;
                    AZStd::vector<VkSubpassDependencyType> subpassDependencies;

                    BuildAttachmentDescriptions(attachmentDescriptions);
                    BuildSubpassAttachmentReferences(subpassInfo);
                    BuildSubpassDescriptions(subpassInfo, subpassDescriptions);
                    BuildSubpassDependencies(subpassDependencies);

                    VkRenderPassCreateInfoType createInfo{};
                    createInfo.sType = VkRenderPassCreateInfoTraits::struct_type;
                    createInfo.pNext = nullptr;
                    createInfo.flags = 0;
                    createInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
                    createInfo.pAttachments = attachmentDescriptions.empty() ? nullptr : attachmentDescriptions.data();
                    createInfo.subpassCount = static_cast<uint32_t>(subpassDescriptions.size());
                    createInfo.pSubpasses = subpassDescriptions.empty() ? nullptr : subpassDescriptions.data();
                    createInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
                    createInfo.pDependencies = subpassDependencies.empty() ? nullptr : subpassDependencies.data();

                    // Fragment shade attachments are declared at a renderpass level (same for all subpasses), so we need to
                    // check if we have one as part of the renderpass declaration. We check if the first subpass contains the shading rate attachment,
                    // and use that one for the whole renderpass. If more than one is found, we raise an assert because
                    // there can only be one fragment attachment per renderpass.
                    VkRenderPassFragmentDensityMapCreateInfoEXT fdmAttachmentCreateInfo;
                    if (m_device.GetImageShadingRateMode() == Device::ShadingRateImageMode::DensityMap)
                    {
                        AZ_Assert(!subpassInfo.empty(), "Subpass info is empty");
                        auto& shadingRateAttachmentRefList = subpassInfo.front()
                            .m_attachmentReferences[static_cast<uint32_t>(AttachmentType::ShadingRate)];

                        if (!shadingRateAttachmentRefList.empty())
                        {
                            AZ_Assert(shadingRateAttachmentRefList.size() == 1, "There's more than one shading rate image");
                            const auto& fragmentDensityReference = shadingRateAttachmentRefList.front();
                            fdmAttachmentCreateInfo = {};
                            fdmAttachmentCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_FRAGMENT_DENSITY_MAP_CREATE_INFO_EXT;
                            fdmAttachmentCreateInfo.fragmentDensityMapAttachment.attachment = fragmentDensityReference.attachment;
                            fdmAttachmentCreateInfo.fragmentDensityMapAttachment.layout = fragmentDensityReference.layout;

                            createInfo.pNext = &fdmAttachmentCreateInfo;
                        }
                    }

                    return Create<VkRenderPassCreateInfoType>(createInfo);
                }

            private:

                // Classes used for handling setting a member that may not exist.
                struct general_ {};
                struct special_ : general_{};
                template<typename> struct int_ { typedef int type; };

                template<typename vkCreateRenderPassType>
                RenderPassResult Create(VkRenderPassCreateInfoType& createInfo)
                {
                    if constexpr (AZStd::is_same_v<vkCreateRenderPassType, VkRenderPassCreateInfo>)
                    {
                        VkRenderPass renderPass = VK_NULL_HANDLE;
                        VkResult result = m_device.GetContext().CreateRenderPass(
                            m_device.GetNativeDevice(), &createInfo, VkSystemAllocator::Get(), &renderPass);
                        return AZStd::make_tuple(result, renderPass);
                    }
                    else if constexpr (AZStd::is_same_v<vkCreateRenderPassType, VkRenderPassCreateInfo2>)
                    {
                        VkRenderPass renderPass = VK_NULL_HANDLE;
                        VkResult result = m_device.GetContext().CreateRenderPass2KHR(
                            m_device.GetNativeDevice(), &createInfo, VkSystemAllocator::Get(), &renderPass);
                        return AZStd::make_tuple(result, renderPass);
                    }
                    else
                    {
                        static_assert(AZStd::is_same_v<vkCreateRenderPassType, VkRenderPassCreateInfo>
                            || AZStd::is_same_v<vkCreateRenderPassType, VkRenderPassCreateInfo2>);
                        return {};
                    }
                }

                // Builds all attachment descriptions from the descriptor.
                void BuildAttachmentDescriptions(AZStd::vector<VkAttachmentDescriptionType>& attachmentDescriptions) const
                {
                    for (uint32_t i = 0; i < m_descriptor->m_attachmentCount; ++i)
                    {
                        const RenderPass::AttachmentBinding& binding = m_descriptor->m_attachments[i];
                        attachmentDescriptions.emplace_back(VkAttachmentDescriptionType{});
                        VkAttachmentDescriptionType& desc = attachmentDescriptions.back();
                        // Set the type if the structure has one.
                        SetStructureType(desc, VkAttachmentDescriptionTraits::struct_type, special_());
                        desc.format = ConvertFormat(binding.m_format);
                        desc.samples = ConvertSampleCount(binding.m_multisampleState.m_samples);
                        desc.loadOp = ConvertAttachmentLoadAction(binding.m_loadStoreAction.m_loadAction);
                        desc.storeOp = ConvertAttachmentStoreAction(binding.m_loadStoreAction.m_storeAction);
                        desc.stencilLoadOp = ConvertAttachmentLoadAction(binding.m_loadStoreAction.m_loadActionStencil);
                        desc.stencilStoreOp = ConvertAttachmentStoreAction(binding.m_loadStoreAction.m_storeActionStencil);
                        desc.initialLayout = binding.m_initialLayout;
                        desc.finalLayout = binding.m_finalLayout;
                    }
                }

                // Builds the attachments references for each subpass.
                void BuildSubpassAttachmentReferences(AZStd::vector<SubpassInfo>& subpassInfo) const
                {
                    subpassInfo.resize(m_descriptor->m_subpassCount);
                    for (uint32_t i = 0; i < m_descriptor->m_subpassCount; ++i)
                    {
                        BuildAttachmentReferences<AttachmentType::Color>(i, subpassInfo[i]);
                        BuildAttachmentReferences<AttachmentType::DepthStencil>(i, subpassInfo[i]);
                        BuildAttachmentReferences<AttachmentType::InputAttachment>(i, subpassInfo[i]);
                        BuildAttachmentReferences<AttachmentType::Resolve>(i, subpassInfo[i]);
                        BuildAttachmentReferences<AttachmentType::Preserve>(i, subpassInfo[i]);
                        BuildAttachmentReferences<AttachmentType::ShadingRate>(i, subpassInfo[i]);
                    }
                }

                // Builds the attachment references for a specific subpass.
                template<AttachmentType type>
                void BuildAttachmentReferences(uint32_t subpassIndex, SubpassInfo& subpassInfo) const
                {
                    // A template cannot be specialized inside of another template
                    // therefore the `if constexpr` statement is used to filter the Preserve AttachmentType functionality
                    // in this single template member function
                    if constexpr (type == AttachmentType::Preserve)
                    {
                        auto& subpassDescriptor = const_cast<RenderPass::SubpassDescriptor&>(m_descriptor->m_subpassDescriptors[subpassIndex]);
                        AZStd::vector<uint32_t>& attachmentReferenceList = subpassInfo.m_preserveAttachments;
                        attachmentReferenceList.insert(
                            attachmentReferenceList.end(),
                            subpassDescriptor.m_preserveAttachments.begin(),
                            subpassDescriptor.m_preserveAttachments.begin() + subpassDescriptor.m_preserveAttachmentCount);
                    }
                    else
                    {
                        AZStd::span<const RenderPass::SubpassAttachment> subpassAttachmentList = GetSubpassAttachments(subpassIndex, type);
                        AZStd::vector<VkAttachmentReferenceType>& attachmentReferenceList =
                            subpassInfo.m_attachmentReferences[static_cast<uint32_t>(type)];
                        attachmentReferenceList.resize(subpassAttachmentList.size());
                        for (uint32_t index = 0; index < subpassAttachmentList.size(); ++index)
                        {
                            if (subpassAttachmentList[index].IsValid())
                            {
                                attachmentReferenceList[index].attachment = subpassAttachmentList[index].m_attachmentIndex;
                                attachmentReferenceList[index].layout = subpassAttachmentList[index].m_layout;
                                SetImageAspectFlags(
                                    attachmentReferenceList[index], subpassAttachmentList[index].m_imageAspectFlags, special_());
                            }
                            else
                            {
                                attachmentReferenceList[index].attachment = VK_ATTACHMENT_UNUSED;
                            }
                            SetStructureType(attachmentReferenceList[index], VkAttachmentReferenceTraits::struct_type, special_());
                        }
                    }
                }

                // Return the list of attachment depending on the type.
                AZStd::span<const RenderPass::SubpassAttachment> GetSubpassAttachments(const uint32_t subpassIndex, const AttachmentType type) const
                {
                    const RenderPass::SubpassDescriptor& descriptor = m_descriptor->m_subpassDescriptors[subpassIndex];
                    switch (type)
                    {
                    case AttachmentType::Color:
                        return AZStd::span<const RenderPass::SubpassAttachment>(descriptor.m_rendertargetAttachments.begin(), descriptor.m_rendertargetCount);
                    case AttachmentType::DepthStencil:
                        return descriptor.m_depthStencilAttachment.IsValid() ? AZStd::span<const RenderPass::SubpassAttachment>(&descriptor.m_depthStencilAttachment, 1)
                            : AZStd::span<const RenderPass::SubpassAttachment>();
                    case AttachmentType::InputAttachment:
                        return AZStd::span<const RenderPass::SubpassAttachment>(descriptor.m_subpassInputAttachments.begin(), descriptor.m_subpassInputCount);
                    case AttachmentType::Resolve:
                        return AZStd::span<const RenderPass::SubpassAttachment>(descriptor.m_resolveAttachments.begin(), descriptor.m_rendertargetCount);
                    case AttachmentType::ShadingRate:
                        return descriptor.m_fragmentShadingRateAttachment.IsValid()
                            ? AZStd::span<const RenderPass::SubpassAttachment>(&descriptor.m_fragmentShadingRateAttachment, 1)
                            : AZStd::span<const RenderPass::SubpassAttachment>();
                    default:
                        AZ_Assert(false, "Invalid attachment type %d", type);
                        return {};
                    }
                }

                // Builds the subpass descriptions using the previously built attachment references.
                void BuildSubpassDescriptions(
                    AZStd::vector<SubpassInfo>& subpassInfo,
                    AZStd::vector<VkSubpassDescriptionType>& subpassDescriptions) const
                {
                    subpassDescriptions.resize(m_descriptor->m_subpassCount);
                    for (uint32_t i = 0; i < m_descriptor->m_subpassCount; ++i)
                    {
                        const auto& attachmentReferencesPerType = subpassInfo[i].m_attachmentReferences;
                        auto& inputAttachmentRefList = attachmentReferencesPerType[static_cast<uint32_t>(AttachmentType::InputAttachment)];
                        auto& colorAttachmentRefList = attachmentReferencesPerType[static_cast<uint32_t>(AttachmentType::Color)];
                        auto& depthAttachmentRefList = attachmentReferencesPerType[static_cast<uint32_t>(AttachmentType::DepthStencil)];
                        auto& resolveAttachmentRefList = attachmentReferencesPerType[static_cast<uint32_t>(AttachmentType::Resolve)];
                        auto& shadingRateAttachmentRefList = attachmentReferencesPerType[static_cast<uint32_t>(AttachmentType::ShadingRate)];
                        auto& preserveAttachmentList = subpassInfo[i].m_preserveAttachments;

                        VkSubpassDescriptionType& desc = subpassDescriptions[i];
                        desc = {};
                        SetStructureType(desc, VkSubpassDescriptionTraits::struct_type, special_());
                        desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                        desc.inputAttachmentCount = static_cast<uint32_t>(inputAttachmentRefList.size());
                        desc.pInputAttachments = inputAttachmentRefList.empty() ? nullptr : inputAttachmentRefList.data();
                        desc.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentRefList.size());
                        desc.pColorAttachments = colorAttachmentRefList.empty() ? nullptr : colorAttachmentRefList.data();
                        desc.pResolveAttachments = resolveAttachmentRefList.empty() ? nullptr : resolveAttachmentRefList.data();
                        desc.pDepthStencilAttachment = depthAttachmentRefList.empty() ? nullptr : depthAttachmentRefList.data();
                        desc.preserveAttachmentCount = static_cast<uint32_t>(preserveAttachmentList.size());
                        desc.pPreserveAttachments = preserveAttachmentList.empty() ? nullptr : preserveAttachmentList.data();

                        // Shading rate attachments are declared at subpass level.
                        // Check if the subpass has a shading rate attachemnt and st the proper information.
                        if (!shadingRateAttachmentRefList.empty() &&
                            m_device.GetImageShadingRateMode() == Device::ShadingRateImageMode::ImageAttachment)
                        {
                            SetFragmentShadingRateAttachmentInfo<VkAttachmentReferenceType>(
                                subpassInfo[i], shadingRateAttachmentRefList.data());
                            SetNext(desc, &subpassInfo[i].m_shadingRateAttachmentExtension, special_());
                        }
                    }
                }

                template<typename AttachmentType>
                void SetFragmentShadingRateAttachmentInfo(
                    [[maybe_unused]] SubpassInfo& subpassInfo, [[maybe_unused]] const VkAttachmentReferenceType* reference) const
                {
                    if constexpr (AZStd::is_same_v<AttachmentType, VkAttachmentReference2>)
                    {
                        auto& tileSize = m_device.GetLimits().m_shadingRateTileSize;
                        subpassInfo.m_shadingRateAttachmentExtension.sType = VK_STRUCTURE_TYPE_FRAGMENT_SHADING_RATE_ATTACHMENT_INFO_KHR;
                        subpassInfo.m_shadingRateAttachmentExtension.pFragmentShadingRateAttachment = reference;
                        subpassInfo.m_shadingRateAttachmentExtension.shadingRateAttachmentTexelSize =
                            VkExtent2D{ tileSize.m_width, tileSize.m_height };
                    }
                }

                // Builds the dependencies between the subpasses.
                void BuildSubpassDependencies(AZStd::vector<VkSubpassDependencyType>& subpassDependencies) const
                {
                    VkPipelineStageFlags supportedStages = GetSupportedPipelineStages(RHI::PipelineStateType::Draw);

                    subpassDependencies.resize(m_descriptor->m_subpassDependencies.size());
                    for (uint32_t i = 0; i < m_descriptor->m_subpassDependencies.size(); ++i)
                    {
                        const VkSubpassDependency& subpassDependency = m_descriptor->m_subpassDependencies[i];
                        auto& dependency = subpassDependencies[i];
                        dependency = {};
                        SetStructureType(dependency, VkSubpassDependencyTraits::struct_type, special_());
                        dependency.srcSubpass = subpassDependency.srcSubpass;
                        dependency.dstSubpass = subpassDependency.dstSubpass;
                        dependency.srcStageMask = RHI::FilterBits(
                            subpassDependency.srcStageMask, subpassDependency.srcSubpass == VK_SUBPASS_EXTERNAL ? ~0 : supportedStages);
                        dependency.dstStageMask = RHI::FilterBits(
                            subpassDependency.dstStageMask, subpassDependency.dstSubpass == VK_SUBPASS_EXTERNAL ? ~0 : supportedStages);
                        dependency.srcAccessMask =
                            RHI::FilterBits(subpassDependency.srcAccessMask, GetSupportedAccessFlags(subpassDependency.srcStageMask));
                        dependency.dstAccessMask =
                            RHI::FilterBits(subpassDependency.dstAccessMask, GetSupportedAccessFlags(subpassDependency.dstStageMask));
                        dependency.dependencyFlags = subpassDependency.dependencyFlags;
                    }
                }

                template<typename StructureType, typename int_<decltype(StructureType::aspectMask)>::type = 0>
                void SetImageAspectFlags(StructureType& structure, VkImageAspectFlags aspectMask, special_) const
                {
                    structure.aspectMask = aspectMask;
                }

                template<typename StructureType>
                void SetImageAspectFlags(
                    [[maybe_unused]] StructureType& structure, [[maybe_unused]] VkImageAspectFlags aspectMask, general_) const
                {}

                template<typename StructureType, typename int_<decltype(StructureType::sType)>::type = 0>
                void SetStructureType(StructureType& structure, VkStructureType type, special_) const
                {
                    structure.sType = type;
                }

                template<typename StructureType>
                void SetStructureType(
                    [[maybe_unused]] StructureType& structure, [[maybe_unused]] VkStructureType type, general_) const
                {}

                template<typename StructureType, typename int_<decltype(StructureType::pNext)>::type = 0>
                void SetNext(StructureType& structure, const void* next, special_) const
                {
                    structure.pNext = next;
                }

                template<typename StructureType>
                void SetNext([[maybe_unused]] StructureType& structure, [[maybe_unused]] const void* next, general_) const
                {
                }

                const Device& m_device;
                const RenderPass::Descriptor* m_descriptor = nullptr;
            };

            template<typename T, VkStructureType type = AZStd::numeric_limits<VkStructureType>::max()>
            struct StructureTypeTraits
            {
                static const VkStructureType struct_type = type;
                typedef T value_type;
            };

            // Builder used for standard renderpass creation.
            using NativeRenderpassBuilder = VkRenderpassBuilder<
                StructureTypeTraits<VkRenderPassCreateInfo, VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO>,
                StructureTypeTraits<VkAttachmentDescription>,
                StructureTypeTraits<VkAttachmentReference>,
                StructureTypeTraits<VkSubpassDescription>,
                StructureTypeTraits<VkSubpassDependency>>;

            // Builder used for creating a renderpass using the Renderpass2 extension.
            using NativeRenderpass2Builder = VkRenderpassBuilder<
                StructureTypeTraits<VkRenderPassCreateInfo2, VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2>,
                StructureTypeTraits<VkAttachmentDescription2, VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2>,
                StructureTypeTraits<VkAttachmentReference2, VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2>,
                StructureTypeTraits<VkSubpassDescription2, VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2>,
                StructureTypeTraits<VkSubpassDependency2, VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2>>;
        }

        RenderPass::AttachmentLoadStoreAction::AttachmentLoadStoreAction(const RHI::AttachmentLoadStoreAction& loadStoreAction)
        {
            *this = loadStoreAction;
        }

        RenderPass::AttachmentLoadStoreAction RenderPass::AttachmentLoadStoreAction::operator=(const RHI::AttachmentLoadStoreAction& loadStoreAction)
        {
            m_loadAction = loadStoreAction.m_loadAction;
            m_storeAction = loadStoreAction.m_storeAction;
            m_loadActionStencil = loadStoreAction.m_loadActionStencil;
            m_storeActionStencil = loadStoreAction.m_storeActionStencil;
            return *this;
        }

        size_t RenderPass::Descriptor::GetHash() const
        {
            size_t hash = 0;
            size_t attachmentsHash = AZStd::hash_range(m_attachments.begin(), m_attachments.begin() + m_attachmentCount);
            size_t subpassHash = AZStd::hash_range(m_subpassDescriptors.begin(), m_subpassDescriptors.begin() + m_subpassCount);
            size_t subpassDependenciesHash = AZStd::hash_range(m_subpassDependencies.begin(), m_subpassDependencies.end());
            AZStd::hash_combine(hash, m_attachmentCount);
            AZStd::hash_combine(hash, m_subpassCount);
            AZStd::hash_combine(hash, attachmentsHash);
            AZStd::hash_combine(hash, subpassHash);
            AZStd::hash_combine(hash, subpassDependenciesHash);
            return hash;
        }

        RHI::Ptr<RenderPass> RenderPass::Create()
        {
            return aznew RenderPass();
        }

        RHI::ResultCode RenderPass::Init(const Descriptor& descriptor)
        {
            m_descriptor = descriptor;
            AZ_Assert(descriptor.m_device, "Device is null.");
            DeviceObject::Init(*m_descriptor.m_device);
            const PhysicalDevice& physicalDevice = static_cast<const PhysicalDevice&>(GetDevice().GetPhysicalDevice());

            RHI::ResultCode result;
            // Check if we can use the renderpass2 extension for building the renderpass.
            if (physicalDevice.IsOptionalDeviceExtensionSupported(OptionalDeviceExtension::Renderpass2))
            {
                result = BuildNativeRenderPass<Internal::NativeRenderpass2Builder>();
            }
            else
            {
                result = BuildNativeRenderPass<Internal::NativeRenderpassBuilder>();
            }
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            SetName(GetName());
            return result;
        }

        VkRenderPass RenderPass::GetNativeRenderPass() const
        {
            return m_nativeRenderPass;
        }

        const RenderPass::Descriptor& RenderPass::GetDescriptor() const
        {
            return m_descriptor;
        }

        uint32_t RenderPass::GetAttachmentCount() const
        {
            return m_descriptor.m_attachmentCount;
        }

        RenderPass::Descriptor RenderPass::ConvertRenderAttachmentLayout(
            Device& device,
            const RHI::RenderAttachmentLayout& layout,
            const RHI::MultisampleState& multisampleState)
        {
            Descriptor renderPassDesc;
            renderPassDesc.m_device = &device;
            renderPassDesc.m_attachmentCount = layout.m_attachmentCount;

            for (uint32_t index = 0; index < renderPassDesc.m_attachmentCount; ++index)
            {
                // Only fill up the necessary information to get a compatible render pass
                renderPassDesc.m_attachments[index].m_format = layout.m_attachmentFormats[index];
                renderPassDesc.m_attachments[index].m_initialLayout = VK_IMAGE_LAYOUT_GENERAL;
                renderPassDesc.m_attachments[index].m_finalLayout = VK_IMAGE_LAYOUT_GENERAL;
                renderPassDesc.m_attachments[index].m_multisampleState = multisampleState;
            }

            renderPassDesc.m_subpassCount = layout.m_subpassCount;
            for (uint32_t subpassIndex = 0; subpassIndex < layout.m_subpassCount; ++subpassIndex)
            {
                AZStd::bitset<RHI::Limits::Pipeline::RenderAttachmentCountMax> usedAttachments;
                const auto& subpassLayout = layout.m_subpassLayouts[subpassIndex];
                auto& subpassDescriptor = renderPassDesc.m_subpassDescriptors[subpassIndex];
                subpassDescriptor.m_rendertargetCount = subpassLayout.m_rendertargetCount;
                subpassDescriptor.m_subpassInputCount = subpassLayout.m_subpassInputCount;
                if (subpassLayout.m_depthStencilDescriptor.IsValid())
                {
                    subpassDescriptor.m_depthStencilAttachment = RenderPass::SubpassAttachment{
                        subpassLayout.m_depthStencilDescriptor.m_attachmentIndex,
                        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
                    usedAttachments.set(subpassLayout.m_depthStencilDescriptor.m_attachmentIndex);
                }

                for (uint32_t colorAttachmentIndex = 0; colorAttachmentIndex < subpassLayout.m_rendertargetCount; ++colorAttachmentIndex)
                {
                    RenderPass::SubpassAttachment& subpassAttachment = subpassDescriptor.m_rendertargetAttachments[colorAttachmentIndex];
                    subpassAttachment.m_attachmentIndex = subpassLayout.m_rendertargetDescriptors[colorAttachmentIndex].m_attachmentIndex;
                    subpassAttachment.m_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    usedAttachments.set(subpassAttachment.m_attachmentIndex);
                    RenderPass::SubpassAttachment& resolveSubpassAttachment = subpassDescriptor.m_resolveAttachments[colorAttachmentIndex];
                    resolveSubpassAttachment.m_attachmentIndex = subpassLayout.m_rendertargetDescriptors[colorAttachmentIndex].m_resolveAttachmentIndex;
                    resolveSubpassAttachment.m_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    if (resolveSubpassAttachment.IsValid())
                    {
                        // Set the number of samples for resolve attachments to 1.
                        renderPassDesc.m_attachments[resolveSubpassAttachment.m_attachmentIndex].m_multisampleState.m_samples = 1;
                        usedAttachments.set(resolveSubpassAttachment.m_attachmentIndex);
                    }
                }

                for (uint32_t inputAttachmentIndex = 0; inputAttachmentIndex < subpassLayout.m_subpassInputCount; ++inputAttachmentIndex)
                {
                    RenderPass::SubpassAttachment& subpassAttachment = subpassDescriptor.m_subpassInputAttachments[inputAttachmentIndex];
                    subpassAttachment.m_attachmentIndex = subpassLayout.m_subpassInputDescriptors[inputAttachmentIndex].m_attachmentIndex;
                    subpassAttachment.m_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    subpassAttachment.m_imageAspectFlags =
                        ConvertImageAspectFlags(subpassLayout.m_subpassInputDescriptors[inputAttachmentIndex].m_aspectFlags);
                    usedAttachments.set(subpassAttachment.m_attachmentIndex);
                }

                if (subpassLayout.m_shadingRateDescriptor.IsValid())
                {
                    VkImageLayout imageLayout;
                    switch (device.GetImageShadingRateMode())
                    {
                    case Device::ShadingRateImageMode::DensityMap:
                        imageLayout = VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT;
                        break;
                    case Device::ShadingRateImageMode::ImageAttachment:
                        imageLayout = VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
                        break;
                    default:
                        AZ_Assert(false, "Invalid image shading rate mode %d", device.GetImageShadingRateMode());
                        imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                        break;
                    }
                    subpassDescriptor.m_fragmentShadingRateAttachment = RenderPass::SubpassAttachment
                    {
                        subpassLayout.m_shadingRateDescriptor.m_attachmentIndex,
                        imageLayout
                    };
                    usedAttachments.set(subpassLayout.m_shadingRateDescriptor.m_attachmentIndex);
                    renderPassDesc.m_attachments[subpassLayout.m_shadingRateDescriptor.m_attachmentIndex].m_loadStoreAction =
                        subpassLayout.m_shadingRateDescriptor.m_loadStoreAction;
                }

                // [GFX_TODO][ATOM-3948] Implement preserve attachments. For now preserve all attachments.
                for (uint32_t attachmentIndex = 0; attachmentIndex < renderPassDesc.m_attachmentCount; ++attachmentIndex)
                {
                    if (!usedAttachments[attachmentIndex])
                    {
                        subpassDescriptor.m_preserveAttachments[subpassDescriptor.m_preserveAttachmentCount++] = attachmentIndex;
                    }
                }
            }

            return renderPassDesc;
        }

        void RenderPass::SetNameInternal(const AZStd::string_view& name)
        {
            if (IsInitialized() && !name.empty())
            {
                Debug::SetNameToObject(reinterpret_cast<uint64_t>(m_nativeRenderPass), name.data(), VK_OBJECT_TYPE_RENDER_PASS, static_cast<Device&>(GetDevice()));
            }
        }

        void RenderPass::Shutdown()
        {
            if (m_nativeRenderPass != VK_NULL_HANDLE)
            {
                auto& device = static_cast<Device&>(GetDevice());
                device.GetContext().DestroyRenderPass(device.GetNativeDevice(), m_nativeRenderPass, VkSystemAllocator::Get());
                m_nativeRenderPass = VK_NULL_HANDLE;
            }
            Base::Shutdown();
        }
    }
}

