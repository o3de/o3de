/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef PROFILERDATAVIEW_H
#define PROFILERDATAVIEW_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/string/string.h>

#include "Source/Driller/DrillerMainWindowMessages.h"
#include "Source/Driller/DrillerOperationTelemetryEvent.h"

#include <QMainWindow>
#include <QDialog>

#include "Source/Driller/DrillerDataTypes.h"
#endif

namespace Ui
{
    class ProfilerDataView;
}

namespace AZ { class ReflectContext; }

namespace Driller
{
    class ProfilerDataAggregator;
    class ProfilerDataViewSavedState;

    class ProfilerDataView 
        : public QDialog
        , public Driller::DrillerMainWindowMessages::Bus::Handler
        , public Driller::DrillerEventWindowMessages::Bus::Handler
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(ProfilerDataView,AZ::SystemAllocator,0);
        ProfilerDataView( ProfilerDataAggregator *aggregator, FrameNumberType atFrame, int profilerIndex, int viewType );
        virtual ~ProfilerDataView(void);

        void SetFrameNumber();
        // called after loading user settings or applying workspace settings on top of local user settings
        int GetViewType() { return m_viewType; }

        // MainWindow Bus Commands
        void FrameChanged(FrameNumberType frame) override;
        void EventFocusChanged(EventNumberType /*eventIndex*/ ) override { };
        void EventChanged(EventNumberType /*eventIndex*/) override {}

        // NB: These three methods mimic the workspace bus.
        // Because the ProfilerDataAggregator can't know to open these DataView windows
        // until after the EBUS message has gone out, the owning aggregator must
        // first create these windows and then pass along the provider manually
        void ApplySettingsFromWorkspace(WorkspaceSettingsProvider*);
        void ActivateWorkspaceSettings(WorkspaceSettingsProvider*);
        void SaveSettingsToWorkspace(WorkspaceSettingsProvider*);
        void ApplyPersistentState();

        void SaveOnExit();
        virtual void closeEvent(QCloseEvent *evt);
        virtual void hideEvent(QHideEvent *evt);

        ProfilerDataAggregator *m_aggregator;
        // persistent state is used as if they were internal variables,
        // though they reside in a storage class
        // this lasts for the entire lifetime of this object
        AZStd::intrusive_ptr<ProfilerDataViewSavedState> m_persistentState;

        FrameNumberType m_frame;
        int m_aggregatorIdentityCached;        
        AZ::u32 m_windowStateCRC;
        AZ::u32 m_dataViewStateCRC;
        int m_viewIndex;
        QMenu *m_threadIDMenu;
        AZ::u64 m_filterThreadID;
        int m_viewType;
        AZ::u32 m_treeStateCRC;
        AZStd::unordered_map<const char*,int> m_chartTypeStringToViewType;

    public:

        QAction *CreateChartTypeAction( QString qs );
        QAction *CreateChartLengthAction( QString qs );
        QAction *CreateThreadSelectorAction( QString qs, AZ::u64 id );
        void ClearThreadSelectorActions();
        bool IsStringCompatibleWithType(AZStd::string candidateStr);

        int m_chartLength;


    public:
        static void Reflect(AZ::ReflectContext* context);

    public slots:
        void OnDataDestroyed();
        void OnChartTypeMenu();
        void OnChartTypeMenu( QString fromMenu );
        void OnChartLengthMenu();
        void OnChartLengthMenu( QString fromMenu );
        void OnThreadSelectorMenu();
        void OnThreadSelectorMenu( QString fromMenu, AZ::u64 id );
        void OnThreadSelectorButtonClick();
        void OnSanityCheck();

    private:
        DrillerWindowLifepsanTelemetry m_lifespanTelemetry;
        Ui::ProfilerDataView* m_gui;
    };

}


#endif // PROFILERDATAVIEW_H
