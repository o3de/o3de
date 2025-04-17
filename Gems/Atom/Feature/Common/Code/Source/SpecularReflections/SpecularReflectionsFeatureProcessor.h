/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/SpecularReflections/SpecularReflectionsFeatureProcessorInterface.h>
#include <Atom/RHI.Reflect/ShaderInputNameIndex.h>

namespace AZ
{
    namespace Render
    {
        class SpecularReflectionsFeatureProcessor final
            : public SpecularReflectionsFeatureProcessorInterface
        {
        public:
            AZ_CLASS_ALLOCATOR(SpecularReflectionsFeatureProcessor, SystemAllocator)
            AZ_RTTI(AZ::Render::SpecularReflectionsFeatureProcessor, "{3C08E4DD-B4A4-4FD6-A56A-D1D97A8C31CD}", AZ::Render::SpecularReflectionsFeatureProcessorInterface);

            static void Reflect(AZ::ReflectContext* context);

            SpecularReflectionsFeatureProcessor() = default;
            virtual ~SpecularReflectionsFeatureProcessor() = default;

            void Activate() override;
            void Deactivate() override;

            // ReflectionProbeFeatureProcessorInterface overrides
            void SetSSROptions(const SSROptions& ssrOptions) override;
            const SSROptions& GetSSROptions() const override { return m_ssrOptions; }

        private:
            AZ_DISABLE_COPY_MOVE(SpecularReflectionsFeatureProcessor);

            // RPI::SceneNotificationBus::Handler overrides
            void OnRenderPipelineChanged(AZ::RPI::RenderPipeline* pipeline, AZ::RPI::SceneNotification::RenderPipelineChangeType changeType) override;

            void UpdatePasses();

            SSROptions m_ssrOptions;

            RHI::ShaderInputNameIndex m_invOutputScaleNameIndex = "m_invOutputScale";
            RHI::ShaderInputNameIndex m_maxRoughnessNameIndex = "m_maxRoughness";
            RHI::ShaderInputNameIndex m_reflectionMethodNameIndex = "m_reflectionMethod";
            RHI::ShaderInputNameIndex m_rayTraceFallbackSpecularNameIndex = "m_rayTraceFallbackSpecular";
        };
    } // namespace Render
} // namespace AZ
