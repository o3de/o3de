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

#include "Woodpecker_precompiled.h"

#include "Woodpecker/Driller/Replica/ReplicaTreeViewModel.hxx"
#include <Woodpecker/Driller/Replica/moc_ReplicaTreeViewModel.cpp>

namespace Driller
{
    ReplicaTreeViewModel::ReplicaTreeViewModel(QObject* parent)
        : QAbstractItemModel(parent)
    {
    }

    ReplicaTreeViewModel::~ReplicaTreeViewModel()
    {
    }

    int ReplicaTreeViewModel::rowCount(const QModelIndex& parentIndex) const
    {
        int rowCount = 0;

        if (!parentIndex.isValid())
        {
            rowCount = GetRootRowCount();
        }
        else
        {
            const BaseDisplayHelper* displayHelper = static_cast<const BaseDisplayHelper*>(parentIndex.internalPointer());
            rowCount = static_cast<int>(displayHelper->GetTreeRowCount());
        }

        return rowCount;
    }

    QModelIndex ReplicaTreeViewModel::index(int row, int column, const QModelIndex& parent) const
    {
        if (!parent.isValid())
        {
            const BaseDisplayHelper* baseDisplayHelper = FindDisplayHelperAtRoot(row);

            if (baseDisplayHelper)
            {
                return createIndex(row, column, (void*)(baseDisplayHelper));
            }
            else
            {
                return QModelIndex();
            }
        }
        else
        {
            const BaseDisplayHelper* parentHelper = static_cast<const BaseDisplayHelper*>(parent.internalPointer());
            const BaseDisplayHelper* displayHelper = parentHelper->FindChildByRow(row);

            if (displayHelper)
            {
                return createIndex(row, column, (void*)(displayHelper));
            }
            else
            {
                AZ_Assert(false, "Invalid Tree Structure");
            }
        }

        return QModelIndex();
    }

    QModelIndex ReplicaTreeViewModel::parent(const QModelIndex& index) const
    {
        if (index.isValid())
        {
            const BaseDisplayHelper* displayHelper = static_cast<const BaseDisplayHelper*>(index.internalPointer());
            const BaseDisplayHelper* parent = displayHelper->GetParent();

            if (parent)
            {
                return createIndex(parent->GetChildIndex(displayHelper), 0, (void*)(parent));
            }
        }

        return QModelIndex();
    }
}
