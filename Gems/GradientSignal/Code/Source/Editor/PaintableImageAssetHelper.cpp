/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(Q_MOC_RUN)
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Preprocessor/EnumReflectUtils.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/Application/Application.h>
#include <AzFramework/IO/FileOperations.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzQtComponents/Components/Widgets/FileDialog.h>
#include <AzQtComponents/Components/Widgets/SpinBox.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>

#include <GradientSignal/Editor/EditorGradientImageCreatorRequestBus.h>
#include <GradientSignal/Editor/EditorGradientImageCreatorUtils.h>
#include <GradientSignal/Editor/PaintableImageAssetHelper.h>

#include <QDialog>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>
#endif

namespace GradientSignal::ImageCreatorUtils
{
    //! CreateImageDialog allows the user to specify a set of image creation parameters for use in creating a new image asset.
    class CreateImageDialog : public QDialog
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(CreateImageDialog, AZ::SystemAllocator);

        CreateImageDialog(QWidget* parent = nullptr)
            : QDialog(parent)
        {
            setModal(true);
            setMinimumWidth(300);
            resize(300, 100);
            setWindowTitle("Create New Image");

            // Create the layout for all the widgets to be stacked vertically.
            auto verticalLayout = new QVBoxLayout();

            // Create the width and height widgets

            m_width = new AzQtComponents::SpinBox();
            m_width->setRange(MinPixels, MaxPixels);
            m_width->setValue(DefaultPixels);

            m_height = new AzQtComponents::SpinBox();
            m_height->setRange(MinPixels, MaxPixels);
            m_height->setValue(DefaultPixels);

            QGridLayout* dimensionsLayout = new QGridLayout();
            dimensionsLayout->addWidget(new QLabel("Width:"), 0, 0);
            dimensionsLayout->addWidget(m_width, 0, 1);
            dimensionsLayout->addWidget(new QLabel("Height:"), 0, 2);
            dimensionsLayout->addWidget(m_height, 0, 3);

            verticalLayout->addLayout(dimensionsLayout);

            // Connect ok and cancel buttons and change "ok" to "next".
            auto buttonBox = new QDialogButtonBox(this);
            buttonBox->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed));
            buttonBox->setOrientation(Qt::Horizontal);
            buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
            QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
            QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
            verticalLayout->addWidget(buttonBox);

            // We set this to "Next" instead of "OK" because after the dialog box completes, a standard native file picker dialog
            // will appear to select the save location for the created image, so the entire process appears as two steps to the end user.
            buttonBox->button(QDialogButtonBox::Ok)->setText("Next");

            auto gridLayout = new QGridLayout(this);
            gridLayout->addLayout(verticalLayout, 0, 0, 1, 1);

            adjustSize();
        }

        ~CreateImageDialog() = default;

        int GetWidth()
        {
            return m_width->value();
        }

        int GetHeight()
        {
            return m_height->value();
        }

    private:
        // Min/max/default values for the image dimensions
        static inline constexpr int MinPixels = 1;
        static inline constexpr int MaxPixels = 8192;
        static inline constexpr int DefaultPixels = 512;

        AzQtComponents::SpinBox* m_width = nullptr;
        AzQtComponents::SpinBox* m_height = nullptr;
    };

    AZ_ENUM_DEFINE_REFLECT_UTILITIES(PaintableImageAssetAutoSaveMode);

    void PaintableImageAssetHelperBase::Reflect(AZ::ReflectContext* context)
    {
        // Don't reflect again if we're already reflected this type to the passed-in context.
        // (The guard is necessary because every subclass of this base will try to reflect the base class as well)
        if (context->IsTypeReflected(azrtti_typeid<PaintableImageAssetHelperBase>()))
        {
            return;
        }

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            PaintableImageAssetAutoSaveModeReflect(*serializeContext);

            serializeContext->Class<PaintableImageAssetHelperBase>()
                ->Version(0)
                ->Field("AutoSaveMode", &PaintableImageAssetHelperBase::m_autoSaveMode)
                ->Field("ComponentMode", &PaintableImageAssetHelperBase::m_componentModeDelegate);

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<PaintableImageAssetHelperBase>("Paintable Image Asset", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    // Auto-save option when editing an image.
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &PaintableImageAssetHelperBase::m_autoSaveMode,
                        "Auto-Save Mode",
                        "When editing an image, this selects whether to manually prompt for the save location, auto-save on every "
                        "edit, "
                        "or auto-save with incrementing file names on every edit.")
                    ->EnumAttribute(PaintableImageAssetAutoSaveMode::SaveAs, "Save As...")
                    ->EnumAttribute(PaintableImageAssetAutoSaveMode::AutoSave, "Auto Save")
                    ->EnumAttribute(PaintableImageAssetAutoSaveMode::AutoSaveWithIncrementalNames, "Auto Save With Incrementing Names")
                    // There's no need to ChangeNotify when this property changes, it doesn't affect the behavior of the comopnent,
                    // it's only queried at the point that an edit is completed.

                    // Paint controls for editing the image
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &PaintableImageAssetHelperBase::m_componentModeDelegate,
                        "Paint Image",
                        "Paint into an image asset")
                    ->Attribute(AZ::Edit::Attributes::ButtonText, "Paint")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &PaintableImageAssetHelperBase::GetPaintModeVisibility)

                    ->UIElement(AZ::Edit::UIHandlers::Button, "CreateImage", "Create a new image asset.")
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                    ->Attribute(AZ::Edit::Attributes::ButtonText, "Create New Image...")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &PaintableImageAssetHelperBase::CreateNewImage)
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &PaintableImageAssetHelperBase::InComponentMode)

                    ;
            }
        }
    }

    AZ::Crc32 PaintableImageAssetHelperBase::GetPaintModeVisibility() const
    {
        return ((m_currentImageAssetStatus == AZ::Data::AssetData::AssetStatus::Ready) && !m_currentImageJobsPending)
            ? AZ::Edit::PropertyVisibility::ShowChildrenOnly
            : AZ::Edit::PropertyVisibility::Hide;
    }

    bool PaintableImageAssetHelperBase::ImageHasPendingJobs(const AZ::Data::AssetId& assetId)
    {
        // If it's not a valid asset, it doesn't have any pending jobs.
        if (!assetId.IsValid())
        {
            return false;
        }

        AZ::Outcome<AzToolsFramework::AssetSystem::JobInfoContainer> jobOutcome = AZ::Failure();
        AzToolsFramework::AssetSystemJobRequestBus::BroadcastResult(
            jobOutcome, &AzToolsFramework::AssetSystemJobRequestBus::Events::GetAssetJobsInfoByAssetID, assetId, false, false);

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

    bool PaintableImageAssetHelperBase::RefreshImageAssetStatus(AZ::Data::Asset<AZ::Data::AssetData> imageAsset)
    {
        bool jobsPending = ImageHasPendingJobs(imageAsset.GetId());
        bool statusChanged = (m_currentImageAssetStatus != imageAsset.GetStatus()) || (m_currentImageJobsPending != jobsPending);

        m_currentImageAssetStatus = imageAsset.GetStatus();
        m_currentImageJobsPending = jobsPending;

        return statusChanged;
    }

    AZStd::string PaintableImageAssetHelperBase::GetImageAssetStatusLabel()
    {
        if (m_currentImageJobsPending)
        {
            return m_baseAssetLabel + " (processing)";
        }

        // No pending asset processing jobs, so just use the current load status of the asset.
        switch (m_currentImageAssetStatus)
        {
        case AZ::Data::AssetData::AssetStatus::NotLoaded:
        case AZ::Data::AssetData::AssetStatus::Error:
            return m_baseAssetLabel + " (not loaded)";
            break;
        case AZ::Data::AssetData::AssetStatus::Queued:
        case AZ::Data::AssetData::AssetStatus::StreamReady:
        case AZ::Data::AssetData::AssetStatus::Loading:
        case AZ::Data::AssetData::AssetStatus::LoadedPreReady:
            return m_baseAssetLabel + " (loading)";
            break;
        case AZ::Data::AssetData::AssetStatus::ReadyPreNotify:
        case AZ::Data::AssetData::AssetStatus::Ready:
        default:
            return m_baseAssetLabel;
        }
    }

    void PaintableImageAssetHelperBase::DisableComponentMode()
    {
        if (!m_componentModeDelegate.IsConnected())
        {
            return;
        }

        m_componentModeDelegate.Disconnect();
    }

    void PaintableImageAssetHelperBase::RefreshComponentModeStatus()
    {
        const bool paintModeVisible = (GetPaintModeVisibility() != AZ::Edit::PropertyVisibility::Hide);

        if (paintModeVisible)
        {
            EnableComponentMode();
        }
        else
        {
            DisableComponentMode();
        }
    }

    bool PaintableImageAssetHelperBase::InComponentMode() const
    {
        return m_componentModeDelegate.AddedToComponentMode();
    }

    void PaintableImageAssetHelperBase::Activate(
        AZ::EntityComponentIdPair ownerEntityComponentIdPair,
        OutputFormat defaultOutputFormat,
        AZStd::string baseAssetLabel,
        DefaultSaveNameCallback defaultSaveNameCallback,
        OnCreateImageCallback onCreateImageCallback)
    {
        m_ownerEntityComponentIdPair = ownerEntityComponentIdPair;
        m_defaultOutputFormat = defaultOutputFormat;
        m_baseAssetLabel = baseAssetLabel;
        m_defaultSaveNameCallback = defaultSaveNameCallback;
        m_onCreateImageCallback = onCreateImageCallback;
    }

    AZStd::string PaintableImageAssetHelperBase::Refresh(AZ::Data::Asset<AZ::Data::AssetData> imageAsset)
    {
        RefreshImageAssetStatus(imageAsset);
        RefreshComponentModeStatus();
        return GetImageAssetStatusLabel();
    }

    void PaintableImageAssetHelperBase::Deactivate()
    {
        DisableComponentMode();
        m_currentImageAssetStatus = AZ::Data::AssetData::AssetStatus::NotLoaded;
        m_currentImageJobsPending = false;
    }

    AZ::IO::Path PaintableImageAssetHelperBase::GetIncrementingAutoSavePath(const AZ::IO::Path& currentPath) const
    {
        // Given a path for a source texture, this will return a new path with an incremented version number on the end.
        // If the input path doesn't have a version number yet, it will get one added.
        // Ex:
        // 'Assets/Gradients/MyGradient_gsi.tif' -> 'Assets/Gradients/MyGradient_gsi.0000.tif'
        // 'Assets/Gradients/MyGradient_gsi.0005.tif' -> 'Assets/Gradients/MyGradient_gsi.0006.tif'

        // We'll use 4 digits as our minimum version number size. We use this to add leading 0s so that alpha-sorting of the
        // numbers still puts them in the right order. For example, we'll get 08, 09, 10 instead of 0, 1, 10, 2. This does
        // mean that the alpha-sorting will look wrong if we hit 5 digits, but it's a readability tradeoff.
        constexpr int NumVersionDigits = 4;

        // Store a copy of the filename in a string so that we can modify it below.
        AZStd::string baseFileName = currentPath.Stem().Native();

        size_t foundDotChar = baseFileName.find_last_of(AZ_FILESYSTEM_EXTENSION_SEPARATOR);
        uint32_t versionNumber = 0;

        // If the base name ends with '.####' (4 or more digits), then we'll treat that as our auto version number.
        // We'll read in the existing number, strip it, and increment it.
        if (foundDotChar <= (baseFileName.length() - NumVersionDigits - 1))
        {
            bool foundVersionNumber = true;
            uint32_t tempVersionNumber = 0;

            for (size_t digitChar = foundDotChar + 1; digitChar < baseFileName.length(); digitChar++)
            {
                // If any character after the . isn't a digit, then this isn't a valid version number, so leave it alone.
                // (Ex: "image_gsi.o3de")
                if (!isdigit(baseFileName.at(digitChar)))
                {
                    foundVersionNumber = false;
                    break;
                }

                // Convert the version number that we've found one character at a time.
                tempVersionNumber = (tempVersionNumber * 10) + (baseFileName.at(digitChar) - '0');
            }

            // If we found a valid version number, auto-increment it by one and strip off the previous one.
            // We'll re-add the new incremented version number back on at the end.
            if (foundVersionNumber)
            {
                versionNumber = tempVersionNumber + 1;
                baseFileName = baseFileName.substr(0, foundDotChar);
            }
        }

        // Create a new string of the form <filename>.####
        // For example, "entity1_gsi.tif" should become "entity1_gsi.0000.tif"
        AZStd::string newFilename = AZStd::string::format(
            AZ_STRING_FORMAT "%c%0*d" AZ_STRING_FORMAT,
            AZ_STRING_ARG(baseFileName),
            AZ_FILESYSTEM_EXTENSION_SEPARATOR,
            NumVersionDigits,
            versionNumber,
            AZ_STRING_ARG(currentPath.Extension().Native()));

        AZ::IO::Path newPath = currentPath;
        newPath.ReplaceFilename(AZ::IO::Path(newFilename));
        return newPath;
    }

    AZStd::string PaintableImageAssetHelperBase::GetRelativePathFromAbsolutePath(AZStd::string_view absolutePath)
    {
        AZStd::string relativePath;

        // Turn the absolute path selected in the "Save file" dialog back into a relative path.
        // It's a way to verify that our path exists within the project asset search hierarchy,
        // and it will get used as an asset hint until the asset is fully processed.
        AZStd::string rootFilePath;
        bool relativePathFound = false;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
            relativePathFound,
            &AzToolsFramework::AssetSystemRequestBus::Events::GenerateRelativeSourcePath,
            absolutePath,
            relativePath,
            rootFilePath);

        if (!relativePathFound)
        {
            relativePath.clear();
        }

        return relativePath;
    }

    bool PaintableImageAssetHelperBase::GetSaveLocation(
        AZ::IO::Path& fullPath, AZStd::string& relativePath, PaintableImageAssetAutoSaveMode autoSaveMode)
    {
        QString absoluteSaveFilePath = QString(fullPath.Native().c_str());
        bool promptForSaveName = false;

        switch (autoSaveMode)
        {
        case PaintableImageAssetAutoSaveMode::SaveAs:
            promptForSaveName = true;
            break;
        case PaintableImageAssetAutoSaveMode::AutoSave:
            // If the user has never been prompted for a save location during this Editor run, make sure they're prompted at least once.
            // If they have been prompted, then skip the prompt and just overwrite the existing location.
            promptForSaveName = !m_promptedForSaveLocation;
            break;
        case PaintableImageAssetAutoSaveMode::AutoSaveWithIncrementalNames:
            fullPath = GetIncrementingAutoSavePath(fullPath);
            absoluteSaveFilePath = QString(fullPath.Native().c_str());

            // Only prompt if our auto-generated name matches an existing file.
            promptForSaveName = AZ::IO::SystemFile::Exists(fullPath.Native().c_str());
            break;
        }

        if (promptForSaveName)
        {
            // Prompt the user for the file name and path.
            const QString fileFilter = ImageCreatorUtils::GetSupportedImagesFilter().c_str();
            absoluteSaveFilePath = AzQtComponents::FileDialog::GetSaveFileName(nullptr, "Save As...", absoluteSaveFilePath, fileFilter);
        }

        // User canceled the save dialog, so exit out.
        if (absoluteSaveFilePath.isEmpty())
        {
            return false;
        }

        // If we prompted for a save name and didn't cancel out with an empty path, track that we've prompted the user so that we don't
        // do it again for autosave.
        m_promptedForSaveLocation = m_promptedForSaveLocation || promptForSaveName;

        const auto absoluteSaveFilePathUtf8 = absoluteSaveFilePath.toUtf8();
        const auto absoluteSaveFilePathCStr = absoluteSaveFilePathUtf8.constData();
        fullPath.Assign(absoluteSaveFilePathCStr);
        fullPath = fullPath.LexicallyNormal();

        relativePath = GetRelativePathFromAbsolutePath(fullPath.Native());

        if (relativePath.empty())
        {
            AZ_Error(
                "PaintableImageAssetHelper",
                false,
                "Selected path exists outside of the asset processing directories: %s",
                absoluteSaveFilePathCStr);
            return false;
        }

        return true;
    }

    void PaintableImageAssetHelperBase::CreateNewImage()
    {
        QWidget* mainWindow = nullptr;
        AzToolsFramework::EditorRequests::Bus::BroadcastResult(mainWindow, &AzToolsFramework::EditorRequests::Bus::Events::GetMainWindow);

        // Prompt the user for width and height values.
        CreateImageDialog dialog(mainWindow);

        // If the user pressed "cancel", then return.
        if (dialog.exec() != QDialog::Accepted)
        {
            return;
        }

        // Get the requested image resolution.
        const int imageResolutionX = dialog.GetWidth();
        const int imageResolutionY = dialog.GetHeight();

        // Call the provided callback to get a default filename to save the created image with.
        AZ::IO::Path fileName(m_defaultSaveNameCallback());

        // Prompt the user for the save location.
        QString absoluteSaveFilePath = AzQtComponents::FileDialog::GetSaveFileName(
            mainWindow, "Save As...", QString(fileName.Native().c_str()), QString(ImageCreatorUtils::GetSupportedImagesFilter().c_str()));

        // If the user pressed "cancel", then return.
        if (absoluteSaveFilePath.isEmpty())
        {
            return;
        }

        // The Utf8 version of the save file path is saved into a variable here to ensure that its lifetime of the data is the
        // same as fileName.
        const auto absoluteSaveFilePathUtf8 = absoluteSaveFilePath.toUtf8();
        fileName.Assign(absoluteSaveFilePathUtf8.constData());
        fileName = fileName.LexicallyNormal();

        // The TGA and EXR formats aren't recognized with only single channel data,
        // so need to use RGBA format for them
        int channels = GetChannels(m_defaultOutputFormat);
        if (fileName.Extension() == ".tga" || fileName.Extension() == ".exr")
        {
            channels = 4;
        }

        AZStd::string relativeName = GetRelativePathFromAbsolutePath(fileName.Native());

        if (relativeName.empty())
        {
            AZ_Error(
                "PaintableImageAssetHelper",
                false,
                "Selected path exists outside of the asset processing directories: %s",
                fileName.Native().c_str());
            return;
        }

        // Create an blank pixel buffer for our created image.
        auto pixelBuffer = ImageCreatorUtils::CreateDefaultImageBuffer(imageResolutionX, imageResolutionY, channels, m_defaultOutputFormat);

        // Save the image.
        AZStd::string relativePath;
        auto createdAsset =
            SaveImageInternal(fileName, relativePath, imageResolutionX, imageResolutionY, channels, m_defaultOutputFormat, pixelBuffer);

        // Set the active image to the created one and refresh.
        if (createdAsset)
        {
            m_onCreateImageCallback(createdAsset.value());
        }
    }

    AZStd::optional<AZ::Data::Asset<AZ::Data::AssetData>> PaintableImageAssetHelperBase::SaveImage(
        int imageResolutionX, int imageResolutionY, OutputFormat format, AZStd::span<const uint8_t> pixelBuffer)
    {
        AZ::IO::Path fullPath = m_defaultSaveNameCallback();
        AZStd::string relativePath;

        if (!GetSaveLocation(fullPath, relativePath, m_autoSaveMode))
        {
            return {};
        }

        int channels = ImageCreatorUtils::GetChannels(format);

        if ((channels == 1) && ((fullPath.Extension() == ".tga" || fullPath.Extension() == ".exr")))
        {
            AZ_Assert(false, "1-channel TGA / EXR isn't currently supported in this method.");
            return {};
        }

        return SaveImageInternal(fullPath, relativePath, imageResolutionX, imageResolutionY, channels, format, pixelBuffer);
    }

    AZStd::optional<AZ::Data::Asset<AZ::Data::AssetData>> PaintableImageAssetHelperBase::SaveImageInternal(
        AZ::IO::Path& fullPath,
        AZStd::string& relativePath,
        int imageResolutionX,
        int imageResolutionY,
        int channels,
        OutputFormat format,
        AZStd::span<const uint8_t> pixelBuffer)
    {
        // Try to write out the image
        constexpr bool showProgressDialog = true;
        if (!ImageCreatorUtils::WriteImage(
                fullPath.c_str(), imageResolutionX, imageResolutionY, channels, format, pixelBuffer, showProgressDialog))
        {
            AZ_Error("PaintableImageAssetHelper", false, "Failed to save image: %s", fullPath.c_str());
            return {};
        }

        // Try to find the source information for the new image in the Asset System.
        bool sourceInfoFound = false;
        AZ::Data::AssetInfo sourceInfo;
        AZStd::string watchFolder;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
            sourceInfoFound,
            &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath,
            fullPath.c_str(),
            sourceInfo,
            watchFolder);

        // If this triggers, the flow for handling newly-created images needs to be examined further.
        // It's possible that we need to wait for some sort of asset processing event before we can get
        // the source asset ID.
        AZ_Warning("PaintableImageAssetHelper", sourceInfoFound, "Could not find source info for %s", fullPath.c_str());

        // Using the source asset ID, get or create an asset referencing using the expected product asset ID.
        // If we're overwriting an existing source asset, this will already exist, but if we're creating a new file,
        // the product asset won't exist yet.
        auto createdAsset = AZ::Data::AssetManager::Instance().FindOrCreateAsset(
            AZ::Data::AssetId(sourceInfo.m_assetId.m_guid, AZ::RPI::StreamingImageAsset::GetImageAssetSubId()),
            azrtti_typeid<AZ::RPI::StreamingImageAsset>(),
            AZ::Data::AssetLoadBehavior::PreLoad);

        // Set the asset hint to the source path so that we can display something reasonably correct in the component while waiting
        // for the product asset to get created.
        createdAsset.SetHint(relativePath);

        // Resync the configurations and refresh the display to hide the "Create" button
        // We need to use "Refresh_EntireTree" because "Refresh_AttributesAndValues" isn't enough to refresh the visibility
        // settings.
        AzToolsFramework::ToolsApplicationNotificationBus::Broadcast(
                &AzToolsFramework::ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplayForComponent,
                m_ownerEntityComponentIdPair,
                AzToolsFramework::Refresh_EntireTree);

        return createdAsset;
    }
} // namespace GradientSignal::ImageCreatorUtils

#include "PaintableImageAssetHelper.moc"
