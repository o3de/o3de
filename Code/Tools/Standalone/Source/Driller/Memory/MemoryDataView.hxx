/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef MEMORYDATAVIEW_H
#define MEMORYDATAVIEW_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <Source/Driller/DrillerMainWindowMessages.h>
#include <Source/Driller/StripChart.hxx>
#include <QMainWindow>
#include <QDialog>
#endif

namespace AZ { class ReflectContext; }

namespace Ui
{
    class MemoryDataView;
}

namespace Driller
{
    /*
    A modeless dialog that combines custom drawing and active widgets.
    */

    class MemoryAxisFormatter : public Charts::QAbstractAxisFormatter
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(MemoryAxisFormatter, AZ::SystemAllocator, 0);
        MemoryAxisFormatter(QObject *pParent);

        static QString formatMemorySize(float value, float scalingValue);
        virtual QString convertAxisValueToText(Charts::AxisType axis, float value, float minDisplayedValue, float maxDisplayedValue, float divisionSize);
        
    private:

    };

    class MemoryDataAggregator;
    class MemoryDataViewSavedState;

    class MemoryDataView
        : public QDialog
        , public Driller::DrillerMainWindowMessages::Bus::Handler
        , public Driller::DrillerEventWindowMessages::Bus::Handler
    {
        Q_OBJECT;
    public:
        AZ_CLASS_ALLOCATOR(MemoryDataView,AZ::SystemAllocator,0);
        MemoryDataView( MemoryDataAggregator *aggregator, FrameNumberType atFrame, int profilerIndex );
        virtual ~MemoryDataView(void);

        MemoryDataAggregator *m_aggregator;
        int m_aggregatorIdentityCached;
        FrameNumberType m_Frame;
        int m_HighestFrameSoFar;
        EventNumberType m_ScrubberIndex;
        AZ::u32 m_windowStateCRC;
        int m_viewIndex;
        AZ::u32 m_viewStateCRC;

        void SetFrameNumber();
        void UpdateChart();

        // NB: These three methods mimic the workspace bus.
        // Because the ProfilerDataAggregator can't know to open these DataView windows
        // until after the EBUS message has gone out, the owning aggregator must
        // first create these windows and then pass along the provider manually
        void ApplySettingsFromWorkspace(WorkspaceSettingsProvider*);
        void ActivateWorkspaceSettings(WorkspaceSettingsProvider*);
        void SaveSettingsToWorkspace(WorkspaceSettingsProvider*);
        void ApplyPersistentState();

        AZStd::intrusive_ptr<MemoryDataViewSavedState> m_persistentState;

        QAction * CreateFilterSelectorAction( QString qs, AZ::u64 id );
        QAction *CreateFrameRangeMenuAction( QString qs, int range );

        void SaveOnExit();
        virtual void closeEvent(QCloseEvent *evt);
        virtual void hideEvent(QHideEvent *evt);

    public:
        // MainWindow Bus Commands
        void FrameChanged(FrameNumberType frame) override;
        void EventFocusChanged(EventNumberType eventIndex) override;
        void EventChanged(EventNumberType /*eventIndex*/) override{}
        
        static void Reflect(AZ::ReflectContext* context);
        
    public slots:
        void OnDataDestroyed();
        void OnViewFull();
        void OnCheckLockRight(int state);
        void onMouseLeftDownDomainValue(float domainValue);
        void onMouseLeftDragDomainValue(float domainValue);
        void onMouseOverDataPoint(int channelID, AZ::u64 sampleID, float primaryAxisValue, float dependentAxisValue);
        void onMouseOverNothing(float primaryAxisValue, float dependentAxisValue);
        void OnFilterButton();
        void OnFilterSelectorMenu();
        void OnFilterSelectorMenu( QString fromMenu, AZ::u64 id );
        void OnFrameRangeMenu();
        void OnAutoZoomChange(bool);

signals:
        void EventRequestEventFocus(AZ::s64);

    private:
        Ui::MemoryDataView* m_gui;
        MemoryAxisFormatter *m_ptrFormatter;
    };

}


#endif // MEMORYDATAVIEW_H
