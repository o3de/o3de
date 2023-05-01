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
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/ViewportContextManager.h>
#include <Atom/RPI.Public/ViewportContext.h>

#include "CameraComponent.h"

#include <AzCore/Math/MatrixUtils.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace Camera
{
    namespace ClassConverters
    {
        extern bool DeprecateCameraComponentWithoutEditor(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
        extern bool UpdateCameraComponentToUseController(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
    }

    CameraComponent::CameraComponent(const CameraComponentConfig& properties)
        : CameraComponentBase(properties)
    {
    }

    void CameraComponent::Activate()
    {
        CameraComponentBase::Activate();
    }

    void CameraComponent::Deactivate()
    {
        CameraComponentBase::Deactivate();
    }

    static bool UpdateGameCameraComponentToUseController(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        if (!ClassConverters::UpdateCameraComponentToUseController(context, classElement))
        {
            return false;
        }

        classElement.Convert<CameraComponent>(context);
        return true;
    }

    class CameraNotificationBus_BehaviorHandler : public CameraNotificationBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(CameraNotificationBus_BehaviorHandler, "{91E442A0-37E7-4E03-AB59-FEC11A06741D}", AZ::SystemAllocator,
            OnCameraAdded, OnCameraRemoved, OnActiveViewChanged);

        void OnCameraAdded(const AZ::EntityId& cameraId) override
        {
            Call(FN_OnCameraAdded, cameraId);
        }

        void OnCameraRemoved(const AZ::EntityId& cameraId) override
        {
            Call(FN_OnCameraRemoved, cameraId);
        }

        void OnActiveViewChanged(const AZ::EntityId& cameraId) override
        {
            Call(FN_OnCameraRemoved, cameraId);
        };
    };

    void CameraComponent::Reflect(AZ::ReflectContext* reflection)
    {
        CameraComponentBase::Reflect(reflection);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->ClassDeprecate("CameraComponent", AZ::Uuid("{A0C21E18-F759-4E72-AF26-7A36FC59E477}"), &ClassConverters::DeprecateCameraComponentWithoutEditor);
            serializeContext->ClassDeprecate("CameraComponent", AZ::Uuid("{E409F5C0-9919-4CA5-9488-1FE8A041768E}"), &UpdateGameCameraComponentToUseController);
            serializeContext->Class<CameraComponent, CameraComponentBase>()
                ->Version(0)
            ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection))
        {
            behaviorContext->EBus<CameraRequestBus>("CameraRequestBus")
                ->Event("GetNearClipDistance", &CameraRequestBus::Events::GetNearClipDistance)
                ->Event("GetFarClipDistance", &CameraRequestBus::Events::GetFarClipDistance)
                ->Event("GetFovDegrees", &CameraRequestBus::Events::GetFovDegrees)
                ->Event("SetFovDegrees", &CameraRequestBus::Events::SetFovDegrees)
                ->Event("GetFovRadians", &CameraRequestBus::Events::GetFovRadians)
                ->Event("SetFovRadians", &CameraRequestBus::Events::SetFovRadians)
                ->Event("GetFov", &CameraRequestBus::Events::GetFov) // Deprecated in 1.13
                ->Event("SetFov", &CameraRequestBus::Events::SetFov) // Deprecated in 1.13
                ->Event("SetNearClipDistance", &CameraRequestBus::Events::SetNearClipDistance)
                ->Event("SetFarClipDistance", &CameraRequestBus::Events::SetFarClipDistance)
                ->Event("MakeActiveView", &CameraRequestBus::Events::MakeActiveView)
                ->Event("IsActiveView", &CameraRequestBus::Events::IsActiveView)
                ->Event("IsOrthographic", &CameraRequestBus::Events::IsOrthographic)
                ->Event("SetOrthographic", &CameraRequestBus::Events::SetOrthographic)
                ->Event("GetOrthographicHalfWidth", &CameraRequestBus::Events::GetOrthographicHalfWidth)
                ->Event("SetOrthographicHalfWidth", &CameraRequestBus::Events::SetOrthographicHalfWidth)
                ->Event("SetXRViewQuaternion", &CameraRequestBus::Events::SetXRViewQuaternion)
                ->Event("ScreenToWorld", &CameraRequestBus::Events::ScreenToWorld)
                ->Event("ScreenNdcToWorld", &CameraRequestBus::Events::ScreenNdcToWorld)
                ->Event("WorldToScreen", &CameraRequestBus::Events::WorldToScreen)
                ->Event("WorldToScreenNdc", &CameraRequestBus::Events::WorldToScreenNdc)
                ->VirtualProperty("FieldOfView","GetFovDegrees","SetFovDegrees")
                ->VirtualProperty("NearClipDistance", "GetNearClipDistance", "SetNearClipDistance")
                ->VirtualProperty("FarClipDistance", "GetFarClipDistance", "SetFarClipDistance")
                ->VirtualProperty("Orthographic", "IsOrthographic", "SetOrthographic")
                ->VirtualProperty("OrthographicHalfWidth", "GetOrthographicHalfWidth", "SetOrthographicHalfWidth")
                ;

            behaviorContext->Class<CameraComponent>()->RequestBus("CameraRequestBus");

            behaviorContext->EBus<CameraSystemRequestBus>("CameraSystemRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Camera")
                ->Event("GetActiveCamera", &CameraSystemRequestBus::Events::GetActiveCamera)
                ;

            behaviorContext->EBus<CameraNotificationBus>("CameraNotificationBus")
                ->Attribute(AZ::Script::Attributes::Category, "Camera")
                ->Handler<CameraNotificationBus_BehaviorHandler>()
                ;
        }
    }
} //namespace Camera

