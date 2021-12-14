/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <native/ui/BuilderListModel.h>

int MyBuilderList::rowCount(const QModelIndex& /*parent*/) const
{
    AssetProcessor::BuilderInfoList builders;
    AssetProcessor::AssetBuilderInfoBus::Broadcast(&AssetProcessor::AssetBuilderInfoBus::Events::GetAllBuildersInfo, builders);
    
    return aznumeric_caster(builders.size());
}

QVariant MyBuilderList::data(const QModelIndex& index, int role) const
{
    AssetProcessor::BuilderInfoList builders;
    AssetProcessor::AssetBuilderInfoBus::Broadcast(&AssetProcessor::AssetBuilderInfoBus::Events::GetAllBuildersInfo, builders);
    
    AZ_Assert(index.row() >= 0, "Index must be >= 0");
    AZ_Assert(index.row() < builders.size(), "Index out of bounds");

    const auto& assetBuilderDesc = builders[index.row()];
    
    if (role == Qt::DisplayRole)
    {
        return QString(assetBuilderDesc.m_name.c_str());
    }

    return {};
}
