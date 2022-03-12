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
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/TransformBus.h>

#include <Source/Components/SurfaceDataColliderComponent.h>

#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/Components/SimulatedBodyComponentBus.h>

namespace UnitTest
{
    // Mock out a generic Physics Collider Component, which is a required dependency for adding a SurfaceDataColliderComponent.
    struct MockPhysicsColliderComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(MockPhysicsColliderComponent, "{4F7C36DE-6475-4E0A-96A7-BFAF21C07C95}", AZ::Component);

        void Activate() override {}
        void Deactivate() override {}

        static void Reflect(AZ::ReflectContext* reflect) { AZ_UNUSED(reflect); }
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("PhysXColliderService", 0x4ff43f7c));
        }
    };

    class MockPhysicsWorldBusProvider
        : public AzPhysics::SimulatedBodyComponentRequestsBus::Handler
    {
    public:
        MockPhysicsWorldBusProvider(const AZ::EntityId& id, AZ::Vector3 inPosition, bool setHitResult, const SurfaceData::SurfacePoint& hitResult)
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
        SurfaceData::SurfacePoint CreateSurfacePoint(AZ::EntityId id, AZ::Vector3 position, AZ::Vector3 normal, AZStd::vector<AZStd::pair<AZStd::string, float>> tags)
        {
            SurfaceData::SurfacePoint point;
            point.m_entityId = id;
            point.m_position = position;
            point.m_normal = normal;
            for (auto& tag : tags)
            {
                point.m_masks[SurfaceData::SurfaceTag(tag.first)] = tag.second;
            }
            return point;
        }

        // Compare two surface points.
        bool SurfacePointsAreEqual(const SurfaceData::SurfacePoint& lhs, const SurfaceData::SurfacePoint& rhs)
        {
            return (lhs.m_entityId == rhs.m_entityId)
                && (lhs.m_position == rhs.m_position)
                && (lhs.m_normal == rhs.m_normal)
                && (lhs.m_masks == rhs.m_masks);
        }

        // Common test function for testing the "Provider" functionality of the component.
        // Given a set of tags and an expected output, check to see if the component provides the
        // expected output point.
        void TestSurfaceDataColliderProvider(AZStd::vector<AZStd::string> providerTags, bool pointOnProvider,
                                             AZ::Vector3 queryPoint, const SurfaceData::SurfacePoint& expectedOutput)
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
            // Initialize our Entity ID to the one passed in on the expectedOutput
            entity->SetId(expectedOutput.m_entityId);
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
            SurfaceData::SurfaceDataProviderRequestBus::Event(providerHandle, &SurfaceData::SurfaceDataProviderRequestBus::Events::GetSurfacePoints,
                                                              queryPoint, pointList);
            if (pointOnProvider)
            {
                ASSERT_TRUE(pointList.size() == 1);
                EXPECT_TRUE(SurfacePointsAreEqual(pointList[0], expectedOutput));
            }
            else
            {
                EXPECT_TRUE(pointList.empty());
            }
        }

        void TestSurfaceDataColliderModifier(AZStd::vector<AZStd::string> modifierTags,
            const SurfaceData::SurfacePoint& input, bool pointInCollider, const SurfaceData::SurfacePoint& expectedOutput)
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
            SurfaceData::SurfacePointList pointList;
            pointList.emplace_back(input);
            SurfaceData::SurfaceDataModifierRequestBus::Event(modifierHandle, &SurfaceData::SurfaceDataModifierRequestBus::Events::ModifySurfacePoints, pointList);
            ASSERT_TRUE(pointList.size() == 1);
            EXPECT_TRUE(SurfacePointsAreEqual(pointList[0], expectedOutput));
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
        SurfaceData::SurfacePoint expectedOutput = CreateSurfacePoint(AZ::EntityId(0x12345678), AZ::Vector3(1.0f), AZ::Vector3::CreateAxisZ(),
            { AZStd::make_pair<AZStd::string, float>(tag, 1.0f) });

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
        SurfaceData::SurfacePoint expectedOutput = CreateSurfacePoint(AZ::EntityId(0x12345678), AZ::Vector3(1.0f), AZ::Vector3::CreateAxisZ(),
            { AZStd::make_pair<AZStd::string, float>(tag, 1.0f) });

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
        SurfaceData::SurfacePoint expectedOutput = CreateSurfacePoint(AZ::EntityId(0x12345678), AZ::Vector3(1.0f), AZ::Vector3::CreateAxisZ(),
                                                                      { AZStd::make_pair<AZStd::string, float>(tag1, 1.0f),
                                                                        AZStd::make_pair<AZStd::string, float>(tag2, 1.0f) });

        // Query from the same XY, but one unit higher on Z, just so we can verify that the output returns the collision
        // result, not the input point.
        constexpr bool pointOnCollider = true;
        TestSurfaceDataColliderProvider({ tag1, tag2 }, pointOnCollider, expectedOutput.m_position + AZ::Vector3::CreateAxisZ(), expectedOutput);
    }

    TEST_F(SurfaceDataTestFixture, SurfaceDataColliderComponent_ModifyPointInCollider)
    {
        // Verify that for a point inside the collider, the output point contains the correct tag and value.

        // Set arbitrary input data
        SurfaceData::SurfacePoint input = CreateSurfacePoint(AZ::EntityId(0x12345678), AZ::Vector3(1.0f), AZ::Vector3(0.0f), {});
        // Output should match the input, but with an added tag / value
        const char* tag = "test_mask";
        SurfaceData::SurfacePoint expectedOutput = CreateSurfacePoint(input.m_entityId, input.m_position, input.m_normal,
                                                                      { AZStd::make_pair<AZStd::string, float>(tag, 1.0f) });

        constexpr bool pointInCollider = true;
        TestSurfaceDataColliderModifier({ tag }, input, pointInCollider, expectedOutput);
    }

    TEST_F(SurfaceDataTestFixture, SurfaceDataColliderComponent_DoNotModifyPointOutsideCollider)
    {
        // Verify that for a point outside the collider, the output point contains no tags / values.

        // Set arbitrary input data
        SurfaceData::SurfacePoint input = CreateSurfacePoint(AZ::EntityId(0x12345678), AZ::Vector3(1.0f), AZ::Vector3(0.0f), {});
        // Output should match the input - no extra tags / values should be added.
        const char* tag = "test_mask";
        SurfaceData::SurfacePoint expectedOutput = CreateSurfacePoint(input.m_entityId, input.m_position, input.m_normal, {});

        constexpr bool pointInCollider = true;
        TestSurfaceDataColliderModifier({ tag }, input, !pointInCollider, expectedOutput);
    }

    TEST_F(SurfaceDataTestFixture, SurfaceDataColliderComponent_ModifyPointInColliderWithMultipleTags)
    {
        // Verify that if the component has multiple tags, all of them get put on the output with the same value.

        // Set arbitrary input data
        SurfaceData::SurfacePoint input = CreateSurfacePoint(AZ::EntityId(0x12345678), AZ::Vector3(1.0f), AZ::Vector3(0.0f), {});
        // Output should match the input, but with two added tags
        const char* tag1 = "test_mask1";
        const char* tag2 = "test_mask2";
        SurfaceData::SurfacePoint expectedOutput = CreateSurfacePoint(input.m_entityId, input.m_position, input.m_normal,
            { AZStd::make_pair<AZStd::string, float>(tag1, 1.0f), AZStd::make_pair<AZStd::string, float>(tag2, 1.0f) });

        constexpr bool pointInCollider = true;
        TestSurfaceDataColliderModifier({ tag1, tag2 }, input, pointInCollider, expectedOutput);
    }

    TEST_F(SurfaceDataTestFixture, SurfaceDataColliderComponent_ModifierPreservesInputTags)
    {
        // Verify that the output contains input tags that are NOT on the modification list and adds any
        // new tags that weren't in the input

        // Set arbitrary input data
        const char* preservedTag = "preserved_tag";
        SurfaceData::SurfacePoint input = CreateSurfacePoint(AZ::EntityId(0x12345678), AZ::Vector3(1.0f), AZ::Vector3(0.0f),
                                                             { AZStd::make_pair<AZStd::string, float>(preservedTag, 1.0f) });
        // Output should match the input, but with two added tags
        const char* modifierTag = "modifier_tag";
        SurfaceData::SurfacePoint expectedOutput = CreateSurfacePoint(input.m_entityId, input.m_position, input.m_normal,
            { AZStd::make_pair<AZStd::string, float>(preservedTag, 1.0f), AZStd::make_pair<AZStd::string, float>(modifierTag, 1.0f) });

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
        SurfaceData::SurfacePoint input = CreateSurfacePoint(AZ::EntityId(0x12345678), AZ::Vector3(1.0f), AZ::Vector3(0.0f),
                                                             { AZStd::make_pair<AZStd::string, float>(tag, inputValue) });
        // Output should match the input, except that the value on the tag gets the higher modifier value
        SurfaceData::SurfacePoint expectedOutput = CreateSurfacePoint(input.m_entityId, input.m_position, input.m_normal,
            { AZStd::make_pair<AZStd::string, float>(tag, 1.0f) });

        constexpr bool pointInCollider = true;
        TestSurfaceDataColliderModifier({ tag }, input, pointInCollider, expectedOutput);
    }
}
