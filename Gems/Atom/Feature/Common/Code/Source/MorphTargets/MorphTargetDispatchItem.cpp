/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <MorphTargets/MorphTargetDispatchItem.h>
#include <SkinnedMesh/SkinnedMeshFeatureProcessor.h>

#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Model/ModelLod.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Public/RPIUtils.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/BufferView.h>

#include <limits>

namespace AZ
{
    namespace Render
    {
        MorphTargetDispatchItem::MorphTargetDispatchItem(
            const AZStd::intrusive_ptr<MorphTargetInputBuffers> inputBuffers,
            const MorphTargetMetaData& morphTargetMetaData,
            SkinnedMeshFeatureProcessor* skinnedMeshFeatureProcessor,
            MorphTargetInstanceMetaData morphInstanceMetaData,
            float morphDeltaIntegerEncoding)
            : m_inputBuffers(inputBuffers)
            , m_morphTargetMetaData(morphTargetMetaData)
            , m_morphInstanceMetaData(morphInstanceMetaData)
            , m_accumulatedDeltaIntegerEncoding(morphDeltaIntegerEncoding)
        {
            m_morphTargetShader = skinnedMeshFeatureProcessor->GetMorphTargetShader();
            RPI::ShaderReloadNotificationBus::Handler::BusConnect(m_morphTargetShader->GetAssetId());
        }

        MorphTargetDispatchItem::~MorphTargetDispatchItem()
        {
            RPI::ShaderReloadNotificationBus::Handler::BusDisconnect();
        }

        bool MorphTargetDispatchItem::Init()
        {
            if (!m_morphTargetShader)
            {
                AZ_Error("MorphTargetDispatchItem", false, "Cannot initialize a MorphTargetDispatchItem with a null shader");
                return false;
            }

            AZ::RPI::ShaderOptionGroup shaderOptionGroup = m_morphTargetShader->CreateShaderOptionGroup();
            // In case there are several options you don't care about, it's good practice to initialize them with default values.
            shaderOptionGroup.SetUnspecifiedToDefaultValues();
            shaderOptionGroup.SetValue(AZ::Name("o_hasColorDeltas"), RPI::ShaderOptionValue{ m_morphTargetMetaData.m_hasColorDeltas });

            // Get the shader variant and instance SRG
            RPI::ShaderReloadNotificationBus::Handler::BusConnect(m_morphTargetShader->GetAssetId());
            const RPI::ShaderVariant& shaderVariant = m_morphTargetShader->GetVariant(shaderOptionGroup.GetShaderVariantId());

            if (!InitPerInstanceSRG())
            {
                return false;
            }

            if (!shaderVariant.IsFullyBaked() && m_instanceSrg->HasShaderVariantKeyFallbackEntry())
            {
                m_instanceSrg->SetShaderVariantKeyFallbackValue(shaderOptionGroup.GetShaderVariantKeyFallbackValue());
            }

            RHI::PipelineStateDescriptorForDispatch pipelineStateDescriptor;
            shaderVariant.ConfigurePipelineState(pipelineStateDescriptor);


            InitRootConstants(pipelineStateDescriptor.m_pipelineLayoutDescriptor->GetRootConstantsLayout());

            m_dispatchItem.m_pipelineState = m_morphTargetShader->AcquirePipelineState(pipelineStateDescriptor);

            // Get the threads-per-group values from the compute shader [numthreads(x,y,z)]
            auto& arguments = m_dispatchItem.m_arguments.m_direct;
            const auto outcome = RPI::GetComputeShaderNumThreads(m_morphTargetShader->GetAsset(), arguments);
            if (!outcome.IsSuccess())
            {
                AZ_Error("MorphTargetDispatchItem", false, outcome.GetError().c_str());
            }

            arguments.m_totalNumberOfThreadsX = m_morphTargetMetaData.m_vertexCount;
            arguments.m_totalNumberOfThreadsY = 1;
            arguments.m_totalNumberOfThreadsZ = 1;

            return true;
        }

        bool MorphTargetDispatchItem::InitPerInstanceSRG()
        {
            auto perInstanceSrgLayout = m_morphTargetShader->FindShaderResourceGroupLayout(AZ::Name{ "MorphTargetInstanceSrg" });
            if (!perInstanceSrgLayout)
            {
                AZ_Error("MorphTargetDispatchItem", false, "Failed to get shader resource group layout");
                return false;
            }

            m_instanceSrg = RPI::ShaderResourceGroup::Create(m_morphTargetShader->GetAsset(), m_morphTargetShader->GetSupervariantIndex(), perInstanceSrgLayout->GetName());
            if (!m_instanceSrg)
            {
                AZ_Error("MorphTargetDispatchItem", false, "Failed to create shader resource group for morph target");
                return false;
            }
            
            m_inputBuffers->SetBufferViewsOnShaderResourceGroup(m_instanceSrg);

            m_instanceSrg->Compile();

            m_dispatchItem.m_uniqueShaderResourceGroup = m_instanceSrg->GetRHIShaderResourceGroup();
            return true;
        }

        void MorphTargetDispatchItem::InitRootConstants(const RHI::ConstantsLayout* rootConstantsLayout)
        {
            auto vertexCountIndex = rootConstantsLayout->FindShaderInputIndex(AZ::Name{ "s_vertexCount" });
            AZ_Error("MorphTargetDispatchItem", vertexCountIndex.IsValid(), "Could not find root constant 's_vertexCount' in the shader");
            auto positionOffsetIndex = rootConstantsLayout->FindShaderInputIndex(AZ::Name{ "s_targetPositionOffset" });
            AZ_Error("MorphTargetDispatchItem", positionOffsetIndex.IsValid(), "Could not find root constant 's_targetPositionOffset' in the shader");
            auto normalOffsetIndex = rootConstantsLayout->FindShaderInputIndex(AZ::Name{ "s_targetNormalOffset" });
            AZ_Error("MorphTargetDispatchItem", normalOffsetIndex.IsValid(), "Could not find root constant 's_targetNormalOffset' in the shader");
            auto tangentOffsetIndex = rootConstantsLayout->FindShaderInputIndex(AZ::Name{ "s_targetTangentOffset" });
            AZ_Error("MorphTargetDispatchItem", tangentOffsetIndex.IsValid(), "Could not find root constant 's_targetTangentOffset' in the shader");
            auto bitangentOffsetIndex = rootConstantsLayout->FindShaderInputIndex(AZ::Name{ "s_targetBitangentOffset" });
            AZ_Error("MorphTargetDispatchItem", bitangentOffsetIndex.IsValid(), "Could not find root constant 's_targetBitangentOffset' in the shader");
            auto colorOffsetIndex = rootConstantsLayout->FindShaderInputIndex(AZ::Name{ "s_targetColorOffset" });
            AZ_Error("MorphTargetDispatchItem", colorOffsetIndex.IsValid(), "Could not find root constant 's_targetColorOffset' in the shader");

            auto minIndex = rootConstantsLayout->FindShaderInputIndex(AZ::Name{ "s_min" });
            AZ_Error("MorphTargetDispatchItem", minIndex.IsValid(), "Could not find root constant 's_min' in the shader");
            auto maxIndex = rootConstantsLayout->FindShaderInputIndex(AZ::Name{ "s_max" });
            AZ_Error("MorphTargetDispatchItem", maxIndex.IsValid(), "Could not find root constant 's_max' in the shader");
            auto morphDeltaIntegerEncodingIndex = rootConstantsLayout->FindShaderInputIndex(AZ::Name{ "s_accumulatedDeltaIntegerEncoding" });
            AZ_Error("MorphTargetDispatchItem", morphDeltaIntegerEncodingIndex.IsValid(), "Could not find root constant 's_accumulatedDeltaIntegerEncoding' in the shader");
            m_weightIndex = rootConstantsLayout->FindShaderInputIndex(AZ::Name{ "s_weight" });
            AZ_Error("MorphTargetDispatchItem", m_weightIndex.IsValid(), "Could not find root constant 's_weight' in the shader");

            m_rootConstantData = AZ::RHI::ConstantsData(rootConstantsLayout);
            m_rootConstantData.SetConstant(minIndex, m_morphTargetMetaData.m_minDelta);
            m_rootConstantData.SetConstant(maxIndex, m_morphTargetMetaData.m_maxDelta);
            m_rootConstantData.SetConstant(morphDeltaIntegerEncodingIndex, m_accumulatedDeltaIntegerEncoding);
            m_rootConstantData.SetConstant(m_weightIndex, 0.0f);
            m_rootConstantData.SetConstant(vertexCountIndex, m_morphTargetMetaData.m_vertexCount);
            // The buffer is using 32-bit integers, so divide the offset by 4 here so it doesn't have to be done in the shader
            m_rootConstantData.SetConstant(positionOffsetIndex, m_morphInstanceMetaData.m_accumulatedPositionDeltaOffsetInBytes / 4);
            m_rootConstantData.SetConstant(normalOffsetIndex, m_morphInstanceMetaData.m_accumulatedNormalDeltaOffsetInBytes / 4);
            m_rootConstantData.SetConstant(tangentOffsetIndex, m_morphInstanceMetaData.m_accumulatedTangentDeltaOffsetInBytes / 4);
            m_rootConstantData.SetConstant(bitangentOffsetIndex, m_morphInstanceMetaData.m_accumulatedBitangentDeltaOffsetInBytes / 4);

            if (m_morphTargetMetaData.m_hasColorDeltas)
            {
                m_rootConstantData.SetConstant(colorOffsetIndex, m_morphInstanceMetaData.m_accumulatedColorDeltaOffsetInBytes / 4);
            }

            m_dispatchItem.m_rootConstantSize = static_cast<uint8_t>(m_rootConstantData.GetConstantData().size());
            m_dispatchItem.m_rootConstants = m_rootConstantData.GetConstantData().data();
        }

        void MorphTargetDispatchItem::SetWeight(float weight)
        {
            m_rootConstantData.SetConstant(m_weightIndex, weight);
            m_dispatchItem.m_rootConstants = m_rootConstantData.GetConstantData().data();
        }

        float MorphTargetDispatchItem::GetWeight() const
        {
            return m_rootConstantData.GetConstant<float>(m_weightIndex);
        }

        const RHI::DispatchItem& MorphTargetDispatchItem::GetRHIDispatchItem() const
        {
            return m_dispatchItem;
        }

        void MorphTargetDispatchItem::OnShaderReinitialized([[maybe_unused]] const RPI::Shader& shader)
        {
            if (!Init())
            {
                AZ_Error("MorphTargetDispatchItem", false, "Failed to re-initialize after the shader was re-loaded.");
            }
        }

        void MorphTargetDispatchItem::OnShaderAssetReinitialized([[maybe_unused]] const Data::Asset<RPI::ShaderAsset>& shaderAsset)
        {
            if (!Init())
            {
                AZ_Error("MorphTargetDispatchItem", false, "Failed to re-initialize after the shader asset was re-loaded.");
            }
        }

        void MorphTargetDispatchItem::OnShaderVariantReinitialized(const RPI::ShaderVariant&)
        {
            if (!Init())
            {
                AZ_Error("MorphTargetDispatchItem", false, "Failed to re-initialize after the shader variant was loaded.");
            }
        }

        float ComputeMorphTargetIntegerEncoding(const AZStd::vector<MorphTargetMetaData>& morphTargetMetaDatas)
        {
            // The accumulation buffer must be stored as an int to support InterlockedAdd in AZSL
            // Conservatively determine the largest value, positive or negative across the entire skinned mesh lod, which is used for encoding/decoding the accumulation buffer
            float range = 0.0f;
            for (const MorphTargetMetaData& metaData : morphTargetMetaDatas)
            {
                float maxWeight = AZStd::max(std::abs(metaData.m_minWeight), std::abs(metaData.m_maxWeight));
                float maxDelta = AZStd::max(std::abs(metaData.m_minDelta), std::abs(metaData.m_maxDelta));
                // Normal, Tangent, and Bitangent deltas can be as high as 2
                maxDelta = AZStd::max(maxDelta, 2.0f);
                // Since multiple morphs can be fully active at once, sum the maximum offset in either positive or negative direction
                // that can be applied each individual morph to get the maximum offset that could be applied across all morphs
                range += maxWeight * maxDelta;
            }

            // Protect against divide-by-zero
            if (range < std::numeric_limits<float>::epsilon())
            {
                AZ_Assert(false, "MorphTargetDispatchItem - attempting to create a morph targets that have no min or max for the metadata");
                range = 1.0f;
            }

            // Given a conservative maximum value of a delta (minimum if negated), set a value for encoding a float as an integer that maximizes precision
            // while still being able to represent the entire range of possible offset values for this instance
            // For example, if at most all the deltas accumulated fell between a -1 and 1 range, we'd encode it as an integer by multiplying it by 2,147,483,647.
            // If the delta has a larger range, we multiply it by a smaller number, increasing the range of representable values but decreasing the precision
            return static_cast<float>(std::numeric_limits<int>::max()) / range;
        }
    } // namespace Render
} // namespace AZ
