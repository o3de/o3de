/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzToolsFramework/AssetBrowser/Search/Filter.h>
#include <AzToolsFramework/AssetBrowser/Entries/FolderAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>

#include <AzQtComponents/Components/Widgets/AssetFolderThumbnailView.h>

#include <AzCore/Console/IConsole.h>

AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option")
#include <AzToolsFramework/AssetBrowser/AssetBrowserTableFilterModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>

#include <QSharedPointer>
#include <QTimer>
#include <QCollator>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        //////////////////////////////////////////////////////////////////////////
        //AssetBrowserFilterModel
        AssetBrowserTableFilterModel::AssetBrowserTableFilterModel(QObject* parent)
            : AssetBrowserFilterModel(parent)
        {
            m_shownColumns.insert(aznumeric_cast<int>(AssetBrowserEntry::Column::Type));
            m_shownColumns.insert(aznumeric_cast<int>(AssetBrowserEntry::Column::DiskSize));
            m_shownColumns.insert(aznumeric_cast<int>(AssetBrowserEntry::Column::Vertices));
            m_shownColumns.insert(aznumeric_cast<int>(AssetBrowserEntry::Column::ApproxSize));
            // The below isn't used at present but will be needed in future
            // m_shownColumns.insert(aznumeric_cast<int>(AssetBrowserEntry::Column::SourceControlStatus));
        }

        bool AssetBrowserTableFilterModel::filterAcceptsRow(
            [[maybe_unused]] int source_row, [[maybe_unused]] const QModelIndex& source_parent) const
        {
            return true;
        }

    } // namespace AssetBrowser
} // namespace AzToolsFramework

#include "AssetBrowser/moc_AssetBrowserTableFilterModel.cpp"
