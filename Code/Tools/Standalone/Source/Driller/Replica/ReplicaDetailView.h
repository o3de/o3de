/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef DRILLER_REPLICA_REPLICADETAILVIEW_H
#define DRILLER_REPLICA_REPLICADETAILVIEW_H

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Math/Random.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/string/string.h>

#include <QtWidgets/QTreeWidget>
#include <QAbstractItemModel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QDialog>
#include <QItemSelectionModel>

#include "Source/Driller/DrillerMainWindowMessages.h"
#include "Source/Driller/StripChart.hxx"
#include "Source/Driller/Replica/ReplicaDataView.hxx"
#include "Source/Driller/Replica/ReplicaDisplayHelpers.h"

#include "Source/Driller/Replica/BaseDetailView.h"

namespace Ui
{
    class ReplicaDetailView;
}

namespace AZ { class ReflectContext; }

namespace Driller
{
    class ReplicaDetailView;
    class ReplicaChunkBandwidthUsage;

    class ReplicaDetailViewModel
        : public BaseDetailTreeViewModel<AZ::u32>
    {
    public:
        enum ColumnDescriptor
        {
            CD_INDEX_FORCE = -1,

            // Ordering of this enum determines the display order
            CD_DISPLAY_NAME,
            CD_TOTAL_SENT,
            CD_TOTAL_RECEIVED,
            CD_RPC_COUNT,

            // Used for sizing of the TableView. Anything after this won't be displayed.
            CD_COUNT
        };

        AZ_CLASS_ALLOCATOR(ReplicaDetailViewModel, AZ::SystemAllocator, 0);
        ReplicaDetailViewModel(ReplicaDetailView* detailView);

        int columnCount(const QModelIndex& parentIndex = QModelIndex()) const override;

        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    };

    class ReplicaDetailView
        : public BaseDetailView<AZ::u32>
    {
        typedef AZStd::unordered_map<AZ::u32, ReplicaChunkDetailDisplayHelper*> ChunkDetailDisplayMap;

        friend class ReplicaDetailViewModel;

    public:
        AZ_CLASS_ALLOCATOR(ReplicaDetailView, AZ::SystemAllocator, 0);

        ReplicaDetailView(ReplicaDataView* replicaDataView, ReplicaDataContainer* dataContainer);
        ~ReplicaDetailView();

        const ReplicaBandwidthChartData<AZ::u32>::FrameMap& GetFrameData() const override;
        BaseDetailDisplayHelper* FindDetailDisplay(const AZ::u32& chunkIndex) override;
        const BaseDetailDisplayHelper* FindDetailDisplay(const AZ::u32& chunkIndex) const override;

    protected:

        void InitializeDisplayData() override;

        void LayoutChanged() override;
        void OnSetupTreeView() override;
        void ShowTreeFrame(FrameNumberType frameId) override;

        AZ::u32 CreateWindowGeometryCRC() override;
        AZ::u32 CreateSplitterStateCRC() override;
        AZ::u32 CreateTreeStateCRC() override;

        void OnInspectedSeries(size_t seriesId) override;

    private:

        size_t                  m_inspectedSeries;

        ChunkDetailDisplayMap   m_typeDisplayMapping;        
        ReplicaDataContainer*   m_replicaData;

        ReplicaDetailViewModel  m_replicaDetailView;

        DrillerWindowLifepsanTelemetry m_lifespanTelemetry;
    };
}

#endif
