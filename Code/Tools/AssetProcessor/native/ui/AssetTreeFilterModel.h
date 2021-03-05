/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/containers/list.h>
#include <QSortFilterProxyModel>
#endif

namespace AZ
{
    struct Uuid;
}

namespace AssetProcessor
{
    class AssetTreeItem;
    class AssetTreeItemData;

    class AssetTreeFilterModel : public QSortFilterProxyModel
    {
        Q_OBJECT
    public:
        AssetTreeFilterModel(QObject* parent = nullptr);

        void FilterChanged(const QString& newFilter);

        // The asset trees have buttons to jump to related assets.
        // If a search is active and one is clicked, force that asset to be visible.
        // This index is to the source model, and not the proxy model.
        void ForceModelIndexVisible(const QModelIndex& sourceIndex);

    protected:
        bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
        bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

        bool DescendantMatchesFilter(const AssetTreeItem& assetTreeItem, const QRegExp& filter, const AZ::Uuid& filterAsUuid) const;

        AZStd::list<AZStd::shared_ptr<AssetTreeItemData>> m_pathToForceVisibleAsset;
    };

}
