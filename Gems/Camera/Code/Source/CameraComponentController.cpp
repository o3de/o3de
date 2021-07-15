/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CameraComponentController.h"
#include "CameraViewRegistrationBus.h"

#include <MathConversion.h>
#include <AzCore/Math/MatrixUtils.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/ViewportContextManager.h>
#include <Atom/RPI.Public/ViewportContext.h>

#include <AzCore/Component/EntityBus.h>

namespace Camera
{
    void CameraComponentConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CameraComponentConfig, AZ::ComponentConfig>()
                ->Version(2)
                ->Field("Field of View", &CameraComponentConfig::m_fov)
                ->Field("Near Clip Plane Distance", &CameraComponentConfig::m_nearClipDistance)
                ->Field("Far Clip Plane Distance", &CameraComponentConfig::m_farClipDistance)
                ->Field("SpecifyDimensions", &CameraComponentConfig::m_specifyFrustumDimensions)
                ->Field("FrustumWidth", &CameraComponentConfig::m_frustumWidth)
                ->Field("FrustumHeight", &CameraComponentConfig::m_frustumHeight)
                ->Field("EditorEntityId", &CameraComponentConfig::m_editorEntityId)
                ->Field("MakeActiveViewOnActivation", &CameraComponentConfig::m_makeActiveViewOnActivation)
            ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<CameraComponentConfig>("CameraComponentConfig", "Configuration for a CameraComponent or EditorCameraComponent")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CameraComponentConfig::m_makeActiveViewOnActivation,
                            "Make active camera on activation?", "If true, this camera will become the active render camera when it activates")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CameraComponentConfig::m_fov, "Field of view", "Vertical field of view in degrees")
                        ->Attribute(AZ::Edit::Attributes::Min, MIN_FOV)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " degrees")
                        ->Attribute(AZ::Edit::Attributes::Step, 1.f)
                        ->Attribute(AZ::Edit::Attributes::Max, AZ::RadToDeg(AZ::Constants::Pi) - 0.0001f)       //We assert at fovs >= Pi so set the max for this field to be just under that
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshValues", 0x28e720d4))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CameraComponentConfig::m_nearClipDistance, "Near clip distance",
                        "Distance to the near clip plane of the view Frustum")
                        ->Attribute(AZ::Edit::Attributes::Min, CAMERA_MIN_NEAR)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                        ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                        ->Attribute(AZ::Edit::Attributes::Max, &CameraComponentConfig::GetFarClipDistance)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshAttributesAndValues", 0xcbc2147c))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CameraComponentConfig::m_farClipDistance, "Far clip distance",
                        "Distance to the far clip plane of the view Frustum")
                        ->Attribute(AZ::Edit::Attributes::Min, &CameraComponentConfig::GetNearClipDistance)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                        ->Attribute(AZ::Edit::Attributes::Step, 10.f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ_CRC("RefreshAttributesAndValues", 0xcbc2147c))
                ;
            }
        }
    }

    float CameraComponentConfig::GetFarClipDistance() const
    {
        return m_farClipDistance;
    }

    float CameraComponentConfig::GetNearClipDistance() const
    {
        return m_nearClipDistance;
    }

    AZ::EntityId CameraComponentConfig::GetEditorEntityId() const
    {
        return AZ::EntityId(m_editorEntityId);
    }

    CameraComponentController::CameraComponentController(const CameraComponentConfig& config)
    {
        SetConfiguration(config);
    }

    void CameraComponentController::ActivateAtomView()
    {
        auto atomViewportRequests = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        if (atomViewportRequests)
        {
            AZ_Assert(m_atomCamera, "Attempted to activate Atom camera before component activation");

            const AZ::Name contextName = atomViewportRequests->GetDefaultViewportContextName();
            // Ensure the Atom camera is updated with our current transform state
            AZ::Transform localTransform;
            AZ::TransformBus::EventResult(localTransform, m_entityId, &AZ::TransformBus::Events::GetLocalTM);
            AZ::Transform worldTransform;
            AZ::TransformBus::EventResult(worldTransform, m_entityId, &AZ::TransformBus::Events::GetWorldTM);
            OnTransformChanged(localTransform, worldTransform);

            // Push the Atom camera after we make sure we're up-to-date with our component's transform to ensure the viewport reads the correct state
            UpdateCamera();
            atomViewportRequests->PushView(contextName, m_atomCamera);
            AZ::RPI::ViewportContextNotificationBus::Handler::BusConnect(contextName);
        }
    }

    void CameraComponentController::DeactivateAtomView()
    {
        auto atomViewportRequests = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        if (atomViewportRequests)
        {
            const AZ::Name contextName = atomViewportRequests->GetDefaultViewportContextName();
            AZ::RPI::ViewportContextNotificationBus::Handler::BusDisconnect(contextName);
            atomViewportRequests->PopView(contextName, m_atomCamera);
        }
    }

    void CameraComponentController::Reflect(AZ::ReflectContext* context)
    {
        CameraComponentConfig::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CameraComponentController>()
                ->Version(1)
                ->Field("Configuration", &CameraComponentController::m_config)
            ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<CameraComponentController>("CameraComponentController", "Controller for a CameraComponent or EditorCameraComponent")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CameraComponentController::m_config, "Configuration", "Camera Controller Configuration")
                ;
            }
        }
    }

    void CameraComponentController::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
    }

    void CameraComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("CameraService", 0x1dd1caa4));
    }

    void CameraComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("CameraService", 0x1dd1caa4));
    }

    void CameraComponentController::Init()
    {
        m_onViewMatrixChanged = AZ::Event<const AZ::Matrix4x4&>::Handler([this](const AZ::Matrix4x4&)
        {
            if (!m_updatingTransformFromEntity)
            {
                AZ::TransformBus::Event(m_entityId, &AZ::TransformInterface::SetWorldTM, m_atomCamera->GetCameraTransform());
            }
        });
    }

    void CameraComponentController::Activate(AZ::EntityId entityId)
    {
        m_entityId = entityId;

        if ((!m_viewSystem)||(!m_system))
        {
            // perform first-time init
            if (gEnv)
            {
                m_system = gEnv->pSystem;
            }
            if (m_system)
            {
                // Initialize local view.
                m_viewSystem = m_system->GetIViewSystem();
                if (!m_viewSystem)
                {
                    AZ_Error("CameraComponent", m_viewSystem != nullptr, "The CameraComponent shouldn't be used without a local view system");
                }
            }
        }

        if (m_viewSystem)
        {
            if (m_view == nullptr)
            {
                m_view = m_viewSystem->CreateView();

                AZ::Entity* entity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, m_entityId);
                m_view->LinkTo(entity);
            }
        }

        auto atomViewportRequests = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        if (atomViewportRequests)
        {
            const AZ::EntityId editorEntityId = m_config.GetEditorEntityId();

            // Lazily create our camera as part of Activate
            // This will be persisted as part of our config so that it may be shared between the Editor & Game components
            if (m_atomCamera == nullptr && editorEntityId.IsValid())
            {
                CameraViewRegistrationRequestsBus::BroadcastResult(m_atomCamera, &CameraViewRegistrationRequests::GetViewForEntity, editorEntityId);
            }

            // If there wasn't already a view registered (or the registration system isn't available), create a View
            if (m_atomCamera == nullptr)
            {
                AZStd::string entityName;
                AZ::ComponentApplicationBus::BroadcastResult(entityName, &AZ::ComponentApplicationBus::Events::GetEntityName, m_entityId);
                AZ::Name cameraName = AZ::Name(AZStd::string::format("%s View", entityName.c_str()));

                m_atomCamera = AZ::RPI::View::CreateView(cameraName, AZ::RPI::View::UsageFlags::UsageCamera);

                if (editorEntityId.IsValid())
                {
                    CameraViewRegistrationRequestsBus::Broadcast(&CameraViewRegistrationRequests::SetViewForEntity, editorEntityId, m_atomCamera);
                }
            }
            AZ::RPI::ViewProviderBus::Handler::BusConnect(m_entityId);

            m_atomCamera->ConnectWorldToViewMatrixChangedHandler(m_onViewMatrixChanged);
        }

        UpdateCamera();

        CameraRequestBus::Handler::BusConnect(m_entityId);
        AZ::TransformNotificationBus::Handler::BusConnect(m_entityId);
        CameraBus::Handler::BusConnect();
        CameraNotificationBus::Broadcast(&CameraNotificationBus::Events::OnCameraAdded, m_entityId);

        // Activate our camera if we're running from the launcher or Editor game mode
        // Otherwise, let the Editor keep managing the active camera
        if (m_config.m_makeActiveViewOnActivation && (!gEnv || !gEnv->IsEditor() || gEnv->IsEditorGameMode()))
        {
            MakeActiveView();
        }
    }

    void CameraComponentController::Deactivate()
    {
        CameraNotificationBus::Broadcast(&CameraNotificationBus::Events::OnCameraRemoved, m_entityId);
        CameraBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect(m_entityId);
        CameraRequestBus::Handler::BusDisconnect(m_entityId);
        if (m_viewSystem)
        {
            if (m_view != nullptr && m_viewSystem->GetViewId(m_view) != 0)
            {
                m_view->Unlink();
            }
            if (m_viewSystem->GetActiveView() == m_view)
            {
                m_viewSystem->SetActiveView(m_prevViewId);
            }
            m_viewSystem->RemoveView(m_view);
            m_prevViewId = 0;
            m_view = nullptr;
        }

        auto atomViewportRequests = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        if (atomViewportRequests)
        {
            AZ::RPI::ViewProviderBus::Handler::BusDisconnect(m_entityId);
            m_onViewMatrixChanged.Disconnect();
        }

        DeactivateAtomView();
    }

    void CameraComponentController::SetConfiguration(const CameraComponentConfig& config)
    {
        m_config = config;
        UpdateCamera();
    }

    const CameraComponentConfig& CameraComponentController::GetConfiguration() const
    {
        return m_config;
    }

    AZ::EntityId CameraComponentController::GetCameras()
    {
        return m_entityId;
    }

    float CameraComponentController::GetFovDegrees()
    {
        return m_config.m_fov;
    }

    float CameraComponentController::GetFovRadians()
    {
        return AZ::DegToRad(m_config.m_fov);
    }

    float CameraComponentController::GetNearClipDistance()
    {
        return m_config.m_nearClipDistance;
    }

    float CameraComponentController::GetFarClipDistance()
    {
        return m_config.m_farClipDistance;
    }

    float CameraComponentController::GetFrustumWidth()
    {
        return m_config.m_frustumWidth;
    }

    float CameraComponentController::GetFrustumHeight()
    {
        return m_config.m_frustumHeight;
    }

    void CameraComponentController::SetFovDegrees(float fov)
    {
        m_config.m_fov = AZ::GetClamp(fov, MinFoV, MaxFoV);
        UpdateCamera();
    }

    void CameraComponentController::SetFovRadians(float fov)
    {
        SetFovDegrees(AZ::RadToDeg(fov));
    }

    void CameraComponentController::SetNearClipDistance(float nearClipDistance)
    {
        m_config.m_nearClipDistance = AZ::GetMin(nearClipDistance, m_config.m_farClipDistance);
        UpdateCamera();
    }

    void CameraComponentController::SetFarClipDistance(float farClipDistance)
    {
        m_config.m_farClipDistance = AZ::GetMax(farClipDistance, m_config.m_nearClipDistance); 
        UpdateCamera();
    }

    void CameraComponentController::SetFrustumWidth(float width)
    {
        m_config.m_frustumWidth = width;
        UpdateCamera();
    }

    void CameraComponentController::SetFrustumHeight(float height)
    {
        m_config.m_frustumHeight = height;
        UpdateCamera();
    }

    void CameraComponentController::MakeActiveView()
    {
        // Set Legacy Cry view, if it exists
        if (m_viewSystem)
        {
            m_prevViewId = AZ::u32(m_viewSystem->GetActiveViewId());
            m_viewSystem->SetActiveView(m_view);
        }

        // Set Atom camera, if it exists
        if (m_atomCamera)
        {
            ActivateAtomView();
        }

        // Update camera parameters
        UpdateCamera();

        // Notify of active view changed
        CameraNotificationBus::Broadcast(&CameraNotificationBus::Events::OnActiveViewChanged, m_entityId);
    }

    void CameraComponentController::OnTransformChanged([[maybe_unused]] const AZ::Transform& local, const AZ::Transform& world)
    {
        if (m_updatingTransformFromEntity)
        {
            return;
        }

        if (m_view)
        {
            CCamera& camera = m_view->GetCamera();
            camera.SetMatrix(AZTransformToLYTransform(world.GetOrthogonalized()));
        }

        if (m_atomCamera)
        {
            m_updatingTransformFromEntity = true;
            m_atomCamera->SetCameraTransform(AZ::Matrix3x4::CreateFromTransform(world.GetOrthogonalized()));
            m_updatingTransformFromEntity = false;
        }
    }

    void CameraComponentController::OnViewportSizeChanged([[maybe_unused]] AzFramework::WindowSize size)
    {
        UpdateCamera();
    }

    AZ::RPI::ViewPtr CameraComponentController::GetView() const
    {
        return m_atomCamera;
    }

    void CameraComponentController::UpdateCamera()
    {
        if (m_view)
        {
            auto viewParams = *m_view->GetCurrentParams();
            viewParams.fov = AZ::DegToRad(m_config.m_fov);
            viewParams.nearplane = m_config.m_nearClipDistance;
            viewParams.farplane = m_config.m_farClipDistance;
            m_view->SetCurrentParams(viewParams);
        }

        auto atomViewportRequests = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        if (m_atomCamera && atomViewportRequests)
        {
            AZ::Matrix4x4 viewToClipMatrix;
            float aspectRatio = m_view ? m_view->GetCamera().GetPixelAspectRatio() : 1.f;
            auto viewportContext = atomViewportRequests->GetViewportContextByName(
                atomViewportRequests->GetDefaultViewportContextName());
            if (viewportContext)
            {
                if (!m_atomAuxGeom)
                {
                    SetupAtomAuxGeom(viewportContext);
                }
                auto windowSize = viewportContext->GetViewportSize();
                aspectRatio = aznumeric_cast<float>(windowSize.m_width) / aznumeric_cast<float>(windowSize.m_height);
            }

            // This assumes a reversed depth buffer, in line with other LY Atom integration
            AZ::MakePerspectiveFovMatrixRH(viewToClipMatrix,
                AZ::DegToRad(m_config.m_fov),
                aspectRatio,
                m_config.m_nearClipDistance,
                m_config.m_farClipDistance,
                true);
            m_updatingTransformFromEntity = true;
            m_atomCamera->SetViewToClipMatrix(viewToClipMatrix);
            m_updatingTransformFromEntity = false;
        }
    }

    void CameraComponentController::SetupAtomAuxGeom(AZ::RPI::ViewportContextPtr viewportContext)
    {
        if (auto scene = viewportContext->GetRenderScene())
        {
            if (auto auxGeomFP = scene->GetFeatureProcessor<AZ::RPI::AuxGeomFeatureProcessorInterface>())
            {
                m_atomAuxGeom = auxGeomFP->GetOrCreateDrawQueueForView(m_atomCamera.get());
            }
        }
    }
} //namespace Camera
