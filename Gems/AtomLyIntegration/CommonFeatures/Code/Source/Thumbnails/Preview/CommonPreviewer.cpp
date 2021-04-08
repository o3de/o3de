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
#include <AzToolsFramework/Thumbnails/ThumbnailContext.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Thumbnails/Thumbnail.h>

#include <Source/Thumbnails/Preview/CommonPreviewer.h>
#include <Source/Thumbnails/ThumbnailUtils.h>

// Disables warning messages triggered by the Qt library
// 4251: class needs to have dll-interface to be used by clients of class 
// 4800: forcing value to bool 'true' or 'false' (performance warning)
AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option")
#include <Source/Thumbnails/Preview/ui_CommonPreviewer.h>
#include <QString>
#include <QResizeEvent>
AZ_POP_DISABLE_WARNING

namespace AZ
{
    namespace LyIntegration
    {
        static constexpr int CharWidth = 6;

        CommonPreviewer::CommonPreviewer(QWidget* parent)
            : Previewer(parent)
            , m_ui(new Ui::CommonPreviewerClass())
        {
            m_ui->setupUi(this);
        }

        CommonPreviewer::~CommonPreviewer()
        {
        }

        void CommonPreviewer::Display(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry)
        {
            using namespace AzToolsFramework::AssetBrowser;
            using namespace AzToolsFramework::Thumbnailer;

            auto thumbnailKey = entry->GetThumbnailKey();
            m_ui->m_previewWidget->SetThumbnailKey(thumbnailKey, ThumbnailContext::DefaultContext);
            m_fileInfo = QString::fromUtf8(entry->GetName().c_str());
            UpdateFileInfo();
        }

        const QString& CommonPreviewer::GetName() const
        {
            return m_name;
        }

        void CommonPreviewer::resizeEvent([[maybe_unused]] QResizeEvent* event)
        {
            m_ui->m_previewWidget->setMaximumHeight(m_ui->m_previewWidget->width());

            UpdateFileInfo();
        }

        void CommonPreviewer::UpdateFileInfo() const
        {
            m_ui->m_fileInfoLabel->setText(Thumbnails::WordWrap(m_fileInfo, m_ui->m_fileInfoLabel->width() / CharWidth));
        }
    } // namespace LyIntegration
} // namespace AZ

#include <Source/Thumbnails/Preview/moc_CommonPreviewer.cpp>
