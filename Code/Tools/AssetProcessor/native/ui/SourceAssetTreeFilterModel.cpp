/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <ui/SourceAssetTreeFilterModel.h>
#include <ui/SourceAssetTreeItemData.h>

namespace AssetProcessor
{
    SourceAssetTreeFilterModel::SourceAssetTreeFilterModel(QObject* parent)
        : AssetTreeFilterModel(parent)
    {
    }
    bool SourceAssetTreeFilterModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
    {
        const int analysisDurationColumn = aznumeric_cast<int>(SourceAssetTreeColumns::AnalysisJobDuration);
        if (left.column() == analysisDurationColumn && right.column() == analysisDurationColumn)
        {
            AssetTreeItem* leftItem = static_cast<AssetTreeItem*>(left.internalPointer());
            AssetTreeItem* rightItem = static_cast<AssetTreeItem*>(right.internalPointer());

            if (leftItem && rightItem)
            {
                auto leftData = AZStd::static_pointer_cast<SourceAssetTreeItemData>(leftItem->GetData());
                auto rightData = AZStd::static_pointer_cast<SourceAssetTreeItemData>(rightItem->GetData());

                // Folders do not have AnalysisJobDuration. Put folders before asset files and sort folders alphabetically.
                if (leftData->m_isFolder && rightData->m_isFolder)
                {
                    return leftData->m_name > rightData->m_name;
                }

                if (leftData->m_isFolder != rightData->m_isFolder)
                {
                    return rightData->m_isFolder;
                }

                // Sort asset files by duration
                return leftData->m_analysisDuration < rightData->m_analysisDuration;
            }
        }

        return AssetTreeFilterModel::lessThan(left, right);
    }
} // namespace AssetProcessor
