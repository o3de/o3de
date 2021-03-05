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

#include <AzCore/IO/FileIO.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzCore/Math/MatrixUtils.h>
#include <AzToolsFramework/Thumbnails/ThumbnailContext.h>
#include <AzToolsFramework/AssetBrowser/Thumbnails/SourceThumbnail.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

#include <Source/Material/Preview/MaterialPreviewer.h>

// Disables warning messages triggered by the Qt library
// 4251: class needs to have dll-interface to be used by clients of class 
// 4800: forcing value to bool 'true' or 'false' (performance warning)
AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option")
#include <Source/Material/Preview/ui_MaterialPreviewer.h>
#include <QString>
#include <QResizeEvent>
AZ_POP_DISABLE_WARNING

namespace AZ
{
    namespace LyIntegration
    {
        static constexpr int CharWidth = 6;

        MaterialPreviewer::MaterialPreviewer(QWidget* parent)
            : Previewer(parent)
            , m_ui(new Ui::MaterialPreviewerClass())
        {
            m_ui->setupUi(this);
        }

        MaterialPreviewer::~MaterialPreviewer()
        {
        }

        void MaterialPreviewer::Display(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry)
        {
            using namespace AzToolsFramework::AssetBrowser;

            if (entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Source)
            {
                const auto source = azrtti_cast<const SourceAssetBrowserEntry*>(entry);
                if (source->GetChildCount() > 0)
                {
                    const auto product = azrtti_cast<const ProductAssetBrowserEntry*>(source->GetChild(0));
                    if (product)
                    {
                        DisplayInternal(product);
                    }
                }
            }
            else if (entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Product)
            {
                const auto product = azrtti_cast<const ProductAssetBrowserEntry*>(entry);
                DisplayInternal(product);
            }
        }

        const QString& MaterialPreviewer::GetName() const
        {
            return m_name;
        }

        void MaterialPreviewer::resizeEvent([[maybe_unused]] QResizeEvent* event)
        {
            m_ui->m_materialPreviewWidget->setMaximumHeight(m_ui->m_materialPreviewWidget->width());

            UpdateFileInfo();
        }

        void MaterialPreviewer::DisplayInternal(const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry* product)
        {
            using namespace AzToolsFramework;
            using namespace Thumbnailer;

            if (product->GetAssetId() == m_assetId)
            {
                return;
            }

            m_assetId = product->GetAssetId();
            m_fileInfo = QString::fromUtf8(product->GetParent()->GetName().c_str());

            bool result = false;
            AZ::Data::AssetInfo assetInfo;
            AZ::Data::AssetType assetType;
            AZStd::string rootFilePath;
            const AZStd::string platformName = ""; // Empty for default
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(result, &AzToolsFramework::AssetSystemRequestBus::Events::GetAssetInfoById,
                m_assetId, assetType, platformName, assetInfo, rootFilePath);

            if (!result)
            {
                return;
            }

            AZStd::string fullSourcePath = AZStd::string::format("%s/%s", rootFilePath.c_str(), assetInfo.m_relativePath.c_str());
            SharedThumbnailKey thumbnailKey = MAKE_TKEY(AzToolsFramework::AssetBrowser::SourceThumbnailKey, fullSourcePath.c_str());
            m_ui->m_materialPreviewWidget->SetThumbnailKey(thumbnailKey, Thumbnailer::ThumbnailContext::DefaultContext);

            UpdateFileInfo();
        }

        void MaterialPreviewer::UpdateFileInfo() const
        {
            m_ui->m_fileInfoLabel->setText(WordWrap(m_fileInfo, m_ui->m_fileInfoLabel->width() / CharWidth));
        }

        QString MaterialPreviewer::WordWrap(const QString& string, int maxLength)
        {
            QString result;
            int length = 0;

            for (const QChar& c : string)
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
    } // namespace LyIntegration
} // namespace AZ

#include <Source/Material/Preview/moc_MaterialPreviewer.cpp>
