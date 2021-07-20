/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef REPLICADATAVIEW_H
#define REPLICADATAVIEW_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Math/Random.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>

#include <QAbstractTableModel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QDialog>
#include <qstyleditemdelegate.h>
#include <qitemselectionmodel.h>

#include "Source/Driller/DrillerMainWindowMessages.h"
#include "Source/Driller/DrillerOperationTelemetryEvent.h"

#include "Source/Driller/StripChart.hxx"
#include "Source/Driller/AreaChart.hxx"
#include "Source/Driller/DrillerDataTypes.h"
#include "Source/Driller/Replica/ReplicaDisplayHelpers.h"
#include "Source/Driller/Replica/ReplicaDisplayTypes.h"
#include "Source/Driller/Replica/ReplicaTreeViewModel.hxx"

#include <Source/Driller/Replica/ui_replicadataview.h>
#include <Source/Driller/Replica/ui_ReplicaDataViewConfigDialog.h>

#include "ReplicaBandwidthChartData.h"
#endif

namespace AZ { class ReflectContext; }

namespace Driller
{
    class ReplicaDataAggregator;

    class ReplicaChunkDataEvent;
    class ReplicaChunkReceivedDataEvent;
    class ReplicaChunkSentDataEvent;
    class ReplicaDataView;    
    class ReplicaTableViewModel;
    class ReplicaDataContainer;
    class ReplicaChunkTypeDataContainer;

    class ReplicaDetailView;
    class ReplicaChunkTypeDetailView;

    class ReplicaDataViewSavedState;

    class OverallReplicaDetailView;

    class FormattingHelper
    {
    public:
        static QString ReplicaID(AZ::u64 replicaId)
        {
            return QString("0x%1").arg(QString::number(replicaId,16).toUpper());
        }
    };

    // Icon Item Delegate
    class InspectIconItemDelegate : public QStyledItemDelegate
    {
        Q_OBJECT
    public:
        explicit InspectIconItemDelegate(Qt::Alignment alignment, QObject* parent = 0)
            : QStyledItemDelegate(parent)
            , m_alignment(alignment)
        {
        
        }

        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
        {
            auto opt = option;
            opt.decorationAlignment = m_alignment;
            QStyledItemDelegate::paint(painter,opt,index);
        }

    private:
        Qt::Alignment m_alignment;        
    };

    // Should be a private class, but Q_OBJECT doesn't support it.
    class ReplicaTableViewModel : public QAbstractTableModel
    {
        Q_OBJECT
        
    public:

        enum ColumnDescriptor
        {
            CD_INDEX_FORCE = -1,

            // Ordering of this enum determines the display order.            
            CD_REPLICA_NAME,
            CD_REPLICA_ID,            
            CD_TOTAL_SENT,
            CD_TOTAL_RECEIVED,
            CD_INSPECT,

            // Used for sizing of the TableView. Anything after this won't be displayed.
            CD_COUNT,
            
        };

        AZ_CLASS_ALLOCATOR(ReplicaTableViewModel, AZ::SystemAllocator, 0);
        ReplicaTableViewModel(ReplicaDataView* replicaDataView);

        void RefreshView();

        int rowCount(const QModelIndex& parentIndex = QModelIndex()) const override;
        int columnCount(const QModelIndex& parentIndex = QModelIndex()) const override;

        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;        

        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;        

        Qt::ItemFlags flags(const QModelIndex& index) const override;

        AZ::u64 GetReplicaIdFromIndex(const QModelIndex& index) const;
        AZ::u64 GetReplicaIdForRow(int row) const;

    private:

        ReplicaDataView* m_replicaDataView;
        AZStd::vector<AZ::u64> m_replicaIds;
    };

    class ReplicaChunkTypeTableViewModel : public QAbstractTableModel
    {
        Q_OBJECT
    
    public:
   
       enum ColumnDescriptor
       {
            CD_INDEX_FORCE = -1,
        
            // Ordering of this enum determines the display order.            
            CD_CHUNK_TYPE,            
            CD_TOTAL_SENT,
            CD_TOTAL_RECEIVED,
            CD_INSPECT,
        
            CD_COUNT
       };
   
       AZ_CLASS_ALLOCATOR(ReplicaChunkTypeTableViewModel, AZ::SystemAllocator, 0);
   
       ReplicaChunkTypeTableViewModel(ReplicaDataView* replicaDataView);

       void RefreshView();
   
       int rowCount(const QModelIndex& parentIndex = QModelIndex()) const override;
       int columnCount(const QModelIndex& parentIndex = QModelIndex()) const override;
   
       QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;       
   
       QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
   
       Qt::ItemFlags flags(const QModelIndex& index) const override;

       const char* GetReplicaChunkTypeFromIndex(const QModelIndex& index) const;
       const char* GetReplicaChunkTypeForRow(int row) const;
   
    private:        

        ReplicaDataView* m_replicaDataView;
        AZStd::vector<AZStd::string> m_replicaChunkTypes;
    };

    class ReplicaDataView 
        : public QDialog
        , public Driller::DrillerMainWindowMessages::Bus::Handler
        , public Driller::DrillerEventWindowMessages::Bus::Handler
    {
        Q_OBJECT

    private:

        friend class ReplicaDataViewSavedState;
        friend class ReplicaDataViewConfigurationDialog;
        
        friend class ReplicaChunkTypeTableViewModel;
        friend class ReplicaChunkTypeDetailView;
        
        friend class ReplicaTableViewModel;
        friend class ReplicaDetailView;

        enum DisplayDataType
        {
            DDT_START = -1,

            DDT_REPLICA,
            DDT_CHUNK,

            DDT_END
        };

        enum TableFilterType
        {
            TFT_START = -1,

            TFT_NONE,
            TFT_ACTIVE_ONLY,

            TFT_END
        };

        static const char* DDT_REPLICA_NAME;
        static const char* DDT_CHUNK_NAME;        

        // Serialization Keys
        static const char* WINDOW_STATE_FORMAT;
        static const char* SPLITTER_STATE_FORMAT;
        static const char* TABLE_STATE_FORMAT;
        static const char* DATA_VIEW_STATE_FORMAT;

        static const char* DATA_VIEW_WORKSPACE_FORMAT;

        // Sizing keys
        static const int INSPECT_ICON_COLUMN_SIZE;
        
    public:

        struct ChartZoomMaintainer
        {
        public:
            ChartZoomMaintainer();            

            void GetZoomFromChart(StripChart::DataStrip& chart, Charts::AxisType axis);
            void SetZoomOnChart(StripChart::DataStrip& chart, Charts::AxisType axis);

        private:

            Charts::AxisType m_axis;
            float m_minValue;
            float m_maxValue;
        };
        
    public:

        AZ_CLASS_ALLOCATOR(ReplicaDataView, AZ::SystemAllocator, 0);

        ReplicaDataView(unsigned int dataViewIndex, FrameNumberType currentFrame, const ReplicaDataAggregator* aggr);
        virtual ~ReplicaDataView();

        // MainWindow Bus Commands
        void FrameChanged(FrameNumberType frame) override;
        void EventFocusChanged(EventNumberType eventIndex) override;
        void EventChanged(EventNumberType eventIndex) override;

        float GetAxisStartFrame() const;
        FrameNumberType GetStartFrame() const;

        float GetAxisEndFrame() const;
        FrameNumberType GetEndFrame() const;

        FrameNumberType GetActiveFrameCount() const;
        FrameNumberType GetCurrentFrame() const;

        bool HideInactiveInspectedElements() const;

        int GetCaptureWindowIdentity() const;
        unsigned int GetAverageFrameBandwidthBudget() const;

        void SignalDialogClosed(QDialog* dialog);

        unsigned int GetDataViewIndex() const;

        // Mimicing the workspace bus, but these need to be invoked manually
        // by the object that creates these windows(since it creates these in response
        // to these events).
        void ApplySettingsFromWorkspace(WorkspaceSettingsProvider*);
        void ActivateWorkspaceSettings(WorkspaceSettingsProvider*);
        void SaveSettingsToWorkspace(WorkspaceSettingsProvider*);
        void ApplyPersistentState();

        static void Reflect(AZ::ReflectContext* context);

    public slots:
        
        void UpdateDisplay(const QModelIndex& startIndex, const QModelIndex& endIndex);

        void RefreshGraph();
        void ReplicaSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
        void ChunkSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
        void OnDisplayRangeChanged(int);        

        void HideAll();
        void ShowAll();
        void SetAllEnabled(bool enabled);

        void HideSelected();
        void ShowSelected();
        void SetSelectedEnabled(bool enabled);

        void OnCellClicked(const QModelIndex& index);
        void OnDoubleClicked(const QModelIndex& idnex);
        
        void OnDataTypeChanged(int selectedIndex);        
        void OnBandwidthUsageDisplayTypeChanged(int selectedIndex);
        void OnTableFilterTypeChanged(int selectedIndex);

        void OnShowOverallStatistics();

        void OnInspectedSeries(size_t seriesId);
        void OnSelectedSeries(size_t seriesId, int position);

    signals:

        void DataRangeChanged();

    private:

        void SaveOnExit();

        void InspectReplica(int tableRow);
        void InspectChunkType(int tableRow);
        
        void RefreshTableView();
        void SetupTableView();
        void SetupReplicaTableView();
        void SetupChunkTableView();

        void DrawFrameGraph();        

        template<typename T>
        void PlotChartDataForFrames(ReplicaBandwidthChartData<T>* chartData)
        {            
            AreaGraphPlotHelper& areaPlotHelper = chartData->GetAreaGraphPlotHelper();
            areaPlotHelper.SetupPlotHelper(m_gui->areaChart, chartData->GetAxisName(),chartData->GetAllFrames().size());                

            if (!areaPlotHelper.IsSetup())
            {
                return;
            }

            areaPlotHelper.SetHighlighted(chartData->IsSelected());
            areaPlotHelper.SetEnabled(chartData->IsEnabled());                

            ReplicaDisplayTypes::BandwidthUsageDisplayType bandwidthDisplayType = GetBandwidthUsageDisplayType();

            for (FrameNumberType frameId = GetStartFrame(); frameId <= GetEndFrame(); ++frameId)
            {
                size_t sentDataUsage = chartData->GetSentUsageForFrame(frameId);
                size_t receivedDataUsage = chartData->GetReceivedUsageForFrame(frameId);                

                switch (bandwidthDisplayType)
                {
                case ReplicaDisplayTypes::BUDT_COMBINED:
                    areaPlotHelper.PlotBatchedData(frameId, static_cast<unsigned int>(sentDataUsage + receivedDataUsage));
                    break;
                case ReplicaDisplayTypes::BUDT_SENT:
                    areaPlotHelper.PlotBatchedData(frameId, static_cast<unsigned int>(sentDataUsage));
                    break;
                case ReplicaDisplayTypes::BUDT_RECEIVED:
                    areaPlotHelper.PlotBatchedData(frameId, static_cast<unsigned int>(receivedDataUsage));
                    break;
                default:
                    AZ_Error("Standalone Tools", false, "Unknown bandwidth display type.");
                    break;
                }
            }
        }

        void InitializeData();
        void UpdateData();        
        void ParseFrameData(FrameNumberType frameId);
        void ParseActiveItems();
        
        int GetDisplayRange() const;
        DisplayDataType GetDisplayDataType() const;
        ReplicaDisplayTypes::BandwidthUsageDisplayType GetBandwidthUsageDisplayType() const;

        ReplicaDataContainer* FindReplicaData(AZ::u64 replicaId) const;
        ReplicaChunkTypeDataContainer* FindReplicaChunkTypeData(const char* chunkType) const;

        typedef AZStd::unordered_map<AZ::u64, ReplicaDataContainer*> ReplicaDataMap;
        typedef AZStd::unordered_set<AZ::u64> ReplicaIdSet;

        typedef AZStd::unordered_map<AZStd::string, ReplicaChunkTypeDataContainer*> ReplicaChunkTypeDataMap;
        typedef AZStd::unordered_set<AZStd::string> ReplicaChunkTypeSet;

        // Used for ordering in the table view/quick mapping of Row -> ReplicaID
        ReplicaDataMap                  m_replicaData;
        ReplicaIdSet                    m_activeReplicaIds;
        ReplicaIdSet                    m_activeInspectedReplicaIds;
        ReplicaTableViewModel           m_replicaTypeTableView;

        ReplicaChunkTypeDataMap         m_replicaChunkTypeData;        
        ReplicaChunkTypeSet             m_activeChunkTypes;
        ReplicaChunkTypeSet             m_activeInspectedChunkTypes;
        ReplicaChunkTypeTableViewModel  m_replicaChunkTypeTableView;        

        unsigned int m_dataViewIndex;
        size_t m_inspectedSeries;

        AZ::u32 m_windowStateCRC;
        AZ::u32 m_splitterStateCRC;
        AZ::u32 m_tableViewCRC;
        AZ::u32 m_dataViewCRC;

        int m_aggregatorIdentity;
        const ReplicaDataAggregator* m_aggregator;
        
        FrameNumberType m_startFrame;
        FrameNumberType m_endFrame;
        FrameNumberType m_currentFrame;

        AZStd::unordered_set< FrameNumberType > m_parsedFrames;        

        OverallReplicaDetailView*           m_overallReplicaDetailView;
        AZStd::vector< ReplicaDetailView* > m_spawnedReplicaDetailViews;
        AZStd::vector< ReplicaChunkTypeDetailView* > m_spawnedChunkDetailViews;

        // Information about the window that we might want to save out
        AZStd::intrusive_ptr<ReplicaDataViewSavedState> m_persistentState;

        DrillerWindowLifepsanTelemetry m_lifespanTelemetry;

        Ui::ReplicaDataView* m_gui;
    };
}

#endif // REPLICADATAVIEW_H
