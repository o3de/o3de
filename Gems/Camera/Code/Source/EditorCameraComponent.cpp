/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "Camera_precompiled.h"
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>

#include "EditorCameraComponent.h"
#include "ViewportCameraSelectorWindow.h"

#include <MathConversion.h>
#include <IRenderer.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

namespace Camera
{
    namespace ClassConverters
    {
        extern bool UpdateCameraComponentToUseController(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
    }

    void EditorCameraComponent::Activate()
    {
        // Ensure our Editor Entity ID is up-to-date to sync camera configurations between Edit & Game mode.
        CameraComponentConfig controllerConfig = m_controller.GetConfiguration();
        controllerConfig.m_editorEntityId = GetEntityId().operator AZ::u64();
        m_controller.SetConfiguration(controllerConfig);

        // Call base class activate, which in turn calls Activate on our controller.
        EditorCameraComponentBase::Activate();

        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
        EditorCameraNotificationBus::Handler::BusConnect();
        EditorCameraViewRequestBus::Handler::BusConnect(GetEntityId());

        AZ::EntityId currentViewEntity;
        EditorCameraRequests::Bus::BroadcastResult(currentViewEntity, &EditorCameraRequests::GetCurrentViewEntityId);
        if (currentViewEntity == GetEntityId())
        {
            m_controller.ActivateAtomView();
            m_isActiveEditorCamera = true;
        }
    }

    void EditorCameraComponent::Deactivate()
    {
        if (m_isActiveEditorCamera)
        {
            m_controller.DeactivateAtomView();
            m_isActiveEditorCamera = false;
        }

        EditorCameraViewRequestBus::Handler::BusDisconnect(GetEntityId());
        EditorCameraNotificationBus::Handler::BusDisconnect();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        EditorCameraComponentBase::Deactivate();
    }

    AZ::u32 EditorCameraComponent::OnConfigurationChanged()
    {
        bool isActiveEditorCamera = m_isActiveEditorCamera;
        AZ::u32 configurationHash = EditorCameraComponentBase::OnConfigurationChanged();
        // If we were the active editor camera before, ensure we get reactivated after our controller gets disabled then re-enabled
        if (isActiveEditorCamera)
        {
            EditorCameraRequests::Bus::Broadcast(&EditorCameraRequests::SetViewFromEntityPerspective, GetEntityId());
        }
        return configurationHash;
    }

    static bool UpdateEditorCameraComponentToUseController(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        if (!ClassConverters::UpdateCameraComponentToUseController(context, classElement))
        {
            return false;
        }

        classElement.Convert<EditorCameraComponent>(context);
        return true;
    }

    void EditorCameraComponent::Reflect(AZ::ReflectContext* reflection)
    {
        EditorCameraComponentBase::Reflect(reflection);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->ClassDeprecate("EditorCameraComponent", "{B99EFE3D-3F1D-4630-8A7B-31C70CC1F53C}", &UpdateEditorCameraComponentToUseController);
            serializeContext->Class<EditorCameraComponent, EditorCameraComponentBase>()
                ->Version(0)
                ->Field("FrustumLengthPercent", &EditorCameraComponent::m_frustumViewPercentLength)
                ->Field("FrustumDrawColor", &EditorCameraComponent::m_frustumDrawColor)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<EditorCameraComponent>("Camera", "The Camera component allows an entity to be used as a camera")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Camera")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Camera.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/Camera.png")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/camera/")
                    ->UIElement(AZ::Edit::UIHandlers::Button,"", "Sets the view to this camera")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorCameraComponent::OnPossessCameraButtonClicked)
                        ->Attribute(AZ::Edit::Attributes::ButtonText, &EditorCameraComponent::GetCameraViewButtonText)
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Debug")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorCameraComponent::m_frustumViewPercentLength, "Frustum length", "Frustum length percent .01 to 100")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100.f)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " percent")
                        ->Attribute(AZ::Edit::Attributes::Step, 1.f)
                    ->DataElement(AZ::Edit::UIHandlers::Color, &EditorCameraComponent::m_frustumDrawColor, "Frustum color", "Frustum draw color RGB")
                ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection))
        {
            behaviorContext->Class<EditorCameraComponent>()->RequestBus("CameraRequestBus");

            behaviorContext->EBus<EditorCameraViewRequestBus>("EditorCameraViewRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "camera")
                ->Event("ToggleCameraAsActiveView", &EditorCameraViewRequests::ToggleCameraAsActiveView)
                ;
        }
    }

    void EditorCameraComponent::OnViewportViewEntityChanged([[maybe_unused]] const AZ::EntityId& newViewId)
    {
        if (newViewId == GetEntityId())
        {
            if (!m_isActiveEditorCamera)
            {
                m_controller.ActivateAtomView();
                m_isActiveEditorCamera = true;
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh, AzToolsFramework::PropertyModificationRefreshLevel::Refresh_AttributesAndValues);
            }
        }
        else if (m_isActiveEditorCamera)
        {
            m_controller.DeactivateAtomView();
            m_isActiveEditorCamera = false;
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh, AzToolsFramework::PropertyModificationRefreshLevel::Refresh_AttributesAndValues);
        }
    }

    AZ::Crc32 EditorCameraComponent::OnPossessCameraButtonClicked()
    {
        AZ::EntityId currentViewEntity;
        EditorCameraRequests::Bus::BroadcastResult(currentViewEntity, &EditorCameraRequests::GetCurrentViewEntityId);
        AzToolsFramework::EditorRequestBus::Broadcast(&AzToolsFramework::EditorRequestBus::Events::ShowViewPane, s_viewportCameraSelectorName);
        if (currentViewEntity != GetEntityId())
        {
            EditorCameraRequests::Bus::Broadcast(&EditorCameraRequests::SetViewFromEntityPerspective, GetEntityId());
        }
        else
        {
            // set the view entity id back to Invalid, thus enabling the editor camera
            EditorCameraRequests::Bus::Broadcast(&EditorCameraRequests::SetViewFromEntityPerspective, AZ::EntityId());
        }
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }

    AZStd::string EditorCameraComponent::GetCameraViewButtonText() const
    {
        AZ::EntityId currentViewEntity;
        EditorCameraRequests::Bus::BroadcastResult(currentViewEntity, &EditorCameraRequests::GetCurrentViewEntityId);
        if (currentViewEntity == GetEntityId())
        {
            return "Return to default editor camera";
        }
        else
        {
            return "Be this camera";
        }
    }

    void EditorCameraComponent::DisplayEntityViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, GetEntityId(), &AZ::TransformInterface::GetWorldTM);
        EditorDisplay(debugDisplay, transform);
    }

    void EditorCameraComponent::EditorDisplay(
        AzFramework::DebugDisplayRequests& debugDisplay, const AZ::Transform& world)
    {
        const CameraComponentConfig& config = m_controller.GetConfiguration();
        const float distance = config.m_farClipDistance * m_frustumViewPercentLength * 0.01f;

        float tangent = static_cast<float>(tan(0.5f * AZ::DegToRad(config.m_fov)));
        float height = distance * tangent;
        float width = height * debugDisplay.GetAspectRatio();

        AZ::Vector3 farPoints[4];
        farPoints[0] = AZ::Vector3( width, distance,  height);
        farPoints[1] = AZ::Vector3(-width, distance,  height);
        farPoints[2] = AZ::Vector3(-width, distance, -height);
        farPoints[3] = AZ::Vector3( width, distance, -height);

        AZ::Vector3 start(0, 0, 0);
        AZ::Vector3 nearPoints[4];
        nearPoints[0] = farPoints[0].GetNormalizedSafe() * config.m_nearClipDistance;
        nearPoints[1] = farPoints[1].GetNormalizedSafe() * config.m_nearClipDistance;
        nearPoints[2] = farPoints[2].GetNormalizedSafe() * config.m_nearClipDistance;
        nearPoints[3] = farPoints[3].GetNormalizedSafe() * config.m_nearClipDistance;

        debugDisplay.PushMatrix(world);
        debugDisplay.SetColor(m_frustumDrawColor.GetAsVector4());
        debugDisplay.DrawLine(nearPoints[0], farPoints[0]);
        debugDisplay.DrawLine(nearPoints[1], farPoints[1]);
        debugDisplay.DrawLine(nearPoints[2], farPoints[2]);
        debugDisplay.DrawLine(nearPoints[3], farPoints[3]);
        debugDisplay.DrawPolyLine(nearPoints, AZ_ARRAY_SIZE(nearPoints));
        debugDisplay.DrawPolyLine(farPoints, AZ_ARRAY_SIZE(farPoints));
        debugDisplay.PopMatrix();
    }
} //namespace Camera

