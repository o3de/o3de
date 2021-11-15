/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <Mocks/ICryPakMock.h>
#include <Mocks/IConsoleMock.h>
#include <Mocks/ISystemMock.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Math/Random.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/std/chrono/clocks.h>
#include <SurfaceDataSystemComponent.h>
#include <SurfaceDataModule.h>
#include <SurfaceData/SurfaceDataProviderRequestBus.h>
#include <SurfaceData/SurfaceDataModifierRequestBus.h>
#include <SurfaceData/SurfaceTag.h>
#include <SurfaceData/Utility/SurfaceDataUtility.h>

struct MockGlobalEnvironment
{
    MockGlobalEnvironment()
    {
        m_stubEnv.pCryPak = &m_stubPak;
        m_stubEnv.pConsole = &m_stubConsole;
        m_stubEnv.pSystem = &m_stubSystem;
        gEnv = &m_stubEnv;
    }

    ~MockGlobalEnvironment()
    {
        gEnv = nullptr;
    }

private:
    SSystemGlobalEnvironment m_stubEnv;
    testing::NiceMock<CryPakMock> m_stubPak;
    testing::NiceMock<ConsoleMock> m_stubConsole;
    testing::NiceMock<SystemMock> m_stubSystem;
};

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
        AZStd::unordered_map<AZStd::pair<float, float>, SurfaceData::SurfacePointList> m_GetSurfacePoints;
        SurfaceData::SurfaceTagVector m_tags;
        ProviderType m_providerType;
        AZ::EntityId m_id;

        void SetPoints(AZ::Vector3 start, AZ::Vector3 end, AZ::Vector3 stepSize)
        {
            m_GetSurfacePoints.clear();

            // Create a set of points that go from start to end (exclusive), with one
            // point per step size.
            // The XY values create new SurfacePoint entries, the Z values are used to create
            // the list of surface points at each XY input point.
            for (float y = start.GetY(); y < end.GetY(); y += stepSize.GetY())
            {
                for (float x = start.GetX(); x < end.GetX(); x += stepSize.GetX())
                {
                    SurfaceData::SurfacePointList points;
                    for (float z = start.GetZ(); z < end.GetZ(); z += stepSize.GetZ())
                    {
                        SurfaceData::SurfacePoint point;
                        point.m_entityId = m_id;
                        point.m_position = AZ::Vector3(x, y, z);
                        point.m_normal = AZ::Vector3::CreateAxisZ();
                        AddMaxValueForMasks(point.m_masks, m_tags, 1.0f);

                        points.push_back(point);
                    }
                    m_GetSurfacePoints[AZStd::pair<float, float>(x, y)] = points;
                }
            }
        }

        AZ::Aabb GetBounds()
        {
            AZ::Aabb bounds = AZ::Aabb::CreateNull();

            for (auto& entry : m_GetSurfacePoints)
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
                SurfaceData::SurfaceDataSystemRequestBus::BroadcastResult(m_providerHandle, &SurfaceData::SurfaceDataSystemRequestBus::Events::RegisterSurfaceDataProvider, registryEntry);
                SurfaceData::SurfaceDataProviderRequestBus::Handler::BusConnect(m_providerHandle);
            }
            else
            {
                SurfaceData::SurfaceDataSystemRequestBus::BroadcastResult(m_providerHandle, &SurfaceData::SurfaceDataSystemRequestBus::Events::RegisterSurfaceDataModifier, registryEntry);
                SurfaceData::SurfaceDataModifierRequestBus::Handler::BusConnect(m_providerHandle);
            }
        }

        void Unregister()
        {
            if (m_providerType == ProviderType::SURFACE_PROVIDER)
            {
                SurfaceData::SurfaceDataProviderRequestBus::Handler::BusDisconnect();
                SurfaceData::SurfaceDataSystemRequestBus::Broadcast(&SurfaceData::SurfaceDataSystemRequestBus::Events::UnregisterSurfaceDataProvider, m_providerHandle);
            }
            else
            {
                SurfaceData::SurfaceDataModifierRequestBus::Handler::BusDisconnect();
                SurfaceData::SurfaceDataSystemRequestBus::Broadcast(&SurfaceData::SurfaceDataSystemRequestBus::Events::UnregisterSurfaceDataModifier, m_providerHandle);
            }

            m_providerHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;
        }


        //////////////////////////////////////////////////////////////////////////
        // SurfaceDataProviderRequestBus
        void GetSurfacePoints(const AZ::Vector3& inPosition, SurfaceData::SurfacePointList& surfacePointList) const override
        {
            auto surfacePoints = m_GetSurfacePoints.find(AZStd::make_pair(inPosition.GetX(), inPosition.GetY()));

            if (surfacePoints != m_GetSurfacePoints.end())
            {
                for (auto& point : surfacePoints->second)
                {
                    surfacePointList.push_back(point);
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // SurfaceDataModifierRequestBus
        void ModifySurfacePoints(SurfaceData::SurfacePointList& surfacePointList) const override
        {
            for (auto& point : surfacePointList)
            {
                auto surfacePoints = m_GetSurfacePoints.find(AZStd::make_pair(point.m_position.GetX(), point.m_position.GetY()));

                if (surfacePoints != m_GetSurfacePoints.end())
                {
                    AddMaxValueForMasks(point.m_masks, m_tags, 1.0f);
                }
            }

        }

        SurfaceData::SurfaceDataRegistryHandle m_providerHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;

};




TEST(SurfaceDataTest, ComponentsWithComponentApplication)
{
    AZ::ComponentApplication::Descriptor appDesc;
    appDesc.m_memoryBlocksByteSize = 10 * 1024 * 1024;
    appDesc.m_recordingMode = AZ::Debug::AllocationRecords::RECORD_FULL;
    appDesc.m_stackRecordLevels = 20;

    MockGlobalEnvironment mocks;

    AZ::ComponentApplication app;
    AZ::Entity* systemEntity = app.Create(appDesc);
    ASSERT_TRUE(systemEntity != nullptr);
    app.RegisterComponentDescriptor(SurfaceData::SurfaceDataSystemComponent::CreateDescriptor());
    systemEntity->CreateComponent<SurfaceData::SurfaceDataSystemComponent>();

    systemEntity->Init();
    systemEntity->Activate();

    app.Destroy();
    ASSERT_TRUE(true);
}

class SurfaceDataTestApp
    : public ::testing::Test
{
public:
    SurfaceDataTestApp()
        : m_application()
        , m_systemEntity(nullptr)
    {
    }

    void SetUp() override
    {
        AZ::ComponentApplication::Descriptor appDesc;
        appDesc.m_memoryBlocksByteSize = 50 * 1024 * 1024;
        appDesc.m_recordingMode = AZ::Debug::AllocationRecords::RECORD_FULL;
        appDesc.m_stackRecordLevels = 20;

        AZ::ComponentApplication::StartupParameters appStartup;
        appStartup.m_createStaticModulesCallback =
            [](AZStd::vector<AZ::Module*>& modules)
        {
            modules.emplace_back(new SurfaceData::SurfaceDataModule);
        };

        m_systemEntity = m_application.Create(appDesc, appStartup);
        m_systemEntity->Init();
        m_systemEntity->Activate();
    }

    void TearDown() override
    {
        m_application.Destroy();
    }

    bool ValidateRegionListSize(AZ::Aabb bounds, AZ::Vector2 stepSize, const SurfaceData::SurfacePointListPerPosition& outputList)
    {
        // We expect the output list to contain width * height output entries.
        // The right edge of the AABB should be treated as exclusive, so a 4x4 box with 1 step size will produce 16 entries (0, 1, 2, 3 on each dimension),
        // but a 4.1 x 4.1 box with 1 step size will produce 25 entries (0, 1, 2, 3, 4 on each dimension).
        return (outputList.size() == aznumeric_cast<size_t>(ceil(bounds.GetXExtent() * stepSize.GetX()) * ceil(bounds.GetYExtent() * stepSize.GetY())));
    }


    AZ::ComponentApplication m_application;
    AZ::Entity* m_systemEntity;
    MockGlobalEnvironment m_mocks;

    // Test Surface Data tags that we can use for testing query functionality
    const AZ::Crc32 m_testSurface1Crc = AZ::Crc32("test_surface1");
    const AZ::Crc32 m_testSurface2Crc = AZ::Crc32("test_surface2");
    const AZ::Crc32 m_testSurfaceNoMatchCrc = AZ::Crc32("test_surface_no_match");

};

TEST_F(SurfaceDataTestApp, SurfaceData_TestRegisteredTags)
{
    AZStd::vector<AZStd::pair<AZ::u32, AZStd::string>> registeredTags = SurfaceData::SurfaceTag::GetRegisteredTags();

    for (const auto& searchTerm : SurfaceData::Constants::s_allTagNames)
    {
        ASSERT_TRUE(AZStd::find_if(registeredTags.begin(), registeredTags.end(), [searchTerm](decltype(registeredTags)::value_type pair)
        {
            return pair.second == searchTerm;
        }));
    }
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
        EXPECT_TRUE(SurfaceData::AabbOverlaps2D(box1, box2) == testCase.m_overlaps);
        EXPECT_TRUE(SurfaceData::AabbOverlaps2D(box2, box1) == testCase.m_overlaps);
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
        EXPECT_TRUE(SurfaceData::AabbContains2D(box, point) == testCase.m_contains);
        // Test the Vector2 version as well.
        EXPECT_TRUE(SurfaceData::AabbContains2D(box, AZ::Vector2(point.GetX(), point.GetY())) == testCase.m_contains);
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
    SurfaceData::SurfacePointListPerPosition availablePointsPerPosition;
    AZ::Vector2 stepSize(1.0f, 1.0f);
    AZ::Aabb regionBounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.0f, 0.0f, 16.0f), AZ::Vector3(4.0f, 4.0f, 16.0f));
    SurfaceData::SurfaceTagVector testTags = providerTags;

    SurfaceData::SurfaceDataSystemRequestBus::Broadcast(
        &SurfaceData::SurfaceDataSystemRequestBus::Events::GetSurfacePointsFromRegion,
        regionBounds, stepSize, testTags, availablePointsPerPosition);

    EXPECT_TRUE(ValidateRegionListSize(regionBounds, stepSize, availablePointsPerPosition));

    // We expect every entry in the output list to have two surface points, at heights 0 and 4, sorted in
    // decreasing height order.  The XY positions should match the query positions, and the masks list should
    // be the same size as the set of masks the provider owns.  We *could* check every mask as well for completeness,
    // but that seems like overkill.
    for (auto& queryPosition : availablePointsPerPosition)
    {
        const SurfaceData::SurfacePointList& pointList = queryPosition.second;
        EXPECT_TRUE(pointList.size() == 2);
        EXPECT_TRUE(pointList[0].m_position.GetZ() == 4.0f);
        EXPECT_TRUE(pointList[1].m_position.GetZ() == 0.0f);
        for (auto& point : pointList)
        {
            EXPECT_TRUE(queryPosition.first.GetX() == point.m_position.GetX());
            EXPECT_TRUE(queryPosition.first.GetY() == point.m_position.GetY());
            EXPECT_TRUE(point.m_masks.size() == providerTags.size());
        }
    }
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
    SurfaceData::SurfacePointListPerPosition availablePointsPerPosition;
    AZ::Vector2 stepSize(1.0f, 1.0f);
    AZ::Aabb regionBounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.0f), AZ::Vector3(4.0f));
    SurfaceData::SurfaceTagVector testTags = { SurfaceData::SurfaceTag(m_testSurfaceNoMatchCrc) };

    SurfaceData::SurfaceDataSystemRequestBus::Broadcast(
        &SurfaceData::SurfaceDataSystemRequestBus::Events::GetSurfacePointsFromRegion,
        regionBounds, stepSize, testTags, availablePointsPerPosition);

    EXPECT_TRUE(ValidateRegionListSize(regionBounds, stepSize, availablePointsPerPosition));

    // We expect every entry in the output list to have no surface points, since the requested mask doesn't match
    // any of the masks from our mock surface provider.
    for (auto& queryPosition : availablePointsPerPosition)
    {
        EXPECT_TRUE(queryPosition.second.size() == 0);
    }
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
    SurfaceData::SurfacePointListPerPosition availablePointsPerPosition;
    AZ::Vector2 stepSize(1.0f, 1.0f);
    AZ::Aabb regionBounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(16.0f), AZ::Vector3(20.0f));
    SurfaceData::SurfaceTagVector testTags = providerTags;

    SurfaceData::SurfaceDataSystemRequestBus::Broadcast(
        &SurfaceData::SurfaceDataSystemRequestBus::Events::GetSurfacePointsFromRegion,
        regionBounds, stepSize, testTags, availablePointsPerPosition);

    EXPECT_TRUE(ValidateRegionListSize(regionBounds, stepSize, availablePointsPerPosition));

    // We expect every entry in the output list to have no surface points, since the input points don't overlap with
    // our surface provider.
    for (auto& queryPosition : availablePointsPerPosition)
    {
        const SurfaceData::SurfacePointList& pointList = queryPosition.second;
        EXPECT_TRUE(pointList.size() == 0);
    }
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
        SurfaceData::SurfacePointListPerPosition availablePointsPerPosition;
        AZ::Vector2 stepSize(1.0f, 1.0f);
        AZ::Aabb regionBounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.0f), AZ::Vector3(4.0f));
        SurfaceData::SurfaceTagVector testTags = tagTest;

        SurfaceData::SurfaceDataSystemRequestBus::Broadcast(
            &SurfaceData::SurfaceDataSystemRequestBus::Events::GetSurfacePointsFromRegion,
            regionBounds, stepSize, testTags, availablePointsPerPosition);

        EXPECT_TRUE(ValidateRegionListSize(regionBounds, stepSize, availablePointsPerPosition));

        // We expect every entry in the output list to have two surface points (with heights 0 and 4),
        // and each point should have both the "test_surface1" and "test_surface2" tag.
        for (auto& queryPosition : availablePointsPerPosition)
        {
            const SurfaceData::SurfacePointList& pointList = queryPosition.second;
            EXPECT_TRUE(pointList.size() == 2);
            for (auto& point : pointList)
            {
                EXPECT_TRUE(point.m_masks.size() == 2);
            }
        }
    }
}

TEST_F(SurfaceDataTestApp, SurfaceData_TestSurfacePointsFromRegion_SimilarPointsMergeTogether)
{
    // This test verifies that if two separate providers create points at very similar heights, the
    // points will get merged together in the results, with the resulting point ending up with both
    // sets of tags.

    // Create two mock Surface Providers that covers from (0, 0) - (8, 8) in space, with points spaced 0.25 apart.
    // The first has heights 0 and 4, with the tag "surfaceTag1".  The second has heights 0.005 and 4.005, with the tag "surfaceTag2".
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
    SurfaceData::SurfacePointListPerPosition availablePointsPerPosition;
    AZ::Vector2 stepSize(1.0f, 1.0f);
    AZ::Aabb regionBounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.0f), AZ::Vector3(4.0f));
    SurfaceData::SurfaceTagVector testTags = { SurfaceData::SurfaceTag(m_testSurface1Crc), SurfaceData::SurfaceTag(m_testSurface2Crc) };

    SurfaceData::SurfaceDataSystemRequestBus::Broadcast(
        &SurfaceData::SurfaceDataSystemRequestBus::Events::GetSurfacePointsFromRegion,
        regionBounds, stepSize, testTags, availablePointsPerPosition);

    EXPECT_TRUE(ValidateRegionListSize(regionBounds, stepSize, availablePointsPerPosition));

    // We expect every entry in the output list to have two surface points, not four.  The two points
    // should have both surface tags on them.
    for (auto& queryPosition : availablePointsPerPosition)
    {
        const SurfaceData::SurfacePointList& pointList = queryPosition.second;
        EXPECT_TRUE(pointList.size() == 2);
        for (auto& point : pointList)
        {
            EXPECT_TRUE(point.m_masks.size() == 2);
        }
    }
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
    SurfaceData::SurfacePointListPerPosition availablePointsPerPosition;
    AZ::Vector2 stepSize(1.0f, 1.0f);
    AZ::Aabb regionBounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.0f), AZ::Vector3(4.0f));
    SurfaceData::SurfaceTagVector testTags = { SurfaceData::SurfaceTag(m_testSurface1Crc), SurfaceData::SurfaceTag(m_testSurface2Crc) };

    SurfaceData::SurfaceDataSystemRequestBus::Broadcast(
        &SurfaceData::SurfaceDataSystemRequestBus::Events::GetSurfacePointsFromRegion,
        regionBounds, stepSize, testTags, availablePointsPerPosition);

    EXPECT_TRUE(ValidateRegionListSize(regionBounds, stepSize, availablePointsPerPosition));

    // We expect every entry in the output list to have four surface points with one tag each,
    // because the points are far enough apart that they won't merge.
    for (auto& queryPosition : availablePointsPerPosition)
    {
        const SurfaceData::SurfacePointList& pointList = queryPosition.second;
        EXPECT_TRUE(pointList.size() == 4);
        for (auto& point : pointList)
        {
            EXPECT_TRUE(point.m_masks.size() == 1);
        }
    }
}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
