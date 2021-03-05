/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerComponent.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/Entity/EditorEntityContextComponent.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

namespace UnitTest
{
    using namespace AzToolsFramework::Thumbnailer;

    class ThumbnailerTests
        : public ::testing::Test
        , public TraceBusRedirector
    {
    protected:
        void SetUp() override
        {
            m_app.Start(m_descriptor);
            // Without this, the user settings component would sometimes attempt to save
            // changes on shutdown. In some cases this would cause a crash while the unit test
            // was running, because the environment wasn't setup for it to save these settings.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

            TraceBusRedirector::BusConnect();

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
            TraceBusRedirector::BusDisconnect();

            AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
                &AzToolsFramework::EditorEntityContextRequestBus::Events::DestroyEditorEntity,
                m_testEntity->GetId());

            m_app.Stop();
        }

        AzToolsFramework::ToolsApplication m_app;
        AZ::ComponentApplication::Descriptor m_descriptor;

        AZ::Entity* m_testEntity = nullptr;
    };

    TEST_F(ThumbnailerTests, ThumbnailerComponent_RegisterUnregisterContext)
    {
        constexpr const char* contextName1 = "Context1";
        constexpr const char* contextName2 = "Context2";
        constexpr int thumbnailSize1 = 128;
        constexpr int thumbnailSize2 = 256;

        auto checkHasContext = [](const char* contextName)
        {
            bool hasContext = false;
            AzToolsFramework::Thumbnailer::ThumbnailerRequestBus::BroadcastResult(hasContext, &AzToolsFramework::Thumbnailer::ThumbnailerRequests::HasContext, contextName);
            return hasContext;
        };

        EXPECT_FALSE(checkHasContext(contextName1));
        EXPECT_FALSE(checkHasContext(contextName2));

        AzToolsFramework::Thumbnailer::ThumbnailerRequestBus::Broadcast(&AzToolsFramework::Thumbnailer::ThumbnailerRequests::RegisterContext, contextName1, thumbnailSize1);

        EXPECT_TRUE(checkHasContext(contextName1));
        EXPECT_FALSE(checkHasContext(contextName2));

        AzToolsFramework::Thumbnailer::ThumbnailerRequestBus::Broadcast(&AzToolsFramework::Thumbnailer::ThumbnailerRequests::RegisterContext, contextName2, thumbnailSize2);

        EXPECT_TRUE(checkHasContext(contextName1));
        EXPECT_TRUE(checkHasContext(contextName2));

        AzToolsFramework::Thumbnailer::ThumbnailerRequestBus::Broadcast(&AzToolsFramework::Thumbnailer::ThumbnailerRequests::UnregisterContext, contextName1);

        EXPECT_FALSE(checkHasContext(contextName1));
        EXPECT_TRUE(checkHasContext(contextName2));

        AzToolsFramework::Thumbnailer::ThumbnailerRequestBus::Broadcast(&AzToolsFramework::Thumbnailer::ThumbnailerRequests::UnregisterContext, contextName2);

        EXPECT_FALSE(checkHasContext(contextName1));
        EXPECT_FALSE(checkHasContext(contextName2));
    }

    TEST_F(ThumbnailerTests, ThumbnailerComponent_Deactivate_ClearTumbnailContexts)
    {
        constexpr const char* contextName1 = "Context1";
        constexpr const char* contextName2 = "Context2";
        constexpr int thumbnailSize1 = 128;
        constexpr int thumbnailSize2 = 256;

        auto checkHasContext = [](const char* contextName)
        {
            bool hasContext = false;
            AzToolsFramework::Thumbnailer::ThumbnailerRequestBus::BroadcastResult(hasContext, &AzToolsFramework::Thumbnailer::ThumbnailerRequests::HasContext, contextName);
            return hasContext;
        };

        AzToolsFramework::Thumbnailer::ThumbnailerRequestBus::Broadcast(&AzToolsFramework::Thumbnailer::ThumbnailerRequests::RegisterContext, contextName1, thumbnailSize1);
        AzToolsFramework::Thumbnailer::ThumbnailerRequestBus::Broadcast(&AzToolsFramework::Thumbnailer::ThumbnailerRequests::RegisterContext, contextName2, thumbnailSize2);

        EXPECT_TRUE(checkHasContext(contextName1));
        EXPECT_TRUE(checkHasContext(contextName2));

        m_testEntity->Deactivate();
        m_testEntity->Activate();

        EXPECT_FALSE(checkHasContext(contextName1));
        EXPECT_FALSE(checkHasContext(contextName2));
    }

    TEST_F(ThumbnailerTests, ThumbnailerComponent_RegisterContextTwice_Assert)
    {
        constexpr const char* contextName1 = "Context1";
        constexpr int thumbnailSize1 = 128;

        AzToolsFramework::Thumbnailer::ThumbnailerRequestBus::Broadcast(&AzToolsFramework::Thumbnailer::ThumbnailerRequests::RegisterContext, contextName1, thumbnailSize1);

        AZ_TEST_START_TRACE_SUPPRESSION;
        AzToolsFramework::Thumbnailer::ThumbnailerRequestBus::Broadcast(&AzToolsFramework::Thumbnailer::ThumbnailerRequests::RegisterContext, contextName1, thumbnailSize1);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(ThumbnailerTests, ThumbnailerComponent_UnregisterUnknownContext_Assert)
    {
        AZ_TEST_START_TRACE_SUPPRESSION;
        AzToolsFramework::Thumbnailer::ThumbnailerRequestBus::Broadcast(&AzToolsFramework::Thumbnailer::ThumbnailerRequests::UnregisterContext, "ContextDoesNotExist");
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }
} // namespace UnitTest
