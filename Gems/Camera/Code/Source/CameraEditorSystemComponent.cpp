/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "CameraEditorSystemComponent.h"
#include "EditorCameraComponent.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/std/functional.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzFramework/Viewport/CameraState.h>
#include <AzFramework/Viewport/ViewportScreen.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorContextIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorMenuIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorActionUpdaterIdentifiers.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Entity/PrefabEditorEntityOwnershipInterface.h>
#include <AzToolsFramework/Prefab/PrefabFocusInterface.h>
#include <AzToolsFramework/UI/Prefab/ActionManagerIdentifiers/PrefabActionUpdaterIdentifiers.h>

#include <AzToolsFramework/API/EditorCameraBus.h>
#include "ViewportCameraSelectorWindow.h"

#include <QAction>
#include <QMenu>

namespace Camera
{
    void CameraEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CameraEditorSystemComponent, AZ::Component>()
                ->Version(1)
            ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<CameraEditorSystemComponent>(
                    "Camera Editor Commands", "Performs global camera requests")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Game")
                    ;
            }
        }
    }

    void CameraEditorSystemComponent::Activate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
        Camera::EditorCameraSystemRequestBus::Handler::BusConnect();
        Camera::CameraViewRegistrationRequestsBus::Handler::BusConnect();
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusConnect();
    }

    void CameraEditorSystemComponent::Deactivate()
    {
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusDisconnect();
        Camera::CameraViewRegistrationRequestsBus::Handler::BusDisconnect();
        Camera::EditorCameraSystemRequestBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
    }

    void CameraEditorSystemComponent::CreateCameraEntityFromViewport()
    {
        auto entityContextId = AzFramework::EntityContextId::CreateNull();
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            entityContextId, &AzToolsFramework::EditorEntityContextRequests::GetEditorEntityContextId);

        if (const auto prefabFocusInterface = AZ::Interface<AzToolsFramework::Prefab::PrefabFocusInterface>::Get();
            prefabFocusInterface && prefabFocusInterface->IsFocusedPrefabInstanceReadOnly(entityContextId))
        {
            return;
        }

        AzFramework::CameraState cameraState{};
        AZ::EBusReduceResult<bool, AZStd::logical_or<bool>> aggregator;
        Camera::EditorCameraRequestBus::BroadcastResult(
            aggregator, &Camera::EditorCameraRequestBus::Events::GetActiveCameraState, cameraState);

        AZ_Assert(aggregator.value, "Did not find active camera state");

        AzToolsFramework::ScopedUndoBatch undoBatch("Create Camera Entity");

        // Create new entity
        AZ::EntityId newEntityId;
        AZ::EBusAggregateResults<AZ::EntityId> cameras;
        Camera::CameraBus::BroadcastResult(cameras, &CameraBus::Events::GetCameras);
        AZStd::string newCameraName = AZStd::string::format("Camera%zu", cameras.values.size() + 1);
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            newEntityId, &AzToolsFramework::EditorEntityContextRequests::CreateNewEditorEntity, newCameraName.c_str());

        // Add CameraComponent
        AzToolsFramework::AddComponents<EditorCameraComponent>::ToEntities(newEntityId);

        // Set transform to that of the viewport, otherwise default to Identity matrix and 60 degree FOV
        const auto worldFromView = AzFramework::CameraTransform(cameraState);
        const auto cameraTransform = AZ::Transform::CreateFromMatrix3x3AndTranslation(
            AZ::Matrix3x3::CreateFromMatrix3x4(worldFromView), worldFromView.GetTranslation());
        AZ::TransformBus::Event(newEntityId, &AZ::TransformInterface::SetWorldTM, cameraTransform);
        CameraRequestBus::Event(newEntityId, &CameraComponentRequests::SetFov, AZ::RadToDeg(cameraState.m_fovOrZoom));
        undoBatch.MarkEntityDirty(newEntityId);
    }

    void CameraEditorSystemComponent::SetViewForEntity(const AZ::EntityId& id, AZ::RPI::ViewPtr view)
    {
        m_entityViewMap[id] = view;
    }

    AZ::RPI::ViewPtr CameraEditorSystemComponent::GetViewForEntity(const AZ::EntityId& id)
    {
        if (auto viewIndex = m_entityViewMap.find(id); viewIndex != m_entityViewMap.end())
        {
            return viewIndex->second.lock();
        }
        return {};
    }

    void CameraEditorSystemComponent::OnActionRegistrationHook()
    {
        auto actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
        if (!actionManagerInterface)
        {
            return;
        }

        // Create camera entity from view
        {
            const AZStd::string_view actionIdentifier = "o3de.action.camera.createFromView";
            AzToolsFramework::ActionProperties actionProperties;
            actionProperties.m_name = "Create camera entity from view";
            actionProperties.m_description = "Create an entity with a camera that shows the current viewport view.";
            actionProperties.m_category = "Edit";

            actionManagerInterface->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier,
                actionIdentifier,
                actionProperties,
                [this]()
                {
                    CreateCameraEntityFromViewport();
                }
            );

            actionManagerInterface->InstallEnabledStateCallback(
                actionIdentifier,
                []() -> bool
                {
                    if (const auto prefabEditorEntityOwnershipInterface =
                            AZ::Interface<AzToolsFramework::PrefabEditorEntityOwnershipInterface>::Get();
                        prefabEditorEntityOwnershipInterface && !prefabEditorEntityOwnershipInterface->IsRootPrefabAssigned())
                    {
                        return false;
                    }

                    auto entityContextId = AzFramework::EntityContextId::CreateNull();
                    AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
                        entityContextId, &AzToolsFramework::EditorEntityContextRequests::GetEditorEntityContextId);
                    if (const auto prefabFocusInterface = AZ::Interface<AzToolsFramework::Prefab::PrefabFocusInterface>::Get();
                        prefabFocusInterface && prefabFocusInterface->IsFocusedPrefabInstanceReadOnly(entityContextId))
                    {
                        return false;
                    }

                    return true;
                }
            );

            // Trigger update whenever entity selection changes.
            actionManagerInterface->AddActionToUpdater(EditorIdentifiers::LevelLoadedUpdaterIdentifier, actionIdentifier);
            actionManagerInterface->AddActionToUpdater(PrefabIdentifiers::PrefabFocusChangedUpdaterIdentifier, actionIdentifier);

            // This action is only accessible outside of Component Modes
            actionManagerInterface->AssignModeToAction(AzToolsFramework::DefaultActionContextModeIdentifier, actionIdentifier);
        }
    }

    void CameraEditorSystemComponent::OnMenuBindingHook()
    {
        auto menuManagerInterface = AZ::Interface<AzToolsFramework::MenuManagerInterface>::Get();
        if (!menuManagerInterface)
        {
            return;
        }

        menuManagerInterface->AddActionToMenu(EditorIdentifiers::ViewportContextMenuIdentifier, "o3de.action.camera.createFromView", 60100);
    }

    void CameraEditorSystemComponent::NotifyRegisterViews()
    {
        RegisterViewportCameraSelectorWindow();
    }
}
