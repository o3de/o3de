/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/Mesh/MeshFeatureProcessorInterface.h>
#include <Atom/Feature/Mesh/MeshInfo.h>
#include <Atom/Feature/Mesh/MeshInfoBus.h>
#include <Atom/Feature/ReflectionProbe/ReflectionProbeFeatureProcessorInterface.h>
#include <Atom/Feature/TransformService/TransformServiceFeatureProcessorInterface.h>
#include <Atom/RPI.Public/Buffer/BufferSystemInterface.h>
#include <Atom/RPI.Public/Buffer/RingBuffer.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Public/Model/Model.h>
#include <Atom/RPI.Public/Scene.h>

namespace AZ::Render
{
    namespace FallbackPBR
    {
        class MaterialManager : public MeshInfoNotificationBus::Handler
        {
        public:
            MaterialManager();
            void Activate(RPI::Scene* scene);
            void Deactivate();
            void Update();

            // --------------------------------------------------------------------------------------
            // MeshInfoNotificationBus::Handler overrides
            void OnAcquireMeshInfoEntry(const MeshInfoHandle meshInfoHandle) override;
            void OnReleaseMeshInfoEntry(const MeshInfoHandle meshInfoHandle) override;
            void OnPopulateMeshInfoEntry(
                const MeshInfoHandle meshInfoHandle,
                ModelDataInstanceInterface* modelData,
                const size_t lodIndex,
                const size_t lodMeshIndex) override;
            // --------------------------------------------------------------------------------------

            // integration with the MeshFeatureProcessor
            const RHI::Ptr<MaterialEntry>& GetFallbackPBRMaterialEntry(const MeshInfoHandle handle) const;
            void UpdateFallbackPBRMaterialEntry(const MeshInfoHandle handle, AZStd::function<bool(MaterialEntry*)> updateFunction);
            const Data::Instance<RPI::Buffer>& GetFallbackPBRMaterialBuffer() const;

            void UpdateReflectionProbes(const TransformServiceFeatureProcessorInterface::ObjectId& objectId, const Aabb& aabbWS);

        private:
            bool m_isEnabled = false;

            void UpdateFallbackPBRMaterial();
            void UpdateFallbackPBRMaterialBuffer();

            void ConvertMaterial(RPI::Material* material, MaterialParameters& convertedMaterial);

            // reflection probe
            struct ReflectionProbe
            {
                AZ::Transform m_modelToWorld;
                AZ::Vector3 m_outerObbHalfLengths;
                AZ::Vector3 m_innerObbHalfLengths;
                bool m_useParallaxCorrection = false;
                float m_exposure = 0.0f;

                Data::Instance<RPI::Image> m_reflectionProbeCubeMap;
            };
            AZStd::unordered_map<TransformServiceFeatureProcessorInterface::ObjectId, ReflectionProbe> m_reflectionProbeData;

            RHI::Ptr<MaterialEntry> m_emptyEntry{ nullptr };
            AZStd::vector<RHI::Ptr<MaterialEntry>> m_materialData;
            RPI::RingBuffer m_materialDataBuffer;
            bool m_bufferNeedsUpdate;

            RHI::ShaderInputNameIndex m_fallbackPBRMaterialIndex = "m_fallbackPBRMaterial";
            RPI::Scene::PrepareSceneSrgEvent::Handler m_updateSceneSrgHandler;
            ReflectionProbeFeatureProcessorInterface* m_rpfp = nullptr;
        };
    } // namespace FallbackPBR
} // namespace AZ::Render