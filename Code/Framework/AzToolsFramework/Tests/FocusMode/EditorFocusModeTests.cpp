/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/TransformBus.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>

#include <AzTest/AzTest.h>

#include <AzToolsFramework/FocusMode/FocusModeInterface.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>

namespace AzToolsFramework
{
    class EditorFocusModeTests
        : public UnitTest::AllocatorsTestFixture
    {
    protected:
        void SetUp() override
        {
            AllocatorsTestFixture::SetUp();

            m_app.Start(m_descriptor);

            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash 
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

            m_focusModeInterface = AZ::Interface<FocusModeInterface>::Get();
            ASSERT_TRUE(m_focusModeInterface != nullptr);

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

            m_entityMap[CityEntityName] =       CreateEditorEntity(CityEntityName,          AZ::EntityId());
            m_entityMap[StreetEntityName] =     CreateEditorEntity(StreetEntityName,        m_entityMap[CityEntityName]);
            m_entityMap[CarEntityName] =        CreateEditorEntity(CarEntityName,           m_entityMap[StreetEntityName]);
            m_entityMap[Passenger1EntityName] = CreateEditorEntity(Passenger1EntityName,    m_entityMap[CarEntityName]);
            m_entityMap[SportsCarEntityName] =  CreateEditorEntity(SportsCarEntityName,     m_entityMap[StreetEntityName]);
            m_entityMap[Passenger2EntityName] = CreateEditorEntity(Passenger2EntityName,    m_entityMap[SportsCarEntityName]);
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

            AllocatorsTestFixture::TearDown();
        }

        UnitTest::ToolsTestApplication m_app{ "EditorFocusModeTests" };
        AZ::ComponentApplication::Descriptor m_descriptor;
        AZStd::unordered_map<AZStd::string, AZ::EntityId> m_entityMap;

        FocusModeInterface* m_focusModeInterface = nullptr;

    public:
        inline static const char* CityEntityName = "City";
        inline static const char* StreetEntityName = "Street";
        inline static const char* CarEntityName = "Car";
        inline static const char* SportsCarEntityName = "SportsCar";
        inline static const char* Passenger1EntityName = "Passenger1";
        inline static const char* Passenger2EntityName = "Passenger2";

    };

    TEST_F(EditorFocusModeTests, EditorFocusModeTests_SetFocus)
    {
        // When an entity is set as the focus root, GetFocusRoot should return its EntityId.
        m_focusModeInterface->SetFocusRoot(m_entityMap[CarEntityName]);
        EXPECT_EQ(m_focusModeInterface->GetFocusRoot(), m_entityMap[CarEntityName]);

        // Calling ClearFocusRoot restores the default focus root (which is an invalid EntityId).
        m_focusModeInterface->ClearFocusRoot();
        EXPECT_EQ(m_focusModeInterface->GetFocusRoot(), AZ::EntityId());
    }

    TEST_F(EditorFocusModeTests, EditorFocusModeTests_IsInFocusSubTree_AncestorsDescendants)
    {
        // When the focus is set to an entity, all its descendants are in the focus subtree while the ancestors aren't.
        {
            m_focusModeInterface->SetFocusRoot(m_entityMap[StreetEntityName]);

            EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[CityEntityName]), false);
            EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[StreetEntityName]), true);
            EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[CarEntityName]), true);
            EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[Passenger1EntityName]), true);
            EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[SportsCarEntityName]), true);
            EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[Passenger2EntityName]), true);
        }

        // Restore default expected focus.
        m_focusModeInterface->ClearFocusRoot();
    }

    TEST_F(EditorFocusModeTests, EditorFocusModeTests_IsInFocusSubTree_Siblings)
    {
        // If the root entity has siblings, they are also outside of the focus subtree.
        {
            m_focusModeInterface->SetFocusRoot(m_entityMap[CarEntityName]);

            EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[CityEntityName]), false);
            EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[StreetEntityName]), false);
            EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[CarEntityName]), true);
            EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[Passenger1EntityName]), true);
            EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[SportsCarEntityName]), false);
            EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[Passenger2EntityName]), false);
        }

        // Restore default expected focus.
        m_focusModeInterface->ClearFocusRoot();
    }

    TEST_F(EditorFocusModeTests, EditorFocusModeTests_IsInFocusSubTree_Leaf)
    {
        // If the root is a leaf, then the focus subtree will consists of just that entity.
        {
            m_focusModeInterface->SetFocusRoot(m_entityMap[Passenger2EntityName]);

            EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[CityEntityName]), false);
            EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[StreetEntityName]), false);
            EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[CarEntityName]), false);
            EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[Passenger1EntityName]), false);
            EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[SportsCarEntityName]), false);
            EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[Passenger2EntityName]), true);
        }

        // Restore default expected focus.
        m_focusModeInterface->ClearFocusRoot();
    }

    TEST_F(EditorFocusModeTests, EditorFocusModeTests_IsInFocusSubTree_Clear)
    {
        // Change the value from the default.
        m_focusModeInterface->SetFocusRoot(m_entityMap[StreetEntityName]);

        // When the focus is cleared, the whole level is in the focus subtree; so we expect all entities to return true.
        {
            m_focusModeInterface->ClearFocusRoot();

            EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[CityEntityName]), true);
            EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[StreetEntityName]), true);
            EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[CarEntityName]), true);
            EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[Passenger1EntityName]), true);
            EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[SportsCarEntityName]), true);
            EXPECT_EQ(m_focusModeInterface->IsInFocusSubTree(m_entityMap[Passenger2EntityName]), true);
        }
    }
}
