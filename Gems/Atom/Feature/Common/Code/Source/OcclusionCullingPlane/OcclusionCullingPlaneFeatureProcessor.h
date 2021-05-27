/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <Atom/Feature/OcclusionCullingPlane/OcclusionCullingPlaneFeatureProcessorInterface.h>

namespace AZ
{
    namespace Render
    {
        //! This class represents an OcclusionCullingPlane which is used to cull meshes that are inside the view frustum
        class OcclusionCullingPlane final
        {
        public:
            OcclusionCullingPlane() = default;
            ~OcclusionCullingPlane() = default;

            void SetTransform(const AZ::Transform& transform) { m_transform = transform; }
            const AZ::Transform& GetTransform() const { return m_transform; }

            void SetEnabled(bool enabled) { m_enabled = enabled; }
            bool GetEnabled() const { return m_enabled; }

        private:
            AZ::Transform m_transform;
            bool m_enabled = true;
        };

        //! This class manages OcclusionCullingPlanes which are used to cull meshes that are inside the view frustum
        class OcclusionCullingPlaneFeatureProcessor final
            : public OcclusionCullingPlaneFeatureProcessorInterface
        {
        public:
            AZ_RTTI(AZ::Render::OcclusionCullingPlaneFeatureProcessor, "{C3DE91D7-EF7A-4A82-A55F-E22BC52074EA}", OcclusionCullingPlaneFeatureProcessorInterface);

            static void Reflect(AZ::ReflectContext* context);

            OcclusionCullingPlaneFeatureProcessor() = default;
            virtual ~OcclusionCullingPlaneFeatureProcessor() = default;

            // OcclusionCullingPlaneFeatureProcessorInterface overrides
            OcclusionCullingPlaneHandle AddOcclusionCullingPlane(const AZ::Transform& transform) override;
            void RemoveOcclusionCullingPlane(OcclusionCullingPlaneHandle& handle) override;
            bool IsValidOcclusionCullingPlaneHandle(const OcclusionCullingPlaneHandle& occlusionCullingPlane) const override { return (occlusionCullingPlane.get() != nullptr); }
            void SetTransform(const OcclusionCullingPlaneHandle& occlusionCullingPlane, const AZ::Transform& transform) override;
            void SetEnabled(const OcclusionCullingPlaneHandle& occlusionCullingPlane, bool enable) override;

            // FeatureProcessor overrides
            void Activate() override;
            void Deactivate() override;
            void Simulate(const FeatureProcessor::SimulatePacket& packet) override;

            // retrieve the full list of occlusion planes
            using OcclusionCullingPlaneVector = AZStd::vector<AZStd::shared_ptr<OcclusionCullingPlane>>;
            OcclusionCullingPlaneVector& GetOcclusionCullingPlanes() { return m_occlusionCullingPlanes; }

        private:
            AZ_DISABLE_COPY_MOVE(OcclusionCullingPlaneFeatureProcessor);

            // list of occlusion planes
            const size_t InitialOcclusionCullingPlanesAllocationSize = 64;
            OcclusionCullingPlaneVector m_occlusionCullingPlanes;
        };
    } // namespace Render
} // namespace AZ
