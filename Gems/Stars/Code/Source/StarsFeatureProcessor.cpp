/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <StarsFeatureProcessor.h>

#include <Atom/RHI/DrawPacketBuilder.h>
#include <Atom/RHI.Reflect/InputStreamLayoutBuilder.h>

#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>

#include <AzCore/Name/NameDictionary.h>

namespace AZ::Render
{
    constexpr float MinViewPortWidth = 1280.f;
    constexpr float MinViewPortHeight = 720.f;

    void StarsFeatureProcessor::Reflect(ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext
                ->Class<StarsFeatureProcessor, FeatureProcessor>()
                ->Version(1);
        }
    }

    void StarsFeatureProcessor::Activate()
    {
        const char* shaderFilePath = "Shaders/stars/stars.azshader";
        m_shader = RPI::LoadCriticalShader(shaderFilePath);
        if (!m_shader)
        {
            AZ_Error("StarsFeatureProcessor", false, "Failed to load required stars shader.");
            return;
        }
        Data::AssetBus::Handler::BusConnect(m_shader->GetAssetId());

        auto drawSrgLayout = m_shader->GetAsset()->GetDrawSrgLayout(m_shader->GetSupervariantIndex());
        AZ_Error("StarsFeatureProcessor", drawSrgLayout, "Failed to get the draw shader resource group layout for the stars shader.");

        if (drawSrgLayout)
        {
            m_drawSrg = RPI::ShaderResourceGroup::Create(m_shader->GetAsset(), m_shader->GetSupervariantIndex(), drawSrgLayout->GetName());
        }

        m_drawListTag = m_shader->GetDrawListTag();

        m_starParamsIndex.Reset();
        m_rotationIndex.Reset();

        auto viewportContextInterface = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        auto viewportContext = viewportContextInterface->GetViewportContextByScene(GetParentScene());
        if (viewportContext)
        {
            m_viewportSize = viewportContext->GetViewportSize();
            RPI::ViewportContextIdNotificationBus::Handler::BusConnect(viewportContext->GetId());
        }
        else
        {
            m_viewportSize = AzFramework::WindowSize(static_cast<uint32_t>(MinViewPortWidth), static_cast<uint32_t>(MinViewPortHeight));
        }
        EnableSceneNotification();

    }

    void StarsFeatureProcessor::Deactivate()
    {
        Data::AssetBus::Handler::BusDisconnect(m_shader->GetAssetId());

        RPI::ViewportContextIdNotificationBus::Handler::BusDisconnect();

        DisableSceneNotification();

        m_shader = nullptr;
    }

    void StarsFeatureProcessor::Simulate([[maybe_unused]] const FeatureProcessor::SimulatePacket& packet)
    {
        AZ_PROFILE_SCOPE(RPI, "StarsFeatureProcessor: Simulate");

        if (m_updateShaderConstants)
        {
            m_updateShaderConstants = false;

            UpdateShaderConstants();
        }
    }

    void StarsFeatureProcessor::UpdateShaderConstants()
    {
        const float width = static_cast<float>(m_viewportSize.m_width);
        const float height = static_cast<float>(m_viewportSize.m_height);

        const float size = m_radiusFactor * AZStd::min<float>(1.f, AZStd::min<float>(width / MinViewPortWidth, height / MinViewPortHeight));

        m_shaderConstants.m_scaleX = size / width;
        m_shaderConstants.m_scaleY = size / height;
        m_shaderConstants.m_scaledExposure = pow(2.f, m_exposure) * AZStd::min(1.f, size);

        if (m_drawSrg)
        {
            m_drawSrg->SetConstant(m_starParamsIndex, m_shaderConstants);
            m_drawSrg->SetConstant(m_rotationIndex, m_orientation);
            m_drawSrg->Compile();
        }
    }

    void StarsFeatureProcessor::UpdateDrawPacket()
    {
        if (m_geometryView.GetStreamBufferViews().size() == 0)
        {
            return;
        }

        if(m_meshPipelineState && m_drawSrg && m_geometryView.GetStreamBufferView(0).GetByteCount() != 0)
        {
            m_drawPacket = BuildDrawPacket();
        }
    }

    void StarsFeatureProcessor::Render(const FeatureProcessor::RenderPacket& packet)
    {
        AZ_PROFILE_FUNCTION(AzRender);

        if (m_drawPacket)
        {
            for (auto& view : packet.m_views)
            {
                if (!view->HasDrawListTag(m_drawListTag))
                {
                   continue;
                }

                constexpr float depth = 0.f;
                view->AddDrawPacket(m_drawPacket.get(), depth);
            }
        }
    }

    void StarsFeatureProcessor::SetStars(const AZStd::vector<StarVertex>& starVertexData)
    {
        const uint32_t elementCount = static_cast<uint32_t>(starVertexData.size());
        const uint32_t elementSize = sizeof(StarVertex);
        const uint32_t bufferSize = elementCount * elementSize; // bytecount

        m_starsMeshData = starVertexData;
        m_numStarsVertices = elementCount;

        if (!m_starsVertexBuffer)
        {
            RPI::CommonBufferDescriptor desc;
            desc.m_poolType = RPI::CommonBufferPoolType::StaticInputAssembly;
            desc.m_bufferName = "StarsMeshBuffer";
            desc.m_byteCount = bufferSize;
            desc.m_elementSize = elementSize;
            desc.m_bufferData = m_starsMeshData.data();
            m_starsVertexBuffer = RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
        }
        else
        {
            if (m_starsVertexBuffer->GetBufferSize() != bufferSize)
            {
                m_starsVertexBuffer->Resize(bufferSize);
            }

            m_starsVertexBuffer->UpdateData(m_starsMeshData.data(), bufferSize);
        }

        m_geometryView.ClearStreamBufferViews();
        m_geometryView.AddStreamBufferView( RHI::StreamBufferView(*m_starsVertexBuffer->GetRHIBuffer(), 0, bufferSize, elementSize) );

        UpdateDrawPacket();
    }

    void StarsFeatureProcessor::SetExposure(float exposure)
    {
        m_exposure = exposure;
        m_updateShaderConstants = true;
    }

    void StarsFeatureProcessor::SetRadiusFactor(float radiusFactor)
    {
        m_radiusFactor = radiusFactor;
        m_updateShaderConstants = true;
    }

    void StarsFeatureProcessor::SetOrientation(AZ::Quaternion orientation)
    {
        m_orientation = AZ::Matrix3x3::CreateFromQuaternion(orientation);
        m_updateShaderConstants = true;
    }

    void StarsFeatureProcessor::SetTwinkleRate(float twinkleRate)
    {
        m_shaderConstants.m_twinkleRate = twinkleRate;
        m_updateShaderConstants = true;
    }

    void StarsFeatureProcessor::OnRenderPipelineChanged([[maybe_unused]] AZ::RPI::RenderPipeline* renderPipeline,
        AZ::RPI::SceneNotification::RenderPipelineChangeType changeType)
    {
        if (changeType == AZ::RPI::SceneNotification::RenderPipelineChangeType::Added)
        {
            if(!m_meshPipelineState)
            {
                m_meshPipelineState = aznew RPI::PipelineStateForDraw;
                m_meshPipelineState->Init(m_shader);


                RHI::InputStreamLayoutBuilder layoutBuilder;
                layoutBuilder.AddBuffer()
                    ->Channel("POSITION", RHI::Format::R32G32B32_FLOAT)
                    ->Channel("COLOR", RHI::Format::R8G8B8A8_UNORM);
                layoutBuilder.SetTopology(RHI::PrimitiveTopology::TriangleList);
                auto inputStreamLayout = layoutBuilder.End();

                m_meshPipelineState->SetInputStreamLayout(inputStreamLayout);
                m_meshPipelineState->SetOutputFromScene(GetParentScene());
                m_meshPipelineState->Finalize();

                UpdateDrawPacket();
                UpdateBackgroundClearColor();
            }
        }
        else if (changeType == AZ::RPI::SceneNotification::RenderPipelineChangeType::PassChanged)
        {
            if(m_meshPipelineState)
            {
                m_meshPipelineState->SetOutputFromScene(GetParentScene());
                m_meshPipelineState->Finalize();

                UpdateDrawPacket();
                UpdateBackgroundClearColor();
            }
        }
    }

    void StarsFeatureProcessor::OnViewportSizeChanged(AzFramework::WindowSize size)
    {
        m_viewportSize = size;
        m_updateShaderConstants = true;
    }

    void StarsFeatureProcessor::OnAssetReloaded([[maybe_unused]] Data::Asset<Data::AssetData> asset)
    {
        UpdateDrawPacket();
    }

    void StarsFeatureProcessor::UpdateBackgroundClearColor()
    {
        // This function is only necessary for now because the default clear value
        // color is not black, and is set in various .pass files in places a user
        // is unlikely to find.  Unfortunately, the viewport will revert to the
        // grey color when resizing momentarily.
        const RHI::ClearValue blackClearValue = RHI::ClearValue::CreateVector4Float(0.f, 0.f, 0.f, 0.f);
        RPI::PassFilter passFilter;
        AZStd::string slot;

        auto setClearValue = [&](RPI::Pass* pass)-> RPI::PassFilterExecutionFlow
        {
            Name slotName = Name::FromStringLiteral(slot, AZ::Interface<AZ::NameDictionary>::Get());
            if (auto binding = pass->FindAttachmentBinding(slotName))
            {
                binding->m_unifiedScopeDesc.m_loadStoreAction.m_clearValue = blackClearValue;
            }
            return RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
        };

        slot = "SpecularOutput";
        passFilter= RPI::PassFilter::CreateWithTemplateName(Name("ForwardPassTemplate"), GetParentScene());
        RPI::PassSystemInterface::Get()->ForEachPass(passFilter, setClearValue);
        passFilter = RPI::PassFilter::CreateWithTemplateName(Name("ForwardMSAAPassTemplate"), GetParentScene());
        RPI::PassSystemInterface::Get()->ForEachPass(passFilter, setClearValue);

        slot = "ReflectionOutput";
        passFilter = RPI::PassFilter::CreateWithTemplateName(Name("ReflectionGlobalFullscreenPassTemplate"), GetParentScene());
        RPI::PassSystemInterface::Get()->ForEachPass(passFilter, setClearValue);
    }

    RHI::ConstPtr<RHI::DrawPacket> StarsFeatureProcessor::BuildDrawPacket()
    {
        m_geometryView.SetDrawArguments(RHI::DrawLinear{ m_numStarsVertices, 0 });

        RHI::DrawPacketBuilder drawPacketBuilder{RHI::MultiDevice::AllDevices};
        drawPacketBuilder.Begin(nullptr);
        drawPacketBuilder.SetGeometryView(&m_geometryView);
        drawPacketBuilder.AddShaderResourceGroup(m_drawSrg->GetRHIShaderResourceGroup());

        RHI::DrawPacketBuilder::DrawRequest drawRequest;
        drawRequest.m_listTag = m_drawListTag;
        drawRequest.m_pipelineState = m_meshPipelineState->GetRHIPipelineState();
        drawRequest.m_streamIndices = m_geometryView.GetFullStreamBufferIndices();
        drawPacketBuilder.AddDrawItem(drawRequest);
        return drawPacketBuilder.End();
    }
} // namespace AZ::Render
