/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerComponent.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/Entity/EditorEntityContextComponent.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/UnitTest/ToolsTestApplication.h>

namespace UnitTest
{
    using namespace AzToolsFramework::Thumbnailer;

    class ThumbnailerTests
        : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            AZ::ComponentApplication::StartupParameters startupParameters;
            startupParameters.m_loadSettingsRegistry = false;
            m_app.Start(m_descriptor, startupParameters);
            // Without this, the user settings component would sometimes attempt to save
            // changes on shutdown. In some cases this would cause a crash while the unit test
            // was running, because the environment wasn't setup for it to save these settings.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

            AZStd::string entityName("test");
            AZ::EntityId testEntityId;
            AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
                testEntityId,
                &AzToolsFramework::EditorEntityContextRequestBus::Events::CreateNewEditorEntity,
                entityName.c_str());

            m_testEntity = AzToolsFramework::GetEntityById(testEntityId);

            ASSERT_TRUE(m_testEntity);

            AZ::Component* thumbnailerComponent = nullptr;
            AZ::ComponentDescriptorBus::EventResult(thumbnailerComponent, azrtti_typeid<AzToolsFramework::Thumbnailer::ThumbnailerComponent>(), &AZ::ComponentDescriptorBus::Events::CreateComponent);

            ASSERT_TRUE(thumbnailerComponent);

            if (m_testEntity->GetState() == AZ::Entity::State::Active)
            {
                m_testEntity->Deactivate();
            }

            ASSERT_TRUE(m_testEntity->AddComponent(thumbnailerComponent));

            m_testEntity->Activate();
        }

        void TearDown() override
        {
            AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
                &AzToolsFramework::EditorEntityContextRequestBus::Events::DestroyEditorEntity,
                m_testEntity->GetId());

            m_app.Stop();
        }

        ToolsTestApplication m_app{ "ThumbnailerTests" };
        AZ::ComponentApplication::Descriptor m_descriptor;

        AZ::Entity* m_testEntity = nullptr;
    };
} // namespace UnitTest
