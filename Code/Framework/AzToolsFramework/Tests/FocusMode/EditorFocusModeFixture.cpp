/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/FocusMode/EditorFocusModeFixture.h>

#include <AzCore/Component/TransformBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

#include <Tests/BoundsTestComponent.h>

namespace UnitTest
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

        m_containerEntityInterface = AZ::Interface<AzToolsFramework::ContainerEntityInterface>::Get();
        ASSERT_TRUE(m_containerEntityInterface != nullptr);

        m_focusModeInterface = AZ::Interface<AzToolsFramework::FocusModeInterface>::Get();
        ASSERT_TRUE(m_focusModeInterface != nullptr);

        if (auto* console = AZ::Interface<AZ::IConsole>::Get(); console != nullptr)
        {
            console->GetCvarValue("ed_enableOutlinerOverrideManagement", m_ed_enableOutlinerOverrideManagement);
            console->PerformCommand("ed_enableOutlinerOverrideManagement true");
        }

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
        AZ::SettingsRegistryInterface* registry = AZ::SettingsRegistry::Get();
        registry->Set("/O3DE/Autoexec/ConsoleCommands/ed_enableOutlinerOverrideManagement", m_ed_enableOutlinerOverrideManagement);
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

        m_entityMap[CityEntityName] = CreateEditorEntity(CityEntityName, AZ::EntityId());
        m_entityMap[StreetEntityName] = CreateEditorEntity(StreetEntityName, m_entityMap[CityEntityName]);
        m_entityMap[CarEntityName] = CreateEditorEntity(CarEntityName, m_entityMap[StreetEntityName]);
        m_entityMap[Passenger1EntityName] = CreateEditorEntity(Passenger1EntityName, m_entityMap[CarEntityName]);
        m_entityMap[SportsCarEntityName] = CreateEditorEntity(SportsCarEntityName, m_entityMap[StreetEntityName]);
        m_entityMap[Passenger2EntityName] = CreateEditorEntity(Passenger2EntityName, m_entityMap[SportsCarEntityName]);

        // Add a BoundsTestComponent to the Car entity.
        AZ::Entity* entity = AzToolsFramework::GetEntityById(m_entityMap[CarEntityName]);

        entity->Deactivate();
        entity->CreateComponent<UnitTest::BoundsTestComponent>();
        entity->Activate();

        // Move the City so that it is in view
        AZ::TransformBus::Event(m_entityMap[CityEntityName], &AZ::TransformBus::Events::SetWorldTranslation, s_worldCityEntityPosition);

        // Move the CarEntity so that it's not overlapping with the rest
        AZ::TransformBus::Event(m_entityMap[CarEntityName], &AZ::TransformBus::Events::SetWorldTranslation, s_worldCarEntityPosition);

        // Setup the camera so the entities is in view.
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

} // namespace UnitTest
