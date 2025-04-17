/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <DiffuseProbeGrid/DiffuseGlobalIlluminationFeatureProcessorInterface.h>

namespace AZ
{
    namespace Render
    {
        //! This class provides general features and configuration for the diffuse global illumination environment,
        //! which consists of DiffuseProbeGrids and the diffuse Global IBL cubemap.
        class DiffuseGlobalIlluminationFeatureProcessor final
            : public DiffuseGlobalIlluminationFeatureProcessorInterface
        {
        public:
            AZ_CLASS_ALLOCATOR(DiffuseGlobalIlluminationFeatureProcessor, SystemAllocator)
            AZ_RTTI(AZ::Render::DiffuseGlobalIlluminationFeatureProcessor, "{14F7DF46-AA2C-49EF-8A2C-0A7CB7390BB7}", AZ::Render::DiffuseGlobalIlluminationFeatureProcessorInterface);

            static void Reflect(AZ::ReflectContext* context);

            DiffuseGlobalIlluminationFeatureProcessor() = default;
            virtual ~DiffuseGlobalIlluminationFeatureProcessor() = default;

            void Activate() override;
            void Deactivate() override;

            // DiffuseGlobalIlluminationFeatureProcessorInterface overrides
            void SetQualityLevel(DiffuseGlobalIlluminationQualityLevel qualityLevel) override;

        private:
            AZ_DISABLE_COPY_MOVE(DiffuseGlobalIlluminationFeatureProcessor);

            // RPI::SceneNotificationBus::Handler overrides
            void OnRenderPipelineChanged(AZ::RPI::RenderPipeline* pipeline, AZ::RPI::SceneNotification::RenderPipelineChangeType changeType) override;

            void UpdatePasses();

            DiffuseGlobalIlluminationQualityLevel m_qualityLevel = DiffuseGlobalIlluminationQualityLevel::Low;
        };
    } // namespace Render
} // namespace AZ
