/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/FocusMode/EditorFocusModeFixture.h>

namespace AzToolsFramework
{
    void EditorFocusModeTestFixture::SetUp()
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

    void EditorFocusModeTestFixture::GenerateTestHierarchy() 
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

    AZ::EntityId EditorFocusModeTestFixture::CreateEditorEntity(const char* name, AZ::EntityId parentId)
    {
        AZ::Entity* entity = nullptr;
        UnitTest::CreateDefaultEditorEntity(name, &entity);

        // Parent
        AZ::TransformBus::Event(entity->GetId(), &AZ::TransformInterface::SetParent, parentId);

        return entity->GetId();
    }

    void EditorFocusModeTestFixture::TearDown()
    {
        m_app.Stop();

        AllocatorsTestFixture::TearDown();
    }
}
