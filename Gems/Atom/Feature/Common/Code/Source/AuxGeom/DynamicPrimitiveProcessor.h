/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI/IndexBufferView.h>
#include <Atom/RHI/StreamBufferView.h>
#include <Atom/RHI/DevicePipelineState.h>
#include <Atom/RHI/DrawList.h>
#include <Atom/RHI/IndexBufferView.h>
#include <Atom/RHI/GeometryView.h>
#include <Atom/RHI/PipelineState.h>
#include <Atom/RHI/StreamBufferView.h>

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
            AZ_CLASS_ALLOCATOR(DynamicPrimitiveProcessor, AZ::SystemAllocator);

            DynamicPrimitiveProcessor() = default;
            ~DynamicPrimitiveProcessor() = default;

            //! Initialize the DynamicPrimitiveProcessor and all its buffers, shaders, stream layouts etc
            bool Initialize(const AZ::RPI::Scene* scene);

            //! Releases the DynamicPrimitiveProcessor and all primitive geometry buffers
            void Release();

            //! Process the list of primitives in the buffer data and add them to the views in the feature processor packet
            void ProcessDynamicPrimitives(AuxGeomBufferData* bufferData, const RPI::FeatureProcessor::RenderPacket& fpPacket);

            //! Prepare frame.
            void PrepareFrame();
                        
            //! Do any cleanup after current frame is rendered.
            void FrameEnd();

            //! Notify this DynamicPrimitiveProcessor to update its pipeline states
            void SetUpdatePipelineStates();

        private: // types

            using StreamBufferViewsForAllStreams = AZStd::fixed_vector<AZ::RHI::StreamBufferView, AZ::RHI::Limits::Pipeline::StreamCountMax>;

            using DrawPackets = AZStd::vector<AZ::RHI::ConstPtr<RHI::DrawPacket>>;

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
            RHI::ConstPtr<RHI::DrawPacket> BuildDrawPacketForDynamicPrimitive(
                RHI::GeometryView& geometryView,
                const RPI::Ptr<RPI::PipelineStateForDraw>& pipelineState,
                Data::Instance<RPI::ShaderResourceGroup> srg,
                RHI::DrawPacketBuilder& drawPacketBuilder,
                RHI::DrawItemSortKey sortKey = 0);

            // Update a dynamic index buffer, given the data from draw requests
            bool UpdateIndexBuffer(const IndexBuffer& indexSource);

            // Update a dynamic vertex buffer, given the data from draw requests
            bool UpdateVertexBuffer(const VertexBuffer& source);

            // Validate the given stream buffer views for the layout used for the given prim type (uses isValidated flags to see if necessary)
            void ValidateStreamBufferViews(AZStd::span<const RHI::StreamBufferView> streamBufferViews, bool* isValidated, int primitiveType);

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

            // Buffers for all primitives
            RHI::GeometryView m_geometryView;

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
