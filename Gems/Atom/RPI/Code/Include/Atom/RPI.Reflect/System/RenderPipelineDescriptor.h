/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Configuration.h>
#include <Atom/RPI.Reflect/Pass/PassAsset.h>
#include <Atom/RPI.Reflect/System/PipelineRenderSettings.h>
#include <AzCore/Asset/AssetCommon.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
         // Rendering Settings
        enum class AntiAliasingMode
        {
            MSAA,
            SMAA,
            TAA,
            Default = -1
        };


        //! A descriptor used to create a render pipeline
        struct ATOM_RPI_REFLECT_API RenderPipelineDescriptor final
        {
            AZ_TYPE_INFO(RenderPipelineDescriptor, "{B1A5CF41-AC8D-440E-A1E9-3544D7F3445B}");
            static void Reflect(AZ::ReflectContext* context);

            static constexpr char const Extension[] = "RenderPipeline.azasset";

            //! The root pass template name 
            AZStd::string m_rootPassTemplate;

            //! The name string of the pipeline view tag we want to use as default view tag.
            //! The view associated to this view tag should be persistent.
            //! Some passes in this pipeline should use this view tag
            AZStd::string m_mainViewTagName = "MainCamera";

            //! Render Pipeline's name. This name will be used to identify the render pipeline when it's added to a scene.
            //! Render pipelines in the same scene won't have same name.
            AZStd::string m_name;

            //! This will be used in the render pipeline's DrawFilterMask for draw item filtering.
            AZStd::string m_materialPipelineTag = "MainPipeline";

            //! Render settings that can be queried by passes to set things like MSAA on attachments
            PipelineRenderSettings m_renderSettings;

            //! Flag indicating if this pipeline should execute one time and then be removed.
            bool m_executeOnce = false;

            //! Flag indicating if this pipeline can accept modifications from scene's feature processors
            bool m_allowModification = false;

            //! Flag indicating if this pipeline can accept subpass merging.
            bool m_allowSubpassMerging = false;

            //! Include(MSAA, TAA, SMAA)
            AZStd::string m_defaultAAMethod = "MSAA";
        };

    } // namespace RPI
} // namespace AZ
