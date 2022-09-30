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
#include <GradientSignal/Editor/EditorGradientImageCreatorRequestBus.h>
#include <GradientSignal/Editor/GradientPreviewer.h>

#include <AzToolsFramework/ComponentMode/ComponentModeDelegate.h>
#include <AzToolsFramework/Manipulators/PaintBrushManipulator.h>
#include <AzToolsFramework/PaintBrush/PaintBrushNotificationBus.h>

#include <Editor/EditorImageGradientRequestBus.h>

namespace GradientSignal
{
    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(ImageGradientAutoSaveMode, uint8_t,
        (SaveAs, 0),
        (AutoSave, 1),
        (AutoSaveWithIncrementalNames, 2)
    );

    // This class inherits from EditorComponentBase instead of EditorGradientComponentBase / EditorWrappedComponentBase so that
    // we can have control over where the Editor-specific parameters for image creation and editing appear in the component
    // relative to the other runtime-only settings.
    class EditorImageGradientComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , protected AzToolsFramework::EditorVisibilityNotificationBus::Handler
        , protected LmbrCentral::DependencyNotificationBus::Handler
        , private GradientImageCreatorRequestBus::Handler
        , private EditorImageGradientRequestBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(
            EditorImageGradientComponent, EditorImageGradientComponentTypeId, AzToolsFramework::Components::EditorComponentBase);
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

        //! GradientImageCreatorRequestBus overrides ...
        AZ::Vector2 GetOutputResolution() const override;
        void SetOutputResolution(const AZ::Vector2& resolution) override;
        OutputFormat GetOutputFormat() const override;
        void SetOutputFormat(OutputFormat outputFormat) override;
        AZ::IO::Path GetOutputImagePath() const override;
        void SetOutputImagePath(const AZ::IO::Path& outputImagePath) override;

    protected:
        enum class ImageCreationOrSelection : uint8_t
        {
            UseExistingImage,
            CreateNewImage
        };

        // EditorImageGradientRequestBus overrides ...
        void StartImageModification() override;
        void EndImageModification() override;
        bool SaveImage() override;

        bool GetSaveLocation(AZ::IO::Path& fullPath, AZStd::string& relativePath, ImageGradientAutoSaveMode autoSaveMode);
        void CreateImage();
        bool SaveImageInternal(
            AZ::IO::Path& fullPath, AZStd::string& relativePath,
            int imageResolutionX, int imageResolutionY, int channels, OutputFormat format, AZStd::span<const uint8_t> pixelBuffer);

        AZ::u32 RefreshCreationSelectionChoice();
        bool GetImageCreationVisibility() const;
        AZ::Crc32 GetAutoSaveVisibility() const;
        AZ::Crc32 GetImageOptionsVisibility() const;
        AZ::Crc32 GetPaintModeVisibility() const;
        bool GetImageOptionsReadOnly() const;

        bool RefreshImageAssetStatus();
        static bool ImageHasPendingJobs(const AZ::Data::AssetId& assetId);

        void RefreshComponentModeStatus();
        void EnableComponentMode();
        void DisableComponentMode();

        bool InComponentMode() const;

        AZStd::string GetImageSourcePath(const AZ::Data::AssetId& imageAssetId) const;
        AZ::IO::Path GetIncrementingAutoSavePath(const AZ::IO::Path& currentPath) const;

        ImageCreationOrSelection m_creationSelectionChoice = ImageCreationOrSelection::UseExistingImage;

        // Parameters used for creating new source image assets
        AZ::Vector2 m_outputResolution = AZ::Vector2(512.0f);
        OutputFormat m_outputFormat = OutputFormat::R32;
        ImageGradientAutoSaveMode m_autoSaveMode = ImageGradientAutoSaveMode::AutoSave;

        // Keep track of the image asset status so that we can know when it has changed.
        AZ::Data::AssetData::AssetStatus m_currentImageAssetStatus = AZ::Data::AssetData::AssetStatus::NotLoaded;
        bool m_currentImageJobsPending = false;
        bool m_waitingForImageReload = false;

        AZ::u32 ConfigurationChanged();

    private:
        //! Delegates the handling of component editing mode to a paint controller.
        using ComponentModeDelegate = AzToolsFramework::ComponentModeFramework::ComponentModeDelegate;
        ComponentModeDelegate m_componentModeDelegate;

        //! Preview of the gradient image
        GradientPreviewer m_previewer;

        //! Copies of the runtime component and configuration - we use these to run the full runtime logic in the Editor.
        ImageGradientComponent m_component;
        ImageGradientConfig m_configuration;
        bool m_visible = true;
        bool m_runtimeComponentActive = false;

        //! Track whether or not we've prompted the user for an image save location at least once since this component was created.
        //! This is intentionally not serialized so that every user is prompted at least once per editor run for autosaves. This choice
        //! prioritizes data safety over lower friction - it's too easy for autosave to overwrite data accidentally, so we want the user
        //! to specifically choose a save location at least once before overwriting without prompts.
        //! We could serialize the flag so that the user only selects a location once per component, instead of once per component per
        //! Editor run, but that serialized flag would be shared with other users, so we would have other users editing the same image
        //! that never get prompted and might overwrite data by mistake.
        bool m_promptedForSaveLocation = false;
    };
}

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(GradientSignal::ImageGradientAutoSaveMode, "{55149135-E4F5-4D43-BB05-03A898BD9EEB}");
}
