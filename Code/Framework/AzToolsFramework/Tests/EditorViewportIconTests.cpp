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
#include <AzToolsFramework/ViewportSelection/EditorVisibleEntityDataCache.h>

namespace UnitTest
{
    class EditorViewportIconFixture : public LeakDetectionFixture
    {
    public:
        inline static constexpr AzFramework::ViewportId TestViewportId = 2468;

        void SetUp() override
        {
            LeakDetectionFixture::SetUp();

            m_focusModeMock = AZStd::make_unique<::testing::NiceMock<MockFocusModeInterface>>();
            m_editorViewportIconDisplayMock = AZStd::make_unique<::testing::NiceMock<MockEditorViewportIconDisplayInterface>>();
            m_entityVisibleEntityDataCacheMock = AZStd::make_unique<::testing::NiceMock<MockEditorVisibleEntityDataCacheInterface>>();
            m_editorHelpers = AZStd::make_unique<AzToolsFramework::EditorHelpers>(m_entityVisibleEntityDataCacheMock.get());
            m_viewportSettings = AZStd::make_unique<ViewportSettingsTestImpl>();

            m_viewportSettings->Connect(TestViewportId);
            m_viewportSettings->m_helpersVisible = false;
            m_viewportSettings->m_iconsVisible = true;

            m_cameraState = AzFramework::CreateDefaultCamera(AZ::Transform::CreateIdentity(), AzFramework::ScreenSize(1024, 768));

            using ::testing::_;
            using ::testing::Return;
            ON_CALL(*m_entityVisibleEntityDataCacheMock, VisibleEntityDataCount()).WillByDefault(Return(1));
            ON_CALL(*m_entityVisibleEntityDataCacheMock, GetVisibleEntityId(_)).WillByDefault(Return(AZ::EntityId()));
            ON_CALL(*m_entityVisibleEntityDataCacheMock, IsVisibleEntityIconHidden(_)).WillByDefault(Return(false));
            ON_CALL(*m_entityVisibleEntityDataCacheMock, IsVisibleEntityVisible(_)).WillByDefault(Return(true));
            ON_CALL(*m_focusModeMock, IsInFocusSubTree(_)).WillByDefault(Return(true));
        }

        void TearDown() override
        {
            m_viewportSettings->Disconnect();
            m_viewportSettings.reset();
            m_editorHelpers.reset();
            m_entityVisibleEntityDataCacheMock.reset();
            m_editorViewportIconDisplayMock.reset();
            m_focusModeMock.reset();

            LeakDetectionFixture::TearDown();
        }

        AZStd::unique_ptr<ViewportSettingsTestImpl> m_viewportSettings;
        AZStd::unique_ptr<AzToolsFramework::EditorHelpers> m_editorHelpers;
        AZStd::unique_ptr<::testing::NiceMock<MockFocusModeInterface>> m_focusModeMock;
        AZStd::unique_ptr<::testing::NiceMock<MockEditorVisibleEntityDataCacheInterface>> m_entityVisibleEntityDataCacheMock;
        AZStd::unique_ptr<::testing::NiceMock<MockEditorViewportIconDisplayInterface>> m_editorViewportIconDisplayMock;
        AzFramework::CameraState m_cameraState;
    };

    TEST_F(EditorViewportIconFixture, ViewportIconsAreNotDisplayedWhenInBetweenCameraAndNearClipPlane)
    {
        NullDebugDisplayRequests nullDebugDisplayRequests;

        const auto insideNearClip = m_cameraState.m_nearClip * 0.5f;

        using ::testing::_;
        using ::testing::Return;
        // given
        // entity position (where icon will be drawn) is in between near clip plane and camera position
        ON_CALL(*m_entityVisibleEntityDataCacheMock, GetVisibleEntityPosition(_))
            .WillByDefault(Return(AZ::Vector3(0.0f, insideNearClip, 0.0f)));

        EXPECT_CALL(*m_editorViewportIconDisplayMock, DrawIcon(_)).Times(0);
        EXPECT_CALL(*m_editorViewportIconDisplayMock, AddIcon(_)).Times(0);
        EXPECT_CALL(*m_editorViewportIconDisplayMock, DrawIcons()).Times(1);


        // when
        m_editorHelpers->DisplayHelpers(
            AzFramework::ViewportInfo{ TestViewportId }, m_cameraState, nullDebugDisplayRequests,
            [](AZ::EntityId)
            {
                return true;
            });
    }

    TEST_F(EditorViewportIconFixture, ViewportIconsAreNotDisplayedWhenBehindCamera)
    {
        NullDebugDisplayRequests nullDebugDisplayRequests;

        using ::testing::_;
        using ::testing::Return;
        // given
        // entity position (where icon will be drawn) behind the camera position
        ON_CALL(*m_entityVisibleEntityDataCacheMock, GetVisibleEntityPosition(_)).WillByDefault(Return(AZ::Vector3(0.0f, -1.0f, 0.0f)));

        EXPECT_CALL(*m_editorViewportIconDisplayMock, DrawIcon(_)).Times(0);
        EXPECT_CALL(*m_editorViewportIconDisplayMock, AddIcon(_)).Times(0);
        EXPECT_CALL(*m_editorViewportIconDisplayMock, DrawIcons()).Times(1);

        // when
        m_editorHelpers->DisplayHelpers(
            AzFramework::ViewportInfo{ TestViewportId }, m_cameraState, nullDebugDisplayRequests,
            [](AZ::EntityId)
            {
                return true;
            });
    }
} // namespace UnitTest
