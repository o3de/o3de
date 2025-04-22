/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TransformBus.h>
#include <Atom/Feature/CubeMapCapture/CubeMapCaptureFeatureProcessorInterface.h>
#include <CubeMapCapture/EditorCubeMapRenderer.h>
#include <CubeMapCapture/CubeMapCaptureComponentConstants.h>

namespace AZ
{
    namespace Render
    {
        class CubeMapCaptureComponentConfig final
            : public AZ::ComponentConfig
        {
        public:
            AZ_RTTI(AZ::Render::CubeMapCaptureComponentConfig, "{3DA089D0-E0D0-4F00-B76E-EC28CFE41547}", ComponentConfig);
            AZ_CLASS_ALLOCATOR(CubeMapCaptureComponentConfig, SystemAllocator);
            static void Reflect(AZ::ReflectContext* context);

            CubeMapCaptureType m_captureType = CubeMapCaptureType::Specular;
            CubeMapSpecularQualityLevel m_specularQualityLevel = CubeMapSpecularQualityLevel::Medium;
            AZStd::string m_relativePath;
            float m_exposure = 0.0f;

            AZ::u32 OnCaptureTypeChanged();
            AZ::u32 GetSpecularQualityVisibilitySetting() const;
            AZ::u32 OnSpecularQualityChanged();
        };

        class CubeMapCaptureComponentController final
            : private TransformNotificationBus::Handler
        {
        public:
            friend class EditorCubeMapCaptureComponent;

            AZ_CLASS_ALLOCATOR(CubeMapCaptureComponentController, AZ::SystemAllocator);
            AZ_RTTI(AZ::Render::CubeMapCaptureComponentController, "{85156008-28A0-4F7B-BC16-0311682E14D7}");

            static void Reflect(AZ::ReflectContext* context);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);

            CubeMapCaptureComponentController() = default;
            CubeMapCaptureComponentController(const CubeMapCaptureComponentConfig& config);

            void Activate(AZ::EntityId entityId);
            void Deactivate();
            void SetConfiguration(const CubeMapCaptureComponentConfig& config);
            const CubeMapCaptureComponentConfig& GetConfiguration() const;

            // set the exposure to use when building the cubemap
            void SetExposure(float bakeExposure);

            // initiate the cubemap capture, invokes callback when complete
            void RenderCubeMap(RenderCubeMapCallback callback, const AZStd::string& relativePath);

        private:
            AZ_DISABLE_COPY(CubeMapCaptureComponentController);

            // TransformNotificationBus overrides ...
            void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

            CubeMapCaptureFeatureProcessorInterface* m_featureProcessor = nullptr;
            TransformInterface* m_transformInterface = nullptr;
            AZ::EntityId m_entityId;
            CubeMapCaptureComponentConfig m_configuration;
            CubeMapCaptureHandle m_handle;
        };
    } // namespace Render
} // namespace AZ
