/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/PlatformDef.h>

AZ_PUSH_DISABLE_WARNING(4127 4251, "-Wunknown-warning-option") // conditional expression is constant
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
AZ_POP_DISABLE_WARNING

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/Component/TickBus.h>

AZ_PUSH_DISABLE_WARNING(4127 4251 4800, "-Wunknown-warning-option") // 4127: conditional expression is constant
                                                                    // 4251: 'QVariant::d': struct 'QVariant::Private' needs to have dll-interface to be used by clients of class 'QVariant'
                                                                    // 4800: 'int': forcing value to bool 'true' or 'false' (performance warning)
#include <QAbstractTableModel>
#include <QVariant>
#include <QMimeData>
#endif
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserEntry;
        class RootAssetBrowserEntry;
        class AssetEntryChangeset;

        class AssetBrowserModel
            : public QAbstractItemModel
            , public AssetBrowserModelRequestBus::Handler
            , public AZ::TickBus::Handler
        {
            Q_OBJECT
        public:
            enum Roles
            {
                EntryRole = Qt::UserRole + 100,
            };

            AZ_CLASS_ALLOCATOR(AssetBrowserModel, AZ::SystemAllocator, 0);

            explicit AssetBrowserModel(QObject* parent = nullptr);
            ~AssetBrowserModel();

            QModelIndex findIndex(const QString& absoluteAssetPath) const;

            //////////////////////////////////////////////////////////////////////////
            // QAbstractTableModel
            //////////////////////////////////////////////////////////////////////////
            QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
            int rowCount(const QModelIndex& parent = QModelIndex()) const override;
            int columnCount(const QModelIndex& parent = QModelIndex()) const override;
            QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
            Qt::ItemFlags flags(const QModelIndex& index) const override;
            QMimeData* mimeData(const QModelIndexList& indexes) const override;
            QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
            QModelIndex parent(const QModelIndex& child) const override;

            //////////////////////////////////////////////////////////////////////////
            // AssetBrowserModelRequestBus
            //////////////////////////////////////////////////////////////////////////
            bool IsLoaded() const override;
            void BeginAddEntry(AssetBrowserEntry* parent) override;
            void EndAddEntry(AssetBrowserEntry* parent) override;
            void BeginRemoveEntry(AssetBrowserEntry* entry) override;
            void EndRemoveEntry() override;
            
            //////////////////////////////////////////////////////////////////////////
            // TickBus
            //////////////////////////////////////////////////////////////////////////
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

            AZStd::shared_ptr<RootAssetBrowserEntry> GetRootEntry() const;
            void SetRootEntry(AZStd::shared_ptr<RootAssetBrowserEntry> rootEntry);

            static void SourceIndexesToAssetIds(const QModelIndexList& indexes, AZStd::vector<AZ::Data::AssetId>& assetIds);
            static void SourceIndexesToAssetDatabaseEntries(const QModelIndexList& indexes, AZStd::vector<AssetBrowserEntry*>& entries);
            
            const static int m_column;

        private:
            AZStd::shared_ptr<RootAssetBrowserEntry> m_rootEntry;
            bool m_loaded;
            bool m_addingEntry;
            bool m_removingEntry;

            bool GetEntryIndex(AssetBrowserEntry* entry, QModelIndex& index) const;
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
