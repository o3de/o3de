/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Preprocessor/Enum.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/containers/vector.h>
#include <AzToolsFramework/ComponentMode/ComponentModeDelegate.h>

#include <GradientSignal/Util.h>
#include <GradientSignal/Editor/EditorGradientImageCreatorRequestBus.h>

namespace GradientSignal::ImageCreatorUtils
{
    //! Defines the different types of auto-save modes when editing a paintable image.
    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(PaintableImageAssetAutoSaveMode, uint8_t,
        (SaveAs, 0),
        (AutoSave, 1),
        (AutoSaveWithIncrementalNames, 2)
    );

    //! Helper class to manage all the common logic and UX for paintable image creation, editing, and saving.
    //! This is split out from PaintableImageAssetHelper so that we can minimize the amount of duplicated code caused
    //! by the templatized parameters required for hooking it up to a specific editor component mode.
    class PaintableImageAssetHelperBase
    {
    public:
        AZ_CLASS_ALLOCATOR(PaintableImageAssetHelperBase, AZ::SystemAllocator);
        AZ_TYPE_INFO(PaintableImageAssetHelperBase, "{7E269EDA-7A80-4B02-9491-96F66BEF5171}");

        static void Reflect(AZ::ReflectContext* context);

        virtual ~PaintableImageAssetHelperBase() = default;

        //! Callback that provides the default name to use when creating or saving an image.
        using DefaultSaveNameCallback = AZStd::function<AZ::IO::Path()>;

        //! Callback that gets notified when a new asset has been created.
        //! The typical use is for the parent component to replace its asset reference and refresh itself.
        using OnCreateImageCallback = AZStd::function<void(AZ::Data::Asset<AZ::Data::AssetData> createdAsset)>;

        //! Activate the PaintableImageAssetHelper.
        //! This should get called from the parent component's Activate() method.
        //! @param ownerEntityComponentIdPair The parent component's entity id and component id, used for component mode communications.
        //! @param defaultOutputFormat The image output format that should be used for image creation and saving.
        //! @param baseAssetLabel The base label to use for the image asset (ex: "Color Texture").
        //! @param defaultSaveNameCallback A callback that provides a default save name when creating or saving an image.
        //! @param onCreateImageCallback A callback that gets called whenever an image gets created or saved.
        void Activate(
            AZ::EntityComponentIdPair ownerEntityComponentIdPair,
            OutputFormat defaultOutputFormat,
            AZStd::string baseAssetLabel,
            DefaultSaveNameCallback defaultSaveNameCallback,
            OnCreateImageCallback onCreateImageCallback);

        //! Refresh the PaintableImageAssetHelper.
        //! This should get called from the parent component whenever the image asset changes its status.
        //! @param EditorComponentType The class type of the parent editor component.
        //! @param EditorComponentModeType The class type for the component mode to use when editing.
        //! @param imageAsset The image asset that can be edited.
        //! @return The refreshed asset label containing the asset status (ex: "Color Texture (not loaded)")
        AZStd::string Refresh(AZ::Data::Asset<AZ::Data::AssetData> imageAsset);

        //! Deactivate the PaintableImageAssetHelper.
        //! This should get called from the parent component's Deactivate() method.
        void Deactivate();

        //! Save a source image with the given data, optionally prompting the user for a location depending on the autosave mode.
        //! @param imageResolutionX The image width in pixels
        //! @param imageResolutionY The image height in pixels
        //! @param format The output format to save the image in
        //! @param pixelBuffer The pixel data to save
        //! @return A reference to the saved asset on success, empty on failure
        AZStd::optional<AZ::Data::Asset<AZ::Data::AssetData>> SaveImage(
            int imageResolutionX,
            int imageResolutionY,
            OutputFormat format,
            AZStd::span<const uint8_t> pixelBuffer);


    protected:
        //! Create a new image.
        void CreateNewImage();

        //! Writes out the image data.
        AZStd::optional<AZ::Data::Asset<AZ::Data::AssetData>> SaveImageInternal(
            AZ::IO::Path& fullPath,
            AZStd::string& relativePath,
            int imageResolutionX,
            int imageResolutionY,
            int channels,
            OutputFormat format,
            AZStd::span<const uint8_t> pixelBuffer);

        //! Create a new filename with an incrementing number for the "auto save with incrementing names" mode.
        AZ::IO::Path GetIncrementingAutoSavePath(const AZ::IO::Path& currentPath) const;

        //! Determine where to save the image based on the autosave mode.
        bool GetSaveLocation(AZ::IO::Path& fullPath, AZStd::string& relativePath, PaintableImageAssetAutoSaveMode autoSaveMode);

        //! Refresh the current image asset load status.
        bool RefreshImageAssetStatus(AZ::Data::Asset<AZ::Data::AssetData> imageAsset);

        //! Get a display label for the image asset that includes the current asset status.
        //! @param baseName The base name to use for the label (ex: "Image" will produce labels like "Image (not loaded)")
        //! @return The full label, based on the image asset's last refreshed status.
        AZStd::string GetImageAssetStatusLabel();

        //! Check to see if an image asset has any pending asset jobs.
        static bool ImageHasPendingJobs(const AZ::Data::AssetId& assetId);

        //! Get the relative asset path from the absolute path, or empty if a relative asset path doesn't exist.
        //! This can happen if the absolute path exists outside of the project folder.
        AZStd::string GetRelativePathFromAbsolutePath(AZStd::string_view absolutePath);

        //! Returns whether or not the edit mode should currently be visible.
        AZ::Crc32 GetPaintModeVisibility() const;

        //! Enable/disable the component mode based on the current image asset load status.
        //! Only fully-loaded image assets are editable.
        void RefreshComponentModeStatus();

        //! Enable component mode for this image asset.
        //! This is a virtual function because it needs to know the specific type of editor component and editor component mode to enable
        //! which are defined as template parameters on the PaintableImageAssetHelper.
        virtual void EnableComponentMode() = 0;

        //! Return whether or not component mode is currently active.
        bool InComponentMode() const;

        //! Disable component mode for this image asset.
        void DisableComponentMode();

        //! Delegates the handling of component editing mode to a paint controller.
        using ComponentModeDelegate = AzToolsFramework::ComponentModeFramework::ComponentModeDelegate;
        ComponentModeDelegate m_componentModeDelegate;

        // Keep track of the image asset status so that we can know when it has changed.
        AZ::Data::AssetData::AssetStatus m_currentImageAssetStatus = AZ::Data::AssetData::AssetStatus::NotLoaded;
        bool m_currentImageJobsPending = false;
        bool m_waitingForImageReload = false;

        AZStd::string m_baseAssetLabel;

        //! Offer a choice of different autosave modes.
        PaintableImageAssetAutoSaveMode m_autoSaveMode = PaintableImageAssetAutoSaveMode::AutoSave;

        //! Track whether or not we've prompted the user for an image save location at least once since this component was created.
        //! This is intentionally not serialized so that every user is prompted at least once per editor run for autosaves. This choice
        //! prioritizes data safety over lower friction - it's too easy for autosave to overwrite data accidentally, so we want the user
        //! to specifically choose a save location at least once before overwriting without prompts.
        //! We could serialize the flag so that the user only selects a location once per component, instead of once per component per
        //! Editor run, but that serialized flag would be shared with other users, so we would have other users editing the same image
        //! that never get prompted and might overwrite data by mistake.
        bool m_promptedForSaveLocation = false;

        AZ::EntityComponentIdPair m_ownerEntityComponentIdPair = {};
        DefaultSaveNameCallback m_defaultSaveNameCallback = {};
        OnCreateImageCallback m_onCreateImageCallback = {};
        OutputFormat m_defaultOutputFormat = OutputFormat::R8G8B8A8;
    };

    //! Helper class to manage all the common logic and UX for paintable image creation, editing, and saving.
    //! This class requires templatized parameters for the EditorComponentType and the EditorComponentModeType so that
    //! it can hook up to a specific type of editor component mode.
    template<typename EditorComponentType, typename EditorComponentModeType>
    class PaintableImageAssetHelper : public PaintableImageAssetHelperBase
    {
    public:
        AZ_CLASS_ALLOCATOR(PaintableImageAssetHelper, AZ::SystemAllocator);
        AZ_RTTI((PaintableImageAssetHelper, "{A06517E2-9D6B-4AD6-AD7C-FBE3BF0FD57B}", EditorComponentType, EditorComponentModeType),
            PaintableImageAssetHelperBase);

        static void Reflect(AZ::ReflectContext* context)
        {
            PaintableImageAssetHelperBase::Reflect(context);

            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<PaintableImageAssetHelper, PaintableImageAssetHelperBase>()->Version(0);

                if (auto editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<PaintableImageAssetHelper>("Paintable Image Asset", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
                }
            }
        }


        ~PaintableImageAssetHelper() override = default;

    protected:
        //! Enable component mode for this image asset.
        void EnableComponentMode() override
        {
            if (m_componentModeDelegate.IsConnected())
            {
                return;
            }

            m_componentModeDelegate.ConnectWithSingleComponentMode<EditorComponentType, EditorComponentModeType>(
                m_ownerEntityComponentIdPair, nullptr);
        }

    };

} // namespace GradientSignal::ImageCreatorUtils

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(GradientSignal::ImageCreatorUtils::PaintableImageAssetAutoSaveMode, "{F2ACB042-333B-4411-AEF9-2A419D01B14E}");
} // namespace AZ
