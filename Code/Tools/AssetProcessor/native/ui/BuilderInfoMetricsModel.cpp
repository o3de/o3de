/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <ui/BuilderInfoMetricsModel.h>
#include <ui/BuilderDataItem.h>
#include <ui/BuilderData.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <QTime>

namespace AssetProcessor
{
    QString DurationToQString(AZ::s64 durationInMs)
    {
        const AZ::s64 dayInMs = 86400000;
        if (durationInMs < 0)
        {
            return QString();
        }

        AZ::s64 dayCount = durationInMs / dayInMs;
        QTime duration = QTime::fromMSecsSinceStartOfDay(durationInMs % dayInMs);
        if (dayCount > 0)
        {
            return duration.toString("'%1d 'hh'h 'mm'm 'ss's 'zzz'ms'").arg(dayCount);
        }
        
        if (duration.isValid())
        {
            if (duration.hour() > 0)
            {
                return duration.toString("hh'h 'mm'm 'ss's 'zzz'ms'");
            }
            if (duration.minute() > 0)
            {
                return duration.toString("mm'm 'ss's 'zzz'ms'");
            }
            if (duration.second() > 0)
            {
                return duration.toString("ss's 'zzz'ms'");
            }
            return duration.toString("zzz'ms'");
        }

        return QString();
    }

    BuilderInfoMetricsModel::BuilderInfoMetricsModel(BuilderData* builderData, QObject* parent)
        : QAbstractItemModel(parent)
        , m_data(builderData)
    {
    }

    void BuilderInfoMetricsModel::Reset()
    {
        beginResetModel();
        m_data->Reset();
        endResetModel();
    }


    QModelIndex BuilderInfoMetricsModel::index(int row, int column, const QModelIndex& parent) const
    {
        if (!hasIndex(row, column, parent))
        {
            return QModelIndex();
        }

        BuilderDataItem* const parentItem =
            parent.isValid() ? static_cast<BuilderDataItem*>(parent.internalPointer()) : m_data->m_root.get();

        if (parentItem)
        {
            AZStd::shared_ptr<BuilderDataItem> childItem = parentItem->GetChild(row);
            if (childItem)
            {
                QModelIndex index = createIndex(row, column, childItem.get());
                Q_ASSERT(checkIndex(index));
                return index;
            }
        }

        return QModelIndex();
    }

    int BuilderInfoMetricsModel::rowCount(const QModelIndex& parent) const
    {
        BuilderDataItem* const parentItem =
            parent.isValid() ? static_cast<BuilderDataItem*>(parent.internalPointer()) : m_data->m_root.get();

        if (!parentItem || parent.column() > 0)
        {
            return 0;
        }
        return parentItem->ChildCount();
    }

    int BuilderInfoMetricsModel::columnCount([[maybe_unused]] const QModelIndex& parent) const
    {
        return aznumeric_cast<int>(Column::Max);
    }

    QVariant BuilderInfoMetricsModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid())
        {
            return QVariant();
        }

        BuilderDataItem* item = static_cast<BuilderDataItem*>(index.internalPointer());
        switch (role)
        {
        case aznumeric_cast<int>(Role::SortRole):
            switch (index.column())
            {
            case aznumeric_cast<int>(Column::AverageDuration):
                if (item->GetJobCount() == 0)
                {
                    return QVariant();
                }
                return item->GetTotalDuration() / item->GetJobCount();
            case aznumeric_cast<int>(Column::TotalDuration):
                return item->GetTotalDuration();
            // Other columns are sorted by Qt::DisplayRole immediately below
            }
        case Qt::DisplayRole:
            switch (index.column())
            {
            case aznumeric_cast<int>(Column::Name):
                return item->GetName().c_str();
            case aznumeric_cast<int>(Column::JobCount):
                return item->GetJobCount();
            case aznumeric_cast<int>(Column::AverageDuration):
                if (item->GetJobCount() == 0)
                {
                    return QVariant();
                }
                return DurationToQString(item->GetTotalDuration() / item->GetJobCount());
            case aznumeric_cast<int>(Column::TotalDuration):
                return DurationToQString(item->GetTotalDuration());
            default:
                break;
            }
        default:
            break;
        }

        return QVariant();
    }

    QVariant BuilderInfoMetricsModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole || section < 0 || section >= aznumeric_cast<int>(Column::Max))
        {
            return QVariant();
        }

        switch (section)
        {
        case aznumeric_cast<int>(Column::Name):
            return tr("Name");
        case aznumeric_cast<int>(Column::JobCount):
            return tr("Job Count");
        case aznumeric_cast<int>(Column::AverageDuration):
            return tr("Average Duration");
        case aznumeric_cast<int>(Column::TotalDuration):
            return tr("Total Duration");
        default:
            AZ_Warning("Asset Processor", false, "Unhandled BuilderInfoMetricsModel header %d", section);
            break;
        }
        return QVariant();
    }

    QModelIndex BuilderInfoMetricsModel::parent(const QModelIndex& index) const
    {
        if (!index.isValid())
        {
            return QModelIndex();
        }

        auto currentItem = static_cast<BuilderDataItem*>(index.internalPointer());
        auto parentItem = currentItem->GetParent();
        auto rootItem = m_data->m_root;
            
        if (parentItem.expired())
        {
            return QModelIndex();
        }

        auto sharedParentItem = parentItem.lock();
        if (sharedParentItem == rootItem || sharedParentItem == nullptr)
        {
            return QModelIndex();
        }

        int rowNum = sharedParentItem->GetRow();
        if (rowNum < 0)
        {
            return QModelIndex();
        }

        QModelIndex parentIndex = createIndex(rowNum, 0, sharedParentItem.get());
        Q_ASSERT(checkIndex(parentIndex));
        return parentIndex;
    }

    void BuilderInfoMetricsModel::OnDurationChanged(BuilderDataItem* item)
    {
        while (item)
        {
            const int rowNum = item->GetRow();
            if (rowNum < 0)
            {
                return;
            }

            dataChanged(
                createIndex(rowNum, aznumeric_cast<int>(Column::JobCount), item),
                createIndex(rowNum, aznumeric_cast<int>(Column::AverageDuration), item));
            item = item->GetParent().expired() ? nullptr : item->GetParent().lock().get();
        }
    }

    BuilderInfoMetricsSortModel::BuilderInfoMetricsSortModel(QObject* parent)
        : QSortFilterProxyModel(parent)
    {
    }
}
