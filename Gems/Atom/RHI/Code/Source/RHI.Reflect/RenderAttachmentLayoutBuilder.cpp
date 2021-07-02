/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/RenderAttachmentLayoutBuilder.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AZ
{
    namespace RHI
    {
        RenderAttachmentLayoutBuilder::RenderAttachmentLayoutBuilder()
        {
            Reset();
        }

        ResultCode RenderAttachmentLayoutBuilder::End(RenderAttachmentLayout& builtRenderAttachmentLayout)
        {
            auto& renderAttachmentFormats = builtRenderAttachmentLayout.m_attachmentFormats;
            Format depthStencilFormat = Format::Unknown;
            AZStd::unordered_map<AZ::Name, uint32_t> renderAttachmentsMap;
            // This lambda will handle if the render attachment needs to resolve and will add a resolve attachment if needed.
            auto handleResolveAttachment = [&](const SubpassAttachmentLayoutBuilder::RenderAttachmentEntry& attachment, uint32_t& resolveAttachmentIndex)
            {
                resolveAttachmentIndex = InvalidRenderAttachmentIndex;
                if (!attachment.m_resolveName.IsEmpty())
                {
                    AttachmentLoadStoreAction loadStoreAction;
                    loadStoreAction.m_loadAction = AttachmentLoadAction::DontCare;
                    loadStoreAction.m_storeAction = AttachmentStoreAction::Store;
                    auto findResolveIter = renderAttachmentsMap.find(attachment.m_resolveName);
                    if (findResolveIter == renderAttachmentsMap.end())
                    {
                        if (attachment.m_format == Format::Unknown)
                        {
                            AZ_Assert(false, "Invalid format for resolve attachment %s", attachment.m_name.GetCStr());
                            return ResultCode::InvalidArgument;
                        }
                        resolveAttachmentIndex = builtRenderAttachmentLayout.m_attachmentCount++;
                        renderAttachmentsMap[attachment.m_resolveName] = resolveAttachmentIndex;
                        renderAttachmentFormats[resolveAttachmentIndex] = attachment.m_format;
                    }
                    else
                    {
                        resolveAttachmentIndex = findResolveIter->second;
                    }

                    if (attachment.m_format != Format::Unknown && renderAttachmentFormats[resolveAttachmentIndex] != attachment.m_format)
                    {
                        AZ_Assert(false, "Incompatible format for resolve attachment %s. Expected %d. Actual %d", attachment.m_name.GetCStr(), renderAttachmentFormats[resolveAttachmentIndex], attachment.m_format);
                        return ResultCode::InvalidArgument;
                    }
                }
                return ResultCode::Success;
            };

            for (const auto& builder : m_subpassLayoutBuilders)
            {
                SubpassRenderAttachmentLayout& subpassLayout = builtRenderAttachmentLayout.m_subpassLayouts[builtRenderAttachmentLayout.m_subpassCount++];
                subpassLayout.m_rendertargetCount = static_cast<uint32_t>(builder.m_renderTargetAttachments.size());
                subpassLayout.m_subpassInputCount = static_cast<uint32_t>(builder.m_subpassInputAttachments.size());

                // First add the resolve attachments, so they can be found when adding the MS attachments.
                for (uint32_t i = 0; i < builder.m_renderTargetAttachments.size(); ++i)
                {
                    const SubpassAttachmentLayoutBuilder::RenderAttachmentEntry& renderTargetAttachment = builder.m_renderTargetAttachments[i];
                    uint32_t resolveAttachmentIndex;
                    handleResolveAttachment(renderTargetAttachment, resolveAttachmentIndex);
                }

                uint32_t resolveAttachmentIndex;
                ResultCode result = handleResolveAttachment(builder.m_depthStencilAttachment, resolveAttachmentIndex);

                for (uint32_t i = 0; i < builder.m_renderTargetAttachments.size(); ++i)
                {
                    const SubpassAttachmentLayoutBuilder::RenderAttachmentEntry& renderTargetAttachment = builder.m_renderTargetAttachments[i];
                    uint32_t attachmentIndex = 0;
                    // First look if the render target has already been added to the list of attachments.
                    auto findIter = renderAttachmentsMap.find(renderTargetAttachment.m_name);
                    if (findIter == renderAttachmentsMap.end())
                    {
                        if (renderTargetAttachment.m_format == Format::Unknown)
                        {
                            AZ_Assert(false, "Invalid format for rendertarget %s", renderTargetAttachment.m_name.GetCStr());
                            return ResultCode::InvalidArgument;
                        }
                        attachmentIndex = builtRenderAttachmentLayout.m_attachmentCount++;
                        renderAttachmentsMap[renderTargetAttachment.m_name] = attachmentIndex;
                        renderAttachmentFormats[attachmentIndex] = renderTargetAttachment.m_format;
                    }
                    else
                    {
                        attachmentIndex = findIter->second;
                    }

                    // Add the use of the attachment to the subpass.
                    result = handleResolveAttachment(renderTargetAttachment, resolveAttachmentIndex);
                    if (result != ResultCode::Success)
                    {
                        return result;
                    }

                    if (renderTargetAttachment.m_format != Format::Unknown && renderAttachmentFormats[attachmentIndex] != renderTargetAttachment.m_format)
                    {
                        AZ_Assert(false, "Incompatible format for attachment %s. Expected %d. Actual %d", renderTargetAttachment.m_name.GetCStr(), renderAttachmentFormats[attachmentIndex], renderTargetAttachment.m_format);
                        return ResultCode::InvalidArgument;
                    }

                    subpassLayout.m_rendertargetDescriptors[i] = RenderAttachmentDescriptor{ attachmentIndex, resolveAttachmentIndex, renderTargetAttachment.m_loadStoreAction };
                }

                if (!builder.m_depthStencilAttachment.m_name.IsEmpty())
                {
                    // Check if the depth/stencil has already been added and if it has, check if the format is the same.
                    if (depthStencilFormat != Format::Unknown &&
                        builder.m_depthStencilAttachment.m_format != Format::Unknown &&
                        depthStencilFormat != builder.m_depthStencilAttachment.m_format)
                    {
                        AZ_Assert(false, "Invalid depth stencil format. Expected %s. Current %s", ToString(depthStencilFormat), ToString(builder.m_depthStencilAttachment.m_format));
                        return ResultCode::InvalidArgument;
                    }

                    // Search for the depth/stencil attachment in the list of added attachments.
                    uint32_t attachmentIndex = 0;
                    auto findIter = renderAttachmentsMap.find(builder.m_depthStencilAttachment.m_name);
                    if (findIter == renderAttachmentsMap.end())
                    {
                        if (builder.m_depthStencilAttachment.m_format == Format::Unknown)
                        {
                            AZ_Assert(false, "Invalid depth stencil format %s", ToString(builder.m_depthStencilAttachment.m_format));
                            return ResultCode::InvalidArgument;
                        }

                        depthStencilFormat = builder.m_depthStencilAttachment.m_format;
                        attachmentIndex = builtRenderAttachmentLayout.m_attachmentCount++;
                        renderAttachmentsMap[builder.m_depthStencilAttachment.m_name] = attachmentIndex;
                        renderAttachmentFormats[attachmentIndex] = depthStencilFormat;
                    }
                    else
                    {
                        attachmentIndex = findIter->second;
                    }

                    result = handleResolveAttachment(builder.m_depthStencilAttachment, resolveAttachmentIndex);
                    if (result != ResultCode::Success)
                    {
                        return result;
                    }
                    subpassLayout.m_depthStencilDescriptor = RenderAttachmentDescriptor{ attachmentIndex, resolveAttachmentIndex, builder.m_depthStencilAttachment.m_loadStoreAction };
                }

                // Add the subpass inputs.
                for (uint32_t i = 0; i < builder.m_subpassInputAttachments.size(); ++i)
                {
                    auto& subpassInputAttachmentName = builder.m_subpassInputAttachments[i];
                    uint32_t attachmentIndex = 0;
                    auto findIter = renderAttachmentsMap.find(subpassInputAttachmentName);
                    if (findIter == renderAttachmentsMap.end())
                    {
                        AZ_Assert(false, "Could not find subpassInput %d", subpassInputAttachmentName.GetCStr());
                        return ResultCode::InvalidArgument;
                    }

                    attachmentIndex += findIter->second;
                    subpassLayout.m_subpassInputIndices[i] = attachmentIndex;
                }
            }

             return ResultCode::Success;
        }

        void RenderAttachmentLayoutBuilder::Reset()
        {
            m_subpassLayoutBuilders.clear();
        }

        RenderAttachmentLayoutBuilder::SubpassAttachmentLayoutBuilder* RenderAttachmentLayoutBuilder::AddSubpass()
        {
            m_subpassLayoutBuilders.push_back(SubpassAttachmentLayoutBuilder(static_cast<uint32_t>(m_subpassLayoutBuilders.size())));
            return &m_subpassLayoutBuilders.back();
        }
        
        RenderAttachmentLayoutBuilder::SubpassAttachmentLayoutBuilder::SubpassAttachmentLayoutBuilder(uint32_t subpassIndex)
            : m_subpassIndex(subpassIndex)
        {}

        RenderAttachmentLayoutBuilder::SubpassAttachmentLayoutBuilder* RenderAttachmentLayoutBuilder::SubpassAttachmentLayoutBuilder::RenderTargetAttachment(
            Format format,
            const AZ::Name& name /*= {}*/,
            const AttachmentLoadStoreAction& loadStoreAction /*= AttachmentLoadStoreAction()*/,            
            bool resolve /*= false*/)
        {
            AZ::Name attachmentName = name;
            if (attachmentName.IsEmpty())
            {
                // Assign a temp name if it's empty.
                attachmentName = AZStd::string::format("Color%zu_Subpass%d", m_renderTargetAttachments.size(), m_subpassIndex);
            }

            m_renderTargetAttachments.push_back({ attachmentName, format, loadStoreAction });
            if (resolve)
            {
                return ResolveAttachment(attachmentName);
            }
            return this;
        }

        RenderAttachmentLayoutBuilder::SubpassAttachmentLayoutBuilder* RenderAttachmentLayoutBuilder::SubpassAttachmentLayoutBuilder::RenderTargetAttachment(
            Format format,
            bool resolve)
        {
            return RenderTargetAttachment(format, {}, AttachmentLoadStoreAction(), resolve);
        }

        RenderAttachmentLayoutBuilder::SubpassAttachmentLayoutBuilder* RenderAttachmentLayoutBuilder::SubpassAttachmentLayoutBuilder::RenderTargetAttachment(
            const AZ::Name& name,
            bool resolve)
        {
            return RenderTargetAttachment(Format::Unknown, name, AttachmentLoadStoreAction(), resolve);
        }

        RenderAttachmentLayoutBuilder::SubpassAttachmentLayoutBuilder* RenderAttachmentLayoutBuilder::SubpassAttachmentLayoutBuilder::RenderTargetAttachment(
            const AZ::Name& name,
            const AttachmentLoadStoreAction& loadStoreAction /*= AttachmentLoadStoreAction()*/,
            bool resolve /*= false*/)
        {
            return RenderTargetAttachment(Format::Unknown, name, loadStoreAction, resolve);
        }

        RenderAttachmentLayoutBuilder::SubpassAttachmentLayoutBuilder* RenderAttachmentLayoutBuilder::SubpassAttachmentLayoutBuilder::ResolveAttachment(
            const AZ::Name& sourceName,
            const AZ::Name& resolveName /*= {}*/)
        {
            AZ::Name attachmentName = resolveName;
            if (attachmentName.IsEmpty())
            {
                // Assign a temp name if it's empty.
                attachmentName = AZStd::string::format("Resolve%zu_Subpass%d", m_renderTargetAttachments.size(), m_subpassIndex);
            }

            auto findIter = AZStd::find_if(m_renderTargetAttachments.begin(), m_renderTargetAttachments.end(), [sourceName](const RenderAttachmentEntry& entry)
            {
                return entry.m_name == sourceName;
            });

            if (findIter == m_renderTargetAttachments.end())
            {
                AZ_Assert(false, "Failed to find render target %d to resolve", sourceName.GetCStr());
                return this;
            }

            (*findIter).m_resolveName = attachmentName;
            return this;
        }

        RenderAttachmentLayoutBuilder::SubpassAttachmentLayoutBuilder* RenderAttachmentLayoutBuilder::SubpassAttachmentLayoutBuilder::DepthStencilAttachment(
            Format format,
            const AZ::Name& name /*= {}*/,
            const AttachmentLoadStoreAction& loadStoreAction /*= AttachmentLoadStoreAction()*/)
        {
            AZ_Assert(m_depthStencilAttachment.m_format == Format::Unknown || format == m_depthStencilAttachment.m_format, "DepthStencil format has already been set");
            // Assign a temp name if it's empty.
            m_depthStencilAttachment = RenderAttachmentEntry{ name.IsEmpty() ? AZ::Name("DepthStencil") : name, format, loadStoreAction, {} };
            return this;
        }

        RenderAttachmentLayoutBuilder::SubpassAttachmentLayoutBuilder* RenderAttachmentLayoutBuilder::SubpassAttachmentLayoutBuilder::DepthStencilAttachment(
            const AZ::Name name /*= {}*/,
            const AttachmentLoadStoreAction& loadStoreAction /*= AttachmentLoadStoreAction()*/)
        {
            return DepthStencilAttachment(m_depthStencilAttachment.m_format, name, loadStoreAction);
        }

        RenderAttachmentLayoutBuilder::SubpassAttachmentLayoutBuilder* RenderAttachmentLayoutBuilder::SubpassAttachmentLayoutBuilder::DepthStencilAttachment(
            const AttachmentLoadStoreAction& loadStoreAction)
        {
            return DepthStencilAttachment(m_depthStencilAttachment.m_format, {}, loadStoreAction);
        }

        RenderAttachmentLayoutBuilder::SubpassAttachmentLayoutBuilder* RenderAttachmentLayoutBuilder::SubpassAttachmentLayoutBuilder::SubpassInputAttachment(
            const AZ::Name& name)
        {
            m_subpassInputAttachments.push_back(name);
            return this;
        }
    } // namespace RHI
} // namespace AZ
