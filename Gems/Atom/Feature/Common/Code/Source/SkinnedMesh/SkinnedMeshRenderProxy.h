/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SkinnedMesh/SkinnedMeshDispatchItem.h>
#include <MorphTargets/MorphTargetDispatchItem.h>
#include <Atom/Feature/SkinnedMesh/SkinnedMeshRenderProxyInterface.h>
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
            : public SkinnedMeshRenderProxyInterface
        {
            friend SkinnedMeshFeatureProcessor;

        public:
            AZ_RTTI(AZ::Render::SkinnedMeshRenderProxy, "{C77A21E7-113A-4DC9-972F-923E1BEFBC9A}", AZ::Render::SkinnedMeshRenderProxyInterface);
            explicit SkinnedMeshRenderProxy(const SkinnedMeshFeatureProcessorInterface::SkinnedMeshRenderProxyDesc& desc);

            void SetTransform(const Transform& transform) override;
            void SetSkinningMatrices(const AZStd::vector<float>& data) override;
            void SetMorphTargetWeights(uint32_t lodIndex, const AZStd::vector<float>& weights) override;

            AZStd::array_view< AZStd::unique_ptr<SkinnedMeshDispatchItem>> GetDispatchItems() const;
        private:

            AZ_DISABLE_COPY_MOVE(SkinnedMeshRenderProxy);

            bool Init(const RPI::Scene& scene, SkinnedMeshFeatureProcessor* featureProcessor);
            bool BuildDispatchItem(const RPI::Scene& scene, size_t modelLodIndex, const SkinnedMeshShaderOptions& shaderOptions);

            Vector3 m_position = Vector3(0.0f, 0.0f, 0.0f); //!< Cached position so SkinnedMeshFeatureProcessor can make faster LOD calculations
            AZStd::fixed_vector<AZStd::unique_ptr<SkinnedMeshDispatchItem>, RPI::ModelLodAsset::LodCountMax> m_dispatchItemsByLod;
            AZStd::fixed_vector<AZStd::vector<AZStd::unique_ptr<MorphTargetDispatchItem>>, RPI::ModelLodAsset::LodCountMax> m_morphTargetDispatchItemsByLod;
            Data::Instance<SkinnedMeshInputBuffers> m_inputBuffers;
            Data::Instance<MorphTargetInputBuffers> m_morphTargetInputBuffers;
            AZStd::intrusive_ptr<SkinnedMeshInstance> m_instance;
            AZStd::shared_ptr<MeshFeatureProcessorInterface::MeshHandle> m_meshHandle;
            SkinnedMeshShaderOptions m_shaderOptions;

            Data::Instance<RPI::Buffer> m_boneTransforms;

            SkinnedMeshFeatureProcessor* m_featureProcessor = nullptr;
            bool m_isQueuedForCompile = false;
        };

        using SkinnedMeshRenderProxyHandle = StableDynamicArrayHandle<SkinnedMeshRenderProxy>;
    } // namespace Render
} // namespace AZ
