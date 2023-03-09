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
#include <AzFramework/PaintBrush/PaintBrush.h>
#include <AzFramework/PaintBrush/PaintBrushNotificationBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <TerrainRenderer/Components/MacroMaterialImageModification.h>
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
        AZ_CLASS_ALLOCATOR(TerrainMacroMaterialConfig, AZ::SystemAllocator);
        AZ_RTTI(TerrainMacroMaterialConfig, "{9DBAFFF0-FD20-4594-8884-E3266D8CCAC8}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

        bool NormalMapAttributesAreReadOnly() const;
        AZStd::string GetMacroColorAssetPropertyName() const;
        void SetMacroColorAssetPropertyName(const AZStd::string& macroColorAssetPropertyName);

        AZ::Data::Asset<AZ::RPI::StreamingImageAsset> m_macroColorAsset = { AZ::Data::AssetLoadBehavior::QueueLoad };
        AZ::Data::Asset<AZ::RPI::StreamingImageAsset> m_macroNormalAsset = { AZ::Data::AssetLoadBehavior::QueueLoad };
        bool m_normalFlipX = false;
        bool m_normalFlipY = false;
        float m_normalFactor = 1.0f;
        int32_t m_priority = 0;

        // Non-serialized properties

        //! Label to use for the macro color asset. This gets modified to show current asset loading/processing state.
        AZStd::string m_macroColorAssetPropertyLabel = "Color Texture";
    };

    class TerrainMacroMaterialComponent
        : public AZ::Component
        , public TerrainMacroMaterialRequestBus::Handler
        , public TerrainMacroColorModificationBus::Handler
        , private AzFramework::PaintBrushNotificationBus::Handler
        , private LmbrCentral::ShapeComponentNotificationsBus::Handler
        , private AZ::Data::AssetBus::MultiHandler
    {
    public:
        friend class EditorTerrainMacroMaterialComponent;
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

        // TerrainMacroMaterialRequestBus overrides...
        MacroMaterialData GetTerrainMacroMaterialData() override;
        AZ::RHI::Size GetMacroColorImageSize() const override;
        AZ::Vector2 GetMacroColorImagePixelsPerMeter() const override;

        // TerrainMacroColorModificationBus overrides...
        void StartMacroColorImageModification() override;
        void EndMacroColorImageModification() override;
        void GetMacroColorPixelValuesByPosition(AZStd::span<const AZ::Vector3> positions, AZStd::span<AZ::Color> outValues) const override;
        void GetMacroColorPixelIndicesForPositions(AZStd::span<const AZ::Vector3> positions, AZStd::span<PixelIndex> outIndices) const override;
        void GetMacroColorPixelValuesByPixelIndex(AZStd::span<const PixelIndex> positions, AZStd::span<AZ::Color> outValues) const override;
        void StartMacroColorPixelModifications() override;
        void EndMacroColorPixelModifications() override;
        void SetMacroColorPixelValuesByPixelIndex(AZStd::span<const PixelIndex> positions, AZStd::span<const AZ::Color> values) override;

        bool MacroColorImageIsModified() const;

    protected:
        void NotifyTerrainSystemOfColorChange(const AZ::Aabb& bounds);

        AZ::Data::Asset<AZ::RPI::StreamingImageAsset> GetMacroColorAsset() const;
        void SetMacroColorAsset(const AZ::Data::Asset<AZ::RPI::StreamingImageAsset>& asset);

        //! Get the macro color modification buffer.
        //! This is *only* exposed so that the editor component has the ability to save the buffer as an asset.
        AZStd::span<const uint32_t> GetMacroColorImageModificationBuffer() const;

        // PaintBrushNotificationBus overrides...
        // Most of the bus is overridden in the MacroMaterialImageModifier.
        // These two notifications are caught here so that we can create and destroy the MacroMaterialImageModifier.
        void OnPaintModeBegin() override;
        void OnPaintModeEnd() override;
        AZ::Color OnGetColor(const AZ::Vector3& brushCenter) const override;

        static inline constexpr uint32_t InvalidMipLevel = AZStd::numeric_limits<uint32_t>::max();

        uint32_t GetHighestLoadedMipLevel() const;
        void CreateMacroColorImageModificationBuffer();
        void DestroyMacroColorImageModificationBuffer();
        bool MacroColorModificationBufferIsActive() const;

        void UpdateMacroMaterialTexture(PixelIndex leftTopPixel, PixelIndex rightBottomPixel);

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

        //! Temporary buffer for runtime modifications of the macro color image data.
        AZStd::vector<uint32_t> m_modifiedMacroColorImageData;

        //! Track whether or not any data has been modified.
        bool m_macroColorImageIsModified = false;

        //! Logic for handling macro color image modification requests from PaintBrush instances.
        //! This is only created and active between OnPaintModeBegin / OnPaintModeEnd calls.
        AZStd::unique_ptr<MacroMaterialImageModifier> m_macroColorImageModifier;

        //! The number of active image modification sessions.
        int m_numImageModificationsActive = 0;

        //! Temporary values for tracking the range of pixels changed during pixel modifications.
        PixelIndex m_leftTopPixel;
        PixelIndex m_rightBottomPixel;
        bool m_modifyingPixels = false;
    };
}
