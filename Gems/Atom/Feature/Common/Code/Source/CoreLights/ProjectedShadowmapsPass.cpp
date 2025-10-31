/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CoreLights/ProjectedShadowmapsPass.h>
#include <Atom/RHI/DrawListTagRegistry.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Reflect/Pass/RasterPassData.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<ProjectedShadowmapsPass> ProjectedShadowmapsPass::Create(const RPI::PassDescriptor& descriptor)
        {
            return aznew ProjectedShadowmapsPass(descriptor);
        }

        ProjectedShadowmapsPass::ProjectedShadowmapsPass(const RPI::PassDescriptor& descriptor)
            : Base(descriptor)
        {
            // Pass has it's own logic for managing children, forgo ParentPass logic
            m_flags.m_createChildren = false;

            const RPI::RasterPassData* passData = RPI::PassUtils::GetPassData<RPI::RasterPassData>(descriptor);
            if (passData)
            {
                auto* rhiSystem = RHI::RHISystemInterface::Get();
                m_drawListTag = rhiSystem->GetDrawListTagRegistry()->AcquireTag(passData->m_drawListTag);
                m_pipelineViewTag = passData->m_pipelineViewTag;
            }
        }

        ProjectedShadowmapsPass::~ProjectedShadowmapsPass()
        {
            if (m_drawListTag.IsValid())
            {
                auto* rhiSystem = RHI::RHISystemInterface::Get();
                rhiSystem->GetDrawListTagRegistry()->ReleaseTag(m_drawListTag);
            }
        }

        RHI::DrawListTag ProjectedShadowmapsPass::GetDrawListTag() const
        {
            return m_drawListTag;
        }

        const RPI::PipelineViewTag& ProjectedShadowmapsPass::GetPipelineViewTag() const
        {
            return m_pipelineViewTag;
        }

        void ProjectedShadowmapsPass::SetAtlasAttachmentImage(Data::Instance<RPI::AttachmentImage> atlasAttachmentIamge)
        {
            if (m_atlasAttachmentImage != atlasAttachmentIamge)
            {
                m_atlasAttachmentImage = atlasAttachmentIamge;
                QueueForBuildAndInitialization();
            }
        }

        void ProjectedShadowmapsPass::BuildInternal()
        {
            for (auto passToAddOrRemove : m_passesToAddOrRemove)
            {
                if (passToAddOrRemove.m_isRemoval)
                {
                    RemoveChild(passToAddOrRemove.m_pass);
                }
                else
                {
                    AddChild(passToAddOrRemove.m_pass);
                }
            }
            m_passesToAddOrRemove.clear();

            if (!m_atlasAttachmentImage)
            {
                auto depthAttachmentImage = RPI::ImageSystemInterface::Get()->GetSystemAttachmentImage(RHI::Format::D32_FLOAT);
                AttachImageToSlot(Name("Shadowmap"), depthAttachmentImage);
                SetEnabled(false);
                return;
            }

            AttachImageToSlot(Name("Shadowmap"), m_atlasAttachmentImage);
            SetEnabled(true);
            Base::BuildInternal();
        }

        void ProjectedShadowmapsPass::QueueAddChild(RPI::Ptr<Pass> pass)
        {
            m_passesToAddOrRemove.push_back({ pass, false });
            QueueForBuildAndInitialization(); // passes in `m_passesToAddOrRemove` are resolved in `BuildInternal()`
        }

        void ProjectedShadowmapsPass::QueueRemoveChild(RPI::Ptr<Pass> pass)
        {
            m_passesToAddOrRemove.push_back({ pass, true });
            QueueForBuildAndInitialization(); // passes in `m_passesToAddOrRemove` are resolved in `BuildInternal()`
        }

    } // namespace Render
} // namespace AZ
