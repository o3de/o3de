/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "EditorDefs.h"

#include "LegacyPreviewer.h"

// AzToolsFramework
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/EBusFindAssetTypeByName.h>

// Editor
#include "Util/Image.h"
#include "Util/ImageUtil.h"

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <AzAssetBrowser/Preview/ui_LegacyPreviewer.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING


static const int s_CharWidth = 6;
const QString LegacyPreviewer::Name{ QStringLiteral("LegacyPreviewer") };

LegacyPreviewer::LegacyPreviewer(QWidget* parent)
    : Previewer(parent)
    , m_ui(new Ui::LegacyPreviewerClass())
    , m_textureType(TextureType::RGB)
{
    m_ui->setupUi(this);
    m_ui->m_comboBoxRGB->addItems(QStringList() << "RGB" << "RGBA" << "Alpha");
    m_ui->m_previewCtrl->SetAspectRatio(4.0f / 3.0f);
    connect(m_ui->m_comboBoxRGB, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated), this,
        [=](int index)
    {
        m_textureType = static_cast<TextureType>(index);
        UpdateTextureType();
    });
    Clear();
}

LegacyPreviewer::~LegacyPreviewer()
{
}

void LegacyPreviewer::Clear() const
{
    m_ui->m_previewCtrl->ReleaseObject();
    m_ui->m_modelPreviewWidget->hide();
    m_ui->m_texturePreviewWidget->hide();
    m_ui->m_fileInfoCtrl->hide();
}

void LegacyPreviewer::Display(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry)
{
    using namespace AzToolsFramework::AssetBrowser;

    if (!entry)
    {
        Clear();
        return;
    }

    switch (entry->GetEntryType())
    {
        case AssetBrowserEntry::AssetEntryType::Source:
        {
            const SourceAssetBrowserEntry* sourceEntry = azrtti_cast<const SourceAssetBrowserEntry*>(entry);
            DisplaySource(sourceEntry);
            break;
        }
        case AssetBrowserEntry::AssetEntryType::Product:
            DisplayProduct(static_cast<const ProductAssetBrowserEntry*>(entry));
            break;
        default:
            Clear();
    }
}

const QString& LegacyPreviewer::GetName() const
{
    return Name;
}

void LegacyPreviewer::resizeEvent(QResizeEvent* /*event*/)
{
    m_ui->m_fileInfoCtrl->setText(WordWrap(m_fileinfo, m_ui->m_fileInfoCtrl->width() / s_CharWidth));
}

bool LegacyPreviewer::DisplayProduct(const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry* product)
{
    m_ui->m_fileInfoCtrl->show();

    m_fileinfo = QString::fromUtf8(product->GetName().c_str());

    m_fileinfo += GetFileSize(product->GetRelativePath().c_str());

    EBusFindAssetTypeByName meshAssetTypeResult("Static Mesh");
    AZ::AssetTypeInfoBus::BroadcastResult(meshAssetTypeResult, &AZ::AssetTypeInfo::GetAssetType);

    QString filename(product->GetRelativePath().c_str());

    // Find item.
    if (product->GetAssetType() == meshAssetTypeResult.GetAssetType())
    {
        m_ui->m_modelPreviewWidget->show();
        m_ui->m_texturePreviewWidget->hide();
        m_ui->m_previewCtrl->LoadFile(filename);

        int nVertexCount = m_ui->m_previewCtrl->GetVertexCount();
        int nFaceCount = m_ui->m_previewCtrl->GetFaceCount();
        int nMaxLod = m_ui->m_previewCtrl->GetMaxLod();
        int nMtls = m_ui->m_previewCtrl->GetMtlCount();
        if (nFaceCount > 0)
        {
            m_fileinfo += tr("\r\n%1 Faces\r\n%2 Verts\r\n%3 MaxLod\r\n%4 Materials").arg(nFaceCount).arg(nVertexCount).arg(nMaxLod).arg(nMtls);
        }
        m_ui->m_fileInfoCtrl->setText(WordWrap(m_fileinfo, m_ui->m_fileInfoCtrl->width() / s_CharWidth));
        updateGeometry();
        return true;
    }

    EBusFindAssetTypeByName textureAssetTypeResult("Texture");
    AZ::AssetTypeInfoBus::BroadcastResult(textureAssetTypeResult, &AZ::AssetTypeInfo::GetAssetType);

    if (product->GetAssetType() == textureAssetTypeResult.GetAssetType())
    {
        // Get full product file path
        const char* assetCachePath = AZ::IO::FileIOBase::GetInstance()->GetAlias("@assets@");
        AZStd::string productFullPath;
        AzFramework::StringFunc::Path::Join(assetCachePath, product->GetRelativePath().c_str(), productFullPath);
        if (AZ::IO::FileIOBase::GetInstance()->Exists(productFullPath.c_str()))
        {
            // Try to display it in modern dds image loader, if no one exists, use the legacy image loader
            bool foundPixmap = DisplayTextureProductModern(productFullPath.c_str());
            return foundPixmap ? foundPixmap : DisplayTextureLegacy(productFullPath.c_str());
        }
        else
        {
            // If we cannot find the product file, means it's not treated as an asset, display its source
            return DisplayTextureLegacy(product->GetFullPath().c_str());
        }
    }

    Clear();
    return false;
}

void LegacyPreviewer::DisplaySource(const AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry* source)
{
    using namespace AzToolsFramework::AssetBrowser;

    EBusFindAssetTypeByName textureAssetType("Texture");
    AZ::AssetTypeInfoBus::BroadcastResult(textureAssetType, &AZ::AssetTypeInfo::GetAssetType);

    if (source->GetPrimaryAssetType() == textureAssetType.GetAssetType())
    {
        m_ui->m_fileInfoCtrl->show();
        m_fileinfo = QString::fromUtf8(source->GetName().c_str());
        m_fileinfo += GetFileSize(source->GetFullPath().c_str());
        const char* fullSourcePath = source->GetFullPath().c_str();
        // If it's a source dds file, try to display it using modern way
        if (AzFramework::StringFunc::Path::IsExtension(fullSourcePath, "dds", false))
        {
            if (DisplayTextureProductModern(fullSourcePath))
            {
                return;
            }
        }
        DisplayTextureLegacy(source->GetFullPath().c_str());
    }
    else
    {
        AZStd::vector<const ProductAssetBrowserEntry*> products;
        source->GetChildrenRecursively<ProductAssetBrowserEntry>(products);
        if (products.empty())
        {
            Clear();
        }
        else
        {
            for (auto* product : products)
            {
                if (DisplayProduct(product))
                {
                    break;
                }
            }
        }
    }
}

QString LegacyPreviewer::GetFileSize(const char* path)
{
    QString fileSizeStr;
    AZ::u64 fileSizeResult = 0;
    if (AZ::IO::FileIOBase::GetInstance()->Size(path, fileSizeResult))
    {
        static double kb = 1024.0f;
        static double mb = kb * 1024.0;
        static double gb = mb * 1024.0;

        static QString byteStr = "B";
        static QString kbStr = "KB";
        static QString mbStr = "MB";
        static QString gbStr = "GB";

#if AZ_TRAIT_OS_PLATFORM_APPLE
        kb = 1000.0;
        mb = kb * 1000.0;
        gb = mb * 1000.0;

        kbStr = "kB";
        mbStr = "mB";
        gbStr = "gB";
#endif // AZ_TRAIT_OS_PLATFORM_APPLE

        if (fileSizeResult < kb)
        {
            fileSizeStr += tr("\r\nFile Size: %1%2").arg(QString::number(fileSizeResult), byteStr);
        }
        else if (fileSizeResult < mb)
        {
            double size = fileSizeResult / kb;
            fileSizeStr += tr("\r\nFile Size: %1%2").arg(QString::number(size, 'f', 2), kbStr);
        }
        else if (fileSizeResult < gb)
        {
            double size = fileSizeResult / mb;
            fileSizeStr += tr("\r\nFile Size: %1%2").arg(QString::number(size, 'f', 2), mbStr);
        }
        else
        {
            double size = fileSizeResult / gb;
            fileSizeStr += tr("\r\nFile Size: %1%2").arg(QString::number(size, 'f', 2), gbStr);
        }
    }
    return fileSizeStr;
}

bool LegacyPreviewer::DisplayTextureLegacy(const char* fullImagePath)
{
    m_ui->m_modelPreviewWidget->hide();
    m_ui->m_texturePreviewWidget->show();

    bool foundPixmap = false;
    if (!AZ::IO::FileIOBase::GetInstance()->IsDirectory(fullImagePath))
    {
        QString strLoadFilename = QString(fullImagePath);
        if (CImageUtil::LoadImage(strLoadFilename, m_previewImageSource))
        {
            m_fileinfo += QStringLiteral("\r\n%1x%2\r\n%3")
                .arg(m_previewImageSource.GetWidth())
                .arg(m_previewImageSource.GetHeight())
                .arg(m_previewImageSource.GetFormatDescription());

            m_fileinfoAlphaTexture = m_fileinfo;
            UpdateTextureType();
            foundPixmap = true;
        }
    }

    if (!foundPixmap)
    {
        m_ui->m_previewImageCtrl->setPixmap(QPixmap());
        m_ui->m_fileInfoCtrl->setText(WordWrap(m_fileinfo, m_ui->m_fileInfoCtrl->width() / s_CharWidth));
    }

    updateGeometry();

    return foundPixmap;
}

bool LegacyPreviewer::DisplayTextureProductModern(const char* fullProductImagePath)
{
    m_ui->m_modelPreviewWidget->hide();
    m_ui->m_texturePreviewWidget->show();

    bool foundPixmap = false;
    QImage previewImage;
    AZStd::string productInfo;
    AZStd::string productAlphaInfo;
    AzToolsFramework::AssetBrowser::AssetBrowserTexturePreviewRequestsBus::BroadcastResult(foundPixmap, &AzToolsFramework::AssetBrowser::AssetBrowserTexturePreviewRequests::GetProductTexturePreview, fullProductImagePath, previewImage, productInfo, productAlphaInfo);

    if (foundPixmap)
    {
        QPixmap pix = QPixmap::fromImage(previewImage);
        m_ui->m_previewImageCtrl->setPixmap(pix);
        m_ui->m_previewImageCtrl->updateGeometry();
        CImageUtil::QImageToImage(previewImage, m_previewImageSource);

        m_fileinfo += QStringLiteral("\r\n%1x%2\r\n%3")
            .arg(m_previewImageSource.GetWidth())
            .arg(m_previewImageSource.GetHeight())
            .arg(m_previewImageSource.GetFormatDescription());

        m_fileinfoAlphaTexture = m_fileinfo;

        m_fileinfo += QString(productInfo.c_str());
        if (productAlphaInfo.empty())
        {
            // If there is no separate info for alpha, use the image info
            m_fileinfoAlphaTexture += QString(productInfo.c_str());
        }
        else
        {
            m_fileinfoAlphaTexture += QString(productAlphaInfo.c_str());
        }


        UpdateTextureType();
    }
    else
    {
        m_ui->m_previewImageCtrl->setPixmap(QPixmap());
        m_ui->m_fileInfoCtrl->setText(WordWrap(m_fileinfo, m_ui->m_fileInfoCtrl->width() / s_CharWidth));
    }

    updateGeometry();
    return foundPixmap;
}

void LegacyPreviewer::UpdateTextureType()
{
    m_previewImageUpdated.Copy(m_previewImageSource);

    switch (m_textureType)
    {
        case TextureType::RGB:
        {
            m_previewImageUpdated.SwapRedAndBlue();
            m_previewImageUpdated.FillAlpha();
            break;
        }
        case TextureType::RGBA:
        {
            m_previewImageUpdated.SwapRedAndBlue();
            break;
        }
        case TextureType::Alpha:
        {
            for (int h = 0; h < m_previewImageUpdated.GetHeight(); h++)
            {
                for (int w = 0; w < m_previewImageUpdated.GetWidth(); w++)
                {
                    int a = m_previewImageUpdated.ValueAt(w, h) >> 24;
                    m_previewImageUpdated.ValueAt(w, h) = RGB(a, a, a) | 0xFF000000;
                }
            }
            break;
        }
    }
    // note that Qt will not deep copy the data, so WE MUST KEEP THE IMAGE DATA AROUND!
    QPixmap qtPixmap = QPixmap::fromImage(
        QImage(reinterpret_cast<uchar*>(m_previewImageUpdated.GetData()), m_previewImageUpdated.GetWidth(), m_previewImageUpdated.GetHeight(), QImage::Format_ARGB32));
    m_ui->m_previewImageCtrl->setPixmap(qtPixmap);
    m_ui->m_fileInfoCtrl->setText(WordWrap(m_textureType == TextureType::Alpha? m_fileinfoAlphaTexture: m_fileinfo, m_ui->m_fileInfoCtrl->width() / s_CharWidth));
    m_ui->m_previewImageCtrl->updateGeometry();
}

bool LegacyPreviewer::FileInfoCompare(const FileInfo& f1, const FileInfo& f2)
{
    if ((f1.attrib & _A_SUBDIR) && !(f2.attrib & _A_SUBDIR))
    {
        return true;
    }
    if (!(f1.attrib & _A_SUBDIR) && (f2.attrib & _A_SUBDIR))
    {
        return false;
    }

    return QString::compare(f1.filename, f2.filename, Qt::CaseInsensitive) < 0;
}

QString LegacyPreviewer::WordWrap(const QString& string, int maxLength)
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

#include <AzAssetBrowser/Preview/moc_LegacyPreviewer.cpp>
