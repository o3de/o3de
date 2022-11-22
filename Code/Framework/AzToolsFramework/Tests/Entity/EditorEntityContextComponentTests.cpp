/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzToolsFramework/Entity/EditorEntityContextComponent.h>
#include <AzToolsFramework/UnitTest/ToolsTestApplication.h>

namespace AzToolsFramework
{
    class EditorEntityContextComponentTests
        : public UnitTest::AllocatorsTestFixture
    {
    protected:
        void SetUp() override
        {
            m_app.Start(m_descriptor);
            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash 
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);
        }
        void TearDown() override
        {
            m_app.Stop();
        }
        UnitTest::ToolsTestApplication m_app{ "EditorEntityContextComponent" }; // Shorted because Settings Registry specializations
                                                                                // are 32 characters max.
        AZ::ComponentApplication::Descriptor m_descriptor;
    };

    TEST_F(EditorEntityContextComponentTests, EditorEntityContextTests_CreateEditorEntity_CreatesValidEntity)
    {
        AZStd::string entityName("TestName");
        AZ::EntityId createdEntityId;
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            createdEntityId,
            &AzToolsFramework::EditorEntityContextRequests::CreateNewEditorEntity,
            entityName.c_str());
        EXPECT_TRUE(createdEntityId.IsValid());

        AZ::Entity* createdEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(createdEntity, &AZ::ComponentApplicationRequests::FindEntity, createdEntityId);
        EXPECT_NE(createdEntity, nullptr);
        EXPECT_EQ(entityName.compare(createdEntity->GetName()), 0);
        EXPECT_EQ(createdEntity->GetId(), createdEntityId);
    }

    TEST_F(EditorEntityContextComponentTests, EditorEntityContextTests_CreateEditorEntityWithValidId_CreatesValidEntity)
    {
        AZ::EntityId validId(AZ::Entity::MakeId());
        EXPECT_TRUE(validId.IsValid());
        AZStd::string entityName("TestName");
        AZ::EntityId createdEntityId;
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            createdEntityId,
            &AzToolsFramework::EditorEntityContextRequestBus::Events::CreateNewEditorEntityWithId,
            entityName.c_str(),
            validId);
        EXPECT_TRUE(createdEntityId.IsValid());
        EXPECT_EQ(createdEntityId, validId);

        AZ::Entity* createdEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(createdEntity, &AZ::ComponentApplicationRequests::FindEntity, createdEntityId);
        EXPECT_NE(createdEntity, nullptr);
        EXPECT_EQ(entityName.compare(createdEntity->GetName()), 0);
        EXPECT_EQ(createdEntity->GetId(), validId);
    }

    TEST_F(EditorEntityContextComponentTests, EditorEntityContextTests_CreateEditorEntityWithInvalidId_NoEntityCreated)
    {
        AZ::EntityId invalidId;
        EXPECT_FALSE(invalidId.IsValid());
        AZStd::string entityName("TestName");
        AZ::EntityId createdEntityId;
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            createdEntityId,
            &AzToolsFramework::EditorEntityContextRequestBus::Events::CreateNewEditorEntityWithId,
            entityName.c_str(),
            invalidId);
        EXPECT_FALSE(createdEntityId.IsValid());

        AZ::Entity* createdEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(createdEntity, &AZ::ComponentApplicationRequests::FindEntity, createdEntityId);
        EXPECT_EQ(createdEntity, nullptr);
    }

    TEST_F(EditorEntityContextComponentTests, EditorEntityContextTests_CreateEditorEntityWithInUseId_NoEntityCreated)
    {
        // Create an entity so we can grab an in-use entity ID.
        AZStd::string entityName("TestName");
        AZ::EntityId createdEntityId;
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            createdEntityId,
            &AzToolsFramework::EditorEntityContextRequestBus::Events::CreateNewEditorEntity,
            entityName.c_str());
        EXPECT_TRUE(createdEntityId.IsValid());

        // Attempt to create another entity with the same ID, and verify this call fails.
        AZ::EntityId secondEntityId;
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            secondEntityId,
            &AzToolsFramework::EditorEntityContextRequestBus::Events::CreateNewEditorEntityWithId,
            entityName.c_str(),
            createdEntityId);
        EXPECT_FALSE(secondEntityId.IsValid());
    }
}
