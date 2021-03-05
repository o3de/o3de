/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
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
            float GetOpacity() const override;
            void SetOpacity(float opacity) override;
            uint8_t GetSortKey() const override;
            void SetSortKey(uint8_t sortKeys) override;
            void SetMaterialAssetId(Data::AssetId) override;
            Data::AssetId GetMaterialAssetId() const override;

            void ConfigurationChanged();
            void AttenuationAngleChanged();
            void OpacityChanged();
            void SortKeyChanged();
            void MaterialChanged();

            DecalComponentConfig m_configuration;
            DecalFeatureProcessorInterface* m_featureProcessor = nullptr;
            DecalFeatureProcessorInterface::DecalHandle m_handle;
            EntityId m_entityId;
        };
    } // namespace Render
} // AZ namespace
