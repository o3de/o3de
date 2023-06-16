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
        // Build a buffer asset that contains the given data.
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

        void ModelAssetHelpers::CreateUnitCube(ModelAsset* modelAsset)
        {
            // Build a mesh containing a unit cube.
            //    5+----+6
            //    /|   /|
            //  1+----+2|
            //   |4+--|-+7
            //   |    |/
            //  0+----+3

            const AZStd::array<uint32_t, 6 * 6> indices = {
                0, 2, 1, 0, 3, 2,   // front face
                3, 6, 2, 3, 7, 6,   // right face
                7, 5, 6, 7, 4, 5,   // back face
                4, 1, 5, 4, 0, 1,   // left face
                1, 6, 5, 1, 2, 6,   // top face
                7, 0, 3, 7, 4, 0    // bottom face
            };

            const AZStd::array<float, 3 * 8> positions = {
                -0.5f, -0.5f, -0.5f,    // front bottom left
                -0.5f, -0.5f, +0.5f,    // front top left
                +0.5f, -0.5f, +0.5f,    // front top right
                +0.5f, -0.5f, -0.5f,    // front bottom right
                -0.5f, +0.5f, -0.5f,    // back bottom left
                -0.5f, +0.5f, +0.5f,    // back top left
                +0.5f, +0.5f, +0.5f,    // back top right
                +0.5f, +0.5f, -0.5f,    // back bottom right
            };

            const AZStd::array<float, 2 * 8> uvs = {
                0.0f, 0.0f,
                0.0f, 1.0f,
                1.0f, 1.0f,
                1.0f, 0.0f,
                1.0f, 0.0f,
                1.0f, 1.0f,
                0.0f, 1.0f,
                0.0f, 0.0f
            };

            const AZStd::array<float, 3 * 8> normals = {
                -0.333f, -0.333f, -0.333f, // front bottom left
                -0.333f, -0.333f, +0.333f, // front top left
                +0.333f, -0.333f, +0.333f, // front top right
                +0.333f, -0.333f, -0.333f, // front bottom right
                -0.333f, +0.333f, -0.333f, // back bottom left
                -0.333f, +0.333f, +0.333f, // back top left
                +0.333f, +0.333f, +0.333f, // back top right
                +0.333f, +0.333f, -0.333f, // back bottom right
            };

            const AZStd::array<float, 4 * 8> tangents = {
                0.0f, 0.0f, 1.0f, 1.0f,
                0.0f, 0.0f, 1.0f, 1.0f,
                0.0f, 0.0f, 1.0f, 1.0f,
                0.0f, 0.0f, 1.0f, 1.0f,
                0.0f, 0.0f, 1.0f, 1.0f,
                0.0f, 0.0f, 1.0f, 1.0f,
                0.0f, 0.0f, 1.0f, 1.0f,
                0.0f, 0.0f, 1.0f, 1.0f,
            };

            const AZStd::array<float, 3 * 8> bitangents = {
                0.0f, 0.0f, 1.0f,
                0.0f, 0.0f, 1.0f,
                0.0f, 0.0f, 1.0f,
                0.0f, 0.0f, 1.0f,
                0.0f, 0.0f, 1.0f,
                0.0f, 0.0f, 1.0f,
                0.0f, 0.0f, 1.0f,
                0.0f, 0.0f, 1.0f,
            };

            // First build a model LOD asset that contains a mesh for the given data.
            AZ::Data::Asset<AZ::RPI::ModelLodAsset> lodAsset;
            {
                AZ::Data::AssetId lodAssetId = AZ::Uuid::CreateRandom();
                lodAsset = AZ::Data::AssetManager::Instance().CreateAsset(
                    lodAssetId, azrtti_typeid<AZ::RPI::ModelLodAsset>(), Data::AssetLoadBehavior::PreLoad);

                AZ::RPI::ModelLodAssetCreator creator;
                creator.Begin(lodAssetId);

                constexpr uint32_t positionElementCount = aznumeric_cast<uint32_t>(positions.size() / 3);
                constexpr uint32_t indexElementCount = aznumeric_cast<uint32_t>(indices.size());
                constexpr uint32_t uvElementCount = aznumeric_cast<uint32_t>(positions.size() / 2);
                constexpr uint32_t normalElementCount = aznumeric_cast<uint32_t>(positions.size() / 3);
                constexpr uint32_t tangentElementCount = aznumeric_cast<uint32_t>(positions.size() / 4);
                constexpr uint32_t bitangentElementCount = aznumeric_cast<uint32_t>(positions.size() / 3);

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
                creator.SetMeshAabb(AZStd::move(aabb));
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
                    { CreateBufferAsset(tangents.data(), bitangentElementCount, BitangentElementSize),
                      AZ::RHI::BufferViewDescriptor::CreateTyped(0, bitangentElementCount, AZ::RHI::Format::R32G32B32_FLOAT) });
                creator.AddMeshStreamBuffer(
                    AZ::RHI::ShaderSemantic(AZ::Name("UV")),
                    AZ::Name(),
                    { CreateBufferAsset(uvs.data(), uvElementCount, UvElementSize),
                      AZ::RHI::BufferViewDescriptor::CreateTyped(0, uvElementCount, AZ::RHI::Format::R32G32_FLOAT) });
                creator.EndMesh();
                creator.End(lodAsset);
            }

            // Create a model asset that contains the single LOD built above.
            modelAsset->InitData(
                AZ::Name("UnitCube"),
                AZStd::span<AZ::Data::Asset<AZ::RPI::ModelLodAsset>>(&lodAsset, 1),
                {},     // no material slots
                {},     // no fallback material
                {}      // no tags
            );
        }

    } // namespace RPI
} // namespace AZ
