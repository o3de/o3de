/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <GradientSignal/Editor/EditorGradientComponentBase.h>
#include <GradientSignal/Components/ImageGradientComponent.h>
#include <GradientSignal/Editor/GradientPreviewer.h>
#include <GradientSignal/Editor/PaintableImageAssetHelper.h>

#include <AzFramework/PaintBrush/PaintBrushNotificationBus.h>

#include <Editor/EditorImageGradientComponentMode.h>

namespace GradientSignal
{
    class EditorImageGradientComponent;

    // Due to the EditorImageGradientComponent having a member where it passes itself as the type below
    // `PaintableImageAssetHelper<EditorImageGradientComponent, EditorImageGradientComponentMode>`
    // The AzTypeInfo can't be queried due to EditorImageGradientComponent still be defined
    // First the AzTypeInfo is defined using the forward declaration above
    // Next the `AZ_RTTI_NO_TYPE_INFO_DECL` is used to declare the RTTI member and static functions within the class
    // Finally the AZ_RTTI_NO_TYPE_INFO_IMPL is used to implement the RTTI functions in the cpp file
    AZ_TYPE_INFO_SPECIALIZE(EditorImageGradientComponent, EditorImageGradientComponentTypeId);

    // This class inherits from EditorComponentBase instead of EditorGradientComponentBase / EditorWrappedComponentBase so that
    // we can have control over where the Editor-specific parameters for image creation and editing appear in the component
    // relative to the other runtime-only settings.
    class EditorImageGradientComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , protected AzToolsFramework::EditorVisibilityNotificationBus::Handler
        , protected LmbrCentral::DependencyNotificationBus::Handler
        , private AzFramework::PaintBrushNotificationBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(EditorImageGradientComponent);
        AZ_COMPONENT_BASE(EditorImageGradientComponent);
        AZ_CLASS_ALLOCATOR(EditorImageGradientComponent, AZ::ComponentAllocator);
        AZ_RTTI_NO_TYPE_INFO_DECL();

        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Gradients";
        static constexpr const char* const s_componentName = "Image Gradient";
        static constexpr const char* const s_componentDescription = "Generates a gradient by sampling an image asset";
        static constexpr const char* const s_icon = "Editor/Icons/Components/Gradient.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/Gradient.svg";
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

        bool GetImageOptionsReadOnly() const;

        AZ::u32 ConfigurationChanged();

    private:
        ImageCreatorUtils::PaintableImageAssetHelper<EditorImageGradientComponent, EditorImageGradientComponentMode>
            m_paintableImageAssetHelper;

        //! Preview of the gradient image
        GradientPreviewer m_previewer;

        //! Copies of the runtime component and configuration - we use these to run the full runtime logic in the Editor.
        ImageGradientComponent m_component;
        ImageGradientConfig m_configuration;
        bool m_visible = true;
        bool m_runtimeComponentActive = false;
    };
}
