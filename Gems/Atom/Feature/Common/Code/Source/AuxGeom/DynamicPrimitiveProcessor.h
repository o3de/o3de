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

#pragma once

#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/BufferPool.h>
#include <Atom/RHI/IndexBufferView.h>
#include <Atom/RHI/StreamBufferView.h>
#include <Atom/RHI/PipelineState.h>
#include <Atom/RHI/DrawList.h>

#include <Atom/RHI.Reflect/InputStreamLayout.h>
#include <Atom/RHI.Reflect/Limits.h>

#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/RPI.Public/PipelineState.h>
#include <AzCore/std/containers/fixed_vector.h>

#include "AuxGeomBase.h"

namespace AZ
{
    namespace RHI
    {
        class DrawPacketBuilder;
    }

    namespace RPI
    {
        class Scene;
        class Shader;
        class ShaderVariant;
        class ShaderOptionGroup;
        class ShaderResourceGroup;
    }

    namespace Render
    {
        /**
         * DynamicPrimitiveProcessor does the feature processor work for dynamic primitives.
         * That is, primitives drawn using dynamic buffers for verts and indices.
         * This class, manages the dynamic RHI buffers, the stream layout, the shader asset
         * and the pipeline states.
         */
        class DynamicPrimitiveProcessor final
        {
        public:
            AZ_TYPE_INFO(DynamicPrimitiveProcessor, "{30391207-E4CB-4FCC-B407-05E361CF6815}");
            AZ_CLASS_ALLOCATOR(DynamicPrimitiveProcessor, AZ::SystemAllocator, 0);

            DynamicPrimitiveProcessor() = default;
            ~DynamicPrimitiveProcessor() = default;

            //! Initialize the DynamicPrimitiveProcessor and all its buffers, shaders, stream layouts etc
            bool Initialize(AZ::RHI::Device& rhiDevice, const AZ::RPI::Scene* scene);

            //! Releases the DynamicPrimitiveProcessor and all primitive geometry buffers
            void Release();

            //! Process the list of primitives in the buffer data and add them to the views in the feature processor packet
            void ProcessDynamicPrimitives(const AuxGeomBufferData* bufferData, const RPI::FeatureProcessor::RenderPacket& fpPacket);

            //! do any cleanup from last frame.
            void PrepareFrame();

            //! Notify this DynamicPrimitiveProcessor to update its pipeline states
            void SetUpdatePipelineStates();

        private: // types

            using StreamBufferViewsForAllStreams = AZStd::fixed_vector<AZ::RHI::StreamBufferView, AZ::RHI::Limits::Pipeline::StreamCountMax>;

            struct DynamicBufferGroup
            {
                //! The index buffer for this set of primitives
                AZ::RHI::Ptr<AZ::RHI::Buffer> m_indexBuffer;

                //! The vertices for this set of primitives
                AZ::RHI::Ptr<AZ::RHI::Buffer> m_vertexBuffer;

                //! The view into the index buffer
                AZ::RHI::IndexBufferView m_indexBufferView;

                //! The stream views into the vertex buffer (we only have one in our case)
                StreamBufferViewsForAllStreams m_streamBufferViews;
            };

            using DrawPackets = AZStd::vector<AZStd::unique_ptr<const RHI::DrawPacket>>;

            struct ShaderData
            {
                RHI::Ptr<RHI::ShaderResourceGroupLayout> m_perDrawSrgLayout;
                Data::Instance<RPI::ShaderResourceGroup> m_defaultSRG; // default SRG for draws not overriding the view projection matrix
                AZ::RHI::DrawListTag m_drawListTag; // The draw list tag from our shader variant (determines which views primitives are in and which pass)

                AZ::RHI::ShaderInputNameIndex m_viewProjectionOverrideIndex = "m_viewProjectionOverride";
                AZ::RHI::ShaderInputNameIndex m_pointSizeIndex = "m_pointSize";
            };

            struct PipelineStateOptions
            {
                AuxGeomShapePerpectiveType m_perpectiveType = PerspectiveType_ViewProjection;
                AuxGeomBlendMode m_blendMode = BlendMode_Alpha;
                AuxGeomPrimitiveType m_primitiveType = PrimitiveType_TriangleList;
                AuxGeomDepthReadType m_depthReadType = DepthRead_On;
                AuxGeomDepthWriteType m_depthWriteType = DepthWrite_Off;
                AuxGeomFaceCullMode m_faceCullMode = FaceCull_Back;
            };

        private: // functions

            //!Uses the given drawPacketBuilder to build a draw packet with given data and returns it
            const RHI::DrawPacket* BuildDrawPacketForDynamicPrimitive(
                DynamicBufferGroup& group,
                const RPI::Ptr<RPI::PipelineStateForDraw>& pipelineState,
                Data::Instance<RPI::ShaderResourceGroup> srg,
                uint32_t indexCount,
                uint32_t indexOffset,
                RHI::DrawPacketBuilder& drawPacketBuilder,
                RHI::DrawItemSortKey sortKey = 0);

            // Creates the dynamic buffers
            bool CreateBuffers();

            // Destroy all the buffers
            void DestroyBuffers();

            // Creates the dynamic buffers in a group
            bool CreateBufferGroup(DynamicBufferGroup& group);

            // Destroy all the buffers in a group
            void DestroyBufferGroup(DynamicBufferGroup& group);

            // Helper function to update a buffer
            void UpdateBuffer(const uint8_t* source, size_t sourceSize, RHI::Ptr<RHI::Buffer> buffer);

            // Update a dynamic index buffer, given the data from draw requests
            void UpdateIndexBuffer(const IndexBuffer& indexSource, DynamicBufferGroup& group);

            // Update a dynamic vertex buffer, given the data from draw requests
            void UpdateVertexBuffer(const VertexBuffer& source, DynamicBufferGroup& group);

            // Validate the given stream buffer views for the layout used for the given prim type (uses isValidated flags to see if necessary)
            void ValidateStreamBufferViews(StreamBufferViewsForAllStreams& streamBufferViews, bool* isValidated, int primitiveType);

            // Sets up stream layout used for dynamic primitive shader for the given toplogy 
            void SetupInputStreamLayout(RHI::InputStreamLayout& inputStreamLayout, RHI::PrimitiveTopology topology);

            // Loads the shader used for dynamic primitives
            void InitShader();

            void InitPipelineState(const PipelineStateOptions& pipelineStateOptions);

            RPI::Ptr<RPI::PipelineStateForDraw>& GetPipelineState(const PipelineStateOptions& pipelineStateOptions);

        private: // data

            // We have a layout for each prim type because the layout contains the topology type
            RHI::InputStreamLayout m_inputStreamLayout[PrimitiveType_Count];

            // The pipeline state for processing opaque dynamic primitives
            RPI::Ptr<RPI::PipelineStateForDraw> m_pipelineStates[PerspectiveType_Count][BlendMode_Count][PrimitiveType_Count][DepthRead_Count][DepthWrite_Count][FaceCull_Count];
            AZStd::list<RPI::Ptr<RPI::PipelineStateForDraw>*> m_createdPipelineStates;

            ShaderData m_shaderData;

            // The buffer pool that manages all our dynamic index and vertex buffers
            RHI::Ptr<AZ::RHI::BufferPool> m_hostPool;

            // Buffers for all primitives
            DynamicBufferGroup m_primitiveBuffers;

            // Flags to see if stream buffer views have been validated for a prim type's layout
            bool m_streamBufferViewsValidatedForLayout[PrimitiveType_Count];

            // We keep all the draw packets around until the next time Process is called
            DrawPackets m_drawPackets;

            // We keep all the srg's around until the next time process is called
            AZStd::vector<AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>> m_processSrgs;

            Data::Instance<AZ::RPI::Shader> m_shader;

            const AZ::RPI::Scene* m_scene = nullptr;

            bool m_needUpdatePipelineStates = false;

        };
    } // namespace Render
} // namespace AZ
