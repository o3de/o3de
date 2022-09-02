/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Asset/AssetCommon.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/UI/UICore/QTreeViewStateSaver.hxx>

#include <QModelIndex>
#include <QPointer>
#endif

class QTimer;

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserEntry;
        class AssetBrowserModel;
        class AssetBrowserFilterModel;
        class EntryDelegate;

        class AssetBrowserTreeView
            : public QTreeViewWithStateSaving
            , public AssetBrowserViewRequestBus::Handler
            , public AssetBrowserComponentNotificationBus::Handler
        {
            Q_OBJECT

        public:
            explicit AssetBrowserTreeView(QWidget* parent = nullptr);
            ~AssetBrowserTreeView() override;

            //////////////////////////////////////////////////////////////////////////
            // QTreeView
            void setModel(QAbstractItemModel* model) override;
            //////////////////////////////////////////////////////////////////////////

            //! Set unique asset browser name, used to persist tree expansion states
            void SetName(const QString& name);
            QString& GetName(){ return m_name; }

            // O3DE_DEPRECATED
            void LoadState(const QString& name);
            void SaveState() const;

            //! Gets the selected entries.  if includeProducts is false, it will only
            //! count sources and folders - many common operations such as deleting, renaming, etc,
            //! can only work on sources and folders.
            AZStd::vector<AssetBrowserEntry*> GetSelectedAssets(bool includeProducts = true) const;

            void SelectFolder(AZStd::string_view folderPath);

            void DeleteEntries();
            void RenameEntry();
            void DuplicateEntries();

            //////////////////////////////////////////////////////////////////////////
            // AssetBrowserViewRequestBus
            void SelectProduct(AZ::Data::AssetId assetID) override;
            void SelectFileAtPath(const AZStd::string& assetPath) override;
            void ClearFilter() override;

            void Update() override;

            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // AssetBrowserComponentNotificationBus
            void OnAssetBrowserComponentReady() override;
            //////////////////////////////////////////////////////////////////////////

            void SetShowSourceControlIcons(bool showSourceControlsIcons);
            void UpdateAfterFilter(bool hasFilter, bool selectFirstValidEntry);

            template <class TEntryType>
            const TEntryType* GetEntryFromIndex(const QModelIndex& index) const;

            bool IsIndexExpandedByDefault(const QModelIndex& index) const override;

        Q_SIGNALS:
            void selectionChangedSignal(const QItemSelection& selected, const QItemSelection& deselected);
            void ClearStringFilter();
            void ClearTypeFilter();

        public Q_SLOTS:
            void OpenItemForEditing(const QModelIndex& index);

        protected:
            QModelIndexList selectedIndexes() const override;

        protected Q_SLOTS:
            void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected) override;
            void rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end) override;

        private:
            QPointer<AssetBrowserModel> m_assetBrowserModel;
            QPointer<AssetBrowserFilterModel> m_assetBrowserSortFilterProxyModel;
            EntryDelegate* m_delegate = nullptr;

            bool m_expandToEntriesByDefault = false;

            QTimer* m_scTimer = nullptr;
            const int m_scUpdateInterval = 100;

            QString m_name;

            bool SelectProduct(const QModelIndex& idxParent, AZ::Data::AssetId assetID);
            bool SelectEntry(const QModelIndex& idxParent, const AZStd::vector<AZStd::string>& entryPathTokens, const uint32_t entryPathIndex = 0, bool useDisplayName = false);

            //! Grab one entry from the source thumbnail list and update it
            void UpdateSCThumbnails();

        private Q_SLOTS:
            void OnContextMenu(const QPoint& point);

            //! Get all visible source entries and place them in a queue to update their source control status
            void OnUpdateSCThumbnailsList();
        };

        template <class TEntryType>
        const TEntryType* AssetBrowserTreeView::GetEntryFromIndex(const QModelIndex& index) const
        {
            if (index.isValid())
            {
                QModelIndex sourceIndex = m_assetBrowserSortFilterProxyModel->mapToSource(index);
                AssetBrowserEntry* entry = static_cast<AssetBrowserEntry*>(sourceIndex.internalPointer());
                return azrtti_cast<const TEntryType*>(entry);
            }
            return nullptr;
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework
