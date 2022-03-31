/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Stars/StarsFeatureProcessor.h>

#include <Atom/RHI/DrawPacketBuilder.h>
#include <Atom/RHI.Reflect/InputStreamLayoutBuilder.h>

#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>

namespace AZ
{
    namespace Render
    {
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
            m_sceneSrg = GetParentScene()->GetShaderResourceGroup();

            const char* shaderFilePath = "Shaders/stars/stars.azshader";
            m_shader = RPI::LoadCriticalShader(shaderFilePath);

            m_drawListTag = m_shader->GetDrawListTag();

            m_starsDataBufferIndex.Reset();

            // do not create the pipeline state here, the scene has no pipelines at this point
        }

        void StarsFeatureProcessor::Deactivate()
        {
            m_sceneSrg = nullptr;
            m_shader = nullptr;
        }

        void StarsFeatureProcessor::Simulate(const FeatureProcessor::SimulatePacket& packet)
        {
            AZ_PROFILE_SCOPE(RPI, "StarsFeatureProcessor: Simulate");
            AZ_UNUSED(packet);

            if (!m_enabled)
            {
                return;
            }

            const AZ::RPI::ViewportContextPtr viewportContext = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get()->GetDefaultViewportContext();
            const AzFramework::WindowSize windowSize = viewportContext->GetViewportSize();
            const float vpWidth = static_cast<float>(windowSize.m_width);
            const float vpHeight = static_cast<float>(windowSize.m_height);
            const float size = m_radiusFactor * AZStd::min<float>(1.f, AZStd::min<float>(vpWidth / 1280.f, vpHeight / 720.f));

            AZ::ScriptTimePoint timePoint;
            TickRequestBus::BroadcastResult(timePoint, &TickRequestBus::Events::GetTimeAtCurrentTick);

            m_starsData[0] = size / vpWidth;
            m_starsData[1] = size / vpHeight;
            m_starsData[2] = m_intensityFactor * AZStd::min(1.f, size);
            m_starsData[3] = static_cast<float>(timePoint.GetSeconds()) * 0.5f;

            m_sceneSrg->SetConstant(m_starsDataBufferIndex, m_starsData);
            m_sceneSrg->SetConstant(m_starsRotationMatrixIndex, m_orientation);
        }

        void StarsFeatureProcessor::Render(const FeatureProcessor::RenderPacket& packet)
        {
            AZ_PROFILE_FUNCTION(AzRender);

            if (!m_enabled || !m_starsVertexBuffer)
            {
                return;
            }

            if (!m_meshPipelineState)
            {
                RHI::InputStreamLayoutBuilder layoutBuilder;
                layoutBuilder.AddBuffer()
                    ->Channel("POSITION", RHI::Format::R32G32B32_FLOAT)
                    ->Channel("COLOR", RHI::Format::R8G8B8A8_UNORM);
                layoutBuilder.SetTopology(RHI::PrimitiveTopology::TriangleList);
                m_starsVertexStreamLayout = layoutBuilder.End();

                m_meshPipelineState = aznew RPI::PipelineStateForDraw;
                m_meshPipelineState->Init(m_shader);
                m_meshPipelineState->SetInputStreamLayout(m_starsVertexStreamLayout);
                m_meshPipelineState->SetOutputFromScene(GetParentScene());
                m_meshPipelineState->Finalize();
            }

            if (!m_meshPipelineState->GetRHIPipelineState())
            {
                // we need an RHI pipeline state
                return;
            }

            // how to apply orientation by overriding object world matrix?
            RHI::DrawLinear drawLinear;
            drawLinear.m_vertexCount = m_numStarsVertices;
            drawLinear.m_vertexOffset = 0;
            drawLinear.m_instanceCount = 1;
            drawLinear.m_instanceOffset = 0;

            RHI::DrawPacketBuilder drawPacketBuilder;
            drawPacketBuilder.Begin(nullptr);
            drawPacketBuilder.SetDrawArguments(drawLinear);
            drawPacketBuilder.AddShaderResourceGroup(m_sceneSrg->GetRHIShaderResourceGroup());

            RHI::DrawPacketBuilder::DrawRequest drawRequest;
            drawRequest.m_listTag = m_drawListTag;
            drawRequest.m_pipelineState = m_meshPipelineState->GetRHIPipelineState();
            drawRequest.m_streamBufferViews = m_meshStreamBufferViews;
            drawPacketBuilder.AddDrawItem(drawRequest);
            const RHI::DrawPacket* drawPacket = drawPacketBuilder.End();
            if (drawPacket)
            {

                for (auto& view : packet.m_views)
                {
                    // If this view is ignoring packets with our draw list tag then skip this view
                    if (!view->HasDrawListTag(m_drawListTag))
                    {
                       continue;
                    }
                    view->AddDrawPacket(drawPacket);
                }
            }
        }

        void StarsFeatureProcessor::Enable(bool enable)
        {
            m_enabled = enable;
        }

        bool StarsFeatureProcessor::IsEnabled()
        {
            return m_enabled;
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

            m_meshStreamBufferViews[0] = RHI::StreamBufferView(*m_starsVertexBuffer->GetRHIBuffer(), 0, bufferSize, elementSize);
        }

        void StarsFeatureProcessor::SetIntensityFactor(float intensityFactor)
        {
            m_intensityFactor = intensityFactor;
        }

        void StarsFeatureProcessor::SetRadiusFactor(float radiusFactor)
        {
            m_radiusFactor = radiusFactor;
        }
        void StarsFeatureProcessor::SetOrientation(AZ::Quaternion orientation)
        {
            m_orientation = AZ::Matrix3x3::CreateFromQuaternion(orientation);
        }

    } // namespace Render
} // namespace AZ
