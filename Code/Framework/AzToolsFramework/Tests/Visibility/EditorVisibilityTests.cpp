/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Viewport/CameraState.h>
#include <AzFramework/Visibility/BoundsBus.h>
#include <AzFramework/Visibility/EntityBoundsUnionBus.h>
#include <AzFramework/Visibility/EntityVisibilityQuery.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>

namespace UnitTest
{
    static auto ScreenDimensions = AzFramework::ScreenSize(1280, 720);

    class EditorVisibilityFixture : public ToolsApplicationFixture<>
    {
    public:
        void SetUpEditorFixtureImpl() override {}

        void CreateEditorEntities(const size_t entityCount)
        {
            std::generate_n(
                AZStd::back_inserter(m_editorEntityIds), entityCount,
                [number = 0]() mutable
                {
                    return CreateDefaultEditorEntity(AZStd::string::format("Entity %d", number++).c_str());
                });
        }

        void SetupRowOfEntities(const AZ::Vector3& worldStartPosition, const AZ::Vector3& worldStepVector)
        {
            for (size_t entityIndex = 0; entityIndex < m_editorEntityIds.size(); ++entityIndex)
            {
                AZ::TransformBus::Event(
                    m_editorEntityIds[entityIndex], &AZ::TransformBus::Events::SetWorldTranslation,
                    worldStartPosition + worldStepVector * aznumeric_cast<float>(entityIndex));
            }
        }

        AZStd::vector<AZ::EntityId> m_editorEntityIds;
    };

    TEST_F(EditorVisibilityFixture, VisibilityQueryReturnsEntitiesInFrustumWithNoOrientation)
    {
        using ::testing::UnorderedElementsAreArray;

        constexpr size_t EditorEntityCount = 21;
        constexpr size_t BeginVisibleEntityRangeOffset = 7;
        constexpr size_t EndVisibleEntityRangeOffset = 14;

        // setup row of editor entities
        CreateEditorEntities(EditorEntityCount);
        SetupRowOfEntities(AZ::Vector3::CreateAxisX(-20.0f), AZ::Vector3::CreateAxisX(2.0f));

        // request the entity union bounds system to update
        AzFramework::IEntityBoundsUnionRequestBus::Broadcast(
            &AzFramework::IEntityBoundsUnionRequestBus::Events::ProcessEntityBoundsUnionRequests);

        // create default camera looking down the negative y-axis moved just back from the origin
        AzFramework::CameraState cameraState = AzFramework::CreateDefaultCamera(
            AZ::Transform::CreateTranslation(AZ::Vector3::CreateAxisY(-5.0f)), ScreenDimensions);

        // perform a visibility query based on the state of the camera
        AzFramework::EntityVisibilityQuery entityVisibilityQuery;
        entityVisibilityQuery.UpdateVisibility(cameraState);

        // build a vector of visible entities
        AZStd::vector<AZ::EntityId> visibleEditorEntityIds;
        AZStd::copy(
            entityVisibilityQuery.Begin(), entityVisibilityQuery.End(), AZStd::back_inserter(visibleEditorEntityIds));

        // build the expected vector of entity ids (the middle portion of the row based on the centered position of the
        // camera
        AZStd::vector<AZ::EntityId> expectedEditorEntities;
        AZStd::copy(
            m_editorEntityIds.begin() + BeginVisibleEntityRangeOffset,
            m_editorEntityIds.begin() + EndVisibleEntityRangeOffset, AZStd::back_inserter(expectedEditorEntities));

        EXPECT_THAT(visibleEditorEntityIds, UnorderedElementsAreArray(expectedEditorEntities));
    }

    TEST_F(EditorVisibilityFixture, VisibilityQueryReturnsEntitiesInFrustumWithOrientationAndOffset)
    {
        using ::testing::UnorderedElementsAreArray;

        constexpr size_t EditorEntityCount = 21;
        constexpr size_t BeginVisibleEntityRangeOffset = 0;
        constexpr size_t EndVisibleEntityRangeOffset = 10;

        // setup row of editor entities
        CreateEditorEntities(EditorEntityCount);
        SetupRowOfEntities(AZ::Vector3::CreateAxisX(-20.0f), AZ::Vector3::CreateAxisX(2.0f));

        // request the entity union bounds system to update
        AzFramework::IEntityBoundsUnionRequestBus::Broadcast(
            &AzFramework::IEntityBoundsUnionRequestBus::Events::ProcessEntityBoundsUnionRequests);

        // create default camera looking down the negative x-axis moved along the x-axis and tilted slightly down
        AzFramework::CameraState cameraState = AzFramework::CreateDefaultCamera(
            AZ::Transform::CreateFromQuaternionAndTranslation(
                AZ::Quaternion::CreateRotationZ(AZ::DegToRad(90.0f)) *
                    AZ::Quaternion::CreateRotationX(AZ::DegToRad(-25.0f)),
                AZ::Vector3(2.0f, 0.0f, 5.0f)),
            ScreenDimensions);

        // perform a visibility query based on the state of the camera
        AzFramework::EntityVisibilityQuery entityVisibilityQuery;
        entityVisibilityQuery.UpdateVisibility(cameraState);

        // build the expected vector of entity ids (the first 10 entities in the row)
        AZStd::vector<AZ::EntityId> expectedEditorEntities;
        AZStd::copy(
            m_editorEntityIds.begin() + BeginVisibleEntityRangeOffset,
            m_editorEntityIds.begin() + EndVisibleEntityRangeOffset, AZStd::back_inserter(expectedEditorEntities));

        // build a vector of visible entities
        AZStd::vector<AZ::EntityId> visibleEditorEntityIds;
        AZStd::copy(
            entityVisibilityQuery.Begin(), entityVisibilityQuery.End(), AZStd::back_inserter(visibleEditorEntityIds));

        EXPECT_THAT(visibleEditorEntityIds, UnorderedElementsAreArray(expectedEditorEntities));
    }

    TEST_F(EditorVisibilityFixture, TranslatedEntityIsRemovedFromVisibilityQueryWhenOutsideFrustum)
    {
        using ::testing::UnorderedElementsAreArray;

        constexpr size_t EditorEntityCount = 21;
        constexpr size_t BeginVisibleEntityRangeOffset = 7;
        constexpr size_t EndVisibleEntityRangeOffset = 14;

        // setup row of editor entities
        CreateEditorEntities(EditorEntityCount);
        SetupRowOfEntities(AZ::Vector3::CreateAxisX(-20.0f), AZ::Vector3::CreateAxisX(2.0f));

        // request the entity union bounds system to update
        AzFramework::IEntityBoundsUnionRequestBus::Broadcast(
            &AzFramework::IEntityBoundsUnionRequestBus::Events::ProcessEntityBoundsUnionRequests);

        const AZ::EntityId entityIdToMove = m_editorEntityIds[10];
        AZ::TransformBus::Event(
            entityIdToMove, &AZ::TransformBus::Events::SetWorldTranslation, AZ::Vector3::CreateAxisZ(100.0f));

        AzFramework::IEntityBoundsUnionRequestBus::Broadcast(
            &AzFramework::IEntityBoundsUnionRequestBus::Events::ProcessEntityBoundsUnionRequests);

        // create default camera looking down the negative y-axis moved just back from the origin
        AzFramework::CameraState cameraState = AzFramework::CreateDefaultCamera(
            AZ::Transform::CreateTranslation(AZ::Vector3::CreateAxisY(-5.0f)), ScreenDimensions);

        // perform a visibility query based on the state of the camera
        AzFramework::EntityVisibilityQuery entityVisibilityQuery;
        entityVisibilityQuery.UpdateVisibility(cameraState);

        // build a vector of visible entities
        AZStd::vector<AZ::EntityId> visibleEditorEntityIds;
        AZStd::copy(
            entityVisibilityQuery.Begin(), entityVisibilityQuery.End(), AZStd::back_inserter(visibleEditorEntityIds));

        // build the expected vector of entity ids (the middle portion of the row based on the centered position of the
        // camera
        AZStd::vector<AZ::EntityId> expectedEditorEntities;
        AZStd::copy(
            m_editorEntityIds.begin() + BeginVisibleEntityRangeOffset,
            m_editorEntityIds.begin() + EndVisibleEntityRangeOffset, AZStd::back_inserter(expectedEditorEntities));
        // remove moved entity from expected vector
        expectedEditorEntities.erase(
            AZStd::remove(expectedEditorEntities.begin(), expectedEditorEntities.end(), entityIdToMove),
            expectedEditorEntities.end());

        EXPECT_THAT(visibleEditorEntityIds, UnorderedElementsAreArray(expectedEditorEntities));
    }

    class TestBoundComponent
        : public AZ::Component
        , public AzFramework::BoundsRequestBus::Handler
    {
    public:
        AZ_COMPONENT(TestBoundComponent, "{20BB6DB0-B6C0-4D11-A963-B2884F764C4E}");
        static void Reflect(AZ::ReflectContext* context);

        TestBoundComponent() = default;

        // BoundsRequestBus overrides
        AZ::Aabb GetWorldBounds() override;
        AZ::Aabb GetLocalBounds() override;

        void ChangeBounds(const AZ::Aabb& localAabb);

    protected:
        // AZ::Component overrides ...
        void Activate() override;
        void Deactivate() override;

    private:
        AZ::Aabb m_localAabb = AZ::Aabb::CreateNull();
    };

    void TestBoundComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TestBoundComponent, AZ::Component>()->Version(1);
        }
    }

    void TestBoundComponent::Activate()
    {
        m_localAabb = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-0.5f), AZ::Vector3(0.5f));

        AzFramework::BoundsRequestBus::Handler::BusConnect(GetEntityId());
    }

    void TestBoundComponent::Deactivate()
    {
        AzFramework::BoundsRequestBus::Handler::BusDisconnect();
    }

    AZ::Aabb TestBoundComponent::GetWorldBounds()
    {
        AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(worldFromLocal, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
        return m_localAabb.GetTransformedAabb(worldFromLocal);
    }

    AZ::Aabb TestBoundComponent::GetLocalBounds()
    {
        return m_localAabb;
    }

    void TestBoundComponent::ChangeBounds(const AZ::Aabb& localAabb)
    {
        m_localAabb = localAabb;

        AzFramework::IEntityBoundsUnionRequestBus::Broadcast(
            &AzFramework::IEntityBoundsUnionRequestBus::Events::RefreshEntityLocalBoundsUnion, GetEntityId());
    }

    TEST_F(EditorVisibilityFixture, UpdatedBoundsIntersectingFrustumAddsVisibleEntity)
    {
        using ::testing::ElementsAre;

        // register new test component
        GetApplication()->RegisterComponentDescriptor(TestBoundComponent::CreateDescriptor());

        AZ::Entity* entity = nullptr;
        const auto entityId = CreateDefaultEditorEntity("Entity", &entity);

        entity->Deactivate();
        auto testBoundComponent = static_cast<TestBoundComponent*>(entity->CreateComponent<TestBoundComponent>());
        entity->Activate();

        // move the entity just out of view (to the right of the view frustum)
        AZ::TransformBus::Event(
            entityId, &AZ::TransformBus::Events::SetWorldTranslation, AZ::Vector3(40.0f, -3.0f, 20.0f));

        // request the entity union bounds system to update
        AzFramework::IEntityBoundsUnionRequestBus::Broadcast(
            &AzFramework::IEntityBoundsUnionRequestBus::Events::ProcessEntityBoundsUnionRequests);

        // create default camera looking down the positive x-axis moved to position offset from world origin
        AzFramework::CameraState cameraState = AzFramework::CreateDefaultCamera(
            AZ::Transform::CreateFromQuaternionAndTranslation(
                AZ::Quaternion::CreateRotationZ(AZ::DegToRad(-90.0f)), AZ::Vector3(20.0f, 20.0f, 20.0f)),
            ScreenDimensions);

        // perform a visibility query based on the state of the camera
        AzFramework::EntityVisibilityQuery entityVisibilityQuery;
        entityVisibilityQuery.UpdateVisibility(cameraState);

        // build a vector of visible entities
        AZStd::vector<AZ::EntityId> visibleEditorEntityIds;
        AZStd::copy(
            entityVisibilityQuery.Begin(), entityVisibilityQuery.End(), AZStd::back_inserter(visibleEditorEntityIds));

        EXPECT_TRUE(visibleEditorEntityIds.empty());

        // increase the size of the bounds
        testBoundComponent->ChangeBounds(AZ::Aabb::CreateFromMinMax(AZ::Vector3(-2.5f), AZ::Vector3(2.5f)));

        // perform an 'update' of the visibility system
        AzFramework::IEntityBoundsUnionRequestBus::Broadcast(
            &AzFramework::IEntityBoundsUnionRequestBus::Events::ProcessEntityBoundsUnionRequests);

        entityVisibilityQuery.UpdateVisibility(cameraState);

        AZStd::copy(
            entityVisibilityQuery.Begin(), entityVisibilityQuery.End(), AZStd::back_inserter(visibleEditorEntityIds));

        // check the entity is now visible as its bound intersects the view volume
        EXPECT_THAT(visibleEditorEntityIds, ElementsAre(entityId));
    }

} // namespace UnitTest
