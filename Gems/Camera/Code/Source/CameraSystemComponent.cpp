/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CameraSystemComponent.h"

#include <AzCore/Math/MatrixUtils.h>
#include <AzCore/Component/TransformBus.h>

#include <AzFramework/Viewport/CameraState.h>

#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/ViewportContextBus.h>

namespace Camera
{
    void CameraSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CameraSystemComponent, AZ::Component>()
                ->Version(1)
                ;
        }
    }

    void CameraSystemComponent::Activate()
    {
        CameraSystemRequestBus::Handler::BusConnect();
        ActiveCameraRequestBus::Handler::BusConnect();
        CameraNotificationBus::Handler::BusConnect();
    }

    void CameraSystemComponent::Deactivate()
    {
        CameraNotificationBus::Handler::BusDisconnect();
        ActiveCameraRequestBus::Handler::BusDisconnect();
        CameraSystemRequestBus::Handler::BusDisconnect();
    }

    AZ::EntityId CameraSystemComponent::GetActiveCamera()
    {
        return m_activeView;
    }

    const AZ::Transform& CameraSystemComponent::GetActiveCameraTransform()
    {
        if (m_activeView.IsValid())
        {
            AZ::TransformBus::EventResult(m_activeViewProperties.transform, m_activeView, &AZ::TransformBus::Events::GetWorldTM);
        }
        else
        {
            // In editor, invalid entity ID for the active view denotes the "default editor camera"
            // In game, this is an impossible state and if we reached here, we'll likely fail somehow...
            m_activeViewProperties.transform = AZ::Transform::CreateIdentity();

            using namespace AZ::RPI;
            if (auto viewSystem = ViewportContextRequests::Get())
            {
                if (auto viewGroup = viewSystem->GetCurrentViewGroup(viewSystem->GetDefaultViewportContextName()))
                {
                    m_activeViewProperties.transform = viewGroup->GetView()->GetCameraTransform();
                }
            }
        }

        return m_activeViewProperties.transform;
    }

    const Configuration& CameraSystemComponent::GetActiveCameraConfiguration()
    {
        if (m_activeView.IsValid())
        {
            CameraRequestBus::EventResult(m_activeViewProperties.configuration, m_activeView, &CameraRequestBus::Events::GetCameraConfiguration);
        }
        else
        {
            auto& cfg = m_activeViewProperties.configuration;
            cfg = Configuration();

            // In editor, invalid entity ID for the active view denotes the "default editor camera"
            // In game, this is an impossible state and if we reached here, we'll likely fail somehow...
            using namespace AZ::RPI;
            if (auto viewSystem = ViewportContextRequests::Get())
            {
                if (auto viewGroup = viewSystem->GetCurrentViewGroup(viewSystem->GetDefaultViewportContextName()))
                {
                    AzFramework::CameraState cam;
                    AzFramework::SetCameraClippingVolumeFromPerspectiveFovMatrixRH(cam, viewGroup->GetView()->GetViewToClipMatrix());

                    cfg.m_fovRadians = cam.m_fovOrZoom;
                    cfg.m_nearClipDistance = cam.m_nearClip;
                    cfg.m_farClipDistance = cam.m_farClip;

                    // No idea what to do here. Seems to be unused?
                    cfg.m_frustumWidth = cfg.m_frustumHeight = 1.0f;
                }
            }
        }

        return m_activeViewProperties.configuration;
    }

    void CameraSystemComponent::OnActiveViewChanged(const AZ::EntityId& activeView)
    {
        m_activeView = activeView;
    }
} // namespace Camera
