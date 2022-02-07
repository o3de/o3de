/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Math/Aabb.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <TerrainRenderer/TerrainMacroMaterialBus.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace Terrain
{
    class TerrainMacroMaterialConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(TerrainMacroMaterialConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(TerrainMacroMaterialConfig, "{9DBAFFF0-FD20-4594-8884-E3266D8CCAC8}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

        bool NormalMapAttributesAreReadOnly() const;

        AZ::Data::Asset<AZ::RPI::StreamingImageAsset> m_macroColorAsset = { AZ::Data::AssetLoadBehavior::QueueLoad };
        AZ::Data::Asset<AZ::RPI::StreamingImageAsset> m_macroNormalAsset = { AZ::Data::AssetLoadBehavior::QueueLoad };
        bool m_normalFlipX = false;
        bool m_normalFlipY = false;
        float m_normalFactor = 1.0f;
    };

    class TerrainMacroMaterialComponent
        : public AZ::Component
        , public TerrainMacroMaterialRequestBus::Handler
        , private LmbrCentral::ShapeComponentNotificationsBus::Handler
        , private AZ::Data::AssetBus::MultiHandler
    {
    public:
        template<typename, typename>
        friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(TerrainMacroMaterialComponent, "{F82379FB-E2AE-4F75-A6F4-1AE5F5DA42E8}");
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        TerrainMacroMaterialComponent(const TerrainMacroMaterialConfig& configuration);
        TerrainMacroMaterialComponent() = default;
        ~TerrainMacroMaterialComponent() = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        MacroMaterialData GetTerrainMacroMaterialData() override;

    private:
        ////////////////////////////////////////////////////////////////////////
        // ShapeComponentNotificationsBus
        void OnShapeChanged(ShapeComponentNotifications::ShapeChangeReasons reasons) override;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Data::AssetBus::Handler
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        void HandleMaterialStateChange();

        TerrainMacroMaterialConfig m_configuration;
        AZ::Aabb m_cachedShapeBounds;
        bool m_macroMaterialActive{ false };
        AZ::Data::Instance<AZ::RPI::Image> m_colorImage;
        AZ::Data::Instance<AZ::RPI::Image> m_normalImage;
    };
}
