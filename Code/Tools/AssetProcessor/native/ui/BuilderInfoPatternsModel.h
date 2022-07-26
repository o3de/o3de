/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QAbstractItemModel>
#include <AzCore/std/containers/vector.h>

namespace AssetBuilderSDK
{
    struct AssetBuilderDesc;
    struct AssetBuilderPattern;
}

namespace AssetProcessor
{
    class BuilderInfoPatternsModel : public QAbstractItemModel
    {
        Q_OBJECT
    public:
        enum class Column
        {
            Type,
            Extension,
            Max
        };

        BuilderInfoPatternsModel(QObject* parent = nullptr);

        // QAbstractItemModel
        QModelIndex index(int row, int column, const QModelIndex& parent) const override;

        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        QModelIndex parent(const QModelIndex& index) const override;

        void Reset(const AssetBuilderSDK::AssetBuilderDesc& builder);
    private:
        AZStd::vector<AssetBuilderSDK::AssetBuilderPattern> m_data;
    };
}
