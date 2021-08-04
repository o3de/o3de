/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CameraSystemComponent.h"

#include <AzCore/Math/MatrixUtils.h>
#include <AzCore/Component/TransformBus.h>

#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/ViewportContextBus.h>

namespace Camera
{
    void CameraSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
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
        CameraSystemRequestBus::Handler::BusDisconnect();
        ActiveCameraRequestBus::Handler::BusDisconnect();
        CameraNotificationBus::Handler::BusDisconnect();
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
                if (auto view = viewSystem->GetCurrentView(viewSystem->GetDefaultViewportContextName()))
                {
                    m_activeViewProperties.transform = view->GetCameraTransform();
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
                if (auto view = viewSystem->GetCurrentView(viewSystem->GetDefaultViewportContextName()))
                {
                    const auto& viewToClip = view->GetViewToClipMatrix();
                    cfg.m_fovRadians = AZ::GetPerspectiveMatrixFOV(viewToClip);

                    // A = f / (n - f)
                    // B = n * f / (n - f)
                    // Then...
                    //    B / A
                    // =  (n * f / (n - f)) / (f / (n - f)) 
                    // =  (n * f) / (f)
                    // = n
                    // and...
                    //    n * f / (n - f) = B
                    //    n * ((n - f) / f)^-1 = B
                    //    n * (n/f - 1)^-1 = B
                    //    (n/f - 1)^-1 = B/n
                    //    n/f - 1 = n/B
                    //    f = n/(n/B + 1)
                    const float A = viewToClip.GetElement(2, 2);
                    const float B = viewToClip.GetElement(2, 3);
                    cfg.m_nearClipDistance = B / A;
                    cfg.m_farClipDistance = cfg.m_nearClipDistance / (cfg.m_nearClipDistance / B + 1.f);

                    // NB: assumes reversed depth!
                    AZStd::swap(cfg.m_farClipDistance, cfg.m_nearClipDistance);

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
