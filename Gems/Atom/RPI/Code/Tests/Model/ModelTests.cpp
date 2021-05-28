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
        for (uint32_t i = 0; i < bufferData.size(); ++i)
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
            AZ::Data::Asset<AZ::RPI::MaterialAsset> m_material;
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

        AZ::Data::Asset<AZ::RPI::ModelLodAsset> BuildTestLod(const uint32_t sharedMeshCount, const uint32_t separateMeshCount, ExpectedLod& expectedLod)
        {
            using namespace AZ;

            //Create an Lod with a given number of meshes
            RPI::ModelLodAssetCreator creator;

            creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));

            const uint32_t indexCount = 36;
            const uint32_t vertexCount = 36;

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
                    expectedMesh.m_material = m_materialAsset;

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
                    creator.SetMeshMaterialAsset(m_materialAsset);
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
                expectedMesh.m_material = m_materialAsset;

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
                creator.SetMeshMaterialAsset(m_materialAsset);
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

        AZ::Data::Asset<AZ::RPI::ModelAsset> BuildTestModel(const uint32_t lodCount, const uint32_t sharedMeshCount, const uint32_t separateMeshCount, ExpectedModel& expectedModel)
        {
            using namespace AZ;

            RPI::ModelAssetCreator creator;

            creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));
            creator.SetName("TestModel");

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
            EXPECT_TRUE(mesh.GetMaterialAsset() == expectedMesh.m_material);
        }

        void ValidateLodAsset(const AZ::RPI::ModelLodAsset* lodAsset, const ExpectedLod& expectedLod)
        {
            ASSERT_NE(lodAsset, nullptr);
            EXPECT_TRUE(lodAsset->GetAabb().IsValid());
            EXPECT_TRUE(lodAsset->GetMeshes().size() == expectedLod.m_meshes.size());
            EXPECT_TRUE(lodAsset->GetAabb() == expectedLod.m_aabb);

            for (size_t i = 0; i < lodAsset->GetMeshes().size(); ++i)
            {
                const AZ::RPI::ModelLodAsset::Mesh& mesh = lodAsset->GetMeshes()[i];
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
        AZ::Data::Asset<AZ::RPI::MaterialAsset> m_materialAsset =
            AZ::Data::Asset<AZ::RPI::MaterialAsset>(AZ::Data::AssetId(AZ::Uuid::CreateRandom(), 0),
                AZ::AzTypeInfo<AZ::RPI::MaterialAsset>::Uuid(), "");

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

    // Tests that if we try to set the material id on a mesh
    // without calling Begin or BeginMesh that it fails 
    // as expected. Also tests the case that Begin *is*
    // called but BeginMesh is not.
    TEST_F(ModelTests, SetMaterialIdNoBeginNoBeginMesh)
    {
        using namespace AZ;
        
        RPI::ModelLodAssetCreator creator;

        {
            ErrorMessageFinder messageFinder("Begin() was not called");
            creator.SetMeshMaterialAsset(m_materialAsset);
        }

        creator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));

        //This should still fail even if we call Begin but not BeginMesh
        {
            ErrorMessageFinder messageFinder("BeginMesh() was not called");
            creator.SetMeshMaterialAsset(m_materialAsset);
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
        const uint32_t vertexBufferSize = vertexCount * vertexSize;

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
            creator.SetMeshMaterialAsset(m_materialAsset);
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
            creator.SetMeshMaterialAsset(m_materialAsset);
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
            creator.SetMeshMaterialAsset(m_materialAsset);
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
            creator.SetMeshMaterialAsset(m_materialAsset);
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

    // This class creates a Model with one LOD, whose mesh contains 2 planes. Plane 1 is in the XY plane at Z=-0.5, and
    // plane 2 is in the XY plane at Z=0.5. The two planes each have 9 quads which have been triangulated. It only has
    // a position and index buffer.
    //
    //      -0.33
    //    -1     0.33  1
    // 0.5 *---*---*---*
    //      \ / \ / \ / \
    //       *---*---*---*
    //        \ / \ / \ / \
    // -0.5 *- *---*---*---*
    //       \  \ / \ / \ / \
    //        *- *---*---*---*
    //         \   \   \   \
    //          *---*---*---*
    //           \ / \ / \ / \
    //            *---*---*---*
    template<class x> class TD;
    class TwoSeparatedPlanesMesh
    {
    public:
        TwoSeparatedPlanesMesh()
        {
            using namespace AZ;

            RPI::ModelLodAssetCreator lodCreator;
            lodCreator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));

            lodCreator.BeginMesh();
            lodCreator.SetMeshAabb(Aabb::CreateFromMinMax({-1.0f, -1.0f, -0.5f}, {1.0f, 1.0f, 0.5f}));
            lodCreator.SetMeshMaterialAsset(
                AZ::Data::Asset<AZ::RPI::MaterialAsset>(AZ::Data::AssetId(AZ::Uuid::CreateRandom(), 0),
                    AZ::AzTypeInfo<AZ::RPI::MaterialAsset>::Uuid(), "")
            );

            {
                AZ::Data::Asset<AZ::RPI::BufferAsset> indexBuffer = BuildTestBuffer(s_indexes.size(), sizeof(uint32_t));
                AZStd::copy(s_indexes.begin(), s_indexes.end(), reinterpret_cast<uint32_t*>(const_cast<uint8_t*>(indexBuffer->GetBuffer().data())));
                lodCreator.SetMeshIndexBuffer({
                    indexBuffer,
                    RHI::BufferViewDescriptor::CreateStructured(0, s_indexes.size(), sizeof(uint32_t))
                });
            }

            {
                AZ::Data::Asset<AZ::RPI::BufferAsset> positionBuffer = BuildTestBuffer(s_positions.size() / 3, sizeof(float) * 3);
                AZStd::copy(s_positions.begin(), s_positions.end(), reinterpret_cast<float*>(const_cast<uint8_t*>(positionBuffer->GetBuffer().data())));
                lodCreator.AddMeshStreamBuffer(
                    AZ::RHI::ShaderSemantic(AZ::Name("POSITION")),
                    AZ::Name(),
                    {
                        positionBuffer,
                        RHI::BufferViewDescriptor::CreateStructured(0, s_positions.size() / 3, sizeof(float) * 3)
                    }
                );
            }
            lodCreator.EndMesh();

            Data::Asset<RPI::ModelLodAsset> lodAsset;
            lodCreator.End(lodAsset);

            RPI::ModelAssetCreator modelCreator;
            modelCreator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()));
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

        static constexpr AZStd::array s_positions{
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
        static constexpr AZStd::array s_indexes{
            uint32_t{0}, 1,  2,  3,  4,  5,  2,  6,  3,  7,  8,  9,  10, 5,  8,  11, 10, 7,  12, 3,  10, 13, 12, 11, 14, 2,  12,
            15,          16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 25, 29, 27, 24, 30, 31, 32, 33, 34, 29, 35, 17, 34,
            0,           36, 1,  3,  6,  4,  2,  1,  6,  7,  10, 8,  10, 3,  5,  11, 12, 10, 12, 2,  3,  13, 14, 12, 14, 0,  2,
            15,          37, 16, 38, 39, 40, 17, 16, 41, 24, 27, 25, 42, 43, 44, 29, 34, 27, 45, 46, 47, 33, 35, 34, 35, 15, 17,
        };

        // Ensure that the index buffer references all the positions in the position buffer
        static constexpr inline auto minmaxElement = AZStd::minmax_element(begin(s_indexes), end(s_indexes));
        static_assert(*minmaxElement.second == (s_positions.size() / 3) - 1);
    };

    struct KdTreeIntersectParams
    {
        float xpos;
        float ypos;
        float zpos;
        float expectedDistance;
        bool expectedShouldIntersect;

        friend std::ostream& operator<<(std::ostream& os, const KdTreeIntersectParams& param)
        {
            return os
                << "xpos:" << param.xpos
                << ", ypos:" << param.ypos
                << ", zpos:" << param.zpos
                << ", dist:" << param.expectedDistance
                << ", shouldIntersect:" << param.expectedShouldIntersect;
        }
    };

    class KdTreeIntersectsFixture
        : public ModelTests
        , public ::testing::WithParamInterface<KdTreeIntersectParams>
    {
    };

    TEST_P(KdTreeIntersectsFixture, KdTreeIntersects)
    {
        TwoSeparatedPlanesMesh mesh;

        AZ::RPI::ModelKdTree kdTree;
        ASSERT_TRUE(kdTree.Build(mesh.GetModel().Get()));

        float distance = AZStd::numeric_limits<float>::max();
        AZ::Vector3 normal;

        EXPECT_THAT(kdTree.RayIntersection(AZ::Vector3(GetParam().xpos, GetParam().ypos, GetParam().zpos), AZ::Vector3::CreateAxisZ(-1.0f), distance, normal), testing::Eq(GetParam().expectedShouldIntersect));
        EXPECT_THAT(distance, testing::FloatEq(GetParam().expectedDistance));
    }

    static constexpr inline AZStd::array<KdTreeIntersectParams, 21> intersectTestData{
        KdTreeIntersectParams{ -0.1f, 0.0f, 1.0f, 0.5f, true },
        KdTreeIntersectParams{  0.0f, 0.0f, 1.0f, 0.5f, true },
        KdTreeIntersectParams{  0.1f, 0.0f, 1.0f, 0.5f, true },

        // Test the center of each triangle
        KdTreeIntersectParams{-0.111f, -0.111f, 1.0f, 0.5f, true},
        KdTreeIntersectParams{-0.111f, -0.778f, 1.0f, 0.5f, true},
        KdTreeIntersectParams{-0.111f, 0.555f, 1.0f, 0.5f, true}, // Should intersect triangle with indices {29, 34, 27} and {11, 12, 10}
        KdTreeIntersectParams{-0.555f, -0.555f, 1.0f, 0.5f, true},
        KdTreeIntersectParams{-0.555f, 0.111f, 1.0f, 0.5f, true},
        KdTreeIntersectParams{-0.555f, 0.778f, 1.0f, 0.5f, true},
        KdTreeIntersectParams{-0.778f, -0.111f, 1.0f, 0.5f, true},
        KdTreeIntersectParams{-0.778f, -0.778f, 1.0f, 0.5f, true},
        KdTreeIntersectParams{-0.778f, 0.555f, 1.0f, 0.5f, true},
        KdTreeIntersectParams{0.111f, -0.555f, 1.0f, 0.5f, true},
        KdTreeIntersectParams{0.111f, 0.111f, 1.0f, 0.5f, true},
        KdTreeIntersectParams{0.111f, 0.778f, 1.0f, 0.5f, true},
        KdTreeIntersectParams{0.555f, -0.111f, 1.0f, 0.5f, true},
        KdTreeIntersectParams{0.555f, -0.778f, 1.0f, 0.5f, true},
        KdTreeIntersectParams{0.555f, 0.555f, 1.0f, 0.5f, true},
        KdTreeIntersectParams{0.778f, -0.555f, 1.0f, 0.5f, true},
        KdTreeIntersectParams{0.778f, 0.111f, 1.0f, 0.5f, true},
        KdTreeIntersectParams{0.778f, 0.778f, 1.0f, 0.5f, true},
    };
    INSTANTIATE_TEST_CASE_P(KdTreeIntersectsPlane, KdTreeIntersectsFixture, ::testing::ValuesIn(intersectTestData));
} // namespace UnitTest
