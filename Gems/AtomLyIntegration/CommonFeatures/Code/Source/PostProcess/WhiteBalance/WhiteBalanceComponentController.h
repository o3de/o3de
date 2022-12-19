/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/PostProcess/WhiteBalance/WhiteBalanceConstants.h>
#include <Atom/Feature/PostProcess/WhiteBalance/WhiteBalanceSettingsInterface.h>
#include <Atom/Feature/PostProcess/PostProcessFeatureProcessorInterface.h>
#include <Atom/Feature/PostProcess/PostProcessSettingsInterface.h>

#include <AtomLyIntegration/CommonFeatures/PostProcess/WhiteBalance/WhiteBalanceBus.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/WhiteBalance/WhiteBalanceComponentConfig.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>


namespace AZ
{
    namespace Render
    {
        class WhiteBalanceComponentController final : public WhiteBalanceRequestBus::Handler
        {
        public:
            friend class EditorWhiteBalanceComponent;

            AZ_TYPE_INFO(AZ::Render::WhiteBalanceComponentController, "{2C27FA4A-49B0-4EF8-A2FF-1820B4B633C9}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            WhiteBalanceComponentController() = default;
            WhiteBalanceComponentController(const WhiteBalanceComponentConfig& config);

            void Activate(EntityId entityId);
            void Deactivate();
            void SetConfiguration(const WhiteBalanceComponentConfig& config);
            const WhiteBalanceComponentConfig& GetConfiguration() const;

            // Auto-gen function override declarations (functions definitions in .cpp)...
#include <Atom/Feature/ParamMacros/StartParamFunctionsOverride.inl>
#include <Atom/Feature/PostProcess/WhiteBalance/WhiteBalanceParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

        private:
            AZ_DISABLE_COPY(WhiteBalanceComponentController);

            void OnConfigChanged();

            PostProcessSettingsInterface* m_postProcessInterface = nullptr;
            WhiteBalanceSettingsInterface* m_settingsInterface = nullptr;
            WhiteBalanceComponentConfig m_configuration;
            EntityId m_entityId;
        };
    } // namespace Render
} // namespace AZ
