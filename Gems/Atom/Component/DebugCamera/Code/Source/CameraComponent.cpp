/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/Component/DebugCamera/CameraComponent.h>

#include <AzCore/Component/TransformBus.h>
#include <AzCore/IO/IOUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/Entity.h>

#include <AzCore/Math/MatrixUtils.h>

#include <Atom/RHI/RHISystemInterface.h>

#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/AuxGeom/AuxGeomFeatureProcessorInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/WindowContext.h>

namespace AZ
{
    namespace Debug
    {
        void CameraComponentConfig::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<CameraComponentConfig, AZ::ComponentConfig>()
                    ->Version(1)
                    ->Field("FovY", &CameraComponentConfig::m_fovY)
                    ->Field("DepthNear", &CameraComponentConfig::m_depthNear)
                    ->Field("DepthFar", &CameraComponentConfig::m_depthFar)
                    ->Field("AspectRatioOverride", &CameraComponentConfig::m_aspectRatioOverride)
                    ;
            }
        }

        void CameraComponent::Reflect(AZ::ReflectContext* context)
        {
            CameraComponentConfig::Reflect(context);

            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<CameraComponent, AZ::Component>()
                    ->Version(1)
                    ->Field("Config", &CameraComponent::m_componentConfig)
                    ;
            }
        }

        void CameraComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        }

        void CameraComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("CameraService", 0x1dd1caa4));
        }

        void CameraComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("CameraService", 0x1dd1caa4));
            incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
        }

        void CameraComponent::Activate()
        {
            AZ::Name viewName = GetEntity() ?
                AZ::Name(AZStd::string::format("Camera View (entity: \"%s\")", GetEntity()->GetName().c_str())) :
                AZ::Name("Camera view (unknown entity)");
            m_view = RPI::View::CreateView(viewName, RPI::View::UsageCamera);
            m_auxGeomFeatureProcessor = RPI::Scene::GetFeatureProcessorForEntity<RPI::AuxGeomFeatureProcessorInterface>(GetEntityId());
            if (m_auxGeomFeatureProcessor)
            {
                m_auxGeomFeatureProcessor->GetOrCreateDrawQueueForView(m_view.get());
            }

            //Get transform at start
            Transform transform;
            TransformBus::BroadcastResult(transform, &TransformBus::Events::GetWorldTM);

            OnTransformChanged(transform, transform);

            TransformNotificationBus::Handler::BusConnect(GetEntityId());
            RPI::ViewProviderBus::Handler::BusConnect(GetEntityId());
            Camera::CameraRequestBus::Handler::BusConnect(GetEntityId());
            Camera::CameraNotificationBus::Broadcast(&Camera::CameraNotificationBus::Events::OnCameraAdded, GetEntityId());
        }

        void CameraComponent::Deactivate()
        {
            Camera::CameraNotificationBus::Broadcast(&Camera::CameraNotificationBus::Events::OnCameraRemoved, GetEntityId());
            Camera::CameraRequestBus::Handler::BusDisconnect();
            RPI::ViewProviderBus::Handler::BusDisconnect();
            TransformNotificationBus::Handler::BusDisconnect();
            RPI::WindowContextNotificationBus::Handler::BusDisconnect();

            if (m_auxGeomFeatureProcessor)
            {
                m_auxGeomFeatureProcessor->ReleaseDrawQueueForView(m_view.get());
            }
            m_view = nullptr;
            m_auxGeomFeatureProcessor = nullptr;
        }

        RPI::ViewPtr CameraComponent::GetView() const
        {
            return m_view;
        }

        bool CameraComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
        {
            auto config = azrtti_cast<const CameraComponentConfig*>(baseConfig);

            if (config != nullptr)
            {
                m_componentConfig = *config;

                if (config->m_target != nullptr)
                {
                    RPI::WindowContextNotificationBus::Handler::BusConnect(m_componentConfig.m_target->GetWindowHandle());
                }

                UpdateAspectRatio();

                return true;
            }
            return false;
        }

        bool CameraComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
        {
            auto config = azrtti_cast<CameraComponentConfig*>(outBaseConfig);

            if (config != nullptr)
            {
                *config = m_componentConfig;
                return true;
            }
            return false;
        }

        void CameraComponent::OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world)
        {
            AZ_UNUSED(local);


            m_view->SetCameraTransform(AZ::Matrix3x4::CreateFromTransform(world));

            UpdateViewToClipMatrix();
        }

        float CameraComponent::GetFovDegrees()
        {
            return RadToDeg(m_componentConfig.m_fovY);
        }

        float CameraComponent::GetFovRadians()
        {
            return m_componentConfig.m_fovY;
        }

        float CameraComponent::GetNearClipDistance() 
        {
            return m_componentConfig.m_depthNear;
        }

        float CameraComponent::GetFarClipDistance()
        {
            return m_componentConfig.m_depthFar;
        }

        float CameraComponent::GetFrustumWidth()
        {
            return m_componentConfig.m_depthFar * tanf(m_componentConfig.m_fovY / 2) * m_aspectRatio * 2;
        }

        float CameraComponent::GetFrustumHeight()
        {
            return m_componentConfig.m_depthFar * tanf(m_componentConfig.m_fovY / 2) * 2;
        }

        bool CameraComponent::IsOrthographic()
        {
            return false;
        }

        float CameraComponent::GetOrthographicHalfWidth()
        {
            return 0.0f;
        }

        void CameraComponent::SetFovDegrees(float fov)
        {
            m_componentConfig.m_fovY = DegToRad(fov);
            UpdateViewToClipMatrix();
        }

        void CameraComponent::SetFovRadians(float fov)
        {
            m_componentConfig.m_fovY = fov;
            UpdateViewToClipMatrix();
        }

        void CameraComponent::SetNearClipDistance(float nearClipDistance)
        {
            m_componentConfig.m_depthNear = nearClipDistance;
            UpdateViewToClipMatrix();
        }

        void CameraComponent::SetFarClipDistance(float farClipDistance) 
        {
            m_componentConfig.m_depthFar = farClipDistance;
            UpdateViewToClipMatrix();
        }

        void CameraComponent::SetFrustumWidth(float width)
        {
            AZ_Assert(m_componentConfig.m_depthFar > 0.f, "Depth Far has to be positive.");
            AZ_Assert(m_aspectRatio > 0.f, "Aspect ratio must be positive.");
            const float height = width / m_aspectRatio;
            m_componentConfig.m_fovY = atanf(height / 2 / m_componentConfig.m_depthFar) * 2;
            UpdateViewToClipMatrix();
        }

        void CameraComponent::SetFrustumHeight(float height)
        {
            AZ_Assert(m_componentConfig.m_depthFar > 0.f, "Depth Far has to be positive.");
            m_componentConfig.m_fovY = atanf(height / 2 / m_componentConfig.m_depthFar) * 2;
            UpdateViewToClipMatrix();
        }

        void CameraComponent::SetOrthographic([[maybe_unused]] bool orthographic)
        {
            AZ_Assert(!orthographic, "DebugCamera does not support orthographic projection");
        }

        void CameraComponent::SetOrthographicHalfWidth([[maybe_unused]] float halfWidth)
        {
            AZ_Assert(false, "DebugCamera does not support orthographic projection");
        }

        void CameraComponent::MakeActiveView() 
        {
            // do nothing
        }

        bool CameraComponent::IsActiveView()
        {
            return false;
        }

        void CameraComponent::OnViewportResized(uint32_t width, uint32_t height)
        {
            AZ_UNUSED(width);
            AZ_UNUSED(height);
            UpdateAspectRatio();
            UpdateViewToClipMatrix();
        }

        void CameraComponent::UpdateAspectRatio()
        {
            if (m_componentConfig.m_aspectRatioOverride > 0.0f)
            {
                m_aspectRatio = m_componentConfig.m_aspectRatioOverride;
            }
            else if (m_componentConfig.m_target)
            {
                const auto& viewport = m_componentConfig.m_target->GetViewport();
                if (viewport.m_maxX > 0.0f && viewport.m_maxY > 0.0f)
                {
                    m_aspectRatio = viewport.m_maxX / viewport.m_maxY;
                }
            }
        }

        void CameraComponent::UpdateViewToClipMatrix()
        {
            // Note: This is projection assumes a setup for reversed depth
            AZ::Matrix4x4 viewToClipMatrix;
            MakePerspectiveFovMatrixRH(viewToClipMatrix,
                m_componentConfig.m_fovY,
                m_aspectRatio,
                m_componentConfig.m_depthNear,
                m_componentConfig.m_depthFar, true);

            m_view->SetViewToClipMatrix(viewToClipMatrix);
        }

    } // namespace Debug
} // namespace AZ
