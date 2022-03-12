/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SkinnedMesh/SkinnedMeshDispatchItem.h>
#include <SkinnedMesh/SkinnedMeshOutputStreamManager.h>
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
        SkinnedMeshDispatchItem::SkinnedMeshDispatchItem(
            AZStd::intrusive_ptr<SkinnedMeshInputBuffers> inputBuffers,
            const AZStd::vector<uint32_t>& outputBufferOffsetsInBytes,
            size_t lodIndex,
            Data::Instance<RPI::Buffer> boneTransforms,
            const SkinnedMeshShaderOptions& shaderOptions,
            SkinnedMeshFeatureProcessor* skinnedMeshFeatureProcessor,
            MorphTargetInstanceMetaData morphTargetInstanceMetaData,
            float morphTargetDeltaIntegerEncoding)
            : m_inputBuffers(inputBuffers)
            , m_outputBufferOffsetsInBytes(outputBufferOffsetsInBytes)
            , m_lodIndex(lodIndex)
            , m_boneTransforms(AZStd::move(boneTransforms))
            , m_shaderOptions(shaderOptions)
            , m_morphTargetInstanceMetaData(morphTargetInstanceMetaData)
            , m_morphTargetDeltaIntegerEncoding(morphTargetDeltaIntegerEncoding)
        {
            m_skinningShader = skinnedMeshFeatureProcessor->GetSkinningShader();

            // Shader options are generally set per-skinned mesh instance, but morph targets may only exist on some lods. Override the option for applying morph targets here
            if (m_morphTargetInstanceMetaData.m_accumulatedPositionDeltaOffsetInBytes != MorphTargetConstants::s_invalidDeltaOffset)
            {
                m_shaderOptions.m_applyMorphTargets = true;
            }
            if (inputBuffers->GetLod(lodIndex).HasDynamicColors())
            {
                m_shaderOptions.m_applyColorMorphTargets = true;
            }

            // CreateShaderOptionGroup will also connect to the SkinnedMeshShaderOptionNotificationBus
            m_shaderOptionGroup = skinnedMeshFeatureProcessor->CreateSkinningShaderOptionGroup(m_shaderOptions, *this);
        }

        SkinnedMeshDispatchItem::~SkinnedMeshDispatchItem()
        {
            SkinnedMeshShaderOptionNotificationBus::Handler::BusDisconnect();
        }

        bool SkinnedMeshDispatchItem::Init()
        {
            if (!m_skinningShader)
            {
                AZ_Error("SkinnedMeshDispatchItem", false, "Cannot initialize a SkinnedMeshDispatchItem with a null shader");
                return false;
            }

            // Get the shader variant and instance SRG
            m_shaderOptionGroup.SetUnspecifiedToDefaultValues();
            const RPI::ShaderVariant& shaderVariant = m_skinningShader->GetVariant(m_shaderOptionGroup.GetShaderVariantId());

            RHI::PipelineStateDescriptorForDispatch pipelineStateDescriptor;
            shaderVariant.ConfigurePipelineState(pipelineStateDescriptor);

            auto perInstanceSrgLayout = m_skinningShader->FindShaderResourceGroupLayout(AZ::Name{ "InstanceSrg" });
            if (!perInstanceSrgLayout)
            {
                AZ_Error("SkinnedMeshDispatchItem", false, "Failed to get shader resource group layout");
                return false;
            }

            m_instanceSrg = RPI::ShaderResourceGroup::Create(m_skinningShader->GetAsset(), m_skinningShader->GetSupervariantIndex(), perInstanceSrgLayout->GetName());
            if (!m_instanceSrg)
            {
                AZ_Error("SkinnedMeshDispatchItem", false, "Failed to create shader resource group for skinned mesh");
                return false;
            }

            // If the shader variation is not fully baked, set the fallback key to use a runtime branch for the shader options
            if (!shaderVariant.IsFullyBaked() && m_instanceSrg->HasShaderVariantKeyFallbackEntry())
            {
                m_instanceSrg->SetShaderVariantKeyFallbackValue(m_shaderOptionGroup.GetShaderVariantKeyFallbackValue());
            }

            m_inputBuffers->SetBufferViewsOnShaderResourceGroup(m_lodIndex, m_instanceSrg);

            // Set the SRG indices
            RHI::ShaderInputBufferIndex actorInstanceBoneTransformsIndex;
            if (m_shaderOptions.m_skinningMethod == SkinningMethod::LinearSkinning)
            {
                actorInstanceBoneTransformsIndex = m_instanceSrg->FindShaderInputBufferIndex(Name{ "m_boneTransformsLinear" });
                if (!actorInstanceBoneTransformsIndex.IsValid())
                {
                    AZ_Error("SkinnedMeshDispatchItem", false, "Failed to find shader input index for m_boneTransformsLinear in the skinning compute shader per-instance SRG.");
                    return false;
                }
            }
            else if(m_shaderOptions.m_skinningMethod == SkinningMethod::DualQuaternion)
            {
                actorInstanceBoneTransformsIndex = m_instanceSrg->FindShaderInputBufferIndex(Name{ "m_boneTransformsDualQuaternion" });
                if (!actorInstanceBoneTransformsIndex.IsValid())
                {
                    AZ_Error("SkinnedMeshDispatchItem", false, "Failed to find shader input index for m_boneTransformsDualQuaternion in the skinning compute shader per-instance SRG.");
                    return false;
                }
            }
            else
            {
                AZ_Assert(false, "Invalid skinning method for SkinnedMeshDispatchItem.");
            }

            AZ_Assert(aznumeric_cast<uint8_t>(m_outputBufferOffsetsInBytes.size()) == static_cast<uint8_t>(SkinnedMeshOutputVertexStreams::NumVertexStreams) && m_shaderOptions.m_applyColorMorphTargets
                   || aznumeric_cast<uint8_t>(m_outputBufferOffsetsInBytes.size()) == static_cast<uint8_t>(SkinnedMeshOutputVertexStreams::NumVertexStreams) - 1 && !m_shaderOptions.m_applyColorMorphTargets,
                "Not enough offsets were given to the SkinnedMeshDispatchItem");

            for (uint8_t outputStream = 0; outputStream < static_cast<uint8_t>(SkinnedMeshOutputVertexStreams::NumVertexStreams); outputStream++)
            {
                // Skip colors if they are not being morphed
                if (outputStream == static_cast<uint8_t>(SkinnedMeshOutputVertexStreams::Color) && !m_shaderOptions.m_applyColorMorphTargets)
                {
                    continue;
                }

                // Set the buffer offsets
                const SkinnedMeshOutputVertexStreamInfo& outputStreamInfo = SkinnedMeshVertexStreamPropertyInterface::Get()->GetOutputStreamInfo(static_cast<SkinnedMeshOutputVertexStreams>(outputStream));
                {
                    RHI::ShaderInputConstantIndex outputOffsetIndex = m_instanceSrg->FindShaderInputConstantIndex(outputStreamInfo.m_shaderResourceGroupName);
                    if (!outputOffsetIndex.IsValid())
                    {
                        AZ_Error("SkinnedMeshDispatchItem", false, "Failed to find shader input index for %s in the skinning compute shader per-instance SRG.", outputStreamInfo.m_shaderResourceGroupName.GetCStr());
                        return false;
                    }

                    // The shader has a view with 4 bytes per element
                    // Divide the byte offset here so it doesn't need to be done in the shader
                    m_instanceSrg->SetConstant(outputOffsetIndex, m_outputBufferOffsetsInBytes[outputStream] / 4);
                }
            }

            m_instanceSrg->SetBuffer(actorInstanceBoneTransformsIndex, m_boneTransforms);

            // Set the morph target related srg constants
            RHI::ShaderInputConstantIndex morphPositionOffsetIndex = m_instanceSrg->FindShaderInputConstantIndex(Name{ "m_morphTargetPositionDeltaOffset" });
            // The buffer is using 32-bit integers, so divide the offset by 4 here so it doesn't have to be done in the shader
            m_instanceSrg->SetConstant(morphPositionOffsetIndex, m_morphTargetInstanceMetaData.m_accumulatedPositionDeltaOffsetInBytes / 4);
            RHI::ShaderInputConstantIndex morphNormalOffsetIndex = m_instanceSrg->FindShaderInputConstantIndex(Name{ "m_morphTargetNormalDeltaOffset" });
            // The buffer is using 32-bit integers, so divide the offset by 4 here so it doesn't have to be done in the shader
            m_instanceSrg->SetConstant(morphNormalOffsetIndex, m_morphTargetInstanceMetaData.m_accumulatedNormalDeltaOffsetInBytes / 4);
            RHI::ShaderInputConstantIndex morphTangentOffsetIndex = m_instanceSrg->FindShaderInputConstantIndex(Name{ "m_morphTargetTangentDeltaOffset" });
            // The buffer is using 32-bit integers, so divide the offset by 4 here so it doesn't have to be done in the shader
            m_instanceSrg->SetConstant(morphTangentOffsetIndex, m_morphTargetInstanceMetaData.m_accumulatedTangentDeltaOffsetInBytes / 4);
            RHI::ShaderInputConstantIndex morphBitangentOffsetIndex = m_instanceSrg->FindShaderInputConstantIndex(Name{ "m_morphTargetBitangentDeltaOffset" });
            // The buffer is using 32-bit integers, so divide the offset by 4 here so it doesn't have to be done in the shader
            m_instanceSrg->SetConstant(morphBitangentOffsetIndex, m_morphTargetInstanceMetaData.m_accumulatedBitangentDeltaOffsetInBytes / 4);

            if (m_shaderOptions.m_applyColorMorphTargets)
            {
                RHI::ShaderInputConstantIndex morphColorOffsetIndex = m_instanceSrg->FindShaderInputConstantIndex(Name{ "m_morphTargetColorDeltaOffset" });
                // The buffer is using 32-bit integers, so divide the offset by 4 here so it doesn't have to be done in the shader
                m_instanceSrg->SetConstant(morphColorOffsetIndex, m_morphTargetInstanceMetaData.m_accumulatedColorDeltaOffsetInBytes / 4);
            }

            RHI::ShaderInputConstantIndex morphDeltaIntegerEncodingIndex = m_instanceSrg->FindShaderInputConstantIndex(Name{ "m_morphTargetDeltaInverseIntegerEncoding" });
            m_instanceSrg->SetConstant(morphDeltaIntegerEncodingIndex, 1.0f / m_morphTargetDeltaIntegerEncoding);
            
            // Set the vertex count
            const uint32_t vertexCount = m_inputBuffers->GetVertexCount(m_lodIndex);

            RHI::ShaderInputConstantIndex numVerticesIndex;
            numVerticesIndex = m_instanceSrg->FindShaderInputConstantIndex(Name{ "m_numVertices" });
            AZ_Error("SkinnedMeshInputBuffers", numVerticesIndex.IsValid(), "Failed to find shader input index for m_numVerticies in the skinning compute shader per-instance SRG.");
            m_instanceSrg->SetConstant(numVerticesIndex, vertexCount);
            
            uint32_t xThreads = 0;
            uint32_t yThreads = 0;
            CalculateSkinnedMeshTotalThreadsPerDimension(vertexCount, xThreads, yThreads);

            // Set the total number of threads in the x dimension, so the shader can calculate the vertex index from the thread ids
            RHI::ShaderInputConstantIndex totalNumberOfThreadsXIndex;
            totalNumberOfThreadsXIndex = m_instanceSrg->FindShaderInputConstantIndex(Name{ "m_totalNumberOfThreadsX" });
            AZ_Error("SkinnedMeshInputBuffers", totalNumberOfThreadsXIndex.IsValid(), "Failed to find shader input index for m_totalNumberOfThreadsX in the skinning compute shader per-instance SRG.");
            m_instanceSrg->SetConstant(totalNumberOfThreadsXIndex, xThreads);

            m_instanceSrg->Compile();
            m_dispatchItem.m_uniqueShaderResourceGroup = m_instanceSrg->GetRHIShaderResourceGroup();
            m_dispatchItem.m_pipelineState = m_skinningShader->AcquirePipelineState(pipelineStateDescriptor);

            auto& arguments = m_dispatchItem.m_arguments.m_direct;
            const auto outcome = RPI::GetComputeShaderNumThreads(m_skinningShader->GetAsset(), arguments);
            if (!outcome.IsSuccess())
            {
                AZ_Error("SkinnedMeshInputBuffers", false, outcome.GetError().c_str());
            }
 
            arguments.m_totalNumberOfThreadsX = xThreads;
            arguments.m_totalNumberOfThreadsY = yThreads;
            arguments.m_totalNumberOfThreadsZ = 1;

            return true;
        }

        const RHI::DispatchItem& SkinnedMeshDispatchItem::GetRHIDispatchItem() const
        {
            return m_dispatchItem;
        }

        Data::Instance<RPI::Buffer> SkinnedMeshDispatchItem::GetBoneTransforms() const
        {
            return m_boneTransforms;
        }

        AZStd::array_view<AZ::RHI::Ptr<RHI::BufferView>> SkinnedMeshDispatchItem::GetSourceUnskinnedBufferViews() const
        {
            return m_inputBuffers->GetInputBufferViews(m_lodIndex);
        }

        AZStd::array_view<AZ::RHI::Ptr<RHI::BufferView>> SkinnedMeshDispatchItem::GetTargetSkinnedBufferViews() const
        {
            return m_actorInstanceBufferViews;
        }

        size_t SkinnedMeshDispatchItem::GetVertexCount() const
        {
            return aznumeric_cast<size_t>(m_inputBuffers->GetVertexCount(m_lodIndex));
        }

        void SkinnedMeshDispatchItem::OnShaderReinitialized(const CachedSkinnedMeshShaderOptions* cachedShaderOptions)
        {
            m_shaderOptionGroup = cachedShaderOptions->CreateShaderOptionGroup(m_shaderOptions);

            if (!Init())
            {
                AZ_Error("SkinnedMeshDispatchItem", false, "Failed to re-initialize after the shader was re-loaded.");
            }
        }

        void CalculateSkinnedMeshTotalThreadsPerDimension(uint32_t vertexCount, uint32_t& xThreads, uint32_t& yThreads)
        {
            const uint32_t maxVerticesPerDimension = static_cast<uint32_t>(std::numeric_limits<uint16_t>::max());
            if (vertexCount > maxVerticesPerDimension * maxVerticesPerDimension)
            {
                AZ_Error("CalculateSkinnedMeshTotalThreadsPerDimension", false, "Vertex count '%d' exceeds maximum supported vertices '%d' for skinned meshes. Not all vertices will be rendered.", vertexCount, maxVerticesPerDimension * maxVerticesPerDimension);
                xThreads = maxVerticesPerDimension;
                yThreads = maxVerticesPerDimension;
                return;
            }
            else if (vertexCount == 0)
            {
                AZ_Error("CalculateSkinnedMeshTotalThreadsPerDimension", false, "Cannot skin mesh with 0 vertices.");
                xThreads = 0;
                yThreads = 0;
                return;
            }
            
            // Get the minimum number of threads in the y dimension needed to cover all the vertices in the mesh
            yThreads = vertexCount % maxVerticesPerDimension != 0 ? vertexCount / maxVerticesPerDimension + 1 : vertexCount / maxVerticesPerDimension;
            
            // Divide the total number of threads across y dimensions, rounding the number of xThreads up to cover any remainder
            xThreads = 1 + ((vertexCount - 1) / yThreads);
        }

    } // namespace Render
} // namespace AZ
