/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Buffer/BufferAssetCreator.h>
#include <Atom/RPI.Reflect/Model/ModelAssetHelpers.h>
#include <Atom/RPI.Reflect/Model/ModelLodAssetCreator.h>
#include <Atom/RPI.Reflect/ResourcePoolAssetCreator.h>

namespace AZ
{
    namespace RPI
    {
        AZ::Data::Asset<AZ::RPI::BufferAsset> ModelAssetHelpers::CreateBufferAsset(
            const void* data, const uint32_t elementCount, const uint32_t elementSize)
        {
            // Create a buffer pool asset for use with the buffer asset
            AZ::Data::Asset<AZ::RPI::ResourcePoolAsset> bufferPoolAsset;
            {
                AZ::Data::AssetId bufferPoolId = AZ::Uuid::CreateRandom();
                bufferPoolAsset = AZ::Data::AssetManager::Instance().CreateAsset(
                    bufferPoolId, azrtti_typeid<AZ::RPI::ResourcePoolAsset>(), Data::AssetLoadBehavior::PreLoad);

                auto bufferPoolDesc = AZStd::make_unique<AZ::RHI::BufferPoolDescriptor>();
                bufferPoolDesc->m_bindFlags = AZ::RHI::BufferBindFlags::InputAssembly;
                bufferPoolDesc->m_heapMemoryLevel = AZ::RHI::HeapMemoryLevel::Host;

                AZ::RPI::ResourcePoolAssetCreator creator;
                creator.Begin(bufferPoolId);
                creator.SetPoolDescriptor(AZStd::move(bufferPoolDesc));
                creator.SetPoolName("ModelAssetHelperBufferPool");
                creator.End(bufferPoolAsset);
            }

            // Create a buffer asset that contains a copy of the input data.
            AZ::Data::Asset<AZ::RPI::BufferAsset> asset;
            {
                AZ::Data::AssetId bufferId = AZ::Uuid::CreateRandom();
                asset = AZ::Data::AssetManager::Instance().CreateAsset(
                    bufferId, azrtti_typeid<AZ::RPI::BufferAsset>(), Data::AssetLoadBehavior::PreLoad);

                AZ::RHI::BufferDescriptor bufferDescriptor;
                bufferDescriptor.m_bindFlags = AZ::RHI::BufferBindFlags::InputAssembly;
                bufferDescriptor.m_byteCount = elementCount * elementSize;

                AZ::RPI::BufferAssetCreator creator;
                creator.Begin(bufferId);
                creator.SetPoolAsset(bufferPoolAsset);
                creator.SetBuffer(data, bufferDescriptor.m_byteCount, bufferDescriptor);
                creator.SetBufferViewDescriptor(AZ::RHI::BufferViewDescriptor::CreateStructured(0, elementCount, elementSize));
                creator.End(asset);
            }

            return asset;
        }

        void ModelAssetHelpers::CreateModel(
            ModelAsset* modelAsset,
            const AZ::Name& name,
            AZStd::span<const uint32_t> indices,
            AZStd::span<const float_t> positions,
            AZStd::span<const float> normals,
            AZStd::span<const float> tangents,
            AZStd::span<const float> bitangents,
            AZStd::span<const float> uvs)
        {
            // First build a model LOD asset that contains a mesh for the given data.
            AZ::Data::Asset<AZ::RPI::ModelLodAsset> lodAsset;

            AZ::Data::AssetId lodAssetId = AZ::Uuid::CreateRandom();
            lodAsset = AZ::Data::AssetManager::Instance().CreateAsset(
                lodAssetId, azrtti_typeid<AZ::RPI::ModelLodAsset>(), Data::AssetLoadBehavior::PreLoad);

            AZ::RPI::ModelLodAssetCreator creator;
            creator.Begin(lodAssetId);

            const uint32_t positionElementCount = aznumeric_cast<uint32_t>(positions.size() / 3);
            const uint32_t indexElementCount = aznumeric_cast<uint32_t>(indices.size());
            const uint32_t uvElementCount = aznumeric_cast<uint32_t>(uvs.size() / 2);
            const uint32_t normalElementCount = aznumeric_cast<uint32_t>(normals.size() / 3);
            const uint32_t tangentElementCount = aznumeric_cast<uint32_t>(tangents.size() / 4);
            const uint32_t bitangentElementCount = aznumeric_cast<uint32_t>(bitangents.size() / 3);

            constexpr uint32_t PositionElementSize = aznumeric_cast<uint32_t>(sizeof(float) * 3);
            constexpr uint32_t IndexElementSize = aznumeric_cast<uint32_t>(sizeof(uint32_t));
            constexpr uint32_t UvElementSize = aznumeric_cast<uint32_t>(sizeof(float) * 2);
            constexpr uint32_t NormalElementSize = aznumeric_cast<uint32_t>(sizeof(float) * 3);
            constexpr uint32_t TangentElementSize = aznumeric_cast<uint32_t>(sizeof(float) * 4);
            constexpr uint32_t BitangentElementSize = aznumeric_cast<uint32_t>(sizeof(float) * 3);

            // Calculate the Aabb for the given positions.
            AZ::Aabb aabb = AZ::Aabb::CreateNull();
            for (uint32_t i = 0; i < positions.size(); i += 3)
            {
                aabb.AddPoint(AZ::Vector3(positions[i], positions[i + 1], positions[i + 2]));
            }

            // Set up a single-mesh asset with only position data.
            creator.BeginMesh();
            creator.SetMeshAabb(aabb);
            creator.SetMeshMaterialSlot(0);
            creator.SetMeshIndexBuffer({ CreateBufferAsset(indices.data(), indexElementCount, IndexElementSize),
                                         AZ::RHI::BufferViewDescriptor::CreateTyped(0, indexElementCount, AZ::RHI::Format::R32_UINT) });
            creator.AddMeshStreamBuffer(
                AZ::RHI::ShaderSemantic(AZ::Name("POSITION")),
                AZ::Name(),
                { CreateBufferAsset(positions.data(), positionElementCount, PositionElementSize),
                  AZ::RHI::BufferViewDescriptor::CreateTyped(0, positionElementCount, AZ::RHI::Format::R32G32B32_FLOAT) });
            creator.AddMeshStreamBuffer(
                AZ::RHI::ShaderSemantic(AZ::Name("NORMAL")),
                AZ::Name(),
                { CreateBufferAsset(normals.data(), normalElementCount, NormalElementSize),
                  AZ::RHI::BufferViewDescriptor::CreateTyped(0, normalElementCount, AZ::RHI::Format::R32G32B32_FLOAT) });
            creator.AddMeshStreamBuffer(
                AZ::RHI::ShaderSemantic(AZ::Name("TANGENT")),
                AZ::Name(),
                { CreateBufferAsset(tangents.data(), tangentElementCount, TangentElementSize),
                  AZ::RHI::BufferViewDescriptor::CreateTyped(0, tangentElementCount, AZ::RHI::Format::R32G32B32A32_FLOAT) });
            creator.AddMeshStreamBuffer(
                AZ::RHI::ShaderSemantic(AZ::Name("BITANGENT")),
                AZ::Name(),
                { CreateBufferAsset(bitangents.data(), bitangentElementCount, BitangentElementSize),
                  AZ::RHI::BufferViewDescriptor::CreateTyped(0, bitangentElementCount, AZ::RHI::Format::R32G32B32_FLOAT) });
            creator.AddMeshStreamBuffer(
                AZ::RHI::ShaderSemantic(AZ::Name("UV")),
                AZ::Name(),
                { CreateBufferAsset(uvs.data(), uvElementCount, UvElementSize),
                  AZ::RHI::BufferViewDescriptor::CreateTyped(0, uvElementCount, AZ::RHI::Format::R32G32_FLOAT) });
            creator.EndMesh();
            creator.End(lodAsset);

            // Create a model asset that contains the single LOD built above.
            modelAsset->InitData(
                name,
                AZStd::span<AZ::Data::Asset<AZ::RPI::ModelLodAsset>>(&lodAsset, 1),
                {}, // no material slots
                {}, // no fallback material
                {} // no tags
            );
        }

        void ModelAssetHelpers::CreateUnitCube(ModelAsset* modelAsset)
        {
            // Build a mesh containing a unit cube.
            // The vertices are duplicated for each face so that we can have correct per-face normals and UVs.

            constexpr int ValuesPerPositionEntry = 3;
            constexpr int ValuesPerUvEntry = 2;
            constexpr int ValuesPerNormalEntry = 3;
            constexpr int ValuesPerTangentEntry = 4;
            constexpr int ValuesPerBitangentEntry = 3;

            constexpr int VerticesPerFace = 6;
            constexpr int MeshFaces = 6;
            constexpr int ValuesPerFace = 4;

            // 6 vertices per face, 6 faces.
            constexpr AZStd::array<uint32_t, VerticesPerFace * MeshFaces> indices = {
                 0,  1,  2,  0,  2,  3,   // front face
                 4,  5,  6,  4,  6,  7,   // right face
                 8,  9, 10,  8, 10, 11,   // back face
                12, 13, 14, 12, 14, 15,   // left face
                16, 17, 18, 16, 18, 19,   // top face
                20, 21, 22, 20, 22, 23    // bottom face
            };

            // 3 values per position, 4 positions per face, 6 faces
            constexpr AZStd::array<float, ValuesPerPositionEntry * ValuesPerFace * MeshFaces> positions = {
                -0.5f, -0.5f, -0.5f, +0.5f, -0.5f, -0.5f, +0.5f, -0.5f, +0.5f, -0.5f, -0.5f, +0.5f,     // front
                +0.5f, -0.5f, -0.5f, +0.5f, +0.5f, -0.5f, +0.5f, +0.5f, +0.5f, +0.5f, -0.5f, +0.5f,     // right
                +0.5f, +0.5f, -0.5f, -0.5f, +0.5f, -0.5f, -0.5f, +0.5f, +0.5f, +0.5f, +0.5f, +0.5f,     // back
                -0.5f, +0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, +0.5f, -0.5f, +0.5f, +0.5f,     // left
                -0.5f, -0.5f, +0.5f, +0.5f, -0.5f, +0.5f, +0.5f, +0.5f, +0.5f, -0.5f, +0.5f, +0.5f,     // top
                -0.5f, +0.5f, -0.5f, +0.5f, +0.5f, -0.5f, +0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,     // bottom
            };

            // 2 values per position, 4 positions per face, 6 faces
            // This aribtrarily maps the UVs to use the full texture on each face.
            // This choice can be changed if a different mapping would be more usable.
            constexpr AZStd::array<float, ValuesPerUvEntry * ValuesPerFace * MeshFaces> uvs = {
                0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,     // front
                0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,     // right
                0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,     // back
                0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,     // left
                0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,     // top
                0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,     // bottom
            };

            // 3 values per position, 4 positions per face, 6 faces
            constexpr AZStd::array<float, ValuesPerNormalEntry * ValuesPerFace * MeshFaces> normals = {
                +0.0f, -1.0f, +0.0f, +0.0f, -1.0f, +0.0f, +0.0f, -1.0f, +0.0f, +0.0f, -1.0f, +0.0f,     // front (-Y)
                +1.0f, +0.0f, +0.0f, +1.0f, +0.0f, +0.0f, +1.0f, +0.0f, +0.0f, +1.0f, +0.0f, +0.0f,     // right (+X)
                +0.0f, +1.0f, +0.0f, +0.0f, +1.0f, +0.0f, +0.0f, +1.0f, +0.0f, +0.0f, +1.0f, +0.0f,     // back (+Y)
                -1.0f, +0.0f, +0.0f, -1.0f, +0.0f, +0.0f, -1.0f, +0.0f, +0.0f, -1.0f, +0.0f, +0.0f,     // left (-X)
                +0.0f, +0.0f, +1.0f, +0.0f, +0.0f, +1.0f, +0.0f, +0.0f, +1.0f, +0.0f, +0.0f, +1.0f,     // top (+Z)
                +0.0f, +0.0f, -1.0f, +0.0f, +0.0f, -1.0f, +0.0f, +0.0f, -1.0f, +0.0f, +0.0f, -1.0f,     // bottom (-Z)
            };

            // 4 values per position, 4 positions per face, 6 faces
            constexpr AZStd::array<float, ValuesPerTangentEntry * ValuesPerFace * MeshFaces> tangents = {
                0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // front (+Z)
                0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // right (+Z)
                0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // back (+Z)
                0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // left (+Z)
                0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, // top (+Y)
                0.0f, -1.0f, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, // bottom (-Y)
            };

            // 3 values per position, 4 positions per face, 6 faces
            constexpr AZStd::array<float, ValuesPerBitangentEntry * ValuesPerFace * MeshFaces> bitangents = {
                +1.0f, +0.0f, +0.0f, +1.0f, +0.0f, +0.0f, +1.0f, +0.0f, +0.0f, +1.0f, +0.0f, +0.0f, // front (+X)
                +0.0f, +1.0f, +0.0f, +0.0f, +1.0f, +0.0f, +0.0f, +1.0f, +0.0f, +0.0f, +1.0f, +0.0f, // right (+Y)
                -1.0f, +0.0f, +0.0f, -1.0f, +0.0f, +0.0f, -1.0f, +0.0f, +0.0f, -1.0f, +0.0f, +0.0f, // back (-X)
                +0.0f, -1.0f, +0.0f, +0.0f, -1.0f, +0.0f, +0.0f, -1.0f, +0.0f, +0.0f, -1.0f, +0.0f, // left (-Y)
                +1.0f, +0.0f, +0.0f, +1.0f, +0.0f, +0.0f, +1.0f, +0.0f, +0.0f, +1.0f, +0.0f, +0.0f, // top (+X)
                +1.0f, +0.0f, +0.0f, +1.0f, +0.0f, +0.0f, +1.0f, +0.0f, +0.0f, +1.0f, +0.0f, +0.0f, // bottom (+X)
            };

            CreateModel(modelAsset, AZ::Name("UnitCube"), indices, positions, normals, tangents, bitangents, uvs);
        }

        void ModelAssetHelpers::CreateUnitX(ModelAsset* modelAsset)
        {
            // Build a mesh containing a unit X model.
            // To make the X double-sided regardless of material, we'll create two faces for each branch of the X.

            constexpr int ValuesPerPositionEntry = 3;
            constexpr int ValuesPerUvEntry = 2;
            constexpr int ValuesPerNormalEntry = 3;
            constexpr int ValuesPerTangentEntry = 4;
            constexpr int ValuesPerBitangentEntry = 3;

            constexpr int VerticesPerFace = 6;
            constexpr int MeshFaces = 4;
            constexpr int ValuesPerFace = 4;

            // 6 vertices per face, 4 faces.
            constexpr AZStd::array<uint32_t, VerticesPerFace * MeshFaces> indices = {
                0,  1,  2,  0,  2,  3, // / face of X
                4,  5,  6,  4,  6,  7, // \ face of X
                8,  9,  10, 8,  10, 11, // / face of X (back)
                12, 13, 14, 12, 14, 15, // \ face of X (back)
            };

            // 3 values per position, 4 positions per face, 2 faces
            constexpr AZStd::array<float, ValuesPerPositionEntry * ValuesPerFace * MeshFaces> positions = {
                -0.5f, -0.5f, -0.5f, +0.5f, +0.5f, -0.5f, +0.5f, +0.5f, +0.5f, -0.5f, -0.5f, +0.5f, //   / face of X
                -0.5f, +0.5f, -0.5f, +0.5f, -0.5f, -0.5f, +0.5f, -0.5f, +0.5f, -0.5f, +0.5f, +0.5f, //   \ face of X
                +0.5f, +0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, +0.5f, +0.5f, +0.5f, +0.5f, //   / face of X (back)
                +0.5f, -0.5f, -0.5f, -0.5f, +0.5f, -0.5f, -0.5f, +0.5f, +0.5f, +0.5f, -0.5f, +0.5f, //   \ face of X (back)
            };

            // 2 values per position, 4 positions per face, 2 faces
            // This aribtrarily maps the UVs to use the full texture on each face.
            // This choice can be changed if a different mapping would be more usable.
            constexpr AZStd::array<float, ValuesPerUvEntry * ValuesPerFace * MeshFaces> uvs = {
                0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, //   / face of X
                0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, //   \ face of X
                0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, //   / face of X (back)
                0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, //   \ face of X (back
            };

            // 3 values per position, 4 positions per face, 2 faces
            constexpr AZStd::array<float, ValuesPerNormalEntry * ValuesPerFace * MeshFaces> normals = {
                +0.5f, -0.5f, +0.0f, +0.5f, -0.5f, +0.0f, +0.5f, -0.5f, +0.0f, +0.5f, -0.5f, +0.0f, //   / face of X
                -0.5f, -0.5f, +0.0f, -0.5f, -0.5f, +0.0f, -0.5f, -0.5f, +0.0f, -0.5f, -0.5f, +0.0f, //   \ face of X
                -0.5f, +0.5f, +0.0f, -0.5f, +0.5f, +0.0f, -0.5f, +0.5f, +0.0f, -0.5f, +0.5f, +0.0f, //   / face of X (back)
                +0.5f, +0.5f, +0.0f, +0.5f, +0.5f, +0.0f, +0.5f, +0.5f, +0.0f, +0.5f, +0.5f, +0.0f, //   \ face of X (back)
            };

            // 4 values per position, 4 positions per face, 2 faces
            constexpr AZStd::array<float, ValuesPerTangentEntry * ValuesPerFace * MeshFaces> tangents = {
                0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, //   / face of X
                0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, //   \ face of X
                0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, //   / face of X (back)
                0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, //   \ face of X (back)
            };

            // 3 values per position, 4 positions per face, 2 faces
            constexpr AZStd::array<float, ValuesPerBitangentEntry * ValuesPerFace * MeshFaces> bitangents = {
                +0.5f, +0.5f, +0.0f, +0.5f, +0.5f, +0.0f, +0.5f, +0.5f, +0.0f, +0.5f, +0.5f, +0.0f, //   / face of X
                -0.5f, +0.5f, +0.0f, -0.5f, +0.5f, +0.0f, -0.5f, +0.5f, +0.0f, -0.5f, +0.5f, +0.0f, //   \ face of X
                -0.5f, -0.5f, +0.0f, -0.5f, -0.5f, +0.0f, -0.5f, -0.5f, +0.0f, -0.5f, -0.5f, +0.0f, //   / face of X (back)
                +0.5f, -0.5f, +0.0f, +0.5f, -0.5f, +0.0f, +0.5f, -0.5f, +0.0f, +0.5f, -0.5f, +0.0f, //   \ face of X (back)
            };

            CreateModel(modelAsset, AZ::Name("UnitX"), indices, positions, normals, tangents, bitangents, uvs);
        }
    } // namespace RPI
} // namespace AZ
