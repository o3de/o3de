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
#include "Camera_precompiled.h"
#include "CameraEditorSystemComponent.h"
#include "EditorCameraComponent.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

#include <IEditor.h>
#include <ViewManager.h>
#include <GameEngine.h>
#include <Objects/BaseObject.h>
#include <Include/IObjectManager.h>

#include <Cry_Geo.h>
#include <MathConversion.h>
#include <AzToolsFramework/API/EditorCameraBus.h>
#include "ViewportCameraSelectorWindow.h"

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
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ;
            }
        }
    }

    void CameraEditorSystemComponent::Activate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
        Camera::EditorCameraSystemRequestBus::Handler::BusConnect();
        Camera::CameraViewRegistrationRequestsBus::Handler::BusConnect();
    }

    void CameraEditorSystemComponent::Deactivate()
    {
        Camera::CameraViewRegistrationRequestsBus::Handler::BusDisconnect();
        Camera::EditorCameraSystemRequestBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
    }

    void CameraEditorSystemComponent::PopulateEditorGlobalContextMenu(QMenu* menu, const AZ::Vector2&, int flags)
    {
        IEditor* editor;
        AzToolsFramework::EditorRequests::Bus::BroadcastResult(editor, &AzToolsFramework::EditorRequests::GetEditor);

        CGameEngine* gameEngine = editor->GetGameEngine();
        if (!gameEngine || !gameEngine->IsLevelLoaded())
        {
            return;
        }

        if (!(flags & AzToolsFramework::EditorEvents::eECMF_HIDE_ENTITY_CREATION))
        {
            QAction* action = menu->addAction(QObject::tr("Create camera entity from view"));
            QObject::connect(action, &QAction::triggered, [this]() { CreateCameraEntityFromViewport(); });
        }
    }

    void CameraEditorSystemComponent::CreateCameraEntityFromViewport()
    {
        IEditor* editor;
        AzToolsFramework::EditorRequests::Bus::BroadcastResult(editor, &AzToolsFramework::EditorRequests::GetEditor);
        CViewport* viewport = editor->GetViewManager()->GetSelectedViewport();
        if (!viewport)
        {
            viewport = editor->GetViewManager()->GetGameViewport();
        }

        AzToolsFramework::ScopedUndoBatch undoBatch("Create Camera Entity");

        // Create new entity
        AZ::EntityId newEntityId;
        AZ::EBusAggregateResults<AZ::EntityId> cameras;
        Camera::CameraBus::BroadcastResult(cameras, &CameraBus::Events::GetCameras);
        AZStd::string newCameraName = AZStd::string::format("Camera%zu", cameras.values.size() + 1);
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(newEntityId, &AzToolsFramework::EditorEntityContextRequests::CreateNewEditorEntity, newCameraName.c_str());

        // Add CameraComponent
        AzToolsFramework::AddComponents<EditorCameraComponent>::ToEntities(newEntityId);

        // Set transform to that of the viewport, otherwise default to Identity matrix and 60 degree FOV
        const Matrix34 matrix = viewport ? viewport->GetViewTM() : Matrix34::CreateIdentity();
        const float fov = viewport ? viewport->GetFOV() : 60.0f;
        AZ::TransformBus::Event(newEntityId, &AZ::TransformInterface::SetWorldTM, LYTransformToAZTransform(matrix));
        CameraRequestBus::Event(newEntityId, &CameraComponentRequests::SetFov, AZ::RadToDeg(fov));
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

    void CameraEditorSystemComponent::NotifyRegisterViews()
    {
        RegisterViewportCameraSelectorWindow();
    }
}
