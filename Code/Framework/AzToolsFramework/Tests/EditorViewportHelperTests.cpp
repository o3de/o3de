/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/UnitTest/TestDebugDisplayRequests.h>
#include <AzFramework/Viewport/CameraState.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/UnitTest/Mocks/MockEditorViewportIconDisplayInterface.h>
#include <AzToolsFramework/UnitTest/Mocks/MockEditorVisibleEntityDataCacheInterface.h>
#include <AzToolsFramework/UnitTest/Mocks/MockFocusModeInterface.h>
#include <AzToolsFramework/ViewportSelection/EditorHelpers.h>

namespace UnitTest
{
    class EditorViewportOnlyDrawHelpersForSelectedEntityFixture
        : public ToolsApplicationFixture
        , public AzFramework::EntityDebugDisplayEventBus::MultiHandler
    {
    public:
        inline static constexpr AzFramework::ViewportId TestViewportId = 2468;

        static bool isEntitySelected(AZ::EntityId entityId)
        {
            AzToolsFramework::EntityIdList entityIds;
            AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
                entityIds, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);

            if (AZStd::ranges::find(entityIds, entityId) != entityIds.end())
            {
                return true;
            }
            return false;
        }

        void SetUpEditorFixtureImpl() override
        {
            // Setup entity to use for EntityDebugDisplayEventBus and tests
            AZ::Entity* entity1 = nullptr;
            m_entityId = CreateDefaultEditorEntity("DebugHelpersEntity", &entity1);

            AZ::Entity* entity2 = nullptr;
            m_entityId2 = CreateDefaultEditorEntity("DebugHelpersEntity2", &entity2);

            AzFramework::EntityDebugDisplayEventBus::MultiHandler::BusConnect(entity2->GetId());
            AzFramework::EntityDebugDisplayEventBus::MultiHandler::BusConnect(entity1->GetId());

            // DebugDisplay to use in DisplayHelpers
            AzFramework::DebugDisplayRequestBus::BusPtr debugDisplayBus;
            AzFramework::DebugDisplayRequestBus::Bind(debugDisplayBus, TestViewportId);
            m_debugDisplay = AzFramework::DebugDisplayRequestBus::FindFirstHandler(debugDisplayBus);

            m_cameraState = AzFramework::CreateDefaultCamera(AZ::Transform::CreateIdentity(), AzFramework::ScreenSize(1024, 768));

            m_entityVisibleEntityDataCacheMock = AZStd::make_unique<::testing::NiceMock<MockEditorVisibleEntityDataCacheInterface>>();
            m_editorHelpers = AZStd::make_unique<AzToolsFramework::EditorHelpers>(m_entityVisibleEntityDataCacheMock.get());
            m_viewportSettings = AZStd::make_unique<ViewportSettingsTestImpl>();

            m_viewportSettings->Connect(TestViewportId);
            m_viewportSettings->m_helpersVisible = true;
            m_viewportSettings->m_iconsVisible = true;

            using ::testing::_;
            using ::testing::Return;

            EXPECT_CALL(*m_entityVisibleEntityDataCacheMock, GetVisibleEntityId(_))
                .WillOnce(Return(m_entityId))
                .WillOnce(Return(m_entityId2));
            ON_CALL(*m_entityVisibleEntityDataCacheMock, VisibleEntityDataCount()).WillByDefault(Return(2));
            ON_CALL(*m_entityVisibleEntityDataCacheMock, IsVisibleEntityVisible(_)).WillByDefault(Return(true));
        }

        void TearDownEditorFixtureImpl() override
        {
            m_viewportSettings->Disconnect();
            m_viewportSettings.reset();
            m_editorHelpers.reset();
            m_entityVisibleEntityDataCacheMock.reset();
            AzFramework::EntityDebugDisplayEventBus::MultiHandler::BusDisconnect();
        }

        // EntityDebugDisplayEventBus overrides ...
        // This function is called in DisplayComponents which is responsible for drawing the helpers, if this is called it means that a
        // helper has been drawn
        void DisplayEntityViewport(const AzFramework::ViewportInfo&, AzFramework::DebugDisplayRequests&) override
        {       
            const AZ::EntityId entityId = *AzFramework::EntityDebugDisplayEventBus::GetCurrentBusId();

            m_displayEntityViewportEvent = true;
            m_drawCount += 1;
            m_drawnEntity = entityId;
        }

        AZ::EntityId m_entityId;
        AZ::EntityId m_entityId2;
        AZ::EntityId m_drawnEntity;
        AzFramework::DebugDisplayRequests* m_debugDisplay = nullptr;
        int m_drawCount = 0;
        bool m_displayEntityViewportEvent = false;
        AZStd::unique_ptr<ViewportSettingsTestImpl> m_viewportSettings;
        AZStd::unique_ptr<AzToolsFramework::EditorHelpers> m_editorHelpers;
        AZStd::unique_ptr<::testing::NiceMock<MockEditorVisibleEntityDataCacheInterface>> m_entityVisibleEntityDataCacheMock;
        AzFramework::CameraState m_cameraState;
    };

    TEST_F(EditorViewportOnlyDrawHelpersForSelectedEntityFixture, DisplayDebugDrawIfSelectedEntitiesOptionDisabledAndEntitySelected)
    {
        // Given the entity is selected and the option to only show helpers for selected entities is false
        const AzToolsFramework::EntityIdList entityIds = { m_entityId };
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities, entityIds);

        m_viewportSettings->m_onlyShowForSelectedEntities = false;

        // When the draw function is called
        m_editorHelpers->DisplayHelpers(
            AzFramework::ViewportInfo{ TestViewportId },
            m_cameraState,
            *m_debugDisplay,
            [](AZ::EntityId)
            {
                return true;
            });

        // Then the helper should be drawn
        EXPECT_TRUE(m_displayEntityViewportEvent);
        EXPECT_EQ(m_drawCount, 2);
    }

    TEST_F(EditorViewportOnlyDrawHelpersForSelectedEntityFixture, DisplayDebugDrawIfSelectedEntitiesOptionDisabledAndEntityNotSelected)
    {
        // Given the entity is not selected and the option to only show helpers for selected entities is false
        m_viewportSettings->m_onlyShowForSelectedEntities = false;

        // When the draw function is called
        m_editorHelpers->DisplayHelpers(
            AzFramework::ViewportInfo{ TestViewportId },
            m_cameraState,
            *m_debugDisplay,
            [](AZ::EntityId)
            {
                return true;
            });

        // Then the helper should be drawn
        EXPECT_TRUE(m_displayEntityViewportEvent);
        EXPECT_EQ(m_drawCount, 2);
    }

    TEST_F(EditorViewportOnlyDrawHelpersForSelectedEntityFixture, DoNotDisplayDebugDrawIfSelectedEntitiesOptionEnabledAndEntityNotSelected)
    {
        // Given the entity is not selected and the option to only show helpers for selected entities is true
        m_viewportSettings->m_onlyShowForSelectedEntities = true;

        using ::testing::_;
        using ::testing::Return;

        // This should only be called if m_onlyShowForSelectedEntities is true
        EXPECT_CALL(*m_entityVisibleEntityDataCacheMock, IsVisibleEntitySelected(_))
            .WillOnce(Return(isEntitySelected(m_entityId)))
            .WillOnce(Return(isEntitySelected(m_entityId2)));

        // When the draw function is called
        m_editorHelpers->DisplayHelpers(
            AzFramework::ViewportInfo{ TestViewportId },
            m_cameraState,
            *m_debugDisplay,
            [](AZ::EntityId)
            {
                return true;
            });

        // Then the helper should not be drawn
        EXPECT_FALSE(m_displayEntityViewportEvent);
        EXPECT_EQ(m_drawCount, 0);
    }

    TEST_F(EditorViewportOnlyDrawHelpersForSelectedEntityFixture, DisplayDebugDrawIfSelectedEntitiesOptionEnabledAndEntityIsSelected)
    {
        // Given the entity is selected and the option to only show helpers for selected entities is true
        const AzToolsFramework::EntityIdList entityIds = { m_entityId };
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities, entityIds);

        using ::testing::_;
        using ::testing::Return;

        EXPECT_CALL(*m_entityVisibleEntityDataCacheMock, IsVisibleEntitySelected(_))
            .WillOnce(Return(isEntitySelected(m_entityId)))
            .WillOnce(Return(isEntitySelected(m_entityId2)));

        m_viewportSettings->m_onlyShowForSelectedEntities = true;

        // When the draw function is called
        m_editorHelpers->DisplayHelpers(
            AzFramework::ViewportInfo{ TestViewportId },
            m_cameraState,
            *m_debugDisplay,
            [](AZ::EntityId)
            {
                return true;
            });

        // Then the helper should be drawn
        EXPECT_TRUE(m_displayEntityViewportEvent);
        EXPECT_EQ(m_drawCount, 1);
        EXPECT_EQ(m_drawnEntity, m_entityId);
    }

    TEST_F(EditorViewportOnlyDrawHelpersForSelectedEntityFixture, DisplayDebugDrawIfSelectedEntitiesOptionEnabledAndEntitiesAreSelected)
    {
        // Given the entity is selected and the option to only show helpers for selected entities is true
        const AzToolsFramework::EntityIdList entityIds = { m_entityId, m_entityId2 };
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities, entityIds);

        m_viewportSettings->m_onlyShowForSelectedEntities = true;

        using ::testing::_;
        using ::testing::Return;

        EXPECT_CALL(*m_entityVisibleEntityDataCacheMock, IsVisibleEntitySelected(_))
            .WillOnce(Return(isEntitySelected(m_entityId)))
            .WillOnce(Return(isEntitySelected(m_entityId2)));

        // When the draw function is called
        m_editorHelpers->DisplayHelpers(
            AzFramework::ViewportInfo{ TestViewportId },
            m_cameraState,
            *m_debugDisplay,
            [](AZ::EntityId)
            {
                return true;
            });

        // Then the helper should be drawn
        EXPECT_TRUE(m_displayEntityViewportEvent);
        EXPECT_EQ(m_drawCount, 2);
        EXPECT_EQ(m_drawnEntity, m_entityId2);

    }
} // namespace UnitTest
