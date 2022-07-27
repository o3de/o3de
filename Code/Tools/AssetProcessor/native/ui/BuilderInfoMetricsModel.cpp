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
            return duration.toString("zzz' ms, 'ss' sec, 'mm' min, 'hh' hr, %1 day'").arg(dayCount);
        }
        
        if (duration.isValid())
        {
            if (duration.hour() > 0)
            {
                return duration.toString("zzz' ms, 'ss' sec, 'mm' min, 'hh' hr'");
            }
            if (duration.minute() > 0)
            {
                return duration.toString("zzz' ms, 'ss' sec, 'mm' min'");
            }
            if (duration.second() > 0)
            {
                return duration.toString("zzz' ms, 'ss' sec'");
            }
            return duration.toString("zzz' ms'");
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

    void BuilderInfoMetricsModel::OnBuilderSelectionChanged(const AssetBuilderSDK::AssetBuilderDesc& builder)
    {
        beginResetModel();
        
        if (m_data->m_builderGuidToIndex.contains(builder.m_busId))
        {
            m_data->m_currentSelectedBuilderIndex = m_data->m_builderGuidToIndex[builder.m_busId];
            m_data->m_root->SetBuilderChild(m_data->m_singleBuilderMetrics[m_data->m_currentSelectedBuilderIndex]);
        }
        else
        {
            AZ_Warning(
                "Asset Processor",
                false,
                "BuilderInfoMetricsModel cannot find the GUID of the builder selected by the user (%s) in itself. No metrics will be "
                "shown in the builder tab.",
                builder.m_busId.ToString<AZStd::string>().c_str());
            m_data->m_currentSelectedBuilderIndex = aznumeric_cast<int>(BuilderData::BuilderSelection::Invalid);
        }
        
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

        if (!parentItem)
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
                return item->GetName();
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

        auto sharedParentitem = parentItem.lock();
        if (sharedParentitem == rootItem || sharedParentitem == nullptr)
        {
            return QModelIndex();
        }

        QModelIndex parentIndex = createIndex(sharedParentitem->GetRow(), 0, sharedParentitem.get());
        Q_ASSERT(checkIndex(parentIndex));
        return parentIndex;
    }

    void BuilderInfoMetricsModel::OnDurationChanged(BuilderDataItem* item)
    {
        while (item)
        {
            const int rowNum = item->GetRow();
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
