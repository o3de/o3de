/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "DynamicPrimitiveProcessor.h"
#include "AuxGeomDrawProcessorShared.h"

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/DrawPacketBuilder.h>
#include <Atom/RHI.Reflect/InputStreamLayoutBuilder.h>

#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/View.h>

namespace AZ
{
    namespace Render
    {
        namespace
        {
            // the max size of a vertex buffer
            static const size_t MaxUploadBufferSize = MaxDynamicVertexCount * sizeof(AuxGeomDynamicVertex);

            static const RHI::PrimitiveTopology PrimitiveTypeToTopology[PrimitiveType_Count] =
            {
                RHI::PrimitiveTopology::PointList,
                RHI::PrimitiveTopology::LineList,
                RHI::PrimitiveTopology::TriangleList,
            };
        }

        bool DynamicPrimitiveProcessor::Initialize(AZ::RHI::Device& rhiDevice, const AZ::RPI::Scene* scene)
        {
            // Note: We use HeapMemoryLevel::Host here so that we can use OrphanBuffer in the update
            RHI::BufferPoolDescriptor dynamicPoolDescriptor;
            dynamicPoolDescriptor.m_heapMemoryLevel = RHI::HeapMemoryLevel::Host;
            dynamicPoolDescriptor.m_bindFlags = RHI::BufferBindFlags::InputAssembly;
            dynamicPoolDescriptor.m_largestPooledAllocationSizeInBytes = MaxUploadBufferSize;

            m_hostPool = RHI::Factory::Get().CreateBufferPool();
            m_hostPool->SetName(Name("AuxGeomDynamicPrimitiveBufferPool"));
            RHI::ResultCode resultCode = m_hostPool->Init(rhiDevice, dynamicPoolDescriptor);

            if (resultCode != RHI::ResultCode::Success)
            {
                AZ_Error("DynamicPrimitiveProcessor", false, "Failed to initialize AuxGeom dynamic primitive buffer pool");
                return false;
            }            

            for (int primitiveType = 0; primitiveType < PrimitiveType_Count; ++primitiveType)
            {
                SetupInputStreamLayout(m_inputStreamLayout[primitiveType], PrimitiveTypeToTopology[primitiveType]);

                m_streamBufferViewsValidatedForLayout[primitiveType] = false;
            }

            if (!CreateBuffers())
            {
                return false;
            }

            m_scene = scene;
            InitShader();

            return true;
        }

        void DynamicPrimitiveProcessor::Release()
        {
            DestroyBuffers();

            if (m_hostPool)
            {
                m_hostPool.reset();
            }

            m_drawPackets.clear();
            m_processSrgs.clear();
            m_shaderData.m_defaultSRG = nullptr;

            m_shader = nullptr;
            m_scene = nullptr;

            for (RPI::Ptr<RPI::PipelineStateForDraw>* pipelineState : m_createdPipelineStates)
            {
                pipelineState->reset();
            }
            m_createdPipelineStates.clear();
        }

        void DynamicPrimitiveProcessor::PrepareFrame()
        {
            m_drawPackets.clear();
            m_processSrgs.clear();

            if (m_needUpdatePipelineStates)
            {
                // for created pipeline state, re-set their data from scene
                for (RPI::Ptr<RPI::PipelineStateForDraw>* pipelineState : m_createdPipelineStates)
                {
                    (*pipelineState)->SetOutputFromScene(m_scene);
                    (*pipelineState)->Finalize();
                }
                m_needUpdatePipelineStates = false;
            }
        }

        void DynamicPrimitiveProcessor::ProcessDynamicPrimitives(const AuxGeomBufferData* bufferData, const RPI::FeatureProcessor::RenderPacket& fpPacket)
        {
            RHI::DrawPacketBuilder drawPacketBuilder;

            const DynamicPrimitiveData& srcPrimitives = bufferData->m_primitiveData;
            // Update the buffers for the dynamic primitives and generate draw packets for them
            if (srcPrimitives.m_indexBuffer.size() > 0)
            {
                // Update the buffers for all dynamic primitives in this frame's data
                // There is just one index buffer and one vertex buffer for all dynamic primitives
                UpdateIndexBuffer(srcPrimitives.m_indexBuffer, m_primitiveBuffers);
                UpdateVertexBuffer(srcPrimitives.m_vertexBuffer, m_primitiveBuffers);

                // Validate the stream buffer views for all stream layout's if necessary
                for (int primitiveType = 0; primitiveType < PrimitiveType_Count; ++primitiveType)
                {
                    ValidateStreamBufferViews(m_primitiveBuffers.m_streamBufferViews, m_streamBufferViewsValidatedForLayout, primitiveType);
                }

                // Loop over all the primitives and use one draw call for each AuxGeom API call
                // We have to create separate draw packets for each view that the AuxGeom is in (typically only one)
                AZStd::vector<RPI::ViewPtr> auxGeomViews;
                for (auto& view : fpPacket.m_views)
                {
                    // If this view is ignoring packets with our draw list tag then skip this view
                    if (!view->HasDrawListTag(m_shaderData.m_drawListTag))
                    {
                        continue;
                    }
                    auxGeomViews.emplace_back(view);
                }

                for (auto& primitive : srcPrimitives.m_primitiveBuffer)
                {
                    bool useManualViewProjectionOverride = primitive.m_viewProjOverrideIndex != -1;

                    PipelineStateOptions pipelineStateOptions;
                    pipelineStateOptions.m_perpectiveType = useManualViewProjectionOverride? PerspectiveType_ManualOverride : PerspectiveType_ViewProjection;
                    pipelineStateOptions.m_blendMode = primitive.m_blendMode;
                    pipelineStateOptions.m_primitiveType = primitive.m_primitiveType;
                    pipelineStateOptions.m_depthReadType = primitive.m_depthReadType;
                    pipelineStateOptions.m_depthWriteType = primitive.m_depthWriteType;
                    pipelineStateOptions.m_faceCullMode = primitive.m_faceCullMode;
                    RPI::Ptr<RPI::PipelineStateForDraw> pipelineState = GetPipelineState(pipelineStateOptions);

                    Data::Instance<RPI::ShaderResourceGroup> srg;
                    if (useManualViewProjectionOverride || primitive.m_primitiveType == PrimitiveType_PointList)
                    {
                        srg = RPI::ShaderResourceGroup::Create(m_shaderData.m_perDrawSrgAsset);
                        if (!srg)
                        {
                            AZ_Warning("AuxGeom", false, "Failed to create a shader resource group for an AuxGeom draw, Ignoring the draw");
                            continue; // failed to create an srg for this draw, just drop the draw.
                        }
                        if (useManualViewProjectionOverride)
                        {
                            srg->SetConstant(m_shaderData.m_viewProjectionOverrideIndex, bufferData->m_viewProjOverrides[primitive.m_viewProjOverrideIndex]);
                            m_shaderData.m_viewProjectionOverrideIndex.AssertValid();
                        }
                        if (primitive.m_primitiveType == PrimitiveType_PointList)
                        {
                            srg->SetConstant(m_shaderData.m_pointSizeIndex, aznumeric_cast<float>(primitive.m_width));
                            m_shaderData.m_pointSizeIndex.AssertValid();
                        }
                        pipelineState->UpdateSrgVariantFallback(srg);
                        srg->Compile();
                    }
                    else
                    {
                        srg = m_shaderData.m_defaultSRG;
                    }

                    for (auto& view : auxGeomViews)
                    {
                        RHI::DrawItemSortKey sortKey = primitive.m_blendMode == BlendMode_Off ? 0 : view->GetSortKeyForPosition(primitive.m_center);


                        const RHI::DrawPacket* drawPacket = BuildDrawPacketForDynamicPrimitive(
                            m_primitiveBuffers,
                            pipelineState,
                            srg,
                            primitive.m_indexCount,
                            primitive.m_indexOffset,
                            drawPacketBuilder,
                            sortKey);

                        if (drawPacket)
                        {
                            m_drawPackets.emplace_back(drawPacket);
                            m_processSrgs.push_back(srg);
                            view->AddDrawPacket(drawPacket);
                        }
                    }
                }
            }
        }

        bool DynamicPrimitiveProcessor::CreateBuffers()
        {
            if (!CreateBufferGroup(m_primitiveBuffers))
            {
                return false;
            }
            return true;
        }

        void DynamicPrimitiveProcessor::DestroyBuffers()
        {
            DestroyBufferGroup(m_primitiveBuffers);
        }

        bool DynamicPrimitiveProcessor::CreateBufferGroup(DynamicBufferGroup& group)
        {
            RHI::ResultCode result = RHI::ResultCode::Fail;

            group.m_indexBuffer = RHI::Factory::Get().CreateBuffer();
            group.m_vertexBuffer = RHI::Factory::Get().CreateBuffer();

            group.m_indexBuffer->SetName(AZ::Name("AuxGeomIndexBuffer"));
            group.m_vertexBuffer->SetName(AZ::Name("AuxGeomVertexBuffer"));

            AZStd::vector<RHI::Ptr<RHI::Buffer>> buffers = { group.m_indexBuffer , group.m_vertexBuffer };

            RHI::BufferInitRequest bufferRequest;
            bufferRequest.m_descriptor = RHI::BufferDescriptor{ RHI::BufferBindFlags::InputAssembly, MaxUploadBufferSize };

            for (const RHI::Ptr<RHI::Buffer>& buffer : buffers)
            {
                bufferRequest.m_buffer = buffer.get();

                result = m_hostPool->InitBuffer(bufferRequest);

                if (result != RHI::ResultCode::Success)
                {
                    AZ_Error("DynamicPrimitiveProcessor", false, "Failed to create GPU buffers for AuxGeom");
                    return false;
                }
            }

            // We have a single stream (position and color are interleaved in the vertex buffer)
            group.m_streamBufferViews.resize(1);

            return true;
        }

        void DynamicPrimitiveProcessor::DestroyBufferGroup(DynamicBufferGroup& group)
        {
            group.m_indexBuffer.reset();
            group.m_vertexBuffer.reset();

            group.m_streamBufferViews.clear();
        }

        void DynamicPrimitiveProcessor::UpdateBuffer(const uint8_t* source, size_t sourceSize, RHI::Ptr<RHI::Buffer> buffer)
        {
            // This should never happen because of tests in AuxGeomDrawQueue in the functions that increase the source size.
            AZ_Assert(sourceSize <= MaxUploadBufferSize, "Max upload buffer size exceeded");

            // We use OrphanBuffer currently. If we have issues we may need to add fences or use FrameCountMax buffers
            // in a round-robin system.
            RHI::ResultCode orphanResult = m_hostPool->OrphanBuffer(*buffer);
            AZ_Assert(orphanResult == RHI::ResultCode::Success, "OrphanBuffer failed");

            if (orphanResult == RHI::ResultCode::Success)
            {
                RHI::BufferMapResponse mapResponse;
                m_hostPool->MapBuffer(RHI::BufferMapRequest(*buffer, 0, sourceSize), mapResponse);

                auto* mappedData = reinterpret_cast<uint8_t*>(mapResponse.m_data);

                if (mappedData)
                {
                    memcpy(mappedData, source, sourceSize);

                    m_hostPool->UnmapBuffer(*buffer);
                }
            }
        }

        void DynamicPrimitiveProcessor::UpdateIndexBuffer(const IndexBuffer& source, DynamicBufferGroup& group)
        {
            const size_t sourceByteSize = source.size() * sizeof(AuxGeomIndex);
            auto* sourceBytes = reinterpret_cast<const uint8_t*>(source.data());

            UpdateBuffer(sourceBytes, sourceByteSize, group.m_indexBuffer);

            group.m_indexBufferView = RHI::IndexBufferView(
                *group.m_indexBuffer, 0, static_cast<uint32_t>(sourceByteSize), RHI::IndexFormat::Uint32);
        }

        void DynamicPrimitiveProcessor::UpdateVertexBuffer(const VertexBuffer& source, DynamicBufferGroup& group)
        {
            const size_t sourceByteSize = source.size() * sizeof(AuxGeomDynamicVertex);
            auto* sourceBytes = reinterpret_cast<const uint8_t*>(source.data());

            UpdateBuffer(sourceBytes, sourceByteSize, group.m_vertexBuffer);

            group.m_streamBufferViews[0] = RHI::StreamBufferView(
                *group.m_vertexBuffer, 0, static_cast<uint32_t>(sourceByteSize), static_cast<uint32_t>(sizeof(AuxGeomDynamicVertex)));
        }

        void DynamicPrimitiveProcessor::ValidateStreamBufferViews(StreamBufferViewsForAllStreams& streamBufferViews, bool* isValidated, int primitiveType)
        {
            if (!isValidated[primitiveType])
            {
                if (!RHI::ValidateStreamBufferViews(m_inputStreamLayout[primitiveType], streamBufferViews))
                {
                    AZ_Error("DynamicPrimitiveProcessor", false, "Failed to validate the stream buffer views");
                    return;
                }
                else
                {
                    isValidated[primitiveType] = true;
                }
            }
        }

        void DynamicPrimitiveProcessor::SetupInputStreamLayout(RHI::InputStreamLayout& inputStreamLayout, RHI::PrimitiveTopology topology)
        {
            RHI::InputStreamLayoutBuilder layoutBuilder;
            layoutBuilder.AddBuffer()
                ->Channel("POSITION", RHI::Format::R32G32B32_FLOAT)
                ->Channel("COLOR", RHI::Format::R8G8B8A8_UNORM);
            layoutBuilder.SetTopology(topology);
            inputStreamLayout = layoutBuilder.End();
        }

        RPI::Ptr<RPI::PipelineStateForDraw>& DynamicPrimitiveProcessor::GetPipelineState(const PipelineStateOptions& pipelineStateOptions)
        {
            // The declaration: m_pipelineStates[PerspectiveType_Count][BlendMode_Count][PrimitiveType_Count][DepthRead_Count][DepthWrite_Count][FaceCull_Count];
            return m_pipelineStates[pipelineStateOptions.m_perpectiveType][pipelineStateOptions.m_blendMode][pipelineStateOptions.m_primitiveType]
                [pipelineStateOptions.m_depthReadType][pipelineStateOptions.m_depthWriteType][pipelineStateOptions.m_faceCullMode];
        }

        void DynamicPrimitiveProcessor::SetUpdatePipelineStates()
        {
            m_needUpdatePipelineStates = true;
        }

        void DynamicPrimitiveProcessor::InitPipelineState(const PipelineStateOptions& pipelineStateOptions)
        {
            // Use the the pipeline state for PipelineStateOptions with default values and input perspective type as base pipeline state. Create one if it was empty.
            PipelineStateOptions defaultOptions;
            defaultOptions.m_perpectiveType = pipelineStateOptions.m_perpectiveType;
            RPI::Ptr<RPI::PipelineStateForDraw>& basePipelineState = GetPipelineState(defaultOptions);
            if (basePipelineState.get() == nullptr)
            {
                basePipelineState = aznew RPI::PipelineStateForDraw;
                Name perspectiveTypeViewProjection = Name("ViewProjectionMode::ViewProjection");
                Name perspectiveTypeManualOverride = Name("ViewProjectionMode::ManualOverride");
                Name optionViewProjectionModeName = Name("o_viewProjMode");

                RPI::ShaderOptionList shaderOptionAndValues;
                shaderOptionAndValues.push_back(RPI::ShaderOption(optionViewProjectionModeName,
                    (pipelineStateOptions.m_perpectiveType == AuxGeomShapePerpectiveType::PerspectiveType_ViewProjection)?perspectiveTypeViewProjection: perspectiveTypeManualOverride));
                basePipelineState->Init(m_shader, &shaderOptionAndValues);

                m_createdPipelineStates.push_back(&basePipelineState);
            }

            RPI::Ptr<RPI::PipelineStateForDraw>& destPipelineState = GetPipelineState(pipelineStateOptions);

            // Copy from base pipeline state. Skip if it's the base pipeline state 
            if (destPipelineState.get() == nullptr)
            {
                destPipelineState = aznew RPI::PipelineStateForDraw(*basePipelineState.get());
                m_createdPipelineStates.push_back(&destPipelineState);
            }

            // blendMode
            RHI::TargetBlendState& blendState = destPipelineState->RenderStatesOverlay().m_blendState.m_targets[0];
            blendState.m_enable = pipelineStateOptions.m_blendMode == AuxGeomBlendMode::BlendMode_Alpha;
            blendState.m_blendSource = RHI::BlendFactor::AlphaSource;
            blendState.m_blendDest = RHI::BlendFactor::AlphaSourceInverse;

            // primitiveType
            destPipelineState->InputStreamLayout() = m_inputStreamLayout[pipelineStateOptions.m_primitiveType];

            // depthReadType
            // Keep the default depth comparison function and only set it when depth read is off
            // Note: since the default PipelineStateOptions::m_depthReadType is DepthRead_On, the basePipelineState keeps the comparison function read from shader variant
            if (pipelineStateOptions.m_depthReadType == AuxGeomDepthReadType::DepthRead_Off)
            {
                destPipelineState->RenderStatesOverlay().m_depthStencilState.m_depth.m_func = RHI::ComparisonFunc::Always;
            }

            // depthWriteType
            destPipelineState->RenderStatesOverlay().m_depthStencilState.m_depth.m_writeMask =
                ConvertToRHIDepthWriteMask(pipelineStateOptions.m_depthWriteType);

            // faceCullMode
            destPipelineState->RenderStatesOverlay().m_rasterState.m_cullMode = ConvertToRHICullMode(pipelineStateOptions.m_faceCullMode);

            destPipelineState->SetOutputFromScene(m_scene);
            destPipelineState->Finalize();
        }

        void DynamicPrimitiveProcessor::InitShader()
        {
            const char* auxGeomWorldShaderFilePath = "Shaders/auxgeom/auxgeomworld.azshader";

            m_shader = RPI::LoadShader(auxGeomWorldShaderFilePath);
            if (!m_shader)
            {
                AZ_Error("DynamicPrimitiveProcessor", false, "Failed to get shader");
                return;
            }

            // Get the per-object SRG and store the indices of the data we need to set per object
            m_shaderData.m_perDrawSrgAsset = m_shader->FindShaderResourceGroupAsset(Name{ "PerDrawSrg" });
            if (!m_shaderData.m_perDrawSrgAsset.GetId().IsValid())
            {
                AZ_Error("DynamicPrimitiveProcessor", false, "Failed to get shader resource group asset");
                return;
            }

            const RHI::ShaderResourceGroupLayout* shaderResourceGroupLayout = m_shaderData.m_perDrawSrgAsset->GetLayout();

            m_shaderData.m_viewProjectionOverrideIndex.Reset();
            m_shaderData.m_pointSizeIndex.Reset();

            // Remember the draw list tag
            m_shaderData.m_drawListTag = m_shader->GetDrawListTag();

            // Create a default SRG for draws that don't use a manual view projection override
            m_shaderData.m_defaultSRG = RPI::ShaderResourceGroup::Create(m_shaderData.m_perDrawSrgAsset);
            AZ_Assert(m_shaderData.m_defaultSRG != nullptr, "Creating the default SRG unexpectedly failed");
            m_shaderData.m_defaultSRG->SetConstant(m_shaderData.m_pointSizeIndex, 10.0f);
            m_shaderData.m_defaultSRG->Compile();

            // Initialize all pipeline states
            PipelineStateOptions pipelineStateOptions;
            // initialize two base pipeline state first to preserve the blend functions
            pipelineStateOptions.m_perpectiveType = PerspectiveType_ViewProjection;
            InitPipelineState(pipelineStateOptions);
            pipelineStateOptions.m_perpectiveType = PerspectiveType_ManualOverride;
            InitPipelineState(pipelineStateOptions);

            // Initialize all pipeline states
            for (uint32_t perspectiveType = 0; perspectiveType < PerspectiveType_Count; perspectiveType++)
            {
                pipelineStateOptions.m_perpectiveType = (AuxGeomShapePerpectiveType)perspectiveType;
                for (uint32_t blendMode = 0; blendMode < BlendMode_Count; blendMode++)
                {
                    pipelineStateOptions.m_blendMode = (AuxGeomBlendMode)blendMode;
                    for (uint32_t primitiveType = 0; primitiveType < PrimitiveType_Count; primitiveType++)
                    {
                        pipelineStateOptions.m_primitiveType = (AuxGeomPrimitiveType)primitiveType;
                        for (uint32_t depthRead = 0; depthRead < DepthRead_Count; depthRead++)
                        {
                            pipelineStateOptions.m_depthReadType = (AuxGeomDepthReadType)depthRead;
                            for (uint32_t depthWrite = 0; depthWrite < DepthWrite_Count; depthWrite++)
                            {
                                pipelineStateOptions.m_depthWriteType = (AuxGeomDepthWriteType)depthWrite;
                                for (uint32_t faceCullMode = 0; faceCullMode < FaceCull_Count; faceCullMode++)
                                {
                                    pipelineStateOptions.m_faceCullMode = (AuxGeomFaceCullMode)faceCullMode;
                                    InitPipelineState(pipelineStateOptions);
                                }
                            }
                        }
                    }
                }
            }
        }

        const RHI::DrawPacket* DynamicPrimitiveProcessor::BuildDrawPacketForDynamicPrimitive(
            DynamicBufferGroup& group,
            const RPI::Ptr<RPI::PipelineStateForDraw>& pipelineState,
            Data::Instance<RPI::ShaderResourceGroup> srg,
            uint32_t indexCount,
            uint32_t indexOffset,
            RHI::DrawPacketBuilder& drawPacketBuilder,
            RHI::DrawItemSortKey sortKey)
        {
            RHI::DrawIndexed drawIndexed;
            drawIndexed.m_indexCount = indexCount;
            drawIndexed.m_indexOffset = indexOffset;
            drawIndexed.m_vertexOffset = 0; // indices are offsets from the start of vertex buffer

            drawPacketBuilder.Begin(nullptr);
            drawPacketBuilder.SetDrawArguments(drawIndexed);
            drawPacketBuilder.SetIndexBufferView(group.m_indexBufferView);
            drawPacketBuilder.AddShaderResourceGroup(srg->GetRHIShaderResourceGroup());

            RHI::DrawPacketBuilder::DrawRequest drawRequest;
            drawRequest.m_listTag = m_shaderData.m_drawListTag;
            drawRequest.m_pipelineState = pipelineState->GetRHIPipelineState();
            drawRequest.m_streamBufferViews = group.m_streamBufferViews;
            drawRequest.m_sortKey = sortKey;
            drawPacketBuilder.AddDrawItem(drawRequest);

            return drawPacketBuilder.End();
        }


    } // namespace Render
} // namespace AZ
