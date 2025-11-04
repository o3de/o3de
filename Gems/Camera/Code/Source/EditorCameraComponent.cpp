/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>

#include "EditorCameraComponent.h"
#include "ViewportCameraSelectorWindow.h"
#include "Entity/EditorEntityHelpers.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/View.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>

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
        // Only allow our camera to activate with the component if we're currently in game mode.
        m_controller.SetShouldActivateFunction([]()
            {
                bool isInGameMode = true;
                AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
                    isInGameMode, &AzToolsFramework::EditorEntityContextRequestBus::Events::IsEditorRunningGame);
                return isInGameMode;
            });

        // Only allow our camera to move when the transform is not locked.
        m_controller.SetIsLockedFunction([this]()
            {
                bool locked = false;
                AzToolsFramework::Components::TransformComponentMessages::Bus::EventResult(
                    locked, GetEntityId(), &AzToolsFramework::Components::TransformComponentMessages::IsTransformLocked);

                return locked;
            });

        // Call base class activate, which in turn calls Activate on our controller.
        EditorCameraComponentBase::Activate();

        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
        EditorCameraViewRequestBus::Handler::BusConnect(GetEntityId());
    }

    void EditorCameraComponent::Deactivate()
    {
        EditorCameraViewRequestBus::Handler::BusDisconnect(GetEntityId());
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        EditorCameraComponentBase::Deactivate();
    }

    AZ::u32 EditorCameraComponent::OnConfigurationChanged()
    {
        bool isActiveEditorCamera = m_controller.IsActiveView();
        AZ::u32 configurationHash = EditorCameraComponentBase::OnConfigurationChanged();
        // If we were the active editor camera before, ensure we get reactivated after our controller gets disabled then re-enabled
        if (isActiveEditorCamera)
        {
            m_controller.MakeActiveView();
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
            serializeContext->ClassDeprecate("EditorCameraComponent", AZ::Uuid("{B99EFE3D-3F1D-4630-8A7B-31C70CC1F53C}"), &UpdateEditorCameraComponentToUseController);
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
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/Camera.svg")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/camera/camera/")
                    ->UIElement(AZ::Edit::UIHandlers::Button,"", "Sets the view to this camera")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorCameraComponent::OnPossessCameraButtonClicked)
                        ->Attribute(AZ::Edit::Attributes::ButtonText, &EditorCameraComponent::GetCameraViewButtonText)
                    ->UIElement(AZ::Edit::UIHandlers::Button,"", "Sets this camera to view")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorCameraComponent::OnMatchViewportClicked)
                        ->Attribute(AZ::Edit::Attributes::ButtonText,  "Match Viewport")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &EditorCameraComponent::IsActiveCamera)
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Debug")
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
                ->Event("MatchViewport", &EditorCameraViewRequests::MatchViewport)
                ->Event("IsActiveCamera", &EditorCameraViewRequests::IsActiveCamera)
                ;
        }
    }

    bool EditorCameraComponent::GetCameraState(AzFramework::CameraState& cameraState)
    {
        const CameraComponentConfig& config = m_controller.GetConfiguration();
        AZ::RPI::ViewportContextPtr viewportContext = m_controller.GetViewportContext();
        AZ::RPI::ViewPtr view = m_controller.GetView();

        if (viewportContext == nullptr || view == nullptr)
        {
            return false;
        }

        AzFramework::SetCameraTransform(cameraState, view->GetCameraTransform());

        {
            const AzFramework::WindowSize viewportSize = viewportContext->GetViewportSize();
            cameraState.m_viewportSize = AzFramework::ScreenSize(viewportSize.m_width, viewportSize.m_height);
        }

        if (config.m_orthographic)
        {
            cameraState.m_fovOrZoom = aznumeric_cast<float>(cameraState.m_viewportSize.m_width) / (config.m_orthographicHalfWidth * 2.0f);
            cameraState.m_orthographic = true;
        }
        else
        {
            cameraState.m_fovOrZoom = config.m_fov;
            cameraState.m_orthographic = false;
        }

        cameraState.m_nearClip = config.m_nearClipDistance;
        cameraState.m_farClip = config.m_farClipDistance;

        return true;
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

    AZ::Crc32 EditorCameraComponent::OnMatchViewportClicked()
    {
        if (IsActiveCamera())
        {
            AZ_Warning("EditorCameraComponent", false, "Camera %s is already active.", GetEntity()->GetName().c_str());
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        }
        AZStd::optional<AZ::Transform> transform = AZStd::nullopt;
        EditorCameraRequests::Bus::BroadcastResult(transform, &EditorCameraRequests::GetActiveCameraTransform);
        if (!transform)
        {
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        }
        AZStd::optional<float> fov = AZStd::nullopt;
        EditorCameraRequests::Bus::BroadcastResult(fov, &EditorCameraRequests::GetCameraFoV);
        if (!fov)
        {
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        }
        const AZ::EntityId entityId = GetEntityId();
        AZ::TransformBus::Event(entityId, &AZ::TransformInterface::SetWorldTM, transform.value());
        CameraRequestBus::Event(entityId, &CameraComponentRequests::SetFovRadians, fov.value());
        EditorCameraRequests::Bus::Broadcast(&EditorCameraRequests::SetViewFromEntityPerspective, entityId);
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }

    AZStd::string EditorCameraComponent::GetCameraViewButtonText() const
    {
        if (IsActiveCamera())
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

    void EditorCameraComponent::ToggleCameraAsActiveView()
    {
        OnPossessCameraButtonClicked();
    }

    void EditorCameraComponent::MatchViewport()
    {
        OnMatchViewportClicked();
    }

    bool EditorCameraComponent::IsActiveCamera() const
    {
        AZ::EntityId currentViewEntity;
        EditorCameraRequests::Bus::BroadcastResult(currentViewEntity, &EditorCameraRequests::GetCurrentViewEntityId);
        return currentViewEntity == GetEntityId();
    }

    void EditorCameraComponent::EditorDisplay(
        AzFramework::DebugDisplayRequests& debugDisplay, const AZ::Transform& world)
    {
        const CameraComponentConfig& config = m_controller.GetConfiguration();
        const float distance = config.m_farClipDistance * m_frustumViewPercentLength * 0.01f;

        float width;
        float height;

        if (config.m_orthographic)
        {
            width = config.m_orthographicHalfWidth;
            height = width / debugDisplay.GetAspectRatio();
        }
        else
        {
            const float tangent = static_cast<float>(tan(0.5f * AZ::DegToRad(config.m_fov)));
            height = distance * tangent;
            width = height * debugDisplay.GetAspectRatio();
        }

        AZ::Vector3 farPoints[4];
        farPoints[0] = AZ::Vector3( width, distance,  height);
        farPoints[1] = AZ::Vector3(-width, distance,  height);
        farPoints[2] = AZ::Vector3(-width, distance, -height);
        farPoints[3] = AZ::Vector3( width, distance, -height);

        AZ::Vector3 nearPoints[4];
        if (config.m_orthographic)
        {
            nearPoints[0] = AZ::Vector3( width, config.m_nearClipDistance,  height);
            nearPoints[1] = AZ::Vector3(-width, config.m_nearClipDistance,  height);
            nearPoints[2] = AZ::Vector3(-width, config.m_nearClipDistance, -height);
            nearPoints[3] = AZ::Vector3( width, config.m_nearClipDistance, -height);
        }
        else
        {
            nearPoints[0] = farPoints[0].GetNormalizedSafe() * config.m_nearClipDistance;
            nearPoints[1] = farPoints[1].GetNormalizedSafe() * config.m_nearClipDistance;
            nearPoints[2] = farPoints[2].GetNormalizedSafe() * config.m_nearClipDistance;
            nearPoints[3] = farPoints[3].GetNormalizedSafe() * config.m_nearClipDistance;
        }

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

