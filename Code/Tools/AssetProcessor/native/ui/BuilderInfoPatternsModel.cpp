/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <ui/BuilderInfoPatternsModel.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

namespace AssetProcessor
{
    BuilderInfoPatternsModel::BuilderInfoPatternsModel(QObject* parent)
        : QAbstractItemModel(parent)
    {
    }

    QModelIndex BuilderInfoPatternsModel::index(int row, int column, const QModelIndex& parent) const
    {
        if (row < 0 || row >= rowCount(parent) || column < 0 || column >= columnCount(parent))
        {
            return QModelIndex();
        }
        return createIndex(row, column);
    }

    int BuilderInfoPatternsModel::rowCount([[maybe_unused]] const QModelIndex& parent) const
    {
        return aznumeric_cast<int>(m_data.size());
    }

    int BuilderInfoPatternsModel::columnCount([[maybe_unused]] const QModelIndex& parent) const
    {
        return aznumeric_cast<int>(Column::Max);
    }

    QVariant BuilderInfoPatternsModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid() || index.row() >= m_data.size())
        {
            return QVariant();
        }
        if (role == Qt::DisplayRole)
        {
            switch (index.column())
            {
            case aznumeric_cast<int>(Column::Type):
                switch (m_data[index.row()].m_type)
                {
                case AssetBuilderSDK::AssetBuilderPattern::Regex:
                    return tr("Regex");
                case AssetBuilderSDK::AssetBuilderPattern::Wildcard:
                    return tr("Wildcard");
                default:
                    return QVariant();
                }
                break;
            case aznumeric_cast<int>(Column::Extension):
                return m_data[index.row()].m_pattern.c_str();
            default:
                break;
            }
        }

        return QVariant();
    }

    QVariant BuilderInfoPatternsModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        {
            switch (section)
            {
            case aznumeric_cast<int>(Column::Type):
                return tr("Type");
            case aznumeric_cast<int>(Column::Extension):
                return tr("Extension");
            default:
                break;
            }
        }

        return QAbstractItemModel::headerData(section, orientation, role);
    }

    QModelIndex BuilderInfoPatternsModel::parent([[maybe_unused]] const QModelIndex& index) const
    {
        return QModelIndex();
    }

    void BuilderInfoPatternsModel::Reset(const AssetBuilderSDK::AssetBuilderDesc& builder)
    {
        beginResetModel();
        m_data = builder.m_patterns;
        endResetModel();
    }
} // namespace AssetProcessor
