/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/Feature/CoreLights/CoreLightsConstants.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/std/containers/vector.h>
#include <CoreLights/ShadowmapPass.h>

namespace AZ
{
    namespace Render
    {
        //! ProjectedShadowmapsPass owns shadowmap passes for projected lights.
        class ProjectedShadowmapsPass final
            : public RPI::ParentPass
        {
            AZ_RPI_PASS(ProjectedShadowmapsPass);
            using Base = RPI::ParentPass;

        public:
            AZ_CLASS_ALLOCATOR(ProjectedShadowmapsPass, SystemAllocator, 0);
            AZ_RTTI(ProjectedShadowmapsPass, "00024B13-1095-40FA-BEC3-B0F68110BEA2", Base);

            virtual ~ProjectedShadowmapsPass();
            static RPI::Ptr<ProjectedShadowmapsPass> Create(const RPI::PassDescriptor& descriptor);

            // RPI::Pass overrides
            RHI::DrawListTag GetDrawListTag() const override;
            const AZ::RPI::PipelineViewTag& GetPipelineViewTag() const override;

            void SetAtlasAttachmentImage(Data::Instance<RPI::AttachmentImage> atlasAttachmentIamge);

            void QueueAddChild(RPI::Ptr<Pass> pass);
            void QueueRemoveChild(RPI::Ptr<Pass> pass);

        private:
            ProjectedShadowmapsPass() = delete;
            explicit ProjectedShadowmapsPass(const RPI::PassDescriptor& descriptor);

            // RPI::Pass overrides...
            void BuildInternal() override;

            RHI::DrawListTag m_drawListTag;
            RPI::PipelineViewTag m_pipelineViewTag;
            Data::Instance<RPI::AttachmentImage> m_atlasAttachmentImage;

            struct PassAddRemove
            {
                RPI::Ptr<Pass> m_pass;
                bool m_isRemoval;
            };
            AZStd::vector<PassAddRemove> m_passesToAddOrRemove;

        };
    } // namespace Render
} // namespace AZ
