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

#include <AtomLyIntegration/CommonFeatures/PostProcess/DisplayMapper/DisplayMapperComponentConfig.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/DisplayMapper/DisplayMapperComponentBus.h>

#include <Atom/Feature/PostProcess/PostProcessSettingsInterface.h>
#include <Atom/Feature/PostProcess/PostProcessFeatureProcessorInterface.h>

namespace AZ
{
    namespace Render
    {
        struct AcesParameterOverrides;

        class DisplayMapperComponentController final
            : DisplayMapperComponentRequestBus::Handler
        {
        public:
            friend class EditorDisplayMapperComponent;

            AZ_TYPE_INFO(AZ::Render::DisplayMapperComponentController, "{85E5AE10-68AD-462D-B389-B8748BB1A19C}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            DisplayMapperComponentController() = default;
            DisplayMapperComponentController(const DisplayMapperComponentConfig& config);

            void Activate(EntityId entityId);
            void Deactivate();
            void SetConfiguration(const DisplayMapperComponentConfig& config);
            const DisplayMapperComponentConfig& GetConfiguration() const;

            //! DisplayMapperComponentRequestBus::Handler overrides...
            void LoadPreset(OutputDeviceTransformType preset) override;
            void SetDisplayMapperOperationType(DisplayMapperOperationType displayMapperOperationType) override;
            DisplayMapperOperationType GetDisplayMapperOperationType() const override;
            void SetAcesParameterOverrides(const AcesParameterOverrides& parameterOverrides) override;
            const AcesParameterOverrides& GetAcesParameterOverrides() const override;
            void SetOverrideAcesParameters(bool value) override;
            bool GetOverrideAcesParameters() const override;
            void SetAlterSurround(bool value) override;
            bool GetAlterSurround() const override;
            void SetApplyDesaturation(bool value) override;
            bool GetApplyDesaturation() const override;
            void SetApplyCATD60toD65(bool value) override;
            bool GetApplyCATD60toD65() const override;
            void SetCinemaLimitsBlack(float value) override;
            float GetCinemaLimitsBlack() const override;
            void SetCinemaLimitsWhite(float value) override;
            float GetCinemaLimitsWhite() const override;
            void SetMinPoint(float value) override;
            float GetMinPoint() const override;
            void SetMidPoint(float value) override;
            float GetMidPoint() const override;
            void SetMaxPoint(float value) override;
            float GetMaxPoint() const override;
            void SetSurroundGamma(float value) override;
            float GetSurroundGamma() const override;
            void SetGamma(float value) override;
            float GetGamma() const override;

        private:
            AZ_DISABLE_COPY(DisplayMapperComponentController);

            void OnConfigChanged();

            PostProcessSettingsInterface* m_postProcessInterface = nullptr;
            DisplayMapperComponentConfig m_configuration;
            EntityId m_entityId;
        };
    }
}
