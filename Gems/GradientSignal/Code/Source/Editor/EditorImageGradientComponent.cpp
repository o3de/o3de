/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/EditorImageGradientComponent.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzQtComponents/Components/Widgets/FileDialog.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyFilePathCtrl.h>
#include <Editor/EditorImageGradientComponentMode.h>

AZ_PUSH_DISABLE_WARNING(4777, "-Wunknown-warning-option")
#include <OpenImageIO/imageio.h>
AZ_POP_DISABLE_WARNING

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QApplication>
#include <QMessageBox>
#include <QProgressDialog>
AZ_POP_DISABLE_WARNING

namespace GradientSignal
{
    void EditorImageGradientComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClassType::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorImageGradientComponent, BaseClassType>()
                ->Version(2)
                ->Field("Previewer", &EditorImageGradientComponent::m_previewer)
                ->Field("CreationSelectionChoice", &EditorImageGradientComponent::m_creationSelectionChoice)
                ->Field("OutputResolution", &EditorImageGradientComponent::m_outputResolution)
                ->Field("OutputFormat", &EditorImageGradientComponent::m_outputFormat)
                ->Field("OutputImagePath", &EditorImageGradientComponent::m_outputImagePath)
                ->Field("ComponentMode", &EditorImageGradientComponent::m_componentModeDelegate)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<ImageGradientConfig>("Image Gradient", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &ImageGradientConfig::m_imageAsset,
                        "Image Asset", "Image asset whose values will be mapped as gradient output.")
                        ->Attribute(AZ::Edit::Attributes::Handler, AZ_CRC_CE("GradientSignalStreamingImageAsset"))
                        ->Attribute(AZ::Edit::Attributes::Visibility, &ImageGradientConfig::GetImageOptionsVisibility)
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &ImageGradientConfig::GetImageAssetPropertyName)
                        // Refresh the attributes because some fields will switch between read-only and writeable when
                        // the image asset is changed.
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)

                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &ImageGradientConfig::m_samplingType,
                        "Sampling Type", "Sampling type to use for the image data.")
                        ->EnumAttribute(SamplingType::Point, "Point")
                        ->EnumAttribute(SamplingType::Bilinear, "Bilinear")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &ImageGradientConfig::GetImageOptionsVisibility)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &ImageGradientConfig::AreImageOptionsReadOnly)

                    ->DataElement(AZ::Edit::UIHandlers::Vector2, &ImageGradientConfig::m_tiling,
                        "Tiling", "Number of times to tile horizontally/vertically.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                        ->Attribute(AZ::Edit::Attributes::SoftMin, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 1024.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.25f)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &ImageGradientConfig::GetImageOptionsVisibility)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &ImageGradientConfig::AreImageOptionsReadOnly)

                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &ImageGradientConfig::m_channelToUse,
                        "Channel To Use", "The channel to use from the image.")
                        ->EnumAttribute(ChannelToUse::Red, "Red (default)")
                        ->EnumAttribute(ChannelToUse::Green, "Green")
                        ->EnumAttribute(ChannelToUse::Blue, "Blue")
                        ->EnumAttribute(ChannelToUse::Alpha, "Alpha")
                        ->EnumAttribute(ChannelToUse::Terrarium, "Terrarium")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &ImageGradientConfig::GetImageOptionsVisibility)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &ImageGradientConfig::AreImageOptionsReadOnly)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &ImageGradientConfig::m_mipIndex, "Mip Index", "Mip index to sample from.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0)
                        ->Attribute(AZ::Edit::Attributes::Max, AZ::RHI::Limits::Image::MipCountMax)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &ImageGradientConfig::GetImageOptionsVisibility)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &ImageGradientConfig::AreImageOptionsReadOnly)

                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &ImageGradientConfig::m_customScaleType,
                        "Custom Scale", "Choose a type of scaling to be applied to the image data.")
                        ->EnumAttribute(CustomScaleType::None, "None (default)")
                        ->EnumAttribute(CustomScaleType::Auto, "Auto")
                        ->EnumAttribute(CustomScaleType::Manual, "Manual")
                        // Refresh the entire tree on scaling changes, because it will show/hide the scale ranges for Manual scaling.
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &ImageGradientConfig::GetImageOptionsVisibility)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &ImageGradientConfig::AreImageOptionsReadOnly)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &ImageGradientConfig::m_scaleRangeMin,
                        "Range Minimum", "The minimum range each value from the image data is scaled against.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &ImageGradientConfig::GetManualScaleVisibility)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &ImageGradientConfig::AreImageOptionsReadOnly)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &ImageGradientConfig::m_scaleRangeMax,
                        "Range Maximum", "The maximum range each value from the image data is scaled against.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &ImageGradientConfig::GetManualScaleVisibility)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &ImageGradientConfig::AreImageOptionsReadOnly)
                    ;


                editContext
                    ->Class<EditorImageGradientComponent>(
                        EditorImageGradientComponent::s_componentName, EditorImageGradientComponent::s_componentDescription)
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Icon, EditorImageGradientComponent::s_icon)
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, EditorImageGradientComponent::s_viewportIcon)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, EditorImageGradientComponent::s_helpUrl)
                        ->Attribute(AZ::Edit::Attributes::Category, EditorImageGradientComponent::s_categoryName)
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorImageGradientComponent::m_previewer, "Previewer", "Gradient Previewer")

                    // Either show the "Create" options or the "use image" options based on how this is set.
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &EditorImageGradientComponent::m_creationSelectionChoice,
                        "Source Type", "Select whether to create a new image or use an existing image.")
                        ->EnumAttribute(ImageCreationOrSelection::UseExistingImage, "Use Existing Image")
                        ->EnumAttribute(ImageCreationOrSelection::CreateNewImage, "Create New Image")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorImageGradientComponent::ChangeCreationSelectionChoice)

                    // Controls for creating a new image
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorImageGradientComponent::m_outputResolution,
                        "Resolution", "Output resolution of the saved image.")
                        ->Attribute(AZ::Edit::Attributes::Decimals, 0)
                        ->Attribute(AZ::Edit::Attributes::Min, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 8192.0f)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &EditorImageGradientComponent::GetImageCreationVisibility)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &EditorImageGradientComponent::m_outputFormat,
                        "Output Format", "Output format of the saved image.")
                        ->Attribute(AZ::Edit::Attributes::EnumValues, &GradientImageCreatorRequests::SupportedOutputFormatOptions)
                        ->Attribute(AZ::Edit::Attributes::Visibility, &EditorImageGradientComponent::GetImageCreationVisibility)
                    ->UIElement(AZ::Edit::UIHandlers::Button, "", "Create Image")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorImageGradientComponent::CreateImage)
                        ->Attribute(AZ::Edit::Attributes::ButtonText, "Create")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &EditorImageGradientComponent::GetImageCreationVisibility)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorImageGradientComponent::m_componentModeDelegate,
                        "Paint Image", "Paint into an image asset")
                        // This needs to be a larger number than the one in EditorWrappedComponentBase so that the element
                        // is drawn below the ImageGradientComponent configuration.
                        ->Attribute(AZ::Edit::Attributes::DisplayOrder, 10)
                        ->Attribute(AZ::Edit::Attributes::ButtonText, "Paint")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &EditorImageGradientComponent::GetPaintModeVisibility)
                    ;

            }
        }
    }

    void EditorImageGradientComponent::Activate()
    {
        LmbrCentral::DependencyNotificationBus::Handler::BusConnect(GetEntityId());
        EditorImageGradientRequestBus::Handler::BusConnect(GetEntityId());

        BaseClassType::Activate();

        m_previewer.Activate(GetEntityId());
        GradientImageCreatorRequestBus::Handler::BusConnect(GetEntityId());
            
        m_currentImageAssetStatus = m_configuration.m_imageAsset.GetStatus();

        m_componentModeDelegate.ConnectWithSingleComponentMode<EditorImageGradientComponent, EditorImageGradientComponentMode>(
            AZ::EntityComponentIdPair(GetEntityId(), GetId()), nullptr);
    }

    void EditorImageGradientComponent::Deactivate()
    {
        m_currentImageAssetStatus = AZ::Data::AssetData::AssetStatus::NotLoaded;
        m_componentModeDelegate.Disconnect();

        GradientImageCreatorRequestBus::Handler::BusDisconnect();
        m_previewer.Deactivate();

        EditorImageGradientRequestBus::Handler::BusDisconnect();
        LmbrCentral::DependencyNotificationBus::Handler::BusDisconnect();

        BaseClassType::Deactivate();
    }

    void EditorImageGradientComponent::OnCompositionChanged()
    {
        m_previewer.RefreshPreview();
        m_component.WriteOutConfig(&m_configuration);
        SetDirty();

        if (RefreshImageAssetStatus())
        {
            AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
                &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_EntireTree);
        }
        else
        {
            AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
                &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_AttributesAndValues);
        }
    }

    bool EditorImageGradientComponent::ImageHasPendingJobs(const AZ::Data::AssetId& assetId)
    {
        // If it's not a valid asset, it doesn't have any pending jobs.
        if (!assetId.IsValid())
        {
            return false;
        }

        AZ::Outcome<AzToolsFramework::AssetSystem::JobInfoContainer> jobOutcome = AZ::Failure();
        AzToolsFramework::AssetSystemJobRequestBus::BroadcastResult(
            jobOutcome,
            &AzToolsFramework::AssetSystemJobRequestBus::Events::GetAssetJobsInfoByAssetID,
            assetId,
            false,
            false);

        if (jobOutcome.IsSuccess())
        {
            // If there are any jobs that are pending, we'll set our current status based on that instead of
            // on the asset loading status.
            auto jobInfo = jobOutcome.GetValue();
            for (auto& job : jobInfo)
            {
                switch (job.m_status)
                {
                case AzToolsFramework::AssetSystem::JobStatus::Queued:
                case AzToolsFramework::AssetSystem::JobStatus::InProgress:
                    return true;
                }
            }
        }

        return false;
    }


    bool EditorImageGradientComponent::RefreshImageAssetStatus()
    {
        bool statusChanged = (m_currentImageAssetStatus != m_configuration.m_imageAsset.GetStatus());
        m_currentImageAssetStatus = m_configuration.m_imageAsset.GetStatus();

        // If there's a valid image selected, check to see if it has any current AP jobs running.
        if (ImageHasPendingJobs(m_configuration.m_imageAsset.GetId()))
        {
            m_configuration.SetImageAssetPropertyName("Image Asset (processing)");
            return statusChanged;
        }

        // No pending asset processing jobs, so just use the current load status of the asset.
        switch (m_currentImageAssetStatus)
        {
        case AZ::Data::AssetData::AssetStatus::NotLoaded:
        case AZ::Data::AssetData::AssetStatus::Error:
            m_configuration.SetImageAssetPropertyName("Image Asset (not loaded)");
            break;
        case AZ::Data::AssetData::AssetStatus::Queued:
        case AZ::Data::AssetData::AssetStatus::StreamReady:
        case AZ::Data::AssetData::AssetStatus::Loading:
        case AZ::Data::AssetData::AssetStatus::LoadedPreReady:
            m_configuration.SetImageAssetPropertyName("Image Asset (loading)");
            break;
        case AZ::Data::AssetData::AssetStatus::ReadyPreNotify:
        case AZ::Data::AssetData::AssetStatus::Ready:
        default:
            m_configuration.SetImageAssetPropertyName("Image Asset");
            break;
        }

        return statusChanged;
    }

    AZ::u32 EditorImageGradientComponent::ConfigurationChanged()
    {
        // Cancel any pending preview refreshes before locking, to help ensure the preview itself isn't holding the lock
        auto entityIds = m_previewer.CancelPreviewRendering();

        auto refreshResult = BaseClassType::ConfigurationChanged();

        // Refresh any of the previews that we canceled that were still in progress so they can be completed
        m_previewer.RefreshPreviews(entityIds);

        // This OnCompositionChanged notification will refresh our own preview so we don't need to call RefreshPreview explicitly
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);

        return refreshResult;
    }

    AZ::u32 EditorImageGradientComponent::ChangeCreationSelectionChoice()
    {
        m_configuration.SetImageOptionsVisibility(m_creationSelectionChoice == ImageCreationOrSelection::UseExistingImage);

        // We need to refresh the entire tree because this selection changes the visibility of other properties.
        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    bool EditorImageGradientComponent::GetImageCreationVisibility() const
    {
        // Only show the image creation options if we don't have an existing image asset selected.
        return !m_configuration.GetImageOptionsVisibility();
    }

    AZ::Crc32 EditorImageGradientComponent::GetPaintModeVisibility() const
    {
        // Only show the image painting button while we're using an image, not while we're creating one.
        return (m_configuration.GetImageOptionsVisibility()
                && m_configuration.m_imageAsset.IsReady()
                && !ImageHasPendingJobs(m_configuration.m_imageAsset.GetId()))
            ? AZ::Edit::PropertyVisibility::ShowChildrenOnly
            : AZ::Edit::PropertyVisibility::Hide;

    }

    AZ::Vector2 EditorImageGradientComponent::GetOutputResolution() const
    {
        return m_outputResolution;
    }

    void EditorImageGradientComponent::SetOutputResolution(const AZ::Vector2& resolution)
    {
        m_outputResolution = resolution;
    }

    OutputFormat EditorImageGradientComponent::GetOutputFormat() const
    {
        return m_outputFormat;
    }

    void EditorImageGradientComponent::SetOutputFormat(OutputFormat outputFormat)
    {
        m_outputFormat = outputFormat;
    }

    AZ::IO::Path EditorImageGradientComponent::GetOutputImagePath() const
    {
        return m_outputImagePath;
    }

    void EditorImageGradientComponent::SetOutputImagePath(const AZ::IO::Path& outputImagePath)
    {
        m_outputImagePath = outputImagePath;
    }

    bool EditorImageGradientComponent::InComponentMode()
    {
        return m_componentModeDelegate.AddedToComponentMode();
    }

    void EditorImageGradientComponent::RefreshPreview()
    {
        m_previewer.RefreshPreview();
    }

    void EditorImageGradientComponent::SaveImage()
    {
        CreateImage();
    }

    AZStd::vector<float>* EditorImageGradientComponent::GetPixelBuffer()
    {
        return nullptr;
    }


    void EditorImageGradientComponent::CreateImage()
    {
        // Create a default path to save our image to if we haven't previously set the image path to anything yet.
        if (m_outputImagePath.empty())
        {
            m_outputImagePath = AZStd::string::format("%.*s_gsi.tif", AZ_STRING_ARG(GetEntity()->GetName()));
        }

        // Turn it into an absolute path to give to the "Save file" dialog.
        AZ::IO::Path fullPathIO = AzToolsFramework::GetAbsolutePathFromRelativePath(m_outputImagePath);

        // Prompt the user for the file name and path.
        const QString fileFilter = GradientImageCreatorRequests::GetSupportedImagesFilter().c_str();
        const QString absoluteSaveFilePath =
            AzQtComponents::FileDialog::GetSaveFileName(nullptr, "Save As...", QString(fullPathIO.Native().c_str()), fileFilter);

        // User canceled the save dialog, so exit out.
        if (absoluteSaveFilePath.isEmpty())
        {
            return;
        }

        const auto absoluteSaveFilePathUtf8 = absoluteSaveFilePath.toUtf8();
        const auto absoluteSaveFilePathCStr = absoluteSaveFilePathUtf8.constData();

        // Turn the absolute path selected in the "Save file" dialog back into a relative path.
        AZStd::string relativePath;
        AZStd::string rootFilePath;
        bool relativePathFound;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
            relativePathFound,
            &AzToolsFramework::AssetSystemRequestBus::Events::GenerateRelativeSourcePath,
            absoluteSaveFilePathCStr, relativePath, rootFilePath);

        if (!relativePathFound)
        {
            AZ_Error("EditorImageGradientComponent", false,
                "Selected path exists outside of the asset processing directories: %s", absoluteSaveFilePathCStr);
            return;
        }

        // check out the file in source control
        bool checkedOutSuccessfully = false;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            checkedOutSuccessfully,
            &AzToolsFramework::ToolsApplicationRequestBus::Events::RequestEditForFileBlocking,
            absoluteSaveFilePathCStr,
            "Checking out for edit...",
            AzToolsFramework::ToolsApplicationRequestBus::Events::RequestEditProgressCallback());

        if (!checkedOutSuccessfully)
        {
            AZ_Error(
                "EditorImageGradientComponent", false, "Failed to check out file from source control: %s", absoluteSaveFilePathCStr);
            return;
        }

        // Try to write out the image
        if (!WriteImage(absoluteSaveFilePathCStr))
        {
            AZ_Error("EditorImageGradientComponent", false, "Failed to create image: %s", absoluteSaveFilePathCStr);
            return;
        }

        // Try to find the source information for the new image in the Asset System.
        bool sourceInfoFound = false;
        AZ::Data::AssetInfo sourceInfo;
        AZStd::string watchFolder;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
            sourceInfoFound,
            &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath,
            absoluteSaveFilePathCStr,
            sourceInfo,
            watchFolder);

        // If this triggers, the flow for handling newly-created images needs to be examined further.
        // It's possible that we need to wait for some sort of asset processing event before we can get
        // the source asset ID.
        AZ_Warning("EditorImageGradientComponent", sourceInfoFound, "Could not find source info for %s", absoluteSaveFilePathCStr);

        // Using the source asset ID, get or create an asset referencing using the expected product asset ID.
        // If we're overwriting an existing source asset, this will already exist, but if we're creating a new file,
        // the product asset won't exist yet.
        auto createdAsset = AZ::Data::AssetManager::Instance().FindOrCreateAsset(
            AZ::Data::AssetId(sourceInfo.m_assetId.m_guid, AZ::RPI::StreamingImageAsset::GetImageAssetSubId()),
            azrtti_typeid<AZ::RPI::StreamingImageAsset>(), AZ::Data::AssetLoadBehavior::QueueLoad);

        // Set the asset hint to the source path so that we can display something reasonably correct in the component while waiting
        // for the product asset to get created.
        createdAsset.SetHint(relativePath);

        // Set our output image path to whatever was selected so that we default to the same file name and path next time.
        m_outputImagePath = absoluteSaveFilePathCStr;

        // Set the active image to the created one.
        m_component.SetImageAsset(createdAsset);

        // Switch our creation/selection choice to using an existing image.
        m_creationSelectionChoice = ImageCreationOrSelection::UseExistingImage;

        // Resync the configurations and refresh the display to hide the "Create" button
        // We need to use "Refresh_EntireTree" because "Refresh_AttributesAndValues" isn't enough to refresh the visibility
        // settings.
        OnCompositionChanged();
        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_EntireTree);
    }

    bool EditorImageGradientComponent::WriteImage(const AZStd::string& absoluteFileName)
    {
        // Show a dialog box letting the user know the image is being written out.
        // For large image sizes, it can take 15+ seconds to create and write out the image.
        QProgressDialog saveDialog;
        saveDialog.setWindowFlags(saveDialog.windowFlags() & ~Qt::WindowCloseButtonHint);
        saveDialog.setLabelText("Saving image...");
        saveDialog.setWindowModality(Qt::WindowModal);
        saveDialog.setMaximumSize(QSize(256, 96));
        saveDialog.setMinimum(0);
        saveDialog.setMaximum(100);
        saveDialog.setMinimumDuration(0);
        saveDialog.setAutoClose(false);
        saveDialog.setCancelButton(nullptr);
        saveDialog.show();
        QApplication::processEvents();

        // Get the actual resolution of our image.
        const int imageResolutionX = aznumeric_cast<int>(m_outputResolution.GetX());
        const int imageResolutionY = aznumeric_cast<int>(m_outputResolution.GetY());

        int bytesPerPixel = 0;
        OIIO::TypeDesc pixelFormat = OIIO::TypeDesc::UINT8;
        switch (m_outputFormat)
        {
        case OutputFormat::R8:
            bytesPerPixel = 1;
            pixelFormat = OIIO::TypeDesc::UINT8;
            break;
        case OutputFormat::R16:
            bytesPerPixel = 2;
            pixelFormat = OIIO::TypeDesc::UINT16;
            break;
        case OutputFormat::R32:
            bytesPerPixel = 4;
            pixelFormat = OIIO::TypeDesc::FLOAT;
            break;
        default:
            AZ_Assert(false, "Unsupported output image format (%d)", m_outputFormat);
            return false;
        }

        AZ::IO::Path fullPathIO(absoluteFileName);

        // The TGA and EXR formats aren't recognized with only single channel data,
        // so need to use RGBA format for them
        int channels = 1;
        if (fullPathIO.Extension() == ".tga" || fullPathIO.Extension() == ".exr")
        {
            channels = 4;
        }

        // Fill in our image buffer. Default all values to 0 (black)
        const size_t imageSize = imageResolutionX * imageResolutionY * channels * bytesPerPixel;
        AZStd::vector<AZ::u8> pixels(imageSize, 0);

        // If we're saving a 4-channel image, loop through and set the Alpha channel to opaque.
        if (channels == 4)
        {
            for (size_t alphaIndex = (channels - 1); alphaIndex < (imageResolutionX * imageResolutionY * channels); alphaIndex += channels)
            {
                switch (m_outputFormat)
                {
                case OutputFormat::R8:
                    {
                        pixels[alphaIndex] = std::numeric_limits<AZ::u8>::max();
                        break;
                    }
                case OutputFormat::R16:
                    {
                        auto actualMem = reinterpret_cast<AZ::u16*>(pixels.data());
                        actualMem[alphaIndex] = std::numeric_limits<AZ::u16>::max();
                        break;
                    }
                case OutputFormat::R32:
                    {
                        auto actualMem = reinterpret_cast<float*>(pixels.data());
                        actualMem[alphaIndex] = 1.0f;
                        break;
                    }
                }
            }
        }

        // Give our progress dialog another chance to update so we don't look frozen.
        saveDialog.setValue(1);
        QApplication::processEvents();

        // Create and save the image on disk

        AZ::IO::Path absolutePath = fullPathIO.LexicallyNormal();
        std::unique_ptr<OIIO::ImageOutput> outputImage = OIIO::ImageOutput::create(absolutePath.c_str());
        if (!outputImage)
        {
            AZ_Error("EditorImageGradientComponent", false, "Failed to create image at path: %s", absolutePath.c_str());
            return false;
        }

        OIIO::ImageSpec spec(imageResolutionX, imageResolutionY, channels, pixelFormat);
        outputImage->open(absolutePath.c_str(), spec);

        // Callback to upgrade our progress dialog during image saving.
        auto WriteProgressCallback = [](void* opaqueData, float percentDone) -> bool
        {
            QProgressDialog* saveDialog = reinterpret_cast<QProgressDialog*>(opaqueData);
            saveDialog->setValue(aznumeric_cast<int>(percentDone * 100.0f));
            QApplication::processEvents();
            return false;
        };

        bool result = outputImage->write_image(
            pixelFormat, pixels.data(), OIIO::AutoStride, OIIO::AutoStride, OIIO::AutoStride, WriteProgressCallback, &saveDialog);
        if (!result)
        {
            AZ_Error("EditorImageGradientComponent", result, "Failed to write out gradient image to path: %s", absolutePath.c_str());
        }

        outputImage->close();

        return result;
    }
}
