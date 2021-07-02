/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef PROFILER_DATA_PANEL_H
#define PROFILER_DATA_PANEL_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/unordered_map.h>

#include <QtCore/QObject>
#include <QtWidgets/QWidget>
#include <QtWidgets/QTreeWidget>
#include <Source/Driller/StripChart.hxx>

#include <Source/Driller/DrillerDataTypes.h>

#include <AzToolsFramework/UI/UICore/QTreeViewStateSaver.hxx>
#endif

namespace StripChart
{
    class DataStrip;
}

class QSortFilterProxyModel;

#pragma once


namespace Driller
{
    class Aggregator;
    class ProfilerDrillerUpdateRegisterEvent;
    class ProfilerDataAggregator;
    class ProfilerFilterModel;
    class ProfilerDrillerNewRegisterEvent;

    class ProfilerDataModel : public QAbstractItemModel
    {
        Q_OBJECT;

    public:

        friend class ProfilerDataWidget;

        ProfilerDataModel();
        ~ProfilerDataModel();

        virtual QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
        virtual Qt::ItemFlags flags ( const QModelIndex & index ) const;
        virtual QModelIndex index ( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
        virtual QModelIndex parent ( const QModelIndex & index ) const;
        virtual int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
        virtual int columnCount ( const QModelIndex & parent = QModelIndex() ) const;
        virtual QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

        void EmptyTheEventCache();
        void BeginAddRegisters();
        virtual void AddRegister( const Driller::ProfilerDrillerUpdateRegisterEvent *newData );
        void SetAggregator( Driller::ProfilerDataAggregator *aggregator );
        void EndAddRegisters();
        void Recolor();
        void SetFlatView( bool on );
        void SetDeltaData( bool on );

        void SetHighlightedRegisterID(AZ::u64 regid);

    protected:
        // the data = a tree of accumulated profiled events
        // cached locally as a vector of pointers into the aggregator data block because
        // the aggregator data block is a stream of different types of events
        typedef AZStd::vector<const Driller::ProfilerDrillerUpdateRegisterEvent*> DVVector; // aggregator hosted pointers guaranteed to not disappear
        DVVector m_profilerDrillerUpdateRegisterEvents;
        Driller::ProfilerDataAggregator *m_SourceAggregator;

        QColor GetColorByIndex( int colorIdx, int maxNumColors );
        static int m_colorIndexTracker;
        AZStd::map<AZ::u64, QColor> m_colorMap;
        AZStd::map<AZ::u64, QIcon> m_iconMap;
        AZStd::map<AZ::u64, int> m_enabledChartingMap;
        AZ::u64 m_totalTime; // used in percentage calculation
        bool m_cachedFlatView;
        bool m_cachedDeltaData;
        AZ::u64 m_highlightedRegisterID;
        QPersistentModelIndex m_LastHighlightedRegister;
    };

    class ProfilerCounterDataModel : public ProfilerDataModel
    {
        Q_OBJECT;

    public:

        friend class ProfilerDataWidget;

        ProfilerCounterDataModel();
        ~ProfilerCounterDataModel();

        virtual QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
        virtual int columnCount ( const QModelIndex & parent = QModelIndex() ) const;
        virtual QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
        virtual void AddRegister( const Driller::ProfilerDrillerUpdateRegisterEvent *newData );
    };

    class ProfilerAxisFormatter : public Charts::QAbstractAxisFormatter
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(ProfilerAxisFormatter, AZ::SystemAllocator, 0);
        ProfilerAxisFormatter(QObject *pParent, int whichTypeOfRegister);

        QString formatMicroseconds(float value);
        virtual QString convertAxisValueToText(Charts::AxisType axis, float value, float minDisplayedValue, float maxDisplayedValue, float divisionSize);
        
    private:
        float m_lastAxisValueForScaling;
        int m_whatKindOfRegister;

    };

    class ProfilerDataWidget
        : public AzToolsFramework::QTreeViewWithStateSaving
    {
        Q_OBJECT;
    public:

        friend class ProfilerDataView;

        ProfilerDataWidget( QWidget * parent = 0 );
        virtual ~ProfilerDataWidget();

        void SetViewType(int viewType);
        void BeginDataModelUpdate();
        void EndDataModelUpdate();
        void ConfigureChart( StripChart::DataStrip *chart, FrameNumberType atFrame, int howFar, FrameNumberType frameCount );

        public slots:
            void OnExpandAll();
            void OnHideSelected();
            void OnShowSelected();
            void OnInvertHidden();
            void OnHideAll();
            void OnShowAll();
            void OnChartTypeMenu();
            void OnChartTypeMenu(QString typeStr);
            void OnAutoZoomChange(bool newValue);
            void OnFlatView(bool);
            void OnDeltaData(bool);
            void ProvideData(StripChart::DataStrip*);

    protected:

        void RedrawChart();
        void PlotTimeHistory( StripChart::DataStrip *chart );

        ProfilerDataModel *m_dataModel;
        QSortFilterProxyModel* m_filterModel;
        StripChart::DataStrip *m_cachedChart;
        ProfilerAxisFormatter* m_formatter;

        FrameNumberType m_cachedStartFrame;
        FrameNumberType m_cachedEndFrame;
        FrameNumberType m_cachedCurrentFrame;
        FrameNumberType m_cachedDisplayRange;
        
        int m_cachedColumn;
        bool m_autoZoom; // do we automatically zoom extents?
        float m_manualZoomMin; // if we're not automatically zooming, then we remember the prior zoom to re-apply it
        float m_manualZoomMax;
        bool m_cachedFlatView;
        bool m_cachedDeltaData;
        
        int m_viewType;

        typedef AZStd::unordered_map<int, const ProfilerDrillerNewRegisterEvent*> ChannelIDToRegisterMap;

        ChannelIDToRegisterMap m_ChannelsToRegisters;

        int m_iLastHighlightedChannel;

        public slots:
            void OnDoubleClicked( const QModelIndex & );
            void selectionChanged( const QItemSelection & selected, const QItemSelection & deselected );
            void onMouseOverDataPoint(int channelID, AZ::u64 sampleID,float primaryAxisValue, float dependentAxisValue);
            void onMouseOverNothing(float primaryAxisValue, float dependentAxisValue);
    };

    
}

#endif //PROFILER_DATA_PANEL_H
