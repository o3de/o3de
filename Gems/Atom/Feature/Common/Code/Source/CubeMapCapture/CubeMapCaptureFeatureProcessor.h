/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/CubeMapCapture/CubeMapCaptureFeatureProcessorInterface.h>
#include <CubeMapCapture/CubeMapCapture.h>

namespace AZ
{
    namespace Render
    {
        class CubeMapCaptureFeatureProcessor final
            : public CubeMapCaptureFeatureProcessorInterface
        {
        public:
            AZ_CLASS_ALLOCATOR(CubeMapCaptureFeatureProcessor, AZ::SystemAllocator)
            AZ_RTTI(AZ::Render::CubeMapCaptureFeatureProcessor, "{821039A3-AF40-4E69-A7EF-D44D81EAF1FA}", AZ::Render::CubeMapCaptureFeatureProcessorInterface);

            static void Reflect(AZ::ReflectContext* context);

            CubeMapCaptureFeatureProcessor() = default;
            virtual ~CubeMapCaptureFeatureProcessor() = default;

            // CubeMapCaptureFeatureProcessorInterface overrides
            CubeMapCaptureHandle AddCubeMapCapture(const AZ::Transform& transform) override;
            void RemoveCubeMapCapture(CubeMapCaptureHandle& cubeMapCapture) override;
            void SetTransform(const CubeMapCaptureHandle& cubeMapCapture, const AZ::Transform& transform) override;
            void SetExposure(const CubeMapCaptureHandle& cubeMapCapture, float exposure) override;
            void SetRelativePath(const CubeMapCaptureHandle& cubeMapCapture, const AZStd::string& relativePath) override;
            void RenderCubeMap(const CubeMapCaptureHandle& cubeMapCapture, RenderCubeMapCallback callback, const AZStd::string& relativePath) override;
            bool IsCubeMapReferenced(const AZStd::string& relativePath) override;

            // FeatureProcessor overrides
            void Activate() override;
            void Deactivate() override;
            void Simulate(const FeatureProcessor::SimulatePacket& packet) override;
            void OnRenderEnd() override;

        private:
            AZ_DISABLE_COPY_MOVE(CubeMapCaptureFeatureProcessor);

            // RPI::SceneNotificationBus::Handler overrides
            void OnRenderPipelineChanged(RPI::RenderPipeline* renderPipeline, RPI::SceneNotification::RenderPipelineChangeType changeType) override;

            // list of CubeMapCaptures
            using CubeMapCaptureVector = AZStd::vector<AZStd::shared_ptr<CubeMapCapture>>;
            const size_t InitialProbeAllocationSize = 64;
            CubeMapCaptureVector m_cubeMapCaptures;
        };
    } // namespace Render
} // namespace AZ
