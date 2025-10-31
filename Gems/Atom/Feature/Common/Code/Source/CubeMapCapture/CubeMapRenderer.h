/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Transform.h>
#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Pass/Specific/EnvironmentCubeMapPass.h>
#include <Atom/RPI.Public/Scene.h>

namespace AZ
{
    namespace Render
    {
        using RenderCubeMapCallback = AZStd::function<void(uint8_t* const* cubeMapTextureData, const RHI::Format cubeMapTextureFormat)>;

        // Mixin class that provides cubemap capture capability
        class CubeMapRenderer
        {
        public:
            // starts the cubemap render using the transform as the capture point
            void StartRender(RenderCubeMapCallback callback, const AZ::Transform& transform, float exposure);

            // called each frame, invokes the callback when rendering is complete
            void Update();

            // removes the render pipeline from the scene if rendering is complete
            // Note: must be called outside of the feature processor Simulate/Render phases
            void CheckAndRemovePipeline();

            bool IsRenderingCubeMap() const { return m_renderingCubeMap; }
            void SetScene(RPI::Scene* scene) { m_scene = scene; }
            void SetDefaultView(RPI::RenderPipeline* renderPipeline);

        protected:
            CubeMapRenderer() = default;
            ~CubeMapRenderer() = default;

        private:
            AZ_DISABLE_COPY_MOVE(CubeMapRenderer);

            RPI::Scene* m_scene = nullptr;
            float m_exposure = 0.0f;

            // render pipeline
            RPI::Ptr<RPI::EnvironmentCubeMapPass> m_environmentCubeMapPass = nullptr;
            RPI::RenderPipelineId m_environmentCubeMapPipelineId;
            RenderCubeMapCallback m_callback;
            RHI::ShaderInputNameIndex m_globalIblExposureConstantIndex = "m_iblExposure";
            RHI::ShaderInputNameIndex m_skyBoxExposureConstantIndex = "m_cubemapExposure";
            float m_previousGlobalIblExposure = 0.0f;
            float m_previousSkyBoxExposure = 0.0f;
            bool m_renderingCubeMap = false;
        };
    } // namespace Render
} // namespace AZ
