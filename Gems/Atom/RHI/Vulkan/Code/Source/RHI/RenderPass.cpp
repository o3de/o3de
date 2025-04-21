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

// Enable this define to enable logging of subpass merge feedback.
//#define AZ_LOG_SUBPASS_MERGE_FEEDBACK 1

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
                    const PhysicalDevice& physicalDevice = static_cast<const PhysicalDevice&>(m_device.GetPhysicalDevice());
                    m_collectSubpassMergeInfo = descriptor.m_subpassCount > 1 &&
                        physicalDevice.IsOptionalDeviceExtensionSupported(OptionalDeviceExtension::SubpassMergeFeedback);
#if !AZ_LOG_SUBPASS_MERGE_FEEDBACK
                    m_collectSubpassMergeInfo = false;
#endif
                    m_descriptor = &descriptor;
                    AZStd::vector<VkAttachmentDescriptionType> attachmentDescriptions;
                    AZStd::vector<SubpassInfo> subpassInfo;
                    AZStd::vector<VkSubpassDescriptionType> subpassDescriptions;
                    AZStd::vector<VkSubpassDependencyType> subpassDependencies;
                    AZStd::vector<SubpassFeedbackInfo> subpassFeedback;

                    BuildAttachmentDescriptions(attachmentDescriptions);
                    BuildSubpassAttachmentReferences(subpassInfo);
                    BuildSubpassDescriptions(subpassInfo, subpassDescriptions, subpassFeedback);
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

                            AppendVkStruct(createInfo, &fdmAttachmentCreateInfo);
                        }
                    }

                    VkRenderPassCreationFeedbackCreateInfoEXT renderPassCreationFeedbackCreateInfo;
                    VkRenderPassCreationFeedbackInfoEXT renderPassCreationFeedbackInfo{};
                    if (m_collectSubpassMergeInfo)
                    {
                        renderPassCreationFeedbackCreateInfo = {};
                        renderPassCreationFeedbackCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATION_FEEDBACK_CREATE_INFO_EXT;
                        renderPassCreationFeedbackCreateInfo.pRenderPassFeedback = &renderPassCreationFeedbackInfo;
                        AppendVkStruct(createInfo, &renderPassCreationFeedbackCreateInfo);
                    }

                    RenderPassResult result = Create<VkRenderPassCreateInfoType>(createInfo);
                    if (m_collectSubpassMergeInfo)
                    {
                        if (renderPassCreationFeedbackInfo.postMergeSubpassCount > 1)
                        {
                            AZ_Printf(
                                "Vulkan",
                                "%d subpasses were merged from %d subpasses available",
                                createInfo.subpassCount - renderPassCreationFeedbackInfo.postMergeSubpassCount,
                                createInfo.subpassCount);

                            for (uint32_t i = 0; i < subpassFeedback.size(); ++i)
                            {
                                const VkRenderPassSubpassFeedbackInfoEXT& info = subpassFeedback[i].second;
                                if (i > 0 && info.subpassMergeStatus != VK_SUBPASS_MERGE_STATUS_MERGED_EXT)
                                {
                                    AZ_Printf("Vulkan", "Subpass %d was not merged due to: %s", i, info.description);
                                }
                            }
                        }
                        else
                        {
                            AZ_Printf(
                                "Vulkan",
                                "All subpasses (%d) were successfully merged",
                                createInfo.subpassCount);
                        }
                    }
                    return result;
                }

            private:
                using SubpassFeedbackInfo = AZStd::pair<VkRenderPassSubpassFeedbackCreateInfoEXT, VkRenderPassSubpassFeedbackInfoEXT>; 

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
                        desc.loadOp = ConvertAttachmentLoadAction(binding.m_loadStoreAction.m_loadAction, m_device);
                        desc.storeOp = ConvertAttachmentStoreAction(binding.m_loadStoreAction.m_storeAction, m_device);
                        desc.stencilLoadOp = ConvertAttachmentLoadAction(binding.m_loadStoreAction.m_loadActionStencil, m_device);
                        desc.stencilStoreOp = ConvertAttachmentStoreAction(binding.m_loadStoreAction.m_storeActionStencil, m_device);
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
                    AZStd::vector<VkSubpassDescriptionType>& subpassDescriptions,
                    AZStd::vector<SubpassFeedbackInfo>& subpassFeedback) const
                {
                    subpassDescriptions.resize(m_descriptor->m_subpassCount);
                    if (m_collectSubpassMergeInfo)
                    {
                        subpassFeedback.resize(m_descriptor->m_subpassCount);
                    }
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

                        if (m_collectSubpassMergeInfo)
                        {
                            auto& subpassFeedbackInfo = subpassFeedback[i];
                            subpassFeedbackInfo.first = {};
                            subpassFeedbackInfo.first.sType = VK_STRUCTURE_TYPE_RENDER_PASS_SUBPASS_FEEDBACK_CREATE_INFO_EXT;
                            subpassFeedbackInfo.second = {};
                            subpassFeedbackInfo.first.pSubpassFeedback = &subpassFeedbackInfo.second;
                            AppendVkStruct(desc, &subpassFeedbackInfo.first);
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
                bool m_collectSubpassMergeInfo = false;
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

        //! The purpose of this helper class is to reduce the complexity of the function
        //! RenderPass::ConvertRenderAttachmentLayout() regarding the definition of the
        //! Subpass Dependencies.
        //! It is expected that an instance of this class to be declared in the stack
        //! of the function RenderPass::ConvertRenderAttachmentLayout().
        class SubpassDependencyHelper final
        {
            struct SrcDstPipelineStageFlags
            {
                PipelineAccessFlags m_srcPipelineAccessFlags;
                PipelineAccessFlags m_dstPipelineAccessFlags;
            };
            using PipelineStageFlagsList = AZStd::vector<SrcDstPipelineStageFlags>; // Indexed by attachment index.

        public:
            SubpassDependencyHelper() = delete;
            ~SubpassDependencyHelper() = default;
            SubpassDependencyHelper(RenderPass::Descriptor& renderPassDesc)
                : m_renderPassDescriptor(renderPassDesc)
            {
                m_subpassCount = renderPassDesc.m_subpassCount;
                AZ_Assert(m_subpassCount > 0, "Invalid Subpass Count from Render Pass Descriptor.");

                // Subpass dependencies only matter when there's more than one subpass.
                // The usage of this helper class will be a no-op.
                if (m_subpassCount < 2)
                {
                    return;
                }

                m_srcPipelineAccessFlags.resize(renderPassDesc.m_attachmentCount);
            }

            //! Marks the beginning of a new subpass.
            void AddSubpassPipelineStageFlags(uint32_t currentSubpassIndex)
            {
                if (m_subpassCount < 2) // Subpass dependencies only matter when there's more than one subpass.
                {
                    return;
                }

                m_subpassesPipelineStageFlagsList.push_back({});
                AZ_Assert(m_currentSubpassIndex != currentSubpassIndex, "The new subpass index can not be the same as the current subpass index");
                m_currentSubpassIndex = currentSubpassIndex;
            }

            //! Adds a subpass dependency to @m_renderPassDescriptor when applicable.
            void AddSubpassDependency(const uint32_t attachmentIndex,
                                      const AZ::RHI::ScopeAttachmentUsage scopeAttachmentUsage,
                                      const AZ::RHI::ScopeAttachmentStage scopeAttachmentStage,
                                      const AZ::RHI::ScopeAttachmentAccess scopeAttachmentAccess,
                                      const VkImageUsageFlags shadingRateAttachmentUsageFlags = 0 /* Only relevant for shading rate attachment usage*/)
            {
                if (m_subpassCount < 2) // Subpass dependencies only matter when there's more than one subpass.
                {
                    return;
                }

                const uint32_t dstSubpassIndex = m_currentSubpassIndex;

                SrcDstPipelineStageFlags srcDstPipelineStageFlags;
                srcDstPipelineStageFlags.m_srcPipelineAccessFlags = m_srcPipelineAccessFlags[attachmentIndex];
                srcDstPipelineStageFlags.m_dstPipelineAccessFlags.m_pipelineStage = GetResourcePipelineStateFlags(scopeAttachmentUsage, scopeAttachmentStage, AZ::RHI::HardwareQueueClass::Graphics, shadingRateAttachmentUsageFlags);
                srcDstPipelineStageFlags.m_dstPipelineAccessFlags.m_access = GetResourceAccessFlags(scopeAttachmentAccess, scopeAttachmentUsage);

                // Resize if necessary.
                if ((attachmentIndex + 1) >= m_subpassesPipelineStageFlagsList[dstSubpassIndex].size())
                {
                    m_subpassesPipelineStageFlagsList[dstSubpassIndex].resize(attachmentIndex + 1);
                }
                m_subpassesPipelineStageFlagsList[dstSubpassIndex][attachmentIndex] = srcDstPipelineStageFlags;

                //! For this attachment, its Destination pipeline access flags will become the Source pipeline access flags
                //! for some future subpass where this attachment may be referenced.
                m_srcPipelineAccessFlags[attachmentIndex] = srcDstPipelineStageFlags.m_dstPipelineAccessFlags;

                auto lastUseIter = m_lastSubpassAttachmentUse.find(attachmentIndex);
                if (lastUseIter == m_lastSubpassAttachmentUse.end())
                {
                    // No need to declare subpass depencies for external dependencies as those will be handled
                    // by the framegraph.
                    m_lastSubpassAttachmentUse[attachmentIndex] = dstSubpassIndex;
                    return;
                }
                uint32_t srcSubpassIndex = lastUseIter->second;
                m_lastSubpassAttachmentUse[attachmentIndex] = dstSubpassIndex;

                // Resolve attachments only depend on their MSAA attachment of this subpass.
                // so no need to add the resource dependency, BUT one thing to keep in mind is that
                // resolve attachments can't be referenced in the following subpass (+1), but they could
                // be referenced in the subpasses (+2, or +3, etc).
                if (scopeAttachmentUsage == AZ::RHI::ScopeAttachmentUsage::Resolve)
                {
                    return;
                }

                m_renderPassDescriptor.m_subpassDependencies.emplace_back();
                VkSubpassDependency& dependency = m_renderPassDescriptor.m_subpassDependencies.back();
                dependency = {};
                dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
                dependency.srcSubpass = srcSubpassIndex;
                dependency.srcStageMask = srcDstPipelineStageFlags.m_srcPipelineAccessFlags.m_pipelineStage;
                dependency.srcAccessMask = srcDstPipelineStageFlags.m_srcPipelineAccessFlags.m_access;
                dependency.dstSubpass = dstSubpassIndex;
                dependency.dstStageMask = srcDstPipelineStageFlags.m_dstPipelineAccessFlags.m_pipelineStage;
                dependency.dstAccessMask = srcDstPipelineStageFlags.m_dstPipelineAccessFlags.m_access;
            }

        private:
            //! The output of this helper class will be stored in @m_renderPassDescriptor.m_subpassDependencies.
            RenderPass::Descriptor& m_renderPassDescriptor;

            uint32_t m_subpassCount; // Cached from @m_renderPassDescriptor.
            uint32_t m_currentSubpassIndex = aznumeric_cast<uint32_t>(-1);
            AZStd::vector<PipelineAccessFlags> m_srcPipelineAccessFlags; // Indexed by attachment index.
            AZStd::vector<PipelineStageFlagsList> m_subpassesPipelineStageFlagsList; // Indexed by subpass index.
            //! For a given attachment (by index), we record here the last Subpass (by Index) where
            //! it was utilized. 
            using AttachmentIndex = uint32_t;
            using SubpassIndex = uint32_t;
            AZStd::unordered_map<AttachmentIndex, SubpassIndex> m_lastSubpassAttachmentUse;
        }; //class SubpassDependencyHelper

        void RenderPass::Descriptor::MergeSubpassDependencies()
        {
            if ( (m_subpassCount < 2) || (m_subpassDependencies.size() < 2) )
            {
                return;
            }

            //! Only two bits are active at a time. One for Source Subpass, the other for Destination Subpass 
            using SubpassPairMask = uint32_t;
            AZStd::unordered_map<SubpassPairMask, VkSubpassDependency> uniqueDependencies;
            for (const auto& subpassDependency : m_subpassDependencies)
            {
                SubpassPairMask subpassPairMask = (1 << subpassDependency.srcSubpass) | (1 << subpassDependency.dstSubpass);
                auto iter = uniqueDependencies.find(subpassPairMask);
                if (iter == uniqueDependencies.end())
                {
                    uniqueDependencies[subpassPairMask] = subpassDependency;
                    continue;
                }
                VkSubpassDependency& mergedDependency = iter->second;
                mergedDependency.srcAccessMask |= subpassDependency.srcAccessMask;
                mergedDependency.srcStageMask |= subpassDependency.srcStageMask;
                mergedDependency.dstAccessMask |= subpassDependency.dstAccessMask;
                mergedDependency.dstStageMask |= subpassDependency.dstStageMask;
            }

            // Collect all unique dependencies in vector form with consistent order using increasing
            // subpass indices.
            AZStd::vector<VkSubpassDependency> mergedDependencies;
            mergedDependencies.reserve(uniqueDependencies.size());
            for (uint32_t srcSubpass = 0; srcSubpass < m_subpassCount; srcSubpass++)
            {
                for (uint32_t dstSubpass = srcSubpass + 1; dstSubpass < m_subpassCount; dstSubpass++)
                {
                    SubpassPairMask subpassPairMask = (1 << srcSubpass) | (1 << dstSubpass);
                    const auto iter = uniqueDependencies.find(subpassPairMask);
                    if (iter == uniqueDependencies.end())
                    {
                        continue;
                    }
                    mergedDependencies.push_back(iter->second);
                }
            }

            AZStd::swap(m_subpassDependencies, mergedDependencies);
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
                renderPassDesc.m_attachments[index].m_loadStoreAction = RHI::AttachmentLoadStoreAction(
                    {},
                    RHI::AttachmentLoadAction::DontCare,
                    RHI::AttachmentStoreAction::DontCare,
                    RHI::AttachmentLoadAction::DontCare,
                    RHI::AttachmentStoreAction::DontCare);
            }

            auto setLayoutFunc = [&](RHI::RenderAttachmentExtras* extras,
                                      RenderPass::SubpassAttachment& subpassAttachment)
            {
                if (const RenderAttachmentLayout* extraInfo = azrtti_cast<const RenderAttachmentLayout*>(extras))
                {
                    subpassAttachment.m_layout = extraInfo->m_layout;
                }
            };

            AZStd::bitset<RHI::Limits::Pipeline::RenderAttachmentCountMax> loadActionSet;
            AZStd::bitset<RHI::Limits::Pipeline::RenderAttachmentCountMax> loadStencilActionSet;
            auto setAttachmentLoadStoreActionFunc =
                [&](const uint32_t attachmentIndex, const RHI::AttachmentLoadStoreAction& loadStoreAction)
            {
                auto& attachmentLoadStoreAction = renderPassDesc.m_attachments[attachmentIndex].m_loadStoreAction;
                attachmentLoadStoreAction.m_loadAction = loadActionSet.test(attachmentIndex)
                    ? CombineLoadOp(attachmentLoadStoreAction.m_loadAction, loadStoreAction.m_loadAction)
                    : loadStoreAction.m_loadAction;
                loadActionSet.set(attachmentIndex);
                attachmentLoadStoreAction.m_storeAction =
                    CombineStoreOp(attachmentLoadStoreAction.m_storeAction, loadStoreAction.m_storeAction);

                attachmentLoadStoreAction.m_loadActionStencil = loadStencilActionSet.test(attachmentIndex)
                    ? CombineLoadOp(attachmentLoadStoreAction.m_loadActionStencil, loadStoreAction.m_loadActionStencil)
                    : loadStoreAction.m_loadActionStencil;
                loadStencilActionSet.set(attachmentIndex);
                attachmentLoadStoreAction.m_storeActionStencil =
                    CombineStoreOp(attachmentLoadStoreAction.m_storeActionStencil, loadStoreAction.m_storeActionStencil);
            };

            renderPassDesc.m_subpassCount = layout.m_subpassCount;
            SubpassDependencyHelper subpassDependencyHelper(renderPassDesc);
            for (uint32_t subpassIndex = 0; subpassIndex < layout.m_subpassCount; ++subpassIndex)
            {
                subpassDependencyHelper.AddSubpassPipelineStageFlags(subpassIndex);

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

                    setLayoutFunc(
                        subpassLayout.m_depthStencilDescriptor.m_extras,
                        subpassDescriptor.m_depthStencilAttachment);

                    setAttachmentLoadStoreActionFunc(
                        subpassLayout.m_depthStencilDescriptor.m_attachmentIndex, subpassLayout.m_depthStencilDescriptor.m_loadStoreAction);

                    subpassDependencyHelper.AddSubpassDependency(subpassLayout.m_depthStencilDescriptor.m_attachmentIndex,
                        RHI::ScopeAttachmentUsage::DepthStencil,
                        subpassLayout.m_depthStencilDescriptor.m_scopeAttachmentStage,
                        subpassLayout.m_depthStencilDescriptor.m_scopeAttachmentAccess);
                }

                for (uint32_t colorAttachmentIndex = 0; colorAttachmentIndex < subpassLayout.m_rendertargetCount; ++colorAttachmentIndex)
                {
                    const auto& renderAttachmentDescriptor = subpassLayout.m_rendertargetDescriptors[colorAttachmentIndex];

                    RenderPass::SubpassAttachment& subpassAttachment = subpassDescriptor.m_rendertargetAttachments[colorAttachmentIndex];
                    subpassAttachment.m_attachmentIndex = renderAttachmentDescriptor.m_attachmentIndex;
                    subpassAttachment.m_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    usedAttachments.set(subpassAttachment.m_attachmentIndex);

                    setLayoutFunc(
                        renderAttachmentDescriptor.m_extras,
                        subpassAttachment);

                    setAttachmentLoadStoreActionFunc(
                        renderAttachmentDescriptor.m_attachmentIndex, renderAttachmentDescriptor.m_loadStoreAction);

                    subpassDependencyHelper.AddSubpassDependency(subpassAttachment.m_attachmentIndex,
                        AZ::RHI::ScopeAttachmentUsage::RenderTarget,
                        renderAttachmentDescriptor.m_scopeAttachmentStage,
                        renderAttachmentDescriptor.m_scopeAttachmentAccess);

                    RenderPass::SubpassAttachment& resolveSubpassAttachment = subpassDescriptor.m_resolveAttachments[colorAttachmentIndex];
                    resolveSubpassAttachment.m_attachmentIndex = renderAttachmentDescriptor.m_resolveAttachmentIndex;
                    resolveSubpassAttachment.m_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    if (resolveSubpassAttachment.IsValid())
                    {
                        // Set the number of samples for resolve attachments to 1.
                        auto& resolveAttachmentDesc = renderPassDesc.m_attachments[resolveSubpassAttachment.m_attachmentIndex];
                        resolveAttachmentDesc.m_multisampleState.m_samples = 1;
                        resolveAttachmentDesc.m_loadStoreAction.m_loadAction = RHI::AttachmentLoadAction::DontCare;
                        resolveAttachmentDesc.m_loadStoreAction.m_storeAction = RHI::AttachmentStoreAction::Store;
                        usedAttachments.set(resolveSubpassAttachment.m_attachmentIndex);

                        subpassDependencyHelper.AddSubpassDependency(
                            resolveSubpassAttachment.m_attachmentIndex,
                            AZ::RHI::ScopeAttachmentUsage::Resolve,
                            AZ::RHI::ScopeAttachmentStage::Any, // stage is irrelevant for Resolve.
                            AZ::RHI::ScopeAttachmentAccess::Write); // access is irrelevant for Resolve.
                    }
                }

                for (uint32_t inputAttachmentIndex = 0; inputAttachmentIndex < subpassLayout.m_subpassInputCount; ++inputAttachmentIndex)
                {
                    const auto& inputAttachmentDescriptor = subpassLayout.m_subpassInputDescriptors[inputAttachmentIndex];

                    const auto& firstSubpassDepthStencilDescriptor = layout.m_subpassLayouts[0].m_depthStencilDescriptor;
                    bool isDepthStencil = (firstSubpassDepthStencilDescriptor.m_attachmentIndex == inputAttachmentDescriptor.m_attachmentIndex);

                    RenderPass::SubpassAttachment& subpassAttachment = subpassDescriptor.m_subpassInputAttachments[inputAttachmentIndex];
                    subpassAttachment.m_attachmentIndex = inputAttachmentDescriptor.m_attachmentIndex;
                    subpassAttachment.m_layout = isDepthStencil ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    RHI::ImageAspectFlags filteredFlags = RHI::FilterBits(
                        inputAttachmentDescriptor.m_aspectFlags,
                        GetImageAspectFlags(layout.m_attachmentFormats[inputAttachmentDescriptor.m_attachmentIndex]));
                    subpassAttachment.m_imageAspectFlags = ConvertImageAspectFlags(filteredFlags);
                    usedAttachments.set(subpassAttachment.m_attachmentIndex);

                    setLayoutFunc(
                        inputAttachmentDescriptor.m_extras,
                        subpassAttachment);

                    setAttachmentLoadStoreActionFunc(subpassAttachment.m_attachmentIndex, inputAttachmentDescriptor.m_loadStoreAction);

                    subpassDependencyHelper.AddSubpassDependency(
                        subpassAttachment.m_attachmentIndex,
                        AZ::RHI::ScopeAttachmentUsage::SubpassInput,
                        inputAttachmentDescriptor.m_scopeAttachmentStage,
                        inputAttachmentDescriptor.m_scopeAttachmentAccess);
                }

                if (subpassLayout.m_shadingRateDescriptor.IsValid())
                {
                    VkImageUsageFlags shadingRateAttachmentUsageFlags = 0;
                    VkImageLayout imageLayout;
                    switch (device.GetImageShadingRateMode())
                    {
                    case Device::ShadingRateImageMode::DensityMap:
                        imageLayout = VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT;
                        shadingRateAttachmentUsageFlags = VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT;
                        break;
                    case Device::ShadingRateImageMode::ImageAttachment:
                        imageLayout = VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
                        shadingRateAttachmentUsageFlags = VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
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

                    setLayoutFunc(
                        subpassLayout.m_shadingRateDescriptor.m_extras,
                        subpassDescriptor.m_fragmentShadingRateAttachment);

                    setAttachmentLoadStoreActionFunc(
                        subpassLayout.m_shadingRateDescriptor.m_attachmentIndex,
                        subpassLayout.m_shadingRateDescriptor.m_loadStoreAction);

                    subpassDependencyHelper.AddSubpassDependency(
                        subpassLayout.m_shadingRateDescriptor.m_attachmentIndex,
                        AZ::RHI::ScopeAttachmentUsage::ShadingRate,
                        AZ::RHI::ScopeAttachmentStage::ShadingRate,
                        AZ::RHI::ScopeAttachmentAccess::Unknown, // access is Irrelevant for shading rate attachments.
                        shadingRateAttachmentUsageFlags);
                }

                for (uint32_t attachmentIndex = 0; attachmentIndex < renderPassDesc.m_attachmentCount; ++attachmentIndex)
                {
                    // First check if the attachment was used in the subpass.
                    if (!usedAttachments[attachmentIndex])
                    {
                        // Find the load store action of the next use of this attachment.
                        AZStd::vector<RHI::AttachmentLoadAction> nextLoadActions;
                        for (uint32_t i = subpassIndex + 1; i < layout.m_subpassCount && nextLoadActions.empty(); ++i)
                        {
                            const auto& subpassLayoutPreserve = layout.m_subpassLayouts[i];
                            if (subpassLayoutPreserve.m_depthStencilDescriptor.m_attachmentIndex == attachmentIndex)
                            {
                                nextLoadActions.push_back(subpassLayoutPreserve.m_depthStencilDescriptor.m_loadStoreAction.m_loadAction);
                            }

                            if (subpassLayoutPreserve.m_shadingRateDescriptor.m_attachmentIndex == attachmentIndex)
                            {
                                nextLoadActions.push_back(subpassLayoutPreserve.m_shadingRateDescriptor.m_loadStoreAction.m_loadAction);
                            }

                            for (uint32_t colorIndex = 0; colorIndex < subpassLayoutPreserve.m_rendertargetCount; ++colorIndex)
                            {
                                const auto& renderTargetDesc = subpassLayoutPreserve.m_rendertargetDescriptors[colorIndex];
                                if (attachmentIndex == renderTargetDesc.m_attachmentIndex)
                                {
                                    nextLoadActions.push_back(renderTargetDesc.m_loadStoreAction.m_loadAction);
                                    break;
                                }
                            }

                            for (uint32_t inputIndex = 0; inputIndex < subpassLayoutPreserve.m_subpassInputCount; ++inputIndex)
                            {
                                const auto& inputDesc = subpassLayoutPreserve.m_subpassInputDescriptors[inputIndex];
                                if (attachmentIndex == inputDesc.m_attachmentIndex)
                                {
                                    nextLoadActions.push_back(RHI::AttachmentLoadAction::Load);
                                    break;
                                }
                            }
                        }

                        bool preverseAttachment;
                        if (nextLoadActions.empty())
                        {
                            // This is the last usage, so we just check if we need to store the content.
                            preverseAttachment = renderPassDesc.m_attachments[attachmentIndex].m_loadStoreAction.m_storeAction !=
                                RHI::AttachmentStoreAction::DontCare;
                        }
                        else
                        {
                            // Check if the next usage wants to load the content. If not, we don't need to preserve it.
                            preverseAttachment = false;
                            for (RHI::AttachmentLoadAction loadAction : nextLoadActions)
                            {
                                if (loadAction != RHI::AttachmentLoadAction::DontCare)
                                {
                                    preverseAttachment = true;
                                    break;
                                }
                            }
                        }

                        if (preverseAttachment)
                        {
                            subpassDescriptor.m_preserveAttachments[subpassDescriptor.m_preserveAttachmentCount++] = attachmentIndex;
                        }
                    }
                }

            }
            renderPassDesc.MergeSubpassDependencies();

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

    } // namespace Vulkan
} // namespace AZ

