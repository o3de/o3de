/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include <QPointer>
#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>

namespace AssetBuilderSDK
{
    struct AssetBuilderDesc;
} // namespace AssetBuilderSDK

namespace AssetProcessor
{
    class BuilderDataItem;
    class BuilderData;
    class JobEntry;

    class BuilderInfoMetricsModel
        : public QAbstractItemModel
    {
    public:
        enum class Column
        {
            Name,
            JobCount,
            TotalDuration,
            AverageDuration,
            Max
        };

        enum class Role
        {
            SortRole = Qt::UserRole
        };

        BuilderInfoMetricsModel(BuilderData* builderData, QObject* parent = nullptr);
        void Reset();

        // QAbstractItemModel
        QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        QModelIndex parent(const QModelIndex& index) const override;

    public Q_SLOTS:
        void OnDurationChanged(BuilderDataItem* item);

    private:
        QPointer<BuilderData> m_data;
    };

    class BuilderInfoMetricsSortModel : public QSortFilterProxyModel
    {
    public:
        BuilderInfoMetricsSortModel(QObject* parent = nullptr);
    };
} // namespace AssetProcessor
