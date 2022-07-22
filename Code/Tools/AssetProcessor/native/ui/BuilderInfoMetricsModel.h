/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AZCore/std/containers/vector.h>
#include <AZCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>
#include <QAbstractItemModel>
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>

namespace AssetBuilderSDK
{
    struct AssetBuilderDesc;
} // namespace AssetBuilderSDK

namespace AssetProcessor
{
    class BuilderInfoMetricsItem;
    class JobEntry;
    class BuilderData;

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

        BuilderInfoMetricsModel(AZStd::shared_ptr<BuilderData> builderData, QObject* parent = nullptr);

        // QAbstractItemModel
        QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;

        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        QModelIndex parent(const QModelIndex& index) const override;

        void Reset();
        void OnBuilderSelectionChanged(const AssetBuilderSDK::AssetBuilderDesc& builder);
    public Q_SLOTS:
        void OnProcessJobDurationChanged(JobEntry jobEntry, int value);
        void OnCreateJobsDurationChanged(QString sourceName);

    private:
        AZStd::shared_ptr<BuilderData> m_data;
    };
} // namespace AssetProcessor
