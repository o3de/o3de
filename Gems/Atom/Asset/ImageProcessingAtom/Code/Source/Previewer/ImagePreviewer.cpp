/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformDef.h>
AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <Source/Previewer/ImagePreviewer.h>
#include <Source/Previewer/ui_ImagePreviewer.h>
#include <Editor/EditorCommon.h>

#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzCore/IO/FileIO.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/EBusFindAssetTypeByName.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <ImageLoader/ImageLoaders.h>
#include <Processing/ImageConvert.h>
#include <Processing/ImagePreview.h>
#include <Processing/ImageToProcess.h>
#include <Processing/Utils.h>
#include <Processing/ImageFlags.h>

#include <QString>
#include <QStringList>

AZ_POP_DISABLE_WARNING

namespace ImageProcessingAtom
{
    static constexpr const int IMAGE_PREVIEWER_CHAR_WIDTH = 6;

    ImagePreviewer::ImagePreviewer(QWidget* parent)
        : Previewer(parent)
        , m_ui(new Ui::ImagePreviewerClass())
        , m_imageAssetLoader(aznew Utils::AsyncImageAssetLoader())
    {
        m_ui->setupUi(this);
        Clear();
    }

    ImagePreviewer::~ImagePreviewer()
    {
        AZ::SystemTickBus::Handler::BusDisconnect();

        if (m_createDisplayTextureResult.isRunning())
        {
            m_createDisplayTextureResult.waitForFinished();
        }
    }

    void ImagePreviewer::Clear() const
    {
        m_ui->m_texturePreviewWidget->hide();
        m_ui->m_fileInfoCtrl->hide();
    }

    void ImagePreviewer::Display(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry)
    {
        using namespace AzToolsFramework::AssetBrowser;
        
        m_previewImageObject = nullptr;

        Clear();
        switch (entry->GetEntryType())
        {
        case AssetBrowserEntry::AssetEntryType::Source:
            DisplaySource(static_cast<const SourceAssetBrowserEntry*>(entry));
            break;
        case AssetBrowserEntry::AssetEntryType::Product:
            DisplayProduct(static_cast<const ProductAssetBrowserEntry*>(entry));
            break;
        }
    }

    const QString& ImagePreviewer::GetName() const
    {
        return m_name;
    }

    void ImagePreviewer::resizeEvent(QResizeEvent* event)
    {
        AZ_UNUSED(event);
        m_ui->m_fileInfoCtrl->setText(WordWrap(m_fileinfo, m_ui->m_fileInfoCtrl->width() / IMAGE_PREVIEWER_CHAR_WIDTH));
    }
    
    // Get the preview of a sub-image specified by mip
    QImage GetSubImagePreview(IImageObjectPtr image, uint32_t mip)
    {
        AZ::u8* imageBuf;
        AZ::u32 pitch;
        image->GetImagePointer(mip, imageBuf, pitch);
        const AZ::u32 width = image->GetWidth(mip);
        const AZ::u32 height = image->GetHeight(mip);
        return QImage(imageBuf, width, height, pitch, QImage::Format_RGBA8888);
    }

    // From image info string from ImageObject
    void GetImageInfoString(IImageObjectPtr image, AZStd::string& output)
    {
        if (!image)
        {
            return;
        }

        output += AZStd::string::format("Dimensions: %dx%d\r\n", image->GetWidth(0), image->GetHeight(0));

        AZ::u32 mipCount = image->GetMipCount();
        output += AZStd::string::format("Mips: %d\r\n", mipCount);

        AZ::u32 memSize = image->GetTextureMemory();
        AZStd::string memSizeString = ImageProcessingAtomEditor::EditorHelper::GetFileSizeString(memSize);
        output += AZStd::string::format("Size: %s\r\n", memSizeString.c_str());

        CPixelFormats& pixelFormats = CPixelFormats::GetInstance();
        EPixelFormat format = image->GetPixelFormat();
        const PixelFormatInfo* info = pixelFormats.GetPixelFormatInfo(format);
        if (info)
        {
            output += AZStd::string::format("Format: %s\r\n", info->szName);
        }

        AZStd::string colorSpaceString;
        if (image->HasImageFlags(EIF_SRGBRead))
        {
            colorSpaceString = "sRGB";
        }
        else
        {
            // If the flag isn't set, then we don't know whether it's linear, or it's sRGB and the file format doesn't support a color space flag.
            colorSpaceString = "Unknown";
        }

        output += AZStd::string::format("Color Space: %s\r\n", colorSpaceString.c_str());

        if (image->HasImageFlags(EIF_Cubemap))
        {
            output += AZStd::string::format("Cubemap\r\n");
        }
    }

    // From image info string from StreamingImageAsset
    void GetImageInfoString(const AZ::Data::Asset<AZ::RPI::StreamingImageAsset>& imageAsset, AZStd::string& output)
    {
        if (!imageAsset.IsReady())
        {
            return;
        }

        AZ::RHI::ImageDescriptor descriptor = imageAsset->GetImageDescriptor();

        output += AZStd::string("\r\nOverall image data...\r\n");

        output += AZStd::string::format("Dimensions: %dx%d\r\n", descriptor.m_size.m_width, descriptor.m_size.m_height);

        AZStd::string memSizeString;
        if (imageAsset->GetTotalImageDataSize() > 0)
        {
            memSizeString = ImageProcessingAtomEditor::EditorHelper::GetFileSizeString(imageAsset->GetTotalImageDataSize());
        }
        else
        {
            memSizeString = "Unknown";
        }
        output += AZStd::string::format("Size: %s\r\n", memSizeString.c_str());

        uint32_t totalMipCount = descriptor.m_mipLevels;
        output += AZStd::string::format("Mips: %d\r\n", totalMipCount);

        output += AZStd::string::format("Format: %s\r\n", AZ::RHI::ToString(descriptor.m_format));

        //[GFX TODO] Put this back in, pending in my other CL
        //output += AZStd::string::format("Color Space: %s\r\n", AZ::RHI::ToString(descriptor.m_colorSpace));

        if (descriptor.m_isCubemap)
        {
            output += AZStd::string::format("Cubemap: true\r\n");
        }


        output += AZStd::string("\r\nBuilt-in mip chain...\r\n");

        uint32_t builtInMipCount = imageAsset->GetTailMipChain().GetMipLevelCount();
        output += AZStd::string::format("Mips: %d\r\n", builtInMipCount);

        memSizeString = ImageProcessingAtomEditor::EditorHelper::GetFileSizeString(imageAsset->GetTailMipChain().GetImageDataSize());
        output += AZStd::string::format("Size: %s\r\n", memSizeString.c_str());

        AZ::RHI::Size builtInMipSizeA = imageAsset->GetTailMipChain().GetSubImageLayout(0).m_size;
        AZStd::string mipChainDimensionString;
        if (builtInMipCount == 1)
        {
            mipChainDimensionString = AZStd::string::format("%dx%d", builtInMipSizeA.m_width, builtInMipSizeA.m_height);
        }
        else
        {
            AZ::RHI::Size builtInMipSizeB = imageAsset->GetTailMipChain().GetSubImageLayout(builtInMipCount - 1).m_size;
            mipChainDimensionString = AZStd::string::format("%dx%d .. %dx%d", builtInMipSizeA.m_width, builtInMipSizeA.m_height, builtInMipSizeB.m_width, builtInMipSizeB.m_height);
        }
        output += AZStd::string::format("Dimensions: %s\r\n", mipChainDimensionString.c_str());

        output += AZStd::string("\r\n");

        size_t additionalMipChainCount = imageAsset->GetMipChainCount() - 1;
        output += AZStd::string::format("Additional Mip Chains: %zu\r\n", additionalMipChainCount);
    }

    void ImagePreviewer::DisplayProduct(const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry* product)
    {
        m_ui->m_fileInfoCtrl->show();
        m_fileinfo = QString::fromUtf8(product->GetName().c_str());
        m_fileinfo += GetFileSize(product->GetRelativePath().c_str());

        m_imageAssetLoader->QueueAsset(
            product->GetAssetId(),
            [this](const AZ::Data::Asset<AZ::RPI::StreamingImageAsset>& asset)
            {
                CreateAndDisplayTextureItemAsync(
                    [asset]() -> CreateDisplayTextureResult
                    {
                        if (IImageObjectPtr image = Utils::LoadImageFromImageAsset(asset); image)
                        {
                            // Add product image info
                            AZStd::string productInfo;
                            GetImageInfoString(asset, productInfo);

                            QString fileInfo = QStringLiteral("\r\n");
                            fileInfo += productInfo.c_str();

                            return { ConvertImageForPreview(image), fileInfo };
                        }

                        return { nullptr, "" };
                    });
            });

        DisplayTextureItem();
    }

    void ImagePreviewer::DisplaySource(const AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry* source)
    {
        m_ui->m_fileInfoCtrl->show();
        m_fileinfo = QString::fromUtf8(source->GetName().c_str());
        m_fileinfo += GetFileSize(source->GetFullPath().c_str());

        CreateAndDisplayTextureItemAsync(
            [fullPath = source->GetFullPath()]() -> CreateDisplayTextureResult
            {
                if (IImageObjectPtr image = IImageObjectPtr(LoadImageFromFile(fullPath)); image)
                {
                    // Add source image info
                    AZStd::string sourceInfo;
                    GetImageInfoString(image, sourceInfo);

                    QString fileInfo = QStringLiteral("\r\n");
                    fileInfo += sourceInfo.c_str();

                    return { ConvertImageForPreview(image), fileInfo };
                }

                return { nullptr, "" };
            });

        DisplayTextureItem();
    }

    QString ImagePreviewer::GetFileSize(const char* path)
    {
        QString fileSizeStr;
        AZ::u64 fileSizeResult = 0;
        if (AZ::IO::FileIOBase::GetInstance()->Size(path, fileSizeResult))
        {
            AZStd::string fileSizeString = ImageProcessingAtomEditor::EditorHelper::GetFileSizeString(fileSizeResult);

            fileSizeStr += tr("\r\nFile Size: ");
            fileSizeStr += fileSizeString.c_str();
        }
        return fileSizeStr;
    }

    void ImagePreviewer::DisplayTextureItem()
    {
        m_ui->m_texturePreviewWidget->show();

        if (m_previewImageObject)
        {
            // Display mip 0 by default
            PreviewSubImage(0);
        }
        else
        {
            m_ui->m_previewImageCtrl->setPixmap(QPixmap());
        }
        
        m_ui->m_fileInfoCtrl->setText(WordWrap(m_fileinfo, m_ui->m_fileInfoCtrl->width() / IMAGE_PREVIEWER_CHAR_WIDTH));

        updateGeometry();
    }

    template<class CreateFn>
    void ImagePreviewer::CreateAndDisplayTextureItemAsync(CreateFn create)
    {
        AZ::SystemTickBus::Handler::BusConnect();
        m_createDisplayTextureResult = QtConcurrent::run(AZStd::move(create));
    }

    void ImagePreviewer::OnSystemTick()
    {
        if (m_createDisplayTextureResult.isFinished())
        {
            CreateDisplayTextureResult result = m_createDisplayTextureResult.result();
            m_previewImageObject = AZStd::move(result.first);
            m_fileinfo += result.second;

            AZ::SystemTickBus::Handler::BusDisconnect();

            DisplayTextureItem();
        }
    }

    void ImagePreviewer::PreviewSubImage(uint32_t mip)
    {
        QImage previewImage = GetSubImagePreview(m_previewImageObject, mip);

        QPixmap pix = QPixmap::fromImage(previewImage);
        m_ui->m_previewImageCtrl->setPixmap(pix);
        m_ui->m_previewImageCtrl->updateGeometry();
    }

    QString ImagePreviewer::WordWrap(const QString& string, int maxLength)
    {
        QString result;
        int length = 0;

        for (auto c : string)
        {
            if (c == '\n')
            {
                length = 0;
            }
            else if (length > maxLength)
            {
                result.append('\n');
                length = 0;
            }
            else
            {
                length++;
            }
            result.append(c);
        }
        return result;
    }
} // namespace ImageProcessingAtom

#include <Source/Previewer/moc_ImagePreviewer.cpp>
