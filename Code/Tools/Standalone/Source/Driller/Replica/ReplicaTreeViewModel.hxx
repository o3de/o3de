/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef DRILLER_REPLICA_REPLICATREEVIEWMODELS_H
#define DRILLER_REPLICA_REPLICATREEVIEWMODELS_H

#if !defined(Q_MOC_RUN)
#include <QAbstractItemModel>

#include "Source/Driller/Replica/ReplicaDisplayHelpers.h"
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
