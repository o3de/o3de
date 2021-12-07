/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Components/CameraBus.h>
#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/RPI.Public/ViewProviderBus.h>
#include <Atom/RPI.Public/AuxGeom/AuxGeomFeatureProcessorInterface.h>

namespace Camera
{
    static constexpr float DefaultFoV = 75.0f;
    static constexpr float MinFoV = std::numeric_limits<float>::epsilon();
    static constexpr float MaxFoV = AZ::RadToDeg(AZ::Constants::Pi);
    static constexpr float MinimumNearPlaneDistance = 0.001f;
    static constexpr float DefaultNearPlaneDistance = 0.2f;
    static constexpr float DefaultFarClipPlaneDistance = 1024.0f;
    static constexpr float DefaultFrustumDimension = 256.f;

    struct CameraComponentConfig final
        : public AZ::ComponentConfig
    {
        AZ_RTTI(CameraComponentConfig, "{064A5D64-8688-4188-B3DE-C80CE4BB7558}", AZ::ComponentConfig);

        static void Reflect(AZ::ReflectContext* context);

        float GetFarClipDistance() const;
        float GetNearClipDistance() const;
        AZ::EntityId GetEditorEntityId() const;

        AZ::u32 GetPerspectiveParameterVisibility() const;
        AZ::u32 GetOrthographicParameterVisibility() const;

        // Reflected members
        float m_fov = DefaultFoV;
        float m_nearClipDistance = DefaultNearPlaneDistance;
        float m_farClipDistance = DefaultFarClipPlaneDistance;
        float m_frustumWidth = DefaultFrustumDimension;
        float m_frustumHeight = DefaultFrustumDimension;
        bool m_specifyFrustumDimensions = false;
        AZ::u64 m_editorEntityId = AZ::EntityId::InvalidEntityId;
        bool m_makeActiveViewOnActivation = true;
        bool m_orthographic = false;
        float m_orthographicHalfWidth = 5.f;
    };

    class CameraComponentController
        : public CameraBus::Handler
        , public CameraRequestBus::Handler
        , public AZ::TransformNotificationBus::Handler
        , public AZ::RPI::ViewportContextNotificationBus::Handler
        , public AZ::RPI::ViewProviderBus::Handler
    {
    public:
        AZ_TYPE_INFO(CameraComponentController, "{A27A0725-8C07-4BF2-BF95-B6CB0CBD01B8}");

        CameraComponentController() = default;
        explicit CameraComponentController(const CameraComponentConfig& config);

        //! Defines a callback for determining whether this camera should push itself to the top of the Atom camera stack.
        //! Used by the Editor to disable undesirable camera changes in edit mode.
        void SetShouldActivateFunction(AZStd::function<bool()> shouldActivateFunction);

        //! Defines a callback for determining whether this camera is currently locked by its transform.
        void SetIsLockedFunction(AZStd::function<bool()> isLockedFunction);

        // Controller interface
        static void Reflect(AZ::ReflectContext* context);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        void Init();
        void Activate(AZ::EntityId entityId);
        void Deactivate();
        void SetConfiguration(const CameraComponentConfig& config);
        const CameraComponentConfig& GetConfiguration() const;
        AZ::RPI::ViewportContextPtr GetViewportContext();

        // CameraBus::Handler interface
        AZ::EntityId GetCameras() override;

        // CameraRequestBus::Handler interface
        float GetFovDegrees() override;
        float GetFovRadians() override;
        float GetNearClipDistance() override;
        float GetFarClipDistance() override;
        float GetFrustumWidth() override;
        float GetFrustumHeight() override;
        bool IsOrthographic() override;
        float GetOrthographicHalfWidth() override;
        void SetFovDegrees(float fov) override;
        void SetFovRadians(float fov) override;
        void SetNearClipDistance(float nearClipDistance) override;
        void SetFarClipDistance(float farClipDistance) override;
        void SetFrustumWidth(float width) override;
        void SetFrustumHeight(float height) override;
        void SetOrthographic(bool orthographic) override;
        void SetOrthographicHalfWidth(float halfWidth) override;

        void MakeActiveView() override;
        bool IsActiveView() override;

        // AZ::TransformNotificationBus::Handler interface
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // AZ::RPI::ViewportContextNotificationBus::Handler interface
        void OnViewportSizeChanged(AzFramework::WindowSize size) override;
        void OnViewportDefaultViewChanged(AZ::RPI::ViewPtr view) override;

        // AZ::RPI::ViewProviderBus::Handler interface
        AZ::RPI::ViewPtr GetView() const override;

    private:
        AZ_DISABLE_COPY(CameraComponentController);

        void ActivateAtomView();
        void DeactivateAtomView();
        void UpdateCamera();
        void SetupAtomAuxGeom(AZ::RPI::ViewportContextPtr viewportContext);

        CameraComponentConfig m_config;
        AZ::EntityId m_entityId;

        // Atom integration
        AZ::RPI::ViewPtr m_atomCamera;
        AZ::RPI::AuxGeomDrawPtr m_atomAuxGeom;
        AZ::Event<const AZ::Matrix4x4&>::Handler m_onViewMatrixChanged;
        bool m_updatingTransformFromEntity = false;
        bool m_isActiveView = false;

        AZStd::function<bool()> m_shouldActivateFn;
        AZStd::function<bool()> m_isLockedFn = []{ return false; };
    };
} // namespace Camera
