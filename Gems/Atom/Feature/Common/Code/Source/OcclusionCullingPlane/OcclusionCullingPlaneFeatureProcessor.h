/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/OcclusionCullingPlane/OcclusionCullingPlaneFeatureProcessorInterface.h>
#include <OcclusionCullingPlane/OcclusionCullingPlane.h>

namespace AZ
{
    namespace Render
    {
        //! This class manages OcclusionCullingPlanes which are used to cull meshes that are inside the view frustum
        class OcclusionCullingPlaneFeatureProcessor final
            : public OcclusionCullingPlaneFeatureProcessorInterface
        {
        public:
            AZ_CLASS_ALLOCATOR(OcclusionCullingPlaneFeatureProcessor, AZ::SystemAllocator)
            AZ_RTTI(AZ::Render::OcclusionCullingPlaneFeatureProcessor, "{C3DE91D7-EF7A-4A82-A55F-E22BC52074EA}", AZ::Render::OcclusionCullingPlaneFeatureProcessorInterface);

            static void Reflect(AZ::ReflectContext* context);

            OcclusionCullingPlaneFeatureProcessor() = default;
            virtual ~OcclusionCullingPlaneFeatureProcessor() = default;

            // OcclusionCullingPlaneFeatureProcessorInterface overrides
            OcclusionCullingPlaneHandle AddOcclusionCullingPlane(const AZ::Transform& transform) override;
            void RemoveOcclusionCullingPlane(OcclusionCullingPlaneHandle& handle) override;
            bool IsValidOcclusionCullingPlaneHandle(const OcclusionCullingPlaneHandle& occlusionCullingPlane) const override { return (occlusionCullingPlane.get() != nullptr); }
            void SetTransform(const OcclusionCullingPlaneHandle& occlusionCullingPlane, const AZ::Transform& transform) override;
            void SetEnabled(const OcclusionCullingPlaneHandle& occlusionCullingPlane, bool enable) override;
            void ShowVisualization(const OcclusionCullingPlaneHandle& occlusionCullingPlane, bool showVisualization) override;
            void SetTransparentVisualization(const OcclusionCullingPlaneHandle& occlusionCullingPlane, bool transparentVisualization) override;

            // FeatureProcessor overrides
            void Activate() override;
            void Deactivate() override;

            // RPI::SceneNotificationBus overrides ...
            void OnBeginPrepareRender() override;

            // retrieve the full list of occlusion planes
            using OcclusionCullingPlaneVector = AZStd::vector<AZStd::shared_ptr<OcclusionCullingPlane>>;
            OcclusionCullingPlaneVector& GetOcclusionCullingPlanes() { return m_occlusionCullingPlanes; }

        private:
            AZ_DISABLE_COPY_MOVE(OcclusionCullingPlaneFeatureProcessor);

            // list of occlusion planes
            const size_t InitialOcclusionCullingPlanesAllocationSize = 64;
            OcclusionCullingPlaneVector m_occlusionCullingPlanes;

            // prebuilt list of RPI scene occlusion planes
            RPI::CullingScene::OcclusionPlaneVector m_rpiOcclusionPlanes;
            bool m_rpiListNeedsUpdate = false;
        };
    } // namespace Render
} // namespace AZ
