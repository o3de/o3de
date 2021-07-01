/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include "ProfilerDataView.hxx"
#include <Source/Driller/Profiler/moc_ProfilerDataView.cpp>

#include "ProfilerDataAggregator.hxx"

#include "ProfilerEvents.h"
#include "Source/Driller/DrillerEvent.h"

#include "Source/Driller/ChannelDataView.hxx"
#include "Source/Driller/DrillerMainWindowMessages.h"
#include "Source/Driller/Profiler/ProfilerOperationTelemetryEvent.h"

#include <AzToolsFramework/UI/UICore/QWidgetSavedState.h>
#include <AzToolsFramework/UI/UICore/QTreeViewStateSaver.hxx>

#include <AzCore/Debug/Profiler.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Source/Driller/Workspaces/Workspace.h>

#include <Source/Driller/Profiler/ui_ProfilerDataView.h>

#include <QMenu>
#include <QAction>

namespace Driller
{
    static const char* menuLengthStrings[] =
    {
        "60 Frames",
        "120 Frames",
        "240 Frames",
        "480 Frames",
        NULL
    };
    static const char* menuTypeStrings[] =
    {
        "Incl.Time",
        "Excl.Time",
        "Calls",
        "Acc.Time",
        "Acc.Calls",
        NULL
    };
    static const int menuTypeViews[] =
    {
        Profiler::RegisterInfo::PRT_TIME,
        Profiler::RegisterInfo::PRT_TIME,
        Profiler::RegisterInfo::PRT_VALUE,
        Profiler::RegisterInfo::PRT_TIME,
        Profiler::RegisterInfo::PRT_VALUE
    };
    static const char* menuValueTypeStrings[] =
    {
        "Value 1",
        "Value 2",
        "Value 3",
        "Value 4",
        "Value 5",
        NULL
    };

    class ProfilerDataViewLocal
        : public AZ::UserSettings
    {
    public:
        AZ_RTTI(ProfilerDataViewLocal, "{7E893482-98BC-4017-B52B-5A36D325976B}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(ProfilerDataViewLocal, AZ::SystemAllocator, 0);

        AZStd::vector<AZ::u8> m_treeColumnStorage;

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<ProfilerDataViewLocal>()
                    ->Field("m_treeColumnStorage", &ProfilerDataViewLocal::m_treeColumnStorage)
                    ->Version(1);
            }
        }
    };

    class ProfilerDataViewSavedState
        : public AZ::UserSettings
    {
    public:
        AZ_RTTI(ProfilerDataViewSavedState, "{432824F6-4078-49F6-BE9E-357EF71B8AB8}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(ProfilerDataViewSavedState, AZ::SystemAllocator, 0);

        AZStd::string   m_chartLengthStr;
        AZStd::string   m_chartTypeStr;
        AZStd::string   m_threadIDStr;
        AZ::u64         m_threadID;
        bool            m_autoZoom;
        bool            m_flatView;
        bool            m_deltaData;
        AZStd::vector< AZStd::string > m_treeExpansionData;

        ProfilerDataViewSavedState()
            : m_chartLengthStr(menuLengthStrings[0])
            , m_chartTypeStr(menuTypeStrings[0])
            , m_threadIDStr("All Threads")
            , m_threadID(0)
            , m_autoZoom(true)
            , m_flatView(false)
            , m_deltaData(true)
        {}

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<ProfilerDataViewSavedState>()
                    ->Field("m_chartLengthStr", &ProfilerDataViewSavedState::m_chartLengthStr)
                    ->Field("m_chartTypeStr", &ProfilerDataViewSavedState::m_chartTypeStr)
                    ->Field("m_threadIDStr", &ProfilerDataViewSavedState::m_threadIDStr)
                    ->Field("m_threadID", &ProfilerDataViewSavedState::m_threadID)
                    ->Field("m_flatView", &ProfilerDataViewSavedState::m_flatView)
                    ->Field("m_treeExpansionData", &ProfilerDataViewSavedState::m_treeExpansionData)
                    ->Field("m_autoZoom", &ProfilerDataViewSavedState::m_autoZoom)
                    ->Field("m_deltaData", &ProfilerDataViewSavedState::m_deltaData)
                    ->Version(10);
            }
        }
    };

    ProfilerDataView::ProfilerDataView(ProfilerDataAggregator* aggregator, FrameNumberType atFrame, int profilerIndex, int viewType)
        : QDialog()
        , m_aggregator(aggregator)
        , m_frame(atFrame)
        , m_chartLength(60)
        , m_windowStateCRC(0)
        , m_dataViewStateCRC(0)
        , m_viewIndex(profilerIndex)
        , m_filterThreadID(0)
        , m_viewType(viewType)
        , m_lifespanTelemetry("ProfilerDataView")
    {
        setAttribute(Qt::WA_DeleteOnClose, true);
        setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint);

        show();
        raise();
        activateWindow();
        setFocus();

        m_gui = azcreate(Ui::ProfilerDataView, ());
        m_gui->setupUi(this);

        QByteArray fileName = m_aggregator->GetInspectionFileName().toUtf8();

        AZStd::string treeViewStateStr = AZStd::string::format("PROFILER DATA TREE VIEW STATE %s", fileName.data());
        AZ::u32 treeViewCrc = AZ::Crc32(treeViewStateStr.c_str());
        m_gui->widgetProfilerData->InitializeTreeViewSaving(treeViewCrc);

        for (int i = 0; menuTypeStrings[i]; ++i)
        {
            m_chartTypeStringToViewType.insert(AZStd::make_pair(menuTypeStrings[i], menuTypeViews[i]));
        }

        m_gui->widgetProfilerData->SetViewType(m_viewType);
        setWindowTitle(m_aggregator->GetDialogTitle());

        connect(m_aggregator, SIGNAL(destroyed(QObject*)), this, SLOT(OnDataDestroyed()));

        connect(m_gui->pushButton_ExpandAll, SIGNAL(clicked()), m_gui->widgetProfilerData, SLOT(OnExpandAll()));
        connect(m_gui->pushButton_ExpandAll, SIGNAL(released()), this, SLOT(OnSanityCheck()));
        connect(m_gui->pushButton_HideSelected, SIGNAL(clicked()), m_gui->widgetProfilerData, SLOT(OnHideSelected()));
        connect(m_gui->pushButton_ShowSelected, SIGNAL(clicked()), m_gui->widgetProfilerData, SLOT(OnShowSelected()));
        connect(m_gui->pushButton_InvertHidden, SIGNAL(clicked()), m_gui->widgetProfilerData, SLOT(OnInvertHidden()));
        connect(m_gui->pushButton_HideAll, SIGNAL(clicked()), m_gui->widgetProfilerData, SLOT(OnHideAll()));
        connect(m_gui->pushButton_ShowAll, SIGNAL(clicked()), m_gui->widgetProfilerData, SLOT(OnShowAll()));
        connect(m_gui->checkBoxAutoZoom, SIGNAL(toggled(bool)), m_gui->widgetProfilerData, SLOT(OnAutoZoomChange(bool)));
        connect(m_gui->checkBoxFlatView, SIGNAL(toggled(bool)), m_gui->widgetProfilerData, SLOT(OnFlatView(bool)));
        connect(m_gui->checkBoxDelta, SIGNAL(toggled(bool)), m_gui->widgetProfilerData, SLOT(OnDeltaData(bool)));

        connect(m_gui->widgetDataStrip, SIGNAL(onMouseOverDataPoint(int, AZ::u64, float, float)), m_gui->widgetProfilerData, SLOT(onMouseOverDataPoint(int, AZ::u64, float, float)));
        connect(m_gui->widgetDataStrip, SIGNAL(onMouseOverNothing(float, float)), m_gui->widgetProfilerData, SLOT(onMouseOverNothing(float, float)));

        m_gui->checkBoxAutoZoom->setChecked(true);
        m_gui->checkBoxFlatView->setChecked(false);

        if (m_viewType == Profiler::RegisterInfo::PRT_TIME)
        {
            QMenu* chartTypeMenu = new QMenu(this);

            for (int i = 0; menuTypeStrings[i]; ++i)
            {
                chartTypeMenu->addAction(CreateChartTypeAction(menuTypeStrings[i]));
            }

            m_gui->chartTypeButton->setText("Excl.Time");
            m_gui->chartTypeButton->setMenu(chartTypeMenu);
        }
        else if (m_viewType == Profiler::RegisterInfo::PRT_VALUE)
        {
            QMenu* chartTypeMenu = new QMenu(this);

            for (int i = 0; menuValueTypeStrings[i]; ++i)
            {
                chartTypeMenu->addAction(CreateChartTypeAction(menuValueTypeStrings[i]));
            }

            m_gui->chartTypeButton->setText("Value 1");
            m_gui->chartTypeButton->setMenu(chartTypeMenu);
        }

        QMenu* chartLengthMenu = new QMenu(this);

        for (int i = 0; menuLengthStrings[i]; ++i)
        {
            chartLengthMenu->addAction(CreateChartLengthAction(menuLengthStrings[i]));
        }

        connect(m_gui->threadSelectorButton, SIGNAL(clicked()), this, SLOT(OnThreadSelectorButtonClick()));

        m_gui->chartLengthButton->setText("60 Frames");
        m_gui->chartLengthButton->setMenu(chartLengthMenu);

        m_aggregatorIdentityCached = m_aggregator->GetIdentity();
        DrillerMainWindowMessages::Handler::BusConnect(m_aggregatorIdentityCached);
        DrillerEventWindowMessages::Handler::BusConnect(m_aggregatorIdentityCached);

        m_gui->widgetDataStrip->AddAxis("Frame", 0.0f, 1.0f, false);
        m_gui->widgetDataStrip->AddAxis("", -1.0f, 1.0f, true);

        SetFrameNumber();

        AZStd::string windowStateStr = AZStd::string::format("PROFILER DATA VIEW WINDOW STATE %i", m_viewIndex);
        m_windowStateCRC = AZ::Crc32(windowStateStr.c_str());
        AZStd::intrusive_ptr<AzToolsFramework::QWidgetSavedState> windowState = AZ::UserSettings::Find<AzToolsFramework::QWidgetSavedState>(m_windowStateCRC, AZ::UserSettings::CT_GLOBAL);
        if (windowState)
        {
            windowState->RestoreGeometry(this);
        }

        AZStd::string treeStateStr = AZStd::string::format("PROFILER DATA VIEW LOCAL STATE %i", m_viewIndex);
        m_treeStateCRC = AZ::Crc32(treeStateStr.c_str());
        auto treeState = AZ::UserSettings::Find<ProfilerDataViewLocal>(m_treeStateCRC, AZ::UserSettings::CT_GLOBAL);
        if (treeState)
        {
            QByteArray treeData((const char*)treeState->m_treeColumnStorage.data(), (int)treeState->m_treeColumnStorage.size());
            m_gui->widgetProfilerData->header()->restoreState(treeData);
        }

        AZStd::string dataViewStateStr = AZStd::string::format("PROFILER DATA VIEW STATE %i", m_viewIndex);
        m_dataViewStateCRC = AZ::Crc32(dataViewStateStr.c_str());
        m_persistentState = AZ::UserSettings::CreateFind<ProfilerDataViewSavedState>(m_dataViewStateCRC, AZ::UserSettings::CT_GLOBAL);
        ApplyPersistentState();
    }

    ProfilerDataView::~ProfilerDataView()
    {
        SaveOnExit();
        azdestroy(m_gui);
    }

    QAction* ProfilerDataView::CreateChartTypeAction(QString qs)
    {
        QAction* act = new QAction(qs, this);
        act->setObjectName(qs);
        connect(act, SIGNAL(triggered()), this, SLOT(OnChartTypeMenu()));
        connect(act, SIGNAL(triggered()), m_gui->widgetProfilerData, SLOT(OnChartTypeMenu()));
        return act;
    }

    QAction* ProfilerDataView::CreateChartLengthAction(QString qs)
    {
        QAction* act = new QAction(qs, this);
        act->setObjectName(qs);
        connect(act, SIGNAL(triggered()), this, SLOT(OnChartLengthMenu()));
        return act;
    }

    QAction* ProfilerDataView::CreateThreadSelectorAction(QString qs, AZ::u64 id)
    {
        QAction* act = new QAction(qs, this);
        act->setObjectName(qs);
        act->setData(id);
        connect(act, SIGNAL(triggered()), this, SLOT(OnThreadSelectorMenu()));
        return act;
    }

    void ProfilerDataView::OnThreadSelectorButtonClick()
    {
        QMenu* threadIDMenu = new QMenu(this);
        threadIDMenu->addAction(CreateThreadSelectorAction("All Threads", 0));

        for (auto iter = m_aggregator->m_lifeTimeThreads.begin(); iter != m_aggregator->m_lifeTimeThreads.end(); ++iter)
        {
            auto threadID = iter->first;
            threadIDMenu->addAction(CreateThreadSelectorAction(QString("Thread = %1").arg(threadID), threadID));
        }

        threadIDMenu->exec(QCursor::pos());
        delete threadIDMenu;
    }

    void ProfilerDataView::OnSanityCheck()
    {
        AZ_TracePrintf("ProfilerDataView", "Released");
    }

    void ProfilerDataView::ApplyPersistentState()
    {
        if (m_persistentState)
        {
            // the bridge between our AZStd::string storage and QT's own string type
            QString lengthMenuText(m_persistentState->m_chartLengthStr.c_str());
            OnChartLengthMenu(lengthMenuText);

            QString typeMenuText(m_persistentState->m_chartTypeStr.c_str());
            if (IsStringCompatibleWithType(m_persistentState->m_chartTypeStr))
            {
                OnChartTypeMenu(typeMenuText);
            }

            QString threadMenuText(m_persistentState->m_threadIDStr.c_str());
            OnThreadSelectorMenu(threadMenuText, m_persistentState->m_threadID);

            m_gui->checkBoxAutoZoom->setChecked(m_persistentState->m_autoZoom);
            m_gui->checkBoxFlatView->setChecked(m_persistentState->m_flatView);
            m_gui->checkBoxDelta->setChecked(m_persistentState->m_deltaData);
            m_gui->widgetProfilerData->OnAutoZoomChange(m_persistentState->m_autoZoom);
            m_gui->widgetProfilerData->OnFlatView(m_persistentState->m_flatView);
            m_gui->widgetProfilerData->OnDeltaData(m_persistentState->m_deltaData);

            QSet<QString> qSetQString;
            AZStd::vector< AZStd::string >::iterator iter = m_persistentState->m_treeExpansionData.begin();
            while (iter != m_persistentState->m_treeExpansionData.end())
            {
                qSetQString.insert(QString(iter->c_str()));
                ++iter;
            }

            m_gui->widgetProfilerData->ReadTreeViewStateFrom(qSetQString);
        }
    }

    bool ProfilerDataView::IsStringCompatibleWithType(AZStd::string candidateStr)
    {
        if (m_viewType == Profiler::RegisterInfo::PRT_TIME)
        {
            for (int i = 0; menuTypeStrings[i]; ++i)
            {
                if (candidateStr == menuTypeStrings[i])
                {
                    return true;
                }
            }
        }
        if (m_viewType == Profiler::RegisterInfo::PRT_VALUE)
        {
            for (int i = 0; menuValueTypeStrings[i]; ++i)
            {
                if (candidateStr == menuValueTypeStrings[i])
                {
                    return true;
                }
            }
        }

        return false;
    }

    void ProfilerDataView::OnChartTypeMenu()
    {
        QAction* qa = qobject_cast<QAction*>(sender());
        if (qa)
        {
            OnChartTypeMenu(qa->objectName());
        }
    }

    void ProfilerDataView::OnChartTypeMenu(QString menuText)
    {
        m_gui->chartTypeButton->setText(menuText);
        m_gui->widgetProfilerData->OnChartTypeMenu(menuText);
    }

    void ProfilerDataView::OnChartLengthMenu()
    {
        QAction* qa = qobject_cast<QAction*>(sender());
        if (qa)
        {
            OnChartLengthMenu(qa->objectName());
        }
    }

    void ProfilerDataView::OnChartLengthMenu(QString fromMenu)
    {
        ProfilerOperationTelemetryEvent chartLengthChange;
        chartLengthChange.SetAttribute("ChartLength", fromMenu.toStdString().c_str());
        chartLengthChange.Log();

        m_gui->chartLengthButton->setText(fromMenu);

        if (fromMenu == "60 Frames")
        {
            m_chartLength = 60;
        }
        if (fromMenu == "120 Frames")
        {
            m_chartLength = 120;
        }
        if (fromMenu == "240 Frames")
        {
            m_chartLength = 240;
        }
        if (fromMenu == "480 Frames")
        {
            m_chartLength = 480;
        }

        SetFrameNumber();
    }

    void ProfilerDataView::OnThreadSelectorMenu()
    {
        QAction* qa = qobject_cast<QAction*>(sender());
        if (qa)
        {
            OnThreadSelectorMenu(qa->objectName(), qa->data().toULongLong());
        }
    }

    void ProfilerDataView::OnThreadSelectorMenu(QString fromMenu, AZ::u64 id)
    {
        const char* k_changeThreadFilter = "ThreadFilter";

        ProfilerOperationTelemetryEvent threadSelector;

        m_gui->threadSelectorButton->setText(fromMenu);
        m_persistentState->m_threadIDStr = fromMenu.toUtf8().data();
        m_persistentState->m_threadID = id;

        if (id == 0)
        {
            threadSelector.SetAttribute(k_changeThreadFilter, "All Threads");
            m_filterThreadID = 0;
        }
        else
        {
            threadSelector.SetAttribute(k_changeThreadFilter, m_persistentState->m_threadIDStr);
            m_filterThreadID = id;
        }

        threadSelector.Log();

        // force a new data set
        SetFrameNumber();
    }

    void ProfilerDataView::SaveOnExit()
    {
        auto treeState = AZ::UserSettings::CreateFind<ProfilerDataViewLocal>(m_treeStateCRC, AZ::UserSettings::CT_GLOBAL);
        if (treeState)
        {
            if (m_gui->widgetProfilerData && m_gui->widgetProfilerData->header())
            {
                QByteArray qba = m_gui->widgetProfilerData->header()->saveState();
                treeState->m_treeColumnStorage.assign((AZ::u8*)qba.begin(), (AZ::u8*)qba.end());
            }
        }

        auto pState = AZ::UserSettings::CreateFind<AzToolsFramework::QWidgetSavedState>(m_windowStateCRC, AZ::UserSettings::CT_GLOBAL);
        if (m_persistentState)
        {
            if (
                m_gui->chartLengthButton
                && m_gui->chartTypeButton
                && m_gui->threadSelectorButton
                && m_gui->checkBoxAutoZoom
                && m_gui->checkBoxDelta
                && m_gui->widgetProfilerData
                && m_gui->widgetProfilerData->IsTreeViewSavingReady()
                )
            {
                pState->CaptureGeometry(this);
                m_persistentState->m_chartLengthStr = m_gui->chartLengthButton->text().toUtf8().data();
                m_persistentState->m_chartTypeStr = m_gui->chartTypeButton->text().toUtf8().data();
                m_persistentState->m_threadIDStr = m_gui->threadSelectorButton->text().toUtf8().data();
                m_persistentState->m_autoZoom = (int)m_gui->checkBoxAutoZoom->isChecked();
                m_persistentState->m_flatView = (int)m_gui->checkBoxFlatView->isChecked();
                m_persistentState->m_deltaData = (int)m_gui->checkBoxDelta->isChecked();

                m_persistentState->m_treeExpansionData.clear();
                QSet<QString> qSetQString;
                m_gui->widgetProfilerData->WriteTreeViewStateTo(qSetQString);
                QSet<QString>::iterator iter = qSetQString.begin();
                for (auto iterStrings = qSetQString.begin(); iterStrings != qSetQString.end(); ++iterStrings)
                {
                    m_persistentState->m_treeExpansionData.push_back(iterStrings->toUtf8().data());
                }
            }
        }
    }
    void ProfilerDataView::hideEvent(QHideEvent* evt)
    {
        QDialog::hideEvent(evt);
    }
    void ProfilerDataView::closeEvent(QCloseEvent* evt)
    {
        DrillerEventWindowMessages::Handler::BusDisconnect(m_aggregatorIdentityCached);
        DrillerMainWindowMessages::Handler::BusDisconnect(m_aggregatorIdentityCached);
        QDialog::closeEvent(evt);
    }
    void ProfilerDataView::OnDataDestroyed()
    {
        deleteLater();
    }

    void ProfilerDataView::FrameChanged(FrameNumberType frame)
    {
        m_frame = frame;
        SetFrameNumber();
    }

    void ProfilerDataView::SetFrameNumber()
    {
        size_t numEvents = m_aggregator->NumOfEventsAtFrame(m_frame);

        m_aggregator->FrameChanged(m_frame);

        m_gui->widgetProfilerData->BeginDataModelUpdate();

        if (numEvents)
        {
            m_gui->widgetProfilerData->m_dataModel->SetAggregator(m_aggregator);

            for (EventNumberType eventIndex = m_aggregator->m_frameToEventIndex[m_frame]; eventIndex < static_cast<EventNumberType>(m_aggregator->m_frameToEventIndex[m_frame] + numEvents); ++eventIndex)
            {
                Driller::DrillerEvent* dep = m_aggregator->GetEvents()[ eventIndex ];
                if (dep->GetEventType() == Driller::Profiler::PET_UPDATE_REGISTER)
                {
                    Driller::ProfilerDrillerUpdateRegisterEvent* regEvt = static_cast<Driller::ProfilerDrillerUpdateRegisterEvent*>(dep);

                    if (regEvt->GetRegister())
                    {
                        if (m_filterThreadID == 0 || regEvt->GetRegister()->GetInfo().m_threadId == m_filterThreadID)
                        {
                            m_gui->widgetProfilerData->m_dataModel->AddRegister(regEvt);
                        }
                    }
                }
            }

            m_gui->widgetProfilerData->EndDataModelUpdate();

            // build the chart
            // this data view responsible for length of history setting and deciding what kind of data is displayed
            // that data panel, which owns the data model, responsible for each register on/off and charting
            m_gui->widgetProfilerData->ConfigureChart(m_gui->widgetDataStrip, m_frame, m_chartLength, static_cast<FrameNumberType>(m_aggregator->GetFrameCount())); // magic number 60 = frame count to go back in time
        }
    }


    void ProfilerDataView::ApplySettingsFromWorkspace(WorkspaceSettingsProvider* provider)
    {
        AZStd::string workspaceStateStr = AZStd::string::format("PROFILER DATA VIEW WORKSPACE STATE %i", m_viewIndex);
        AZ::u32 workspaceStateCRC = AZ::Crc32(workspaceStateStr.c_str());

        if (m_persistentState)
        {
            ProfilerDataViewSavedState* workspace = provider->FindSetting<ProfilerDataViewSavedState>(workspaceStateCRC);
            if (workspace)
            {
                m_persistentState->m_chartLengthStr = workspace->m_chartLengthStr;
                m_persistentState->m_chartTypeStr = workspace->m_chartTypeStr;
                m_persistentState->m_threadIDStr = workspace->m_threadIDStr;
                m_persistentState->m_threadID = workspace->m_threadID;
                m_persistentState->m_flatView = workspace->m_flatView;
                m_persistentState->m_autoZoom = workspace->m_autoZoom;
                m_persistentState->m_deltaData = workspace->m_deltaData;
                m_persistentState->m_treeExpansionData = workspace->m_treeExpansionData;
            }
        }
    }

    void ProfilerDataView::ActivateWorkspaceSettings(WorkspaceSettingsProvider*)
    {
        ApplyPersistentState();
    }

    void ProfilerDataView::SaveSettingsToWorkspace(WorkspaceSettingsProvider* provider)
    {
        AZStd::string workspaceStateStr = AZStd::string::format("PROFILER DATA VIEW WORKSPACE STATE %i", m_viewIndex);
        AZ::u32 workspaceStateCRC = AZ::Crc32(workspaceStateStr.c_str());

        if (m_persistentState)
        {
            ProfilerDataViewSavedState* workspace = provider->CreateSetting<ProfilerDataViewSavedState>(workspaceStateCRC);
            if (workspace)
            {
                workspace->m_chartLengthStr = m_persistentState->m_chartLengthStr;
                workspace->m_chartTypeStr = m_persistentState->m_chartTypeStr;
                workspace->m_threadIDStr = m_persistentState->m_threadIDStr;
                workspace->m_threadID = m_persistentState->m_threadID;
                workspace->m_flatView = m_persistentState->m_flatView;
                workspace->m_autoZoom = m_persistentState->m_autoZoom;
                workspace->m_deltaData = m_persistentState->m_deltaData;
                workspace->m_treeExpansionData = m_persistentState->m_treeExpansionData;
            }
        }
    }

    void ProfilerDataView::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            ProfilerDataViewSavedState::Reflect(context);
            ProfilerDataViewLocal::Reflect(context);

            // Driller doesn't use AzToolsFramework directly, so we have to initialize the serialization for the QTreeViewStateSaver
            AzToolsFramework::QTreeViewWithStateSaving::Reflect(context);
        }
    }
}
