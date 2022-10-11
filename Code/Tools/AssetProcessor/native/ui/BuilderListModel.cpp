/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <native/ui/BuilderListModel.h>
#include <utilities/AssetUtilEBusHelper.h>

int BuilderListModel::rowCount(const QModelIndex& /*parent*/) const
{
    AssetProcessor::BuilderInfoList builders;
    AssetProcessor::AssetBuilderInfoBus::Broadcast(&AssetProcessor::AssetBuilderInfoBus::Events::GetAllBuildersInfo, builders);

    return aznumeric_caster(builders.size());
}

QVariant BuilderListModel::data(const QModelIndex& index, int role) const
{
    AssetProcessor::BuilderInfoList builders;
    AssetProcessor::AssetBuilderInfoBus::Broadcast(&AssetProcessor::AssetBuilderInfoBus::Events::GetAllBuildersInfo, builders);
    
    AZ_Assert(index.isValid(), "BuilderListModel index out of bounds");

    const auto& assetBuilderDesc = builders[index.row()];

    if (role == Qt::DisplayRole)
    {
        return QString(assetBuilderDesc.m_name.c_str());
    }

    return {};
}

void BuilderListModel::Reset()
{
    beginResetModel();
    endResetModel();
}

bool BuilderListSortFilterProxy::lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const
{
    return sourceModel()->data(source_left).toString().compare(sourceModel()->data(source_right).toString(), Qt::CaseInsensitive) < 0;
}
