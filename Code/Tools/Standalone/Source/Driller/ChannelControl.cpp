/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include "ChannelControl.hxx"
#include <Source/Driller/moc_ChannelControl.cpp>

#include "Annotations/Annotations.hxx"
#include "ChannelDataView.hxx"
#include "ChannelProfilerWidget.hxx"
#include "DrillerAggregator.hxx"
#include "DrillerMainWindowMessages.h"
#include "Source/Driller/ChannelConfigurationDialog.hxx"
#include "Source/Driller/ChannelConfigurationWidget.hxx"
#include "Source/Driller/CollapsiblePanel.hxx"
#include "Source/Driller/CustomizeCSVExportWidget.hxx"
#include "Source/Driller/DrillerOperationTelemetryEvent.h"

namespace Driller
{
    ChannelControl::ChannelControl(const char* channelName, AnnotationsProvider* ptrAnnotations, QWidget* parent, Qt::WindowFlags flags)
        : QWidget(parent, flags)
        , m_isSetup(false)
        , m_captureMode(CaptureMode::Unknown)
        , m_channelId(channelName)
        , m_configurationDialog(nullptr)
    {
        setupUi(this);

        m_State.m_EndFrame = -1;
        m_State.m_FramesInView = 10; // magic numbers (this matches the default drop-down menu option)
        m_State.m_ContractedHeight = false;
        m_State.m_ScrubberFrame = 0;
        m_State.m_FrameOffset = 0;
        m_State.m_LoopBegin = 0;
        m_State.m_LoopEnd = 0;

        this->channelName->setText(channelName ? channelName : "Channel Name Here");

        channelDataView->RegisterToChannel(this, ptrAnnotations);
        channelDataView->setAutoFillBackground(true);

        configChannel->setVisible(false);

        connect(channelDataView, SIGNAL(InformOfMouseClick(Qt::MouseButton, FrameNumberType, FrameNumberType, int)), SIGNAL(InformOfMouseClick(Qt::MouseButton, FrameNumberType, FrameNumberType, int)));
        connect(channelDataView, SIGNAL(InformOfMouseMove(FrameNumberType, FrameNumberType, int)), SIGNAL(InformOfMouseMove(FrameNumberType, FrameNumberType, int)));
        connect(channelDataView, SIGNAL(InformOfMouseRelease(Qt::MouseButton, FrameNumberType, FrameNumberType, int)), SIGNAL(InformOfMouseRelease(Qt::MouseButton, FrameNumberType, FrameNumberType, int)));
        connect(channelDataView, SIGNAL(InformOfMouseWheel(FrameNumberType, int, FrameNumberType, int)), SIGNAL(InformOfMouseWheel(FrameNumberType, int, FrameNumberType, int)));
        connect(configChannel, SIGNAL(clicked()), SLOT(OnConfigureChannel()));        

        ConfigureUI();
    }

    ChannelControl::~ChannelControl()
    {
        if (m_configurationDialog)
        {
            m_configurationDialog->close();
        }
        m_configurationDialog = nullptr;

        QList<QWidget*>::iterator iter = m_OpenDrills.begin();
        while (iter != m_OpenDrills.end())
        {
            delete *iter;
            ++iter;
        }
        m_OpenDrills.clear();
    }

    bool ChannelControl::IsInCaptureMode(CaptureMode captureMode) const
    {
        return m_captureMode == captureMode;
    }

    void ChannelControl::SetCaptureMode(CaptureMode captureMode)
    {
        if (captureMode != m_captureMode)
        {
            m_captureMode = captureMode;

            ConfigureUI();

            emit OnCaptureModeChanged(m_captureMode);
        }
    }

    void ChannelControl::OnContractedToggled(bool toggleState)
    {
        (void)toggleState;

        // We're going to want to contract this eventually. So keeping the function
        // to avoid unwiring all of the other methods, while we transition
        /*
        m_Contracted->setChecked(toggleState);

        if (toggleState) // MINIMIZED
        {
            m_State.m_ContractedHeight = true;
            m_Info->hide();
            setFixedHeight(k_contractedSize);
            QIcon eyecon(":/general/expand");
            m_Contracted->setIcon(eyecon);
        }
        else // NORMAL
        {
            m_State.m_ContractedHeight = false;
            m_Info->show();
            this->setFixedHeight(k_expandedSize);
            QIcon eyecon(":/general/contract");
            m_Contracted->setIcon(eyecon);
        }
        emit ExpandedContracted();
        */
    }

    void ChannelControl::OnSuccessfulDrillDown(QWidget* drillerWidget)
    {
        if (drillerWidget)
        {
            connect(drillerWidget, SIGNAL(destroyed(QObject*)), this, SLOT(OnDrillDestroyed(QObject*)));
            m_OpenDrills.push_back(drillerWidget);
        }
    }

    void ChannelControl::OnDrillDestroyed(QObject* drill)
    {
        QWidget* qw = qobject_cast<QWidget*>(drill);
        m_OpenDrills.removeOne(qw);
    }

    void ChannelControl::OnShowCommand()
    {
        //todo: phughes consider auto-hide toggle to control this behavior

        //QList<QPoint>::iterator ptiter = m_OpenDrillsPositions.begin();
        //QList<QWidget*>::iterator iter = m_OpenDrills.begin();
        //while (iter != m_OpenDrills.end())
        //{
        //  (*iter)->show();
        //  (*iter)->move(*ptiter);
        //  ++iter;
        //  ++ptiter;
        //}
    }

    void ChannelControl::OnHideCommand()
    {
        //todo: phughes consider auto-hide toggle to control this behavior

        //m_OpenDrillsPositions.clear();
        //QList<QWidget*>::iterator iter = m_OpenDrills.begin();
        //while (iter != m_OpenDrills.end())
        //{
        //  if (*iter)
        //  {
        //      m_OpenDrillsPositions.push_back((*iter)->pos());
        //      (*iter)->hide();
        //  }
        //  ++iter;
        //}
    }

    bool ChannelControl::IsSetup() const
    {
        return m_isSetup;
    }

    void ChannelControl::SignalSetup()
    {
        m_isSetup = true;

        ConfigureUI();
    }

    bool ChannelControl::IsActive() const
    {
        bool isActive = false;

        for (ChannelProfilerWidget* profiler : m_profilerWidgets)
        {
            if (profiler->IsActive())
            {
                isActive = true;
                break;
            }
        }

        return isActive;
    }

    const AZStd::list< ChannelProfilerWidget* >&  ChannelControl::GetProfilers() const
    {
        return m_profilerWidgets;
    }

    ChannelProfilerWidget* ChannelControl::GetMainProfiler() const
    {
        ChannelProfilerWidget* retVal = nullptr;

        for (ChannelProfilerWidget* profiler : m_profilerWidgets)
        {
            if (profiler->IsActive())
            {
                retVal = profiler;
                break;
            }
        }

        return retVal;
    }

    ChannelProfilerWidget* ChannelControl::AddAggregator(Aggregator* aggregator)
    {
        ChannelProfilerWidget* retVal = nullptr;

        for (ChannelProfilerWidget* profiler : m_profilerWidgets)
        {
            if (profiler->GetID() == aggregator->GetID())
            {
                AZ_Warning("ChannelControl", false, "Trying to register two aggregators with the same ID");
                retVal = profiler;
                break;
            }
        }

        if (retVal == nullptr)
        {
            retVal = aznew ChannelProfilerWidget(this, aggregator);

            if (retVal != nullptr)
            {
                ConnectProfilerWidget(retVal);

                profilerLayout->addWidget(retVal);
                m_profilerWidgets.push_back(retVal);

                aggregator->AnnotateChannelView(channelDataView);

                if (aggregator->HasConfigurations())
                {
                    configChannel->setVisible(true);
                }

                connect(aggregator, SIGNAL(NormalizedRangeChanged()), SLOT(OnNormalizedRangeChanged()));
            }
        }

        return retVal;
    }

    bool ChannelControl::RemoveAggregator(Aggregator* aggregator)
    {
        bool removed = false;

        for (auto profilerIter = m_profilerWidgets.begin();
             profilerIter != m_profilerWidgets.end();
             ++profilerIter)
        {
            if ((*profilerIter)->GetID() == aggregator->GetID())
            {
                removed = true;
                m_profilerWidgets.erase(profilerIter);
                break;
            }
        }

        AZ_Warning("ChannelControl", removed, "Trying to remove aggregator from the wrong Channel Control");
        return removed;
    }

    void ChannelControl::SetAllProfilersEnabled(bool enabled)
    {
        for (ChannelProfilerWidget* profiler : m_profilerWidgets)
        {
            profiler->SetIsActive(enabled);
        }
    }

    AZ::Crc32 ChannelControl::GetChannelId() const
    {
        return m_channelId;
    }

    void ChannelControl::SetEndFrame(FrameNumberType endFrame)
    {
        channelDataView->DirtyGraphData();
        m_State.m_EndFrame = endFrame;
        channelDataView->update();
    }

    void ChannelControl::SetSliderOffset(FrameNumberType frameOffset)
    {
        channelDataView->DirtyGraphData();
        m_State.m_FrameOffset = frameOffset;
        //AZ_TracePrintf("Driller"," SetSliderOffset %d = %d - %d\n", m_State.m_FrameOffset, m_State.m_EndFrame, frame);
        channelDataView->update();
    }

    void ChannelControl::SetLoopBegin(FrameNumberType frameNum)
    {
        if (m_State.m_LoopBegin != frameNum)
        {
            m_State.m_LoopBegin = frameNum;

            FrameNumberType lastFrame = m_State.m_FrameOffset + m_State.m_FramesInView - 1;

            if (frameNum < m_State.m_FrameOffset)
            {
                emit RequestScrollToFrame(frameNum);
            }
            else if (frameNum > lastFrame)
            {
                emit RequestScrollToFrame(m_State.m_FrameOffset + (frameNum - lastFrame));
            }

            channelDataView->update();
        }
    }
    void ChannelControl::SetLoopEnd(FrameNumberType frameNum)
    {
        if (m_State.m_LoopEnd != frameNum)
        {
            m_State.m_LoopEnd = frameNum;

            FrameNumberType lastFrame = m_State.m_FrameOffset + m_State.m_FramesInView - 1;

            if (frameNum < m_State.m_FrameOffset)
            {
                emit RequestScrollToFrame(frameNum);
            }
            else if (frameNum > lastFrame)
            {
                emit RequestScrollToFrame(m_State.m_FrameOffset + (frameNum - lastFrame));
            }

            channelDataView->update();
        }
    }

    void ChannelControl::SetScrubberFrame(FrameNumberType frameNum)
    {
        if (m_State.m_ScrubberFrame != frameNum)
        {
            m_State.m_ScrubberFrame = frameNum;

            FrameNumberType lastFrame = m_State.m_FrameOffset + m_State.m_FramesInView - 1;

            if (frameNum < m_State.m_FrameOffset)
            {
                emit RequestScrollToFrame(frameNum);
            }
            else if (frameNum > lastFrame)
            {
                emit RequestScrollToFrame(m_State.m_FrameOffset + (frameNum - lastFrame));
            }

            channelDataView->update();
        }
    }

    void ChannelControl::SetDataPointsInView(FrameNumberType count)
    {
        m_State.m_FramesInView = count;
        channelDataView->DirtyGraphData();
        channelDataView->update();
    }

    void ChannelControl::OnRefreshView()
    {
        channelDataView->update();
    }

    void ChannelControl::OnActivationChanged(ChannelProfilerWidget* profilerWidget, bool activated)
    {
        if (activated)
        {
            profilerWidget->GetAggregator()->AnnotateChannelView(channelDataView);
        }
        else
        {
            profilerWidget->GetAggregator()->RemoveChannelAnnotation(channelDataView);
        }

        channelDataView->DirtyGraphData();
        channelDataView->update();
    }

    void ChannelControl::OnConfigureChannel()
    {
        if (m_configurationDialog == nullptr)
        {
            m_configurationDialog = aznew ChannelConfigurationDialog();
            QVBoxLayout* layout = new QVBoxLayout();
            
            QMargins contentMargins(3,5,3,5);
            layout->setContentsMargins(contentMargins);
            layout->setSpacing(5);

            for (ChannelProfilerWidget* profilerWidget : m_profilerWidgets)
            {
                if (profilerWidget->IsActive())
                {
                    ChannelConfigurationWidget* configurationWidget = profilerWidget->CreateConfigurationWidget();

                    if (configurationWidget)
                    {
                        connect(configurationWidget, SIGNAL(ConfigurationChanged()), SLOT(OnConfigurationChanged()));
                        layout->addWidget(configurationWidget);
                    }
                }
            }

            m_configurationDialog->setLayout(layout);
            m_configurationDialog->show();
            m_configurationDialog->setFocus();

            connect(m_configurationDialog, SIGNAL(DialogClosed(QDialog*)), SLOT(OnDialogClosed(QDialog*)));

            m_configurationDialog->setWindowTitle(QString("%1's Channel Configurations.").arg(channelName->text()));
        }

        if (m_configurationDialog->isMinimized())
        {
            m_configurationDialog->showNormal();
        }

        m_configurationDialog->raise();
        m_configurationDialog->activateWindow();
    }

    void ChannelControl::OnDialogClosed(QDialog* dialog)
    {
        if (m_configurationDialog == dialog)
        {
            m_configurationDialog = nullptr;
        }
    }

    void ChannelControl::OnConfigurationChanged()
    {
        if (IsInCaptureMode(CaptureMode::Inspecting))
        {
            for (ChannelProfilerWidget* profilerWidget : m_profilerWidgets)
            {
                profilerWidget->GetAggregator()->OnConfigurationChanged();
            }

            // Force the channel data view to regrab all of the aggregator data.
            channelDataView->RefreshGraphData();
        }
    }

    void ChannelControl::OnNormalizedRangeChanged()
    {
        // Only want to allow this while we are inspecting.
        // Otherwise just too much noise with the incoming data.
        if (IsInCaptureMode(CaptureMode::Inspecting))
        {
            channelDataView->RefreshGraphData();
        }
    }

    void ChannelControl::ConnectProfilerWidget(ChannelProfilerWidget* profilerWidget)
    {
        connect(this, SIGNAL(OnCaptureModeChanged(CaptureMode)), profilerWidget, SLOT(SetCaptureMode(CaptureMode)));

        connect(profilerWidget, SIGNAL(OnActivationChanged(ChannelProfilerWidget*, bool)), this, SLOT(OnActivationChanged(ChannelProfilerWidget*, bool)));

        profilerWidget->SetCaptureMode(m_captureMode);
    }

    void ChannelControl::ConfigureUI()
    {
        if (IsSetup())
        {
            configChannel->setEnabled(!IsInCaptureMode(CaptureMode::Capturing));
        }
    }
}
