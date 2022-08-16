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

#include <Editor/EditorImageGradientRequestBus.h>

namespace GradientSignal
{
    // This class inherits from EditorWrappedComponentBase instead of EditorGradientComponentBase so that we can have control
    // over where the Editor-specific parameters for image creation and editing appear in the component relative to the other
    // runtime-only settings.
    class EditorImageGradientComponent
        : public LmbrCentral::EditorWrappedComponentBase<ImageGradientComponent, ImageGradientConfig>
        , protected LmbrCentral::DependencyNotificationBus::Handler
        , private GradientImageCreatorRequestBus::Handler
        , private EditorImageGradientRequestBus::Handler
    {
    public:
        using BaseClassType = LmbrCentral::EditorWrappedComponentBase<ImageGradientComponent, ImageGradientConfig>;
        AZ_EDITOR_COMPONENT(EditorImageGradientComponent, EditorImageGradientComponentTypeId, BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Gradients";
        static constexpr const char* const s_componentName = "Image Gradient";
        static constexpr const char* const s_componentDescription = "Generates a gradient by sampling an image asset";
        static constexpr const char* const s_icon = "Editor/Icons/Components/Gradient.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/Gradient.svg";
        static constexpr const char* const s_helpUrl = "";

        //! Component overrides ...
        void Activate() override;
        void Deactivate() override;

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


        void CreateImage();
        bool SaveImage() override;
        bool GetSaveLocation(AZ::IO::Path& fullPath, AZStd::string& relativePath);

        AZ::u32 ChangeCreationSelectionChoice();
        bool GetImageCreationVisibility() const;
        AZ::Crc32 GetPaintModeVisibility() const;

        bool RefreshImageAssetStatus();
        static bool ImageHasPendingJobs(const AZ::Data::AssetId& assetId);

        bool InComponentMode();

        ImageCreationOrSelection m_creationSelectionChoice = ImageCreationOrSelection::UseExistingImage;

        // Parameters used for creating new source image assets
        AZ::Vector2 m_outputResolution = AZ::Vector2(512.0f);
        OutputFormat m_outputFormat = OutputFormat::R32;
        AZ::IO::Path m_outputImagePath;

        // Keep track of the image asset status so that we can know when it has changed.
        AZ::Data::AssetData::AssetStatus m_currentImageAssetStatus = AZ::Data::AssetData::AssetStatus::NotLoaded;
        bool m_currentImageJobsPending = false;

        using BaseClassType::m_component;
        using BaseClassType::m_configuration;

        AZ::u32 ConfigurationChanged() override;

    private:
        //! Delegates the handling of component editing mode to a paint controller.
        using ComponentModeDelegate = AzToolsFramework::ComponentModeFramework::ComponentModeDelegate;
        ComponentModeDelegate m_componentModeDelegate;

        //! Preview of the gradient image
        GradientPreviewer m_previewer;
    };
}
