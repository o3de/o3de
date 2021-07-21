/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include "MemoryDataView.hxx"
#include <Source/Driller/Memory/moc_MemoryDataView.cpp>

#include "MemoryDataAggregator.hxx"
#include "MemoryEvents.h"
#include "Source/Driller/DrillerEvent.h"
#include <AzToolsFramework/UI/UICore/QWidgetSavedState.h>

#include "Source/Driller/ChannelDataView.hxx"
#include "Source/Driller/DrillerMainWindowMessages.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <Source/Driller/Workspaces/Workspace.h>

#include <Source/Driller/Memory/ui_MemoryDataView.h>

#include <QMenu>
#include <QAction>
#include <QToolTip>

namespace Driller
{
    static const char* frameRangeToDisplayString[] =
    {
        "Show 1 Frame",
        "Show 2 Frames",
        "Show 5 Frames",
        "Show 15 Frames",
        "Show 30 Frames",
        "Show 60 Frames",
        "Show 120 Frames",
        NULL
    };
    static const int frameRangeFromIndex[] = { 1, 2, 5, 15, 30, 60, 120, 0 };

    MemoryAxisFormatter::MemoryAxisFormatter(QObject* pParent)
        : QAbstractAxisFormatter(pParent)
    {
    }

    QString MemoryAxisFormatter::formatMemorySize(float value, float scalingValue)
    {
        // data is in whole byte numbers.

        // so how big is the division size?
        if (scalingValue > 128.0f * 1024.0f) // greater than a 0.1 mb
        {
            if (scalingValue > 1024.0f * 1024.0f) // whole seconds
            {
                return QObject::tr("%1MB").arg(QString::number(value / (1024.0f * 1024.0f), 'f', 0));
            }
            else
            {
                return QObject::tr("%1MB").arg(QString::number(value / (1024.0f * 1024.0f), 'f', 1));
            }
        }
        else if (scalingValue > 128.0f) // greater than a 128 bytes
        {
            if (scalingValue > 1024.0f) // whole kilobytes
            {
                return QObject::tr("%1KB").arg(QString::number(value / 1024.0f, 'f', 0));
            }
            else
            {
                return QObject::tr("%1KB").arg(QString::number(value / 1024, 'f', 1));
            }
        }
        else
        {
            return QObject::tr("%1B").arg((AZ::s64)value);
        }
    }


    QString MemoryAxisFormatter::convertAxisValueToText(Charts::AxisType axis, float value, float /*minDisplayedValue*/, float /*maxDisplayedValue*/, float divisionSize)
    {
        if (axis == Charts::AxisType::Vertical)
        {
            return formatMemorySize(value, divisionSize);
        }
        else
        {
            return QString::number((int)value);
        }
    };


    class MemoryDataViewSavedState
        : public AZ::UserSettings
    {
    public:
        AZ_RTTI(MemoryDataViewSavedState, "{1F25755D-8477-48B3-AAB5-6CDBB4152723}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(MemoryDataViewSavedState, AZ::SystemAllocator, 0);

        AZStd::string   m_filterMenuString;
        AZ::u64         m_filterId;
        int             m_frameRange;
        bool            m_autoZoom;
        float           m_manualZoomMin; // if we're not automatically zooming, then we remember the prior zoom to re-apply it
        float           m_manualZoomMax;

        MemoryDataViewSavedState()
            : m_filterMenuString("Filter: All")
            , m_filterId(0)
            , m_frameRange(1)
            , m_autoZoom(true)
            , m_manualZoomMin(2000000000.0f)
            , m_manualZoomMax(-2000000000.0f)
        {}

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<MemoryDataViewSavedState>()
                    ->Field("m_filterMenuString", &MemoryDataViewSavedState::m_filterMenuString)
                    ->Field("m_filterId", &MemoryDataViewSavedState::m_filterId)
                    ->Field("m_frameRange", &MemoryDataViewSavedState::m_frameRange)
                    ->Field("m_autoZoom", &MemoryDataViewSavedState::m_autoZoom)
                    ->Field("m_manualZoomMin", &MemoryDataViewSavedState::m_manualZoomMin)
                    ->Field("m_manualZoomMax", &MemoryDataViewSavedState::m_manualZoomMax)
                    ->Version(3);
            }
        }
    };


    MemoryDataView::MemoryDataView(MemoryDataAggregator* aggregator, FrameNumberType atFrame, int profilerIndex)
        : QDialog()
        , m_aggregator(aggregator)
        , m_Frame(atFrame)
        , m_HighestFrameSoFar(-1)
        , m_viewIndex(profilerIndex)
    {
        setAttribute(Qt::WA_DeleteOnClose, true);
        setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint);

        m_ScrubberIndex = 0;

        show();
        raise();
        activateWindow();
        setFocus();

        m_gui = azcreate(Ui::MemoryDataView, ());
        m_gui->setupUi(this);

        setWindowTitle(QString("Memory Data View %1 from %2").arg(profilerIndex).arg(aggregator->GetIdentity()));

        m_ptrFormatter = aznew MemoryAxisFormatter(this);
        m_gui->widgetDataStrip->SetAxisTextFormatter(m_ptrFormatter);

        connect(m_aggregator, SIGNAL(destroyed(QObject*)), this, SLOT(OnDataDestroyed()));

        connect(m_gui->widgetDataStrip, SIGNAL(onMouseLeftDownDomainValue(float)), this, SLOT(onMouseLeftDownDomainValue(float)));
        connect(m_gui->widgetDataStrip, SIGNAL(onMouseLeftDragDomainValue(float)), this, SLOT(onMouseLeftDragDomainValue(float)));

        connect(m_gui->widgetDataStrip, SIGNAL(onMouseOverDataPoint(int, AZ::u64, float, float)), this, SLOT(onMouseOverDataPoint(int, AZ::u64, float, float)));
        connect(m_gui->widgetDataStrip, SIGNAL(onMouseOverNothing(float, float)), this, SLOT(onMouseOverNothing(float, float)));

        connect(m_gui->checkLockRight, SIGNAL(stateChanged(int)), this, SLOT(OnCheckLockRight(int)));
        connect(m_gui->buttonViewFull, SIGNAL(pressed()), this, SLOT(OnViewFull()));

        connect(m_gui->filterButton, SIGNAL(pressed()), this, SLOT(OnFilterButton()));

        connect(m_gui->checkBoxAutoZoom, SIGNAL(toggled(bool)), this, SLOT(OnAutoZoomChange(bool)));

        {
            QMenu* frameRangeMenu = new QMenu(this);
            for (int i = 0; frameRangeToDisplayString[i]; ++i)
            {
                frameRangeMenu->addAction(CreateFrameRangeMenuAction(frameRangeToDisplayString[i], frameRangeFromIndex[i]));
            }

            m_gui->frameRangeButton->setText(frameRangeToDisplayString[0]);
            m_gui->frameRangeButton->setMenu(frameRangeMenu);
        }

        m_aggregatorIdentityCached = m_aggregator->GetIdentity();
        DrillerMainWindowMessages::Handler::BusConnect(m_aggregatorIdentityCached);
        DrillerEventWindowMessages::Handler::BusConnect(m_aggregatorIdentityCached);

        AZStd::string windowStateStr = AZStd::string::format("MEMORY DATA VIEW WINDOW STATE %i", m_viewIndex);
        m_windowStateCRC = AZ::Crc32(windowStateStr.c_str());
        AZStd::intrusive_ptr<AzToolsFramework::QWidgetSavedState> windowState = AZ::UserSettings::Find<AzToolsFramework::QWidgetSavedState>(m_windowStateCRC, AZ::UserSettings::CT_GLOBAL);
        if (windowState)
        {
            windowState->RestoreGeometry(this);
        }

        AZStd::string dataViewStateStr = AZStd::string::format("MEMORY DATA VIEW STATE %i", m_viewIndex);
        m_viewStateCRC = AZ::Crc32(dataViewStateStr.c_str());
        m_persistentState = AZ::UserSettings::CreateFind<MemoryDataViewSavedState>(m_viewStateCRC, AZ::UserSettings::CT_GLOBAL);
        ApplyPersistentState();

        SetFrameNumber();
    }

    MemoryDataView::~MemoryDataView()
    {
        SaveOnExit();
        azdestroy(m_gui);
    }

    void MemoryDataView::SaveOnExit()
    {
        DrillerEventWindowMessages::Handler::BusDisconnect(m_aggregatorIdentityCached);
        DrillerMainWindowMessages::Handler::BusDisconnect(m_aggregatorIdentityCached);

        AZStd::intrusive_ptr<AzToolsFramework::QWidgetSavedState> pState = AZ::UserSettings::CreateFind<AzToolsFramework::QWidgetSavedState>(m_windowStateCRC, AZ::UserSettings::CT_GLOBAL);
        pState->CaptureGeometry(this);
    }
    void MemoryDataView::hideEvent(QHideEvent* evt)
    {
        QDialog::hideEvent(evt);
    }
    void MemoryDataView::closeEvent(QCloseEvent* evt)
    {
        QDialog::closeEvent(evt);
    }
    void MemoryDataView::OnDataDestroyed()
    {
        deleteLater();
    }

    QAction* MemoryDataView::CreateFrameRangeMenuAction(QString qs, int range)
    {
        QAction* act = new QAction(qs, this);
        act->setObjectName(qs);
        act->setProperty("Range", range);
        connect(act, SIGNAL(triggered()), this, SLOT(OnFrameRangeMenu()));
        return act;
    }

    QAction* MemoryDataView::CreateFilterSelectorAction(QString qs, AZ::u64 id)
    {
        QAction* act = new QAction(qs, this);
        act->setObjectName(qs);
        act->setData(id);
        connect(act, SIGNAL(triggered()), this, SLOT(OnFilterSelectorMenu()));
        return act;
    }

    void MemoryDataView::OnFrameRangeMenu()
    {
        QAction* qa = qobject_cast<QAction*>(sender());
        if (qa)
        {
            m_gui->frameRangeButton->setText(qa->objectName());
            int range = qa->property("Range").toInt();
            m_persistentState->m_frameRange = range;

            // force a new data build
            SetFrameNumber();
            update();
        }
    }

    void MemoryDataView::OnFilterButton()
    {
        QMenu* filterIDMenu = new QMenu(this);
        filterIDMenu->addAction(CreateFilterSelectorAction("Filter: All", 0));

        for (auto iter = m_aggregator->m_allocators.begin(); iter != m_aggregator->m_allocators.end(); ++iter)
        {
            filterIDMenu->addAction(CreateFilterSelectorAction(QString("Filter: %1").arg((*iter)->m_name), (*iter)->m_id));
        }

        filterIDMenu->exec(QCursor::pos());
        delete filterIDMenu;
    }

    void MemoryDataView::OnFilterSelectorMenu()
    {
        QAction* qa = qobject_cast<QAction*>(sender());
        if (qa)
        {
            OnFilterSelectorMenu(qa->objectName(), qa->data().toULongLong());
        }
    }

    void MemoryDataView::OnFilterSelectorMenu(QString fromMenu, AZ::u64 id)
    {
        m_gui->filterButton->setText(fromMenu);

        m_persistentState->m_filterMenuString = fromMenu.toUtf8().data();
        m_persistentState->m_filterId = id;

        // force a new data build
        SetFrameNumber();
        update();
    }

    void MemoryDataView::OnAutoZoomChange(bool newValue)
    {
        if (!newValue)
        {
            m_persistentState->m_autoZoom = false;
            m_gui->widgetDataStrip->GetWindowRange(Charts::AxisType::Vertical, m_persistentState->m_manualZoomMin, m_persistentState->m_manualZoomMax);
        }
        else
        {
            m_persistentState->m_autoZoom = true;
            m_persistentState->m_manualZoomMin = 2000000000.0f;
            m_persistentState->m_manualZoomMax = -2000000000.0f;
        }

        UpdateChart();
    }

    void MemoryDataView::ApplyPersistentState()
    {
        if (m_persistentState)
        {
            m_gui->checkBoxAutoZoom->setChecked(m_persistentState->m_autoZoom);
            OnAutoZoomChange(m_persistentState->m_autoZoom);

            OnFilterSelectorMenu(m_persistentState->m_filterMenuString.c_str(), m_persistentState->m_filterId);
            m_gui->frameRangeButton->setText(frameRangeToDisplayString[0]);
            for (int i = 0; frameRangeFromIndex[i]; ++i)
            {
                if (m_persistentState->m_frameRange == frameRangeFromIndex[i])
                {
                    m_gui->frameRangeButton->setText(frameRangeToDisplayString[i]);
                    break;
                }
            }
        }
    }

    void MemoryDataView::ApplySettingsFromWorkspace(WorkspaceSettingsProvider* provider)
    {
        AZStd::string workspaceStateStr = AZStd::string::format("MEMORY DATA VIEW WORKSPACE STATE %i", m_viewIndex);
        AZ::u32 workspaceStateCRC = AZ::Crc32(workspaceStateStr.c_str());

        if (m_persistentState)
        {
            MemoryDataViewSavedState* workspace = provider->FindSetting<MemoryDataViewSavedState>(workspaceStateCRC);
            if (workspace)
            {
                m_persistentState->m_filterMenuString = workspace->m_filterMenuString;
                m_persistentState->m_filterId = workspace->m_filterId;
                m_persistentState->m_frameRange = workspace->m_frameRange;
            }
        }
    }

    void MemoryDataView::ActivateWorkspaceSettings(WorkspaceSettingsProvider*)
    {
        ApplyPersistentState();
    }

    void MemoryDataView::SaveSettingsToWorkspace(WorkspaceSettingsProvider* provider)
    {
        AZStd::string workspaceStateStr = AZStd::string::format("MEMORY DATA VIEW WORKSPACE STATE %i", m_viewIndex);
        AZ::u32 workspaceStateCRC = AZ::Crc32(workspaceStateStr.c_str());

        if (m_persistentState)
        {
            MemoryDataViewSavedState* workspace = provider->CreateSetting<MemoryDataViewSavedState>(workspaceStateCRC);
            if (workspace)
            {
                workspace->m_filterMenuString = m_persistentState->m_filterMenuString;
                workspace->m_filterId = m_persistentState->m_filterId;
                workspace->m_frameRange = m_persistentState->m_frameRange;
            }
        }
    }

    void MemoryDataView::onMouseOverDataPoint(int channelID, AZ::u64 sampleID, float primaryAxisValue, float dependentAxisValue)
    {
        (void)primaryAxisValue;
        (void)dependentAxisValue;
        (void)channelID;
        DrillerEvent* dep = m_aggregator->GetEvents()[ sampleID ];
        QString finalText;

        // highlight both channels

        m_gui->widgetDataStrip->SetChannelSampleHighlight(0, sampleID, true);
        m_gui->widgetDataStrip->SetChannelSampleHighlight(1, sampleID, true);

        switch (dep->GetEventType())
        {
        case Driller::Memory::MET_REGISTER_ALLOCATION:
        {
            Memory::AllocationInfo* mai = &static_cast<MemoryDrillerRegisterAllocationEvent*>(dep)->m_allocationInfo;

            finalText = tr("<U><B>ALLOCATE %1</B><U><BR>%2:%3<BR>%4")
                    .arg(m_ptrFormatter->formatMemorySize((float)mai->m_size, (float)mai->m_size))
                    .arg(mai->m_fileName ? mai->m_fileName : "")
                    .arg(mai->m_fileLine)
                    .arg(mai->m_name ? mai->m_name : "");
        }
        break;
        case Driller::Memory::MET_UNREGISTER_ALLOCATION:
        {
            MemoryDrillerUnregisterAllocationEvent* uae = static_cast<MemoryDrillerUnregisterAllocationEvent*>(dep);
            if (uae->m_removedAllocationInfo)
            {
                finalText = tr("<U><B>DEALLOCATE %1</B><U><BR>%2:%3<BR>%4")
                        .arg(m_ptrFormatter->formatMemorySize((float)uae->m_removedAllocationInfo->m_size, (float)uae->m_removedAllocationInfo->m_size))
                        .arg(uae->m_removedAllocationInfo->m_fileName ? uae->m_removedAllocationInfo->m_fileName : "")
                        .arg(uae->m_removedAllocationInfo->m_fileLine)
                        .arg(uae->m_removedAllocationInfo->m_name ? uae->m_removedAllocationInfo->m_name : "");
            }
            else
            {
                finalText = tr("<U><B>DEALLOCATE UNKNOWN </B></U>");
            }
        }
        break;
        case Driller::Memory::MET_RESIZE_ALLOCATION:
        {
            MemoryDrillerResizeAllocationEvent* rae = static_cast<MemoryDrillerResizeAllocationEvent*>(dep);
            if (rae->m_modifiedAllocationInfo)
            {
                finalText = tr("<U><B>RESIZE %1 TO %2</B><U><BR>%3:%4<BR>%5")
                        .arg(m_ptrFormatter->formatMemorySize((float)rae->m_oldSize, (float)rae->m_oldSize))
                        .arg(m_ptrFormatter->formatMemorySize((float)rae->m_newSize, (float)rae->m_newSize))
                        .arg(rae->m_modifiedAllocationInfo->m_fileName ? rae->m_modifiedAllocationInfo->m_fileName : "")
                        .arg(rae->m_modifiedAllocationInfo->m_fileLine)
                        .arg(rae->m_modifiedAllocationInfo->m_name ? rae->m_modifiedAllocationInfo->m_name : "");
            }
            else
            {
                finalText = tr("<U><B>RESIZE UNKNOWN %1 TO %2</B></U>")
                        .arg(m_ptrFormatter->formatMemorySize((float)rae->m_oldSize, (float)rae->m_oldSize))
                        .arg(m_ptrFormatter->formatMemorySize((float)rae->m_newSize, (float)rae->m_newSize));
            }
        }
        break;
        }

        if (finalText.length() > 0)
        {
            if (QApplication::activeWindow() == this)
            {
                QToolTip::showText(m_gui->widgetDataStrip->mapToGlobal(QPoint(0, -10)), finalText, m_gui->widgetDataStrip);
            }
        }
    }

    void MemoryDataView::onMouseOverNothing(float primaryAxisValue, float dependentAxisValue)
    {
        (void)primaryAxisValue;
        (void)dependentAxisValue;
        m_gui->widgetDataStrip->SetChannelSampleHighlight(0, 0, false);
        m_gui->widgetDataStrip->SetChannelSampleHighlight(1, 0, false);
        QToolTip::hideText();
    }

    void MemoryDataView::onMouseLeftDownDomainValue(float domainValue)
    {
        AZ::s64 globalEvtID = (AZ::s64)(domainValue);
        emit EventRequestEventFocus(globalEvtID);
    }

    void MemoryDataView::onMouseLeftDragDomainValue(float domainValue)
    {
        AZ::s64 globalEvtID = (AZ::s64)(domainValue);
        emit EventRequestEventFocus(globalEvtID);
    }


    void MemoryDataView::FrameChanged(FrameNumberType frame)
    {
        m_Frame = frame;
        m_aggregator->FrameChanged(frame);
        SetFrameNumber();
        OnViewFull();
    }

    void MemoryDataView::SetFrameNumber()
    {
        if (m_persistentState->m_filterId)
        {
            auto found = m_aggregator->FindAllocatorById(m_persistentState->m_filterId);
            if (found == m_aggregator->GetAllocatorEnd())
            {
                m_gui->progressBar->hide();
            }
            else
            {
                Memory::AllocatorInfo* pinfo = (*found);
                if (pinfo->m_capacity)
                {
                    m_gui->progressBar->show();
                    int newValue = (int)(((double)pinfo->m_allocatedMemory / (double)pinfo->m_capacity) * 100.0);
                    if (m_gui->progressBar->value() != newValue)
                    {
                        m_gui->progressBar->setValue(newValue);
                        m_gui->progressBar->setFormat(QString("%1 / %2")
                                .arg(MemoryAxisFormatter::formatMemorySize((float)pinfo->m_allocatedMemory, (float)pinfo->m_allocatedMemory * 0.1f))
                                .arg(MemoryAxisFormatter::formatMemorySize((float)pinfo->m_capacity, (float)pinfo->m_capacity * 0.1f)));
                    }
                }
                else
                {
                    m_gui->progressBar->hide();
                }
            }
        }
        else
        {
            m_gui->progressBar->hide();
        }

        UpdateChart();
    }

    void MemoryDataView::UpdateChart()
    {
        m_gui->widgetDataStrip->Reset();

        m_gui->widgetDataStrip->AddAxis("Event", 0.0f, 1.0f, false, false);
        m_gui->widgetDataStrip->AddAxis("Size", m_persistentState->m_manualZoomMin, m_persistentState->m_manualZoomMax, true, true);

        m_gui->widgetDataStrip->AddChannel("Total Change");
        m_gui->widgetDataStrip->SetChannelColor(0, QColor(255, 64, 255, 255));
        m_gui->widgetDataStrip->SetChannelStyle(0, StripChart::Channel::STYLE_CONNECTED_LINE);

        m_gui->widgetDataStrip->AddChannel("Delta");
        m_gui->widgetDataStrip->SetChannelColor(1, QColor(255, 255, 0, 255));
        m_gui->widgetDataStrip->SetChannelStyle(1, StripChart::Channel::STYLE_PLUSMINUS);

        if (m_aggregator->IsValid())
        {
            float accumulator = 0.0f;

            int frameOffset = m_persistentState->m_frameRange - 1;
            if ((m_Frame - frameOffset) < 0)
            {
                frameOffset = 0;
            }

            for (EventNumberType index = m_aggregator->m_frameToEventIndex[m_Frame - frameOffset]; index < static_cast<EventNumberType>(m_aggregator->m_frameToEventIndex[m_Frame] + m_aggregator->NumOfEventsAtFrame(m_Frame)); ++index)
            {
                DrillerEvent* dep = m_aggregator->GetEvents()[ index ];
                unsigned int gevtID = dep->GetGlobalEventId();

                switch (dep->GetEventType())
                {
                case Driller::Memory::MET_REGISTER_ALLOCATION:
                {
                    auto mai = static_cast<MemoryDrillerRegisterAllocationEvent*>(dep);
                    if (m_persistentState->m_filterId && m_persistentState->m_filterId != mai->m_modifiedAllocatorInfo->m_id)
                    {
                        continue;     // outer for() loop
                    }
                    accumulator += mai->m_allocationInfo.m_size;
                    m_gui->widgetDataStrip->AddData(1, (AZ::u64)index, (float)(gevtID), (float)mai->m_allocationInfo.m_size);
                }
                break;
                case Driller::Memory::MET_UNREGISTER_ALLOCATION:
                {
                    auto uae = static_cast<MemoryDrillerUnregisterAllocationEvent*>(dep);
                    if (m_persistentState->m_filterId && m_persistentState->m_filterId != uae->m_modifiedAllocatorInfo->m_id)
                    {
                        continue;     // outer for() loop
                    }
                    float uaeValue = (float)(uae->m_removedAllocationInfo != NULL ? uae->m_removedAllocationInfo->m_size : 0.0f);
                    accumulator -= uaeValue;
                    m_gui->widgetDataStrip->AddData(1, (AZ::u64)index, (float)(gevtID), -uaeValue);
                }
                break;
                case Driller::Memory::MET_RESIZE_ALLOCATION:
                {
                    auto rae = static_cast<MemoryDrillerResizeAllocationEvent*>(dep);
                    if (rae->m_modifiedAllocationInfo)
                    {
                        auto testIter = m_aggregator->FindAllocatorByRecordsId(rae->m_modifiedAllocationInfo->m_recordsId);
                        if (testIter != m_aggregator->m_allocators.end())
                        {
                            if (m_persistentState->m_filterId && m_persistentState->m_filterId != (*testIter)->m_id)
                            {
                                continue;     // outer for() loop
                            }
                        }
                    }
                    accumulator += rae->m_newSize - rae->m_oldSize;
                    m_gui->widgetDataStrip->AddData(1, (AZ::u64)index, (float)(gevtID), (float)rae->m_newSize - (float)rae->m_oldSize);
                }
                break;
                }

                m_gui->widgetDataStrip->AddData(0, (AZ::u64)index, (float)(gevtID), accumulator);
            }

            FrameNumberType hCalculated = m_Frame - m_persistentState->m_frameRange + 1;
            if (hCalculated < 0)
            {
                hCalculated = 0;
            }

            float h1 = (float)(m_aggregator->GetEvents()[ m_aggregator->m_frameToEventIndex[hCalculated] ]->GetGlobalEventId());
            float h2 = (float)(m_aggregator->GetEvents()[ m_aggregator->m_frameToEventIndex[m_Frame] + m_aggregator->NumOfEventsAtFrame(m_Frame) - 1 ]->GetGlobalEventId());
            m_gui->widgetDataStrip->SetWindowRange(Charts::AxisType::Horizontal, h1, h2);

            if (m_persistentState->m_autoZoom)
            {
                m_gui->widgetDataStrip->ZoomExtents(Charts::AxisType::Vertical);
            }
            else
            {
                m_gui->widgetDataStrip->SetWindowRange(Charts::AxisType::Vertical, m_persistentState->m_manualZoomMin, m_persistentState->m_manualZoomMax);
                m_gui->widgetDataStrip->ZoomManual(Charts::AxisType::Vertical, m_persistentState->m_manualZoomMin, m_persistentState->m_manualZoomMax);
            }
        }
    }

    void MemoryDataView::OnViewFull()
    {
        m_gui->widgetDataStrip->SetViewFull();
    }
    void MemoryDataView::OnCheckLockRight(int state)
    {
        m_gui->widgetDataStrip->SetLockRight(state ? true : false);
    }

    //////////////////////////////////////////////////////////////////////////
    // Event Window Messages
    void MemoryDataView::EventFocusChanged(EventNumberType eventIdx)
    {
        m_ScrubberIndex = eventIdx;
        m_gui->widgetDataStrip->SetMarkerPosition((float)m_ScrubberIndex);
    }

    void MemoryDataView::Reflect(AZ::ReflectContext* context)
    {
        MemoryDataViewSavedState::Reflect(context);
    }
}
