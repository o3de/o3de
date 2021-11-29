/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/FocusMode/EditorFocusModeFixture.h>

#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

#include <Tests/BoundsTestComponent.h>

namespace AzToolsFramework
{
    void ClearSelectedEntities()
    {
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequestBus::Events::SetSelectedEntities, AzToolsFramework::EntityIdList());
    }

    AzToolsFramework::EntityIdList EditorFocusModeFixture::GetSelectedEntities()
    {
        AzToolsFramework::EntityIdList selectedEntities;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            selectedEntities, &AzToolsFramework::ToolsApplicationRequestBus::Events::GetSelectedEntities);
        return selectedEntities;
    }

    void EditorFocusModeFixture::SetUpEditorFixtureImpl()
    {
        // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
        // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash 
        // in the unit tests.
        AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

        m_containerEntityInterface = AZ::Interface<ContainerEntityInterface>::Get();
        ASSERT_TRUE(m_containerEntityInterface != nullptr);

        m_focusModeInterface = AZ::Interface<FocusModeInterface>::Get();
        ASSERT_TRUE(m_focusModeInterface != nullptr);

        // register a simple component implementing BoundsRequestBus and EditorComponentSelectionRequestsBus
        GetApplication()->RegisterComponentDescriptor(UnitTest::BoundsTestComponent::CreateDescriptor());

        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            m_editorEntityContextId, &AzToolsFramework::EditorEntityContextRequestBus::Events::GetEditorEntityContextId);

        GenerateTestHierarchy();

        // Clear the focus, disabling focus mode
        m_focusModeInterface->ClearFocusRoot(m_editorEntityContextId);

        // Clear selection
        ClearSelectedEntities();
    }

    void EditorFocusModeFixture::TearDownEditorFixtureImpl()
    {
        // Clear Container Entity preserved open states
        m_containerEntityInterface->Clear(m_editorEntityContextId);

        // Clear the focus, disabling focus mode
        m_focusModeInterface->ClearFocusRoot(m_editorEntityContextId);

        // Clear selection
        ClearSelectedEntities();
    }

    void EditorFocusModeFixture::GenerateTestHierarchy() 
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

        // Add a BoundsTestComponent to the Car entity.
        AZ::Entity* entity = GetEntityById(m_entityMap[CarEntityName]);

        entity->Deactivate();
        entity->CreateComponent<UnitTest::BoundsTestComponent>();
        entity->Activate();

        // Move the CarEntity so it's out of the way.
        AZ::TransformBus::Event(m_entityMap[CarEntityName], &AZ::TransformBus::Events::SetWorldTranslation, WorldCarEntityPosition);

        // Setup the camera so the Car entity is in view.
        AzFramework::SetCameraTransform(
            m_cameraState,
            AZ::Transform::CreateFromQuaternionAndTranslation(
                AZ::Quaternion::CreateFromEulerAnglesDegrees(AZ::Vector3(0.0f, 0.0f, 0.0f)), CameraPosition));
    }

    AZ::EntityId EditorFocusModeFixture::CreateEditorEntity(const char* name, AZ::EntityId parentId)
    {
        AZ::Entity* entity = nullptr;
        UnitTest::CreateDefaultEditorEntity(name, &entity);

        // Parent
        AZ::TransformBus::Event(entity->GetId(), &AZ::TransformInterface::SetParent, parentId);

        return entity->GetId();
    }
}
