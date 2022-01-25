/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/BufferPool.h>
#include <Atom/RHI/IndexBufferView.h>
#include <Atom/RHI/StreamBufferView.h>
#include <Atom/RHI/PipelineState.h>

#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/RPI.Public/PipelineState.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

#include <Atom/RHI.Reflect/InputStreamLayout.h>
#include <Atom/RHI.Reflect/Limits.h>

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
    }

    namespace Render
    {
        /**
         * FixedShapeProcessor does the feature processor work for fixed shapes such as
         * Sphere, Cone, Cylinder.
         * This class, manages setting up the shape buffers, the stream layout, the shader asset
         * and the pipeline states.
         */
        class FixedShapeProcessor final
        {
        public:
            using StreamBufferViewsForAllStreams = AZStd::fixed_vector<AZ::RHI::StreamBufferView, AZ::RHI::Limits::Pipeline::StreamCountMax>;

            using AuxGeomNormal = AuxGeomPosition;

            AZ_TYPE_INFO(FixedShapeProcessor, "{20A11645-F8B1-4BAC-847D-F8F49FD2E339}");
            AZ_CLASS_ALLOCATOR(FixedShapeProcessor, AZ::SystemAllocator, 0);

            FixedShapeProcessor() = default;
            ~FixedShapeProcessor() = default;

            //! Initialize the FixedShapeProcessor and all its buffers, shaders, stream layouts etc
            bool Initialize(AZ::RHI::Device& rhiDevice, const AZ::RPI::Scene* scene);

            //! Releases the FixedShapeProcessor and all buffers
            void Release();

            //! Processes all the fixed shape objects for a frame
            void ProcessObjects(const AuxGeomBufferData* bufferData, const RPI::FeatureProcessor::RenderPacket& fpPacket);

            //! do any cleanup from last frame.
            void PrepareFrame();

            //! Notify this FixedShapeProcessor to update its pipeline states
            void SetUpdatePipelineStates();

        private:

            using LodIndex = uint32_t;
            struct ShaderData; // forward declare internal struct;

            //! We store a struct of this type for each fixed object geometry (both shapes and boxes)
            struct ObjectBuffers
            {
                uint32_t m_pointIndexCount;
                AZ::RHI::Ptr<AZ::RHI::Buffer> m_pointIndexBuffer;
                AZ::RHI::IndexBufferView m_pointIndexBufferView;

                uint32_t m_lineIndexCount;
                AZ::RHI::Ptr<AZ::RHI::Buffer> m_lineIndexBuffer;
                AZ::RHI::IndexBufferView m_lineIndexBufferView;

                uint32_t m_triangleIndexCount;
                AZ::RHI::Ptr<AZ::RHI::Buffer> m_triangleIndexBuffer;
                AZ::RHI::IndexBufferView m_triangleIndexBufferView;

                AZ::RHI::Ptr<AZ::RHI::Buffer> m_positionBuffer;
                AZ::RHI::Ptr<AZ::RHI::Buffer> m_normalBuffer;
                StreamBufferViewsForAllStreams m_streamBufferViews;
                StreamBufferViewsForAllStreams m_streamBufferViewsWithNormals;
            };

            // This is a temporary structure used when building object meshes. The data is then copied into RHI buffers.
            struct MeshData
            {
                AZStd::vector<uint16_t> m_pointIndices; // Use indices because draws are all indexed.
                AZStd::vector<uint16_t> m_lineIndices;
                AZStd::vector<uint16_t> m_triangleIndices;
                AZStd::vector<AuxGeomPosition> m_positions;
                AZStd::vector<AuxGeomNormal> m_normals;
            };

            struct Shape
            {
                LodIndex m_numLods;
                AZStd::vector<ObjectBuffers> m_lodBuffers;
                AZStd::vector<float> m_lodScreenPercentages;
            };

            struct PipelineStateOptions
            {
                AuxGeomShapePerpectiveType m_perpectiveType = PerspectiveType_ViewProjection;
                AuxGeomBlendMode m_blendMode = BlendMode_Alpha;
                AuxGeomDrawStyle m_drawStyle = DrawStyle_Line;
                AuxGeomDepthReadType m_depthReadType = DepthRead_On;
                AuxGeomDepthWriteType m_depthWriteType = DepthWrite_Off;
                AuxGeomFaceCullMode m_faceCullMode = FaceCull_Back;
            };

        private: // functions

            enum class Facing
            {
                Up,
                Down,
                Both,
            };

            bool CreateSphereBuffersAndViews(AuxGeomShapeType sphereShapeType);
            void CreateSphereMeshData(MeshData& meshData, uint32_t numRings, uint32_t numSections, AuxGeomShapeType sphereShapeType);

            bool CreateQuadBuffersAndViews();
            void CreateQuadMeshDataSide(MeshData& meshData, bool isUp, bool drawLines);
            void CreateQuadMeshData(MeshData& meshData, Facing facing = Facing::Up);

            bool CreateDiskBuffersAndViews();
            void CreateDiskMeshDataSide(MeshData& meshData, uint32_t numSections, bool isUp, float yPosition);
            void CreateDiskMeshData(MeshData& meshData, uint32_t numSections, Facing facing = Facing::Up, float yPosition = 0.0f);

            bool CreateConeBuffersAndViews();
            void CreateConeMeshData(MeshData& meshData, uint32_t numRings, uint32_t numSections);

            bool CreateCylinderBuffersAndViews(AuxGeomShapeType cylinderShapeType);
            void CreateCylinderMeshData(MeshData& meshData, uint32_t numSections, AuxGeomShapeType cylinderShapeType);

            bool CreateBoxBuffersAndViews();
            void CreateBoxMeshData(MeshData& meshData);

            bool CreateBuffersAndViews(ObjectBuffers& objectBuffers, const MeshData& meshData);

            LodIndex GetLodIndexForShape(AuxGeomShapeType shapeType, const AZ::RPI::View* view, const AZ::Vector3& worldPosition, const AZ::Vector3& scale);

            void SetupInputStreamLayout(RHI::InputStreamLayout& inputStreamLayout, RHI::PrimitiveTopology topology, bool includeNormals);

            void LoadShaders();
            void FillShaderData(Data::Instance<RPI::Shader>& shader, ShaderData& shaderData);

            void InitPipelineState(const PipelineStateOptions& options);
            RPI::Ptr<RPI::PipelineStateForDraw>& GetPipelineState(const PipelineStateOptions& pipelineStateOptions);

            const AZ::RHI::IndexBufferView& GetShapeIndexBufferView(AuxGeomShapeType shapeType, int drawStyle, LodIndex lodIndex) const;
            const StreamBufferViewsForAllStreams& GetShapeStreamBufferViews(AuxGeomShapeType shapeType, LodIndex lodIndex, int drawStyle) const;
            uint32_t GetShapeIndexCount(AuxGeomShapeType shapeType, int drawStyle, LodIndex lodIndex);

            //! Uses the given drawPacketBuilder to build a draw packet for given shape and state and returns it
            const RHI::DrawPacket* BuildDrawPacketForShape(
                RHI::DrawPacketBuilder& drawPacketBuilder,
                const ShapeBufferEntry& shape,
                int drawStyle,
                const AZStd::vector<AZ::Matrix4x4>& viewProjOverrides,
                const RPI::Ptr<RPI::PipelineStateForDraw>& pipelineState,
                LodIndex lodIndex,
                RHI::DrawItemSortKey sortKey = 0);

            const AZ::RHI::IndexBufferView& GetBoxIndexBufferView(int drawStyle) const;
            const StreamBufferViewsForAllStreams& GetBoxStreamBufferViews(int drawStyle) const;
            uint32_t GetBoxIndexCount(int drawStyle);

            //! Uses the given drawPacketBuilder to build a draw packet for given box and state and returns it
            const RHI::DrawPacket* BuildDrawPacketForBox(
                RHI::DrawPacketBuilder& drawPacketBuilder,
                const BoxBufferEntry& box,
                int drawStyle,
                const AZStd::vector<AZ::Matrix4x4>& overrideViewProjMatrices,
                const RPI::Ptr<RPI::PipelineStateForDraw>& pipelineState,
                RHI::DrawItemSortKey sortKey = 0);

            //! Uses the given drawPacketBuilder to build a draw packet with the given data
            const RHI::DrawPacket* BuildDrawPacket(
                RHI::DrawPacketBuilder& drawPacketBuilder,
                AZ::Data::Instance<RPI::ShaderResourceGroup>& srg,
                uint32_t indexCount,
                const RHI::IndexBufferView& indexBufferView,
                const StreamBufferViewsForAllStreams& streamBufferViews,
                RHI::DrawListTag drawListTag,
                const AZ::RHI::PipelineState* pipelineState,
                RHI::DrawItemSortKey sortKey);

        private: // data

            //! The buffer pool that manages the index and vertex buffers for each shape
            RHI::Ptr<AZ::RHI::BufferPool> m_bufferPool;

            //! The descriptor for drawing an object of each draw style using predefined streams
            RHI::InputStreamLayout m_objectStreamLayout[DrawStyle_Count];
            
            //! Array of shape buffers for all shapes
            AZStd::array<Shape, ShapeType_Count> m_shapes;

            ObjectBuffers m_boxBuffers;

            // not sure what the required lifetime of these is
            AZStd::vector<AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>> m_processSrgs;

            // The PSOs generated by this feature processor
            RPI::Ptr<RPI::PipelineStateForDraw> m_pipelineStates[PerspectiveType_Count][BlendMode_Count][DrawStyle_Count][DepthRead_Count][DepthWrite_Count][FaceCull_Count];
            AZStd::list<RPI::Ptr<RPI::PipelineStateForDraw>*> m_createdPipelineStates;
            Data::Instance<RPI::Shader> m_unlitShader;
            Data::Instance<RPI::Shader> m_litShader;

            enum ShapeLightingStyle
            {
                ShapeLightingStyle_ConstantColor, // color from srg
                ShapeLightingStyle_Directional, // color from srg * dot product(normal, hard coded direction)

                ShapeLightingStyle_Count
            };

            struct ShaderData
            {
                AZ::Data::Asset<AZ::RPI::ShaderAsset> m_shaderAsset; // For @m_perObjectSrgLayout.
                AZ::RPI::SupervariantIndex m_supervariantIndex; // For @m_perObjectSrgLayout.
                AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout> m_perObjectSrgLayout; // Comes from @m_shaderAsset
                AZ::RHI::DrawListTag m_drawListTag;
                AZ::RHI::ShaderInputNameIndex m_colorIndex = "m_color";
                AZ::RHI::ShaderInputNameIndex m_modelToWorldIndex = "m_modelToWorld";
                AZ::RHI::ShaderInputNameIndex m_normalMatrixIndex = "m_normalMatrix";
                AZ::RHI::ShaderInputNameIndex m_viewProjectionOverrideIndex = "m_viewProjectionOverride";
                AZ::RHI::ShaderInputNameIndex m_pointSizeIndex = "m_pointSize";
            };
            ShaderData m_perObjectShaderData[ShapeLightingStyle_Count];
            ShaderData& GetShaderDataForDrawStyle(int drawStyle) {return m_perObjectShaderData[drawStyle == DrawStyle_Shaded];}

            AZStd::vector<AZStd::unique_ptr<const RHI::DrawPacket>> m_drawPackets;

            const AZ::RPI::Scene* m_scene = nullptr;

            bool m_needUpdatePipelineStates = false;
        };
    } // namespace Render
} // namespace AZ
