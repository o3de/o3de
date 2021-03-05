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

#ifndef DRILLER_REPLICA_REPLICACHUNKTYPEDETAILVIEW_H
#define DRILLER_REPLICA_REPLICACHUNKTYPEDETAILVIEW_H

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Math/Random.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>

#include <QAbstractItemModel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QDialog>
#include <QItemSelectionModel>

#include "Woodpecker/Driller/StripChart.hxx"
#include "Woodpecker/Driller/Replica/ReplicaDisplayHelpers.h"
#include "Woodpecker/Driller/Replica/ReplicaDataView.hxx"

#include "Woodpecker/Driller/Replica/BaseDetailView.h"

namespace Ui
{
    class ReplicaDetailView;
}

namespace Driller
{
    class ReplicaBandwidthUsage;
    class ReplicaChunkTypeDetailView;

    class ReplicaChunkTypeDetailViewModel
        : public BaseDetailTreeViewModel<AZ::u64>
    {
    public:
        enum ColumnDescriptor
        {
            // Forcing the index to start at 0
            CD_INDEX_FORCE = -1,

            // Ordering of this enum determines the display order
            CD_DISPLAY_NAME,
            CD_REPLICA_ID,
            CD_TOTAL_SENT,
            CD_TOTAL_RECEIVED,
            CD_RPC_COUNT,

            // Used for sizing of the TableView. Anything after this won't be displayed.
            CD_COUNT
        };

        AZ_CLASS_ALLOCATOR(ReplicaChunkTypeDetailViewModel, AZ::SystemAllocator, 0);
        ReplicaChunkTypeDetailViewModel(ReplicaChunkTypeDetailView* detailView);

        int columnCount(const QModelIndex& parentIndex = QModelIndex()) const override;

        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    };

    class ReplicaChunkTypeDetailView
        : public BaseDetailView<AZ::u64>
    {
        typedef AZStd::unordered_map<AZ::u64, ReplicaDetailDisplayHelper*> ReplicaDetailDisplayMap;

        friend class ReplicaChunkTypeDetailViewModel;

    public:
        AZ_CLASS_ALLOCATOR(ReplicaChunkTypeDetailView, AZ::SystemAllocator, 0);

        ReplicaChunkTypeDetailView(ReplicaDataView* replicaDataView, ReplicaChunkTypeDataContainer* chunkTypeDataContainer);
        ~ReplicaChunkTypeDetailView();

        const ReplicaBandwidthChartData<AZ::u64>::FrameMap& GetFrameData() const override;
        BaseDetailDisplayHelper* FindDetailDisplay(const AZ::u64& replicaId) override;
        const BaseDetailDisplayHelper* FindDetailDisplay(const AZ::u64& replicaId) const override;

        BaseDetailDisplayHelper* FindAggregateDisplay() override;
        AZ::u64 FindAggregateID() const override;

    protected:

        void InitializeDisplayData() override;

        void LayoutChanged() override;
        void OnSetupTreeView() override;
        void ShowTreeFrame(FrameNumberType frameId) override;

        AZ::u32 CreateWindowGeometryCRC() override;
        AZ::u32 CreateSplitterStateCRC() override;
        AZ::u32 CreateTreeStateCRC() override;

        void OnInspectedSeries(size_t seriesId);

    private:

        size_t                          m_inspectedSeries;
        
        ReplicaDetailDisplayHelper*     m_aggregateDisplayHelper;

        ReplicaDetailDisplayMap         m_replicaDisplayMapping;
        ReplicaChunkTypeDataContainer*  m_replicaChunkData;

        ReplicaChunkTypeDetailViewModel m_chunkTypeDetailView;

        DrillerWindowLifepsanTelemetry m_lifespanTelemetry;
    };
}

#endif