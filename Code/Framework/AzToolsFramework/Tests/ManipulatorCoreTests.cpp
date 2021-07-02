/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorLockComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityComponent.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>

using namespace AzToolsFramework;

namespace UnitTest
{
    class ManipulatorCoreFixture
        : public ToolsApplicationFixture
    {
    public:
        void SetUpEditorFixtureImpl() override
        {
            m_linearManipulator = LinearManipulator::MakeShared(AZ::Transform::CreateIdentity());

            m_entityId = CreateDefaultEditorEntity("Default", &m_entity);

            if (const auto* transformComponent = m_entity->FindComponent<Components::TransformComponent>())
            {
                m_transformComponentId = transformComponent->GetId();
                m_linearManipulator->AddEntityComponentIdPair(
                    AZ::EntityComponentIdPair{ m_entityId, m_transformComponentId });
            }

            if (const auto* lockComponent = m_entity->FindComponent<Components::EditorLockComponent>())
            {
                m_lockComponentId = lockComponent->GetId();
                m_linearManipulator->AddEntityComponentIdPair(
                    AZ::EntityComponentIdPair{ m_entityId, m_lockComponentId });
            }

            if (const auto* visibilityComponent = m_entity->FindComponent<Components::EditorVisibilityComponent>())
            {
                m_visibiltyComponentId = visibilityComponent->GetId();
                m_linearManipulator->AddEntityComponentIdPair(
                    AZ::EntityComponentIdPair{ m_entityId, m_visibiltyComponentId });
            }

            m_editorEntityComponentChangeDetector
                = AZStd::make_unique<EditorEntityComponentChangeDetector>(m_entityId);
        }

        void TearDownEditorFixtureImpl() override
        {
        }

        AZStd::shared_ptr<LinearManipulator> m_linearManipulator;
        AZ::Entity* m_entity = nullptr;
        AZ::EntityId m_entityId;
        AZStd::unique_ptr<EditorEntityComponentChangeDetector> m_editorEntityComponentChangeDetector;
        AZ::ComponentId m_transformComponentId;
        AZ::ComponentId m_lockComponentId;
        AZ::ComponentId m_visibiltyComponentId;
    };

    TEST_F(ManipulatorCoreFixture, AllEntityIdComponentPairsRemovedFromManipulatorAfterRemoveEntity)
    {
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        // handled in Fixture::SetUp()
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        m_linearManipulator->RemoveEntityId(m_entityId);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_FALSE(m_linearManipulator->HasEntityId(m_entityId));
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    TEST_F(ManipulatorCoreFixture, EntityIdComponentPairRemovedFromManipulatorAfterRemoveEntityComponentId)
    {
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        const auto entityLockComponentIdPair = AZ::EntityComponentIdPair{ m_entityId, m_lockComponentId };
        const auto entityVisibiltyComponentIdPair = AZ::EntityComponentIdPair{ m_entityId, m_visibiltyComponentId };
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
         m_linearManipulator->RemoveEntityComponentIdPair(entityLockComponentIdPair);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_FALSE(m_linearManipulator->HasEntityComponentIdPair(entityLockComponentIdPair));
        EXPECT_TRUE(m_linearManipulator->HasEntityComponentIdPair(entityVisibiltyComponentIdPair));
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    TEST_F(ManipulatorCoreFixture, EntityComponentsNotifiedAfterManipulatorAction)
    {
        using testing::UnorderedElementsAre;
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        // handled in Fixture::SetUp()
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        m_linearManipulator->OnLeftMouseDown(
            AzToolsFramework::ViewportInteraction::MouseInteraction{}, 0.0f);

        m_linearManipulator->OnLeftMouseUp(
            AzToolsFramework::ViewportInteraction::MouseInteraction{});
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_THAT(
            m_editorEntityComponentChangeDetector->m_componentIds,
            UnorderedElementsAre(m_transformComponentId, m_lockComponentId, m_visibiltyComponentId));

        EXPECT_TRUE(m_editorEntityComponentChangeDetector->PropertyDisplayInvalidated());
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }
} // namespace UnitTest
