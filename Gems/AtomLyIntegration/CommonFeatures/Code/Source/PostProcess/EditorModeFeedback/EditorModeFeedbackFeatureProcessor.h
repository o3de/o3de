/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>

namespace AZ
{
    namespace RPI
    {
        class FullscreenTrianglePass;
    }

    namespace Render
    {
        class EditorModeFeatureProcessor
            : public RPI::FeatureProcessor
        {
        public:
            AZ_RTTI(AZ::Render::EditorModeFeatureProcessor, "{78D40D57-F564-4ECD-B9F5-D8C9784B15D0}", RPI::FeatureProcessor);

            static void Reflect(AZ::ReflectContext* context);

            // FeatureProcessor overrides ...
            void Activate() override;
            void Deactivate() override;
            void ApplyRenderPipelineChange(RPI::RenderPipeline* renderPipeline) override;
            void Render(const FeatureProcessor::RenderPacket& packet) override;
            void Simulate(const SimulatePacket& packet) override;

            // RPI::SceneNotificationBus overrides ...
            void OnRenderPipelineAdded(RPI::RenderPipelinePtr pipeline) override;
            void OnRenderPipelinePassesChanged(RPI::RenderPipeline* renderPipeline) override;
            void OnBeginPrepareRender() override;

        private:
            void InitPasses(RPI::RenderPipeline* renderPipeline);

            RPI::FullscreenTrianglePass* m_tintFullscreenTrianglePass = nullptr;

            // Cache the pass request data for creating a hair parent pass
            AZ::Data::Asset<AZ::RPI::AnyAsset> m_parentPassRequestAsset;
        };
    } // namespace Render
} // namespace AZ
