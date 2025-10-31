/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SkinnedMesh/SkinnedMeshRenderProxy.h>
#include <SkinnedMesh/SkinnedMeshFeatureProcessor.h>
#include <SkinnedMesh/SkinnedMeshComputePass.h>
#include <Mesh/MeshFeatureProcessor.h>
#include <MorphTargets/MorphTargetComputePass.h>

#include <Atom/RPI.Public/Model/Model.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Shader/Shader.h>

#include <Atom/Utils/Utils.h>
namespace AZ
{
    namespace Render
    {
        SkinnedMeshRenderProxy::SkinnedMeshRenderProxy(const SkinnedMeshFeatureProcessorInterface::SkinnedMeshHandleDescriptor& desc)
            : m_inputBuffers(desc.m_inputBuffers)
            , m_instance(desc.m_instance)
            , m_meshHandle(desc.m_meshHandle)
            , m_boneTransforms(desc.m_boneTransforms)
            , m_shaderOptions(desc.m_shaderOptions)
        {
        }

        bool SkinnedMeshRenderProxy::Init(const RPI::Scene& scene, SkinnedMeshFeatureProcessor* featureProcessor)
        {
            AZ_PROFILE_FUNCTION(AzRender);
            if(!m_instance->m_model)
            {
                return false;
            }
            const size_t modelLodCount = m_instance->m_model->GetLodCount();
            m_featureProcessor = featureProcessor;

            for (uint32_t modelLodIndex = 0; modelLodIndex < modelLodCount; ++modelLodIndex)
            {
                if (!BuildDispatchItem(scene, modelLodIndex, m_shaderOptions))
                {
                    return false;
                }
            }
            
            return true;
        }

        bool SkinnedMeshRenderProxy::BuildDispatchItem([[maybe_unused]] const RPI::Scene& scene, uint32_t modelLodIndex, [[maybe_unused]] const SkinnedMeshShaderOptions& shaderOptions)
        {
            Data::Instance<RPI::Shader> skinningShader = m_featureProcessor->GetSkinningShader();
            if (!skinningShader)
            {
                AZ_Error("Skinned Mesh Feature Processor", false, "Failed to get skinning shader from skinning pass");
                return false;
            }

            // Get the data needed to create a morph target dispatch item
            Data::Instance<RPI::Shader> morphTargetShader = m_featureProcessor->GetMorphTargetShader();
            if (!morphTargetShader)
            {
                AZ_Error("Skinned Mesh Feature Processor", false, "Failed to get morph target shader from morph target pass");
                return false;
            }

            // Create a vector of dispatch items for each lod
            m_dispatchItemsByLod.emplace_back(AZStd::vector<AZStd::unique_ptr<SkinnedMeshDispatchItem>>());
            m_morphTargetDispatchItemsByLod.emplace_back(AZStd::vector<AZStd::unique_ptr<MorphTargetDispatchItem>>());

            size_t meshCount = m_inputBuffers->GetMeshCount(modelLodIndex);
            m_dispatchItemsByLod[modelLodIndex].reserve(meshCount);

            // Populate the vector with a dispatch item for each mesh
            for (uint32_t meshIndex = 0; meshIndex < meshCount; ++meshIndex)
            {            
                // Create the skinning dispatch Item
                m_dispatchItemsByLod[modelLodIndex].emplace_back(
                    aznew SkinnedMeshDispatchItem{
                        m_inputBuffers,
                        m_instance->m_outputStreamOffsetsInBytes[modelLodIndex][meshIndex],
                        m_instance->m_positionHistoryBufferOffsetsInBytes[modelLodIndex][meshIndex],
                        modelLodIndex, meshIndex, m_boneTransforms,
                        m_shaderOptions,
                        m_featureProcessor,
                        m_instance->m_morphTargetInstanceMetaData[modelLodIndex][meshIndex],
                        m_inputBuffers->GetMorphTargetIntegerEncoding(modelLodIndex, meshIndex)});
            }

            AZ_Assert(m_dispatchItemsByLod.size() == modelLodIndex + 1, "Skinned Mesh Feature Processor - Mismatch in size between the fixed vector of dispatch items and the lod being initialized");
            for (size_t dispatchIndex = 0; dispatchIndex < m_dispatchItemsByLod[modelLodIndex].size(); ++dispatchIndex)
            {
                if (!m_dispatchItemsByLod[modelLodIndex][dispatchIndex]->Init())
                {
                    return false;
                }
            }

            size_t morphTargetCount = m_inputBuffers->GetMorphTargetInputBuffers(modelLodIndex).size();
            AZ_Assert(
                m_inputBuffers->GetMorphTargetComputeMetaDatas(modelLodIndex).size() == morphTargetCount,
                "SkinnedMeshRenderProxy: Invalid SkinnedMeshInputBuffers have mis-matched morph target input buffers and compute metadata");
            m_morphTargetDispatchItemsByLod[modelLodIndex].reserve(morphTargetCount);

            // Create one dispatch item per morph target, in the order that they were originally added
            // to the skinned mesh to stay in sync with the animation system
            for (size_t morphTargetIndex = 0; morphTargetIndex < morphTargetCount; ++morphTargetIndex)
            {
                const MorphTargetComputeMetaData& metaData =
                    m_inputBuffers->GetMorphTargetComputeMetaDatas(modelLodIndex)[morphTargetIndex];

                m_morphTargetDispatchItemsByLod[modelLodIndex].emplace_back(
                    aznew MorphTargetDispatchItem
                    {
                        m_inputBuffers->GetMorphTargetInputBuffers(modelLodIndex)[morphTargetIndex],
                        metaData,
                        m_featureProcessor,
                        m_instance->m_morphTargetInstanceMetaData[modelLodIndex][metaData.m_meshIndex],
                        m_inputBuffers->GetMorphTargetIntegerEncoding(modelLodIndex, metaData.m_meshIndex)
                    });

                // Initialize the MorphTargetDispatchItem we just created
                if (!m_morphTargetDispatchItemsByLod[modelLodIndex].back()->Init())
                {
                    return false;
                }
            }
            
            return true;
        }

        void SkinnedMeshRenderProxy::SetSkinningMatrices(const AZStd::vector<float>& data)
        {
            if (m_boneTransforms)
            {
                m_boneTransforms->UpdateData(data.data(), data.size() * sizeof(float));
            }
        }

        void SkinnedMeshRenderProxy::SetMorphTargetWeights(uint32_t lodIndex, const AZStd::vector<float>& weights)
        {
            auto& morphTargetDispatchItems = m_morphTargetDispatchItemsByLod[lodIndex];

            AZ_Assert(morphTargetDispatchItems.size() == weights.size(), "Skinned Mesh Feature Processor - Morph target weights passed into SetMorphTargetWeight don't align with morph target dispatch items.");
            for (size_t morphIndex = 0; morphIndex < weights.size(); ++morphIndex)
            {
                morphTargetDispatchItems[morphIndex]->SetWeight(weights[morphIndex]);
            }
        }

        void SkinnedMeshRenderProxy::EnableSkinning(uint32_t lodIndex, uint32_t meshIndex)
        {
            m_dispatchItemsByLod[lodIndex][meshIndex]->Enable();
        }

        void SkinnedMeshRenderProxy::DisableSkinning(uint32_t lodIndex, uint32_t meshIndex)
        {
            m_dispatchItemsByLod[lodIndex][meshIndex]->Disable();
        }

        uint32_t SkinnedMeshRenderProxy::GetLodCount() const
        {
            return aznumeric_caster(m_dispatchItemsByLod.size());
        }

        AZStd::span<const AZStd::unique_ptr<SkinnedMeshDispatchItem>> SkinnedMeshRenderProxy::GetDispatchItems(uint32_t lodIndex) const
        {
            return m_dispatchItemsByLod[lodIndex];
        }

    } // namespace Render
} // namespace AZ
