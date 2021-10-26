/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "FixedShapeProcessor.h"
#include "AuxGeomDrawProcessorShared.h"

#include <AzCore/Debug/EventTrace.h>
#include <AzCore/std/containers/array.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/DrawPacketBuilder.h>

#include <Atom/RHI.Reflect/InputStreamLayoutBuilder.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroup.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderVariant.h>
#include <Atom/RPI.Public/View.h>

namespace AZ
{
    namespace Render
    {
        namespace
        {
            static const char* const ShapePerspectiveTypeViewProjection = "ViewProjectionMode::ViewProjection";
            static const char* const ShapePerspectiveTypeManualOverride = "ViewProjectionMode::ManualOverride";
            AZ::Name GetAuxGeomPerspectiveTypeName(AuxGeomShapePerpectiveType shapePerspectiveType)
            {
                switch (shapePerspectiveType)
                {
                    case PerspectiveType_ViewProjection:
                        return Name(ShapePerspectiveTypeViewProjection);
                    case PerspectiveType_ManualOverride:
                        return Name(ShapePerspectiveTypeManualOverride);
                    default:
                        AZ_Assert(false, "Invalid perspective type value %d", aznumeric_cast<int>(shapePerspectiveType));
                        return Name(ShapePerspectiveTypeViewProjection);
                }
            }
        }

        bool FixedShapeProcessor::Initialize(AZ::RHI::Device& rhiDevice, const AZ::RPI::Scene* scene)
        {
            RHI::BufferPoolDescriptor desc;
            desc.m_heapMemoryLevel = RHI::HeapMemoryLevel::Device;
            desc.m_bindFlags = RHI::BufferBindFlags::InputAssembly;

            m_bufferPool = RHI::Factory::Get().CreateBufferPool();
            m_bufferPool->SetName(Name("AuxGeomFixedShapeBufferPool"));
            RHI::ResultCode resultCode = m_bufferPool->Init(rhiDevice, desc);

            if (resultCode != RHI::ResultCode::Success)
            {
                AZ_Error("FixedShapeProcessor", false, "Failed to initialize AuxGeom fixed shape buffer pool");
                return false;
            }

            SetupInputStreamLayout(m_objectStreamLayout[DrawStyle_Point], RHI::PrimitiveTopology::PointList, false);
            SetupInputStreamLayout(m_objectStreamLayout[DrawStyle_Line], RHI::PrimitiveTopology::LineList, false);
            SetupInputStreamLayout(m_objectStreamLayout[DrawStyle_Solid], RHI::PrimitiveTopology::TriangleList, false);
            SetupInputStreamLayout(m_objectStreamLayout[DrawStyle_Shaded], RHI::PrimitiveTopology::TriangleList, true);

            CreateSphereBuffersAndViews();
            CreateQuadBuffersAndViews();
            CreateDiskBuffersAndViews();
            CreateConeBuffersAndViews();
            CreateCylinderBuffersAndViews();
            CreateBoxBuffersAndViews();

            // cache scene pointer for RHI::PipelineState creation.
            m_scene = scene;

            LoadShaders();

            return true;
        }

        void FixedShapeProcessor::Release()
        {
            if (m_bufferPool)
            {
                m_bufferPool.reset();
            }

            m_processSrgs.clear();
            m_drawPackets.clear();

            m_litShader = nullptr;
            m_unlitShader = nullptr;
            m_scene = nullptr;

            for (RPI::Ptr<RPI::PipelineStateForDraw>* pipelineState : m_createdPipelineStates)
            {
                pipelineState->reset();
            }
            m_createdPipelineStates.clear();
            m_needUpdatePipelineStates = false;
        }

        void FixedShapeProcessor::PrepareFrame()
        {
            AZ_PROFILE_SCOPE(AzRender, "FixedShapeProcessor: PrepareFrame");
            m_processSrgs.clear();
            m_drawPackets.clear();

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

        void FixedShapeProcessor::ProcessObjects(const AuxGeomBufferData* bufferData, const RPI::FeatureProcessor::RenderPacket& fpPacket)
        {
            AZ_PROFILE_SCOPE(AzRender, "FixedShapeProcessor: ProcessObjects");

            RHI::DrawPacketBuilder drawPacketBuilder;

            // Draw opaque shapes with LODs. This requires a separate draw packet per shape per view that it is in (usually only one)

            // We do each draw style together to reduce state changes
            for (int drawStyle = 0; drawStyle < DrawStyle_Count; ++drawStyle)
            {
                auto drawListTag = GetShaderDataForDrawStyle(drawStyle).m_drawListTag;

                // Skip this draw style if the owner scene doesn't have this drawListTag (which means this FP won't even create the RHI PipelineState for draw)
                if (!m_scene->HasOutputForPipelineState(drawListTag))
                {
                    return;
                }

                // Draw all of the opaque shapes of this draw style
                // Possible TODO: Batch together shapes of the same type and LOD using instanced draw [ATOM-1032]
                // Note that this optimization may not be worth it for shapes because of LODs
                for (const auto& shape : bufferData->m_opaqueShapes[drawStyle])
                {
                    PipelineStateOptions pipelineStateOptions;
                    pipelineStateOptions.m_perpectiveType = (AuxGeomShapePerpectiveType)(shape.m_viewProjOverrideIndex >= 0);
                    pipelineStateOptions.m_blendMode = BlendMode_Off;
                    pipelineStateOptions.m_drawStyle = (AuxGeomDrawStyle)drawStyle;
                    pipelineStateOptions.m_depthReadType = shape.m_depthRead;
                    pipelineStateOptions.m_depthWriteType = shape.m_depthWrite;
                    pipelineStateOptions.m_faceCullMode = shape.m_faceCullMode;

                    RPI::Ptr<RPI::PipelineStateForDraw> pipelineState = GetPipelineState(pipelineStateOptions);

                    const AZ::Vector3 position = shape.m_position;
                    const AZ::Vector3 scale = shape.m_scale;

                    for (auto& view : fpPacket.m_views)
                    {
                        // If this view is ignoring packets with our draw list tag then skip this view
                        if (!view->HasDrawListTag(drawListTag))
                        {
                            continue;
                        }
                        LodIndex lodIndex = GetLodIndexForShape(shape.m_shapeType, view.get(), position, scale);
                        const RHI::DrawPacket* drawPacket = BuildDrawPacketForShape(
                            drawPacketBuilder, shape, drawStyle, bufferData->m_viewProjOverrides, pipelineState, lodIndex);
                        if (drawPacket)
                        {
                            m_drawPackets.emplace_back(drawPacket);
                            view->AddDrawPacket(drawPacket);
                        }
                    }
                }

                for (const auto& box : bufferData->m_opaqueBoxes[drawStyle])
                {
                    PipelineStateOptions pipelineStateOptions;
                    pipelineStateOptions.m_perpectiveType = (AuxGeomShapePerpectiveType)(box.m_viewProjOverrideIndex >= 0);
                    pipelineStateOptions.m_blendMode = BlendMode_Off;
                    pipelineStateOptions.m_drawStyle = (AuxGeomDrawStyle)drawStyle;
                    pipelineStateOptions.m_depthReadType = box.m_depthRead;
                    pipelineStateOptions.m_depthWriteType = box.m_depthWrite;
                    pipelineStateOptions.m_faceCullMode = box.m_faceCullMode;

                    RPI::Ptr<RPI::PipelineStateForDraw> pipelineState = GetPipelineState(pipelineStateOptions);

                    const RHI::DrawPacket* drawPacket =
                        BuildDrawPacketForBox(drawPacketBuilder, box, drawStyle, bufferData->m_viewProjOverrides, pipelineState);
                    if (drawPacket)
                    {
                        m_drawPackets.emplace_back(drawPacket);

                        for (auto& view : fpPacket.m_views)
                        {
                            // If this view is ignoring packets with our draw list tag then skip this view
                            if (!view->HasDrawListTag(drawListTag))
                            {
                                continue;
                            }
                            view->AddDrawPacket(drawPacket);
                        }
                    }
                }

            }

            // Draw all of the translucent objects (shapes and boxes) with a distance sort key per view
            // We have to create separate draw packets for each view that the AuxGeom is in (typically only one)
            // because of distance sorting
            for (int drawStyle = 0; drawStyle < DrawStyle_Count; ++drawStyle)
            {
                auto drawListTag = GetShaderDataForDrawStyle(drawStyle).m_drawListTag;

                // Skip this draw style if the owner scene doesn't have this drawListTag (which means this FP won't even create the RHI PipelineState for draw)
                if (!m_scene->HasOutputForPipelineState(drawListTag))
                {
                    return;
                }

                // Draw all the shapes of this draw style
                for (const auto& shape : bufferData->m_translucentShapes[drawStyle])
                {
                    PipelineStateOptions pipelineStateOptions;
                    pipelineStateOptions.m_perpectiveType = (AuxGeomShapePerpectiveType)(shape.m_viewProjOverrideIndex >= 0);
                    pipelineStateOptions.m_blendMode = BlendMode_Alpha;
                    pipelineStateOptions.m_drawStyle = (AuxGeomDrawStyle)drawStyle;
                    pipelineStateOptions.m_depthReadType = shape.m_depthRead;
                    pipelineStateOptions.m_depthWriteType = shape.m_depthWrite;
                    pipelineStateOptions.m_faceCullMode = shape.m_faceCullMode;

                    RPI::Ptr<RPI::PipelineStateForDraw> pipelineState = GetPipelineState(pipelineStateOptions);

                    const AZ::Vector3 position = shape.m_position;
                    const AZ::Vector3 scale = shape.m_scale;
                    for (auto& view : fpPacket.m_views)
                    {
                        // If this view is ignoring packets with our draw list tag then skip this view
                        if (!view->HasDrawListTag(drawListTag))
                        {
                            continue;
                        }
                        RHI::DrawItemSortKey sortKey = view->GetSortKeyForPosition(position);
                        LodIndex lodIndex = GetLodIndexForShape(shape.m_shapeType, view.get(), position, scale);

                        const RHI::DrawPacket* drawPacket = BuildDrawPacketForShape(
                            drawPacketBuilder, shape, drawStyle, bufferData->m_viewProjOverrides, pipelineState, lodIndex, sortKey);
                        if (drawPacket)
                        {
                            m_drawPackets.emplace_back(drawPacket);
                            view->AddDrawPacket(drawPacket);
                        }
                    }
                }

                // Draw all the boxes of this draw style
                for (const auto& box : bufferData->m_translucentBoxes[drawStyle])
                {
                    PipelineStateOptions pipelineStateOptions;
                    pipelineStateOptions.m_perpectiveType = (AuxGeomShapePerpectiveType)(box.m_viewProjOverrideIndex >= 0);
                    pipelineStateOptions.m_blendMode = BlendMode_Alpha;
                    pipelineStateOptions.m_drawStyle = (AuxGeomDrawStyle)drawStyle;
                    pipelineStateOptions.m_depthReadType = box.m_depthRead;
                    pipelineStateOptions.m_depthWriteType = box.m_depthWrite;
                    pipelineStateOptions.m_faceCullMode = box.m_faceCullMode;

                    RPI::Ptr<RPI::PipelineStateForDraw> pipelineState = GetPipelineState(pipelineStateOptions);

                    const AZ::Vector3 position = box.m_position;
                    for (auto& view : fpPacket.m_views)
                    {
                        // If this view is ignoring packets with our draw list tag then skip this view
                        if (!view->HasDrawListTag(drawListTag))
                        {
                            continue;
                        }
                        RHI::DrawItemSortKey sortKey = view->GetSortKeyForPosition(position);
                        const RHI::DrawPacket* drawPacket = BuildDrawPacketForBox(
                            drawPacketBuilder, box, drawStyle, bufferData->m_viewProjOverrides, pipelineState, sortKey);
                        if (drawPacket)
                        {
                            m_drawPackets.emplace_back(drawPacket);
                            view->AddDrawPacket(drawPacket);
                        }
                    }
                }
            }
        }

        bool FixedShapeProcessor::CreateSphereBuffersAndViews()
        {
            const uint32_t numSphereLods = 5;
            struct LodInfo
            {
                uint32_t numRings;
                uint32_t numSections;
                float    screenPercentage;
            };
            const AZStd::array<LodInfo, numSphereLods> lodInfo =
            {{
                { 25, 25, 0.1000f},
                { 21, 21, 0.0100f},
                { 17, 17, 0.0010f},
                { 13, 13, 0.0001f},
                {  9,  9, 0.0000f}
            }};

            auto& m_shape = m_shapes[ShapeType_Sphere];
            m_shape.m_numLods = numSphereLods;

            for (uint32_t lodIndex = 0; lodIndex < numSphereLods; ++lodIndex)
            {
                MeshData meshData;
                CreateSphereMeshData(meshData, lodInfo[lodIndex].numRings, lodInfo[lodIndex].numSections);

                ObjectBuffers objectBuffers;

                if (!CreateBuffersAndViews(objectBuffers, meshData))
                {
                    m_shape.m_numLods = 0;
                    return false;
                }

                m_shape.m_lodBuffers.emplace_back(objectBuffers);
                m_shape.m_lodScreenPercentages.push_back(lodInfo[lodIndex].screenPercentage);
            }

            return true;
        }

        void FixedShapeProcessor::CreateSphereMeshData(MeshData& meshData, uint32_t numRings, uint32_t numSections)
        {
            const float radius = 1.0f;

            // calc required number of vertices/indices/triangles to build a sphere for the given parameters
            uint32_t numVertices = (numRings - 1) * numSections + 2;

            // setup buffers
            auto& positions = meshData.m_positions;
            positions.clear();
            positions.reserve(numVertices);

            auto& normals = meshData.m_normals;
            normals.clear();
            normals.reserve(numVertices);

            using PosType = AuxGeomPosition;
            using NormalType = AuxGeomNormal;

            // 1st pole vertex
            positions.push_back(PosType(0.0f, 0.0f, radius));
            normals.push_back(NormalType(0.0f, 0.0f, 1.0f));

            // calculate "inner" vertices
            float sectionAngle(DegToRad(360.0f / static_cast<float>(numSections)));
            float ringSlice(DegToRad(180.0f / static_cast<float>(numRings)));

            for (uint32_t ring = 1; ring < numRings; ++ring)
            {
                float w(sinf(ring * ringSlice));
                for (uint32_t section = 0; section < numSections; ++section)
                {
                    float x = radius * cosf(section * sectionAngle) * w;
                    float y = radius * sinf(section * sectionAngle) * w;
                    float z = radius * cosf(ring * ringSlice);
                    Vector3 radialVector(x, y, z);
                    positions.push_back(radialVector);
                    normals.push_back(radialVector.GetNormalized());
                }
            }

            // 2nd vertex of pole (for end cap)
            positions.push_back(PosType(0.0f, 0.0f, -radius));
            normals.push_back(NormalType(0.0f, 0.0f, -1.0f));

            // point indices
            {
                auto& indices = meshData.m_pointIndices;
                indices.clear();
                indices.reserve(positions.size());

                for (uint16_t index = 0; index < aznumeric_cast<uint16_t>(positions.size()); ++index)
                {
                    indices.push_back(index);
                }
            }

            // line indices
            {
                const uint32_t numEdges = (numRings - 2) * numSections * 2 + 2 * numSections * 2;
                const uint32_t numLineIndices = numEdges * 2;

                // build "inner" faces
                auto& indices = meshData.m_lineIndices;
                indices.clear();
                indices.reserve(numLineIndices);

                for (uint16_t ring = 0; ring < numRings - 2; ++ring)
                {
                    uint16_t firstVertOfThisRing = static_cast<uint16_t>(1 + ring * numSections);
                    uint16_t firstVertOfNextRing = static_cast<uint16_t>(1 + (ring + 1) * numSections);
                    for (uint16_t section = 0; section < numSections; ++section)
                    {
                        uint32_t nextSection = (section + 1) % numSections;

                        // line around ring
                        indices.push_back(firstVertOfThisRing + section);
                        indices.push_back(static_cast<uint16_t>(firstVertOfThisRing + nextSection));

                        // line around section
                        indices.push_back(firstVertOfThisRing + section);
                        indices.push_back(firstVertOfNextRing + section);
                    }
                }

                // build faces for end caps (to connect "inner" vertices with poles)
                uint16_t firstPoleVert = 0;
                uint16_t firstVertOfFirstRing = static_cast<uint16_t>(1 + (0) * numSections);
                for (uint16_t section = 0; section < numSections; ++section)
                {
                    indices.push_back(firstPoleVert);
                    indices.push_back(firstVertOfFirstRing + section);
                }

                uint16_t lastPoleVert = static_cast<uint16_t>((numRings - 1) * numSections + 1);
                uint16_t firstVertOfLastRing = static_cast<uint16_t>(1 + (numRings - 2) * numSections);
                for (uint16_t section = 0; section < numSections; ++section)
                {
                    indices.push_back(firstVertOfLastRing + section);
                    indices.push_back(lastPoleVert);
                }
            }

            // triangle indices
            {
                const uint32_t numTriangles = (numRings - 2) * numSections * 2  + 2 * numSections;
                const uint32_t numTriangleIndices = numTriangles * 3;

                // build "inner" faces
                auto& indices = meshData.m_triangleIndices;
                indices.clear();
                indices.reserve(numTriangleIndices);

                for (uint32_t ring = 0; ring < numRings - 2; ++ring)
                {
                    uint32_t firstVertOfThisRing = 1 + ring * numSections;
                    uint32_t firstVertOfNextRing = 1 + (ring + 1) * numSections;

                    for (uint32_t section = 0; section < numSections; ++section)
                    {
                        uint32_t nextSection = (section + 1) % numSections;
                        indices.push_back(static_cast<uint16_t>(firstVertOfThisRing + nextSection));
                        indices.push_back(static_cast<uint16_t>(firstVertOfThisRing + section));
                        indices.push_back(static_cast<uint16_t>(firstVertOfNextRing + nextSection));

                        indices.push_back(static_cast<uint16_t>(firstVertOfNextRing + section));
                        indices.push_back(static_cast<uint16_t>(firstVertOfNextRing + nextSection));
                        indices.push_back(static_cast<uint16_t>(firstVertOfThisRing + section));
                    }
                }

                // build faces for end caps (to connect "inner" vertices with poles)
                uint32_t firstPoleVert = 0;
                uint32_t firstVertOfFirstRing = 1 + (0) * numSections;
                for (uint32_t section = 0; section < numSections; ++section)
                {
                    uint32_t nextSection = (section + 1) % numSections;
                    indices.push_back(static_cast<uint16_t>(firstVertOfFirstRing + section));
                    indices.push_back(static_cast<uint16_t>(firstVertOfFirstRing + nextSection));
                    indices.push_back(static_cast<uint16_t>(firstPoleVert));
                }

                uint32_t lastPoleVert = (numRings - 1) * numSections + 1;
                uint32_t firstVertOfLastRing = 1 + (numRings - 2) * numSections;
                for (uint32_t section = 0; section < numSections; ++section)
                {
                    uint32_t nextSection = (section + 1) % numSections;
                    indices.push_back(static_cast<uint16_t>(firstVertOfLastRing + nextSection));
                    indices.push_back(static_cast<uint16_t>(firstVertOfLastRing + section));
                    indices.push_back(static_cast<uint16_t>(lastPoleVert));
                }
            }
        }

        bool FixedShapeProcessor::CreateQuadBuffersAndViews()
        {
            auto& m_shape = m_shapes[ShapeType_Quad];
            m_shape.m_numLods = 1;

            MeshData meshData;
            CreateQuadMeshData(meshData, Facing::Both);

            ObjectBuffers objectBuffers;

            if (!CreateBuffersAndViews(objectBuffers, meshData))
            {
                m_shape.m_numLods = 0;
                return false;
            }

            m_shape.m_lodBuffers.emplace_back(objectBuffers);
            m_shape.m_lodScreenPercentages.push_back(0.0f);

            return true;
        }

        void FixedShapeProcessor::CreateQuadMeshDataSide(MeshData& meshData, bool isUp, bool drawLines)
        {
            uint16_t startPos = aznumeric_cast<uint16_t>(meshData.m_positions.size());

            // Positions
            meshData.m_positions.push_back(AuxGeomPosition(-0.5f, 0.0f,  0.5f));
            meshData.m_positions.push_back(AuxGeomPosition( 0.5f, 0.0f,  0.5f));
            meshData.m_positions.push_back(AuxGeomPosition(-0.5f, 0.0f, -0.5f));
            meshData.m_positions.push_back(AuxGeomPosition( 0.5f, 0.0f, -0.5f));

            // Normals
            AuxGeomNormal normal(0.0f, isUp ? 1.0f : -1.0f, 0.0f);
            meshData.m_normals.insert(meshData.m_normals.end(), { normal, normal, normal, normal });

            // Triangles
            if (isUp)
            {
                meshData.m_triangleIndices.insert(meshData.m_triangleIndices.end(), { 1, 2, 0, 3, 2, 1 });
            }
            else
            {
                meshData.m_triangleIndices.insert(meshData.m_triangleIndices.end(), { 0, 2, 1, 1, 2, 3 });
            }

            // Update indices based on starting position of vertex.
            for (size_t index = meshData.m_triangleIndices.size(); index < meshData.m_triangleIndices.size(); ++index)
            {
                meshData.m_triangleIndices.at(index) += startPos;
            }

            // Lines
            if (drawLines)
            {
                meshData.m_lineIndices.insert(meshData.m_lineIndices.end(), { 0, 1, 1, 2, 2, 3, 3, 0 });
                meshData.m_pointIndices.insert(meshData.m_pointIndices.end(), { 0, 1, 2, 3 });
            }
        }

        void FixedShapeProcessor::CreateQuadMeshData(MeshData& meshData, Facing facing)
        {
            if (facing == Facing::Up || facing == Facing::Both)
            {
                const bool isUp = true;
                const bool drawLines = true;
                CreateQuadMeshDataSide(meshData, isUp, drawLines);
            }
            if (facing == Facing::Down || facing == Facing::Both)
            {
                const bool isUp = false;
                const bool drawLines = facing != Facing::Both;
                CreateQuadMeshDataSide(meshData, isUp, drawLines);
            }
        }

        bool FixedShapeProcessor::CreateDiskBuffersAndViews()
        {
            const uint32_t numDiskLods = 5;
            struct LodInfo
            {
                uint32_t numSections;
                float    screenPercentage;
            };
            const AZStd::array<LodInfo, numDiskLods> lodInfo =
            {{
                {38, 0.1000f},
                {22, 0.0100f},
                {14, 0.0010f},
                {10, 0.0001f},
                { 8, 0.0000f}
            }};

            auto& m_shape = m_shapes[ShapeType_Disk];
            m_shape.m_numLods = numDiskLods;

            for (uint32_t lodIndex = 0; lodIndex < numDiskLods; ++lodIndex)
            {
                MeshData meshData;
                CreateDiskMeshData(meshData, lodInfo[lodIndex].numSections, Facing::Both);

                ObjectBuffers objectBuffers;

                if (!CreateBuffersAndViews(objectBuffers, meshData))
                {
                    m_shape.m_numLods = 0;
                    return false;
                }

                m_shape.m_lodBuffers.emplace_back(objectBuffers);
                m_shape.m_lodScreenPercentages.push_back(lodInfo[lodIndex].screenPercentage);
            }

            return true;
        }

        void FixedShapeProcessor::CreateDiskMeshDataSide(MeshData& meshData, uint32_t numSections, bool isUp, float yPosition)
        {
            AuxGeomNormal normal(0.0f, isUp ? 1.0f : -1.0f, 0.0f);

            // Create center position
            uint16_t centerIndex = aznumeric_cast<uint16_t>(meshData.m_positions.size());
            uint16_t firstSection = centerIndex + 1;

            meshData.m_positions.push_back(AuxGeomPosition(0.0f, yPosition, 0.0f));
            meshData.m_normals.push_back(normal);

            // Create ring around it
            const float radius = 1.0f;
            float sectionAngle(DegToRad(360.0f / (float)numSections));
            for (uint32_t section = 0; section < numSections; ++section)
            {
                meshData.m_positions.push_back(AuxGeomPosition(radius * cosf(section * sectionAngle), yPosition, radius * sinf(section * sectionAngle)));
                meshData.m_normals.push_back(normal);
            }

            // Create point indices
            for (uint16_t index = 0; index < aznumeric_cast<uint16_t>(meshData.m_positions.size()); ++index)
            {
                meshData.m_pointIndices.push_back(index);
            }

            // Create line indices
            for (uint32_t section = 0; section < numSections; ++section)
            {
                // Line from center of disk to outer edge
                meshData.m_lineIndices.push_back(centerIndex);
                meshData.m_lineIndices.push_back(static_cast<uint16_t>(firstSection + section));

                // Line from outer edge to next edge
                meshData.m_lineIndices.push_back(static_cast<uint16_t>(firstSection + section));
                uint32_t nextSection = (section + 1) % numSections;
                meshData.m_lineIndices.push_back(static_cast<uint16_t>(firstSection + nextSection));
            }

            // Create triangle indices
            for (uint32_t section = 0; section < numSections; ++section)
            {
                uint32_t nextSection = (section + 1) % numSections;
                meshData.m_triangleIndices.push_back(centerIndex);
                if (isUp)
                {
                    meshData.m_triangleIndices.push_back(static_cast<uint16_t>(firstSection + nextSection));
                    meshData.m_triangleIndices.push_back(static_cast<uint16_t>(firstSection + section));
                }
                else
                {
                    meshData.m_triangleIndices.push_back(static_cast<uint16_t>(firstSection + section));
                    meshData.m_triangleIndices.push_back(static_cast<uint16_t>(firstSection + nextSection));
                }
            }
        }

        void FixedShapeProcessor::CreateDiskMeshData(MeshData& meshData, uint32_t numSections, Facing facing, float yPosition)
        {
            if (facing == Facing::Up || facing == Facing::Both)
            {
                CreateDiskMeshDataSide(meshData, numSections, true, yPosition);
            }
            if (facing == Facing::Down || facing == Facing::Both)
            {
                CreateDiskMeshDataSide(meshData, numSections, false, yPosition);
            }
        }

        bool FixedShapeProcessor::CreateConeBuffersAndViews()
        {
            const uint32_t numConeLods = 5;
            struct LodInfo
            {
                uint32_t numRings;
                uint32_t numSections;
                float    screenPercentage;
            };
            const AZStd::array<LodInfo, numConeLods> lodInfo =
            {{
                { 16, 38, 0.1000f},
                { 8,  22, 0.0100f},
                { 4,  14, 0.0010f},
                { 2,  10, 0.0001f},
                { 1,   8, 0.0000f}
            }};

            auto& m_shape = m_shapes[ShapeType_Cone];
            m_shape.m_numLods = numConeLods;

            for (uint32_t lodIndex = 0; lodIndex < numConeLods; ++lodIndex)
            {
                MeshData meshData;
                CreateConeMeshData(meshData, lodInfo[lodIndex].numRings, lodInfo[lodIndex].numSections);

                ObjectBuffers objectBuffers;

                if (!CreateBuffersAndViews(objectBuffers, meshData))
                {
                    m_shape.m_numLods = 0;
                    return false;
                }

                m_shape.m_lodBuffers.emplace_back(objectBuffers);
                m_shape.m_lodScreenPercentages.push_back(lodInfo[lodIndex].screenPercentage);
            }

            return true;
        }

        void FixedShapeProcessor::CreateConeMeshData(MeshData& meshData, uint32_t numRings, uint32_t numSections)
        {
            AZ_Assert(numRings >= 1, "CreateConeMeshData: at least one ring is required");

            // Because we support DrawStyle::Shaded we need normals. Creating normals for a cone that shade
            // smoothly is actually not trival. One option is to create one vertex for the point with the normal
            // point along the Y axis. This doesn't give good shading anywhere except by the bottom cap.
            // That is what we do when numRings is one.
            // One approach is to create a ring of coincident verts at the point with the correct normals. But
            // that would give non-smooth shading.
            // So we sub-divide the cone into rings, the first subdivision being halfway between the base and the point.

            const float radius = 1.0f;
            const float height = 1.0f;

            // calc required number of vertices to build a cone for the given parameters
            uint32_t numVertices = numRings * numSections + numSections + 2;

            // setup buffers
            auto& positions = meshData.m_positions;
            positions.clear();
            positions.reserve(numVertices);

            auto& normals = meshData.m_normals;
            normals.clear();
            normals.reserve(numVertices);

            // Create bottom cap with normal facing down
            CreateDiskMeshData(meshData, numSections, Facing::Down);

            // Create vertices for the sides, the sides never quite reach the point. There is a single point vertex for that
            float sectionAngle(DegToRad(360.0f / (float)numSections));
            Vector3 conePoint(0.0f, height, 0.0f);
            for (uint32_t section = 0; section < numSections; ++section)
            {
                Vector3 pointOnCapEdge(radius * cosf(section * sectionAngle), 0.0f, radius * sinf(section * sectionAngle));

                Vector3 vecAlongConeSide(conePoint - pointOnCapEdge);
                Vector3 vecAlongCapEdge = pointOnCapEdge.Cross(vecAlongConeSide);

                Vector3 normal = vecAlongConeSide.Cross(vecAlongCapEdge).GetNormalized();

                float ringDistance = 0.0f;
                float ringSpacing = height * 0.5f;
                for (uint32_t ring = 0; ring < numRings; ++ring)
                {
                    Vector3 pointOnRing = pointOnCapEdge + vecAlongConeSide * ringDistance;
                    positions.push_back(pointOnRing);
                    normals.push_back(normal);

                    ringDistance += ringSpacing;
                    ringSpacing *= 0.5f;
                }
            }

            // cone point vertex
            positions.push_back(conePoint);
            normals.push_back(AuxGeomNormal(0.0f, 1.0f, 0.0f));

            // vertex indexes for start of the cone sides and for the cone point
            uint16_t indexOfSidesStart = static_cast<uint16_t>(numSections + 1);
            uint32_t indexOfConePoint = indexOfSidesStart + numRings * numSections;

            // indices for points
            {
                auto& indices = meshData.m_pointIndices;
                for (uint16_t index = 0; index < aznumeric_cast<uint16_t>(meshData.m_positions.size()); ++index)
                {
                    indices.push_back(index);
                }
            }

            // indices for lines (we ignore the rings beyond the first (at base) when drawing lines)
            {
                auto& indices = meshData.m_lineIndices;

                // build lines between already completed cap for each section
                for (uint16_t section = 0; section < numSections; ++section)
                {
                    indices.push_back(static_cast<uint16_t>(indexOfSidesStart + numRings * section));
                    indices.push_back(static_cast<uint16_t>(indexOfConePoint));
                }
            }

            // indices for triangles
            {
                auto& indices = meshData.m_triangleIndices;

                // build faces
                for (uint16_t section = 0; section < numSections; ++section)
                {
                    uint16_t nextSection = (section + 1) % numSections;

                    // faces from end cap to close to point
                    for (uint32_t ring = 0; ring < numRings - 1; ++ring)
                    {
                        indices.push_back(static_cast<uint16_t>(indexOfSidesStart + numRings * nextSection + ring + 1));
                        indices.push_back(static_cast<uint16_t>(indexOfSidesStart + numRings * nextSection + ring));
                        indices.push_back(static_cast<uint16_t>(indexOfSidesStart + numRings * section + ring));

                        indices.push_back(static_cast<uint16_t>(indexOfSidesStart + numRings * section + ring));
                        indices.push_back(static_cast<uint16_t>(indexOfSidesStart + numRings * section + ring + 1));
                        indices.push_back(static_cast<uint16_t>(indexOfSidesStart + numRings * nextSection + ring + 1));
                    }

                    // faces for point (from last ring of verts to point)
                    indices.push_back(static_cast<uint16_t>(indexOfConePoint));
                    indices.push_back(static_cast<uint16_t>(indexOfSidesStart + numRings * nextSection + numRings - 1));
                    indices.push_back(static_cast<uint16_t>(indexOfSidesStart + numRings * section + numRings - 1));
                }
            }
        }

        bool FixedShapeProcessor::CreateCylinderBuffersAndViews()
        {
            const uint32_t numCylinderLods = 5;
            struct LodInfo
            {
                uint32_t numSections;
                float    screenPercentage;
            };
            const AZStd::array<LodInfo, numCylinderLods> lodInfo =
            {{
                { 38, 0.1000f},
                { 22, 0.0100f},
                { 14, 0.0010f},
                { 10, 0.0001f},
                {  8, 0.0000f}
            }};

            auto& m_shape = m_shapes[ShapeType_Cylinder];
            m_shape.m_numLods = numCylinderLods;

            for (uint32_t lodIndex = 0; lodIndex < numCylinderLods; ++lodIndex)
            {
                MeshData meshData;
                CreateCylinderMeshData(meshData, lodInfo[lodIndex].numSections);

                ObjectBuffers objectBuffers;

                if (!CreateBuffersAndViews(objectBuffers, meshData))
                {
                    m_shape.m_numLods = 0;
                    return false;
                }

                m_shape.m_lodBuffers.emplace_back(objectBuffers);
                m_shape.m_lodScreenPercentages.push_back(lodInfo[lodIndex].screenPercentage);
            }

            return true;
        }

        void FixedShapeProcessor::CreateCylinderMeshData(MeshData& meshData, uint32_t numSections)
        {
            const float radius = 1.0f;
            const float height = 1.0f;

            // calc required number of vertices to build a cylinder for the given parameters
            uint32_t numVertices = 4 * numSections + 2;

            // setup buffers
            auto& positions = meshData.m_positions;
            positions.clear();
            positions.reserve(numVertices);

            auto& normals = meshData.m_normals;
            normals.clear();
            normals.reserve(numVertices);

            float bottomHeight = -height * 0.5f;
            float topHeight = height * 0.5f;

            // Create caps
            CreateDiskMeshData(meshData, numSections, Facing::Down, bottomHeight);
            CreateDiskMeshData(meshData, numSections, Facing::Up, topHeight);

            // create vertices for side (so normal points out correctly)
            float sectionAngle(DegToRad(360.0f / (float)numSections));
            for (uint32_t section = 0; section < numSections; ++section)
            {
                Vector3 bottom(radius * cosf(section * sectionAngle), bottomHeight, radius * sinf(section * sectionAngle));
                Vector3 top = bottom + Vector3(0.0f, height, 0.0f);
                Vector3 normal = bottom.GetNormalized();

                positions.push_back(bottom);
                normals.push_back(normal);

                positions.push_back(top);
                normals.push_back(normal);
            }

            //uint16_t indexOfBottomCenter = 0;
            //uint16_t indexOfBottomStart = 1;
            //uint16_t indexOfTopCenter = numSections + 1;
            //uint16_t indexOfTopStart = numSections + 2;
            uint16_t indexOfSidesStart = static_cast<uint16_t>(2 * numSections + 2);

            // build point indices
            {
                auto& indices = meshData.m_pointIndices;
                for (uint16_t index = 0; index < aznumeric_cast<uint16_t>(positions.size()); ++index)
                {
                    indices.push_back(index);
                }
            }

            // build lines for each section between the already created caps
            {
                auto& indices = meshData.m_lineIndices;
                for (uint16_t section = 0; section < numSections; ++section)
                {
                    // line between the caps
                    indices.push_back(indexOfSidesStart + 2 * section);
                    indices.push_back(indexOfSidesStart + 2 * section + 1);
                }
            }

            // indices for triangles
            {
                auto& indices = meshData.m_triangleIndices;

                // build faces for end cap
                for (uint16_t section = 0; section < numSections; ++section)
                {
                    uint16_t nextSection = (section + 1) % numSections;

                    // face from end cap to point
                    indices.push_back(indexOfSidesStart + 2 * nextSection + 1);
                    indices.push_back(indexOfSidesStart + 2 * nextSection);
                    indices.push_back(indexOfSidesStart + 2 * section);

                    indices.push_back(indexOfSidesStart + 2 * section);
                    indices.push_back(indexOfSidesStart + 2 * section + 1);
                    indices.push_back(indexOfSidesStart + 2 * nextSection + 1);
                }
            }
        }

        bool FixedShapeProcessor::CreateBoxBuffersAndViews()
        {
            MeshData meshData;
            CreateBoxMeshData(meshData);

            if (!CreateBuffersAndViews(m_boxBuffers, meshData))
            {
                return false;
            }

            return true;
        }

        void FixedShapeProcessor::CreateBoxMeshData(MeshData& meshData)
        {
            // calc required number of vertices/indices/triangles to build a sphere for the given parameters
            const uint32_t numVertices = 24;
            const uint32_t numTriangles = 12;
            const uint32_t numEdges = 12;
            const uint32_t numTriangleIndices = numTriangles * 3;
            const uint32_t numLineIndices = numEdges * 2;


            // setup vertex buffer
            auto& positions = meshData.m_positions;
            positions.clear();
            positions.reserve(numVertices);

            auto& normals = meshData.m_normals;
            normals.clear();
            normals.reserve(numVertices);

            using PosType = AuxGeomPosition;
            using NormalType = AuxGeomNormal;

            const uint32_t numVertsPerFace = 4;

            // Front face verts (looking along negative z-axis)
            positions.push_back(PosType(-0.5f, -0.5f,  0.5f));
            positions.push_back(PosType( 0.5f, -0.5f,  0.5f));
            positions.push_back(PosType( 0.5f,  0.5f,  0.5f));
            positions.push_back(PosType(-0.5f,  0.5f,  0.5f));

            for (uint32_t vertex = 0; vertex < numVertsPerFace; ++vertex)
            {
                normals.push_back(NormalType(0.0, 0.0f, 1.0f));
            }

            // Back Face verts
            positions.push_back(PosType(-0.5f, -0.5f, -0.5f));
            positions.push_back(PosType( 0.5f, -0.5f, -0.5f));
            positions.push_back(PosType( 0.5f,  0.5f, -0.5f));
            positions.push_back(PosType(-0.5f,  0.5f, -0.5f));

            for (uint32_t vertex = 0; vertex < numVertsPerFace; ++vertex)
            {
                normals.push_back(NormalType(0.0, 0.0f, -1.0f));
            }

            // Left Face verts
            positions.push_back(PosType(-0.5f, -0.5f,  0.5f));
            positions.push_back(PosType(-0.5f,  0.5f,  0.5f));
            positions.push_back(PosType(-0.5f,  0.5f, -0.5f));
            positions.push_back(PosType(-0.5f, -0.5f, -0.5f));

            for (uint32_t vertex = 0; vertex < numVertsPerFace; ++vertex)
            {
                normals.push_back(NormalType(-1.0, 0.0f, 0.0f));
            }

            // Right Face verts
            positions.push_back(PosType(0.5f, -0.5f,  0.5f));
            positions.push_back(PosType(0.5f,  0.5f,  0.5f));
            positions.push_back(PosType(0.5f,  0.5f, -0.5f));
            positions.push_back(PosType(0.5f, -0.5f, -0.5f));

            for (uint32_t vertex = 0; vertex < numVertsPerFace; ++vertex)
            {
                normals.push_back(NormalType(1.0, 0.0f, 0.0f));
            }

            // Bottom Face verts
            positions.push_back(PosType(-0.5f, -0.5f,  0.5f));
            positions.push_back(PosType( 0.5f, -0.5f,  0.5f));
            positions.push_back(PosType( 0.5f, -0.5f, -0.5f));
            positions.push_back(PosType(-0.5f, -0.5f, -0.5f));

            for (uint32_t vertex = 0; vertex < numVertsPerFace; ++vertex)
            {
                normals.push_back(NormalType(0.0, -1.0f, 0.0f));
            }

            // Top Face verts
            positions.push_back(PosType(-0.5f, 0.5f,  0.5f));
            positions.push_back(PosType( 0.5f, 0.5f,  0.5f));
            positions.push_back(PosType( 0.5f, 0.5f, -0.5f));
            positions.push_back(PosType(-0.5f, 0.5f, -0.5f));

            for (uint32_t vertex = 0; vertex < numVertsPerFace; ++vertex)
            {
                normals.push_back(NormalType(0.0, 1.0f, 0.0f));
            }

            // Setup point index buffer
            {
                auto& indices = meshData.m_pointIndices;
                indices.clear();
                indices.reserve(8);

                // indices - front face points
                indices.push_back(0);
                indices.push_back(1);
                indices.push_back(2);
                indices.push_back(3);

                // indices - back face points
                indices.push_back(4);
                indices.push_back(5);
                indices.push_back(6);
                indices.push_back(7);
            }

            // Setup line index buffer
            {
                auto& indices = meshData.m_lineIndices;
                indices.clear();
                indices.reserve(numLineIndices);

                // indices - front face edges
                indices.push_back(0);
                indices.push_back(1);
                indices.push_back(1);
                indices.push_back(2);
                indices.push_back(2);
                indices.push_back(3);
                indices.push_back(3);
                indices.push_back(0);

                // indices - back face edges
                indices.push_back(4);
                indices.push_back(5);
                indices.push_back(5);
                indices.push_back(6);
                indices.push_back(6);
                indices.push_back(7);
                indices.push_back(7);
                indices.push_back(4);

                // indices - side edges
                indices.push_back(0);
                indices.push_back(4);
                indices.push_back(1);
                indices.push_back(5);
                indices.push_back(2);
                indices.push_back(6);
                indices.push_back(3);
                indices.push_back(7);
            }

            // Setup triangle index buffer
            {
                auto& indices = meshData.m_triangleIndices;
                indices.clear();
                indices.reserve(numTriangleIndices);

                // indices - front face
                indices.push_back(0);
                indices.push_back(1);
                indices.push_back(2);
                indices.push_back(2);
                indices.push_back(3);
                indices.push_back(0);

                // indices - back face
                indices.push_back(5);
                indices.push_back(4);
                indices.push_back(7);
                indices.push_back(7);
                indices.push_back(6);
                indices.push_back(5);

                // indices - left face
                indices.push_back(8);
                indices.push_back(9);
                indices.push_back(10);
                indices.push_back(10);
                indices.push_back(11);
                indices.push_back(8);

                // indices - right face
                indices.push_back(14);
                indices.push_back(13);
                indices.push_back(12);
                indices.push_back(12);
                indices.push_back(15);
                indices.push_back(14);

                // indices - bottom face
                indices.push_back(18);
                indices.push_back(17);
                indices.push_back(16);
                indices.push_back(16);
                indices.push_back(19);
                indices.push_back(18);

                // indices - top face
                indices.push_back(23);
                indices.push_back(20);
                indices.push_back(21);
                indices.push_back(21);
                indices.push_back(22);
                indices.push_back(23);
            }
        }

        bool FixedShapeProcessor::CreateBuffersAndViews(ObjectBuffers& objectBuffers, const MeshData& meshData)
        {
            AZ::RHI::ResultCode result = AZ::RHI::ResultCode::Fail;

            AZ::RHI::BufferInitRequest request;

            // setup m_pointIndexBuffer
            objectBuffers.m_pointIndexBuffer = AZ::RHI::Factory::Get().CreateBuffer();
            const auto pointIndexDataSize = static_cast<uint32_t>(meshData.m_pointIndices.size() * sizeof(uint16_t));

            request.m_buffer = objectBuffers.m_pointIndexBuffer.get();
            request.m_descriptor = AZ::RHI::BufferDescriptor{ AZ::RHI::BufferBindFlags::InputAssembly, pointIndexDataSize };
            request.m_initialData = meshData.m_pointIndices.data();
            result = m_bufferPool->InitBuffer(request);

            if (result != AZ::RHI::ResultCode::Success)
            {
                AZ_Error( "FixedShapeProcessor", false, "Failed to initialize shape index buffer with error code: %d", result);
                return false;
            }

            // setup m_lineIndexBuffer
            objectBuffers.m_lineIndexBuffer = AZ::RHI::Factory::Get().CreateBuffer();
            const auto lineIndexDataSize = static_cast<uint32_t>(meshData.m_lineIndices.size() * sizeof(uint16_t));

            request.m_buffer = objectBuffers.m_lineIndexBuffer.get();
            request.m_descriptor = AZ::RHI::BufferDescriptor{ AZ::RHI::BufferBindFlags::InputAssembly, lineIndexDataSize };
            request.m_initialData = meshData.m_lineIndices.data();
            result = m_bufferPool->InitBuffer(request);

            if (result != AZ::RHI::ResultCode::Success)
            {
                AZ_Error("FixedShapeProcessor", false, "Failed to initialize shape index buffer with error code: %d", result);
                return false;
            }

            // setup m_triangleIndexBuffer
            objectBuffers.m_triangleIndexBuffer = AZ::RHI::Factory::Get().CreateBuffer();
            const auto triangleIndexDataSize = static_cast<uint32_t>(meshData.m_triangleIndices.size() * sizeof(uint16_t));
            request.m_buffer = objectBuffers.m_triangleIndexBuffer.get();
            request.m_descriptor = AZ::RHI::BufferDescriptor{ AZ::RHI::BufferBindFlags::InputAssembly, triangleIndexDataSize };
            request.m_initialData = meshData.m_triangleIndices.data();
            result = m_bufferPool->InitBuffer(request);

            if (result != AZ::RHI::ResultCode::Success)
            {
                AZ_Error("FixedShapeProcessor", false, "Failed to initialize shape index buffer with error code: %d", result);
                return false;
            }

            // setup m_positionBuffer
            objectBuffers.m_positionBuffer = AZ::RHI::Factory::Get().CreateBuffer();
            const auto positionDataSize = static_cast<uint32_t>(meshData.m_positions.size() * sizeof(AuxGeomPosition));

            request.m_buffer = objectBuffers.m_positionBuffer.get();
            request.m_descriptor = AZ::RHI::BufferDescriptor{ AZ::RHI::BufferBindFlags::InputAssembly, positionDataSize };
            request.m_initialData = meshData.m_positions.data();
            result = m_bufferPool->InitBuffer(request);
            if (result != AZ::RHI::ResultCode::Success)
            {
                AZ_Error("FixedShapeProcessor", false, "Failed to initialize shape position buffer with error code: %d", result);
                return false;
            }

            // setup m_normalBuffer
            objectBuffers.m_normalBuffer = AZ::RHI::Factory::Get().CreateBuffer();
            const auto normalDataSize = static_cast<uint32_t>(meshData.m_normals.size() * sizeof(AuxGeomNormal));

            request.m_buffer = objectBuffers.m_normalBuffer.get();
            request.m_descriptor = AZ::RHI::BufferDescriptor{ AZ::RHI::BufferBindFlags::InputAssembly, normalDataSize };
            request.m_initialData = meshData.m_normals.data();
            result = m_bufferPool->InitBuffer(request);
            if (result != AZ::RHI::ResultCode::Success)
            {
                AZ_Error("FixedShapeProcessor", false, "Failed to initialize shape normal buffer with error code: %d", result);
                return false;
            }

            // Setup point index buffer view
            objectBuffers.m_pointIndexCount = static_cast<uint32_t>(meshData.m_pointIndices.size());
            AZ::RHI::IndexBufferView pointIndexBufferView =
            {
                *objectBuffers.m_pointIndexBuffer,
                0,
                static_cast<uint32_t>(objectBuffers.m_pointIndexCount * sizeof(uint16_t)),
                AZ::RHI::IndexFormat::Uint16,
            };
            objectBuffers.m_pointIndexBufferView = pointIndexBufferView;

            // Setup line index buffer view
            objectBuffers.m_lineIndexCount = static_cast<uint32_t>(meshData.m_lineIndices.size());
            AZ::RHI::IndexBufferView lineIndexBufferView =
            {
                *objectBuffers.m_lineIndexBuffer,
                0,
                static_cast<uint32_t>(objectBuffers.m_lineIndexCount * sizeof(uint16_t)),
                AZ::RHI::IndexFormat::Uint16,
            };
            objectBuffers.m_lineIndexBufferView = lineIndexBufferView;

            // Setup triangle index buffer view
            objectBuffers.m_triangleIndexCount = static_cast<uint32_t>(meshData.m_triangleIndices.size());
            AZ::RHI::IndexBufferView triangleIndexBufferView =
            {
                *objectBuffers.m_triangleIndexBuffer,
                0,
                static_cast<uint32_t>(objectBuffers.m_triangleIndexCount * sizeof(uint16_t)),
                AZ::RHI::IndexFormat::Uint16,
            };
            objectBuffers.m_triangleIndexBufferView = triangleIndexBufferView;

            // Setup vertex buffer view
            const auto positionCount = static_cast<uint32_t>(meshData.m_positions.size());
            const uint32_t positionSize = sizeof(float) * 3;

            AZ::RHI::StreamBufferView positionBufferView =
            {
                *objectBuffers.m_positionBuffer,
                0,
                positionCount * positionSize,
                positionSize,
            };

            // Setup normal buffer view
            const auto normalCount = static_cast<uint32_t>(meshData.m_normals.size());
            const uint32_t normalSize = sizeof(float) * 3;

            AZ::RHI::StreamBufferView normalBufferView =
            {
                *objectBuffers.m_normalBuffer,
                0,
                normalCount * normalSize,
                normalSize,
            };

            objectBuffers.m_streamBufferViews = { positionBufferView };
            objectBuffers.m_streamBufferViewsWithNormals = { positionBufferView, normalBufferView };

            // Validate for each draw style
            AZ::RHI::ValidateStreamBufferViews(m_objectStreamLayout[DrawStyle_Point], objectBuffers.m_streamBufferViews);
            AZ::RHI::ValidateStreamBufferViews(m_objectStreamLayout[DrawStyle_Line], objectBuffers.m_streamBufferViews);
            AZ::RHI::ValidateStreamBufferViews(m_objectStreamLayout[DrawStyle_Solid], objectBuffers.m_streamBufferViews);
            AZ::RHI::ValidateStreamBufferViews(m_objectStreamLayout[DrawStyle_Shaded], objectBuffers.m_streamBufferViewsWithNormals);

            return true;
        }

        FixedShapeProcessor::LodIndex FixedShapeProcessor::GetLodIndexForShape(AuxGeomShapeType shapeType, const AZ::RPI::View* view, const AZ::Vector3& worldPosition, const AZ::Vector3& scale)
        {
            const Shape& shape = m_shapes[shapeType];
            if (shape.m_numLods <= 1)
            {
                return 0;   // No LODs for this shape
            }

            // For LODs we really only care about the radius of the curve. i.e. a really long cylinder with a radius R
            // and a short cylinder with radius R should use same LOD if same distance from screen since the LOD is just used
            // to make the curved part look smoother. For all our curved geometries X and Z scale are the radius and Y scale is the length.
            float radius = scale.GetX();

            float screenPercentage = view->CalculateSphereAreaInClipSpace(worldPosition, radius);

            LodIndex lodIndex = shape.m_numLods - 1;

            // No need to test the last LOD since we always choose it if we get that far
            // (unless at some point we implement a test to not draw at all if below that value - but
            // that concern might be better addressed by frustum culling before this)
            for (LodIndex testIndex = 0; testIndex < shape.m_numLods - 1; ++testIndex)
            {
                if (screenPercentage >= shape.m_lodScreenPercentages[testIndex])
                {
                    lodIndex = testIndex;
                    break;
                }
            }

            return lodIndex;
        }

        void FixedShapeProcessor::SetupInputStreamLayout(RHI::InputStreamLayout& inputStreamLayout, RHI::PrimitiveTopology topology, bool includeNormals)
        {
            RHI::InputStreamLayoutBuilder layoutBuilder;

            layoutBuilder.AddBuffer()->Channel("POSITION", RHI::Format::R32G32B32_FLOAT);

            if (includeNormals)
            {
                layoutBuilder.AddBuffer()->Channel("NORMAL", RHI::Format::R32G32B32_FLOAT);
            }
            layoutBuilder.SetTopology(topology);
            inputStreamLayout = layoutBuilder.End();
        }

        void FixedShapeProcessor::FillShaderData(Data::Instance<RPI::Shader>& shader, ShaderData& shaderData)
        {
            // Get the per-object SRG and store the indices of the data we need to set per object
            shaderData.m_shaderAsset = shader->GetAsset();
            shaderData.m_supervariantIndex = shader->GetSupervariantIndex();
            shaderData.m_perObjectSrgLayout = shader->FindShaderResourceGroupLayout(Name{ "ObjectSrg" });
            if (!shaderData.m_perObjectSrgLayout)
            {
                AZ_Error("FixedShapeProcessor", false, "Failed to get shader resource group layout");
                return;
            }

            shaderData.m_drawListTag = shader->GetDrawListTag();
        }

        void FixedShapeProcessor::LoadShaders()
        {
            // load shaders for constant color and direction light
            const char* unlitObjectShaderFilePath = "Shaders/auxgeom/auxgeomobject.azshader";
            const char* litObjectShaderFilePath = "Shaders/auxgeom/auxgeomobjectlit.azshader";

            // constant color shader
            m_unlitShader = RPI::LoadCriticalShader(unlitObjectShaderFilePath);
            // direction light shader
            m_litShader = RPI::LoadCriticalShader(litObjectShaderFilePath);

            if (m_unlitShader.get() == nullptr || m_litShader == nullptr)
            {
                return;
            }

            FillShaderData(m_unlitShader, m_perObjectShaderData[ShapeLightingStyle_ConstantColor]);
            FillShaderData(m_litShader, m_perObjectShaderData[ShapeLightingStyle_Directional]);

            // Initialize all pipeline states
            PipelineStateOptions pipelineStateOptions;
            // initialize two base pipeline state first to preserve the blend functions
            pipelineStateOptions.m_perpectiveType = PerspectiveType_ViewProjection;
            InitPipelineState(pipelineStateOptions);
            pipelineStateOptions.m_perpectiveType = PerspectiveType_ManualOverride;
            InitPipelineState(pipelineStateOptions);

            for (uint32_t perspectiveType = 0; perspectiveType < PerspectiveType_Count; perspectiveType++)
            {
                pipelineStateOptions.m_perpectiveType = (AuxGeomShapePerpectiveType)perspectiveType;
                for (uint32_t blendMode = 0; blendMode < BlendMode_Count; blendMode++)
                {
                    pipelineStateOptions.m_blendMode = (AuxGeomBlendMode)blendMode;
                    for (uint32_t drawStyle = 0; drawStyle < DrawStyle_Count; drawStyle++)
                    {
                        pipelineStateOptions.m_drawStyle = (AuxGeomDrawStyle)drawStyle;
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

        RPI::Ptr<RPI::PipelineStateForDraw>& FixedShapeProcessor::GetPipelineState(const PipelineStateOptions& pipelineStateOptions)
        {
            // The declaration: m_pipelineStates[PerspectiveType_Count][BlendMode_Count][DrawStyle_Count][DepthRead_Count][DepthWrite_Count][FaceCull_Count];
            return m_pipelineStates[pipelineStateOptions.m_perpectiveType][pipelineStateOptions.m_blendMode][pipelineStateOptions.m_drawStyle]
                [pipelineStateOptions.m_depthReadType][pipelineStateOptions.m_depthWriteType][pipelineStateOptions.m_faceCullMode];
        }

        void FixedShapeProcessor::SetUpdatePipelineStates()
        {
            m_needUpdatePipelineStates = true;
        }

        void FixedShapeProcessor::InitPipelineState(const PipelineStateOptions& pipelineStateOptions)
        {
            // Use the the pipeline state for PipelineStateOptions with default values and input perspective type as base pipeline state. Create one if it was empty.
            PipelineStateOptions defaultOptions;
            defaultOptions.m_perpectiveType = pipelineStateOptions.m_perpectiveType;
            defaultOptions.m_drawStyle = pipelineStateOptions.m_drawStyle;
            RPI::Ptr<RPI::PipelineStateForDraw>& basePipelineState = GetPipelineState(defaultOptions);
            if (basePipelineState.get() == nullptr)
            {
                // Only DrawStyle_Shaded uses the lit shader. Others use unlit shader
                auto& shader = (pipelineStateOptions.m_drawStyle == DrawStyle_Shaded) ? m_litShader : m_unlitShader;

                basePipelineState = aznew RPI::PipelineStateForDraw;

                // shader option data for shader variant
                Name optionViewProjectionModeName = Name("o_viewProjMode");
                RPI::ShaderOptionList shaderOptionAndValues;
                shaderOptionAndValues.push_back(RPI::ShaderOption(optionViewProjectionModeName, GetAuxGeomPerspectiveTypeName(pipelineStateOptions.m_perpectiveType)));

                // initialize pipeline state with shader and shader options
                basePipelineState->Init(shader, &shaderOptionAndValues);

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
            destPipelineState->InputStreamLayout() = m_objectStreamLayout[pipelineStateOptions.m_drawStyle];

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
            destPipelineState->RenderStatesOverlay().m_rasterState.m_cullMode =
                ConvertToRHICullMode(pipelineStateOptions.m_faceCullMode);

            // finalize
            destPipelineState->SetOutputFromScene(m_scene);
            destPipelineState->Finalize();
        }

        const AZ::RHI::IndexBufferView& FixedShapeProcessor::GetShapeIndexBufferView(AuxGeomShapeType shapeType, int drawStyle, LodIndex lodIndex) const
        {
            switch(drawStyle)
            {
                case DrawStyle_Point:
                    return m_shapes[shapeType].m_lodBuffers[lodIndex].m_pointIndexBufferView;
                case DrawStyle_Line:
                    return m_shapes[shapeType].m_lodBuffers[lodIndex].m_lineIndexBufferView;
                case DrawStyle_Solid: [[fallthrough]];
                case DrawStyle_Shaded:
                    return m_shapes[shapeType].m_lodBuffers[lodIndex].m_triangleIndexBufferView;
                default:
                    AZ_Assert(false, "Unknown AuxGeom Draw Style %d.", drawStyle);
                    return m_shapes[shapeType].m_lodBuffers[lodIndex].m_triangleIndexBufferView; // default to triangle since it should be drawable for any draw style
            }
        }

        const FixedShapeProcessor::StreamBufferViewsForAllStreams& FixedShapeProcessor::GetShapeStreamBufferViews(AuxGeomShapeType shapeType, LodIndex lodIndex, int drawStyle) const
        {
            return (drawStyle == DrawStyle_Shaded)
                ? m_shapes[shapeType].m_lodBuffers[lodIndex].m_streamBufferViewsWithNormals
                : m_shapes[shapeType].m_lodBuffers[lodIndex].m_streamBufferViews;
        }

        uint32_t FixedShapeProcessor::GetShapeIndexCount(AuxGeomShapeType shapeType, int drawStyle, LodIndex lodIndex)
        {
            switch(drawStyle)
            {
                case DrawStyle_Point:
                    return m_shapes[shapeType].m_lodBuffers[lodIndex].m_pointIndexCount;
                case DrawStyle_Line:
                    return m_shapes[shapeType].m_lodBuffers[lodIndex].m_lineIndexCount;
                case DrawStyle_Solid: // intentional fall through
                case DrawStyle_Shaded:
                    return m_shapes[shapeType].m_lodBuffers[lodIndex].m_triangleIndexCount;
                default:
                    AZ_Assert(false, "Unknown AuxGeom Draw Style %d.", drawStyle);
                    return m_shapes[shapeType].m_lodBuffers[lodIndex].m_triangleIndexCount; // default to triangle since it should be drawable for any draw style
            }
        }

        const RHI::DrawPacket* FixedShapeProcessor::BuildDrawPacketForShape(
            RHI::DrawPacketBuilder& drawPacketBuilder,
            const ShapeBufferEntry& shape,
            int drawStyle,
            const AZStd::vector<AZ::Matrix4x4>& viewProjOverrides,
            const RPI::Ptr<RPI::PipelineStateForDraw>& pipelineState,
            LodIndex lodIndex,
            RHI::DrawItemSortKey sortKey)
        {
            ShaderData& shaderData = m_perObjectShaderData[drawStyle==DrawStyle_Shaded?1:0];

            // Create a SRG for the shape to specify its transform and color
            // [GFX TODO] [ATOM-2333] Try to avoid doing SRG create/compile per draw. Possibly using instancing.
            auto srg = RPI::ShaderResourceGroup::Create(shaderData.m_shaderAsset, shaderData.m_supervariantIndex, shaderData.m_perObjectSrgLayout->GetName());
            if (!srg)
            {
                AZ_Warning("AuxGeom", false, "Failed to create a shader resource group for an AuxGeom draw, Ignoring the draw");
                return nullptr;
            }

            const AZ::Matrix3x4 drawMatrix = AZ::Matrix3x4::CreateFromMatrix3x3AndTranslation(shape.m_rotationMatrix, shape.m_position) * AZ::Matrix3x4::CreateScale(shape.m_scale);
            if (drawStyle == DrawStyle_Shaded)
            {
                Matrix3x3 rotation = shape.m_rotationMatrix;
                rotation.MultiplyByScale(shape.m_scale.GetReciprocal());
                srg->SetConstant(shaderData.m_colorIndex, shape.m_color);
                srg->SetConstant(shaderData.m_modelToWorldIndex, drawMatrix);
                srg->SetConstant(shaderData.m_normalMatrixIndex, rotation);
            }
            else
            {
                srg->SetConstant(shaderData.m_colorIndex, shape.m_color);
                srg->SetConstant(shaderData.m_modelToWorldIndex, drawMatrix);
            }
            if (drawStyle == DrawStyle_Point)
            {
                srg->SetConstant(shaderData.m_pointSizeIndex, shape.m_pointSize);
            }
            if (shape.m_viewProjOverrideIndex >= 0)
            {
                srg->SetConstant(shaderData.m_viewProjectionOverrideIndex, viewProjOverrides[shape.m_viewProjOverrideIndex]);
            }

            pipelineState->UpdateSrgVariantFallback(srg);
            
            srg->Compile();
            m_processSrgs.push_back(srg);
            if (m_shapes[shape.m_shapeType].m_lodBuffers.size() > 0)
            {
                uint32_t indexCount = GetShapeIndexCount(shape.m_shapeType, drawStyle, lodIndex);
                auto& indexBufferView = GetShapeIndexBufferView(shape.m_shapeType, drawStyle, lodIndex);
                auto& streamBufferViews = GetShapeStreamBufferViews(shape.m_shapeType, lodIndex, drawStyle);
                auto& drawListTag = shaderData.m_drawListTag;

                return BuildDrawPacket(
                    drawPacketBuilder, srg, indexCount, indexBufferView, streamBufferViews, drawListTag,
                    pipelineState->GetRHIPipelineState(), sortKey);
            }
            return nullptr;
        }

        const AZ::RHI::IndexBufferView& FixedShapeProcessor::GetBoxIndexBufferView(int drawStyle) const
        {
            switch(drawStyle)
            {
                case DrawStyle_Point:
                    return m_boxBuffers.m_pointIndexBufferView;
                case DrawStyle_Line:
                    return m_boxBuffers.m_lineIndexBufferView;
                case DrawStyle_Solid: // intentional fall through
                case DrawStyle_Shaded:
                    return m_boxBuffers.m_triangleIndexBufferView;
                default:
                    AZ_Assert(false, "Unknown AuxGeom Draw Style %d.", drawStyle);
                    return m_boxBuffers.m_triangleIndexBufferView; // default to triangle since it should be drawable for any draw style
            }
        }

        const FixedShapeProcessor::StreamBufferViewsForAllStreams& FixedShapeProcessor::GetBoxStreamBufferViews(int drawStyle) const
        {
            return (drawStyle == DrawStyle_Shaded)
                ? m_boxBuffers.m_streamBufferViewsWithNormals
                : m_boxBuffers.m_streamBufferViews;
        }

        uint32_t FixedShapeProcessor::GetBoxIndexCount(int drawStyle)
        {
            switch(drawStyle)
            {
                case DrawStyle_Point:
                    return m_boxBuffers.m_pointIndexCount;
                case DrawStyle_Line:
                    return m_boxBuffers.m_lineIndexCount;
                case DrawStyle_Solid: // intentional fall through
                case DrawStyle_Shaded:
                    return m_boxBuffers.m_triangleIndexCount;
                default:
                    AZ_Assert(false, "Unknown AuxGeom Draw Style %d.", drawStyle);
                    return m_boxBuffers.m_triangleIndexCount; // default to triangle since it should be drawable for any draw style
            }
        }

        const RHI::DrawPacket* FixedShapeProcessor::BuildDrawPacketForBox(
            RHI::DrawPacketBuilder& drawPacketBuilder,
            const BoxBufferEntry& box,
            int drawStyle,
            const AZStd::vector<AZ::Matrix4x4>& viewProjOverrides,
            const RPI::Ptr<RPI::PipelineStateForDraw>& pipelineState,
            RHI::DrawItemSortKey sortKey)
        {
            ShaderData& shaderData = m_perObjectShaderData[drawStyle==DrawStyle_Shaded?1:0];
            // Create a SRG for the box to specify its transform and color
            auto srg = RPI::ShaderResourceGroup::Create(shaderData.m_shaderAsset, shaderData.m_supervariantIndex, shaderData.m_perObjectSrgLayout->GetName());
            if (!srg)
            {
                AZ_Warning("AuxGeom", false, "Failed to create a shader resource group for an AuxGeom draw, Ignoring the draw");
                return nullptr;
            }
            const AZ::Matrix3x4 drawMatrix = AZ::Matrix3x4::CreateFromMatrix3x3AndTranslation(box.m_rotationMatrix, box.m_position) * AZ::Matrix3x4::CreateScale(box.m_scale);
            if (drawStyle == DrawStyle_Shaded)
            {
                Matrix3x3 rotation = box.m_rotationMatrix;
                rotation.MultiplyByScale(box.m_scale.GetReciprocal());
                srg->SetConstant(shaderData.m_colorIndex, box.m_color);
                srg->SetConstant(shaderData.m_modelToWorldIndex, drawMatrix);
                srg->SetConstant(shaderData.m_normalMatrixIndex, rotation);
            }
            else
            {
                srg->SetConstant(shaderData.m_colorIndex, box.m_color);
                srg->SetConstant(shaderData.m_modelToWorldIndex, drawMatrix);
            }
            if (drawStyle == DrawStyle_Point)
            {
                srg->SetConstant(shaderData.m_pointSizeIndex, box.m_pointSize);
            }
            if (box.m_viewProjOverrideIndex >= 0)
            {
                srg->SetConstant(shaderData.m_viewProjectionOverrideIndex, viewProjOverrides[box.m_viewProjOverrideIndex]);
            }
            pipelineState->UpdateSrgVariantFallback(srg);
            srg->Compile();
            m_processSrgs.push_back(srg);

            uint32_t indexCount = GetBoxIndexCount(drawStyle);
            auto& indexBufferView = GetBoxIndexBufferView(drawStyle);
            auto& streamBufferViews = GetBoxStreamBufferViews(drawStyle);
            auto& drawListTag = shaderData.m_drawListTag;

            return BuildDrawPacket(
                drawPacketBuilder, srg, indexCount, indexBufferView, streamBufferViews, drawListTag, pipelineState->GetRHIPipelineState(),
                sortKey);
        }

        const RHI::DrawPacket* FixedShapeProcessor::BuildDrawPacket(
            RHI::DrawPacketBuilder& drawPacketBuilder,
            AZ::Data::Instance<RPI::ShaderResourceGroup>& srg,
            uint32_t indexCount,
            const RHI::IndexBufferView& indexBufferView,
            const StreamBufferViewsForAllStreams& streamBufferViews,
            RHI::DrawListTag drawListTag,
            const AZ::RHI::PipelineState* pipelineState,
            RHI::DrawItemSortKey sortKey)
        {
            RHI::DrawIndexed drawIndexed;
            drawIndexed.m_indexCount = indexCount;
            drawIndexed.m_indexOffset = 0;
            drawIndexed.m_vertexOffset = 0;

            drawPacketBuilder.Begin(nullptr);
            drawPacketBuilder.SetDrawArguments(drawIndexed);
            drawPacketBuilder.SetIndexBufferView(indexBufferView);
            drawPacketBuilder.AddShaderResourceGroup(srg->GetRHIShaderResourceGroup());

            RHI::DrawPacketBuilder::DrawRequest drawRequest;
            drawRequest.m_listTag = drawListTag;
            drawRequest.m_pipelineState = pipelineState;
            drawRequest.m_streamBufferViews = streamBufferViews;
            drawRequest.m_sortKey = sortKey;
            drawPacketBuilder.AddDrawItem(drawRequest);

            return drawPacketBuilder.End();
        }
    } // namespace Render
} // namespace AZ
