/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/FileIO.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/Thumbnails/Thumbnail.h>
#include <AzToolsFramework/Thumbnails/ThumbnailContext.h>
#include <SharedPreview/SharedPreviewUtils.h>
#include <SharedPreview/SharedPreviewer.h>

// Disables warning messages triggered by the Qt library
// 4251: class needs to have dll-interface to be used by clients of class
// 4800: forcing value to bool 'true' or 'false' (performance warning)
AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option")
#include <QResizeEvent>
#include <QString>
#include <SharedPreview/ui_SharedPreviewer.h>
AZ_POP_DISABLE_WARNING

namespace AZ
{
    namespace LyIntegration
    {
        static constexpr int CharWidth = 6;

        SharedPreviewer::SharedPreviewer(QWidget* parent)
            : Previewer(parent)
            , m_ui(new Ui::SharedPreviewerClass())
        {
            m_ui->setupUi(this);
        }

        SharedPreviewer::~SharedPreviewer()
        {
        }

        void SharedPreviewer::Clear() const
        {
        }

        void SharedPreviewer::Display(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry)
        {
            using namespace AzToolsFramework::AssetBrowser;
            using namespace AzToolsFramework::Thumbnailer;

            auto thumbnailKey = entry->GetThumbnailKey();
            m_ui->m_previewWidget->SetThumbnailKey(thumbnailKey, ThumbnailContext::DefaultContext);
            m_fileInfo = QString::fromUtf8(entry->GetName().c_str());
            UpdateFileInfo();
        }

        const QString& SharedPreviewer::GetName() const
        {
            return m_name;
        }

        void SharedPreviewer::resizeEvent([[maybe_unused]] QResizeEvent* event)
        {
            m_ui->m_previewWidget->setMaximumHeight(m_ui->m_previewWidget->width());

            UpdateFileInfo();
        }

        void SharedPreviewer::UpdateFileInfo() const
        {
            m_ui->m_fileInfoLabel->setText(SharedPreviewUtils::WordWrap(m_fileInfo, m_ui->m_fileInfoLabel->width() / CharWidth));
        }
    } // namespace LyIntegration
} // namespace AZ

#include <SharedPreview/moc_SharedPreviewer.cpp>
