/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RPI.Reflect/Material/MaterialTypeAsset.h>
#include <Atom/Feature/RenderCommon.h>
#include <Atom/RHI/DrawPacketBuilder.h>
#include <DeferredMaterial/DeferredDrawPacket.h>
#include <DeferredMaterial/DeferredDrawPacketManager.h>

namespace AZ
{
    namespace Render
    {
        DeferredDrawPacket::DeferredDrawPacket(
            DeferredDrawPacketManager* drawPacketManager,
            const RPI::Scene* scene,
            RPI::Material* material,
            const Name& materialPipelineName,
            const RPI::ShaderCollection::Item& shaderItem,
            const int32_t drawPacketId)
            : m_drawPacketManager(drawPacketManager)
            , m_drawPacketId(drawPacketId)
        {
            Init(scene, material, materialPipelineName, shaderItem);
        }

        // ShaderReloadNotificationBus::Handler overrides...
        void DeferredDrawPacket::OnShaderReinitialized(const RPI::Shader& shader)
        {
            m_needsRebuild = true;
            m_drawPacketManager->SetNeedsUpdate(true);
        }
        void DeferredDrawPacket::OnShaderAssetReinitialized(const Data::Asset<RPI::ShaderAsset>& shaderAsset)
        {
            m_needsRebuild = true;
            m_drawPacketManager->SetNeedsUpdate(true);
        }
        void DeferredDrawPacket::OnShaderVariantReinitialized(const RPI::ShaderVariant& shaderVariant)
        {
            m_needsRebuild = true;
            m_drawPacketManager->SetNeedsUpdate(true);
        }

        void DeferredDrawPacket::Init(
            const RPI::Scene* scene,
            RPI::Material* material,
            const Name& materialPipelineName,
            const RPI::ShaderCollection::Item& shaderItem)
        {
            auto materialAsset = material->GetAsset();

#ifdef DEFERRED_DRAWPACKET_DEBUG_PRINT
            m_instigatingMaterialAsset =
                Data::Asset<RPI::MaterialAsset>(materialAsset.GetId(), materialAsset.GetType(), materialAsset.GetHint());
            // the draw-packet can outlast the original material asset it was created for, so don't keep a real reference to the asset
            m_instigatingMaterialAsset.SetAutoLoadBehavior(Data::AssetLoadBehavior::NoLoad);

            auto materialTypeAsset = materialAsset->GetMaterialTypeAsset();
            m_instigatingMaterialTypeAsset =
                Data::Asset<RPI::MaterialTypeAsset>(materialTypeAsset.GetId(), materialTypeAsset.GetType(), materialTypeAsset.GetHint());
            m_instigatingMaterialTypeAsset.SetAutoLoadBehavior(Data::AssetLoadBehavior::NoLoad);
#endif /* DEFERRED_DRAWPACKET_DEBUG_PRINT */

            RPI::ShaderOptionGroup shaderOptions = *shaderItem.GetShaderOptions();
            shaderOptions.SetUnspecifiedToDefaultValues();
            m_shaderVariantId = shaderOptions.GetShaderVariantId();

            m_shader = RPI::Shader::FindOrCreate(shaderItem.GetShaderAsset());
            if (!m_shader)
            {
                AZ_Error(
                    "DeferredDrawPacket",
                    false,
                    "Shader '%s' of material %s:. Failed to find or create instance",
                    shaderItem.GetShaderAsset()->GetName().GetCStr(),
                    materialAsset.GetHint().c_str());
                return;
            }

            m_drawListTag = shaderItem.GetDrawListTagOverride();
            if (!m_drawListTag.IsValid())
            {
                m_drawListTag = m_shader->GetDrawListTag();
            }

            // Shader - Variant and pipeline-State
            const AZ::RPI::ShaderVariant& variant = m_shader->GetVariant(m_shaderVariantId);
            if (variant.IsRootVariant())
            {
                // TODO: why only for the root variant?
                // AZ::RPI::ShaderReloadNotificationBus::MultiHandler::BusConnect();
            }

            // Deferred Draw-SRG: Holds the shaderOptions, the material-Type - ID and the shaderDrawPacketId
            m_drawSrg = m_shader->CreateDrawSrgForShaderVariant(shaderOptions, false);
            if (!m_drawSrg)
            {
                AZ_Assert(false, "Failed to create deferred drawSrg");
                return;
            }
            {
                // material-Type ID to make sure the Materialparameter - layout matches
                RHI::ShaderInputNameIndex materialTypeIdIndex{ "m_materialTypeId" };
                m_drawSrg->SetConstant(materialTypeIdIndex, material->GetMaterialTypeId());

                // mapping from the meshInfoIndex written by the Visibility-buffer to the material-type + shader options
                RHI::ShaderInputNameIndex shaderDrawPacketIdIndex{ "m_shaderDrawPacketId" };
                m_drawSrg->SetConstant(shaderDrawPacketIdIndex, m_drawPacketId);

                // don't compile the draw-srg yet, we still need the DrawPacketIds-buffer
            }

            m_materialSrg = material->GetShaderResourceGroup();
            m_materialPipelineName = materialPipelineName;
            m_shaderTag = shaderItem.GetShaderTag();

            AZ::RHI::PipelineStateDescriptorForDraw pipelineStateDescriptor;
            variant.ConfigurePipelineState(pipelineStateDescriptor, shaderOptions);

            // Render states need to merge the runtime variation.
            // This allows materials to customize the render states that the shader uses.
            const RHI::RenderStates& renderStatesOverlay = *shaderItem.GetRenderStatesOverlay();
            RHI::MergeStateInto(renderStatesOverlay, pipelineStateDescriptor.m_renderStates);

            // Render a single fullscreen triangle
            RHI::InputStreamLayout inputStreamLayout;
            inputStreamLayout.SetTopology(RHI::PrimitiveTopology::TriangleList);
            inputStreamLayout.Finalize();

            pipelineStateDescriptor.m_inputStreamLayout = inputStreamLayout;

            scene->ConfigurePipelineState(m_drawListTag, pipelineStateDescriptor);

            // This draw item purposefully does not reference any geometry buffers.
            // Instead it's expected that the vertex shader generates a full-screen triangle completely from vertex ids.
            m_geometryView = AZStd::make_shared<RHI::GeometryView>(RHI::GeometryView{});
            RHI::DrawLinear draw = RHI::DrawLinear();
            draw.m_vertexCount = 3;
            m_geometryView->SetDrawArguments(RHI::DrawLinear(3, 0));

            m_rootConstantsLayout = pipelineStateDescriptor.m_pipelineLayoutDescriptor->GetRootConstantsLayout();
            bool hasRootConstants = m_rootConstantsLayout && m_rootConstantsLayout->GetDataSize() > 0;
            if (hasRootConstants)
            {
                m_rootConstants.resize(m_rootConstantsLayout->GetDataSize());
            }

            m_pipelineState = m_shader->AcquirePipelineState(pipelineStateDescriptor);
            if (!(m_pipelineState && m_pipelineState->GetType() == AZ::RHI::PipelineStateType::Draw))
            {
                AZ_Error("DeferredDrawPacket", false, "Failed to create pipelineState");
                return;
            }

            RHI::DrawPacketBuilder::DrawRequest drawRequest;
            drawRequest.m_listTag = m_drawListTag;
            drawRequest.m_pipelineState = m_pipelineState;
            // TODO: why do i need a stencil ref here?
            // drawRequest.m_stencilRef = StencilRefs::UseIBLSpecularPass;
            // Note: this doesn't do anything, since the geometry-view doesn't have any stream-buffers
            drawRequest.m_streamIndices = m_geometryView->GetFullStreamBufferIndices();
            drawRequest.m_sortKey = 0;
            drawRequest.m_uniqueShaderResourceGroup = m_drawSrg->GetRHIShaderResourceGroup();

            if (m_materialPipelineName != RPI::MaterialPipelineNone)
            {
                RHI::DrawFilterTag pipelineTag = scene->GetDrawFilterTagRegistry()->AcquireTag(m_materialPipelineName);
                AZ_Assert(pipelineTag.IsValid(), "Could not acquire pipeline filter tag '%s'.", m_materialPipelineName.GetCStr());
                drawRequest.m_drawFilterMask = 1 << pipelineTag.GetIndex();
            }

            AZ::RHI::DrawPacketBuilder drawPacketBuilder{ AZ::RHI::MultiDevice::AllDevices };
            drawPacketBuilder.Begin(nullptr);
            drawPacketBuilder.SetGeometryView(m_geometryView.get());
            if (hasRootConstants)
            {
                drawPacketBuilder.SetRootConstants(m_rootConstants);
            }
            drawPacketBuilder.AddShaderResourceGroup(m_materialSrg->GetRHIShaderResourceGroup());
            drawPacketBuilder.AddDrawItem(drawRequest);

            m_drawPacket = drawPacketBuilder.End();
        }

        void DeferredDrawPacket::CompileDrawSrg(Data::Instance<RPI::Buffer> drawPacketIdBuffer)
        {
            // unique id for combination of the material type and shader options
            RHI::ShaderInputNameIndex shaderDrawPacketIds{ "m_drawPacketIds" };
            m_drawSrg->SetBuffer(shaderDrawPacketIds, drawPacketIdBuffer);

            m_drawSrg->Compile();
        }

    } // namespace Render
} // namespace AZ