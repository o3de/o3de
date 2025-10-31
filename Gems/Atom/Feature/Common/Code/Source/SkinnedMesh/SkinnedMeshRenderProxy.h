/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SkinnedMesh/SkinnedMeshDispatchItem.h>
#include <MorphTargets/MorphTargetDispatchItem.h>
#include <Atom/Feature/SkinnedMesh/SkinnedMeshFeatureProcessorInterface.h>
#include <Atom/Feature/SkinnedMesh/SkinnedMeshShaderOptions.h>

#include <Atom/RPI.Reflect/Model/ModelLodAsset.h>
#include <Atom/Utils/StableDynamicArray.h>

#include <AzCore/base.h>
#include <AzCore/Math/Vector3.h>

namespace AZ
{
    namespace RPI
    {
        class Model;
        class Scene;
    }

    namespace Render
    {
        class SkinnedMeshFeatureProcessor;

        class SkinnedMeshRenderProxy final
        {
            friend SkinnedMeshFeatureProcessor;

        public:
            AZ_RTTI(AZ::Render::SkinnedMeshRenderProxy, "{C77A21E7-113A-4DC9-972F-923E1BEFBC9A}");
            explicit SkinnedMeshRenderProxy(const SkinnedMeshFeatureProcessorInterface::SkinnedMeshHandleDescriptor& desc);

            void SetSkinningMatrices(const AZStd::vector<float>& data);
            void SetMorphTargetWeights(uint32_t lodIndex, const AZStd::vector<float>& weights);
            void EnableSkinning(uint32_t lodIndex, uint32_t meshIndex);
            void DisableSkinning(uint32_t lodIndex, uint32_t meshIndex);

            uint32_t GetLodCount() const;
            AZStd::span<const AZStd::unique_ptr<SkinnedMeshDispatchItem>> GetDispatchItems(uint32_t lodIndex) const;
        private:

            AZ_DISABLE_COPY_MOVE(SkinnedMeshRenderProxy);

            bool Init(const RPI::Scene& scene, SkinnedMeshFeatureProcessor* featureProcessor);
            bool BuildDispatchItem(const RPI::Scene& scene, uint32_t modelLodIndex, const SkinnedMeshShaderOptions& shaderOptions);

            AZStd::fixed_vector<AZStd::vector<AZStd::unique_ptr<SkinnedMeshDispatchItem>>, RPI::ModelLodAsset::LodCountMax> m_dispatchItemsByLod;
            AZStd::fixed_vector<AZStd::vector<AZStd::unique_ptr<MorphTargetDispatchItem>>, RPI::ModelLodAsset::LodCountMax> m_morphTargetDispatchItemsByLod;
            Data::Instance<SkinnedMeshInputBuffers> m_inputBuffers;
            AZStd::intrusive_ptr<SkinnedMeshInstance> m_instance;
            AZStd::shared_ptr<MeshFeatureProcessorInterface::MeshHandle> m_meshHandle;
            SkinnedMeshShaderOptions m_shaderOptions;

            Data::Instance<RPI::Buffer> m_boneTransforms;

            SkinnedMeshFeatureProcessor* m_featureProcessor = nullptr;
        };
    } // namespace Render
} // namespace AZ
