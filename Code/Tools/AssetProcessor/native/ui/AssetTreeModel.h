/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AssetDatabase/AssetDatabase.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzToolsFramework/API/AssetDatabaseBus.h>
#include <native/utilities/ApplicationManagerAPI.h>
#include <QAbstractItemModel>
#include <QFileIconProvider>
#include <QItemSelectionModel>

namespace AssetProcessor
{
    class AssetTreeItem;
    class AssetTreeModel :
        public QAbstractItemModel,
        AzToolsFramework::AssetDatabase::AssetDatabaseNotificationBus::Handler,
        AssetProcessor::ApplicationManagerNotifications::Bus::Handler
    {
    public:
        AssetTreeModel(AZStd::shared_ptr<AzToolsFramework::AssetDatabase::AssetDatabaseConnection> sharedDbConnection, QObject *parent = nullptr);
        virtual ~AssetTreeModel();

        // ApplicationManagerNotifications::Bus::Handler
        void ApplicationShutdownRequested() override;

        // QAbstractItemModel
        QModelIndex index(int row, int column, const QModelIndex &parent) const override;

        int rowCount(const QModelIndex &parent = QModelIndex()) const override;
        int columnCount(const QModelIndex &parent = QModelIndex()) const override;
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
        QModelIndex parent(const QModelIndex &index) const override;
        bool hasChildren(const QModelIndex &parent) const override;
        Qt::ItemFlags flags(const QModelIndex &index) const override;

        void Reset();

        static QFlags<QItemSelectionModel::SelectionFlag> GetAssetTreeSelectionFlags();

    protected:
        // Called by reset, while a Qt model reset is active.
        virtual void ResetModel() = 0;

        AZStd::unique_ptr<AssetTreeItem> m_root;
        AZStd::shared_ptr<AzToolsFramework::AssetDatabase::AssetDatabaseConnection> m_sharedDbConnection;

        QIcon m_errorIcon;
    };
}
