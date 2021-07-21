/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Utils/TypeHash.h>

#include <Atom/RHI.Reflect/InputStreamLayoutBuilder.h>

#include <Atom/RPI.Public/DynamicDraw/DynamicBuffer.h>
#include <Atom/RPI.Public/DynamicDraw/DynamicDrawContext.h>
#include <Atom/RPI.Public/DynamicDraw/DynamicDrawInterface.h>
#include <Atom/RPI.Public/RenderPipeline.h>

#include <Atom/RPI.Public/View.h>

namespace AZ
{
    namespace RPI
    {
        namespace
        {
            constexpr const char* PerContextSrgName = "PerContextSrg";
            constexpr const char* PerDrawSrgName = "PerDrawSrg";
        };
               
        void DynamicDrawContext::MultiStates::UpdateHash(const DrawStateOptions& drawStateOptions)
        {
            if (!m_isDirty)
            {
                return;
            }

            HashValue64 seed = HashValue64{ 0 };

            if (RHI::CheckBitsAny(drawStateOptions, DrawStateOptions::PrimitiveType))
            {
                seed = TypeHash64(m_topology, seed);
            }

            if (RHI::CheckBitsAny(drawStateOptions, DrawStateOptions::DepthState))
            {
                seed = TypeHash64(m_depthState.m_enable, seed);
                seed = TypeHash64(m_depthState.m_func, seed);
                seed = TypeHash64(m_depthState.m_writeMask, seed);
            }

            if (RHI::CheckBitsAny(drawStateOptions, DrawStateOptions::StencilState))
            {
                seed = TypeHash64(m_stencilState.m_enable, seed);
                seed = TypeHash64(m_stencilState.m_readMask, seed);
                seed = TypeHash64(m_stencilState.m_writeMask, seed);
                seed = TypeHash64(m_stencilState.m_frontFace.m_failOp, seed);
                seed = TypeHash64(m_stencilState.m_frontFace.m_depthFailOp, seed);
                seed = TypeHash64(m_stencilState.m_frontFace.m_passOp, seed);
                seed = TypeHash64(m_stencilState.m_frontFace.m_func, seed);
                seed = TypeHash64(m_stencilState.m_backFace.m_failOp, seed);
                seed = TypeHash64(m_stencilState.m_backFace.m_depthFailOp, seed);
                seed = TypeHash64(m_stencilState.m_backFace.m_passOp, seed);
                seed = TypeHash64(m_stencilState.m_backFace.m_func, seed);
            }

            if (RHI::CheckBitsAny(drawStateOptions, DrawStateOptions::FaceCullMode))
            {
                seed = TypeHash64(m_cullMode, seed);
            }

            if (RHI::CheckBitsAny(drawStateOptions, DrawStateOptions::BlendMode))
            {
                seed = TypeHash64(m_blendState0.m_enable, seed);
                seed = TypeHash64(m_blendState0.m_blendOp, seed);
                seed = TypeHash64(m_blendState0.m_blendSource, seed);
                seed = TypeHash64(m_blendState0.m_blendDest, seed);
                seed = TypeHash64(m_blendState0.m_blendAlphaOp, seed);
                seed = TypeHash64(m_blendState0.m_blendAlphaSource, seed);
                seed = TypeHash64(m_blendState0.m_blendAlphaDest, seed);
            }

            m_hash = seed;
            m_isDirty = false;
        }


        void DynamicDrawContext::InitShader(Data::Asset<ShaderAsset> shaderAsset)
        {
            Data::Instance<Shader> shader = Shader::FindOrCreate(shaderAsset);
            InitShader(shader);
        }

        void DynamicDrawContext::InitShader(Data::Instance<Shader> shader)
        {
            InitShaderWithVariant(shader, nullptr);
            m_supportShaderVariants = true;
        }

        void DynamicDrawContext::InitShaderWithVariant(Data::Asset<ShaderAsset> shaderAsset, const ShaderOptionList* optionAndValues)
        {
            Data::Instance<Shader> shader = Shader::FindOrCreate(shaderAsset);
            InitShaderWithVariant(shader, optionAndValues);
        }

        void DynamicDrawContext::InitShaderWithVariant(Data::Instance<Shader> shader, const ShaderOptionList* optionAndValues)
        {
            AZ_Assert(!m_initialized, "Can't call InitShader after context was initialized (EndInit was called)");

            if (shader == nullptr)
            {
                AZ_Error("RPI", false, "Initializing DynamicDrawContext with invalid shader");
                return;
            }

            m_supportShaderVariants = false;
            m_shader = shader;

            m_pipelineState = aznew RPI::PipelineStateForDraw;
            m_pipelineState->Init(shader, optionAndValues);

            // Set DrawListTag from shader only if it wasn't set
            if (!m_drawListTag.IsValid())
            {
                m_drawListTag = shader->GetDrawListTag();
            }

            // Create per context srg if it exist
            auto shaderAsset = shader->GetAsset();
            auto contextSrgAsset = shaderAsset->FindShaderResourceGroupAsset(Name{ PerContextSrgName });
            if (contextSrgAsset.GetId().IsValid())
            {
                m_srgPerContext = AZ::RPI::ShaderResourceGroup::Create(contextSrgAsset);
                m_srgGroups[0] = m_srgPerContext->GetRHIShaderResourceGroup();
            }

            // Save per draw srg asset which can be used to create draw srg later
            m_drawSrgAsset = shaderAsset->FindShaderResourceGroupAsset(SrgBindingSlot::Draw);
            m_hasShaderVariantKeyFallbackEntry = (m_drawSrgAsset && m_drawSrgAsset->GetLayout()->HasShaderVariantKeyFallbackEntry());
        }

        void DynamicDrawContext::InitVertexFormat(const AZStd::vector<VertexChannel>& vertexChannels)
        {
            AZ_Assert(!m_initialized, "Can't call InitVertexFormat after context was initialized (EndInit was called)");

            m_perVertexDataSize = 0;
            RHI::InputStreamLayoutBuilder layoutBuilder;
            RHI::InputStreamLayoutBuilder::BufferDescriptorBuilder* bufferBuilder = layoutBuilder.AddBuffer();
            for (auto& channel : vertexChannels)
            {
                bufferBuilder->Channel(channel.m_channel, channel.m_format);
                m_perVertexDataSize += RHI::GetFormatSize(channel.m_format);
            }
            m_pipelineState->InputStreamLayout() = layoutBuilder.End();
        }

        void DynamicDrawContext::InitDrawListTag(RHI::DrawListTag drawListTag)
        {
            AZ_Assert(!m_initialized, "Can't call InitDrawListTag after context was initialized (EndInit was called)");
            m_drawListTag = drawListTag;
        }

        void DynamicDrawContext::CustomizePipelineState(AZStd::function<void(Ptr<PipelineStateForDraw>)> updatePipelineState)
        {
            AZ_Assert(!m_initialized, "Can't call CustomizePipelineState after context was initialized (EndInit was called)");
            AZ_Assert(m_pipelineState, "Can't call CustomizePipelineState before InitShader is called");
            updatePipelineState(m_pipelineState);
        }

        uint32_t DynamicDrawContext::GetPerVertexDataSize()
        {
            return m_perVertexDataSize;
        }

        void DynamicDrawContext::EndInit()
        {
            AZ_Assert(m_scene != nullptr, "DynamicDrawContext should always belong to a scene");

            AZ_Warning("RPI", m_pipelineState, "Failed to initialized shader for DynamicDrawContext");
            AZ_Warning("RPI", m_drawListTag.IsValid(), "DynamicDrawContext doesn't have a valid DrawListTag");

            if (!m_drawListTag.IsValid() || m_pipelineState == nullptr)
            {
                return;
            }

            m_pipelineState->SetOutputFromScene(m_scene, m_drawListTag);
            m_pipelineState->Finalize();
            m_initialized = true;

            // Acquire MultiStates from m_pipelineState
            m_currentStates.m_cullMode = m_pipelineState->ConstDescriptor().m_renderStates.m_rasterState.m_cullMode;
            m_currentStates.m_topology = m_pipelineState->ConstDescriptor().m_inputStreamLayout.GetTopology();
            m_currentStates.m_depthState = m_pipelineState->ConstDescriptor().m_renderStates.m_depthStencilState.m_depth;
            m_currentStates.m_stencilState = m_pipelineState->ConstDescriptor().m_renderStates.m_depthStencilState.m_stencil;
            m_currentStates.m_blendState0 = m_pipelineState->ConstDescriptor().m_renderStates.m_blendState.m_targets[0];
            m_currentStates.UpdateHash(m_drawStateOptions);

            m_cachedRhiPipelineStates[m_currentStates.m_hash] = m_pipelineState->GetRHIPipelineState();
            m_rhiPipelineState = m_pipelineState->GetRHIPipelineState();
        }

        void DynamicDrawContext::SetScene(Scene* scene)
        {
            AZ_Assert(scene, "SetScene called with an invalid scene");
            if (!scene || m_scene == scene)
            {
                return;
            }
            m_scene = scene;
            m_drawFilter = RHI::DrawFilterMaskDefaultValue;
            // Reinitialize if it was initialized
            if (m_initialized)
            {
                // Report warning if there were some draw data
                AZ_Warning(
                    "DynamicDrawContext", m_cachedDrawItems.size() == 0,
                    "DynamicDrawContext::SetForScene should be called"
                    " when there is no cached draw data");
                // Clear some cached data
                FrameEnd();
                m_cachedRhiPipelineStates.clear();
                // Reinitialize
                EndInit();
            }
        }

        void DynamicDrawContext::SetRenderPipeline(RenderPipeline* pipeline)
        {
            AZ_Assert(pipeline, "SetRenderPipeline called with an invalid pipeline");
            if (!pipeline)
            {
                return;
            }
            SetScene(pipeline->GetScene());
            m_drawFilter = pipeline->GetDrawFilterMask();
        }

        bool DynamicDrawContext::IsReady()
        {
            return m_initialized;
        }

        ShaderVariantId DynamicDrawContext::UseShaderVariant(const ShaderOptionList& optionAndValues)
        {
            AZ_Assert(m_initialized && m_supportShaderVariants, "DynamicDrawContext is not initialized or unable to support shader variants. "
                "Check if it was initialized with InitShaderWithVariant");

            ShaderVariantId variantId;

            if (!m_supportShaderVariants)
            {
                return variantId;
            }

            RPI::ShaderOptionGroup shaderOptionGroup = m_shader->CreateShaderOptionGroup();
            shaderOptionGroup.SetUnspecifiedToDefaultValues();

            for (const auto& optionAndValue : optionAndValues)
            {
                shaderOptionGroup.SetValue(optionAndValue.first, optionAndValue.second);
            }

            variantId = shaderOptionGroup.GetShaderVariantId();
            return variantId;
        }

        void DynamicDrawContext::AddDrawStateOptions(DrawStateOptions options)
        {
            AZ_Assert(!m_initialized, "Can't call AddDrawStateOptions after context was initialized (EndInit was called)");
            m_drawStateOptions |= options;
        }

        bool DynamicDrawContext::HasDrawStateOptions(DrawStateOptions options)
        {
            return RHI::CheckBitsAny(m_drawStateOptions, options);
        }

        void DynamicDrawContext::SetDepthState(RHI::DepthState depthState)
        {
            if (RHI::CheckBitsAny(m_drawStateOptions, DrawStateOptions::DepthState))
            {
                if (!(m_currentStates.m_depthState == depthState))
                {
                    m_currentStates.m_depthState = depthState;
                    m_currentStates.m_isDirty = true;
                }
            }
            else
            {
                AZ_Warning("RHI", false, "Can't set SetDepthState if DrawVariation::DepthState wasn't enabled");
            }
        }

        void DynamicDrawContext::SetStencilState(RHI::StencilState stencilState)
        {
            if (RHI::CheckBitsAny(m_drawStateOptions, DrawStateOptions::StencilState))
            {
                if (!(m_currentStates.m_stencilState == stencilState))
                {
                    m_currentStates.m_stencilState = stencilState;
                    m_currentStates.m_isDirty = true;
                }
            }
            else
            {
                AZ_Warning("RHI", false, "Can't set SetStencilState if DrawVariation::StencilState wasn't enabled");
            }

        }

        void DynamicDrawContext::SetCullMode(RHI::CullMode cullMode)
        {
            if (RHI::CheckBitsAny(m_drawStateOptions, DrawStateOptions::FaceCullMode))
            {
                if (m_currentStates.m_cullMode != cullMode)
                {
                    m_currentStates.m_cullMode = cullMode;
                    m_currentStates.m_isDirty = true;
                }
            }
            else
            {
                AZ_Warning("RHI", false, "Can't set CullMode if DrawVariation::FaceCullMode wasn't enabled");
            }
        }

        void DynamicDrawContext::SetTarget0BlendState(RHI::TargetBlendState blendState)
        {
            if (RHI::CheckBitsAny(m_drawStateOptions, DrawStateOptions::BlendMode))
            {
                if (!(m_currentStates.m_blendState0 == blendState))
                {
                    m_currentStates.m_blendState0 = blendState;
                    m_currentStates.m_isDirty = true;
                }
            }
            else
            {
                AZ_Warning("RHI", false, "Can't set TargetBlendState if DrawVariation::BlendMode wasn't enabled");
            }
        }

        void DynamicDrawContext::SetPrimitiveType(RHI::PrimitiveTopology topology)
        {
            if (RHI::CheckBitsAny(m_drawStateOptions, DrawStateOptions::PrimitiveType))
            {
                if (m_currentStates.m_topology != topology)
                {
                    m_currentStates.m_topology = topology;
                    m_currentStates.m_isDirty = true;
                }
            }
            else
            {
                AZ_Warning("RHI", false, "Can't set PrimitiveTopology if DrawVariation::PrimitiveType wasn't enabled");
            }
        }

        void DynamicDrawContext::SetScissor(RHI::Scissor scissor)
        {
            m_useScissor = true;
            m_scissor = scissor;
        }

        void DynamicDrawContext::UnsetScissor()
        {
            m_useScissor = false;
        }

        void DynamicDrawContext::SetViewport(RHI::Viewport viewport)
        {
            m_useViewport = true;
            m_viewport = viewport;
        }

        void DynamicDrawContext::UnsetViewport()
        {
            m_useViewport = false;
        }

        void DynamicDrawContext::SetStencilReference(uint8_t stencilRef)
        {
            m_stencilRef = stencilRef;
        }

        uint8_t DynamicDrawContext::GetStencilReference() const
        {
            return m_stencilRef;
        }

        void DynamicDrawContext::SetShaderVariant(ShaderVariantId shaderVariantId)
        {
            AZ_Assert( m_initialized && m_supportShaderVariants, "DynamicDrawContext is not initialized or unable to support shader variants. "
                "Check if it was initialized with InitShaderWithVariant");
            m_currentShaderVariantId = shaderVariantId;
        }

        void DynamicDrawContext::DrawIndexed(const void* vertexData, uint32_t vertexCount, const void* indexData, uint32_t indexCount, RHI::IndexFormat indexFormat, Data::Instance < ShaderResourceGroup> drawSrg)
        {
            if (!m_initialized)
            {
                AZ_Assert(false, "DynamicDrawContext isn't initialized");
                return;
            }

            if (m_drawSrgAsset.GetId().IsValid() && drawSrg == nullptr)
            {
                AZ_Assert(false, "PerDrawSrg need to be provided since the shader uses it");
                return;
            }

            // DrawIndexed requires vertex data and index data
            if (indexData == nullptr || indexCount == 0 || vertexData == nullptr || vertexCount == 0)
            {
                AZ_Assert(false, "Failed to draw due to invalid index or vertex data");
                return;
            }

            // Get dynamic buffers for vertex and index buffer. Skip draw if failed to allocate buffers
            uint32_t vertexDataSize = vertexCount * m_perVertexDataSize;
            RHI::Ptr<DynamicBuffer> vertexBuffer;
            vertexBuffer = DynamicDrawInterface::Get()->GetDynamicBuffer(vertexDataSize);

            uint32_t indexDataSize = indexCount * RHI::GetIndexFormatSize(indexFormat);
            RHI::Ptr<DynamicBuffer> indexBuffer = DynamicDrawInterface::Get()->GetDynamicBuffer(indexDataSize);

            if (indexBuffer == nullptr || vertexBuffer == nullptr)
            {
                return;
            }

            DrawItemInfo drawItemInfo;
            RHI::DrawItem& drawItem = drawItemInfo.m_drawItem;

            // Draw argument
            RHI::DrawIndexed drawIndexed;
            drawIndexed.m_indexCount = indexCount;
            drawIndexed.m_instanceCount = 1;
            drawItem.m_arguments = drawIndexed;

            // Get RHI pipeline state from cached RHI pipeline states based on current draw state options
            drawItem.m_pipelineState = GetCurrentPipelineState();

            // Write data to vertex buffer and set up stream buffer views for DrawItem
            // The stream buffer view need to be cached before the frame is end
            vertexBuffer->Write(vertexData, vertexDataSize);
            m_cachedStreamBufferViews.push_back(vertexBuffer->GetStreamBufferView(m_perVertexDataSize));
            drawItem.m_streamBufferViewCount = 1;
            drawItemInfo.m_vertexBufferViewIndex = uint32_t(m_cachedStreamBufferViews.size() - 1);

            // Write data to index buffer and set up index buffer view for DrawItem
            indexBuffer->Write(indexData, indexDataSize);
            m_cachedIndexBufferViews.push_back(indexBuffer->GetIndexBufferView(indexFormat));
            drawItemInfo.m_indexBufferViewIndex = uint32_t(m_cachedIndexBufferViews.size() - 1);

            // Setup per context srg if it exists
            if (m_srgPerContext)
            {
                drawItem.m_shaderResourceGroupCount = 1;
                drawItem.m_shaderResourceGroups = m_srgGroups;
            }

            // Setup per draw srg
            if (drawSrg)
            {
                drawItem.m_uniqueShaderResourceGroup = drawSrg->GetRHIShaderResourceGroup();
                m_cachedDrawSrg.push_back(drawSrg);
            }

            // Set scissor per draw if scissor is enabled.
            if (m_useScissor)
            {
                drawItem.m_scissorsCount = 1;
                drawItem.m_scissors = &m_scissor;
            }

            // Set viewport per draw if viewport is enabled.
            if (m_useViewport)
            {
                drawItem.m_viewportsCount = 1;
                drawItem.m_viewports = &m_viewport;
            }

            // Set stencil reference. Used when stencil is enabled.
            drawItem.m_stencilRef = m_stencilRef;

            drawItemInfo.m_sortKey = m_sortKey++;
            m_cachedDrawItems.emplace_back(drawItemInfo);
        }
                
        void DynamicDrawContext::DrawLinear(const void* vertexData, uint32_t vertexCount, Data::Instance<ShaderResourceGroup> drawSrg)
        {
            if (!m_initialized)
            {
                AZ_Assert(false, "DynamicDrawContext isn't initialized");
                return;
            }

            if (m_drawSrgAsset.GetId().IsValid() && drawSrg == nullptr)
            {
                AZ_Assert(false, "PerDrawSrg need to be provided since the shader uses it");
                return;
            }

            if (vertexData == nullptr || vertexCount == 0)
            {
                AZ_Assert(false, "Failed to draw due to invalid vertex data");
                return;
            }

            // Get dynamic buffers for vertex and index buffer. Skip draw if failed to allocate buffers
            uint32_t vertexDataSize = vertexCount * m_perVertexDataSize;
            RHI::Ptr<DynamicBuffer> vertexBuffer;
            vertexBuffer = DynamicDrawInterface::Get()->GetDynamicBuffer(vertexDataSize);

            if (vertexBuffer == nullptr)
            {
                return;
            }

            DrawItemInfo drawItemInfo;
            RHI::DrawItem& drawItem = drawItemInfo.m_drawItem;

            // Draw argument
            RHI::DrawLinear drawLinear;
            drawLinear.m_instanceCount = 1;
            drawLinear.m_vertexCount = vertexCount;
            drawItem.m_arguments = drawLinear;

            // Get RHI pipeline state from cached RHI pipeline states based on current draw state options
            drawItem.m_pipelineState = GetCurrentPipelineState();

            // Write data to vertex buffer and set up stream buffer views for DrawItem
            // The stream buffer view need to be cached before the frame is end
            vertexBuffer->Write(vertexData, vertexDataSize);
            m_cachedStreamBufferViews.push_back(vertexBuffer->GetStreamBufferView(m_perVertexDataSize));
            drawItem.m_streamBufferViewCount = 1;
            drawItemInfo.m_vertexBufferViewIndex = uint32_t(m_cachedStreamBufferViews.size() - 1);

            // Setup per context srg if it exists
            if (m_srgPerContext)
            {
                drawItem.m_shaderResourceGroupCount = 1;
                drawItem.m_shaderResourceGroups = m_srgGroups;
            }

            // Setup per draw srg
            if (drawSrg)
            {
                drawItem.m_uniqueShaderResourceGroup = drawSrg->GetRHIShaderResourceGroup();
                m_cachedDrawSrg.push_back(drawSrg);
            }

            // Set scissor per draw if scissor is enabled.
            if (m_useScissor)
            {
                drawItem.m_scissorsCount = 1;
                drawItem.m_scissors = &m_scissor;
            }

            // Set viewport per draw if viewport is enabled.
            if (m_useViewport)
            {
                drawItem.m_viewportsCount = 1;
                drawItem.m_viewports = &m_viewport;
            }

            drawItemInfo.m_sortKey = m_sortKey++;
            m_cachedDrawItems.emplace_back(drawItemInfo);
        }


        Data::Instance<ShaderResourceGroup> DynamicDrawContext::NewDrawSrg()
        {
            if (!m_drawSrgAsset.IsReady())
            {
                return nullptr;
            }
            auto drawSrg = AZ::RPI::ShaderResourceGroup::Create(m_drawSrgAsset);

            // Set fallback value for shader variant if draw srg contains constant for shader variant fallback 
            if (m_hasShaderVariantKeyFallbackEntry)
            {
                // If the dynamic draw context support multiple shader variants, it uses m_currentShaderVariantId to setup srg shader variant fallback key
                if (m_supportShaderVariants)
                {
                    drawSrg->SetShaderVariantKeyFallbackValue(m_currentShaderVariantId.m_key);
                }
                // otherwise use the m_pipelineState to config the fallback
                else
                {
                    m_pipelineState->UpdateSrgVariantFallback(drawSrg);
                }
            }

            return drawSrg;
        }

        Data::Instance<ShaderResourceGroup> DynamicDrawContext::GetPerContextSrg()
        {
            return m_srgPerContext;
        }

        bool DynamicDrawContext::IsVertexSizeValid(uint32_t vertexSize)
        {
            return m_perVertexDataSize == vertexSize;
        }

        RHI::DrawListTag DynamicDrawContext::GetDrawListTag()
        {
            return m_drawListTag;
        }

        const Data::Instance<Shader>& DynamicDrawContext::GetShader() const
        {
            return m_shader;
        }

        void DynamicDrawContext::SetSortKey(RHI::DrawItemSortKey key)
        {
            m_sortKey = key;
        }

        RHI::DrawItemSortKey DynamicDrawContext::GetSortKey() const
        {
            return m_sortKey;
        }

        void DynamicDrawContext::SubmitDrawData(ViewPtr view)
        {
            if (!m_initialized)
            {
                return;
            }

            if (!view->HasDrawListTag(m_drawListTag))
            {
                return;
            }
 
            for (auto& drawItemInfo : m_cachedDrawItems)
            {
                if (drawItemInfo.m_indexBufferViewIndex != InvalidIndex)
                {
                    drawItemInfo.m_drawItem.m_indexBufferView = &m_cachedIndexBufferViews[drawItemInfo.m_indexBufferViewIndex];
                }

                if (drawItemInfo.m_vertexBufferViewIndex != InvalidIndex)
                {
                    drawItemInfo.m_drawItem.m_streamBufferViews = &m_cachedStreamBufferViews[drawItemInfo.m_vertexBufferViewIndex];
                }

                RHI::DrawItemProperties drawItemProperties;
                drawItemProperties.m_sortKey = drawItemInfo.m_sortKey;
                drawItemProperties.m_item = &drawItemInfo.m_drawItem;
                drawItemProperties.m_drawFilterMask = m_drawFilter;
                view->AddDrawItem(m_drawListTag, drawItemProperties);
            }
        }

        void DynamicDrawContext::FrameEnd()
        {
            m_sortKey = 0;
            m_cachedDrawItems.clear();
            m_cachedStreamBufferViews.clear();
            m_cachedIndexBufferViews.clear();
            m_cachedDrawSrg.clear();
        }

        const RHI::PipelineState* DynamicDrawContext::GetCurrentPipelineState()
        {
            // If m_currentStates wasn't changed, it's safe to return m_rhiPipelineState directly.
            if (!m_currentStates.m_isDirty)
            {
                return m_rhiPipelineState;
            }

            // m_currentStates is dirty. need to update m_currentStates's hash
            m_currentStates.UpdateHash(m_drawStateOptions);

            // search for cached pipeline state by using the updated hash 
            auto findResult = m_cachedRhiPipelineStates.find(m_currentStates.m_hash);
            if (findResult == m_cachedRhiPipelineStates.end())
            {
                // Create pipelineState for current m_currentStates
                if (RHI::CheckBitsAny(m_drawStateOptions, DrawStateOptions::PrimitiveType))
                {
                    if (m_pipelineState->ConstDescriptor().m_inputStreamLayout.GetTopology() != m_currentStates.m_topology)
                    {
                        RHI::InputStreamLayout& inputStreamLayout = m_pipelineState->InputStreamLayout();
                        inputStreamLayout.SetTopology(m_currentStates.m_topology);
                        inputStreamLayout.Finalize();
                    }
                }
                if (RHI::CheckBitsAny(m_drawStateOptions, DrawStateOptions::DepthState))
                {
                    m_pipelineState->RenderStatesOverlay().m_depthStencilState.m_depth = m_currentStates.m_depthState;
                }
                if (RHI::CheckBitsAny(m_drawStateOptions, DrawStateOptions::StencilState))
                {
                    m_pipelineState->RenderStatesOverlay().m_depthStencilState.m_stencil = m_currentStates.m_stencilState;
                }
                if (RHI::CheckBitsAny(m_drawStateOptions, DrawStateOptions::FaceCullMode))
                {
                    m_pipelineState->RenderStatesOverlay().m_rasterState.m_cullMode = m_currentStates.m_cullMode;
                }
                if (RHI::CheckBitsAny(m_drawStateOptions, DrawStateOptions::BlendMode))
                {
                    m_pipelineState->RenderStatesOverlay().m_blendState.m_targets[0] = m_currentStates.m_blendState0;
                }

                const RHI::PipelineState* pipelineState = m_pipelineState->Finalize();                
                m_cachedRhiPipelineStates[m_currentStates.m_hash] = pipelineState;
                m_rhiPipelineState = pipelineState;
            }
            else
            {
                m_rhiPipelineState = findResult->second;
            }

            return m_rhiPipelineState;
        }
    }
}
