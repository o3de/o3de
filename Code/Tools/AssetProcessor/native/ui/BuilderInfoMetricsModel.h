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

    class BuilderInfoMetricsModel : public QAbstractItemModel
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

        BuilderInfoMetricsModel(
            AZStd::shared_ptr<AzToolsFramework::AssetDatabase::AssetDatabaseConnection> dbConnection, QObject* parent = nullptr);

        // QAbstractItemModel
        QModelIndex index(int row, int column, const QModelIndex& parent) const override;

        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        QModelIndex parent(const QModelIndex& index) const override;

        void Reset();
        void OnBuilderSelectionChanged(const AssetBuilderSDK::AssetBuilderDesc& builder);
    /*public Q_SLOTS:
        void OnReplaceStat(AZStd::string key, AZ::s64 value);*/

    private:
        BuilderInfoMetricsItem* GetCurrentModelRoot() const;

        AZStd::shared_ptr<AzToolsFramework::AssetDatabase::AssetDatabaseConnection> m_dbConnection;
        AZStd::unique_ptr<BuilderInfoMetricsItem> m_allBuildersMetrics;
        AZStd::vector<AZStd::unique_ptr<BuilderInfoMetricsItem>> m_singleBuilderMetrics;
        AZStd::unordered_map<AZStd::string, int> m_builderNameToIndex;
        AZStd::unordered_map<AZ::Uuid, int> m_builderGuidToIndex;

        //! This value, when being non-negative, refers to index of m_singleBuilderMetrics.
        //! When it is -1, currently selects m_allBuildersMetrics.
        //! When it is -2, it means BuilderInfoMetricsModel cannot find the selected builder in m_builderGuidToIndex.
        int m_currentSelectedBuilderIndex = -1;
    };
} // namespace AssetProcessor
