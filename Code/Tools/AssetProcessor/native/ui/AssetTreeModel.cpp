/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AssetTreeModel.h"
#include "AssetTreeItem.h"
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/smart_ptr/make_shared.h>

namespace AssetProcessor
{

    AssetTreeModel::AssetTreeModel(AZStd::shared_ptr<AzToolsFramework::AssetDatabase::AssetDatabaseConnection> sharedDbConnection, QObject *parent) :
        QAbstractItemModel(parent),
        m_sharedDbConnection(sharedDbConnection),
        m_errorIcon(QStringLiteral(":/stylesheet/img/logging/error.svg"))
    {
        ApplicationManagerNotifications::Bus::Handler::BusConnect();
        AzToolsFramework::AssetDatabase::AssetDatabaseNotificationBus::Handler::BusConnect();
    }

    AssetTreeModel::~AssetTreeModel()
    {
        AzToolsFramework::AssetDatabase::AssetDatabaseNotificationBus::Handler::BusDisconnect();
        ApplicationManagerNotifications::Bus::Handler::BusDisconnect();
    }

    void AssetTreeModel::ApplicationShutdownRequested()
    {
        AzToolsFramework::AssetDatabase::AssetDatabaseNotificationBus::Handler::BusDisconnect();
        // AssetTreeModels can queue functions on the systemTickBus for processing on the main thread in response to asset changes
        // We need to clear out any left pending before we go away
        AZ::SystemTickBus::ExecuteQueuedEvents();
    }

    void AssetTreeModel::Reset()
    {
        beginResetModel();
        m_root.reset(new AssetTreeItem(AZStd::make_shared<AssetTreeItemData>("", "", true, AZ::Uuid::CreateNull()), m_errorIcon));

        ResetModel();

        endResetModel();
    }

    int AssetTreeModel::rowCount(const QModelIndex &parent) const
    {
        if (parent.column() > 0)
        {
            return 0;
        }

        AssetTreeItem* parentItem = nullptr;
        if (!parent.isValid())
        {
            parentItem = m_root.get();
        }
        else
        {
            parentItem = static_cast<AssetTreeItem*>(parent.internalPointer());
        }

        if (!parentItem)
        {
            return 0;
        }
        return parentItem->getChildCount();
    }

    int AssetTreeModel::columnCount(const QModelIndex &parent) const
    {
        if (parent.isValid())
        {
            return static_cast<AssetTreeItem*>(parent.internalPointer())->GetColumnCount();
        }
        if (m_root)
        {
            return m_root->GetColumnCount();
        }
        return 0;
    }

    QVariant AssetTreeModel::data(const QModelIndex &index, int role) const
    {
        if (!index.isValid())
        {
            return QVariant();
        }

        AssetTreeItem* item = static_cast<AssetTreeItem*>(index.internalPointer());
        switch (role)
        {
        case Qt::DisplayRole:
            return item->GetDataForColumn(index.column());
        case Qt::DecorationRole:
            // Only show the icon in the name column
            if (index.column() == static_cast<int>(AssetTreeColumns::Name))
            {
                return item->GetIcon();
            }
            break;
        case Qt::ToolTipRole:
            {
                QString toolTip = item->GetData()->m_unresolvedIssuesTooltip;
                if (!toolTip.isEmpty())
                {
                    return toolTip;
                }
                // Purposely return an empty string, so mousing over rows clear out.
                return QString("");
            }
        }
        return QVariant();
    }

    QVariant AssetTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        {
            return QVariant();
        }
        if (section < 0 || section >= static_cast<int>(AssetTreeColumns::Max))
        {
            return QVariant();
        }

        switch (section)
        {
            case static_cast<int>(AssetTreeColumns::Name):
                return tr("Name");
            case static_cast<int>(AssetTreeColumns::Extension):
                return tr("Extension");
            default:
                AZ_Warning("AssetProcessor", false, "Unhandled AssetTree section %d", section);
                break;
        }
        return QVariant();
    }

    QModelIndex AssetTreeModel::index(int row, int column, const QModelIndex &parent) const
    {
        if (!hasIndex(row, column, parent))
        {
            return QModelIndex();
        }

        AssetTreeItem* parentItem = nullptr;

        if (!parent.isValid())
        {
            parentItem = m_root.get();
        }
        else
        {
            parentItem = static_cast<AssetTreeItem*>(parent.internalPointer());
        }

        if (!parentItem)
        {
            return QModelIndex();
        }

        AssetTreeItem* childItem = parentItem->GetChild(row);

        if (childItem)
        {
            QModelIndex index = createIndex(row, column, childItem);
            Q_ASSERT(checkIndex(index));
            return index;
        }
        return QModelIndex();
    }

    bool AssetTreeModel::setData(const QModelIndex &/*index*/, const QVariant &/*value*/, int /*role*/)
    {
        return false;
    }

    Qt::ItemFlags AssetTreeModel::flags(const QModelIndex &index) const
    {
        return Qt::ItemIsSelectable | QAbstractItemModel::flags(index);
    }

    QModelIndex AssetTreeModel::parent(const QModelIndex &index) const
    {
        if (!index.isValid())
        {
            return QModelIndex();
        }

        AssetTreeItem* childItem = static_cast<AssetTreeItem*>(index.internalPointer());
        AssetTreeItem* parentItem = childItem->GetParent();

        if (parentItem == m_root.get() || parentItem == nullptr)
        {
            return QModelIndex();
        }
        QModelIndex parentIndex = createIndex(parentItem->GetRow(), 0, parentItem);
        Q_ASSERT(checkIndex(parentIndex));
        return parentIndex;
    }

    bool AssetTreeModel::hasChildren(const QModelIndex &parent) const
    {
        AssetTreeItem* parentItem = nullptr;

        if (!parent.isValid())
        {
            parentItem = m_root.get();
        }
        else
        {
            parentItem = static_cast<AssetTreeItem*>(parent.internalPointer());
        }

        if (!parentItem)
        {
            return false;
        }

        return parentItem->getChildCount() > 0;
    }

    QFlags<QItemSelectionModel::SelectionFlag> AssetTreeModel::GetAssetTreeSelectionFlags()
    {
        return QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows | QItemSelectionModel::QItemSelectionModel::Current;
    }

}
