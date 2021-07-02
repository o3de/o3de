/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SkinnedMesh/SkinnedMeshRenderProxy.h>
#include <SkinnedMesh/SkinnedMeshFeatureProcessor.h>
#include <SkinnedMesh/SkinnedMeshComputePass.h>
#include <MorphTargets/MorphTargetComputePass.h>

#include <Atom/Feature/Mesh/MeshFeatureProcessor.h>

#include <Atom/RPI.Public/Model/Model.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Shader/Shader.h>

#include <Atom/Utils/Utils.h>
#include <AzCore/Debug/EventTrace.h>

namespace AZ
{
    namespace Render
    {
        SkinnedMeshRenderProxy::SkinnedMeshRenderProxy(const SkinnedMeshFeatureProcessorInterface::SkinnedMeshRenderProxyDesc& desc)
            : m_inputBuffers(desc.m_inputBuffers)
            , m_instance(desc.m_instance)
            , m_meshHandle(desc.m_meshHandle)
            , m_boneTransforms(desc.m_boneTransforms)
            , m_shaderOptions(desc.m_shaderOptions)
        {
        }

        bool SkinnedMeshRenderProxy::Init(const RPI::Scene& scene, SkinnedMeshFeatureProcessor* featureProcessor)
        {
            AZ_TRACE_METHOD();
            if(!m_instance->m_model)
            {
                return false;
            }
            const size_t modelLodCount = m_instance->m_model->GetLodCount();
            m_featureProcessor = featureProcessor;

            for (size_t modelLodIndex = 0; modelLodIndex < modelLodCount; ++modelLodIndex)
            {
                if (!BuildDispatchItem(scene, modelLodIndex, m_shaderOptions))
                {
                    return false;
                }
            }
            
            return true;
        }

        bool SkinnedMeshRenderProxy::BuildDispatchItem([[maybe_unused]] const RPI::Scene& scene, size_t modelLodIndex, [[maybe_unused]] const SkinnedMeshShaderOptions& shaderOptions)
        {
            Data::Instance<RPI::Shader> skinningShader = m_featureProcessor->GetSkinningShader();
            if (!skinningShader)
            {
                AZ_Error("Skinned Mesh Feature Processor", false, "Failed to get skinning shader from skinning pass");
                return false;
            }

            // Get the value needed for encoding/decoding floats as integers when passing them
            // from the morph target pass to the skinning pass.
            // Integer encoding is used so that AZSL's InterlockedAdd can be used, wich only supports int/uint
            const AZStd::vector<MorphTargetMetaData>& morphTargetMetaDatas = m_inputBuffers->GetMorphTargetMetaData(modelLodIndex);
            float morphDeltaIntegerEncoding = 0.0f;
            if (morphTargetMetaDatas.size() > 0)
            {
                morphDeltaIntegerEncoding = ComputeMorphTargetIntegerEncoding(morphTargetMetaDatas);
            }

            m_dispatchItemsByLod.emplace_back(
                aznew SkinnedMeshDispatchItem{
                    m_inputBuffers,
                    m_instance->m_outputStreamOffsetsInBytes[modelLodIndex],
                    modelLodIndex, m_boneTransforms,
                    m_shaderOptions,
                    m_featureProcessor,
                    m_instance->m_morphTargetInstanceMetaData[modelLodIndex],
                    morphDeltaIntegerEncoding });

            AZ_Assert(m_dispatchItemsByLod.size() == modelLodIndex + 1, "Skinned Mesh Feature Processor - Mismatch in size between the fixed vector of dispatch items and the lod being initialized");
            if (!m_dispatchItemsByLod[modelLodIndex]->Init())
            {
                return false;
            }

            // Get the data needed to create a morph target dispatch item
            Data::Instance<RPI::Shader> morphTargetShader = m_featureProcessor->GetMorphTargetShader();
            const AZStd::vector<AZStd::intrusive_ptr<MorphTargetInputBuffers>>& morphTargetInputBuffersVector = m_inputBuffers->GetMorphTargetInputBuffers(modelLodIndex);
            AZ_Assert(morphTargetMetaDatas.size() == morphTargetInputBuffersVector.size(), "Skinned Mesh Feature Processor - Mismatch in morph target metadata count and morph target input buffer count");

            if (!morphTargetShader && morphTargetMetaDatas.size() > 0)
            {
                AZ_Error("Skinned Mesh Feature Processor", false, "Failed to get morph target shader from morph target pass");
                return false;
            }

            // Create one dispatch item per morph target
            m_morphTargetDispatchItemsByLod.emplace_back(AZStd::vector<AZStd::unique_ptr<MorphTargetDispatchItem>>());
            for (size_t morphTargetIndex = 0; morphTargetIndex < morphTargetMetaDatas.size(); ++morphTargetIndex)
            {
                m_morphTargetDispatchItemsByLod[modelLodIndex].emplace_back(
                    aznew MorphTargetDispatchItem{
                        morphTargetInputBuffersVector[morphTargetIndex],
                        morphTargetMetaDatas[morphTargetIndex],
                        m_featureProcessor,
                        m_instance->m_morphTargetInstanceMetaData[modelLodIndex],
                        morphDeltaIntegerEncoding });

                // Initialize the MorphTargetDispatchItem we just created
                if (!m_morphTargetDispatchItemsByLod[modelLodIndex].back()->Init())
                {
                    return false;
                }
            }
            
            return true;
        }

        void SkinnedMeshRenderProxy::SetTransform(const AZ::Transform& transform)
        {
            // Set the position to be used for determining lod
            m_position = transform.GetTranslation();
        }

        void SkinnedMeshRenderProxy::SetSkinningMatrices(const AZStd::vector<float>& data)
        {
            if (m_boneTransforms)
            {
                WriteToBuffer(m_boneTransforms->GetRHIBuffer(), data);
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

        AZStd::array_view<AZStd::unique_ptr<SkinnedMeshDispatchItem>> SkinnedMeshRenderProxy::GetDispatchItems() const
        {
            return m_dispatchItemsByLod;
        }

    } // namespace Render
} // namespace AZ
