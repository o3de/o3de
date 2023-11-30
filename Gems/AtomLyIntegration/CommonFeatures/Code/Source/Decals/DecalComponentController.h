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
#include <AzCore/Component/NonUniformScaleBus.h>
#include <AtomLyIntegration/CommonFeatures/Decals/DecalBus.h>
#include <AtomLyIntegration/CommonFeatures/Decals/DecalComponentConfig.h>
#include <Atom/Feature/Decals/DecalFeatureProcessorInterface.h>

namespace AZ
{
    namespace Render
    {
        class DecalComponentController final
            : private TransformNotificationBus::Handler
            , public DecalRequestBus::Handler
        {
        public:
            friend class EditorDecalComponent;

            AZ_TYPE_INFO(AZ::Render::DecalComponentController, "{95834373-5D39-4C96-B0B2-F06E6B40B5BB}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

            DecalComponentController() = default;
            DecalComponentController(const DecalComponentConfig& config);

            void Activate(EntityId entityId);
            void Deactivate();
            void SetConfiguration(const DecalComponentConfig& config);
            const DecalComponentConfig& GetConfiguration() const;

        private:

            AZ_DISABLE_COPY(DecalComponentController);

            // TransformNotificationBus::Handler overrides ...
            void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

            // DecalRequestBus::Handler overrides ...
            float GetAttenuationAngle() const override;
            void SetAttenuationAngle(float intensity) override;
            const AZ::Vector3& GetDecalColor() const override;
            void SetDecalColor(const AZ::Vector3& color) override;
            float GetDecalColorFactor() const override;
            void SetDecalColorFactor(float colorFactor) override;
            float GetOpacity() const override;
            void SetOpacity(float opacity) override;
            float GetNormalMapOpacity() const override;
            void SetNormalMapOpacity(float opacity) override;
            uint8_t GetSortKey() const override;
            void SetSortKey(uint8_t sortKeys) override;
            void SetMaterialAssetId(Data::AssetId) override;
            Data::AssetId GetMaterialAssetId() const override;

            void ConfigurationChanged();
            void AttenuationAngleChanged();
            void DecalColorChanged();
            void DecalColorFactorChanged();
            void OpacityChanged();
            void NormalMapOpacityChanged();
            void SortKeyChanged();
            void MaterialChanged();
            void HandleNonUniformScaleChange(const AZ::Vector3& nonUniformScale);

            DecalComponentConfig m_configuration;
            DecalFeatureProcessorInterface* m_featureProcessor = nullptr;
            DecalFeatureProcessorInterface::DecalHandle m_handle;
            EntityId m_entityId;
            AZ::Vector3 m_cachedNonUniformScale = AZ::Vector3::CreateOne();

            AZ::NonUniformScaleChangedEvent::Handler m_nonUniformScaleChangedHandler
            {
                [&](const AZ::Vector3& nonUniformScale) { HandleNonUniformScaleChange(nonUniformScale); }
            };
        };
    } // namespace Render
} // AZ namespace
