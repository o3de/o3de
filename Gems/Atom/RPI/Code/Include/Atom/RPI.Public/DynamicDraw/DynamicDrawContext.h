/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI.Reflect/Base.h> // for AZ_BITS and AZ_DEFINE_ENUM_BITWISE_OPERATORS

#include <Atom/RPI.Public/PipelineState.h>

#include <AzCore/std/smart_ptr/intrusive_base.h>

namespace AZ
{
    namespace RPI
    {
        class Scene;
        class RenderPipeline;
        class RasterPass;

        //! This class helps setup dynamic draw data as well as provide draw functions to draw dynamic items.
        //! The draw calls added to the context are only valid for one frame.
        //! DynamicDrawContext is only associated with
        //!     * One shader.
        //!     * One draw list tag which is initialized from shader but can be overwritten.
        //! DynamicDrawContext may allow some render states change or few other changes which are defined in DynamicDrawContext::DrawVariation
        class DynamicDrawContext
            : public AZStd::intrusive_base
        {
            friend class DynamicDrawSystem;
            AZ_RTTI(AZ::RPI::DynamicDrawContext, "{9F6645D7-2C64-4963-BAAB-5144E92F61E2}");
            AZ_CLASS_ALLOCATOR(DynamicDrawContext, AZ::SystemAllocator, 0);

        public:
            virtual ~DynamicDrawContext() = default;

            // Type of render state which can be changed for dynamic draw context
            enum class DrawStateOptions : uint32_t
            {
                PrimitiveType = AZ_BIT(0),
                DepthState = AZ_BIT(1),
                StencilState = AZ_BIT(2),
                FaceCullMode = AZ_BIT(3),
                BlendMode = AZ_BIT(4)
            };

            struct VertexChannel
            {
                VertexChannel(const AZStd::string& name, RHI::Format format)
                    : m_channel(name)
                    , m_format(format)
                {}

                AZStd::string m_channel;
                RHI::Format m_format;
            };

            // Required initialization functions
            //! Initialize this context with the input shader/shader asset with only one shader variant
            //! DynamicDrawContext initialized with this function can't use other variant later (AddShaderVariant and SetShaderVariant).
            void InitShaderWithVariant(Data::Asset<ShaderAsset> shaderAsset, const ShaderOptionList* optionAndValues);
            void InitShaderWithVariant(Data::Instance<Shader> shader, const ShaderOptionList* optionAndValues);

            //! Initialize this context with the input shader/shader asset.
            //! DynamicDrawContext initialized with this function may use shader variant later (AddShaderVariant and SetShaderVariant).
            void InitShader(Data::Asset<ShaderAsset> shaderAsset);
            void InitShader(Data::Instance<Shader> shader);

            // Optional initialization functions
            //! Initialize input stream layout with vertex channel information
            void InitVertexFormat(const AZStd::vector<VertexChannel>& vertexChannels);
            //! Initialize draw list tag of this 
            void InitDrawListTag(RHI::DrawListTag drawListTag);

            //! Customize pipeline state through a function
            //! This function is intended to do pipeline state customization after all initialization function calls but before EndInit
            void CustomizePipelineState(AZStd::function<void(Ptr<PipelineStateForDraw>)> updatePipelineState);

            //! Enable draw state changes for this DynamicDrawContext.
            //! This function can only be called before EndInit() is called
            void AddDrawStateOptions(DrawStateOptions options);

            //! Call any one of these functions to decide which scope this DynamicDrawContext may draw to.
            //! One of the function has to be called once before EndInit() is called.
            //! After DynamicDrawContext is initialized, the output scope can be changed. 
            //! But it has to be called after existing draw calls are submitted.
            //! @Param scene Draw calls made with this DynamicDrawContext will be submit to this scene
            //! @Param pipeline Draw calls made with this DynamicDrawContext will be submit to this render pipeline
            //! @Param pass Draw calls made with this DynamicDrawContext will only be submit to this pass
            void SetOutputScope(Scene* scene);
            void SetOutputScope(RenderPipeline* pipeline);
            void SetOutputScope(RasterPass* pass);

            //! Finalize and validate initialization. Any initialization functions should be called before EndInit is called. 
            void EndInit();

            //! Return if this DynamicDrawContext is ready to add draw calls
            bool IsReady();

            //! Return if some draw state options change are enabled. 
            bool HasDrawStateOptions(DrawStateOptions options);

            //! Tell DynamicDrawContext it will use the shader variant specified by ShaderOptionList. 
            //! The returned ShaderVariantId can be used via SetShaderVariant() before making any draw calls.
            //! Note, if the DynamicDrawContext was initialized with default shader variant, it won't return a valid variant Id.
            ShaderVariantId UseShaderVariant(const ShaderOptionList& optionAndValues);

            // States which can be changed for this DyanimcDrawContext

            //! Set DepthState if DrawStateOptions::DepthState option is enabled
            void SetDepthState(RHI::DepthState depthState);
            //! Set StencilState if DrawStateOptions::StencilState option is enabled
            void SetStencilState(RHI::StencilState stencilState);
            //! Set CullMode if DrawStateOptions::FaceCullMode option is enabled
            void SetCullMode(RHI::CullMode cullMode);
            //! Set TargetBlendState for target 0 if DrawStateOptions::BlendMode option is enabled
            void SetTarget0BlendState(RHI::TargetBlendState blendState);
            //! Set PrimitiveType if DrawStateOptions::PrimitiveType option is enabled
            void SetPrimitiveType(RHI::PrimitiveTopology topology);
            //! Set the shader variant as the current variant which is used for any following draw calls. 
            //! Note, SetShaderVariant() needs to be called before NewDrawSrg() if a draw srg is used in following draw calls.
            void SetShaderVariant(ShaderVariantId shaderVariantId);

            //! Setup scissor for following draws which are added to this DynamicDrawContext
            //! Note: it won't effect any draws submitted out of this DynamicDrawContext
            void SetScissor(RHI::Scissor scissor);

            //! Remove per draw scissor for draws added to this DynamicDrawContext
            //! Without per draw scissor, the scissor setup in pass is usually used. 
            void UnsetScissor();

            //! Setup viewport for following draws which are added to this DynamicDrawContext
            //! Note: it won't effect any draws submitted out of this DynamicDrawContext
            void SetViewport(RHI::Viewport viewport);

            //! Remove per draw viewport for draws added to this DynamicDrawContext
            //! Without per draw viewport, the viewport setup in pass is usually used. 
            void UnsetViewport();

            //! Set stencil reference for following draws which are added to this DynamicDrawContext
            void SetStencilReference(uint8_t stencilRef);

            //! Get the current stencil reference.
            uint8_t GetStencilReference() const;

            //! Draw Indexed primitives with vertex and index data and per draw srg
            //! The per draw srg need to be provided if it's required by shader. 
            void DrawIndexed(const void* vertexData, uint32_t vertexCount, const void* indexData, uint32_t indexCount, RHI::IndexFormat indexFormat, Data::Instance < ShaderResourceGroup> drawSrg = nullptr);

            //! Draw linear indexed primitives with vertex data and per draw srg
            //! The per draw srg need to be provided if it's required by shader. 
            void DrawLinear(const void* vertexData, uint32_t vertexCount, Data::Instance<ShaderResourceGroup> drawSrg);

            //! Get per vertex size. The size was evaluated when vertex format was set
            uint32_t GetPerVertexDataSize();

            //! Get DrawListTag of this DynamicDrawContext
            RHI::DrawListTag GetDrawListTag();

            //! Create a draw srg
            Data::Instance<ShaderResourceGroup> NewDrawSrg();

            //! Get per context srg
            Data::Instance<ShaderResourceGroup> GetPerContextSrg();

            //! return whether the vertex data size is valid
            bool IsVertexSizeValid(uint32_t vertexSize);
                        
            //! Get the shader which is associated with this DynamicDrawContext
            const Data::Instance<Shader>& GetShader() const;

            //! Set the sort key for the next draw.
            //! Note: The sort key will be increased by 1 whenever the a draw function is called.
            void SetSortKey(RHI::DrawItemSortKey key);

            //! Get the current sort key.
            RHI::DrawItemSortKey GetSortKey() const;

        private:
            DynamicDrawContext() = default;

            // Submit draw items to a view
            void SubmitDrawList(ViewPtr view);

            // Finalize the draw list for all submiited draws.
            void FinalizeDrawList();

            RHI::DrawListView GetDrawList();
            
            // Reset cached draw data when frame is end (draw data was submitted)
            void FrameEnd();

            void ReInit();

            // Get rhi pipeline state which matches current states
            const RHI::PipelineState* GetCurrentPipelineState();
                        
            struct MultiStates
            {
                // states available for change 
                RHI::CullMode m_cullMode;
                RHI::DepthState m_depthState;
                RHI::StencilState m_stencilState;
                RHI::PrimitiveTopology m_topology;
                RHI::TargetBlendState m_blendState0;

                HashValue64 m_hash = HashValue64{ 0 };
                bool m_isDirty = false;

                void UpdateHash(const DrawStateOptions& drawStateOptions);
            };

            MultiStates m_currentStates;

            // current scissor
            bool m_useScissor = false;
            RHI::Scissor m_scissor;

            // current viewport
            bool m_useViewport = false;
            RHI::Viewport m_viewport;

            // Current stencil reference value
            uint8_t m_stencilRef = 0;

            // Cached RHI pipeline states for different combination of render states 
            AZStd::unordered_map<HashValue64, const RHI::PipelineState*> m_cachedRhiPipelineStates;

            // Current RHI pipeline state for current MultiStates
            const RHI::PipelineState* m_rhiPipelineState = nullptr;

            // Data for draw item
            Ptr<PipelineStateForDraw> m_pipelineState;
            Data::Instance<ShaderResourceGroup> m_srgPerContext;
            RHI::ShaderResourceGroup* m_srgGroups[1]; // array for draw item's srg groups
            uint32_t m_perVertexDataSize = 0;
            RHI::Ptr<RHI::ShaderResourceGroupLayout> m_drawSrgLayout;
            bool m_hasShaderVariantKeyFallbackEntry = false;

            // Draw variations allowed in this DynamicDrawContext
            DrawStateOptions m_drawStateOptions;

            // DrawListTag used to help setup PipelineState's output
            // and also for submitting draw items to views 
            RHI::DrawListTag m_drawListTag;

            // Output scope related
            enum class OutputScopeType
            {
                Unset,
                Scene,
                RenderPipeline,
                RasterPass
            };
            Scene* m_scene = nullptr;
            RasterPass* m_pass = nullptr;
            OutputScopeType m_outputScope = OutputScopeType::Unset;

            // All draw items use this filter when submit them to views
            // It's set to RenderPipeline's draw filter mask if the DynamicDrawContext was created for a render pipeline.
            RHI::DrawFilterMask m_drawFilter = RHI::DrawFilterMaskDefaultValue;

            // Cached draw data
            AZStd::vector<RHI::StreamBufferView> m_cachedStreamBufferViews;
            AZStd::vector<RHI::IndexBufferView> m_cachedIndexBufferViews;
            AZStd::vector<Data::Instance<ShaderResourceGroup>> m_cachedDrawSrg;

            uint32_t m_nextDrawSrgIdx = 0;
            
            // structure includes DrawItem and stream and index buffer index
            using BufferViewIndexType = uint32_t;
            static const BufferViewIndexType InvalidIndex = static_cast<BufferViewIndexType>(-1);
            struct DrawItemInfo
            {
                RHI::DrawItem m_drawItem;
                RHI::DrawItemSortKey m_sortKey;
                BufferViewIndexType m_vertexBufferViewIndex = InvalidIndex;
                BufferViewIndexType m_indexBufferViewIndex = InvalidIndex;
            };
            AZStd::vector<DrawItemInfo> m_cachedDrawItems;

            // Cached draw list for render to rasterpass
            RHI::DrawList m_cachedDrawList;

            // Flags if this DynamicDrawContext can change shader variants
            bool m_supportShaderVariants = false;
            ShaderVariantId m_currentShaderVariantId;

            Data::Instance<Shader> m_shader;

            // This variable is used to see if the context is initialized.
            // You can only add draw calls when the context is initialized.
            bool m_initialized = false;

            RHI::DrawItemSortKey m_sortKey = 0;

            bool m_drawFinalized = false;
        };

        AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::RPI::DynamicDrawContext::DrawStateOptions);
    }
}
