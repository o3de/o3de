/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/PostProcess/MotionBlur/MotionBlurConstants.h>
#include <Atom/Feature/PostProcess/MotionBlur/MotionBlurSettingsInterface.h>
#include <Atom/Feature/PostProcess/PostProcessFeatureProcessorInterface.h>
#include <Atom/Feature/PostProcess/PostProcessSettingsInterface.h>

#include <AtomLyIntegration/CommonFeatures/PostProcess/MotionBlur/MotionBlurBus.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/MotionBlur/MotionBlurComponentConfig.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>


namespace AZ
{
    namespace Render
    {
        class MotionBlurComponentController final : public MotionBlurRequestBus::Handler
        {
        public:
            friend class EditorMotionBlurComponent;

            AZ_TYPE_INFO(AZ::Render::MotionBlurComponentController, "{36B8A2D0-A113-4C8D-B567-AE9F314E03F9}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            MotionBlurComponentController() = default;
            MotionBlurComponentController(const MotionBlurComponentConfig& config);

            void Activate(EntityId entityId);
            void Deactivate();
            void SetConfiguration(const MotionBlurComponentConfig& config);
            const MotionBlurComponentConfig& GetConfiguration() const;

            // Auto-gen function override declarations (functions definitions in .cpp)...
#include <Atom/Feature/ParamMacros/StartParamFunctionsOverride.inl>
#include <Atom/Feature/PostProcess/MotionBlur/MotionBlurParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

        private:
            AZ_DISABLE_COPY(MotionBlurComponentController);

            void OnConfigChanged();

            PostProcessSettingsInterface* m_postProcessInterface = nullptr;
            MotionBlurSettingsInterface* m_settingsInterface = nullptr;
            MotionBlurComponentConfig m_configuration;
            EntityId m_entityId;
        };
    } // namespace Render
} // namespace AZ
