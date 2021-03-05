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

#ifndef DRILLER_REPLICA_REPLICATREEVIEWMODELS_H
#define DRILLER_REPLICA_REPLICATREEVIEWMODELS_H

#if !defined(Q_MOC_RUN)
#include <QAbstractItemModel>

#include "Woodpecker/Driller/Replica/ReplicaDisplayHelpers.h"
#endif

namespace Driller
{

    class ReplicaTreeViewModel
        : public QAbstractItemModel
    {
        Q_OBJECT

    protected:
        ReplicaTreeViewModel(QObject* parent = nullptr);
        
    public:
        AZ_CLASS_ALLOCATOR(ReplicaTreeViewModel,AZ::SystemAllocator,0);

        virtual ~ReplicaTreeViewModel();

        int rowCount(const QModelIndex& parentIndex = QModelIndex()) const override;
        QModelIndex index(int row, int column, const QModelIndex& parent) const;
        QModelIndex parent(const QModelIndex& index) const;

    protected:

        virtual int GetRootRowCount() const = 0;
        virtual const BaseDisplayHelper* FindDisplayHelperAtRoot(int row) const = 0;
    };

}

#endif