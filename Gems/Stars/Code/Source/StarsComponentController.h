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
#include <AzCore/Asset/AssetCommon.h>
#include <StarsComponentConfig.h>

namespace AZ::RPI
{
    class Scene;
}

namespace AZ::Render
{
    class StarsFeatureProcessor;

    class StarsComponentController final
        : private TransformNotificationBus::Handler
        , private Data::AssetBus::MultiHandler
    {
    public:
        friend class EditorStarsComponent;

        AZ_TYPE_INFO(AZ::Render::StarsComponentController, "{774F8FA2-3465-46FA-B635-DBF573230643}");
        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        StarsComponentController() = default;
        StarsComponentController(const StarsComponentConfig& config);

        void Activate(EntityId entityId);
        void Deactivate();
        void SetConfiguration(const StarsComponentConfig& config);
        const StarsComponentConfig& GetConfiguration() const;

    private:
        AZ_DISABLE_COPY(StarsComponentController);

        //! TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        //! Data::AssetBus interface
        void OnAssetReady(Data::Asset<Data::AssetData> asset) override;
        void OnAssetReloaded(Data::Asset<Data::AssetData> asset) override;

        void OnConfigChanged();
        void OnStarsAssetChanged();
        void UpdateStarsFromAsset(Data::Asset<Data::AssetData> asset);
        void RegisterFeatureProcessor(EntityId entityId);
        void UnregisterFeatureProcessor();

        StarsComponentConfig m_configuration;

        AZ::Render::StarsFeatureProcessor* m_starsFeatureProcessor = nullptr;

        struct Star
        {
            float ascension;
            float declination;
            uint8_t red;
            uint8_t green;
            uint8_t blue;
            uint8_t magnitude;
        };

        RPI::Scene* m_scene;
    };
} // AZ::Render
