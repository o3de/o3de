/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Buffer/BufferAssetCreator.h>
#include <Atom/RPI.Reflect/Model/ModelAssetCreator.h>
#include <Atom/RPI.Reflect/Model/ModelLodAssetCreator.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/RPI.Reflect/Model/ModelKdTree.h>
#include <Atom/RPI.Reflect/Model/ModelLodAsset.h>
#include <Atom/RPI.Reflect/ResourcePoolAssetCreator.h>
#include <Atom/RPI.Public/Model/UvStreamTangentBitmask.h>

#include <AzCore/std/limits.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Math/Sfmt.h>

#include <AZTestShared/Math/MathTestHelpers.h>
#include <AzTest/AzTest.h>

#include <Common/RPITestFixture.h>
#include <Common/SerializeTester.h>
#include <Common/ErrorMessageFinder.h>

namespace UnitTest
{
    AZ::Data::Asset<AZ::RPI::BufferAsset> BuildTestBuffer(const uint32_t elementCount, const uint32_t elementSize)
    {
        using namespace AZ;

        const uint32_t bufferSize = elementCount * elementSize;

        AZStd::vector<uint8_t> bufferData;
        bufferData.resize(bufferSize);

        //The actual data doesn't matter
        const uint8_t bufferDataSize = aznumeric_cast<uint8_t>(bufferData.size());
        for (uint8_t i = 0; i < bufferDataSize; ++i)
        {
            bufferData[i] = i;
        }

        Data::Asset<RPI::ResourcePoolAsset> bufferPoolAsset;

        {
            auto bufferPoolDesc = AZStd::make_unique<RHI::BufferPoolDescriptor>();
            bufferPoolDesc->m_bindFlags = RHI::BufferBindFlags::InputAssembly;
            bufferPoolDesc->m_heapMemoryLevel = RHI::HeapMemoryLevel::Host;

            RPI::ResourcePoolAssetCreator creator;
            creator.Begin(Uuid::CreateRandom());
            creator.SetPoolDescriptor(AZStd::move(bufferPoolDesc));
            creator.SetPoolName("TestPool");
            EXPECT_TRUE(creator.End(bufferPoolAsset));
        }

        Data::Asset<RPI::BufferAsset> asset;

        {
            RHI::BufferDescriptor bufferDescriptor;
            bufferDescriptor.m_bindFlags = RHI::BufferBindFlags::InputAssembly;
            bufferDescriptor.m_byteCount = bufferSize;

            RHI::BufferViewDescriptor bufferViewDescriptor =
                RHI::BufferViewDescriptor::CreateStructured(0, elementCount, elementSize);

            RPI::BufferAssetCreator creator;

            creator.Begin(AZ::Uuid::CreateRandom());
            creator.SetPoolAsset(bufferPoolAsset);
            creator.SetBuffer(bufferData.data(), bufferDescriptor.m_byteCount, bufferDescriptor);
            creator.SetBufferViewDescriptor(bufferViewDescriptor);

            EXPECT_TRUE(creator.End(asset));
            EXPECT_TRUE(asset.IsReady());
            EXPECT_NE(asset.Get(), nullptr);
        }

        return asset;
    }

    class ModelTests
        : public RPITestFixture
    {
    protected:

        struct ExpectedMesh
        {
            AZ::Aabb m_aabb = AZ::Aabb::CreateNull();
            uint32_t m_indexCount = 0;
            uint32_t m_vertexCount = 0;
            AZ::RPI::ModelMaterialSlot::StableId m_materialSlotId = AZ::RPI::ModelMaterialSlot::InvalidStableId;
        };

        struct ExpectedLod
        {
            AZ::Aabb m_aabb = AZ::Aabb::CreateNull();
            AZStd::vector<ExpectedMesh> m_meshes;
        };

        struct ExpectedModel
        {
            AZ::Aabb m_aabb = AZ::Aabb::CreateNull();
            AZStd::vector<ExpectedLod> m_lods;
        };

        void SetUp() override
        {
            RPITestFixture::SetUp();

            auto assetId = AZ::Data::AssetId(AZ::Uuid::CreateRandom(), 0);
            auto typeId = AZ::AzTypeInfo<AZ::RPI::MaterialAsset>::Uuid();
            m_materialAsset = AZ::Data::Asset<AZ::RPI::MaterialAsset>(assetId, typeId, "");

            // Some tests attempt to serialize-in the model asset, which should not attempt to actually load this dummy asset reference.
            m_materialAsset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehaviorNamespace::NoLoad);
        }

        AZ::RHI::ShaderSemantic GetPositionSemantic() const
        {
            return AZ::RHI::ShaderSemantic(AZ::Name("POSITION"));
        }

        bool CalculateAABB(const AZ::RHI::BufferViewDescriptor& bufferViewDesc, const AZ::RPI::BufferAsset& bufferAsset, AZ::Aabb& aabb)
        {
            const uint32_t elementSize = bufferViewDesc.m_elementSize;
            const uint32_t elementCount = bufferViewDesc.m_elementCount;

            // Position is 3 floats
            if (elementSize == sizeof(float) * 3)
            {
                const uint8_t* buffer = reinterpret_cast<const uint8_t*>(&bufferAsset.GetBuffer()[0]);

                for (uint32_t i = 0; i < elementCount; ++i)
                {
                    const uint8_t* p = buffer + (i * 3);
                    aabb.AddPoint(AZ::Vector3(float(p[0]), float(p[1]), float(p[2])));
                }
            }
            else
            {
                // No idea what type of position stream this is
                return false;
            }

            return true;
        }

        //! This function assumes the model has "sharedMeshCount + separateMeshCount" unique material slots, with incremental IDs starting at 0.
        AZ::Data::Asset<AZ::RPI::ModelLodAsset> BuildTestLod(const uint32_t sharedMeshCount, const uint32_t separateMeshCount, ExpectedLod& expectedLod)
        {
            using namespace AZ;

            //Create an Lod with a given number of meshes
            RPI::ModelLodAssetCreator creator;

            creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));

            const uint32_t indexCount = 36;
            const uint32_t vertexCount = 36;

            RPI::ModelMaterialSlot::StableId materialSlotId = 0;

            if(sharedMeshCount > 0)
            {
                const uint32_t sharedIndexCount = indexCount * sharedMeshCount;
                const uint32_t sharedVertexCount = vertexCount * sharedMeshCount;

                Data::Asset<RPI::BufferAsset> sharedIndexBuffer = BuildTestBuffer(sharedIndexCount, sizeof(uint32_t));
                Data::Asset<RPI::BufferAsset> sharedPositionBuffer = BuildTestBuffer(sharedVertexCount, sizeof(uint32_t));

                creator.SetLodIndexBuffer(sharedIndexBuffer);
                creator.AddLodStreamBuffer(sharedPositionBuffer);

                for (uint32_t i = 0; i < sharedMeshCount; ++i)
                {
                    ExpectedMesh expectedMesh;
                    expectedMesh.m_indexCount = indexCount;
                    expectedMesh.m_vertexCount = vertexCount;
                    expectedMesh.m_materialSlotId = i;

                    RHI::BufferViewDescriptor indexBufferViewDescriptor =
                        RHI::BufferViewDescriptor::CreateStructured(i * indexCount, indexCount, sizeof(uint32_t));

                    RHI::BufferViewDescriptor vertexBufferViewDescriptor =
                        RHI::BufferViewDescriptor::CreateStructured(i * vertexCount, vertexCount, sizeof(float) * 3);

                    if (!CalculateAABB(vertexBufferViewDescriptor, *sharedPositionBuffer.Get(), expectedMesh.m_aabb))
                    {
                        return {};
                    }

                    creator.BeginMesh();
                    Aabb aabb = expectedMesh.m_aabb;
                    creator.SetMeshAabb(AZStd::move(aabb));
                    creator.SetMeshMaterialSlot(materialSlotId++);
                    creator.SetMeshIndexBuffer({ sharedIndexBuffer, indexBufferViewDescriptor });
                    creator.AddMeshStreamBuffer(GetPositionSemantic(), AZ::Name(), { sharedPositionBuffer, vertexBufferViewDescriptor });
                    creator.EndMesh();

                    expectedLod.m_aabb.AddAabb(expectedMesh.m_aabb);
                    expectedLod.m_meshes.emplace_back(AZStd::move(expectedMesh));
                }
            }

            for (uint32_t i = 0; i < separateMeshCount; ++i)
            {
                ExpectedMesh expectedMesh;
                expectedMesh.m_indexCount = indexCount;
                expectedMesh.m_vertexCount = vertexCount;
                expectedMesh.m_materialSlotId = sharedMeshCount + i;

                RHI::BufferViewDescriptor indexBufferViewDescriptor =
                    RHI::BufferViewDescriptor::CreateStructured(0, indexCount, sizeof(uint32_t));

                RHI::BufferViewDescriptor positionBufferViewDescriptor =
                    RHI::BufferViewDescriptor::CreateStructured(0, vertexCount, sizeof(float) * 3);

                Data::Asset<AZ::RPI::BufferAsset> positonBuffer = BuildTestBuffer(vertexCount, sizeof(float) * 3);

                if (!CalculateAABB(positionBufferViewDescriptor, *positonBuffer.Get(), expectedMesh.m_aabb))
                {
                    return {};
                }

                creator.BeginMesh();
                Aabb aabb = expectedMesh.m_aabb;
                creator.SetMeshAabb(AZStd::move(aabb));
                creator.SetMeshMaterialSlot(materialSlotId++);
                creator.SetMeshIndexBuffer({ BuildTestBuffer(indexCount, sizeof(uint32_t)), indexBufferViewDescriptor });
                creator.AddMeshStreamBuffer(GetPositionSemantic(), AZ::Name(), { positonBuffer, positionBufferViewDescriptor });

                creator.EndMesh();

                expectedLod.m_aabb.AddAabb(expectedMesh.m_aabb);
                expectedLod.m_meshes.emplace_back(AZStd::move(expectedMesh));
            }

            Data::Asset<RPI::ModelLodAsset> asset;
            EXPECT_TRUE(creator.End(asset));
            EXPECT_TRUE(asset.IsReady());
            EXPECT_NE(asset.Get(), nullptr);

            return asset;
        }

        AZ::Data::Asset<AZ::RPI::ModelAsset> BuildTestModel(
            const uint32_t lodCount, const uint32_t sharedMeshCount, const uint32_t separateMeshCount, ExpectedModel& expectedModel)
        {
            using namespace AZ;

            RPI::ModelAssetCreator creator;

            creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));
            creator.SetName("TestModel");

            for (RPI::ModelMaterialSlot::StableId materialSlotId = 0; materialSlotId < sharedMeshCount + separateMeshCount; ++materialSlotId)
            {
                RPI::ModelMaterialSlot slot;
                slot.m_defaultMaterialAsset = m_materialAsset;
                slot.m_displayName = AZStd::string::format("Slot%d", materialSlotId);
                slot.m_stableId = materialSlotId;
                creator.AddMaterialSlot(slot);
            }

            for (uint32_t i = 0; i < lodCount; ++i)
            {
                ExpectedLod expectedLod;

                creator.AddLodAsset(BuildTestLod(sharedMeshCount, separateMeshCount, expectedLod));

                expectedModel.m_aabb.AddAabb(expectedLod.m_aabb);
                expectedModel.m_lods.emplace_back(AZStd::move(expectedLod));
            }

            Data::Asset<RPI::ModelAsset> asset;
            EXPECT_TRUE(creator.End(asset));
            EXPECT_TRUE(asset.IsReady());
            EXPECT_NE(asset.Get(), nullptr);

            return asset;
        }

        void ValidateMesh(const AZ::RPI::ModelLodAsset::Mesh& mesh, const ExpectedMesh& expectedMesh)
        {
            EXPECT_TRUE(mesh.GetAabb() == expectedMesh.m_aabb);
            EXPECT_TRUE(mesh.GetIndexCount() == expectedMesh.m_indexCount);
            EXPECT_TRUE(mesh.GetVertexCount() == expectedMesh.m_vertexCount);
            EXPECT_TRUE(mesh.GetMaterialSlotId() == expectedMesh.m_materialSlotId);
        }

        void ValidateLodAsset(const AZ::RPI::ModelLodAsset* lodAsset, const ExpectedLod& expectedLod)
        {
            ASSERT_NE(lodAsset, nullptr);
            EXPECT_TRUE(lodAsset->GetAabb().IsValid());
            EXPECT_TRUE(lodAsset->GetMeshes().size() == expectedLod.m_meshes.size());
            EXPECT_TRUE(lodAsset->GetAabb() == expectedLod.m_aabb);

            for (size_t i = 0; i < lodAsset->GetMeshes().size(); ++i)
            {
                const auto meshes = lodAsset->GetMeshes();
                const AZ::RPI::ModelLodAsset::Mesh& mesh = meshes[i];
                const ExpectedMesh& expectedMesh = expectedLod.m_meshes[i];

                ValidateMesh(mesh, expectedMesh);
            }
        }

        void ValidateModelAsset(const AZ::RPI::ModelAsset* modelAsset, const ExpectedModel& expectedModel)
        {
            ASSERT_NE(modelAsset, nullptr);
            EXPECT_TRUE(modelAsset->GetAabb().IsValid());
            EXPECT_TRUE(modelAsset->GetLodAssets().size() == expectedModel.m_lods.size());
            EXPECT_TRUE(modelAsset->GetAabb() == expectedModel.m_aabb);

            for (size_t i = 0; i < modelAsset->GetLodAssets().size(); ++i)
            {
                const AZ::RPI::ModelLodAsset* lodAsset = modelAsset->GetLodAssets()[i].Get();
                const ExpectedLod& expectedLod = expectedModel.m_lods[i];

                ValidateLodAsset(lodAsset, expectedLod);
            }
        }

        const uint32_t m_manyMesh = 100; // Not too much to hold up the tests but enough to stress them
        AZ::Data::Asset<AZ::RPI::MaterialAsset> m_materialAsset;

    };

    TEST_F(ModelTests, SerializeModelOneLodOneSeparateMesh)
    {
        using namespace AZ;

        ExpectedModel expectedModel;

        const uint32_t lodCount = 1;
        const uint32_t sharedMeshCount = 0;
        const uint32_t separateMeshCount = 1;

        Data::Asset<RPI::ModelAsset> modelAsset = BuildTestModel(lodCount, sharedMeshCount, separateMeshCount, expectedModel);
        ValidateModelAsset(modelAsset.Get(), expectedModel);

        SerializeTester<RPI::ModelAsset> tester(GetSerializeContext());
        tester.SerializeOut(modelAsset.Get());

        Data::Asset<RPI::ModelAsset> serializedModelAsset = tester.SerializeIn(Data::AssetId(Uuid::CreateRandom()));
        ValidateModelAsset(serializedModelAsset.Get(), expectedModel);
    }

    TEST_F(ModelTests, SerializeModelOneLodOneSharedMesh)
    {
        using namespace AZ;

        ExpectedModel expectedModel;

        const uint32_t lodCount = 1;
        const uint32_t sharedMeshCount = 1;
        const uint32_t separateMeshCount = 0;

        Data::Asset<RPI::ModelAsset> modelAsset = BuildTestModel(lodCount, sharedMeshCount, separateMeshCount, expectedModel);
        ValidateModelAsset(modelAsset.Get(), expectedModel);

        SerializeTester<RPI::ModelAsset> tester(GetSerializeContext());
        tester.SerializeOut(modelAsset.Get());

        Data::Asset<RPI::ModelAsset> serializedModelAsset = tester.SerializeIn(Data::AssetId(Uuid::CreateRandom()));
        ValidateModelAsset(serializedModelAsset.Get(), expectedModel);
    }

    TEST_F(ModelTests, SerializeModelMaxLodOneSeparateMesh)
    {
        using namespace AZ;

        ExpectedModel expectedModel;

        const uint32_t lodCount = RPI::ModelLodAsset::LodCountMax;
        const uint32_t sharedMeshCount = 0;
        const uint32_t separateMeshCount = 1;

        Data::Asset<RPI::ModelAsset> modelAsset = BuildTestModel(lodCount, sharedMeshCount, separateMeshCount, expectedModel);
        ValidateModelAsset(modelAsset.Get(), expectedModel);

        SerializeTester<RPI::ModelAsset> tester(GetSerializeContext());
        tester.SerializeOut(modelAsset.Get());

        Data::Asset<RPI::ModelAsset> serializedModelAsset = tester.SerializeIn(Data::AssetId(Uuid::CreateRandom()));
        ValidateModelAsset(serializedModelAsset.Get(), expectedModel);
    }

    TEST_F(ModelTests, SerializeModelMaxLodOneSharedMesh)
    {
        using namespace AZ;

        ExpectedModel expectedModel;

        const uint32_t lodCount = RPI::ModelLodAsset::LodCountMax;
        const uint32_t sharedMeshCount = 1;
        const uint32_t separateMeshCount = 0;

        Data::Asset<RPI::ModelAsset> modelAsset = BuildTestModel(lodCount, sharedMeshCount, separateMeshCount, expectedModel);
        ValidateModelAsset(modelAsset.Get(), expectedModel);

        SerializeTester<RPI::ModelAsset> tester(GetSerializeContext());
        tester.SerializeOut(modelAsset.Get());

        Data::Asset<RPI::ModelAsset> serializedModelAsset = tester.SerializeIn(Data::AssetId(Uuid::CreateRandom()));
        ValidateModelAsset(serializedModelAsset.Get(), expectedModel);
    }

    TEST_F(ModelTests, SerializeModelOneLodManySeparateMeshes)
    {
        using namespace AZ;

        ExpectedModel expectedModel;

        const uint32_t lodCount = 1;
        const uint32_t sharedMeshCount = 0;
        const uint32_t separateMeshCount = m_manyMesh;

        Data::Asset<RPI::ModelAsset> modelAsset = BuildTestModel(lodCount, sharedMeshCount, separateMeshCount, expectedModel);
        ValidateModelAsset(modelAsset.Get(), expectedModel);

        SerializeTester<RPI::ModelAsset> tester(GetSerializeContext());
        tester.SerializeOut(modelAsset.Get());

        Data::Asset<RPI::ModelAsset> serializedModelAsset = tester.SerializeIn(Data::AssetId(Uuid::CreateRandom()));
        ValidateModelAsset(serializedModelAsset.Get(), expectedModel);
    }

    TEST_F(ModelTests, SerializeModelOneLodManySharedMeshes)
    {
        using namespace AZ;

        ExpectedModel expectedModel;

        const uint32_t lodCount = 1;
        const uint32_t sharedMeshCount = m_manyMesh;
        const uint32_t separateMeshCount = 0;

        Data::Asset<RPI::ModelAsset> modelAsset = BuildTestModel(lodCount, sharedMeshCount, separateMeshCount, expectedModel);
        ValidateModelAsset(modelAsset.Get(), expectedModel);

        SerializeTester<RPI::ModelAsset> tester(GetSerializeContext());
        tester.SerializeOut(modelAsset.Get());

        Data::Asset<RPI::ModelAsset> serializedModelAsset = tester.SerializeIn(Data::AssetId(Uuid::CreateRandom()));
        ValidateModelAsset(serializedModelAsset.Get(), expectedModel);
    }

    TEST_F(ModelTests, SerializeModelMaxLodManySeparateMeshes)
    {
        using namespace AZ;

        ExpectedModel expectedModel;

        const uint32_t lodCount = RPI::ModelLodAsset::LodCountMax;
        const uint32_t sharedMeshCount = 0;
        const uint32_t separateMeshCount = m_manyMesh;

        Data::Asset<RPI::ModelAsset> modelAsset = BuildTestModel(lodCount, sharedMeshCount, separateMeshCount, expectedModel);
        ValidateModelAsset(modelAsset.Get(), expectedModel);

        SerializeTester<RPI::ModelAsset> tester(GetSerializeContext());
        tester.SerializeOut(modelAsset.Get());

        Data::Asset<RPI::ModelAsset> serializedModelAsset = tester.SerializeIn(Data::AssetId(Uuid::CreateRandom()));
        ValidateModelAsset(serializedModelAsset.Get(), expectedModel);
    }

    TEST_F(ModelTests, SerializeModelMaxLodManySharedMeshes)
    {
        using namespace AZ;

        ExpectedModel expectedModel;

        const uint32_t lodCount = RPI::ModelLodAsset::LodCountMax;
        const uint32_t sharedMeshCount = m_manyMesh;
        const uint32_t separateMeshCount = 0;

        Data::Asset<RPI::ModelAsset> modelAsset = BuildTestModel(lodCount, sharedMeshCount, separateMeshCount, expectedModel);
        ValidateModelAsset(modelAsset.Get(), expectedModel);

        SerializeTester<RPI::ModelAsset> tester(GetSerializeContext());
        tester.SerializeOut(modelAsset.Get());

        Data::Asset<RPI::ModelAsset> serializedModelAsset = tester.SerializeIn(Data::AssetId(Uuid::CreateRandom()));
        ValidateModelAsset(serializedModelAsset.Get(), expectedModel);
    }

    TEST_F(ModelTests, SerializeModelOneLodOneSharedMeshOneSeparateMesh)
    {
        using namespace AZ;

        ExpectedModel expectedModel;

        const uint32_t lodCount = 1;
        const uint32_t sharedMeshCount = 1;
        const uint32_t separateMeshCount = 1;

        Data::Asset<RPI::ModelAsset> modelAsset = BuildTestModel(lodCount, sharedMeshCount, separateMeshCount, expectedModel);
        ValidateModelAsset(modelAsset.Get(), expectedModel);

        SerializeTester<RPI::ModelAsset> tester(GetSerializeContext());
        tester.SerializeOut(modelAsset.Get());

        Data::Asset<RPI::ModelAsset> serializedModelAsset = tester.SerializeIn(Data::AssetId(Uuid::CreateRandom()));
        ValidateModelAsset(serializedModelAsset.Get(), expectedModel);
    }

    TEST_F(ModelTests, SerializeModelMaxLodOneSharedMeshOneSeparateMesh)
    {
        using namespace AZ;

        ExpectedModel expectedModel;

        const uint32_t lodCount = RPI::ModelLodAsset::LodCountMax;
        const uint32_t sharedMeshCount = 1;
        const uint32_t separateMeshCount = 1;

        Data::Asset<RPI::ModelAsset> modelAsset = BuildTestModel(lodCount, sharedMeshCount, separateMeshCount, expectedModel);
        ValidateModelAsset(modelAsset.Get(), expectedModel);

        SerializeTester<RPI::ModelAsset> tester(GetSerializeContext());
        tester.SerializeOut(modelAsset.Get());

        Data::Asset<RPI::ModelAsset> serializedModelAsset = tester.SerializeIn(Data::AssetId(Uuid::CreateRandom()));
        ValidateModelAsset(serializedModelAsset.Get(), expectedModel);
    }

    TEST_F(ModelTests, SerializeModelMaxLodManySharedMeshOneSeparateMesh)
    {
        using namespace AZ;

        ExpectedModel expectedModel;

        const uint32_t lodCount = RPI::ModelLodAsset::LodCountMax;
        const uint32_t sharedMeshCount = m_manyMesh;
        const uint32_t separateMeshCount = 1;

        Data::Asset<RPI::ModelAsset> modelAsset = BuildTestModel(lodCount, sharedMeshCount, separateMeshCount, expectedModel);
        ValidateModelAsset(modelAsset.Get(), expectedModel);

        SerializeTester<RPI::ModelAsset> tester(GetSerializeContext());
        tester.SerializeOut(modelAsset.Get());

        Data::Asset<RPI::ModelAsset> serializedModelAsset = tester.SerializeIn(Data::AssetId(Uuid::CreateRandom()));
        ValidateModelAsset(serializedModelAsset.Get(), expectedModel);
    }

    TEST_F(ModelTests, SerializeModelMaxLodOneSharedMeshManySeparateMesh)
    {
        using namespace AZ;

        ExpectedModel expectedModel;

        const uint32_t lodCount = RPI::ModelLodAsset::LodCountMax;
        const uint32_t sharedMeshCount = 1;
        const uint32_t separateMeshCount = m_manyMesh;

        Data::Asset<RPI::ModelAsset> modelAsset = BuildTestModel(lodCount, sharedMeshCount, separateMeshCount, expectedModel);
        ValidateModelAsset(modelAsset.Get(), expectedModel);

        SerializeTester<RPI::ModelAsset> tester(GetSerializeContext());
        tester.SerializeOut(modelAsset.Get());

        Data::Asset<RPI::ModelAsset> serializedModelAsset = tester.SerializeIn(Data::AssetId(Uuid::CreateRandom()));
        ValidateModelAsset(serializedModelAsset.Get(), expectedModel);
    }

    TEST_F(ModelTests, SerializeModelMaxLodManySharedMeshManySeparateMesh)
    {
        using namespace AZ;

        ExpectedModel expectedModel;

        const uint32_t lodCount = RPI::ModelLodAsset::LodCountMax;
        const uint32_t sharedMeshCount = m_manyMesh;
        const uint32_t separateMeshCount = m_manyMesh;

        Data::Asset<RPI::ModelAsset> modelAsset = BuildTestModel(lodCount, sharedMeshCount, separateMeshCount, expectedModel);
        ValidateModelAsset(modelAsset.Get(), expectedModel);

        SerializeTester<RPI::ModelAsset> tester(GetSerializeContext());
        tester.SerializeOut(modelAsset.Get());

        Data::Asset<RPI::ModelAsset> serializedModelAsset = tester.SerializeIn(Data::AssetId(Uuid::CreateRandom()));
        ValidateModelAsset(serializedModelAsset.Get(), expectedModel);
    }

    // Tests that if we try to set the name on a Model
    // before calling Begin that it will fail.
    TEST_F(ModelTests, SetNameNoBegin)
    {
        using namespace AZ;

        RPI::ModelAssetCreator creator;

        ErrorMessageFinder messageFinder("Begin() was not called");

        creator.SetName("TestName");
    }

    // Tests that if we try to add a ModelLod to a Model
    // before calling Begin that it will fail.
    TEST_F(ModelTests, AddLodNoBegin)
    {
        using namespace AZ;

        RPI::ModelAssetCreator creator;

        //Build a valid lod
        ExpectedLod expectedLod;
        Data::Asset<RPI::ModelLodAsset> lod = BuildTestLod(0, 1, expectedLod);

        ErrorMessageFinder messageFinder("Begin() was not called");

        creator.AddLodAsset(AZStd::move(lod));
    }

    // Tests that if we create a ModelAsset without adding
    // any ModelLodAssets that the creator will properly fail to produce an asset.
    TEST_F(ModelTests, CreateModelNoLods)
    {
        using namespace AZ;

        RPI::ModelAssetCreator creator;

        creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));

        ErrorMessageFinder messageFinder("No valid ModelLodAssets have been added to this ModelAsset.");

        // Since there are no LODs set on this model it
        // we should not be able to successfully end the creator
        Data::Asset<RPI::ModelAsset> asset;
        ASSERT_FALSE(creator.End(asset));
        ASSERT_FALSE(asset.IsReady());
        ASSERT_EQ(asset.Get(), nullptr);
    }

    // Tests that if we call SetLodIndexBuffer without calling
    // Begin first on the ModelLodAssetCreator that it
    // fails as expected.
    TEST_F(ModelTests, SetLodIndexBufferNoBegin)
    {
        using namespace AZ;

        Data::Asset<RPI::BufferAsset> validIndexBuffer = BuildTestBuffer(10, sizeof(uint32_t));

        ErrorMessageFinder messageFinder("Begin() was not called");

        RPI::ModelLodAssetCreator creator;
        creator.SetLodIndexBuffer(validIndexBuffer);
    }

    // Tests that if we call AddLodStreamBuffer without calling
    // Begin first on the ModelLodAssetCreator that it
    // fails as expected.
    TEST_F(ModelTests, AddLodStreamBufferNoBegin)
    {
        using namespace AZ;

        Data::Asset<RPI::BufferAsset> validStreamBuffer = BuildTestBuffer(10, sizeof(float) * 3);

        ErrorMessageFinder messageFinder("Begin() was not called");

        RPI::ModelLodAssetCreator creator;
        creator.AddLodStreamBuffer(validStreamBuffer);
    }

    // Tests that if we call BeginMesh without calling
    // Begin first on the ModelLodAssetCreator that it
    // fails as expected.
    TEST_F(ModelTests, BeginMeshNoBegin)
    {
        using namespace AZ;

        ErrorMessageFinder messageFinder("Begin() was not called");

        RPI::ModelLodAssetCreator creator;
        creator.BeginMesh();
    }

    // Tests that if we try to set an AABB on a mesh
    // without calling Begin or BeginMesh that it fails
    // as expected. Also tests the case that Begin *is*
    // called but BeginMesh is not.
    TEST_F(ModelTests, SetAabbNoBeginNoBeginMesh)
    {
        using namespace AZ;

        RPI::ModelLodAssetCreator creator;

        AZ::Aabb aabb = AZ::Aabb::CreateCenterRadius(AZ::Vector3::CreateZero(), 1.0f);
        ASSERT_TRUE(aabb.IsValid());

        {
            ErrorMessageFinder messageFinder("Begin() was not called");
            AZ::Aabb testAabb = aabb;
            creator.SetMeshAabb(AZStd::move(testAabb));
        }

        creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));

        //This should still fail even if we call Begin but not BeginMesh
        {
            ErrorMessageFinder messageFinder("BeginMesh() was not called");
            AZ::Aabb testAabb = aabb;
            creator.SetMeshAabb(AZStd::move(testAabb));
        }
    }

    // Tests that if we try to set the material slot on a mesh
    // without calling Begin or BeginMesh that it fails
    // as expected. Also tests the case that Begin *is*
    // called but BeginMesh is not.
    TEST_F(ModelTests, SetMaterialSlotNoBeginNoBeginMesh)
    {
        using namespace AZ;

        RPI::ModelLodAssetCreator creator;

        {
            ErrorMessageFinder messageFinder("Begin() was not called");
            creator.SetMeshMaterialSlot(0);
        }

        creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));

        //This should still fail even if we call Begin but not BeginMesh
        {
            ErrorMessageFinder messageFinder("BeginMesh() was not called");
            creator.SetMeshMaterialSlot(0);
        }
    }

    // Tests that if we try to set the index buffer on a mesh
    // without calling Begin or BeginMesh that it fails
    // as expected. Also tests the case that Begin *is*
    // called but BeginMesh is not.
    TEST_F(ModelTests, SetIndexBufferNoBeginNoBeginMesh)
    {
        using namespace AZ;

        RPI::ModelLodAssetCreator creator;

        const uint32_t indexCount = 36;
        const uint32_t indexSize = sizeof(uint32_t);

        RHI::BufferViewDescriptor validIndexBufferViewDescriptor =
            RHI::BufferViewDescriptor::CreateStructured(0, indexCount, indexSize);

        Data::Asset<RPI::BufferAsset> validIndexBuffer = BuildTestBuffer(indexCount, indexSize);
        ASSERT_TRUE(validIndexBuffer.Get() != nullptr);

        {
            ErrorMessageFinder messageFinder("Begin() was not called");
            creator.SetMeshIndexBuffer({ AZStd::move(validIndexBuffer), validIndexBufferViewDescriptor });
        }

        creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));

        //This should still fail even if we call Begin but not BeginMesh
        validIndexBuffer = BuildTestBuffer(indexCount, indexSize);
        ASSERT_TRUE(validIndexBuffer.Get() != nullptr);

        {
            ErrorMessageFinder messageFinder("BeginMesh() was not called");
            creator.SetMeshIndexBuffer({ AZStd::move(validIndexBuffer), validIndexBufferViewDescriptor });
        }
    }

    // Tests that if we try to add a stream buffer on a mesh
    // without calling Begin or BeginMesh that it fails
    // as expected. Also tests the case that Begin *is*
    // called but BeginMesh is not.
    TEST_F(ModelTests, AddStreamBufferNoBeginNoBeginMesh)
    {
        using namespace AZ;

        RPI::ModelLodAssetCreator creator;

        const uint32_t vertexCount = 36;
        const uint32_t vertexSize = sizeof(float) * 3;

        RHI::BufferViewDescriptor validStreamBufferViewDescriptor =
            RHI::BufferViewDescriptor::CreateStructured(0, vertexCount, vertexSize);

        Data::Asset<RPI::BufferAsset> validStreamBuffer = BuildTestBuffer(vertexCount, vertexSize);

        {
            ErrorMessageFinder messageFinder("Begin() was not called");
            creator.AddMeshStreamBuffer(GetPositionSemantic(), AZ::Name(), { AZStd::move(validStreamBuffer), validStreamBufferViewDescriptor });
        }

        creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));

        //This should still fail even if we call Begin but not BeginMesh
        validStreamBuffer = BuildTestBuffer(vertexCount, vertexSize);

        {
            ErrorMessageFinder messageFinder("BeginMesh() was not called");
            creator.AddMeshStreamBuffer(GetPositionSemantic(), AZ::Name(), { AZStd::move(validStreamBuffer), validStreamBufferViewDescriptor });
        }
    }

    // Tests that if we try to end the creation of a
    // ModelLodAsset that has no meshes that it fails
    // as expected.
    TEST_F(ModelTests, CreateLodNoMeshes)
    {
        using namespace AZ;

        RPI::ModelLodAssetCreator creator;

        creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));

        ErrorMessageFinder messageFinder("No meshes have been provided for this LOD");

        Data::Asset<RPI::ModelLodAsset> asset;
        ASSERT_FALSE(creator.End(asset));
        ASSERT_FALSE(asset.IsReady());
        ASSERT_EQ(asset.Get(), nullptr);
    }

    // Tests that validation still fails when expected
    // even after producing a valid mesh due to a missing
    // BeginMesh call
    TEST_F(ModelTests, SecondMeshFailureNoBeginMesh)
    {
        using namespace AZ;

        RPI::ModelLodAssetCreator creator;

        creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));

        uint32_t indexCount = 36;
        uint32_t vertexCount = 36;

        RHI::BufferViewDescriptor indexBufferViewDescriptor =
            RHI::BufferViewDescriptor::CreateStructured(0, indexCount, sizeof(uint32_t));

        RHI::BufferViewDescriptor vertexBufferViewDescriptor =
            RHI::BufferViewDescriptor::CreateStructured(0, vertexCount, sizeof(float) * 3);

        //Creating this first mesh should work as expected
        {
            AZ::Aabb aabb = AZ::Aabb::CreateCenterRadius(Vector3::CreateZero(), 1.0f);

            creator.BeginMesh();
            creator.SetMeshAabb(AZStd::move(aabb));
            creator.SetMeshMaterialSlot(0);
            creator.SetMeshIndexBuffer({ BuildTestBuffer(indexCount, sizeof(uint32_t)), indexBufferViewDescriptor });
            creator.AddMeshStreamBuffer(GetPositionSemantic(), AZ::Name(), { BuildTestBuffer(vertexCount, sizeof(float) * 3), vertexBufferViewDescriptor });

            creator.EndMesh();
        }

        // This second mesh should fail at every point since we have forgotten to
        // call BeginMesh again
        {
            AZ::Aabb aabb = AZ::Aabb::CreateCenterRadius(Vector3::CreateZero(), 1.0f);

            ErrorMessageFinder messageFinder("BeginMesh() was not called", 5);

            creator.SetMeshAabb(AZStd::move(aabb));
            creator.SetMeshMaterialSlot(0);
            creator.SetMeshIndexBuffer({ BuildTestBuffer(indexCount, sizeof(uint32_t)), indexBufferViewDescriptor });
            creator.AddMeshStreamBuffer(GetPositionSemantic(), AZ::Name(), { BuildTestBuffer(vertexCount, sizeof(float) * 3), vertexBufferViewDescriptor });

            creator.EndMesh();
        }

        // We should still be able to produce a valid asset however
        Data::Asset<RPI::ModelLodAsset> asset;
        EXPECT_TRUE(creator.End(asset));
        EXPECT_TRUE(asset.IsReady());
        EXPECT_NE(asset.Get(), nullptr);

        // Make sure that this lod only has one mesh like we expect
        ASSERT_EQ(asset->GetMeshes().size(), 1);
    }

    // Tests that validation still fails when expected
    // even after producing a valid mesh due to SetMeshX
    // calls coming after End
    TEST_F(ModelTests, SecondMeshAfterEnd)
    {
        using namespace AZ;

        RPI::ModelLodAssetCreator creator;

        creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));

        uint32_t indexCount = 36;
        uint32_t vertexCount = 36;

        RHI::BufferViewDescriptor indexBufferViewDescriptor =
            RHI::BufferViewDescriptor::CreateStructured(0, indexCount, sizeof(uint32_t));

        RHI::BufferViewDescriptor vertexBufferViewDescriptor =
            RHI::BufferViewDescriptor::CreateStructured(0, vertexCount, sizeof(float) * 3);

        //Creating this first mesh should work as expected
        {
            AZ::Aabb aabb = AZ::Aabb::CreateCenterRadius(Vector3::CreateZero(), 1.0f);

            creator.BeginMesh();
            creator.SetMeshAabb(AZStd::move(aabb));
            creator.SetMeshMaterialSlot(0);
            creator.SetMeshIndexBuffer({ BuildTestBuffer(indexCount, sizeof(uint32_t)), indexBufferViewDescriptor });
            creator.AddMeshStreamBuffer(GetPositionSemantic(), AZ::Name(), { BuildTestBuffer(vertexCount, sizeof(float) * 3), vertexBufferViewDescriptor });

            creator.EndMesh();
        }

        // This asset creation should be valid
        Data::Asset<RPI::ModelLodAsset> asset;
        EXPECT_TRUE(creator.End(asset));
        EXPECT_TRUE(asset.IsReady());
        EXPECT_NE(asset.Get(), nullptr);

        // This second mesh should fail at every point since we have already
        // called End
        {
            AZ::Aabb aabb = AZ::Aabb::CreateCenterRadius(Vector3::CreateZero(), 1.0f);

            ErrorMessageFinder messageFinder("Begin() was not called", 6);

            creator.BeginMesh();
            creator.SetMeshAabb(AZStd::move(aabb));
            creator.SetMeshMaterialSlot(0);
            creator.SetMeshIndexBuffer({ BuildTestBuffer(indexCount, sizeof(uint32_t)), indexBufferViewDescriptor });
            creator.AddMeshStreamBuffer(GetPositionSemantic(), AZ::Name(), { BuildTestBuffer(vertexCount, sizeof(float) * 3), vertexBufferViewDescriptor });

            creator.EndMesh();
        }
    }

    TEST_F(ModelTests, UvStream)
    {
        AZ::RPI::UvStreamTangentBitmask uvStreamTangentBitmask;
        EXPECT_EQ(uvStreamTangentBitmask.GetFullTangentBitmask(), 0u);

        uvStreamTangentBitmask.ApplyTangent(1u);
        EXPECT_EQ(uvStreamTangentBitmask.GetTangentAtUv(0u), 1u);
        EXPECT_EQ(uvStreamTangentBitmask.GetFullTangentBitmask(), 0x10000001);
        EXPECT_EQ(uvStreamTangentBitmask.GetUvStreamCount(), 1u);

        uvStreamTangentBitmask.ApplyTangent(5u);
        EXPECT_EQ(uvStreamTangentBitmask.GetTangentAtUv(0u), 1u);
        EXPECT_EQ(uvStreamTangentBitmask.GetTangentAtUv(1u), 5u);
        EXPECT_EQ(uvStreamTangentBitmask.GetFullTangentBitmask(), 0x20000051);
        EXPECT_EQ(uvStreamTangentBitmask.GetUvStreamCount(), 2u);

        uvStreamTangentBitmask.ApplyTangent(100u);
        EXPECT_EQ(uvStreamTangentBitmask.GetTangentAtUv(0u), 1u);
        EXPECT_EQ(uvStreamTangentBitmask.GetTangentAtUv(1u), 5u);
        EXPECT_EQ(uvStreamTangentBitmask.GetTangentAtUv(2u), AZ::RPI::UvStreamTangentBitmask::UnassignedTangent);
        EXPECT_EQ(uvStreamTangentBitmask.GetFullTangentBitmask(), 0x30000F51);
        EXPECT_EQ(uvStreamTangentBitmask.GetUvStreamCount(), 3u);

        for (uint32_t i = 3; i < AZ::RPI::UvStreamTangentBitmask::MaxUvSlots; ++i)
        {
            uvStreamTangentBitmask.ApplyTangent(0u);
        }

        EXPECT_EQ(uvStreamTangentBitmask.GetFullTangentBitmask(), 0x70000F51);

        AZ_TEST_START_TRACE_SUPPRESSION;
        uvStreamTangentBitmask.ApplyTangent(0u);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        EXPECT_EQ(uvStreamTangentBitmask.GetFullTangentBitmask(), 0x70000F51);
    }

    /*
         +----+
        /    /|
       +----+ |
       |    | +
       |    |/
       +----+
    */
    static constexpr AZStd::array CubePositions = { -1.0f, 1.0f, 1.0f,  1.0f, 1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f, -1.0f, 1.0f,
                                                    -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f };
    static constexpr AZStd::array CubeIndices = {
        uint32_t{ 0 }, 2, 1, 1, 2, 3, 4, 5, 6, 5, 7, 6, 0, 4, 2, 4, 6, 2, 1, 3, 5, 5, 3, 7, 0, 1, 4, 4, 1, 5, 2, 6, 3, 6, 7, 3,
    };

    static constexpr AZStd::array QuadPositions = { -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f };
    static constexpr AZStd::array QuadIndices = { uint32_t{ 0 }, 2, 1, 1, 2, 3 };

    /*
       This class creates a Model with one LOD, whose mesh contains 2 planes. Plane 1 is in the XY plane at Z=-0.5, and
       plane 2 is in the XY plane at Z=0.5. The two planes each have 9 quads which have been triangulated. It only has
       a position and index buffer.

            -0.33
          -1     0.33  1
       0.5 *---*---*---*
            \ / \ / \ / \
             *---*---*---*
              \ / \ / \ / \
       -0.5 *- *---*---*---*
             \  \ / \ / \ / \
              *- *---*---*---*
               \   \   \   \
                *---*---*---*
                 \ / \ / \ / \
                  *---*---*---*
    */
    static constexpr AZStd::array TwoSeparatedPlanesPositions{
        -1.0f,   -0.333f, -0.5f, -0.333f, -1.0f,   -0.5f, -0.333f, -0.333f, -0.5f, 0.333f, -0.333f, -0.5f, 1.0f,    -1.0f,   -0.5f,
        1.0f,    -0.333f, -0.5f, 0.333f,  -1.0f,   -0.5f, 0.333f,  1.0f,    -0.5f, 1.0f,   0.333f,  -0.5f, 1.0f,    1.0f,    -0.5f,
        0.333f,  0.333f,  -0.5f, -0.333f, 1.0f,    -0.5f, -0.333f, 0.333f,  -0.5f, -1.0f,  1.0f,    -0.5f, -1.0f,   0.333f,  -0.5f,
        -1.0f,   -0.333f, 0.5f,  -0.333f, -1.0f,   0.5f,  -0.333f, -0.333f, 0.5f,  0.333f, -0.333f, 0.5f,  1.0f,    -1.0f,   0.5f,
        1.0f,    -0.333f, 0.5f,  -0.333f, -0.333f, 0.5f,  0.333f,  -1.0f,   0.5f,  0.333f, -0.333f, 0.5f,  0.333f,  1.0f,    0.5f,
        1.0f,    0.333f,  0.5f,  1.0f,    1.0f,    0.5f,  0.333f,  0.333f,  0.5f,  1.0f,   -0.333f, 0.5f,  -0.333f, 1.0f,    0.5f,
        -0.333f, 0.333f,  0.5f,  0.333f,  -0.333f, 0.5f,  0.333f,  0.333f,  0.5f,  -1.0f,  1.0f,    0.5f,  -0.333f, 0.333f,  0.5f,
        -1.0f,   0.333f,  0.5f,  -1.0f,   -1.0f,   -0.5f, -1.0f,   -1.0f,   0.5f,  0.333f, -0.333f, 0.5f,  0.333f,  -1.0f,   0.5f,
        1.0f,    -1.0f,   0.5f,  0.333f,  -1.0f,   0.5f,  0.333f,  0.333f,  0.5f,  0.333f, -0.333f, 0.5f,  1.0f,    -0.333f, 0.5f,
        -0.333f, 0.333f,  0.5f,  -0.333f, -0.333f, 0.5f,  0.333f,  -0.333f, 0.5f,
    };
    // clang-format off
    static constexpr AZStd::array TwoSeparatedPlanesIndices{
        uint32_t{ 0 }, 1,  2,  3,  4,  5,  2,  6,  3,  7,  8,  9,  10, 5,  8,  11, 10, 7,  12, 3,  10, 13, 12, 11, 14, 2,  12,
        15,            16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 25, 29, 27, 24, 30, 31, 32, 33, 34, 29, 35, 17, 34,
        0,             36, 1,  3,  6,  4,  2,  1,  6,  7,  10, 8,  10, 3,  5,  11, 12, 10, 12, 2,  3,  13, 14, 12, 14, 0,  2,
        15,            37, 16, 38, 39, 40, 17, 16, 41, 24, 27, 25, 42, 43, 44, 29, 34, 27, 45, 46, 47, 33, 35, 34, 35, 15, 17,
    };
    // clang-format on

    // Ensure that the index buffer references all the positions in the position buffer
    static constexpr inline auto minmaxElement = AZStd::minmax_element(begin(TwoSeparatedPlanesIndices), end(TwoSeparatedPlanesIndices));
    static_assert(*minmaxElement.second == (TwoSeparatedPlanesPositions.size() / 3) - 1);

    class TestMesh
    {
    public:
        TestMesh() = default;

        TestMesh(const float* positions, size_t positionCount, const uint32_t* indices, size_t indicesCount)
        {
            AZ::RPI::ModelLodAssetCreator lodCreator;
            Begin(lodCreator);
            Add(lodCreator, positions, positionCount, /*positionOffset=*/0, indices, indicesCount, /*indexOffset=*/0);
            End(lodCreator);
        }

        // initiate the asset lod creation process (note: End must be called after meshes have been added).
        void Begin(AZ::RPI::ModelLodAssetCreator& lodCreator)
        {
            lodCreator.Begin(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
        }

        // add a sub mesh and reuse existing position/index buffer (be very careful with the offsets used)
        void Add(
            AZ::RPI::ModelLodAssetCreator& lodCreator,
            const float* positions,
            size_t positionCount,
            size_t positionOffset,
            AZ::Data::Asset<AZ::RPI::BufferAsset> positionBuffer,
            const uint32_t* indices,
            size_t indexCount,
            size_t indexOffset,
            AZ::Data::Asset<AZ::RPI::BufferAsset> indexBuffer)
        {
            lodCreator.BeginMesh();
            lodCreator.SetMeshAabb(AZ::Aabb::CreateFromMinMax({ -1.0f, -1.0f, -0.5f }, { 1.0f, 1.0f, 0.5f }));
            lodCreator.SetMeshMaterialSlot(AZ::Sfmt::GetInstance().Rand32());

            AZStd::copy(
                indices, indices + indexCount,
                reinterpret_cast<uint32_t*>(const_cast<uint8_t*>(indexBuffer->GetBuffer().data())) + indexOffset);
            lodCreator.SetMeshIndexBuffer(
                { indexBuffer,
                  AZ::RHI::BufferViewDescriptor::CreateStructured(
                      aznumeric_cast<uint32_t>(indexOffset), aznumeric_cast<uint32_t>(indexCount), sizeof(uint32_t)) });
            AZStd::copy(
                positions, positions + positionCount,
                reinterpret_cast<float*>(const_cast<uint8_t*>(positionBuffer->GetBuffer().data())) + positionOffset);
            lodCreator.AddMeshStreamBuffer(
                AZ::RHI::ShaderSemantic(AZ::Name("POSITION")), AZ::Name(),
                { positionBuffer,
                  AZ::RHI::BufferViewDescriptor::CreateStructured(
                      aznumeric_cast<uint32_t>(positionOffset / 3), aznumeric_cast<uint32_t>(positionCount / 3), sizeof(float) * 3) });

            lodCreator.EndMesh();
        }

        // overload of Add - here a new index/position buffer is created for the new data instead of potentially reusing an existing buffer
        void Add(
            AZ::RPI::ModelLodAssetCreator& lodCreator,
            const float* positions,
            size_t positionCount,
            size_t positionOffset,
            const uint32_t* indices,
            size_t indexCount,
            size_t indexOffset)
        {
            AZ::Data::Asset<AZ::RPI::BufferAsset> indexBuffer = BuildTestBuffer(aznumeric_cast<uint32_t>(indexCount), sizeof(uint32_t));
            AZ::Data::Asset<AZ::RPI::BufferAsset> positionBuffer =
                BuildTestBuffer(aznumeric_cast<uint32_t>(positionCount / 3), sizeof(float) * 3);

            Add(lodCreator, positions, positionCount, positionOffset, positionBuffer, indices, indexCount, indexOffset, indexBuffer);
        }

        // complete the asset lod creation process
        void End(AZ::RPI::ModelLodAssetCreator& lodCreator)
        {
            AZ::Data::Asset<AZ::RPI::ModelLodAsset> lodAsset;
            lodCreator.End(lodAsset);

            AZ::RPI::ModelAssetCreator modelCreator;
            modelCreator.Begin(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
            modelCreator.SetName("TestModel");
            modelCreator.AddLodAsset(AZStd::move(lodAsset));
            modelCreator.End(m_modelAsset);
        }

        [[nodiscard]] AZ::Data::Asset<AZ::RPI::ModelAsset> GetModel() const
        {
            return m_modelAsset;
        }

    private:
        AZ::Data::Asset<AZ::RPI::ModelAsset> m_modelAsset;
    };

    struct IntersectParams
    {
        float xpos;
        float ypos;
        float zpos;
        float xdir;
        float ydir;
        float zdir;
        float expectedDistance;
        bool expectedShouldIntersect;

        friend std::ostream& operator<<(std::ostream& os, const IntersectParams& param)
        {
            return os
                << "xpos:" << param.xpos
                << ", ypos:" << param.ypos
                << ", zpos:" << param.zpos
                << ", dist:" << param.expectedDistance
                << ", shouldIntersect:" << param.expectedShouldIntersect;
        }
    };

    class KdTreeIntersectsParameterizedFixture
        : public ModelTests
        , public ::testing::WithParamInterface<IntersectParams>
    {
    };

    TEST_P(KdTreeIntersectsParameterizedFixture, KdTreeIntersects)
    {
        TestMesh mesh(
            TwoSeparatedPlanesPositions.data(), TwoSeparatedPlanesPositions.size(), TwoSeparatedPlanesIndices.data(),
            TwoSeparatedPlanesIndices.size());

        AZ::RPI::ModelKdTree kdTree;
        ASSERT_TRUE(kdTree.Build(mesh.GetModel().Get()));

        float distance = AZStd::numeric_limits<float>::max();
        AZ::Vector3 normal;

        EXPECT_THAT(
            kdTree.RayIntersection(
                AZ::Vector3(GetParam().xpos, GetParam().ypos, GetParam().zpos),
                AZ::Vector3(GetParam().xdir, GetParam().ydir, GetParam().zdir), distance, normal),
            testing::Eq(GetParam().expectedShouldIntersect));
        EXPECT_THAT(distance, testing::FloatEq(GetParam().expectedDistance));
    }

    static constexpr AZStd::array KdTreeIntersectTestData{
        IntersectParams{ -0.1f, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f, true },
        IntersectParams{ 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f, true },
        IntersectParams{ 0.1f, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f, true },

        // Test the center of each triangle
        IntersectParams{ -0.111f, -0.111f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f, true },
        IntersectParams{ -0.111f, -0.778f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f, true },
        IntersectParams{ -0.111f, 0.555f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f,
                         true }, // Should intersect triangle with indices {29, 34, 27} and {11, 12, 10}
        IntersectParams{ -0.555f, -0.555f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f, true },
        IntersectParams{ -0.555f, 0.111f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f, true },
        IntersectParams{ -0.555f, 0.778f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f, true },
        IntersectParams{ -0.778f, -0.111f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f, true },
        IntersectParams{ -0.778f, -0.778f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f, true },
        IntersectParams{ -0.778f, 0.555f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f, true },
        IntersectParams{ 0.111f, -0.555f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f, true },
        IntersectParams{ 0.111f, 0.111f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f, true },
        IntersectParams{ 0.111f, 0.778f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f, true },
        IntersectParams{ 0.555f, -0.111f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f, true },
        IntersectParams{ 0.555f, -0.778f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f, true },
        IntersectParams{ 0.555f, 0.555f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f, true },
        IntersectParams{ 0.778f, -0.555f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f, true },
        IntersectParams{ 0.778f, 0.111f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f, true },
        IntersectParams{ 0.778f, 0.778f, 1.0f, 0.0f, 0.0f, -1.0f, 0.5f, true },
    };

    INSTANTIATE_TEST_CASE_P(KdTreeIntersectsPlane, KdTreeIntersectsParameterizedFixture, ::testing::ValuesIn(KdTreeIntersectTestData));

    class KdTreeIntersectsFixture
        : public ModelTests
    {
    public:
        void SetUp() override
        {
            ModelTests::SetUp();

            m_mesh = AZStd::make_unique<TestMesh>(
                TwoSeparatedPlanesPositions.data(), TwoSeparatedPlanesPositions.size(), TwoSeparatedPlanesIndices.data(),
                TwoSeparatedPlanesIndices.size());

            m_kdTree = AZStd::make_unique<AZ::RPI::ModelKdTree>();
            ASSERT_TRUE(m_kdTree->Build(m_mesh->GetModel().Get()));
        }

        void TearDown() override
        {
            m_kdTree.reset();
            m_mesh.reset();

            ModelTests::TearDown();
        }

        AZStd::unique_ptr<TestMesh> m_mesh;
        AZStd::unique_ptr<AZ::RPI::ModelKdTree> m_kdTree;
    };

    TEST_F(KdTreeIntersectsFixture, KdTreeIntersectionReturnsNormalizedDistance)
    {
        float t = AZStd::numeric_limits<float>::max();
        AZ::Vector3 normal;

        constexpr float rayLength = 100.0f;
        EXPECT_THAT(
            m_kdTree->RayIntersection(
                AZ::Vector3::CreateZero(), AZ::Vector3::CreateAxisZ(-rayLength), t, normal), testing::IsTrue());
        EXPECT_THAT(t, testing::FloatEq(0.005f));
    }

    TEST_F(KdTreeIntersectsFixture, KdTreeIntersectionHandlesInvalidStartingNormalizedDistance)
    {
        float t = -0.5f; // invalid starting distance
        AZ::Vector3 normal;

        constexpr float rayLength = 10.0f;
        EXPECT_THAT(
            m_kdTree->RayIntersection(AZ::Vector3::CreateAxisZ(0.75f), AZ::Vector3::CreateAxisZ(-rayLength), t, normal), testing::IsTrue());
        EXPECT_THAT(t, testing::FloatEq(0.025f));
    }

    TEST_F(KdTreeIntersectsFixture, KdTreeIntersectionDoesNotScaleRayByStartingDistance)
    {
        float t = 10.0f; // starting distance (used to check it is not read from initially by RayIntersection)
        AZ::Vector3 normal;

        EXPECT_THAT(
            m_kdTree->RayIntersection(AZ::Vector3::CreateAxisZ(5.0f), -AZ::Vector3::CreateAxisZ(), t, normal), testing::Eq(false));
    }

    class BruteForceIntersectsParameterizedFixture
        : public ModelTests
        , public ::testing::WithParamInterface<IntersectParams>
    {
    };

    TEST_P(BruteForceIntersectsParameterizedFixture, BruteForceIntersectsCube)
    {
        TestMesh mesh(CubePositions.data(), CubePositions.size(), CubeIndices.data(), CubeIndices.size());

        float distance = AZStd::numeric_limits<float>::max();
        AZ::Vector3 normal;
        constexpr bool AllowBruteForce = false;

        EXPECT_THAT(
            mesh.GetModel()->LocalRayIntersectionAgainstModel(
                AZ::Vector3(GetParam().xpos, GetParam().ypos, GetParam().zpos),
                AZ::Vector3(GetParam().xdir, GetParam().ydir, GetParam().zdir), AllowBruteForce, distance, normal),
            testing::Eq(GetParam().expectedShouldIntersect));
        EXPECT_THAT(distance, testing::FloatEq(GetParam().expectedDistance));
    }

    static constexpr AZStd::array BruteForceIntersectTestData{
        IntersectParams{ 5.0f, 0.0f, 5.0f, 0.0f, 0.0f, -1.0f, AZStd::numeric_limits<float>::max(), false },
        IntersectParams{ 0.0f, 0.0f, 1.5f, 0.0f, 0.0f, -1.0f, 0.5f, true },
        IntersectParams{ 5.0f, 0.0f, 0.0f, -10.0f, 0.0f, 0.0f, 0.4f, true },
        IntersectParams{ -5.0f, 0.0f, 0.0f, 20.0f, 0.0f, 0.0f, 0.2f, true },
        IntersectParams{ 0.0f, -10.0f, 0.0f, 0.0f, 20.0f, 0.0f, 0.45f, true },
        IntersectParams{ 0.0f,  20.0f, 0.0f, 0.0f, -40.0f, 0.0f, 0.475f, true },
        IntersectParams{ 0.0f,  20.0f, 0.0f, 0.0f, -19.0f, 0.0f, 1.0f, true },
    };

    INSTANTIATE_TEST_CASE_P(
        BruteForceIntersects, BruteForceIntersectsParameterizedFixture, ::testing::ValuesIn(BruteForceIntersectTestData));

    class BruteForceModelIntersectsFixture
        : public ModelTests
    {
    public:
        void SetUp() override
        {
            ModelTests::SetUp();
            m_mesh = AZStd::make_unique<TestMesh>(CubePositions.data(), CubePositions.size(), CubeIndices.data(), CubeIndices.size());
        }

        void TearDown() override
        {
            m_mesh.reset();
            ModelTests::TearDown();
        }

        AZStd::unique_ptr<TestMesh> m_mesh;
    };

    TEST_F(BruteForceModelIntersectsFixture, BruteForceIntersectionDetectedWithCube)
    {
        float t = 0.0f;
        AZ::Vector3 normal;

        // firing down the negative z axis, positioned 5 units from cube (cube is 2x2x2 so intersection
        // happens at 1 in z)
        constexpr bool AllowBruteForce = false;
        EXPECT_THAT(
            m_mesh->GetModel()->LocalRayIntersectionAgainstModel(
                AZ::Vector3::CreateAxisZ(5.0f), -AZ::Vector3::CreateAxisZ(10.0f), AllowBruteForce, t, normal),
            testing::IsTrue());
        EXPECT_THAT(t, testing::FloatEq(0.4f));
    }

    TEST_F(BruteForceModelIntersectsFixture, BruteForceIntersectionDetectedAndNormalSetAtEndOfRay)
    {
        float t = 0.0f;
        AZ::Vector3 normal = AZ::Vector3::CreateOne(); // invalid starting normal

        // ensure the intersection happens right at the end of the ray
        constexpr bool AllowBruteForce = false;
        EXPECT_THAT(
            m_mesh->GetModel()->LocalRayIntersectionAgainstModel(
                AZ::Vector3::CreateAxisY(10.0f), -AZ::Vector3::CreateAxisY(9.0f), AllowBruteForce, t, normal),
            testing::IsTrue());
        EXPECT_THAT(t, testing::FloatEq(1.0f));
        EXPECT_THAT(normal, IsClose(AZ::Vector3::CreateAxisY()));
    }

    // test to verify that each secondary sub meshes are still intersected with correctly when using brute-force
    // ray intersection
    class BruteForceMultiModelIntersectsFixture : public ModelTests
    {
    public:
        inline static const float QuadOffsetX = 15.0f;

        void SetUp() override
        {
            ModelTests::SetUp();
            m_mesh = AZStd::make_unique<TestMesh>();

            AZ::RPI::ModelLodAssetCreator lodCreator;
            m_mesh->Begin(lodCreator);

            // take default quad positions and offset in X by set amount
            AZStd::vector<float> offsetQuadPositions;
            offsetQuadPositions.resize(QuadPositions.size());
            AZStd::copy(QuadPositions.begin(), QuadPositions.end(), offsetQuadPositions.begin());
            for (size_t xVertIndex = 0; xVertIndex < offsetQuadPositions.size(); xVertIndex += 3)
            {
                offsetQuadPositions[xVertIndex] += QuadOffsetX;
            }

            // create shared buffer to store cube and quad mesh in the same buffer
            const size_t indicesCount = QuadIndices.size() + CubeIndices.size();
            const size_t positionCount = QuadPositions.size() + CubePositions.size();
            AZ::Data::Asset<AZ::RPI::BufferAsset> indexBuffer = BuildTestBuffer(aznumeric_cast<uint32_t>(indicesCount), sizeof(uint32_t));
            AZ::Data::Asset<AZ::RPI::BufferAsset> positionBuffer =
                BuildTestBuffer(aznumeric_cast<uint32_t>(positionCount / 3), sizeof(float) * 3);

            // add the cube mesh
            m_mesh->Add(
                lodCreator, CubePositions.data(), CubePositions.size(), 0, positionBuffer, CubeIndices.data(), CubeIndices.size(), 0,
                indexBuffer);
            // add the quad mesh (offset by the cube position and index data into the same buffer)
            m_mesh->Add(
                lodCreator, offsetQuadPositions.data(), offsetQuadPositions.size(), /*offset=*/CubePositions.size(), positionBuffer,
                QuadIndices.data(), QuadIndices.size(), /*offset=*/CubeIndices.size(), indexBuffer);

            m_mesh->End(lodCreator);
        }

        void TearDown() override
        {
            m_mesh.reset();
            ModelTests::TearDown();
        }

        AZStd::unique_ptr<TestMesh> m_mesh;
        inline static constexpr bool AllowBruteForce = false;
    };

    TEST_F(BruteForceMultiModelIntersectsFixture, RayIntersectsWithFirstSubMesh)
    {
        float t = 0.0f;
        AZ::Vector3 normal = AZ::Vector3::CreateOne(); // invalid starting normal
        // fire a ray at the first sub mesh and ensure a successful hit is returned
        EXPECT_THAT(
            m_mesh->GetModel()->LocalRayIntersectionAgainstModel(
                AZ::Vector3(0.0f, 0.0f, 5.0f), -AZ::Vector3::CreateAxisZ(10.0f), AllowBruteForce, t, normal),
            testing::IsTrue());
        EXPECT_THAT(t, testing::FloatEq(0.4f));
        EXPECT_THAT(normal, IsClose(AZ::Vector3::CreateAxisZ()));
    }

    TEST_F(BruteForceMultiModelIntersectsFixture, RayIntersectsWithSecondSubMesh)
    {
        float t = 0.0f;
        AZ::Vector3 normal = AZ::Vector3::CreateOne(); // invalid starting normal
        // fire a ray at the second sub mesh and ensure a successful hit is returned
        EXPECT_THAT(
            m_mesh->GetModel()->LocalRayIntersectionAgainstModel(
                AZ::Vector3(QuadOffsetX, 0.0f, 5.0f), -AZ::Vector3::CreateAxisZ(10.0f), AllowBruteForce, t, normal),
            testing::IsTrue());
        EXPECT_THAT(t, testing::FloatEq(0.5f));
        EXPECT_THAT(normal, IsClose(AZ::Vector3::CreateAxisZ()));
    }
} // namespace UnitTest
