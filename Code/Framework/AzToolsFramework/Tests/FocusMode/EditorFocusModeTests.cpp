/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzCore/Component/TransformBus.h>
#include <AzToolsFramework/FocusMode/FocusModeInterface.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>

namespace AzToolsFramework
{
    class EditorFocusModeTests
        : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            m_app.Start(m_descriptor);

            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash 
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

            GenerateTestHierarchy();
        }

        void GenerateTestHierarchy() 
        {
            /*
            *   City
            *   |_  Street
            *       |_  Car
            *       |   |_ Passenger
            *       |_  SportsCar
            *           |_ Passenger
            */

            m_entityMap["cityId"] =         CreateEditorEntity("City",      AZ::EntityId());
            m_entityMap["streetId"] =       CreateEditorEntity("Street",    m_entityMap["cityId"]);
            m_entityMap["carId"] =          CreateEditorEntity("Car",       m_entityMap["streetId"]);
            m_entityMap["passengerId1"] =   CreateEditorEntity("Passenger", m_entityMap["carId"]);
            m_entityMap["sportsCarId"] =    CreateEditorEntity("SportsCar", m_entityMap["streetId"]);
            m_entityMap["passengerId2"] =   CreateEditorEntity("Passenger", m_entityMap["sportsCarId"]);
        }

        AZ::EntityId CreateEditorEntity(const char* name, AZ::EntityId parentId)
        {
            AZ::Entity* entity = nullptr;
            UnitTest::CreateDefaultEditorEntity(name, &entity);

            // Parent
            AZ::TransformBus::Event(entity->GetId(), &AZ::TransformInterface::SetParent, parentId);

            return entity->GetId();
        }

        void TearDown() override
        {
            m_app.Stop();
        }

        UnitTest::ToolsTestApplication m_app{ "EditorFocusModeTests" };
        AZ::ComponentApplication::Descriptor m_descriptor;
        AZStd::unordered_map<AZStd::string, AZ::EntityId> m_entityMap;
    };

    TEST_F(EditorFocusModeTests, EditorFocusModeTests_SetFocus)
    {
        FocusModeInterface* focusModeInterface = AZ::Interface<FocusModeInterface>::Get();
        EXPECT_TRUE(focusModeInterface != nullptr);

        focusModeInterface->SetFocusRoot(m_entityMap["carId"]);
        EXPECT_EQ(focusModeInterface->GetFocusRoot(), m_entityMap["carId"]);

        focusModeInterface->ClearFocusRoot();
        EXPECT_EQ(focusModeInterface->GetFocusRoot(), AZ::EntityId());
    }

    TEST_F(EditorFocusModeTests, EditorFocusModeTests_IsInFocusSubTree)
    {
        FocusModeInterface* focusModeInterface = AZ::Interface<FocusModeInterface>::Get();
        EXPECT_TRUE(focusModeInterface != nullptr);

        focusModeInterface->ClearFocusRoot();
        
        EXPECT_EQ(focusModeInterface->IsInFocusSubTree(m_entityMap["cityId"]), true);
        EXPECT_EQ(focusModeInterface->IsInFocusSubTree(m_entityMap["streetId"]), true);
        EXPECT_EQ(focusModeInterface->IsInFocusSubTree(m_entityMap["carId"]), true);
        EXPECT_EQ(focusModeInterface->IsInFocusSubTree(m_entityMap["passengerId1"]), true);
        EXPECT_EQ(focusModeInterface->IsInFocusSubTree(m_entityMap["sportsCarId"]), true);
        EXPECT_EQ(focusModeInterface->IsInFocusSubTree(m_entityMap["passengerId2"]), true);

        focusModeInterface->SetFocusRoot(m_entityMap["streetId"]);

        EXPECT_EQ(focusModeInterface->IsInFocusSubTree(m_entityMap["cityId"]), false);
        EXPECT_EQ(focusModeInterface->IsInFocusSubTree(m_entityMap["streetId"]), true);
        EXPECT_EQ(focusModeInterface->IsInFocusSubTree(m_entityMap["carId"]), true);
        EXPECT_EQ(focusModeInterface->IsInFocusSubTree(m_entityMap["passengerId1"]), true);
        EXPECT_EQ(focusModeInterface->IsInFocusSubTree(m_entityMap["sportsCarId"]), true);
        EXPECT_EQ(focusModeInterface->IsInFocusSubTree(m_entityMap["passengerId2"]), true);

        focusModeInterface->SetFocusRoot(m_entityMap["carId"]);

        EXPECT_EQ(focusModeInterface->IsInFocusSubTree(m_entityMap["cityId"]), false);
        EXPECT_EQ(focusModeInterface->IsInFocusSubTree(m_entityMap["streetId"]), false);
        EXPECT_EQ(focusModeInterface->IsInFocusSubTree(m_entityMap["carId"]), true);
        EXPECT_EQ(focusModeInterface->IsInFocusSubTree(m_entityMap["passengerId1"]), true);
        EXPECT_EQ(focusModeInterface->IsInFocusSubTree(m_entityMap["sportsCarId"]), false);
        EXPECT_EQ(focusModeInterface->IsInFocusSubTree(m_entityMap["passengerId2"]), false);

        focusModeInterface->SetFocusRoot(m_entityMap["passengerId2"]);

        EXPECT_EQ(focusModeInterface->IsInFocusSubTree(m_entityMap["cityId"]), false);
        EXPECT_EQ(focusModeInterface->IsInFocusSubTree(m_entityMap["streetId"]), false);
        EXPECT_EQ(focusModeInterface->IsInFocusSubTree(m_entityMap["carId"]), false);
        EXPECT_EQ(focusModeInterface->IsInFocusSubTree(m_entityMap["passengerId1"]), false);
        EXPECT_EQ(focusModeInterface->IsInFocusSubTree(m_entityMap["sportsCarId"]), false);
        EXPECT_EQ(focusModeInterface->IsInFocusSubTree(m_entityMap["passengerId2"]), true);

        focusModeInterface->ClearFocusRoot();
    }
}
