/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Preprocessor/Enum.h>

#include <AzFramework/PaintBrush/PaintBrushNotificationBus.h>

#include <GradientSignal/Editor/PaintableImageAssetHelper.h>
#include <TerrainRenderer/Components/TerrainMacroMaterialComponent.h>
#include <TerrainRenderer/EditorComponents/EditorTerrainMacroMaterialComponentMode.h>
#include <TerrainRenderer/TerrainMacroMaterialBus.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ComponentMode/ComponentModeDelegate.h>
#include <AzToolsFramework/Manipulators/PaintBrushManipulator.h>


namespace Terrain
{
    // Due to the EditorTerrainMacroMaterialComponent having a member where it passes itself as the type below
    // `PaintableImageAssetHelper<EditorTerrainMacroMaterialComponent, EditorTerrainMacroMaterialComponentMode>`
    // The AzTypeInfo can't be queried due to EditorImageGradientComponent still be defined
    // So define AzTypeInfo using a forward declaration
    // Then use the AZ_RTTI_NO_TYPE_INFO_DECL/AZ_RTTI_NO_TYPE_INFO_IMPL to add Rtti
    class EditorTerrainMacroMaterialComponent;
    AZ_TYPE_INFO_SPECIALIZE(EditorTerrainMacroMaterialComponent, "{24D87D5F-6845-4F1F-81DC-05B4CEBA3EF4}");

    class EditorTerrainMacroMaterialComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , protected AzToolsFramework::EditorVisibilityNotificationBus::Handler
        , protected LmbrCentral::DependencyNotificationBus::Handler
        , protected AzFramework::PaintBrushNotificationBus::Handler
        , protected TerrainMacroMaterialNotificationBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(EditorTerrainMacroMaterialComponent);
        AZ_COMPONENT_BASE(EditorTerrainMacroMaterialComponent);
        AZ_CLASS_ALLOCATOR(EditorTerrainMacroMaterialComponent, AZ::ComponentAllocator);
        AZ_RTTI_NO_TYPE_INFO_DECL();

        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Terrain";
        static constexpr const char* const s_componentName = "Terrain Macro Material";
        static constexpr const char* const s_componentDescription = "Provides a macro material for a region to the terrain renderer";
        static constexpr const char* const s_icon = "Editor/Icons/Components/TerrainMacroMaterial.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/TerrainMacroMaterial.svg";
        static constexpr const char* const s_helpUrl = "";

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& services);

        //! Component overrides ...
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        void BuildGameEntity(AZ::Entity* gameEntity) override;

        // AzToolsFramework::EditorVisibilityNotificationBus overrides ...
        void OnEntityVisibilityChanged(bool visibility) override;

        // DependencyNotificationBus overrides ...
        void OnCompositionChanged() override;
        void OnCompositionRegionChanged(const AZ::Aabb& dirtyRegion) override;

    protected:
        // TerrainMacroMaterialNotificationBus overrides ...
        void OnTerrainMacroMaterialCreated(AZ::EntityId macroMaterialEntity, const MacroMaterialData& macroMaterial) override;
        void OnTerrainMacroMaterialChanged(AZ::EntityId macroMaterialEntity, const MacroMaterialData& macroMaterial) override;
        void OnTerrainMacroMaterialDestroyed(AZ::EntityId macroMaterialEntity) override;

        bool SavePaintedData();

        // PaintBrushNotificationBus overrides
        void OnPaintModeBegin() override;
        void OnPaintModeEnd() override;
        void OnBrushStrokeBegin(const AZ::Color& color) override;
        void OnBrushStrokeEnd() override;
        void OnPaint(const AZ::Color& color, const AZ::Aabb& dirtyArea, ValueLookupFn& valueLookupFn, BlendFn& blendFn) override;
        void OnSmooth(
            const AZ::Color& color,
            const AZ::Aabb& dirtyArea,
            ValueLookupFn& valueLookupFn,
            AZStd::span<const AZ::Vector3> valuePointOffsets,
            SmoothFn& smoothFn) override;
        AZ::Color OnGetColor(const AZ::Vector3& brushCenter) const override;

        AZStd::vector<uint8_t> ConvertLinearToSrgbGamma(AZStd::span<const uint32_t> pixelBuffer) const;

        void RefreshPaintableAssetStatus();

        AZ::u32 ConfigurationChanged();

    private:
        GradientSignal::ImageCreatorUtils::
            PaintableImageAssetHelper<EditorTerrainMacroMaterialComponent, EditorTerrainMacroMaterialComponentMode>
                m_paintableMacroColorAssetHelper;

        //! Copies of the runtime component and configuration - we use these to run the full runtime logic in the Editor.
        TerrainMacroMaterialComponent m_component;
        TerrainMacroMaterialConfig m_configuration;
        bool m_visible = true;
        bool m_runtimeComponentActive = false;
    };
} // namespace Terrain
