/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <Atom/RPI.Reflect/Buffer/BufferAssetCreator.h>
#include <Atom/RPI.Reflect/Model/ModelAssetCreator.h>
#include <Atom/RPI.Reflect/Model/ModelLodAssetCreator.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/RPI.Reflect/Model/ModelKdTree.h>
#include <Atom/RPI.Reflect/Model/ModelLodAsset.h>
#include <Atom/RPI.Reflect/ResourcePoolAssetCreator.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Math/Random.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/std/chrono/chrono.h>
#include <SurfaceData/Components/SurfaceDataSystemComponent.h>
#include <SurfaceDataModule.h>
#include <SurfaceData/SurfaceDataProviderRequestBus.h>
#include <SurfaceData/SurfaceDataModifierRequestBus.h>
#include <SurfaceData/SurfaceTag.h>
#include <SurfaceData/Utility/SurfaceDataUtility.h>
#include <Tests/SurfaceDataTestFixtures.h>

// Simple class for mocking out a surface provider, so that we can control exactly what points we expect to query in our tests.
// This can be used to either provide a surface or modify a surface.
class MockSurfaceProvider
    : private SurfaceData::SurfaceDataProviderRequestBus::Handler
    , private SurfaceData::SurfaceDataModifierRequestBus::Handler

{
    public:
        enum class ProviderType
        {
            SURFACE_PROVIDER,
            SURFACE_MODIFIER
        };

        MockSurfaceProvider(ProviderType providerType, const SurfaceData::SurfaceTagVector& surfaceTags,
                            AZ::Vector3 start, AZ::Vector3 end, AZ::Vector3 stepSize,
                            AZ::EntityId id = AZ::EntityId(0x12345678))
        {
            m_tags = surfaceTags;
            m_providerType = providerType;
            m_id = id;
            SetPoints(start, end, stepSize);
            Register();
        }

        ~MockSurfaceProvider()
        {
            Unregister();
        }

    private:
        AZStd::unordered_map<AZStd::pair<float, float>, AZStd::vector<AzFramework::SurfaceData::SurfacePoint>> m_surfacePoints;
        SurfaceData::SurfaceTagVector m_tags;
        ProviderType m_providerType;
        AZ::EntityId m_id;

        void SetPoints(AZ::Vector3 start, AZ::Vector3 end, AZ::Vector3 stepSize)
        {
            m_surfacePoints.clear();

            // Create a set of points that go from start to end (exclusive), with one
            // point per step size.
            // The XY values create new SurfacePoint entries, the Z values are used to create
            // the list of surface points at each XY input point.
            for (float y = start.GetY(); y < end.GetY(); y += stepSize.GetY())
            {
                for (float x = start.GetX(); x < end.GetX(); x += stepSize.GetX())
                {
                    AZStd::vector<AzFramework::SurfaceData::SurfacePoint> points;
                    for (float z = start.GetZ(); z < end.GetZ(); z += stepSize.GetZ())
                    {
                        AzFramework::SurfaceData::SurfacePoint point;
                        point.m_position = AZ::Vector3(x, y, z);
                        point.m_normal = AZ::Vector3::CreateAxisZ();
                        for (auto& tag : m_tags)
                        {
                            point.m_surfaceTags.emplace_back(tag, 1.0f);
                        }
                        points.push_back(point);
                    }
                    m_surfacePoints[AZStd::pair<float, float>(x, y)] = points;
                }
            }
        }

        AZ::Aabb GetBounds()
        {
            AZ::Aabb bounds = AZ::Aabb::CreateNull();

            for (auto& entry : m_surfacePoints)
            {
                for (auto& point : entry.second)
                {
                    bounds.AddPoint(point.m_position);
                }
            }

            return bounds;
        }

        void Register()
        {
            SurfaceData::SurfaceDataRegistryEntry registryEntry;
            registryEntry.m_entityId = m_id;
            registryEntry.m_bounds = GetBounds();
            registryEntry.m_tags = m_tags;

            m_providerHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;

            if (m_providerType == ProviderType::SURFACE_PROVIDER)
            {
                // If the mock provider is generating points, examine the size of the points lists we've added to the mock provider
                // to determine the maximum number of points that we will output from a single input position.
                registryEntry.m_maxPointsCreatedPerInput = 1;
                for (auto& entry : m_surfacePoints)
                {
                    registryEntry.m_maxPointsCreatedPerInput = AZ::GetMax(registryEntry.m_maxPointsCreatedPerInput, entry.second.size());
                }

                m_providerHandle = AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->RegisterSurfaceDataProvider(registryEntry);
                SurfaceData::SurfaceDataProviderRequestBus::Handler::BusConnect(m_providerHandle);
            }
            else
            {
                m_providerHandle = AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->RegisterSurfaceDataModifier(registryEntry);
                SurfaceData::SurfaceDataModifierRequestBus::Handler::BusConnect(m_providerHandle);
            }
        }

        void Unregister()
        {
            if (m_providerType == ProviderType::SURFACE_PROVIDER)
            {
                SurfaceData::SurfaceDataProviderRequestBus::Handler::BusDisconnect();
                AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->UnregisterSurfaceDataProvider(m_providerHandle);
            }
            else
            {
                SurfaceData::SurfaceDataModifierRequestBus::Handler::BusDisconnect();
                AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->UnregisterSurfaceDataModifier(m_providerHandle);
            }

            m_providerHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;
        }


        //////////////////////////////////////////////////////////////////////////
        // SurfaceDataProviderRequestBus
        void GetSurfacePoints(const AZ::Vector3& inPosition, SurfaceData::SurfacePointList& surfacePointList) const override
        {
            auto surfacePoints = m_surfacePoints.find(AZStd::make_pair(inPosition.GetX(), inPosition.GetY()));

            if (surfacePoints != m_surfacePoints.end())
            {
                for (auto& point : surfacePoints->second)
                {
                    SurfaceData::SurfaceTagWeights weights(point.m_surfaceTags);
                    surfacePointList.AddSurfacePoint(m_id, inPosition, point.m_position, point.m_normal, weights);
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // SurfaceDataModifierRequestBus
        void ModifySurfacePoints(
            AZStd::span<const AZ::Vector3> positions,
            [[maybe_unused]] AZStd::span<const AZ::EntityId> creatorEntityIds,
            AZStd::span<SurfaceData::SurfaceTagWeights> weights) const override
        {
            for (size_t index = 0; index < positions.size(); index++)
            {
                auto surfacePoints = m_surfacePoints.find(AZStd::make_pair(positions[index].GetX(), positions[index].GetY()));

                if (surfacePoints != m_surfacePoints.end())
                {
                    weights[index].AddSurfaceTagWeights(m_tags, 1.0f);
                }
            }
        }

        SurfaceData::SurfaceDataRegistryHandle m_providerHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;

};

TEST(SurfaceDataTest, ComponentsWithComponentApplication)
{
    AZ::Entity* testSystemEntity = new AZ::Entity();
    testSystemEntity->CreateComponent<SurfaceData::SurfaceDataSystemComponent>();

    testSystemEntity->Init();
    testSystemEntity->Activate();
    EXPECT_EQ(testSystemEntity->GetState(), AZ::Entity::State::Active);
    testSystemEntity->Deactivate();
    delete testSystemEntity;
}

class SurfaceDataTestApp
    : public ::testing::Test
{
public:
    void SetUp() override
    {
        m_surfaceDataSystemEntity = AZStd::make_unique<AZ::Entity>();
        m_surfaceDataSystemEntity->CreateComponent<SurfaceData::SurfaceDataSystemComponent>();
        m_surfaceDataSystemEntity->Init();
        m_surfaceDataSystemEntity->Activate();
    }

    void TearDown() override
    {
        m_surfaceDataSystemEntity.reset();
    }

    void CompareSurfacePointListWithGetSurfacePoints(
        const AZStd::vector<AZ::Vector3>& queryPositions, SurfaceData::SurfacePointList& surfacePointLists,
        const SurfaceData::SurfaceTagVector& testTags)
    {
        AZStd::vector<AzFramework::SurfaceData::SurfacePoint> singleQueryResults;
        SurfaceData::SurfacePointList tempSingleQueryPointList;

        for (size_t inputIndex = 0; inputIndex < queryPositions.size(); inputIndex++)
        {
            tempSingleQueryPointList.Clear();
            singleQueryResults.clear();

            AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->GetSurfacePoints(
                queryPositions[inputIndex], testTags, tempSingleQueryPointList);
            tempSingleQueryPointList.EnumeratePoints([&singleQueryResults](
                    [[maybe_unused]] size_t inPositionIndex, const AZ::Vector3& position,
                    const AZ::Vector3& normal, const SurfaceData::SurfaceTagWeights& masks) -> bool
                {
                    AzFramework::SurfaceData::SurfacePoint point;
                    point.m_position = position;
                    point.m_normal = normal;
                    point.m_surfaceTags = masks.GetSurfaceTagWeightList();
                    singleQueryResults.emplace_back(AZStd::move(point));
                    return true;
                });

            size_t resultIndex = 0;
            surfacePointLists.EnumeratePoints(
                inputIndex,
                [&resultIndex, singleQueryResults](
                    const AZ::Vector3& position, const AZ::Vector3& normal, const SurfaceData::SurfaceTagWeights& masks) -> bool
                {
                    EXPECT_NE(resultIndex, singleQueryResults.size());

                    EXPECT_EQ(position, singleQueryResults[resultIndex].m_position);
                    EXPECT_EQ(normal, singleQueryResults[resultIndex].m_normal);
                    EXPECT_TRUE(masks.SurfaceWeightsAreEqual(singleQueryResults[resultIndex].m_surfaceTags));
                    ++resultIndex;
                    return true;
                });
            EXPECT_EQ(resultIndex, singleQueryResults.size());
        }
    }

    // Build a buffer asset that contains the given data. This buffer asset is used in construction of an in-memory
    // test Atom model asset that can be used for testing SurfaceData raycasts.
    AZ::Data::Asset<AZ::RPI::BufferAsset> BuildTestBuffer(const void* data, const uint32_t elementCount, const uint32_t elementSize)
    {
        // Create a buffer pool asset for use with the buffer asset
        AZ::Data::Asset<AZ::RPI::ResourcePoolAsset> bufferPoolAsset;
        {
            auto bufferPoolDesc = AZStd::make_unique<AZ::RHI::BufferPoolDescriptor>();
            bufferPoolDesc->m_bindFlags = AZ::RHI::BufferBindFlags::InputAssembly;
            bufferPoolDesc->m_heapMemoryLevel = AZ::RHI::HeapMemoryLevel::Host;

            AZ::RPI::ResourcePoolAssetCreator creator;
            creator.Begin(AZ::Uuid::CreateRandom());
            creator.SetPoolDescriptor(AZStd::move(bufferPoolDesc));
            creator.SetPoolName("TestPool");
            creator.End(bufferPoolAsset);
        }

        // Create a buffer asset that contains a copy of the input data.
        AZ::Data::Asset<AZ::RPI::BufferAsset> asset;
        {
            AZ::RHI::BufferDescriptor bufferDescriptor;
            bufferDescriptor.m_bindFlags = AZ::RHI::BufferBindFlags::InputAssembly;
            bufferDescriptor.m_byteCount = elementCount * elementSize;

            AZ::RPI::BufferAssetCreator creator;
            creator.Begin(AZ::Uuid::CreateRandom());
            creator.SetPoolAsset(bufferPoolAsset);
            creator.SetBuffer(data, bufferDescriptor.m_byteCount, bufferDescriptor);
            creator.SetBufferViewDescriptor(AZ::RHI::BufferViewDescriptor::CreateStructured(0, elementCount, elementSize));
            creator.End(asset);
        }

        return asset;
    }

    // Build an in-memory test Atom model asset out of the given positions and indices.
    AZ::Data::Asset<AZ::RPI::ModelAsset> BuildTestModel(const AZStd::vector<float>& positions, const AZStd::vector<uint32_t>& indices)
    {
        // First build a model LOD asset that contains a mesh for the given positions and indices.
        AZ::Data::Asset<AZ::RPI::ModelLodAsset> lodAsset;
        {
            AZ::RPI::ModelLodAssetCreator creator;
            creator.Begin(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));

            const uint32_t positionElementCount = aznumeric_cast<uint32_t>(positions.size() / 3);
            constexpr uint32_t PositionElementSize = aznumeric_cast<uint32_t>(sizeof(float) * 3);
            const uint32_t indexElementCount = aznumeric_cast<uint32_t>(indices.size());
            constexpr uint32_t IndexElementSize = aznumeric_cast<uint32_t>(sizeof(uint32_t));

            // Calculate the Aabb for the given positions.
            AZ::Aabb aabb;
            for (uint32_t i = 0; i < positions.size(); i += 3)
            {
                aabb.AddPoint(AZ::Vector3(positions[i], positions[i + 1], positions[i + 2]));
            }

            // Set up a single-mesh asset with only position data.
            creator.BeginMesh();
            creator.SetMeshAabb(AZStd::move(aabb));
            creator.SetMeshMaterialSlot(0);
            creator.SetMeshIndexBuffer({ BuildTestBuffer(indices.data(), indexElementCount, IndexElementSize),
                                         AZ::RHI::BufferViewDescriptor::CreateStructured(0, indexElementCount, IndexElementSize)
                });
            creator.AddMeshStreamBuffer(
                AZ::RHI::ShaderSemantic(AZ::Name("POSITION")), AZ::Name(),
                { BuildTestBuffer(positions.data(), positionElementCount, PositionElementSize),
                  AZ::RHI::BufferViewDescriptor::CreateStructured(0, positionElementCount, PositionElementSize) });
            creator.EndMesh();
            creator.End(lodAsset);
        }

        // Create a model asset that contains the single LOD built above.
        AZ::Data::Asset<AZ::RPI::ModelAsset> asset;
        {
            AZ::RPI::ModelAssetCreator creator;

            creator.Begin(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
            creator.SetName("TestModel");
            creator.AddLodAsset(AZStd::move(lodAsset));
            creator.End(asset);
        }

        return asset;
    }

    // Test Surface Data tags that we can use for testing query functionality
    const AZ::Crc32 m_testSurface1Crc = AZ::Crc32("test_surface1");
    const AZ::Crc32 m_testSurface2Crc = AZ::Crc32("test_surface2");
    const AZ::Crc32 m_testSurfaceNoMatchCrc = AZ::Crc32("test_surface_no_match");

    AZStd::unique_ptr<AZ::Entity> m_surfaceDataSystemEntity;
};

TEST_F(SurfaceDataTestApp, SurfaceData_TestRegisteredTags)
{
    // Check that only the unassigned tag exists if no other providers are registered.
    AZStd::vector<AZStd::pair<AZ::u32, AZStd::string>> registeredTags = SurfaceData::SurfaceTag::GetRegisteredTags();

    const auto& searchTerm = SurfaceData::Constants::s_unassignedTagName;

    ASSERT_TRUE(AZStd::find_if(
        registeredTags.begin(), registeredTags.end(),
        [=](decltype(registeredTags)::value_type pair)
        {
            return pair.second == searchTerm;
        }));
}

#if AZ_TRAIT_DISABLE_FAILED_SURFACE_DATA_TESTS
TEST_F(SurfaceDataTestApp, DISABLED_SurfaceData_TestGetQuadListRayIntersection)
#else
TEST_F(SurfaceDataTestApp, SurfaceData_TestGetQuadListRayIntersection)
#endif // AZ_TRAIT_DISABLE_FAILED_SURFACE_DATA_TESTS
{
    AZStd::vector<AZ::Vector3> quads;
    AZ::Vector3 outPosition;
    AZ::Vector3 outNormal;
    bool result;

    struct RayTest
    {
        // Input quad
        AZ::Vector3 quadVertices[4];
        // Input ray
        AZ::Vector3 rayOrigin;
        AZ::Vector3 rayDirection;
        float       rayMaxRange;
        // Expected outputs
        bool        expectedResult;
        AZ::Vector3 expectedOutPosition;
        AZ::Vector3 expectedOutNormal;
    };

    RayTest tests[] = 
    {
        // Ray intersects quad
        {{AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(100.0f, 0.0f, 0.0f), AZ::Vector3(0.0f, 100.0f, 0.0f), AZ::Vector3(100.0f, 100.0f, 0.0f)},
            AZ::Vector3(50.0f, 50.0f, 10.0f), AZ::Vector3(0.0f, 0.0f, -1.0f), 20.0f,  true, AZ::Vector3(50.0f, 50.0f, 0.0f), AZ::Vector3(0.0f, 0.0f, 1.0f)},

        // Ray not long enough to intersect
        {{AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(100.0f, 0.0f, 0.0f), AZ::Vector3(0.0f, 100.0f, 0.0f), AZ::Vector3(100.0f, 100.0f, 0.0f)},
            AZ::Vector3(50.0f, 50.0f, 10.0f), AZ::Vector3(0.0f, 0.0f, -1.0f),  5.0f, false, AZ::Vector3( 0.0f,  0.0f, 0.0f), AZ::Vector3(0.0f, 0.0f, 0.0f)},

        // 0-length ray on quad surface
        {{AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(100.0f, 0.0f, 0.0f), AZ::Vector3(0.0f, 100.0f, 0.0f), AZ::Vector3(100.0f, 100.0f, 0.0f)},
            AZ::Vector3(50.0f, 50.0f,  0.0f), AZ::Vector3(0.0f, 0.0f, -1.0f),  0.0f,  true, AZ::Vector3(50.0f, 50.0f, 0.0f), AZ::Vector3(0.0f, 0.0f, 1.0f)},
        
        // ray in wrong direction
        {{AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(100.0f, 0.0f, 0.0f), AZ::Vector3(0.0f, 100.0f, 0.0f), AZ::Vector3(100.0f, 100.0f, 0.0f)},
            AZ::Vector3(50.0f, 50.0f, 10.0f), AZ::Vector3(0.0f, 0.0f,  1.0f), 20.0f, false, AZ::Vector3( 0.0f,  0.0f, 0.0f), AZ::Vector3(0.0f, 0.0f, 0.0f)},

        // The following tests are specific cases that worked differently when the implementation of GetQuadRayListIntersection used AZ::Intersect::IntersectRayQuad
        // instead of IntersectSegmentTriangle.  We'll keep them here both as good non-trivial tests and to ensure that if anyone ever tries to change the implmentation,
        // they can easily validate whether or not IntersectRayQuad will produce the same results.

        // ray passes IntersectSegmentTriangle but fails IntersectRayQuad
        {{AZ::Vector3(499.553, 688.946, 48.788), AZ::Vector3(483.758, 698.655, 48.788), AZ::Vector3(498.463, 687.181, 48.916), AZ::Vector3(482.701, 696.942, 48.916)},
            AZ::Vector3(485.600, 695.200, 49.501), AZ::Vector3(-0.000f, -0.000f, -1.000f), 18.494f, true, AZ::Vector3(485.600, 695.200, 48.913), AZ::Vector3(0.033, 0.053, 0.998)},

        // ray fails IntersectSegmentTriangle but passes IntersectRayQuad
        // IntersectRayQuad hits with the following position / normal:  AZ::Vector3(480.000, 688.800, 49.295), AZ::Vector3(0.020, 0.032, 0.999)
        {{AZ::Vector3(495.245, 681.984, 49.218), AZ::Vector3(479.450, 691.692, 49.218), AZ::Vector3(494.205, 680.282, 49.292), AZ::Vector3(478.356, 689.902, 49.292)},
            AZ::Vector3(480.000, 688.800, 49.501), AZ::Vector3(-0.000, -0.000, -1.000), 18.494f, false, AZ::Vector3(0.0f,  0.0f, 0.0f), AZ::Vector3(0.0f, 0.0f, 0.0f)},

        // ray passes IntersectSegmentTriangle and IntersectRayQuad, but hits at different positions
        // IntersectRayQuad hits with the following position / normal:  AZ::Vector3(498.400, 700.000, 48.073), AZ::Vector3(0.046, 0.085, 0.995)
        {{AZ::Vector3(504.909, 698.078, 47.913), AZ::Vector3(488.641, 706.971, 47.913), AZ::Vector3(503.867, 696.206, 48.121), AZ::Vector3(487.733, 705.341, 48.121)},
            AZ::Vector3(498.400, 700.000, 49.501), AZ::Vector3(-0.000f, -0.000f, -1.000f), 53.584f, true, AZ::Vector3(498.400, 700.000, 48.062), AZ::Vector3(0.048, 0.084, 0.995)},

        // ray passes IntersectSegmentTriangle and IntersectRayQuad, but hits at different normals
        // IntersectRayQuad hits with the following position / normal:  AZ::Vector3(492.800, 703.200, 48.059), AZ::Vector3(0.046, 0.085, 0.995) 
        {{AZ::Vector3(504.909, 698.078, 47.913), AZ::Vector3(488.641, 706.971, 47.913), AZ::Vector3(503.867, 696.206, 48.121), AZ::Vector3(487.733, 705.341, 48.121)},
            AZ::Vector3(492.800, 703.200, 49.501), AZ::Vector3(-0.000f, -0.000f, -1.000f), 18.494f, true, AZ::Vector3(492.800, 703.200, 48.059), AZ::Vector3(0.053, 0.097, 0.994)},

    };

    for (const auto &test : tests)
    {
        quads.clear();
        outPosition.Set(0.0f, 0.0f, 0.0f);
        outNormal.Set(0.0f, 0.0f, 0.0f);

        quads.push_back(test.quadVertices[0]);
        quads.push_back(test.quadVertices[1]);
        quads.push_back(test.quadVertices[2]);
        quads.push_back(test.quadVertices[3]);

        result = SurfaceData::GetQuadListRayIntersection(quads, test.rayOrigin, test.rayDirection, test.rayMaxRange, outPosition, outNormal);
        ASSERT_TRUE(result == test.expectedResult);
        if (result || test.expectedResult)
        {
            ASSERT_TRUE(outPosition.IsClose(test.expectedOutPosition));
            ASSERT_TRUE(outNormal.IsClose(test.expectedOutNormal));
        }
    }
}

TEST_F(SurfaceDataTestApp, SurfaceData_TestAabbOverlaps2D)
{
    // Test to make sure the utility method "AabbOverlaps2D" functions as expected.

    struct TestCase
    {
        enum TestIndex
        {
            SOURCE_MIN,
            SOURCE_MAX,
            DEST_MIN,
            DEST_MAX,

            NUM_PARAMS
        };

        AZ::Vector3 m_testData[NUM_PARAMS];
        bool m_overlaps;
    };

    TestCase testCases[]
    {
        // Overlap=TRUE  Boxes fully overlap in 3D space
        {{AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(2.0f, 2.0f, 2.0f), AZ::Vector3(1.0f, 1.0f, 1.0f), AZ::Vector3(3.0f, 3.0f, 3.0f)}, true },
        // Overlap=TRUE  Boxes overlap in 2D space, but not 3D
        {{AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(2.0f, 2.0f, 2.0f), AZ::Vector3(1.0f, 1.0f, 4.0f), AZ::Vector3(3.0f, 3.0f, 6.0f)}, true},
        // Overlap=TRUE  Boxes are equal
        {{AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(2.0f, 2.0f, 2.0f), AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(2.0f, 2.0f, 2.0f)}, true},
        // Overlap=TRUE  Box contains other box
        {{AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(2.0f, 2.0f, 2.0f), AZ::Vector3(1.0f, 1.0f, 1.0f), AZ::Vector3(1.5f, 1.5f, 1.5f)}, true },

        // Overlap=FALSE Boxes only overlap in X and Z, not Y
        {{AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(2.0f, 2.0f, 2.0f), AZ::Vector3(1.0f, 4.0f, 1.0f), AZ::Vector3(3.0f, 6.0f, 3.0f)}, false},
        // Overlap=FALSE Boxes only overlap in Y and Z, not X
        {{AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(2.0f, 2.0f, 2.0f), AZ::Vector3(4.0f, 1.0f, 1.0f), AZ::Vector3(6.0f, 3.0f, 3.0f)}, false },
    };

    for (auto& testCase : testCases)
    {
        AZ::Aabb box1 = AZ::Aabb::CreateFromMinMax(testCase.m_testData[TestCase::SOURCE_MIN], testCase.m_testData[TestCase::SOURCE_MAX]);
        AZ::Aabb box2 = AZ::Aabb::CreateFromMinMax(testCase.m_testData[TestCase::DEST_MIN],   testCase.m_testData[TestCase::DEST_MAX]);

        // Make sure the test produces the correct result.
        // Also make sure it's correct regardless of which order the boxes are passed in.
        EXPECT_EQ(SurfaceData::AabbOverlaps2D(box1, box2), testCase.m_overlaps);
        EXPECT_EQ(SurfaceData::AabbOverlaps2D(box2, box1), testCase.m_overlaps);
    }
}

TEST_F(SurfaceDataTestApp, SurfaceData_TestAabbContains2D)
{
    // Test to make sure the utility method "AabbContains2D" functions as expected.

    struct TestCase
    {
        enum TestIndex
        {
            BOX_MIN,
            BOX_MAX,
            POINT,

            NUM_PARAMS
        };

        AZ::Vector3 m_testData[NUM_PARAMS];
        bool m_contains;
    };

    TestCase testCases[]
    {
        // Contains=TRUE  Box and point fully overlap in 3D space
        {{AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(2.0f, 2.0f, 2.0f), AZ::Vector3(1.0f, 1.0f, 1.0f)}, true},
        // Contains=TRUE  Box and point overlap in 2D space, but not 3D
        {{AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(2.0f, 2.0f, 2.0f), AZ::Vector3(1.0f, 1.0f, 4.0f)}, true},
        // Contains=TRUE  Point on box min corner
        {{AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(2.0f, 2.0f, 2.0f), AZ::Vector3(0.0f, 0.0f, 0.0f)}, true },
        // Contains=TRUE  Point on box max corner
        {{AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(2.0f, 2.0f, 2.0f), AZ::Vector3(2.0f, 2.0f, 2.0f)}, true},

        // Contains=FALSE Box and point only overlap in X and Z, not Y
        {{ AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(2.0f, 2.0f, 2.0f), AZ::Vector3(1.0f, 4.0f, 1.0f) }, false},
        // Contains=FALSE Box and point only overlap in Y and Z, not X
        {{ AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(2.0f, 2.0f, 2.0f), AZ::Vector3(4.0f, 1.0f, 1.0f) }, false},
        // Contains=FALSE Box and point don't overlap at all
        {{ AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(2.0f, 2.0f, 2.0f), AZ::Vector3(4.0f, 4.0f, 4.0f) }, false},
    };

    for (auto& testCase : testCases)
    {
        AZ::Aabb box = AZ::Aabb::CreateFromMinMax(testCase.m_testData[TestCase::BOX_MIN], testCase.m_testData[TestCase::BOX_MAX]);
        AZ::Vector3& point = testCase.m_testData[TestCase::POINT];

        // Make sure the test produces the correct result.
        EXPECT_EQ(SurfaceData::AabbContains2D(box, point), testCase.m_contains);
        // Test the Vector2 version as well.
        EXPECT_EQ(SurfaceData::AabbContains2D(box, AZ::Vector2(point.GetX(), point.GetY())), testCase.m_contains);
    }
}

TEST_F(SurfaceDataTestApp, SurfaceData_TestGetMeshRayIntersection)
{
    AZ::Vector3 outPosition;
    AZ::Vector3 outNormal;
    bool result;

    struct RayTest
    {
        // Input ray
        AZ::Vector3 rayStart;
        AZ::Vector3 rayEnd;
        // Expected outputs
        bool expectedResult;
        AZ::Vector3 expectedOutPosition;
        AZ::Vector3 expectedOutNormal;
    };

    RayTest tests[] = {
        // Tiny ray intersects mesh
        { 
          AZ::Vector3(2.0f, 2.0f, 5.01f),
          AZ::Vector3(2.0f, 2.0f, 4.99f),
          true,
          AZ::Vector3(2.0f, 2.0f, 5.0f),
          AZ::Vector3(0.0f, 0.0f, 1.0f)
        },

        // Ray intersects mesh
        { 
          AZ::Vector3(2.0f, 2.0f, 10.0f),
          AZ::Vector3(2.0f, 2.0f, -10.0f),
          true,
          AZ::Vector3(2.0f, 2.0f, 5.0f),
          AZ::Vector3(0.0f, 0.0f, 1.0f)
        },

        // Ray intersects mesh on min corner
        { 
          AZ::Vector3(0.0f, 0.0f, 10.0f),
          AZ::Vector3(0.0f, 0.0f, -10.0f),
          true,
          AZ::Vector3(0.0f, 0.0f, 5.0f),
          AZ::Vector3(0.0f, 0.0f, 1.0f)
        },

        // Ray intersects mesh on max corner
        { 
          AZ::Vector3(5.0f, 5.0f, 10.0f),
          AZ::Vector3(5.0f, 5.0f, -10.0f),
          true,
          AZ::Vector3(5.0f, 5.0f, 5.0f),
          AZ::Vector3(0.0f, 0.0f, 1.0f)
        },

        // Ray misses mesh
        { 
          AZ::Vector3(10.0f, 0.0f, 10.0f),
          AZ::Vector3(10.0f, 0.0f, -10.0f),
          false,
          AZ::Vector3(0.0f, 0.0f, 0.0f),
          AZ::Vector3(0.0f, 0.0f, 1.0f)
        },
    };

    // Register all the asset handlers necessary for constructing the test model.
    auto resourcePoolAssetHandler = AZ::RPI::MakeAssetHandler<AZ::RPI::ResourcePoolAssetHandler>();
    auto bufferAssetHandler = AZ::RPI::MakeAssetHandler<AZ::RPI::BufferAssetHandler>();
    auto modelLodAssetHandler = AZ::RPI::MakeAssetHandler<AZ::RPI::ModelLodAssetHandler>();
    auto modelAssetHandler = AZ::RPI::MakeAssetHandler<AZ::RPI::ModelAssetHandler>();

    // Build a mesh containing a test quad. The test quad goes from 0-5 in the XY plane, at a height of 5 on the Z axis.
    const AZStd::vector<uint32_t> indices = { 0, 1, 2, 1, 3, 2 };
    const AZStd::vector<float> positions = { 0.0f, 0.0f, 5.0f, 5.0f, 0.0f, 5.0f, 5.0f, 5.0f, 5.0f, 0.0f, 5.0f, 5.0f };
    const AZ::Transform meshTransform = AZ::Transform::CreateTranslation(AZ::Vector3::CreateZero());
    const AZ::Transform meshTransformInverse = meshTransform.GetInverse();
    const AZ::Vector3 nonUniformScale(1.0f);

    auto modelAsset = BuildTestModel(positions, indices);

    for (const auto& test : tests)
    {
        outPosition.Set(0.0f, 0.0f, 0.0f);
        outNormal.Set(0.0f, 0.0f, 0.0f);

        result = SurfaceData::GetMeshRayIntersection(
            *(modelAsset.Get()), meshTransform, meshTransformInverse,
            nonUniformScale, test.rayStart, test.rayEnd, outPosition, outNormal);

        EXPECT_EQ(result, test.expectedResult);
        if (result || test.expectedResult)
        {
            EXPECT_TRUE(outPosition.IsClose(test.expectedOutPosition));
            EXPECT_TRUE(outNormal.IsClose(test.expectedOutNormal));
        }
    }
}

TEST_F(SurfaceDataTestApp, SurfaceData_TestSurfacePointsFromRegion)
{
    // This tests the basic functionality of GetSurfacePointsFromRegion:
    // - The surface points are queried by stepping through an AABB, which is inclusive on one side, and exclusive on the other.
    //   i.e. (0,0) - (4,4) will include (0,0), but exclude (4,4)
    // - The Z range of the input region is ignored when querying for points.  (This is consistent with GetSurfacePoints)
    // - The output has one list entry per surface point queried
    // - The output has the correct expected points and masks

    // Create a mock Surface Provider that covers from (0, 0) - (8, 8) in space.
    // It defines points spaced 0.25 apart, with heights of 0 and 4, and with the tags "test_surface1" and "test_surface2".
    // (We're creating points spaced more densely than we'll query just to verify that we only get back the queried points)
    SurfaceData::SurfaceTagVector providerTags = { SurfaceData::SurfaceTag(m_testSurface1Crc), SurfaceData::SurfaceTag(m_testSurface2Crc) };
    MockSurfaceProvider mockProvider(MockSurfaceProvider::ProviderType::SURFACE_PROVIDER, providerTags,
                                     AZ::Vector3(0.0f), AZ::Vector3(8.0f), AZ::Vector3(0.25f, 0.25f, 4.0f));

    // Query for all the surface points from (0, 0, 16) - (4, 4, 16) with a step size of 1.
    // Note that the Z range is deliberately chosen to be outside the surface provider range to demonstrate
    // that it is ignored when selecting points.
    SurfaceData::SurfacePointList availablePointsPerPosition;
    AZ::Vector2 stepSize(1.0f, 1.0f);
    AZ::Aabb regionBounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.0f, 0.0f, 16.0f), AZ::Vector3(4.0f, 4.0f, 16.0f));
    SurfaceData::SurfaceTagVector testTags = providerTags;

    AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->GetSurfacePointsFromRegion(
        regionBounds, stepSize, testTags, availablePointsPerPosition);

    // We expect every entry in the output list to have two surface points, at heights 0 and 4, sorted in
    // decreasing height order.  The masks list should be the same size as the set of masks the provider owns.
    // We *could* check every mask as well for completeness, but that seems like overkill.
    float expectedZ = 4.0f;
    availablePointsPerPosition.EnumeratePoints(
        [availablePointsPerPosition, providerTags, &expectedZ](size_t inPositionIndex, const AZ::Vector3& position,
            [[maybe_unused]] const AZ::Vector3& normal, const SurfaceData::SurfaceTagWeights& masks) -> bool
        {
            EXPECT_EQ(availablePointsPerPosition.GetSize(inPositionIndex), 2);
            EXPECT_EQ(position.GetZ(), expectedZ);
            EXPECT_EQ(masks.GetSize(), providerTags.size());
            expectedZ = (expectedZ == 4.0f) ? 0.0f : 4.0f;
            return true;
        });
}

TEST_F(SurfaceDataTestApp, SurfaceData_TestSurfacePointsFromRegion_NoMatchingMasks)
{
    // This test verifies that if we query surfaces with a non-matching mask, the points will get filtered out.

    // Create a mock Surface Provider that covers from (0, 0) - (8, 8) in space.
    // It defines points spaced 0.25 apart, with heights of 0 and 4, and with the tags "test_surface1" and "test_surface2".
    SurfaceData::SurfaceTagVector providerTags = { SurfaceData::SurfaceTag(m_testSurface1Crc), SurfaceData::SurfaceTag(m_testSurface2Crc) };
    MockSurfaceProvider mockProvider(MockSurfaceProvider::ProviderType::SURFACE_PROVIDER, providerTags,
                                     AZ::Vector3(0.0f), AZ::Vector3(8.0f), AZ::Vector3(0.25f, 0.25f, 4.0f));

    // Query for all the surface points from (0, 0, 0) - (4, 4, 4) with a step size of 1.
    // We only include a surface tag that does NOT exist in the surface provider.
    SurfaceData::SurfacePointList availablePointsPerPosition;
    AZ::Vector2 stepSize(1.0f, 1.0f);
    AZ::Aabb regionBounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.0f), AZ::Vector3(4.0f));
    SurfaceData::SurfaceTagVector testTags = { SurfaceData::SurfaceTag(m_testSurfaceNoMatchCrc) };

    AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->GetSurfacePointsFromRegion(
        regionBounds, stepSize, testTags, availablePointsPerPosition);

    // We expect every entry in the output list to have no surface points, since the requested mask doesn't match
    // any of the masks from our mock surface provider.
    EXPECT_TRUE(availablePointsPerPosition.IsEmpty());
}

TEST_F(SurfaceDataTestApp, SurfaceData_TestSurfacePointsFromRegion_NoMatchingRegion)
{
    // This test verifies that if we query surfaces with a non-overlapping region, no points are returned.

    // Create a mock Surface Provider that covers from (0,0) - (8, 8) in space.
    // It defines points spaced 0.25 apart, with heights of 0 and 4, and with the tags "test_surface1" and "test_surface2".
    SurfaceData::SurfaceTagVector providerTags = { SurfaceData::SurfaceTag(m_testSurface1Crc), SurfaceData::SurfaceTag(m_testSurface2Crc) };
    MockSurfaceProvider mockProvider(MockSurfaceProvider::ProviderType::SURFACE_PROVIDER, providerTags,
                                     AZ::Vector3(0.0f), AZ::Vector3(8.0f), AZ::Vector3(0.25f, 0.25f, 4.0f));

    // Query for all the surface points from (16, 16) - (20, 20) with a step size of 1.
    SurfaceData::SurfacePointList availablePointsPerPosition;
    AZ::Vector2 stepSize(1.0f, 1.0f);
    AZ::Aabb regionBounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(16.0f), AZ::Vector3(20.0f));
    SurfaceData::SurfaceTagVector testTags = providerTags;

    AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->GetSurfacePointsFromRegion(
        regionBounds, stepSize, testTags, availablePointsPerPosition);

    // We expect every entry in the output list to have no surface points, since the input points don't overlap with
    // our surface provider.
    EXPECT_TRUE(availablePointsPerPosition.IsEmpty());
}

TEST_F(SurfaceDataTestApp, SurfaceData_TestSurfacePointsFromRegion_ProviderModifierMasksCombine)
{
    // This test verifies that SurfaceDataModifiers can successfully modify the tags on each point.
    // It also verifies that points won't be dropped from the results as long as either the provider
    // or the modifier add the correct tag to the point.

    // Create a mock Surface Provider that covers from (0,0) - (8, 8) in space.
    // It defines points spaced 1 apart, with heights of 0 and 4, and with the tag "test_surface1".
    SurfaceData::SurfaceTagVector providerTags = { SurfaceData::SurfaceTag(m_testSurface1Crc) };
    MockSurfaceProvider mockProvider(MockSurfaceProvider::ProviderType::SURFACE_PROVIDER, providerTags,
                                     AZ::Vector3(0.0f), AZ::Vector3(8.0f), AZ::Vector3(1.0f, 1.0f, 4.0f));

    // Create a mock Surface Modifier that covers from (0,0) - (8, 8) in space.
    // It will modify points spaced 1 apart, with heights of 0 and 4, and add the tag "test_surface2".
    SurfaceData::SurfaceTagVector modifierTags = { SurfaceData::SurfaceTag(m_testSurface2Crc) };
    MockSurfaceProvider mockModifier(MockSurfaceProvider::ProviderType::SURFACE_MODIFIER, modifierTags,
                                     AZ::Vector3(0.0f), AZ::Vector3(8.0f), AZ::Vector3(1.0f, 1.0f, 4.0f));


    // Query for all the surface points from (0, 0) - (4, 4) with a step size of 1.
    // We perform this test 3 times - once with just the provider tag, once with just the modifier tag,
    // and once with both.  We expect identical results on each test, since each point should get both
    // the provider and the modifier tag.

    SurfaceData::SurfaceTagVector tagTests[] =
    {
        { SurfaceData::SurfaceTag(m_testSurface1Crc) },
        { SurfaceData::SurfaceTag(m_testSurface2Crc) },
        { SurfaceData::SurfaceTag(m_testSurface1Crc), SurfaceData::SurfaceTag(m_testSurface2Crc) },
    };

    for (auto& tagTest : tagTests)
    {
        SurfaceData::SurfacePointList availablePointsPerPosition;
        AZ::Vector2 stepSize(1.0f, 1.0f);
        AZ::Aabb regionBounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.0f), AZ::Vector3(4.0f));
        SurfaceData::SurfaceTagVector testTags = tagTest;

        AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->GetSurfacePointsFromRegion(
            regionBounds, stepSize, testTags, availablePointsPerPosition);

        // We expect every entry in the output list to have two surface points (with heights 0 and 4),
        // and each point should have both the "test_surface1" and "test_surface2" tag.
        float expectedZ = 4.0f;
        availablePointsPerPosition.EnumeratePoints(
            [availablePointsPerPosition, &expectedZ](size_t inPositionIndex, const AZ::Vector3& position,
                [[maybe_unused]] const AZ::Vector3& normal, const SurfaceData::SurfaceTagWeights& masks) -> bool
            {
                EXPECT_EQ(availablePointsPerPosition.GetSize(inPositionIndex), 2);
                EXPECT_EQ(position.GetZ(), expectedZ);
                EXPECT_EQ(masks.GetSize(), 2);
                expectedZ = (expectedZ == 4.0f) ? 0.0f : 4.0f;
                return true;
            });
    }
}

TEST_F(SurfaceDataTestApp, SurfaceData_TestSurfacePointsFromRegion_SimilarPointsMergeTogether)
{
    // This test verifies that if two separate providers create points at very similar heights, the
    // points will get merged together in the results, with the resulting point ending up with both
    // sets of tags.

    // Create two mock Surface Providers that covers from (0, 0) - (8, 8) in space, with points spaced 0.25 apart.
    // The first has heights 0 and 4, with the tag "surfaceTag1".  The second has heights 0.0005 and 4.0005, with the tag "surfaceTag2".
    SurfaceData::SurfaceTagVector provider1Tags = { SurfaceData::SurfaceTag(m_testSurface1Crc) };
    MockSurfaceProvider mockProvider1(MockSurfaceProvider::ProviderType::SURFACE_PROVIDER, provider1Tags,
                                      AZ::Vector3(0.0f), AZ::Vector3(8.0f), AZ::Vector3(0.25f, 0.25f, 4.0f),
                                      AZ::EntityId(0x11111111));

    SurfaceData::SurfaceTagVector provider2Tags = { SurfaceData::SurfaceTag(m_testSurface2Crc) };
    MockSurfaceProvider mockProvider2(MockSurfaceProvider::ProviderType::SURFACE_PROVIDER, provider2Tags,
                                      AZ::Vector3(0.0f, 0.0f, 0.0f + (AZ::Constants::Tolerance / 2.0f)),
                                      AZ::Vector3(8.0f, 8.0f, 8.0f + (AZ::Constants::Tolerance / 2.0f)),
                                      AZ::Vector3(0.25f, 0.25f, 4.0f),
                                      AZ::EntityId(0x22222222));


    // Query for all the surface points from (0, 0) - (4, 4) with a step size of 1.
    SurfaceData::SurfacePointList availablePointsPerPosition;
    AZ::Vector2 stepSize(1.0f, 1.0f);
    AZ::Aabb regionBounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.0f), AZ::Vector3(4.0f));
    SurfaceData::SurfaceTagVector testTags = { SurfaceData::SurfaceTag(m_testSurface1Crc), SurfaceData::SurfaceTag(m_testSurface2Crc) };

    AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->GetSurfacePointsFromRegion(
        regionBounds, stepSize, testTags, availablePointsPerPosition);

    // We expect every entry in the output list to have two surface points, not four.  The two points
    // should have both surface tags on them.
    float expectedZ = 4.0f;
    availablePointsPerPosition.EnumeratePoints(
        [availablePointsPerPosition, &expectedZ](
            size_t inPositionIndex, const AZ::Vector3& position, [[maybe_unused]] const AZ::Vector3& normal,
            const SurfaceData::SurfaceTagWeights& masks) -> bool
        {
            EXPECT_EQ(availablePointsPerPosition.GetSize(inPositionIndex), 2);

            // Similar points get merged, but there's no guarantee which value will be kept, so we set our comparison tolerance
            // high enough to allow both x.0 and x.0005 to pass.
            EXPECT_NEAR(position.GetZ(), expectedZ, 0.001f);
            EXPECT_EQ(masks.GetSize(), 2);
            expectedZ = (expectedZ == 4.0f) ? 0.0f : 4.0f;
            return true;
        });
}

TEST_F(SurfaceDataTestApp, SurfaceData_TestSurfacePointsFromRegion_DissimilarPointsDoNotMergeTogether)
{
    // This test verifies that if two separate providers create points at dissimilar heights, the
    // points will NOT get merged together in the results.

    // Create two mock Surface Providers that covers from (0, 0) - (8, 8) in space, with points spaced 0.25 apart.
    // The first has heights 0 and 4, with the tag "surfaceTag1".  The second has heights 0.02 and 4.02, with the tag "surfaceTag2".
    SurfaceData::SurfaceTagVector provider1Tags = { SurfaceData::SurfaceTag(m_testSurface1Crc) };
    MockSurfaceProvider mockProvider1(MockSurfaceProvider::ProviderType::SURFACE_PROVIDER, provider1Tags,
                                      AZ::Vector3(0.0f), AZ::Vector3(8.0f), AZ::Vector3(0.25f, 0.25f, 4.0f),
                                      AZ::EntityId(0x11111111));

    SurfaceData::SurfaceTagVector provider2Tags = { SurfaceData::SurfaceTag(m_testSurface2Crc) };
    MockSurfaceProvider mockProvider2(MockSurfaceProvider::ProviderType::SURFACE_PROVIDER, provider2Tags,
                                      AZ::Vector3(0.0f, 0.0f, 0.0f + (AZ::Constants::Tolerance * 2.0f)),
                                      AZ::Vector3(8.0f, 8.0f, 8.0f + (AZ::Constants::Tolerance * 2.0f)),
                                      AZ::Vector3(0.25f, 0.25f, 4.0f),
                                      AZ::EntityId(0x22222222));


    // Query for all the surface points from (0, 0) - (4, 4) with a step size of 1.
    SurfaceData::SurfacePointList availablePointsPerPosition;
    AZ::Vector2 stepSize(1.0f, 1.0f);
    AZ::Aabb regionBounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.0f), AZ::Vector3(4.0f));
    SurfaceData::SurfaceTagVector testTags = { SurfaceData::SurfaceTag(m_testSurface1Crc), SurfaceData::SurfaceTag(m_testSurface2Crc) };

    AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->GetSurfacePointsFromRegion(
        regionBounds, stepSize, testTags, availablePointsPerPosition);

    // We expect every entry in the output list to have four surface points with one tag each,
    // because the points are far enough apart that they won't merge.
    availablePointsPerPosition.EnumeratePoints(
        [availablePointsPerPosition](size_t inPositionIndex, [[maybe_unused]] const AZ::Vector3& position,
            [[maybe_unused]] const AZ::Vector3& normal, const SurfaceData::SurfaceTagWeights& masks) -> bool
        {
            EXPECT_EQ(availablePointsPerPosition.GetSize(inPositionIndex), 4);
            EXPECT_EQ(masks.GetSize(), 1);
            return true;
        });
}

TEST_F(SurfaceDataTestApp, SurfaceData_VerifyGetSurfacePointsFromRegionAndGetSurfacePointsMatch)
{
    // This ensures that both GetSurfacePointsFromRegion and GetSurfacePoints produce the same results.

    // Create a mock Surface Provider that covers from (0, 0) - (8, 8) in space.
    // It defines points spaced 0.25 apart, with heights of 0 and 4, and with the tags "test_surface1" and "test_surface2".
    // (We're creating points spaced more densely than we'll query just to verify that we only get back the queried points)
    SurfaceData::SurfaceTagVector providerTags = { SurfaceData::SurfaceTag(m_testSurface1Crc), SurfaceData::SurfaceTag(m_testSurface2Crc) };
    MockSurfaceProvider mockProvider(
        MockSurfaceProvider::ProviderType::SURFACE_PROVIDER, providerTags, AZ::Vector3(0.0f), AZ::Vector3(8.0f),
        AZ::Vector3(0.25f, 0.25f, 4.0f));

    // Query for all the surface points from (0, 0, 16) - (4, 4, 16) with a step size of 1.
    SurfaceData::SurfacePointList availablePointsPerPosition;
    AZ::Vector2 stepSize(1.0f, 1.0f);
    AZ::Aabb regionBounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.0f, 0.0f, 16.0f), AZ::Vector3(4.0f, 4.0f, 16.0f));

    AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->GetSurfacePointsFromRegion(
        regionBounds, stepSize, providerTags, availablePointsPerPosition);

    // For each point entry returned from GetSurfacePointsFromRegion, call GetSurfacePoints and verify the results match.
    AZStd::vector<AZ::Vector3> queryPositions;
    for (float y = 0.0f; y < 4.0f; y += 1.0f)
    {
        for (float x = 0.0f; x < 4.0f; x += 1.0f)
        {
            queryPositions.push_back(AZ::Vector3(x, y, 16.0f));
        }
    }

    CompareSurfacePointListWithGetSurfacePoints(queryPositions, availablePointsPerPosition, providerTags);
}

TEST_F(SurfaceDataTestApp, SurfaceData_VerifyGetSurfacePointsFromListAndGetSurfacePointsMatch)
{
    // This ensures that both GetSurfacePointsFromList and GetSurfacePoints produce the same results.

    // Create a mock Surface Provider that covers from (0, 0) - (8, 8) in space.
    // It defines points spaced 0.25 apart, with heights of 0 and 4, and with the tags "test_surface1" and "test_surface2".
    // (We're creating points spaced more densely than we'll query just to verify that we only get back the queried points)
    SurfaceData::SurfaceTagVector providerTags = { SurfaceData::SurfaceTag(m_testSurface1Crc), SurfaceData::SurfaceTag(m_testSurface2Crc) };
    MockSurfaceProvider mockProvider(
        MockSurfaceProvider::ProviderType::SURFACE_PROVIDER, providerTags, AZ::Vector3(0.0f), AZ::Vector3(8.0f),
        AZ::Vector3(0.25f, 0.25f, 4.0f));

    // Query for all the surface points from (0, 0, 16) - (4, 4, 16) with a step size of 1.
    SurfaceData::SurfacePointList availablePointsPerPosition;
    AZStd::vector<AZ::Vector3> queryPositions;
    for (float y = 0.0f; y < 4.0f; y += 1.0f)
    {
        for (float x = 0.0f; x < 4.0f; x += 1.0f)
        {
            queryPositions.push_back(AZ::Vector3(x, y, 16.0f));
        }
    }

    AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->GetSurfacePointsFromList(
        queryPositions, providerTags, availablePointsPerPosition);

    // For each point entry returned from GetSurfacePointsFromList, call GetSurfacePoints and verify the results match.
    CompareSurfacePointListWithGetSurfacePoints(queryPositions, availablePointsPerPosition, providerTags);
}

TEST_F(SurfaceDataTestApp, SurfaceData_FirstPointFilteredOut_SurfacePointListRemovesFilteredPointsCorrectly)
{
    // Arbitrary set of input points.
    AZStd::array<AZ::Vector3, 3> inPositions = {
        AZ::Vector3(0.0f),
        AZ::Vector3(1.0f),
        AZ::Vector3(2.0f)
    };

    // The surface tag to filter by. Any point with this tag will be kept, any point without this tag will be removed.
    AZ::Crc32 filterTag("keep_this_point");
    AZStd::array<SurfaceData::SurfaceTag, 1> filterTags = { filterTag };

    // Arbitrary number of output points to generate per input point.
    const uint32_t outputPointsPerInput = 3;

    // Create a set of test points where we generate multiple outputs for every input, but don't put the filter tag on the first output
    // for each point. Our expectation is that the first output point for each input will get filtered out.
    SurfaceData::SurfacePointList testPoints;
    testPoints.StartListConstruction(inPositions, outputPointsPerInput, filterTags);
    for (size_t inPosition = 0; inPosition < inPositions.size(); inPosition++)
    {
        const AZ::Vector3& input = inPositions[inPosition];
        for (uint32_t outPosition = 0; outPosition < outputPointsPerInput; outPosition++)
        {
            // Store different Z values for each output so that we can verify which output got filtered.
            // We use a Z value in the 0-1 range so that we can also use it as our surface tag weight.
            AZ::Vector3 position(input.GetX(), input.GetY(), aznumeric_cast<float>(outPosition) / outputPointsPerInput);
            AZ::Vector3 normal = AZ::Vector3::CreateAxisZ();
            SurfaceData::SurfaceTagWeights weights;

            // Only put a filter weight on points after the first one.
            if (outPosition > 0)
            {
                weights.AddSurfaceTagWeight(filterTag, position.GetZ());
            }

            testPoints.AddSurfacePoint(AZ::EntityId(), inPositions[inPosition], position, normal, weights); 
        }
    }
    testPoints.EndListConstruction();

    // TEST: Verify that our SurfacePointList has the correct number of inputs.
    EXPECT_EQ(testPoints.GetInputPositionSize(), inPositions.size());

    // TEST: Verify that our SurfacePointList has the correct number of outputs, where one output point was filtered out for each input.
    EXPECT_EQ(testPoints.GetSize(), inPositions.size() * (outputPointsPerInput - 1));

    // For each input position, make sure that the outputs we have are the correct ones.
    for (size_t inPosition = 0; inPosition < inPositions.size(); inPosition++)
    {
        // TEST: Verify that one output point was filtered out for each input.
        EXPECT_EQ(testPoints.GetSize(inPosition), outputPointsPerInput - 1);

        testPoints.EnumeratePoints(inPosition, [filterTag]
            (const AZ::Vector3& position, const AZ::Vector3& normal, const SurfaceData::SurfaceTagWeights& surfaceWeights) -> bool
            {
                // TEST: Verify that we didn't keep the first generated position.
                EXPECT_NE(position.GetZ(), 0.0f);

                // TEST: Trivially verify that the normal contains the value we put on all the points.
                EXPECT_EQ(normal, AZ::Vector3::CreateAxisZ());

                // TEST: Verify that we have exactly one surface weight for each point. It should contain our filterTag and a
                // weight that matches our position Z value.
                EXPECT_EQ(surfaceWeights.GetSize(), 1);
                surfaceWeights.EnumerateWeights(
                    [filterTag, position](AZ::Crc32 tag, float weight) -> bool
                    {
                        EXPECT_EQ(tag, filterTag);
                        EXPECT_EQ(weight, position.GetZ());
                        return true;
                    });
                return true;
            });
    }
}

// This uses custom test / benchmark hooks so that we can load LmbrCentral and use Shape components in our unit tests and benchmarks.
AZ_UNIT_TEST_HOOK(new UnitTest::SurfaceDataTestEnvironment, UnitTest::SurfaceDataBenchmarkEnvironment);
