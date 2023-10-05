/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Entity/EntityOwnershipServiceBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <Prefab/PrefabTestFixture.h>

namespace UnitTest
{
    class EditorEntityContextNotificationOrderingDetector
        : private AzToolsFramework::EditorEntityContextNotificationBus::Handler
        , private AzFramework::EntityOwnershipServiceNotificationBus::Handler
    {
    public:
        void Connect();
        void Disconnect();

        // EditorEntityContextNotificationBus overrides ...
        void OnPrepareForContextReset() override;
        void OnContextReset() override;

        bool m_prepareForContextReset = false;
        bool m_contextReset = false;
    };

    void EditorEntityContextNotificationOrderingDetector::Connect()
    {
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
        AzFramework::EntityOwnershipServiceNotificationBus::Handler::BusDisconnect();
    }

    void EditorEntityContextNotificationOrderingDetector::Disconnect()
    {
        AzFramework::EntityOwnershipServiceNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
    }

    void EditorEntityContextNotificationOrderingDetector::OnPrepareForContextReset()
    {
        EXPECT_THAT(m_contextReset, ::testing::IsFalse());
        EXPECT_THAT(m_prepareForContextReset, ::testing::IsFalse());

        m_prepareForContextReset = true;
    }

    void EditorEntityContextNotificationOrderingDetector::OnContextReset()
    {
        EXPECT_THAT(m_contextReset, ::testing::IsFalse());
        EXPECT_THAT(m_prepareForContextReset, ::testing::IsTrue());

        m_contextReset = true;
    }

    class EditorEntityContextNotificationFixture : public LeakDetectionFixture
    {
    public:
        EditorEntityContextNotificationOrderingDetector m_editorEntityContextNotificationOrderingDetector;
    };

    TEST_F(EditorEntityContextNotificationFixture, EditorEntityContextNotificationsReceivedInCorrectOrder)
    {
        auto app = AZStd::make_unique<ToolsTestApplication>("DummyApplication");
        AZ::ComponentApplication::StartupParameters startupParameters;
        startupParameters.m_loadAssetCatalog = false;

        app->Start(AzFramework::Application::Descriptor(), startupParameters);

        // without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
        // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash 
        // in the unit tests.
        AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

        m_editorEntityContextNotificationOrderingDetector.Connect();

        app->Stop();
        app.reset();

        EXPECT_THAT(m_editorEntityContextNotificationOrderingDetector.m_prepareForContextReset, ::testing::IsTrue());
        EXPECT_THAT(m_editorEntityContextNotificationOrderingDetector.m_contextReset, ::testing::IsTrue());

        m_editorEntityContextNotificationOrderingDetector.Disconnect();
    }
} // namespace UnitTest
