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

#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Vector3.h>

#include <AzFramework/Components/CameraBus.h>

#include <Atom/RPI.Public/ViewProviderBus.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/ViewGroup.h>
#include <Atom/RPI.Public/WindowContext.h>
#include <Atom/RPI.Public/WindowContextBus.h>
#include <Atom/RPI.Public/XR/XRRenderingInterface.h>

namespace AZ
{
    namespace RPI
    {
        class AuxGeomFeatureProcessorInterface;
    }

    namespace Debug
    {
        class CameraComponentConfig
            : public ComponentConfig
        {
        public:
            AZ_RTTI(CameraComponentConfig, "{F4F7512E-D744-4DD9-9AA8-2F5AB15480F6}", AZ::ComponentConfig);
            AZ_CLASS_ALLOCATOR(CameraComponentConfig, AZ::SystemAllocator);

            static void Reflect(AZ::ReflectContext* context);

            CameraComponentConfig() {}

            explicit CameraComponentConfig(AZStd::shared_ptr<RPI::WindowContext> target)
                : m_target(target)
            {}

            float m_fovY = AZ::Constants::QuarterPi;
            float m_depthNear = 0.1f;
            float m_depthFar = 100.0f;
            float m_aspectRatioOverride = 0.0f;
            AZStd::shared_ptr<RPI::WindowContext> m_target = nullptr;
        };

        class CameraComponent
            : public Component
            , public TransformNotificationBus::Handler
            , public RPI::ViewProviderBus::Handler
            , public Camera::CameraRequestBus::Handler
            , public RPI::WindowContextNotificationBus::Handler
        {
        public:
            AZ_COMPONENT(CameraComponent, "{2BAFDA24-B354-4C5C-95BE-D7254B4BD415}", AZ::Component);

            static void Reflect(AZ::ReflectContext* context);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

            CameraComponent();
            virtual ~CameraComponent() override = default;

            // AZ::Component overrides...
            void Activate() override;
            void Deactivate() override;

            // RPI::ViewProviderBus::Handler overrides...
            RPI::ViewPtr GetView() const override;
            RPI::ViewPtr GetStereoscopicView(RPI::ViewType viewType) const override;

        private:
            // AZ::Component overrides...
            bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
            bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

            // TransformNotificationBus::Handler overrides...
            void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

            // CameraRequestBus::Handler overrides...
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
            void SetXRViewQuaternion(const AZ::Quaternion& viewQuat, uint32_t xrViewIndex) override;

            void MakeActiveView() override;
            bool IsActiveView() override;
            AZ::Vector3 ScreenToWorld(const AZ::Vector2& screenPosition, float depth) override;
            AZ::Vector3 ScreenNdcToWorld(const AZ::Vector2& screenPosition, float depth) override;
            AZ::Vector2 WorldToScreen(const AZ::Vector3& worldPosition) override;
            AZ::Vector2 WorldToScreenNdc(const AZ::Vector3& worldPosition) override;
            
            // RPI::WindowContextNotificationBus overrides...
            void OnViewportResized(uint32_t width, uint32_t height) override;

            void UpdateAspectRatio();
            void UpdateViewToClipMatrix();

            // ViewGroup to help manage stereoscopic and non-stereoscopic views.
            // This will allow us to render to PC window as well as a XR device at the same time 
            AZ::RPI::ViewGroupPtr m_cameraView;

            // Stereoscopic view related data
            RPI::XRRenderingInterface* m_xrSystem = nullptr;
            AZ::u32 m_numXrViews = 0;
            AZStd::fixed_vector<AZ::Quaternion, AZ::RPI::XRMaxNumViews> m_stereoscopicViewQuats;
            // Boolean used to indicate when the stereoscopic view was updated
            bool m_stereoscopicViewUpdate = false;

            // Work around the EntityContext being detached before the camera component is deactivated.
            // Without EntityContext class can't get AuxGeomFeatureProcessor to clean up per view draw interface.
            AZ::RPI::AuxGeomFeatureProcessorInterface* m_auxGeomFeatureProcessor = nullptr;

            CameraComponentConfig m_componentConfig;
            float m_aspectRatio = 1.0f;
        };
    } // namespace Debug
} // namespace AZ
