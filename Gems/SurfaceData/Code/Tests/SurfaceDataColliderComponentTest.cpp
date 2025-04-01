/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SurfaceData/Tests/SurfaceDataTestMocks.h>

#include <AzTest/AzTest.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TransformBus.h>

#include <SurfaceData/Components/SurfaceDataColliderComponent.h>

#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/Components/SimulatedBodyComponentBus.h>

namespace UnitTest
{
    class MockPhysicsWorldBusProvider
        : public AzPhysics::SimulatedBodyComponentRequestsBus::Handler
    {
    public:
        MockPhysicsWorldBusProvider(
            const AZ::EntityId& id, AZ::Vector3 inPosition, bool setHitResult, const AzFramework::SurfaceData::SurfacePoint& hitResult)
        {
            AzPhysics::SimulatedBodyComponentRequestsBus::Handler::BusConnect(id);

            // Whether or not the test should return a successful hit, we still want to create a valid
            // AABB so that the SurfaceData component registers itself as a provider.
            m_aabb = AZ::Aabb::CreateCenterRadius(inPosition, 1.0f);

            // Only initialize our mock physics to return a raycast result if the test wants the point to hit.
            if (setHitResult)
            {
                m_rayCastHit.m_resultFlags = AzPhysics::SceneQuery::ResultFlags::Distance |
                    AzPhysics::SceneQuery::ResultFlags::Position |
                    AzPhysics::SceneQuery::ResultFlags::Normal |
                    AzPhysics::SceneQuery::ResultFlags::BodyHandle;
                m_rayCastHit.m_distance = 0.0f;
                m_rayCastHit.m_position = hitResult.m_position;
                m_rayCastHit.m_normal = hitResult.m_normal;
                // Just need to set this to a non-null value, it gets checked vs InvalidSimulatedBodyHandle but not otherwise used.
                m_rayCastHit.m_bodyHandle = AzPhysics::SimulatedBodyHandle(AZ::Crc32(12345), 0);
            }
        }

        virtual ~MockPhysicsWorldBusProvider()
        {
            AzPhysics::SimulatedBodyComponentRequestsBus::Handler::BusDisconnect();
        }

        // Minimal mocks needed to mock out this ebus
        void EnablePhysics() override {}
        void DisablePhysics() override {}
        bool IsPhysicsEnabled() const override { return true; }
        AzPhysics::SimulatedBody* GetSimulatedBody() override { return nullptr; }
        AzPhysics::SimulatedBodyHandle GetSimulatedBodyHandle() const override { return AzPhysics::InvalidSimulatedBodyHandle; }

        // Functional mocks to mock out the data needed by the component
        AZ::Aabb GetAabb() const override { return m_aabb; }
        AzPhysics::SceneQueryHit RayCast([[maybe_unused]] const AzPhysics::RayCastRequest& request) override { return m_rayCastHit; }

        AZ::Aabb m_aabb = AZ::Aabb::CreateNull();
        AzPhysics::SceneQueryHit m_rayCastHit;

    };


    // Provide a set of common helper methods for our tests.
    struct SurfaceDataTestFixture
        : public SurfaceDataTest
    {
    protected:
        // Create a new SurfacePoint with the given fields.
        AzFramework::SurfaceData::SurfacePoint CreateSurfacePoint(
            AZ::Vector3 position, AZ::Vector3 normal, AZStd::vector<AZStd::pair<AZStd::string, float>> tags)
        {
            AzFramework::SurfaceData::SurfacePoint point;
            point.m_position = position;
            point.m_normal = normal;
            for (auto& tag : tags)
            {
                point.m_surfaceTags.emplace_back(SurfaceData::SurfaceTag(tag.first), tag.second);
            }
            return point;
        }

        // Compare two surface points.
        bool SurfacePointsAreEqual(
            const AZ::Vector3& lhsPosition,
            const AZ::Vector3& lhsNormal,
            const SurfaceData::SurfaceTagWeights& lhsMasks,
            const AzFramework::SurfaceData::SurfacePoint& rhs)
        {
            return ((lhsPosition == rhs.m_position)
                && (lhsNormal == rhs.m_normal)
                && (lhsMasks.SurfaceWeightsAreEqual(rhs.m_surfaceTags)));
        }

        // Common test function for testing the "Provider" functionality of the component.
        // Given a set of tags and an expected output, check to see if the component provides the
        // expected output point.
        void TestSurfaceDataColliderProvider(AZStd::vector<AZStd::string> providerTags, bool pointOnProvider,
                                             AZ::Vector3 queryPoint, const AzFramework::SurfaceData::SurfacePoint& expectedOutput)
        {
            // This lets our component register with surfaceData successfully.
            MockSurfaceDataSystem mockSurfaceDataSystem;

            // Create the test configuration for the SurfaceDataColliderComponent component
            SurfaceData::SurfaceDataColliderConfig config;
            for (auto& tag : providerTags)
            {
                config.m_providerTags.emplace_back(tag);
            }

            // Create the test entity with the SurfaceDataCollider component and the required physics collider dependency
            auto entity = CreateEntity();
            // Create the components
            CreateComponent<MockPhysicsColliderComponent>(entity.get());
            CreateComponent<SurfaceData::SurfaceDataColliderComponent>(entity.get(), config);
            // Before activating the entity, set up our mock physics provider for this entity 
            MockPhysicsWorldBusProvider mockPhysics(entity->GetId(), expectedOutput.m_position, pointOnProvider, expectedOutput);
            // Now that our mocks are set up, activate the entity.
            ActivateEntity(entity.get());

            // Get our registered provider handle (and verify that it's valid)
            auto providerHandle = mockSurfaceDataSystem.GetSurfaceProviderHandle(entity->GetId());
            EXPECT_TRUE(providerHandle != SurfaceData::InvalidSurfaceDataRegistryHandle);

            // Call GetSurfacePoints and verify the results
            SurfaceData::SurfacePointList pointList;
            pointList.StartListConstruction(AZStd::span<const AZ::Vector3>(&queryPoint, 1), 1, {});
            SurfaceData::SurfaceDataProviderRequestBus::Event(providerHandle, &SurfaceData::SurfaceDataProviderRequestBus::Events::GetSurfacePoints,
                                                              queryPoint, pointList);
            pointList.EndListConstruction();

            if (pointOnProvider)
            {
                ASSERT_EQ(pointList.GetSize(), 1);
                pointList.EnumeratePoints([this, expectedOutput](
                        [[maybe_unused]] size_t inPositionIndex, const AZ::Vector3& position,
                        const AZ::Vector3& normal, const SurfaceData::SurfaceTagWeights& masks) -> bool
                    {
                        EXPECT_TRUE(SurfacePointsAreEqual(position, normal, masks, expectedOutput));
                        return true;
                    });
            }
            else
            {
                EXPECT_TRUE(pointList.IsEmpty());
            }
        }

        void TestSurfaceDataColliderModifier(AZStd::vector<AZStd::string> modifierTags,
            const AzFramework::SurfaceData::SurfacePoint& input,
            bool pointInCollider,
            const AzFramework::SurfaceData::SurfacePoint& expectedOutput)
        {
            // This lets our component register with surfaceData successfully.
            MockSurfaceDataSystem mockSurfaceDataSystem;

            // Create the test configuration for the SurfaceDataColliderComponent component
            SurfaceData::SurfaceDataColliderConfig config;
            for (auto& tag : modifierTags)
            {
                config.m_modifierTags.emplace_back(tag);
            }

            // Create the test entity with the SurfaceDataCollider component and the required physics collider dependency
            auto entity = CreateEntity();
            CreateComponent<MockPhysicsColliderComponent>(entity.get());
            CreateComponent<SurfaceData::SurfaceDataColliderComponent>(entity.get(), config);
            // Before activating the entity, set up our mock physics provider for this entity 
            MockPhysicsWorldBusProvider mockPhysics(entity->GetId(), input.m_position, pointInCollider, expectedOutput);
            // Now that our mocks are set up, activate the entity.
            ActivateEntity(entity.get());

            // Get our registered modifier handle (and verify that it's valid)
            auto modifierHandle = mockSurfaceDataSystem.GetSurfaceModifierHandle(entity->GetId());
            EXPECT_TRUE(modifierHandle != SurfaceData::InvalidSurfaceDataRegistryHandle);

            // Call ModifySurfacePoints and verify the results
            // Add the surface point with a different entity ID than the entity doing the modification, so that the point doesn't get
            // filtered out.
            SurfaceData::SurfacePointList pointList;
            pointList.StartListConstruction(AZStd::span<const AzFramework::SurfaceData::SurfacePoint>(&input, 1));
            pointList.ModifySurfaceWeights(modifierHandle);
            pointList.EndListConstruction();
            ASSERT_EQ(pointList.GetSize(), 1);
            pointList.EnumeratePoints([this, expectedOutput](
                    [[maybe_unused]] size_t inPositionIndex, const AZ::Vector3& position,
                    const AZ::Vector3& normal, const SurfaceData::SurfaceTagWeights& masks) -> bool
                {
                    EXPECT_TRUE(SurfacePointsAreEqual(position, normal, masks, expectedOutput));
                    return true;
                });
        }
    };


    TEST_F(SurfaceDataTestFixture, SurfaceDataColliderComponent_CreateComponent)
    {
        // Verify that we can trivially create and destroy the component.

        // This lets our component potentially register with surfaceData successfully.
        MockSurfaceDataSystem mockSurfaceDataSystem;

        // Create an empty configuration for the SurfaceDataColliderComponent component
        SurfaceData::SurfaceDataColliderConfig config;

        // Create the test entity with the SurfaceDataCollider component with the required PhysicsCollider dependency
        auto entity = CreateEntity();
        CreateComponent<MockPhysicsColliderComponent>(entity.get());
        CreateComponent<SurfaceData::SurfaceDataColliderComponent>(entity.get(), config);
        ActivateEntity(entity.get());

        // Verify that we haven't registered as a provider or modifier, because we never mocked up a valid AABB
        // for this collider.
        auto providerHandle = mockSurfaceDataSystem.GetSurfaceProviderHandle(entity->GetId());
        auto modifierHandle = mockSurfaceDataSystem.GetSurfaceModifierHandle(entity->GetId());
        EXPECT_TRUE(providerHandle == SurfaceData::InvalidSurfaceDataRegistryHandle);
        EXPECT_TRUE(modifierHandle == SurfaceData::InvalidSurfaceDataRegistryHandle);

    }

    TEST_F(SurfaceDataTestFixture, SurfaceDataColliderComponent_ProvidePointOnCollider)
    {
        // Verify that for a point on the collider, the output point contains the correct tag and value.

        // Set the expected output to an arbitrary entity ID, position, and normal.
        // We'll use this to initialize the mock physics, so the output of the query should match.
        const char* tag = "test_mask";
        AzFramework::SurfaceData::SurfacePoint expectedOutput =
            CreateSurfacePoint(AZ::Vector3(1.0f), AZ::Vector3::CreateAxisZ(),
            { AZStd::pair<AZStd::string, float>(tag, 1.0f) });

        // Query from the same XY, but one unit higher on Z, just so we can verify that the output returns the collision
        // result, not the input point.
        constexpr bool pointOnCollider = true;
        TestSurfaceDataColliderProvider({ tag }, pointOnCollider, expectedOutput.m_position + AZ::Vector3::CreateAxisZ(), expectedOutput);
    }

    TEST_F(SurfaceDataTestFixture, SurfaceDataColliderComponent_DoNotProvidePointNotOnCollider)
    {
        // Verify that for a point not on the collider, the output point is empty.

        // Set the expected output to an arbitrary entity ID, position, and normal.
        // We'll use this to initialize the mock physics.
        const char* tag = "test_mask";
        AzFramework::SurfaceData::SurfacePoint expectedOutput =
            CreateSurfacePoint(AZ::Vector3(1.0f), AZ::Vector3::CreateAxisZ(),
            { AZStd::pair<AZStd::string, float>(tag, 1.0f) });

        // Query from the same XY, but one unit higher on Z.  However, we're also telling our test to provide
        // a "no hit" result from physics, so the expectedOutput will be ignored on the result check, and instead
        // the output will be verified to be an empty list of points.
        constexpr bool pointOnCollider = true;
        TestSurfaceDataColliderProvider({ tag }, !pointOnCollider, expectedOutput.m_position + AZ::Vector3::CreateAxisZ(), expectedOutput);
    }

    TEST_F(SurfaceDataTestFixture, SurfaceDataColliderComponent_ProvidePointOnColliderWithMultipleTags)
    {
        // Verify that if the component has multiple tags, all of them get put on the output with the same value.

        // Set the expected output to an arbitrary entity ID, position, and normal.
        // We'll use this to initialize the mock physics.
        const char* tag1 = "test_mask1";
        const char* tag2 = "test_mask2";
        AzFramework::SurfaceData::SurfacePoint expectedOutput = CreateSurfacePoint(
            AZ::Vector3(1.0f), AZ::Vector3::CreateAxisZ(),
            { AZStd::pair<AZStd::string, float>(tag1, 1.0f), AZStd::pair<AZStd::string, float>(tag2, 1.0f) });

        // Query from the same XY, but one unit higher on Z, just so we can verify that the output returns the collision
        // result, not the input point.
        constexpr bool pointOnCollider = true;
        TestSurfaceDataColliderProvider({ tag1, tag2 }, pointOnCollider, expectedOutput.m_position + AZ::Vector3::CreateAxisZ(), expectedOutput);
    }

    TEST_F(SurfaceDataTestFixture, SurfaceDataColliderComponent_ModifyPointInCollider)
    {
        // Verify that for a point inside the collider, the output point contains the correct tag and value.

        // Set arbitrary input data
        AzFramework::SurfaceData::SurfacePoint input = CreateSurfacePoint(AZ::Vector3(1.0f), AZ::Vector3(0.0f), {});
        // Output should match the input, but with an added tag / value
        const char* tag = "test_mask";
        AzFramework::SurfaceData::SurfacePoint expectedOutput =
            CreateSurfacePoint(input.m_position, input.m_normal,
            { AZStd::pair<AZStd::string, float>(tag, 1.0f) });

        constexpr bool pointInCollider = true;
        TestSurfaceDataColliderModifier({ tag }, input, pointInCollider, expectedOutput);
    }

    TEST_F(SurfaceDataTestFixture, SurfaceDataColliderComponent_DoNotModifyPointOutsideCollider)
    {
        // Verify that for a point outside the collider, the output point contains no tags / values.

        // Set arbitrary input data
        AzFramework::SurfaceData::SurfacePoint input = CreateSurfacePoint(AZ::Vector3(1.0f), AZ::Vector3(0.0f), {});
        // Output should match the input - no extra tags / values should be added.
        const char* tag = "test_mask";
        AzFramework::SurfaceData::SurfacePoint expectedOutput = CreateSurfacePoint(input.m_position, input.m_normal, {});

        constexpr bool pointInCollider = true;
        TestSurfaceDataColliderModifier({ tag }, input, !pointInCollider, expectedOutput);
    }

    TEST_F(SurfaceDataTestFixture, SurfaceDataColliderComponent_ModifyPointInColliderWithMultipleTags)
    {
        // Verify that if the component has multiple tags, all of them get put on the output with the same value.

        // Set arbitrary input data
        AzFramework::SurfaceData::SurfacePoint input = CreateSurfacePoint(AZ::Vector3(1.0f), AZ::Vector3(0.0f), {});
        // Output should match the input, but with two added tags
        const char* tag1 = "test_mask1";
        const char* tag2 = "test_mask2";
        AzFramework::SurfaceData::SurfacePoint expectedOutput = CreateSurfacePoint(
            input.m_position, input.m_normal,
            { AZStd::pair<AZStd::string, float>(tag1, 1.0f), AZStd::pair<AZStd::string, float>(tag2, 1.0f) });

        constexpr bool pointInCollider = true;
        TestSurfaceDataColliderModifier({ tag1, tag2 }, input, pointInCollider, expectedOutput);
    }

    TEST_F(SurfaceDataTestFixture, SurfaceDataColliderComponent_ModifierPreservesInputTags)
    {
        // Verify that the output contains input tags that are NOT on the modification list and adds any
        // new tags that weren't in the input

        // Set arbitrary input data
        const char* preservedTag = "preserved_tag";
        AzFramework::SurfaceData::SurfacePoint input =
            CreateSurfacePoint(AZ::Vector3(1.0f), AZ::Vector3(0.0f),
                                                             { AZStd::pair<AZStd::string, float>(preservedTag, 1.0f) });
        // Output should match the input, but with two added tags
        const char* modifierTag = "modifier_tag";
        AzFramework::SurfaceData::SurfacePoint expectedOutput = CreateSurfacePoint(
            input.m_position, input.m_normal,
            { AZStd::pair<AZStd::string, float>(preservedTag, 1.0f), AZStd::pair<AZStd::string, float>(modifierTag, 1.0f) });

        constexpr bool pointInCollider = true;
        TestSurfaceDataColliderModifier({ modifierTag }, input, pointInCollider, expectedOutput);
    }

    TEST_F(SurfaceDataTestFixture, SurfaceDataColliderComponent_KeepsHigherValueFromModifier)
    {
        // Verify that if the input has a lower value on the tag than the modifier, it keeps the higher value.

        const char* tag = "test_mask";

        // Select an input value that's lower than the collider value
        float inputValue = 0.25f;

        // Set arbitrary input data
        AzFramework::SurfaceData::SurfacePoint input =
            CreateSurfacePoint(AZ::Vector3(1.0f), AZ::Vector3(0.0f),
                                                             { AZStd::pair<AZStd::string, float>(tag, inputValue) });
        // Output should match the input, except that the value on the tag gets the higher modifier value
        AzFramework::SurfaceData::SurfacePoint expectedOutput =
            CreateSurfacePoint(input.m_position, input.m_normal,
            { AZStd::pair<AZStd::string, float>(tag, 1.0f) });

        constexpr bool pointInCollider = true;
        TestSurfaceDataColliderModifier({ tag }, input, pointInCollider, expectedOutput);
    }
}
