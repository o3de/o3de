/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/FeatureProcessor.h>

namespace AZ::RPI
{
    class Pass;
    class RasterPass;
}

namespace AZ::Render
{
    // Adds a silhouette gather pass and a silhouette full screen pass for drawing and
    // blocking silhouettes
    // The gather pass draws silhoutte meshes that use the "Silhouette" Material type
    // to a render target, using the depth and stencil buffer to determine where to draw
    // The silhouette full screen pass then composites the render target with all the 
    // silhouettes into the framebuffer and adds an outline.
    class SilhouetteFeatureProcessor final
        : public AZ::RPI::FeatureProcessor
    {
    public:
        AZ_CLASS_ALLOCATOR(SilhouetteFeatureProcessor, AZ::SystemAllocator)

        AZ_RTTI(AZ::Render::SilhouetteFeatureProcessor, "{E32ABBE6-2472-4404-AEDB-1CE7A12E7C43}", AZ::RPI::FeatureProcessor);

        static void Reflect(AZ::ReflectContext* context);

        SilhouetteFeatureProcessor();
        ~SilhouetteFeatureProcessor() override;

        //! Set the enabled state of the gather and composite passes
        //! @param enabled Whether to enable the passes or not
        void SetPassesEnabled(bool enabled);

        //! FeatureProcessor 
        void Activate() override;
        void Deactivate() override;
        void AddRenderPasses(AZ::RPI::RenderPipeline* renderPipeline) override;

        //! RPI::SceneNotificationBus
        void OnRenderEnd() override;
        void OnRenderPipelineChanged(AZ::RPI::RenderPipeline* pipeline, AZ::RPI::SceneNotification::RenderPipelineChangeType changeType) override;

        bool NeedsCompositePass() const;

    private:
        void UpdatePasses(AZ::RPI::RenderPipeline* renderPipeline);

        AZ::RPI::RasterPass* m_rasterPass = nullptr;
        AZ::RPI::Pass* m_compositePass = nullptr;
        AZ::RPI::RenderPipeline* m_renderPipeline = nullptr;
    };
}

