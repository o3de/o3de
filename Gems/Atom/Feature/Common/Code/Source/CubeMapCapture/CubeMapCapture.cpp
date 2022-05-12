/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CubeMapCapture/CubeMapCapture.h>
#include <CubeMapCapture/CubeMapCaptureFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        void CubeMapCapture::Init(RPI::Scene* scene)
        {
            AZ_Assert(scene, "CubeMapCapture::Init called with a null Scene pointer");
            m_scene = scene;

            CubeMapRenderer::SetScene(m_scene);
        }

        void CubeMapCapture::Simulate()
        {
            CubeMapRenderer::Update();
        }

        void CubeMapCapture::OnRenderEnd()
        {
            CubeMapRenderer::CheckAndRemovePipeline();
        }

        void CubeMapCapture::SetTransform(const AZ::Transform& transform)
        {
            m_transform = transform;
        }

        void CubeMapCapture::RenderCubeMap(RenderCubeMapCallback callback, const AZStd::string& relativePath)
        {
            m_relativePath = relativePath;

            CubeMapRenderer::StartRender(callback, m_transform, m_exposure);
        }

        void CubeMapCapture::OnRenderPipelinePassesChanged(RPI::RenderPipeline* renderPipeline)
        {
            CubeMapRenderer::SetDefaultView(renderPipeline);
        }

        void CubeMapCapture::SetExposure(float exposure)
        {
            m_exposure = exposure;
        }
    } // namespace Render
} // namespace AZ
