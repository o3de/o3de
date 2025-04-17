/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GradientSignal/Editor/EditorGradientImageCreatorUtils.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzFramework/IO/FileOperations.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>

#if !defined(Q_MOC_RUN)
#include <QApplication>
#include <QMessageBox>
#include <QProgressDialog>
#endif

AZ_PUSH_DISABLE_WARNING(4777, "-Wunknown-warning-option")
#include <OpenImageIO/imageio.h>
AZ_POP_DISABLE_WARNING

namespace GradientSignal::ImageCreatorUtils
{
    AZStd::string GetSupportedImagesFilter()
    {
        // Build filter for supported streaming image formats that will be used on the
        // native file dialog when creating/picking an output file for the painted image.
        // ImageProcessingAtom::s_SupportedImageExtensions actually has more formats
        // that will produce streaming image assets, but not all of them support
        // all of the bit depths we care about (8/16/32), so we've reduced the list
        // to the image formats that do.
        return "Images (*.png *.tif *.tiff *.tga *.exr)";
    }

    AZStd::vector<AZ::Edit::EnumConstant<OutputFormat>> SupportedOutputFormatOptions()
    {
        return { AZ::Edit::EnumConstant<OutputFormat>(OutputFormat::R8, "R8 (8-bit)"),
                    AZ::Edit::EnumConstant<OutputFormat>(OutputFormat::R16, "R16 (16-bit)"),
                    AZ::Edit::EnumConstant<OutputFormat>(OutputFormat::R32, "R32 (32-bit)") };
    }

    int GetChannels(OutputFormat format)
    {
        switch (format)
        {
        case OutputFormat::R8:
            return 1;
        case OutputFormat::R16:
            return 1;
        case OutputFormat::R32:
            return 1;
        case OutputFormat::R8G8B8A8:
            return 4;
        default:
            AZ_Assert(false, "Unsupported output image format (%d)", format);
            return 0;
        }
    }

    int GetBytesPerChannel(OutputFormat format)
    {
        switch (format)
        {
        case OutputFormat::R8:
            return 1;
        case OutputFormat::R16:
            return 2;
        case OutputFormat::R32:
            return 4;
        case OutputFormat::R8G8B8A8:
            return 1;
        default:
            AZ_Assert(false, "Unsupported output image format (%d)", format);
            return 0;
        }
    }

    AZStd::vector<AZ::u8> CreateDefaultImageBuffer(int imageResolutionX, int imageResolutionY, int channels, OutputFormat format)
    {
        // Fill in our image buffer. Default all values to 0 (black)
        int bytesPerChannel = ImageCreatorUtils::GetBytesPerChannel(format);
        const size_t imageSize = imageResolutionX * imageResolutionY * channels * bytesPerChannel;
        AZStd::vector<AZ::u8> pixels(imageSize, 0);

        // If we're saving a 4-channel image, loop through and set the Alpha channel to opaque.
        if (channels == 4)
        {
            for (size_t alphaIndex = (channels - 1); alphaIndex < (imageResolutionX * imageResolutionY * channels); alphaIndex += channels)
            {
                switch (format)
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
                case OutputFormat::R8G8B8A8:
                    {
                        pixels[alphaIndex] = std::numeric_limits<AZ::u8>::max();
                        break;
                    }
                }
            }
        }

        return pixels;
    }


    bool WriteImage(
        const AZStd::string& absoluteFileName,
        int imageResolutionX,
        int imageResolutionY,
        int channels,
        OutputFormat format,
        const AZStd::span<const AZ::u8>& pixelBuffer,
        bool showProgressDialog)
    {
        OIIO::TypeDesc pixelFormat = OIIO::TypeDesc::UINT8;
        switch (format)
        {
        case OutputFormat::R8:
            pixelFormat = OIIO::TypeDesc::UINT8;
            break;
        case OutputFormat::R16:
            pixelFormat = OIIO::TypeDesc::UINT16;
            break;
        case OutputFormat::R32:
            pixelFormat = OIIO::TypeDesc::FLOAT;
            break;
        case OutputFormat::R8G8B8A8:
            pixelFormat = OIIO::TypeDesc::UINT8;
            break;
        default:
            AZ_Assert(false, "Unsupported output image format (%d)", format);
            return false;
        }

        // We *could* declare this as a local variable and just never show it, but then calling WriteImage would always
        // require Qt to be started. This way, we can call WriteImage from unit tests without starting Qt as long as we
        // set showProgressDialog = false.
        QProgressDialog* saveDialog = nullptr;

        // Show a dialog box letting the user know the image is being written out.
        // For large image sizes, it can take 15+ seconds to create and write out the image.
        if (showProgressDialog)
        {
            saveDialog = new QProgressDialog(AzToolsFramework::GetActiveWindow());
            saveDialog->setWindowFlags(saveDialog->windowFlags() & ~Qt::WindowCloseButtonHint);
            saveDialog->setLabelText("Saving image...");
            saveDialog->setWindowModality(Qt::WindowModal);
            saveDialog->setMaximumSize(QSize(256, 96));
            saveDialog->setMinimum(0);
            saveDialog->setMaximum(100);
            saveDialog->setMinimumDuration(0);
            saveDialog->setAutoClose(false);
            saveDialog->setCancelButton(nullptr);
            saveDialog->show();
            QApplication::processEvents();
        }

        AZ::IO::Path fullPathIO(absoluteFileName);
        AZ::IO::Path absolutePath = fullPathIO.LexicallyNormal();

        // Give our progress dialog another chance to update so we don't look frozen.
        if (showProgressDialog)
        {
            saveDialog->setValue(1);
            QApplication::processEvents();
        }

        // check out the file in source control if source control exists.
        bool checkedOutSuccessfully = true;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            checkedOutSuccessfully,
            &AzToolsFramework::ToolsApplicationRequestBus::Events::RequestEditForFileBlocking,
            absolutePath.c_str(),
            "Checking out for edit...",
            AzToolsFramework::ToolsApplicationRequestBus::Events::RequestEditProgressCallback());

        if (!checkedOutSuccessfully)
        {
            AZ_Error("EditorImageGradientComponent", false, "Failed to check out file from source control: %s", absolutePath.c_str());
            delete saveDialog;
            return false;
        }

        // Create and save the image on disk. We initially save it to a temporary name so that the Asset Processor won't start
        // processing it, and then we'll move it to the correct name at the end.
        AZStd::string tempSavePath;
        AZ::IO::CreateTempFileName(absolutePath.c_str(), tempSavePath);

        std::unique_ptr<OIIO::ImageOutput> outputImage = OIIO::ImageOutput::create(tempSavePath.c_str());
        if (!outputImage)
        {
            AZ_Error("EditorImageGradientComponent", false, "Failed to create image at path: %s", tempSavePath.c_str());
            delete saveDialog;
            return false;
        }

        OIIO::ImageSpec spec(imageResolutionX, imageResolutionY, channels, pixelFormat);
        outputImage->open(tempSavePath.c_str(), spec);

        // Callback to upgrade our progress dialog during image saving.
        auto WriteProgressCallback = [](void* opaqueData, float percentDone) -> bool
        {
            QProgressDialog* saveDialog = reinterpret_cast<QProgressDialog*>(opaqueData);
            if (saveDialog && saveDialog->isVisible())
            {
                saveDialog->setValue(aznumeric_cast<int>(percentDone * 100.0f));
                QApplication::processEvents();
            }
            return false;
        };

        bool writeResult = outputImage->write_image(
            pixelFormat, pixelBuffer.data(), OIIO::AutoStride, OIIO::AutoStride, OIIO::AutoStride, WriteProgressCallback, saveDialog);
        if (!writeResult)
        {
            AZ_Error("EditorImageGradientComponent", writeResult, "Failed to write out gradient image to path: %s", tempSavePath.c_str());
        }

        outputImage->close();

        // Now that we're done saving the temporary image, rename it to the correct file name.
        auto moveResult = AZ::IO::SmartMove(tempSavePath.c_str(), absolutePath.c_str());
        AZ_Error("EditorImageGradientComponent", moveResult,
            "Failed to rename temporary image asset %s to %s", tempSavePath.c_str(), absolutePath.c_str());

        delete saveDialog;
        return writeResult && moveResult;
    }

    AZStd::string GetDefaultImageSourcePath(const AZ::Data::AssetId& imageAssetId, const AZStd::string& defaultFileName)
    {
        // If the image asset ID is valid, try getting the source asset path to use as the default source path.
        // Otherwise, create a new name.
        if (imageAssetId.IsValid())
        {
            AZStd::string sourcePath;
            bool sourceFileFound = false;
            AZ::Data::AssetInfo assetInfo;
            AZStd::string watchFolder;

            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                sourceFileFound,
                &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourceUUID,
                imageAssetId.m_guid,
                assetInfo,
                watchFolder);

            if (sourceFileFound)
            {
                bool success =
                    AzFramework::StringFunc::Path::ConstructFull(watchFolder.c_str(), assetInfo.m_relativePath.c_str(), sourcePath, true);

                if (success)
                {
                    return sourcePath;
                }
            }
        }

        // Invalid image asset or failed path creation, try creating a new name.
        AZ::IO::Path defaultPath;
        if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
        {
            settingsRegistry->Get(defaultPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath);
        }

        defaultPath /= AZ::IO::FixedMaxPathString(AZ::RPI::AssetUtils::SanitizeFileName(defaultFileName));

        return defaultPath.Native();
    }
}
