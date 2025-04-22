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
#include <Atom/RHI/DeviceBufferView.h>

#include <limits>

namespace AZ
{
    namespace Render
    {
        MorphTargetDispatchItem::MorphTargetDispatchItem(
            const AZStd::intrusive_ptr<MorphTargetInputBuffers> inputBuffers,
            const MorphTargetComputeMetaData& morphTargetComputeMetaData,
            SkinnedMeshFeatureProcessor* skinnedMeshFeatureProcessor,
            MorphTargetInstanceMetaData morphInstanceMetaData,
            float morphDeltaIntegerEncoding)
            : m_dispatchItem(RHI::MultiDevice::AllDevices)
            , m_inputBuffers(inputBuffers)
            , m_morphTargetComputeMetaData(morphTargetComputeMetaData)
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

            // Get the shader variant and instance SRG
            RPI::ShaderReloadNotificationBus::Handler::BusConnect(m_morphTargetShader->GetAssetId());
            const RPI::ShaderVariant& shaderVariant = m_morphTargetShader->GetVariant(shaderOptionGroup.GetShaderVariantId());

            if (!InitPerInstanceSRG())
            {
                return false;
            }

            if (shaderVariant.UseKeyFallback() && m_instanceSrg->HasShaderVariantKeyFallbackEntry())
            {
                m_instanceSrg->SetShaderVariantKeyFallbackValue(shaderOptionGroup.GetShaderVariantKeyFallbackValue());
            }

            RHI::PipelineStateDescriptorForDispatch pipelineStateDescriptor;
            shaderVariant.ConfigurePipelineState(pipelineStateDescriptor, shaderOptionGroup);


            InitRootConstants(pipelineStateDescriptor.m_pipelineLayoutDescriptor->GetRootConstantsLayout());

            m_dispatchItem.SetPipelineState(m_morphTargetShader->AcquirePipelineState(pipelineStateDescriptor));

            // Get the threads-per-group values from the compute shader [numthreads(x,y,z)]
            RHI::DispatchDirect arguments;
            const auto outcome = RPI::GetComputeShaderNumThreads(m_morphTargetShader->GetAsset(), arguments);
            if (!outcome.IsSuccess())
            {
                AZ_Error("MorphTargetDispatchItem", false, outcome.GetError().c_str());
            }

            arguments.m_totalNumberOfThreadsX = m_morphTargetComputeMetaData.m_vertexCount;
            arguments.m_totalNumberOfThreadsY = 1;
            arguments.m_totalNumberOfThreadsZ = 1;

            m_dispatchItem.SetArguments(arguments);

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

            m_dispatchItem.SetUniqueShaderResourceGroup(m_instanceSrg->GetRHIShaderResourceGroup());
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

            auto minIndex = rootConstantsLayout->FindShaderInputIndex(AZ::Name{ "s_min" });
            AZ_Error("MorphTargetDispatchItem", minIndex.IsValid(), "Could not find root constant 's_min' in the shader");
            auto maxIndex = rootConstantsLayout->FindShaderInputIndex(AZ::Name{ "s_max" });
            AZ_Error("MorphTargetDispatchItem", maxIndex.IsValid(), "Could not find root constant 's_max' in the shader");
            auto morphDeltaIntegerEncodingIndex = rootConstantsLayout->FindShaderInputIndex(AZ::Name{ "s_accumulatedDeltaIntegerEncoding" });
            AZ_Error("MorphTargetDispatchItem", morphDeltaIntegerEncodingIndex.IsValid(), "Could not find root constant 's_accumulatedDeltaIntegerEncoding' in the shader");
            m_weightIndex = rootConstantsLayout->FindShaderInputIndex(AZ::Name{ "s_weight" });
            AZ_Error("MorphTargetDispatchItem", m_weightIndex.IsValid(), "Could not find root constant 's_weight' in the shader");

            m_rootConstantData = AZ::RHI::ConstantsData(rootConstantsLayout);
            m_rootConstantData.SetConstant(minIndex, m_morphTargetComputeMetaData.m_minDelta);
            m_rootConstantData.SetConstant(maxIndex, m_morphTargetComputeMetaData.m_maxDelta);
            m_rootConstantData.SetConstant(morphDeltaIntegerEncodingIndex, m_accumulatedDeltaIntegerEncoding);
            m_rootConstantData.SetConstant(m_weightIndex, 0.0f);
            m_rootConstantData.SetConstant(vertexCountIndex, m_morphTargetComputeMetaData.m_vertexCount);
            // The buffer is using 32-bit integers, so divide the offset by 4 here so it doesn't have to be done in the shader
            m_rootConstantData.SetConstant(positionOffsetIndex, m_morphInstanceMetaData.m_accumulatedPositionDeltaOffsetInBytes / 4);
            m_rootConstantData.SetConstant(normalOffsetIndex, m_morphInstanceMetaData.m_accumulatedNormalDeltaOffsetInBytes / 4);
            m_rootConstantData.SetConstant(tangentOffsetIndex, m_morphInstanceMetaData.m_accumulatedTangentDeltaOffsetInBytes / 4);
            m_rootConstantData.SetConstant(bitangentOffsetIndex, m_morphInstanceMetaData.m_accumulatedBitangentDeltaOffsetInBytes / 4);

            m_dispatchItem.SetRootConstantSize(static_cast<uint8_t>(m_rootConstantData.GetConstantData().size()));
            m_dispatchItem.SetRootConstants(m_rootConstantData.GetConstantData().data());
        }

        void MorphTargetDispatchItem::SetWeight(float weight)
        {
            m_rootConstantData.SetConstant(m_weightIndex, weight);
            m_dispatchItem.SetRootConstants(m_rootConstantData.GetConstantData().data());
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
    } // namespace Render
} // namespace AZ
