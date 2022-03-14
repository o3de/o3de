/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/Scene.h>
#include <CubeMapCapture/CubeMapRenderer.h>

namespace AZ
{
    namespace Render
    {
        // captures a cubeMap using the specified transform as the capture position
        class CubeMapCapture final
            : private CubeMapRenderer
        {
        public:
            CubeMapCapture() = default;
            ~CubeMapCapture() = default;

            void Init(RPI::Scene* scene);
            void Simulate();
            void OnRenderEnd();

            // initiates the cubeMap render and invokes the callback after all of the faces are rendered
            void RenderCubeMap(RenderCubeMapCallback callback, const AZStd::string& relativePath);

            // called by the feature processor, sets the default view if it's for the cubeMap capture pipeline
            void OnRenderPipelinePassesChanged(RPI::RenderPipeline* renderPipeline);

            void SetRelativePath(const AZStd::string& relativePath) { m_relativePath = relativePath; }
            const AZStd::string& GetRelativePath() const { return m_relativePath; }

            void SetTransform(const AZ::Transform& transform);
            void SetExposure(float exposure);

        private:
            AZ_DISABLE_COPY_MOVE(CubeMapCapture);

            RPI::Scene* m_scene = nullptr;
            AZ::Transform m_transform = AZ::Transform::CreateIdentity();
            AZStd::string m_relativePath;
            float m_exposure = 0.0f;
        };
    } // namespace Render
} // namespace AZ
