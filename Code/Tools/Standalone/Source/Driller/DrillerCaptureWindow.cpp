/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include "DrillerCaptureWindow.hxx"
#include <Source/Driller/moc_DrillerCaptureWindow.cpp>

#include "DrillerMainWindowMessages.h"
#include "DrillerAggregator.hxx"
#include "ChannelControl.hxx"
#include "ChannelProfilerWidget.hxx"
#include "CombinedEventsControl.hxx"
#include "DrillerDataContainer.h"
#include "DrillerMainWindow.hxx"
#include "DrillerOperationTelemetryEvent.h"
#include "Workspaces/Workspace.h"

#include <AzToolsFramework/UI/LegacyFramework/UIFrameworkAPI.h>
#include <AzToolsFramework/UI/UICore/ProgressShield.hxx>

#include <AzCore/Debug/Trace.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/delegate/delegate.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include "QtGui/QPalette"
#include "Annotations/AnnotationHeaderView.hxx"
#include "Annotations/ConfigureAnnotationsWindow.hxx"

#include <AzCore/std/sort.h>

#include <Source/Driller/ui_DrillerCaptureWindow.h>

#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QSignalMapper>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QTimer>
#include <QToolTip>

void initSharedResources()
{
    Q_INIT_RESOURCE(sharedResources);
}

namespace
{
    const char* drillerDebugName = "Driller";
    const char* drillerInfoName = "Driller";
    const char* baseTempFileName = "drillercapture.drl";
}

namespace Driller
{
    class DrillerCaptureWindowSavedState
        : public AzToolsFramework::MainWindowSavedState
    {
    public:
        AZ_RTTI(DrillerCaptureWindowSavedState, "{19721873-2FB0-4B5B-BCFC-C774FEC7687A}", AzToolsFramework::MainWindowSavedState);
        AZ_CLASS_ALLOCATOR(DrillerCaptureWindowSavedState, AZ::SystemAllocator, 0);

        AZStd::list<AZ::Uuid> m_ChannelIDs;
        int m_fpsValue;
        FrameNumberType m_scrubberCurrentFrame;
        EventNumberType m_scrubberCurrentEvent;
        FrameNumberType m_playbackLoopBegin;
        FrameNumberType m_playbackLoopEnd;
        AZStd::string m_priorSaveFolder;

        DrillerCaptureWindowSavedState()
            : m_fpsValue(60)
            , m_scrubberCurrentFrame(0)
            , m_scrubberCurrentEvent(0)
            , m_playbackLoopBegin(0)
            , m_playbackLoopEnd(0)
        {}

        void Init(const QByteArray& windowState, const QByteArray& windowGeom)
        {
            AzToolsFramework::MainWindowSavedState::Init(windowState, windowGeom);
        }

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<DrillerCaptureWindowSavedState, AzToolsFramework::MainWindowSavedState>()
                    ->Field("m_ChannelIDs", &DrillerCaptureWindowSavedState::m_ChannelIDs)
                    ->Field("m_fpsValue", &DrillerCaptureWindowSavedState::m_fpsValue)
                    ->Field("m_scrubberCurrentFrame", &DrillerCaptureWindowSavedState::m_scrubberCurrentFrame)
                    ->Field("m_scrubberCurrentEvent", &DrillerCaptureWindowSavedState::m_scrubberCurrentEvent)
                    ->Field("m_playbackLoopBegin", &DrillerCaptureWindowSavedState::m_playbackLoopBegin)
                    ->Field("m_playbackLoopEnd", &DrillerCaptureWindowSavedState::m_playbackLoopEnd)
                    ->Field("m_priorSaveFolder", &DrillerCaptureWindowSavedState::m_priorSaveFolder)
                    ->Version(8);
            }
        }
    };

    // WORKSPACES are files loaded and stored independent of the global application
    // designed to be used for DRL data specific view settings and to pass around
    class DrillerCaptureWindowWorkspace
        : public AZ::UserSettings
    {
    public:
        AZ_RTTI(DrillerCaptureWindowWorkspace, "{EB67D4B6-41F5-4CED-85F1-E98586036BC6}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(DrillerCaptureWindowWorkspace, AZ::SystemAllocator, 0);

        DrillerCaptureWindowWorkspace() {}

        AZStd::list<AZ::Uuid> m_ChannelIDs;
        AZStd::string m_matchingDataFileName;
        FrameNumberType m_scrubberCurrentFrame;
        FrameNumberType m_frameRangeBegin;
        FrameNumberType m_frameRangeEnd;
        FrameNumberType m_visibleFrames;
        int m_sliderPosition;
        EventNumberType m_scrubberCurrentEvent;

        FrameNumberType m_playbackLoopBegin;
        FrameNumberType m_playbackLoopEnd;

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<DrillerCaptureWindowWorkspace>()
                    ->Field("m_ChannelIDs", &DrillerCaptureWindowWorkspace::m_ChannelIDs)
                    ->Field("m_matchingDataFileName", &DrillerCaptureWindowWorkspace::m_matchingDataFileName)
                    ->Field("m_scrubberCurrentFrame", &DrillerCaptureWindowWorkspace::m_scrubberCurrentFrame)
                    ->Field("m_frameRangeBegin", &DrillerCaptureWindowWorkspace::m_frameRangeBegin)
                    ->Field("m_frameRangeEnd", &DrillerCaptureWindowWorkspace::m_frameRangeEnd)
                    ->Field("m_visibleFrames", &DrillerCaptureWindowWorkspace::m_visibleFrames)
                    ->Field("m_playbackLoopBegin", &DrillerCaptureWindowWorkspace::m_playbackLoopBegin)
                    ->Field("m_playbackLoopEnd", &DrillerCaptureWindowWorkspace::m_playbackLoopEnd)
                    ->Field("m_sliderPosition", &DrillerCaptureWindowWorkspace::m_sliderPosition)
                    ->Field("m_scrubberCurrentEvent", &DrillerCaptureWindowWorkspace::m_scrubberCurrentEvent)
                    ->Version(6);
            }
        }
    };
}

namespace Driller
{
    extern AZ::Uuid ContextID;

    const int DrillerCaptureWindow::s_availableFrameQuantities[] = { 30, 60, 120, 240, 480, 960, 0 };

    //////////////////////////////////////////////////////////////////////////
    //DrillerCaptureWindow
    DrillerCaptureWindow::DrillerCaptureWindow(CaptureMode captureMode, int identity, QWidget* parent, Qt::WindowFlags flags)
        : QDockWidget(parent, flags)
        , m_captureMode(captureMode)
        , m_identity(identity)
        , m_windowStateCRC(0)
    {
        initSharedResources();

        AZStd::string windowStateStr = AZStd::string::format("DRILLER CAPTURE WINDOW STATE %i", m_identity);
        m_windowStateCRC = AZ::Crc32(windowStateStr.c_str());

        m_captureIsDirty = false;

        m_playbackIsActive = false;
        m_draggingPlaybackLoopBegin = false;
        m_draggingPlaybackLoopEnd = false;
        m_draggingAnything = false;
        m_manipulatingScrollBar = false;
        m_frameRangeEnd = 0;
        m_frameRangeBegin = 0;
        m_ptrConfigureAnnotationsWindow = NULL;
        m_isLoadingFile = false;
        m_TargetConnected = false;
        m_captureIsDirty = false;
        m_bForceNextScrub = true;
        m_captureId = 0;

        m_gui = azcreate(Ui::DrillerCaptureWindow, ());
        m_gui->setupUi(this);

        if (IsInCaptureMode(CaptureMode::Inspecting))
        {
            setFeatures(QDockWidget::DockWidgetClosable);
        }

        m_gui->combinedEventsWidget->SetIdentity(m_identity);

        // Removing the title bar, since it's meaningless for us.
        setTitleBarWidget(new QWidget());

        connect(m_gui->playButton, SIGNAL(toggled(bool)), this, SLOT(OnPlayToggled(bool)));

        if (IsInLiveMode())
        {
            connect(m_gui->captureButton, SIGNAL(toggled(bool)), this, SLOT(OnCaptureToggled(bool)));
        }
        else
        {
            m_gui->captureButton->setDisabled(true);
        }

        connect(m_gui->frameScrubberBox, SIGNAL(valueChanged(int)), this, SLOT(OnFrameScrubberboxChanged(int)));
        connect(m_gui->controlScrollBar, SIGNAL(sliderPressed()), this, SLOT(OnSliderPressed()));
        connect(m_gui->controlScrollBar, SIGNAL(sliderMoved(int)), this, SLOT(OnNewSliderValue(int)));
        connect(m_gui->controlScrollBar, SIGNAL(valueChanged(int)), this, SLOT(OnNewSliderValue(int)));

        QMenu* quantMenu = new QMenu(this);

        QSignalMapper* ptrMapper = new QSignalMapper(this);
        int numQuantsAvailable = sizeof(s_availableFrameQuantities) / sizeof(int);
        for (int idx = 0; idx < numQuantsAvailable; ++idx)
        {
            int thisQuant = s_availableFrameQuantities[idx];

            QString label = thisQuant ? tr("%1 frames").arg(s_availableFrameQuantities[idx]) :  tr("All frames");

            // connect to mapper:
            QAction* act = new QAction(label, this);
            connect(act, SIGNAL(triggered()), ptrMapper, SLOT(map()));
            ptrMapper->setMapping(act, thisQuant);
            quantMenu->addAction(act);
        }
        connect(ptrMapper, SIGNAL(mapped(int)), this, SLOT(OnQuantMenuFinal(int)));

        m_gui->quantityButton->setText("120 frames");
        m_gui->quantityButton->setMenu(quantMenu);

        m_gui->scrollArea->setBackgroundRole(QPalette::Dark);
        DrillerNetworkMessages::Handler::BusConnect(m_identity);

        m_visibleFrames = 120;

        QString tmpCapturePath = PrepTempFile(baseTempFileName);
        m_data = aznew DrillerDataContainer(m_identity, tmpCapturePath.toUtf8().data());

        connect(this, SIGNAL(ScrubberFrameUpdate(FrameNumberType)), m_gui->combinedEventsWidget, SLOT(SetScrubberFrame(FrameNumberType)));

        m_ptrAnnotationsHeaderView = aznew AnnotationHeaderView(&m_AnnotationProvider, this);
        m_gui->channelLayout->addWidget(m_ptrAnnotationsHeaderView);

        connect(m_ptrAnnotationsHeaderView, SIGNAL(OnOptionsClick()), this, SLOT(OnAnnotationOptionsClick()));
        connect(m_ptrAnnotationsHeaderView, SIGNAL(InformOfMouseOverAnnotation(const Annotation&)), this, SLOT(InformOfMouseOverAnnotation(const Annotation&)));
        connect(m_ptrAnnotationsHeaderView, SIGNAL(InformOfClickAnnotation(const Annotation&)), this, SLOT(InformOfClickAnnotation(const Annotation&)));
        connect(&m_AnnotationProvider, SIGNAL(SelectedAnnotationsChanged()), this, SLOT(OnSelectedAnnotationChannelsChanged()));

        // button state maintenance courtesy of the TargetManagerClient bus message(s) we handle
        m_gui->captureButton->setEnabled(false);

        if (IsInLiveMode())
        {
            AzFramework::TargetManagerClient::Bus::Handler::BusConnect();
        }

        StateReset();

        m_gui->combinedEventsWidget->SetAnnotationsProvider(&m_AnnotationProvider);

        connect(m_gui->combinedEventsWidget->m_annotationHeaderView, SIGNAL(InformOfMouseOverAnnotation(const Annotation&)), this, SLOT(InformOfMouseOverAnnotation(const Annotation&)));
        connect(m_gui->combinedEventsWidget->m_annotationHeaderView, SIGNAL(InformOfClickAnnotation(const Annotation&)), this, SLOT(InformOfClickAnnotation(const Annotation&)));
        connect(this, SIGNAL(ScrubberFrameUpdate(FrameNumberType)), m_gui->combinedEventsWidget->m_annotationHeaderView, SLOT(OnScrubberFrameUpdate(FrameNumberType)));
        connect(m_gui->actionClose, SIGNAL(triggered()), this, SLOT(OnCloseFile()));

        connect(m_gui->combinedEventsWidget, SIGNAL(EventRequestEventFocus(EventNumberType)), this, SLOT(EventRequestEventFocus(EventNumberType)));

        UpdateLiveControls();
        emit CaptureWindowSetToLive(IsInLiveMode());

        RestoreWindowState();
        QTimer::singleShot(0, this, SLOT(OnUpdateScrollSize()));

        DrillerCaptureWindowRequestBus::Handler::BusConnect(m_identity);
    }

    DrillerCaptureWindow::~DrillerCaptureWindow(void)
    {
        DrillerCaptureWindowRequestBus::Handler::BusDisconnect(m_identity);
        DrillerNetworkMessages::Handler::BusDisconnect(m_identity);
        AzFramework::TargetManagerClient::Bus::Handler::BusDisconnect();

        delete m_data;
        azdestroy(m_gui);
    }

    bool DrillerCaptureWindow::event(QEvent* evt)
    {
        if (evt->type() == QEvent::WindowActivate)
        {
            emit CaptureWindowSetToLive(static_cast<bool>(IsInLiveMode()));
        }

        // parent class
        return QDockWidget::event(evt);
    }

    //////////////////////////////////////////////////////////////////////////
    // internal workings
    void DrillerCaptureWindow::StateReset()
    {
        if (m_visibleFrames == m_frameRangeEnd - m_frameRangeBegin + 1)
        {
            // full range was visible.
            OnQuantMenuFinal(120);
        }

        OnPlayToggled(false);
        SetPlaybackLoopBegin(0);
        SetPlaybackLoopEnd(0);
        SetFrameRangeBegin(0);
        SetFrameRangeEnd(0);
        SetScrubberFrame(0);
        m_bForceNextScrub = true;

        for (auto iter = m_channels.begin(); iter != m_channels.end(); ++iter)
        {
            (*iter)->SetEndFrame(0);
        }

        m_ptrAnnotationsHeaderView->SetEndFrame(0);
    }

    void DrillerCaptureWindow::UpdateLiveControls()
    {
        bool isViewingStoredData = IsInCaptureMode(CaptureMode::Inspecting);

        // these overlapping frames must be both hidden and then one made visible
        // otherwise the containing window is forced wide enough to support both
        // which overrides previous sizes and leaves a ton of dead space behind
        m_gui->targetFrame->setVisible(!isViewingStoredData);

        m_gui->playButton->setVisible(isViewingStoredData);
        m_gui->frameFPS->setVisible(isViewingStoredData);
        m_gui->frameScrubberBox->setEnabled(isViewingStoredData);

        // For now, we're not sure if we want to keep this, or expand on it
        // going ahead. So for now we'll just hide it.
        m_gui->combinedEventsWidget->setVisible(false);

        if (IsInCaptureMode(CaptureMode::Configuration))
        {
            m_gui->frame->setVisible(false);
            m_gui->quantityButton->setVisible(false);
        }
        else
        {
            m_gui->frame->setVisible(true);
            m_gui->quantityButton->setVisible(true);
        }

        emit CaptureWindowSetToLive(!isViewingStoredData);
    }

    void DrillerCaptureWindow::SetCaptureMode(CaptureMode captureMode)
    {
        if (m_captureMode != captureMode)
        {
            m_captureMode = captureMode;
            emit OnCaptureModeChange(m_captureMode);
        }
    }

    void DrillerCaptureWindow::ResetCaptureControls()
    {
        // Reset the state of the UI Controls
        m_currentDataFilename = "";
        SetCaptureMode(CaptureMode::Configuration);
        OnCloseFile();
        m_data->CloseCaptureData();
        m_data->CreateAggregators();
        UpdateLiveControls();
    }

    bool DrillerCaptureWindow::IsInLiveMode() const
    {
        return IsInCaptureMode(CaptureMode::Capturing) || IsInCaptureMode(CaptureMode::Configuration);
    }

    bool DrillerCaptureWindow::IsInCaptureMode(CaptureMode captureMode) const
    {
        return m_captureMode == captureMode;
    }

    void DrillerCaptureWindow::ClearExistingChannels()
    {
        ClearChannelDisplay(true);
    }

    void DrillerCaptureWindow::ClearChannelDisplay(bool withDeletion)
    {
        if (withDeletion)
        {
            m_gui->combinedEventsWidget->ClearAggregatorList();

            for (auto iter = m_channels.begin(); iter != m_channels.end(); ++iter)
            {
                m_gui->channelLayout->removeWidget(*iter);
                delete *iter;
            }
            m_channels.clear();
        }
        else
        {
            for (auto iter = m_channels.begin(); iter != m_channels.end(); ++iter)
            {
                m_gui->channelLayout->removeWidget(*iter);
            }
        }

        // layouts take one message process to update their sizes, we need to queue a refresh of our scroll area at the end of the event queue
        // so that the sizeHint() will be correct.
        QTimer::singleShot(0, this, SLOT(OnUpdateScrollSize()));
    }

    void DrillerCaptureWindow::SortChannels()
    {
        SortedChannels temp;

        // two passes, first pushes active channels and second pushes the remaining inactive channels
        // this maintains the relative order of the list within categories to avoid surprises
        for (ChannelControl* channelControl : m_channels)
        {
            if (channelControl->IsActive())
            {
                temp.push_back(channelControl);
            }
        }

        for (ChannelControl* channelControl : m_channels)
        {
            if (!channelControl->IsActive())
            {
                channelControl->OnContractedToggled(false);
                temp.push_back(channelControl);
            }
        }

        m_channels.clear();
        m_channels = temp;
        temp.clear();
    }

    void DrillerCaptureWindow::PopulateChannelDisplay()
    {
        for (auto iter = m_channels.begin(); iter != m_channels.end(); ++iter)
        {
            m_gui->channelLayout->addWidget(*iter);
            (*iter)->SetDataPointsInView(m_visibleFrames);
        }
        // layouts take one message process to update their sizes, we need to queue a refresh of our scroll area at the end of the event queue
        // so that the sizeHint() will be correct.
        QTimer::singleShot(0, this, SLOT(OnUpdateScrollSize()));
    }

    void DrillerCaptureWindow::OnUpdateScrollSize()
    {
        // this just tells the scroll area to tell its layout that it has a new sizehint
        m_gui->scrollArea->updateGeometry();
    }

    ChannelControl* DrillerCaptureWindow::FindChannelControl(Aggregator* aggregator)
    {
        ChannelControl* retVal = nullptr;

        QString channelName = aggregator->GetChannelName();
        AZ::Crc32 groupCRC = AZ::Crc32(channelName.toStdString().c_str());

        for (ChannelControl* channelControl : m_channels)
        {
            if (groupCRC == channelControl->GetChannelId())
            {
                retVal = channelControl;
                break;
            }
        }

        if (retVal == nullptr)
        {
            retVal = aznew ChannelControl(channelName.toStdString().c_str(), &m_AnnotationProvider);

            if (retVal)
            {
                m_channels.push_back(retVal);
            }
        }

        return retVal;
    }

    void DrillerCaptureWindow::AddChannelDisplay(ChannelControl* cc)
    {
        m_gui->channelLayout->addWidget(cc);

        // layouts take one message process to update their sizes, we need to queue a refresh of our scroll area at the end of the event queue
        // so that the sizeHint() will be correct.
        QTimer::singleShot(0, this, SLOT(OnUpdateScrollSize()));
    }

    //////////////////////////////////////////////////////////////////////////
    // Driller Network Messages
    void DrillerCaptureWindow::ConnectedToNetwork()
    {
        if (m_isLoadingFile)
        {
            return;
        }

        if (IsInCaptureMode(CaptureMode::Inspecting))
        {
            return;
        }

        StateReset();
    }

    void DrillerCaptureWindow::ConnectChannelControl(ChannelControl* dc)
    {
        if (!dc->IsSetup())
        {
            connect(dc, SIGNAL(GetInspectionFileName()), this, SLOT(GetOpenFileName()));
            connect(dc, SIGNAL(RequestScrollToFrame(FrameNumberType)), this, SLOT(HandleScrollToFrameRequest(FrameNumberType)));
            connect(dc, SIGNAL(InformOfMouseClick(Qt::MouseButton, FrameNumberType, FrameNumberType, int)), this, SLOT(OnChannelControlMouseDown(Qt::MouseButton, FrameNumberType, FrameNumberType, int)));
            connect(dc, SIGNAL(InformOfMouseMove(FrameNumberType, FrameNumberType, int)), this, SLOT(OnChannelControlMouseMove(FrameNumberType, FrameNumberType, int)));
            connect(dc, SIGNAL(InformOfMouseRelease(Qt::MouseButton, FrameNumberType, FrameNumberType, int)), this, SLOT(OnChannelControlMouseUp(Qt::MouseButton, FrameNumberType, FrameNumberType, int)));
            connect(dc, SIGNAL(InformOfMouseWheel(FrameNumberType, int, FrameNumberType, int)), this, SLOT(OnChannelControlMouseWheel(FrameNumberType, int, FrameNumberType, int)));
            connect(dc, SIGNAL(ExpandedContracted()), this, SLOT(OnUpdateScrollSize()));

            connect(this, SIGNAL(ScrubberFrameUpdate(FrameNumberType)), dc, SLOT(SetScrubberFrame(FrameNumberType)));
            connect(this, SIGNAL(ShowYourself()), dc, SLOT(OnShowCommand()));
            connect(this, SIGNAL(HideYourself()), dc, SLOT(OnHideCommand()));
            connect(this, SIGNAL(OnCaptureModeChange(CaptureMode)), dc, SLOT(SetCaptureMode(CaptureMode)));

            dc->SetCaptureMode(m_captureMode);

            dc->SignalSetup();
        }
    }

    // target that you're connected to knows what aggregators are ready.
    void DrillerCaptureWindow::NewAggregatorsAvailable()
    {
        if (m_isLoadingFile)
        {
            return;
        }

        if (IsInCaptureMode(CaptureMode::Inspecting))
        {
            return;
        }

        // otherwise, make em, if we're live.
        m_data->CreateAggregators();
    }

    // incoming driller bus message
    void DrillerCaptureWindow::NewAggregatorList(AggregatorList& theList)
    {
        ClearExistingChannels();

        if (theList.size())
        {
            for (AggregatorList::iterator iter = theList.begin(); iter != theList.end(); ++iter)
            {
                ChannelControl* channelControl = FindChannelControl((*iter));
                ConnectChannelControl(channelControl);

                ChannelProfilerWidget* profilerWidget = channelControl->AddAggregator((*iter));

                if (profilerWidget)
                {
                    // defaults to active
                    // restore previous inactive state if GUIDs match
                    bool wasInactive = (m_inactiveChannels.find(profilerWidget->GetID()) != m_inactiveChannels.end());
                    profilerWidget->SetIsActive(!wasInactive);
                }
            }

            PopulateChannelDisplay();

            m_gui->combinedEventsWidget->AddAggregatorList(theList);
            m_gui->captureButton->setEnabled(true);
        }
    }

    void DrillerCaptureWindow::AddAggregator(Aggregator& theAggregator)
    {
        ChannelControl* channelControl = FindChannelControl(&theAggregator);

        if (!channelControl->IsSetup())
        {
            ConnectChannelControl(channelControl);
            AddChannelDisplay(channelControl);
        }

        ChannelProfilerWidget* profilerWidget = channelControl->AddAggregator(&theAggregator);

        if (profilerWidget)
        {
            // defaults to active
            // restore previous inactive state if GUIDs match
            int wasInactive = (m_inactiveChannels.find(profilerWidget->GetID()) != m_inactiveChannels.end());
            profilerWidget->SetIsActive(!wasInactive);
        }

        m_gui->captureButton->setEnabled(true);
        m_gui->combinedEventsWidget->AddAggregator(theAggregator);
    }

    void DrillerCaptureWindow::DiscardAggregators()
    {
        ClearExistingChannels();
        m_gui->captureButton->setEnabled(false);
    }

    // incoming driller bus message
    void DrillerCaptureWindow::DisconnectedFromNetwork()
    {
        //todo: phughes message the user or something
    }

    void DrillerCaptureWindow::UpdateEndFrameInControls()
    {
        if (!m_isLoadingFile)
        {
            for (auto iter = m_channels.begin(); iter != m_channels.end(); ++iter)
            {
                (*iter)->SetEndFrame(m_frameRangeEnd);
            }
            m_ptrAnnotationsHeaderView->SetEndFrame(m_frameRangeEnd);
        }
    }

    QString DrillerCaptureWindow::GetOpenFileName() const
    {
        AZ_Error("DrillerCaptureWindow", IsInCaptureMode(CaptureMode::Inspecting), "Trying to get file name in non-inspected case");
        return m_currentDataFilename;
    }

    // incoming driller bus message
    void DrillerCaptureWindow::EndFrame(FrameNumberType frame)
    {
        // if we're loading a file then we do not scrub these live:
        SetFrameRangeEnd(frame);
        UpdateEndFrameInControls();
    }

    // incoming driller bus message
    void DrillerCaptureWindow::ScrubberFrame(FrameNumberType frame)
    {
        emit ScrubberFrameUpdate(frame);
        m_gui->frameScrubberBox->setValue(static_cast<int>(frame));
    }

    //////////////////////////////////////////////////////////////////////////
    // Data Viewer Request Messages
    void DrillerCaptureWindow::EventRequestEventFocus(EventNumberType eventIdx)
    {
        m_scrubberCurrentEvent = eventIdx;

        EBUS_EVENT_ID(m_identity, Driller::DrillerEventWindowMessages::Bus, EventFocusChanged, eventIdx);
    }

    void DrillerCaptureWindow::SetScrubberEvent(EventNumberType eventIdx)
    {
        EventRequestEventFocus(eventIdx);
    }

    //////////////////////////////////////////////////////////////////////////
    // GUI Messages
    void DrillerCaptureWindow::OnCaptureToggled(bool toggleState)
    {
        DrillerOperationTelemetryEvent captureEvent;

        if (toggleState)
        {
            captureEvent.SetAttribute("StartDataCapture", "");
            captureEvent.SetMetric("CaptureId", m_captureId);

            QString activeChannels;

            bool appendComma = false;

            for (ChannelControl* channel : m_channels)
            {
                const AZStd::list< ChannelProfilerWidget* >& profilers = channel->GetProfilers();

                for (ChannelProfilerWidget* profiler : profilers)
                {
                    if (profiler->IsActive())
                    {
                        if (appendComma)
                        {
                            activeChannels.append(",");
                        }

                        appendComma = true;
                        activeChannels.append(profiler->GetName());
                    }
                }
            }

            captureEvent.SetAttribute("ActiveChannels", activeChannels.toStdString().c_str());

            AZ_TracePrintf(drillerInfoName, "Capture ON, starting a new data session\n");
            OnPlayToggled(false);
            m_gui->captureButton->setText(tr("Stop Capture"));
            m_gui->captureButton->setToolTip(tr("Stop Capturing Driller Data"));
            StateReset();

            SetCaptureMode(CaptureMode::Capturing);

            m_AnnotationProvider.Clear();

            ClearChannelDisplay(false);
            SortChannels();
            PopulateChannelDisplay();

            if (m_data)
            {
                m_data->StartDrilling();
            }

            SetCaptureDirty(true);
            UpdateLiveControls();
        }
        else
        {
            captureEvent.SetAttribute("StopDataCapture", "");
            captureEvent.SetMetric("CaptureId", m_captureId);

            ++m_captureId;

            AZ_TracePrintf(drillerInfoName, "Capture OFF, freezing data for analysis\n");
            m_gui->captureButton->setText(tr("Capture"));
            m_gui->captureButton->setToolTip(tr("Begin Capturing Driller Data"));

            bool wascapturing = IsInCaptureMode(CaptureMode::Capturing);

            CaptureMode::Inspecting;
            emit OnCaptureModeChange(m_captureMode);

            if (m_data)
            {
                m_data->StopDrilling();
            }

            if (wascapturing)
            {
                OnSaveDrillerFile();
                ScrubberToEnd();
                SetFrameRangeEnd(0);
                // counting on the OnSave to recognize the TMP file from the capture and copy appropriately
                // and setting it to our currently active data file
            }
        }

        captureEvent.Log();

        update();
    }

    void DrillerCaptureWindow::SetCaptureDirty(bool isDirty)
    {
        m_captureIsDirty = isDirty;
    }

    void DrillerCaptureWindow::OnMenuCloseCurrentWindow()
    {
        AZ_TracePrintf(drillerDebugName, "Close requested\n");

        OnCaptureToggled(false);
        OnCloseFile();

        EBUS_EVENT(AzToolsFramework::FrameworkMessages::Bus, RequestMainWindowClose, ContextID);
    }

    void DrillerCaptureWindow::OnOpen()
    {
        AZ_TracePrintf(drillerDebugName, "Open requested\n");

        this->show();
        emit ShowYourself();
    }

    void DrillerCaptureWindow::OnClose()
    {
        OnCloseFile();
    }

    void DrillerCaptureWindow::OnCloseFile()
    {
        SaveWindowState();

        if (IsInCaptureMode(CaptureMode::Inspecting))
        {
            AZ_TracePrintf(drillerDebugName, "Close requested of file\n");
            m_data->CloseCaptureData();
            this->close();
            deleteLater();
        }
    }

    void DrillerCaptureWindow::OnContractAllChannels()
    {
        for (auto iter = m_channels.begin(); iter != m_channels.end(); ++iter)
        {
            (*iter)->OnContractedToggled(true);
        }
    }

    void DrillerCaptureWindow::OnExpandAllChannels()
    {
        for (auto iter = m_channels.begin(); iter != m_channels.end(); ++iter)
        {
            (*iter)->OnContractedToggled(false);
        }
    }

    void DrillerCaptureWindow::OnDisableAllChannels()
    {
        for (ChannelControl* channelControl : m_channels)
        {
            channelControl->SetAllProfilersEnabled(false);
        }
    }
    void DrillerCaptureWindow::OnEnableAllChannels()
    {
        for (ChannelControl* channelControl : m_channels)
        {
            channelControl->SetAllProfilersEnabled(true);
        }
    }

    void DrillerCaptureWindow::OnToBegin()
    {
        ScrubberToBegin();
        m_gui->controlScrollBar->setValue(m_frameRangeBegin);
    }

    void DrillerCaptureWindow::OnToEnd()
    {
        // set the scroll view to scroll to the end:
        ScrubberToEnd();
    }

    void DrillerCaptureWindow::OnPlayToggled(bool toggleState)
    {
        if (toggleState)
        {
            m_gui->playButton->setText(tr("Stop"));
            m_gui->playButton->setToolTip(tr("Stop recorded session playback"));
            m_playbackIsActive = true;
            OnCaptureToggled(false);
            int msec = 1000 / m_gui->fpsBox->value();
            QTimer::singleShot(msec, this, SLOT(PlaybackTick()));
        }
        else
        {
            m_gui->playButton->setText(tr("Play"));
            m_gui->playButton->setToolTip(tr("Playback recorded session"));
            m_gui->playButton->blockSignals(true);
            m_gui->playButton->setChecked(false);
            m_gui->playButton->blockSignals(false);
            m_playbackIsActive = false;
        }
    }

    void DrillerCaptureWindow::PlaybackTick()
    {
        if (m_playbackIsActive)
        {
            if (m_scrubberCurrentFrame >= m_playbackLoopEnd)
            {
                SetScrubberFrame(m_playbackLoopBegin);
            }
            else if (m_scrubberCurrentFrame < m_playbackLoopBegin)
            {
                SetScrubberFrame(m_playbackLoopBegin);
            }
            else
            {
                SetScrubberFrame(m_scrubberCurrentFrame + 1);
            }

            FocusScrollbar(m_scrubberCurrentFrame - (m_visibleFrames / 2));

            int msec = 1000 / m_gui->fpsBox->value();
            QTimer::singleShot(msec, this, SLOT(PlaybackTick()));
        }
    }

    void DrillerCaptureWindow::OnSliderPressed()
    {
        if (m_playbackIsActive)
        {
            OnPlayToggled(false);
        }
    }

    void DrillerCaptureWindow::OnNewSliderValue(int newValue)
    {
        if (!m_manipulatingScrollBar && m_playbackIsActive)
        {
            OnPlayToggled(false);
        }

        for (auto iter = m_channels.begin(); iter != m_channels.end(); ++iter)
        {
            (*iter)->SetSliderOffset(newValue);
        }

        m_ptrAnnotationsHeaderView->SetSliderOffset(newValue);
    }

    void DrillerCaptureWindow::OnFrameScrubberboxChanged(int newValue)
    {
        SetScrubberFrame(newValue);
    }

    void DrillerCaptureWindow::OnQuantMenuFinal(int range)
    {
        FrameNumberType frameRange = static_cast<FrameNumberType>(range);

        if (frameRange <= 1)
        {
            frameRange = m_frameRangeEnd - m_frameRangeBegin + 1;
        }
        m_visibleFrames = frameRange;

        m_gui->quantityButton->setText(QString("%1 frames").arg(frameRange));

        for (auto iter = m_channels.begin(); iter != m_channels.end(); ++iter)
        {
            (*iter)->SetDataPointsInView(frameRange);
        }
        m_ptrAnnotationsHeaderView->SetDataPointsInView(frameRange);
    }

    void DrillerCaptureWindow::FocusScrollbar(FrameNumberType focusFrame)
    {
        m_manipulatingScrollBar = true;

        // range of motion for the scrollbar covers the off-window area, not the total
        FrameNumberType range = m_frameRangeEnd - m_visibleFrames + 1;
        range = range >= m_frameRangeBegin ? range : m_frameRangeBegin;

        m_gui->controlScrollBar->setRange(m_frameRangeBegin, range);

        FrameNumberType curVal = focusFrame < 0 ? 0 : focusFrame;
        curVal = focusFrame > range ? range : focusFrame;
        m_gui->controlScrollBar->setValue(curVal);

        m_manipulatingScrollBar = false;
    }

    //////////////////////////////////////////////////////////////////////////
    // State Control and Maintenance
    void DrillerCaptureWindow::ScrubberToBegin()
    {
        SetScrubberFrame(GetFrameRangeBegin());
    }

    void DrillerCaptureWindow::ScrubberToEnd()
    {
        SetScrubberFrame(GetFrameRangeEnd());
    }

    void DrillerCaptureWindow::HandleScrollToFrameRequest(FrameNumberType frame)
    {
        FocusScrollbar(frame);
    }

    void DrillerCaptureWindow::OnChannelControlMouseDown(Qt::MouseButton whichButton, FrameNumberType frame, FrameNumberType range, int modifiers)
    {
        // If we aren't inspecting data, we don't to mess around with anything.
        if (!IsInCaptureMode(CaptureMode::Inspecting))
        {
            return;
        }

        if (modifiers & Qt::AltModifier)
        {
            if (whichButton == Qt::LeftButton)
            {
                SetPlaybackLoopBegin(frame);
            }
            if (whichButton == Qt::RightButton)
            {
                SetPlaybackLoopEnd(frame);
            }
            return;
        }

        // Don't want to fight the user for control, relinquish our manipulation once they start doing stuff.
        if (m_playbackIsActive)
        {
            OnPlayToggled(false);
        }

        // we grab with the left button, pan with the right.
        if (whichButton == Qt::LeftButton)
        {
            m_draggingAnything = true;

            if (abs(frame - m_playbackLoopBegin) <= range)
            {
                m_draggingPlaybackLoopBegin = true;
                m_draggingPlaybackLoopEnd = false;
            }
            else if (abs(frame - m_playbackLoopEnd) <= range)
            {
                m_draggingPlaybackLoopBegin = false;
                m_draggingPlaybackLoopEnd = true;
            }
            else
            {
                m_draggingPlaybackLoopBegin = false;
                m_draggingPlaybackLoopEnd = false;

                SetScrubberFrame(frame);
            }
        }
    }

    void DrillerCaptureWindow::OnChannelControlMouseMove(FrameNumberType frame, FrameNumberType range, int modifiers)
    {
        (void)range;
        (void)modifiers;

        //ShowDrillerChannelTip(frame);

        if (m_draggingAnything)
        {
            if (m_draggingPlaybackLoopBegin)
            {
                SetPlaybackLoopBegin(frame);
            }
            else if (m_draggingPlaybackLoopEnd)
            {
                SetPlaybackLoopEnd(frame);
            }
            else
            {
                SetScrubberFrame(frame);
            }
        }
    }

    void DrillerCaptureWindow::OnChannelControlMouseUp(Qt::MouseButton whichButton, FrameNumberType frame, FrameNumberType range, int modifiers)
    {
        (void)range;
        (void)modifiers;
        (void)frame;

        if (whichButton == Qt::LeftButton)
        {
            m_draggingAnything = false;
        }
    }

    void DrillerCaptureWindow::OnChannelControlMouseWheel(FrameNumberType frame, int wheelAmount, FrameNumberType range, int modifiers)
    {
        (void)range;
        (void)modifiers;

        FrameNumberType currentVisibleFrames = m_visibleFrames;
        bool zoomingIn = wheelAmount > 0;
        if (zoomingIn)
        {
            // zooming in:
            // find the next step DOWN from where we are and set our quant zoom to that
            // since its from zoomed all the way in to out.   The last element is assumed the special one.

            if (currentVisibleFrames == s_availableFrameQuantities[0])
            {
                return;
            }

            // before we zoom in, where is the given frame within our scroll area?
            int leftSideOfScreen = m_gui->controlScrollBar->value();
            float fraction = (float)(frame - leftSideOfScreen) / (float)m_visibleFrames;

            int numQuantsAvailable = sizeof(s_availableFrameQuantities) / sizeof(int);
            int quantChosen = -1;
            for (int quantIndex = numQuantsAvailable - 2; quantIndex >= 0; --quantIndex)
            {
                if (currentVisibleFrames > s_availableFrameQuantities[quantIndex])
                {
                    quantChosen = s_availableFrameQuantities[quantIndex];
                    break;
                }
            }

            if (quantChosen != -1)
            {
                OnQuantMenuFinal(quantChosen);

                // we want to focus the scrollbar at the same fraction as before
                FocusScrollbar(frame - (int)((float)m_visibleFrames * fraction));
            }
        }
        else
        {
            // zooming out:
            // find the next step DOWN from where we are and set our quant zoom to that
            // since its from zoomed all the way in to out.   The last element is assumed the special one.

            FrameNumberType fullRange = m_frameRangeEnd - m_frameRangeBegin + 1;

            if (currentVisibleFrames == fullRange)
            {
                return;
            }

            int leftSideOfScreen = m_gui->controlScrollBar->value();
            float fraction = (float)(frame - leftSideOfScreen) / (float)m_visibleFrames;

            int numQuantsAvailable = sizeof(s_availableFrameQuantities) / sizeof(int);

            int quantChosen = -1;
            for (int quantIndex = 0; quantIndex < numQuantsAvailable - 1; ++quantIndex)
            {
                if (currentVisibleFrames < s_availableFrameQuantities[quantIndex])
                {
                    quantChosen = s_availableFrameQuantities[quantIndex];
                    break;
                }
            }

            if (quantChosen != -1)
            {
                OnQuantMenuFinal(quantChosen);
                FocusScrollbar(frame - (int)((float)m_visibleFrames * fraction));
            }
        }
    }

    void DrillerCaptureWindow::SetScrubberFrame(FrameNumberType frame)
    {
        if ((!m_bForceNextScrub) && (frame == m_scrubberCurrentFrame))
        {
            return;
        }

        m_bForceNextScrub = false;

        m_scrubberCurrentFrame = frame >= m_frameRangeBegin ? frame : m_frameRangeBegin;
        m_scrubberCurrentFrame = m_scrubberCurrentFrame <= m_frameRangeEnd ? m_scrubberCurrentFrame : m_frameRangeEnd;

        UpdateFrameScrubberbox();

        ScrubberFrame(m_scrubberCurrentFrame);

        EBUS_EVENT_ID(m_identity, Driller::DrillerMainWindowMessages::Bus, FrameChanged, m_scrubberCurrentFrame);
        m_AnnotationProvider.Finalize();
    }

    void DrillerCaptureWindow::SetPlaybackLoopBegin(FrameNumberType frame)
    {
        m_playbackLoopBegin = frame >= m_frameRangeBegin ? frame : m_frameRangeBegin;
        m_playbackLoopBegin = m_playbackLoopBegin <= m_frameRangeEnd ? m_playbackLoopBegin : m_frameRangeEnd;

        m_playbackLoopEnd = m_playbackLoopEnd >= m_playbackLoopBegin ? m_playbackLoopEnd : m_playbackLoopBegin;
        m_playbackLoopEnd = m_playbackLoopEnd <= m_frameRangeEnd ? m_playbackLoopEnd : m_frameRangeEnd;

        EBUS_EVENT_ID(m_identity, Driller::DrillerMainWindowMessages::Bus, PlaybackLoopBeginChanged, m_playbackLoopBegin);

        UpdatePlaybackLoopPoints();
    }

    void DrillerCaptureWindow::SetPlaybackLoopEnd(FrameNumberType frame)
    {
        m_playbackLoopEnd = frame >= m_frameRangeBegin ? frame : m_frameRangeBegin;
        m_playbackLoopEnd = m_playbackLoopEnd <= m_frameRangeEnd ? m_playbackLoopEnd : m_frameRangeEnd;

        m_playbackLoopBegin = m_playbackLoopBegin >= m_playbackLoopEnd ? m_playbackLoopEnd : m_playbackLoopBegin;
        m_playbackLoopBegin = m_playbackLoopBegin >= m_frameRangeBegin ? m_playbackLoopBegin : m_frameRangeBegin;

        EBUS_EVENT_ID(m_identity, Driller::DrillerMainWindowMessages::Bus, PlaybackLoopEndChanged, m_playbackLoopEnd);

        UpdatePlaybackLoopPoints();
    }

    void DrillerCaptureWindow::UpdatePlaybackLoopPoints()
    {
        for (auto iter = m_channels.begin(); iter != m_channels.end(); ++iter)
        {
            (*iter)->SetLoopBegin(m_playbackLoopBegin);
            (*iter)->SetLoopEnd(m_playbackLoopEnd);
        }
    }

    void DrillerCaptureWindow::SetFrameRangeBegin(FrameNumberType frame)
    {
        m_frameRangeBegin = frame;

        SetPlaybackLoopBegin(m_playbackLoopBegin);
        SetPlaybackLoopEnd(m_playbackLoopEnd);
        SetScrubberFrame(m_scrubberCurrentFrame);
        UpdateFrameScrubberbox();
        UpdateScrollbar();
    }

    void DrillerCaptureWindow::SetFrameRangeEnd(FrameNumberType frame)
    {
        // if the scrubber/loop is on the last frame we always advance it to the new end
        bool setScrubberToo = m_scrubberCurrentFrame == m_frameRangeEnd;
        bool setEndloopToo = m_playbackLoopEnd == m_frameRangeEnd;

        FrameNumberType range = m_frameRangeEnd - m_visibleFrames + 1;
        range = range >= m_frameRangeBegin ? range : m_frameRangeBegin;

        FrameNumberType priorFrameRangeEnd = m_frameRangeEnd;

        int diff = static_cast<int>(frame - m_frameRangeEnd);
        m_frameRangeEnd = frame;

        if (priorFrameRangeEnd < m_frameRangeEnd)
        {
            for (ChannelControl* channelControl : m_channels)
            {
                for (ChannelProfilerWidget* channelProfiler : channelControl->GetProfilers())
                {
                    Aggregator* aggregator = channelProfiler->GetAggregator();
                    if (aggregator != nullptr)
                    {
                        aggregator->EmitAnnotationChannelsForFrameRange(priorFrameRangeEnd, m_frameRangeEnd, &m_AnnotationProvider);
                        aggregator->EmitAllAnnotationsForFrameRange(priorFrameRangeEnd, m_frameRangeEnd, &m_AnnotationProvider);
                    }
                }
            }
        }

        if (!m_isLoadingFile)
        {
            SetScrubberFrame(setScrubberToo ? m_frameRangeEnd : m_scrubberCurrentFrame);

            SetPlaybackLoopBegin(m_playbackLoopBegin);
            SetPlaybackLoopEnd(setEndloopToo ? m_frameRangeEnd : m_playbackLoopEnd);
            UpdateFrameScrubberbox();
            UpdateScrollbar(diff);
        }
    }

    void DrillerCaptureWindow::UpdateFrameScrubberbox()
    {
        m_gui->frameScrubberBox->setRange(m_frameRangeBegin, m_frameRangeEnd);
        m_gui->frameScrubberBox->setValue(m_scrubberCurrentFrame);
    }

    void DrillerCaptureWindow::UpdateScrollbar(int diff)
    {
        int curVal = m_gui->controlScrollBar->value();

        // range of motion for the scrollbar covers the off-window area, not the total
        FrameNumberType range = m_frameRangeEnd - m_visibleFrames + 1;
        range = range >= m_frameRangeBegin ? range : m_frameRangeBegin;

        m_gui->controlScrollBar->setRange(m_frameRangeBegin, range);
        if (m_gui->controlScrollBar->value() >= range - 1)
        {
            m_gui->controlScrollBar->setValue(static_cast<int>(range));
        }
        else if (diff)
        {
            m_gui->controlScrollBar->setValue(static_cast<int>(curVal));
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // when the Editor Main window is requested to close, it is not destroyed.
    //////////////////////////////////////////////////////////////////////////
    // Qt Events
    void DrillerCaptureWindow::closeEvent(QCloseEvent* event)
    {
        OnCloseFile();
        event->ignore();
    }

    void DrillerCaptureWindow::showEvent(QShowEvent* /*event*/)
    {
        emit ShowYourself();
    }
    void DrillerCaptureWindow::hideEvent(QHideEvent* /*event*/)
    {
        emit HideYourself();
    }

    bool DrillerCaptureWindow::OnGetPermissionToShutDown()
    {
        OnCaptureToggled(false);

        bool willShutDown = true;

        ClearChannelDisplay(true);

        AZ_TracePrintf(drillerDebugName, "                            willShutDown == %d\n", (int)willShutDown);
        return willShutDown;
    }

    void DrillerCaptureWindow::ScrubToFrameRequest(FrameNumberType frame)
    {
        if (m_playbackIsActive)
        {
            OnPlayToggled(false);
        }

        SetScrubberFrame(frame);
    }

    void DrillerCaptureWindow::SaveWindowState()
    {
        m_inactiveChannels.clear();

        for (auto iter = m_channels.begin(); iter != m_channels.end(); ++iter)
        {
            const AZStd::list< ChannelProfilerWidget*>& profilers = (*iter)->GetProfilers();

            for (ChannelProfilerWidget* profiler : profilers)
            {
                if (!profiler->IsActive())
                {
                    m_inactiveChannels.insert(profiler->GetID());
                }
            }
        }

        // build state and store it.
        auto newState = AZ::UserSettings::CreateFind<DrillerCaptureWindowSavedState>(m_windowStateCRC, AZ::UserSettings::CT_GLOBAL);
        //newState->Init(saveState(), saveGeometry());
        newState->m_ChannelIDs.clear();
        for (AZStd::set<AZ::Uuid>::iterator iter = m_inactiveChannels.begin(); iter != m_inactiveChannels.end(); ++iter)
        {
            newState->m_ChannelIDs.push_back(*iter);
        }

        newState->m_fpsValue = m_gui->fpsBox->value();

        newState->m_scrubberCurrentFrame = m_scrubberCurrentFrame;
        newState->m_playbackLoopBegin = m_playbackLoopBegin;
        newState->m_playbackLoopEnd = m_playbackLoopEnd;
        newState->m_scrubberCurrentEvent = m_scrubberCurrentEvent;
    }

    void DrillerCaptureWindow::RestoreWindowState() // call this after you have rebuilt everything.
    {
        // load the state from our state block:
        auto savedState = AZ::UserSettings::Find<DrillerCaptureWindowSavedState>(m_windowStateCRC, AZ::UserSettings::CT_GLOBAL);
        if (savedState)
        {
            QByteArray geomData((const char*)savedState->m_windowGeometry.data(), (int)savedState->m_windowGeometry.size());
            QByteArray stateData((const char*)savedState->GetWindowState().data(), (int)savedState->GetWindowState().size());

            restoreGeometry(geomData);
            if (this->isMaximized())
            {
                this->showNormal();
                this->showMaximized();
            }
            //restoreState(stateData);

            m_inactiveChannels.clear();
            for (auto iter = savedState->m_ChannelIDs.begin(); iter != savedState->m_ChannelIDs.end(); ++iter)
            {
                m_inactiveChannels.insert(*iter);
            }

            m_gui->fpsBox->setValue(savedState->m_fpsValue);

            SetScrubberFrame(savedState->m_scrubberCurrentFrame);
            SetPlaybackLoopBegin(savedState->m_playbackLoopBegin);
            SetPlaybackLoopEnd(savedState->m_playbackLoopEnd);
            SetScrubberEvent(savedState->m_scrubberCurrentEvent);
        }
        else
        {
            // default state!
        }
    }

    void DrillerCaptureWindow::OnOpenDrillerFile()
    {
        auto paths = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);
        QString capturePath;
        if (paths.isEmpty())
        {
            paths = QStandardPaths::standardLocations(QStandardPaths::TempLocation);
        }
        if (!paths.isEmpty())
        {
            capturePath = paths.first();
        }

        QString fileName = QFileDialog::getOpenFileName(this, "Open Driller File", capturePath, "Driller Files (*.drl)");
        if (!fileName.isNull())
        {
            OnOpenDrillerFile(fileName);
        }
    }

    void DrillerCaptureWindow::OnOpenDrillerFile(QString fileName)
    {
        if (m_data)
        {
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            m_AnnotationProvider.Clear();

            SetCaptureDirty(false);
            m_currentDataFilename = fileName;

            m_isLoadingFile = true;
            m_data->LoadCaptureData(fileName.toUtf8().data());
            m_isLoadingFile = false;

            SetCaptureMode(CaptureMode::Inspecting);

            m_bForceNextScrub = true;
            EndFrame(m_frameRangeEnd);
            SetPlaybackLoopBegin(0);
            SetPlaybackLoopEnd(m_frameRangeEnd);

            OnQuantMenuFinal(m_visibleFrames);

            UpdateLiveControls();
            m_bForceNextScrub = true;

            ScrubberToEnd();
            QApplication::restoreOverrideCursor();
        }
    }

    void DrillerCaptureWindow::OnOpenDrillerFileForWorkspace(QString fileName, QString workspaceFileName)
    {
        if (m_data)
        {
            QString successFileName;

            // does a file local to our given Workspace DRW exist?  It gets preference on load.
            QString localFileName(workspaceFileName.left(workspaceFileName.size() - 3) + "drl");
            if (AZ::IO::SystemFile::Exists(localFileName.toUtf8().data()))
            {
                successFileName = localFileName;
            }
            // does the workspace's suggested file exist?
            else if (AZ::IO::SystemFile::Exists(fileName.toUtf8().data()))
            {
                successFileName = fileName;
            }
            // fall through to prompting the user for a DRL to use
            else
            {
                QString userFileName = QFileDialog::getOpenFileName(this, "Find Driller File", localFileName, "Driller Files (*.drl)");
                if (!userFileName.isNull())
                {
                    successFileName = userFileName;
                }
            }

            if (!successFileName.isEmpty() && !successFileName.isNull())
            {
                SetCaptureDirty(false);
                m_currentDataFilename = successFileName;

                m_isLoadingFile = true;
                m_data->LoadCaptureData(m_currentDataFilename.toUtf8().data());
                m_isLoadingFile = false;

                SetCaptureMode(CaptureMode::Inspecting);

                m_bForceNextScrub = true;
                EndFrame(m_frameRangeEnd);

                OnQuantMenuFinal(m_visibleFrames);
                m_bForceNextScrub = true;
                ScrubberToEnd();
                UpdateLiveControls();
            }
        }
    }

    void DrillerCaptureWindow::RepopulateAnnotations()
    {
        // re-query all the annotations now that you have your settings.
        m_AnnotationProvider.Clear();

        if (m_frameRangeEnd != 0)
        {
            for (ChannelControl* channelControl : m_channels)
            {
                for (ChannelProfilerWidget* profiler : channelControl->GetProfilers())
                {
                    Aggregator* aggregator = profiler->GetAggregator();

                    if (aggregator != nullptr)
                    {
                        aggregator->EmitAnnotationChannelsForFrameRange(0, m_frameRangeEnd, &m_AnnotationProvider);
                        aggregator->EmitAllAnnotationsForFrameRange(0, m_frameRangeEnd, &m_AnnotationProvider);
                    }
                }
            }
        }
        m_AnnotationProvider.Finalize();
    }

    void DrillerCaptureWindow::OnOpenWorkspaceFile(QString workspaceFileName, bool openDrillerFileAlso)
    {
        OnCaptureToggled(false);

        if (m_data)
        {
            m_AnnotationProvider.Clear();
            // 1: spawn a new local settings object using the DRW
            if (!AZ::IO::SystemFile::Exists(workspaceFileName.toUtf8().data()))
            {
                QMessageBox::warning(this, tr("File not found"), tr("Unable to find the specified file '%1'").arg(workspaceFileName), QMessageBox::Ok, QMessageBox::Ok);
                return;
            }

            WorkspaceSettingsProvider* provider = WorkspaceSettingsProvider::CreateFromFile(workspaceFileName.toUtf8().data());
            if (!provider)
            {
                QMessageBox::warning(this, tr("Corrupted file?"), tr("Unable to parse the specified file '%1'").arg(workspaceFileName), QMessageBox::Ok, QMessageBox::Ok);
                return;
            }

            // 2: extract therefrom the associated DRL file
            AZStd::string windowStateStr = AZStd::string::format("DRILLER CAPTURE WINDOW WORKSPACE");
            AZ::u32 workspaceCRC = AZ::Crc32(windowStateStr.c_str());

            DrillerCaptureWindowWorkspace* workspace = provider->FindSetting<DrillerCaptureWindowWorkspace>(workspaceCRC);
            if (!workspace)
            {
                QMessageBox::warning(this, tr("Corrupted file?"), tr("Specified file '%1' does not appear to contain a workspace.").arg(workspaceFileName), QMessageBox::Ok, QMessageBox::Ok);
                return;
            }

            m_inactiveChannels.clear();
            for (auto iter = workspace->m_ChannelIDs.begin(); iter != workspace->m_ChannelIDs.end(); ++iter)
            {
                m_inactiveChannels.insert(*iter);
            }

            // 3: load that data, which in turn clears and re-instantiates all needed aggregators
            // other side effects include changing the current filename and replacing any current data loaded
            if (openDrillerFileAlso)
            {
                m_isLoadingFile = true;
                OnOpenDrillerFileForWorkspace(QString(workspace->m_matchingDataFileName.c_str()), workspaceFileName);
                m_isLoadingFile = false;
            }

            SetCaptureMode(CaptureMode::Inspecting);

            // 4: extract from the DRW any settings that I have saved there
            // 5: synchronous EBUS message that informs all the aggregators that new settings are available
            EBUS_EVENT_ID(m_identity, Driller::DrillerWorkspaceWindowMessages::Bus, ApplySettingsFromWorkspace, provider);
            m_AnnotationProvider.LoadSettingsFromWorkspace(provider);

            // 6: aggregators are responsible for checking if any of their data dialogs are required, and open them

            EBUS_EVENT_ID(m_identity, Driller::DrillerWorkspaceWindowMessages::Bus, ActivateWorkspaceSettings, provider);

            // 7: main window itself should load its settings, which will include the current scrubber frame
            m_scrubberCurrentFrame = 0;
            m_scrubberCurrentEvent = 0;
            m_playbackLoopBegin = 0;
            m_playbackLoopEnd =  0;

            SetFrameRangeBegin(workspace->m_frameRangeBegin);
            SetFrameRangeEnd(workspace->m_frameRangeEnd);
            m_bForceNextScrub = true;
            SetScrubberFrame(workspace->m_scrubberCurrentFrame);
            SetPlaybackLoopBegin(workspace->m_playbackLoopBegin);
            SetPlaybackLoopEnd(workspace->m_playbackLoopEnd);
            OnQuantMenuFinal(workspace->m_visibleFrames);
            m_gui->controlScrollBar->setSliderPosition(workspace->m_sliderPosition);

            SetScrubberEvent(workspace->m_scrubberCurrentEvent);

            // 8: close the local settings DRW
            delete provider;

            RepopulateAnnotations();
        }
    }

    void DrillerCaptureWindow::OnApplyWorkspaceFile(QString fileName)
    {
        if (!fileName.isNull())
        {
            if (m_data)
            {
                OnOpenWorkspaceFile(fileName, false);
            }
        }
    }

    void DrillerCaptureWindow::OnSaveWorkspaceFile(QString fileName, bool automated)
    {
        if (!fileName.isNull())
        {
            if (m_data)
            {
                // 1: spawn a new local settings object using the DRW
                WorkspaceSettingsProvider provider;

                // 2: push my own settings into the DRW
                // plus logic to copy/rename tmp DRL files
                AZStd::string windowStateStr = AZStd::string::format("DRILLER CAPTURE WINDOW WORKSPACE");
                AZ::u32 workspaceCRC = AZ::Crc32(windowStateStr.c_str());

                DrillerCaptureWindowWorkspace* workspace = provider.CreateSetting<DrillerCaptureWindowWorkspace>(workspaceCRC);
                if (!automated)
                {
                    m_currentDataFilename = PrepDataFileForSaving(m_currentDataFilename, fileName);
                }
                workspace->m_matchingDataFileName = m_currentDataFilename.toUtf8().data();

                m_inactiveChannels.clear();
                for (ChannelControl* channelControl : m_channels)
                {
                    for (ChannelProfilerWidget* profilerWidget : channelControl->GetProfilers())
                    {
                        if (!profilerWidget->IsActive())
                        {
                            m_inactiveChannels.insert(profilerWidget->GetID());
                        }
                    }
                }

                workspace->m_ChannelIDs.clear();
                for (AZStd::set<AZ::Uuid>::iterator iter = m_inactiveChannels.begin(); iter != m_inactiveChannels.end(); ++iter)
                {
                    workspace->m_ChannelIDs.push_back(*iter);
                }
                workspace->m_scrubberCurrentFrame = m_scrubberCurrentFrame;
                workspace->m_frameRangeBegin = m_frameRangeBegin;
                workspace->m_frameRangeEnd = m_frameRangeEnd;
                workspace->m_visibleFrames = m_visibleFrames;
                workspace->m_scrubberCurrentEvent = m_scrubberCurrentEvent;

                workspace->m_playbackLoopBegin = m_playbackLoopBegin;
                workspace->m_playbackLoopEnd = m_playbackLoopEnd;
                workspace->m_sliderPosition = m_gui->controlScrollBar->sliderPosition();

                // 3: synchronous EBUS message that informs all the aggregators to push their own settings into the DRW
                // 4: aggregators are responsible for dealing with their display view dialogs internally
                EBUS_EVENT_ID(m_identity, Driller::DrillerWorkspaceWindowMessages::Bus, SaveSettingsToWorkspace, &provider);
                m_AnnotationProvider.SaveSettingsToWorkspace(&provider);

                if (!provider.WriteToFile(fileName.toUtf8().data()))
                {
                    SetCaptureDirty(true);
                    QMessageBox::warning(this, tr("Could not save workspace"), tr("Unable to write data to the specified file '%1'").arg(fileName), QMessageBox::Ok, QMessageBox::Ok);
                }
                else
                {
                    SetCaptureDirty(false);
                }
                UpdateLiveControls();
            }
        }
    }

    void DrillerCaptureWindow::OnSaveDrillerFile()
    {
        if (m_frameRangeEnd <= 0)
        {
            if (m_identity == 0)
            {
                ResetCaptureControls();
            }
            return;
        }

        QString saveCapturePath;
        QString tempWorkspaceName;

        auto newState = AZ::UserSettings::CreateFind<DrillerCaptureWindowSavedState>(m_windowStateCRC, AZ::UserSettings::CT_GLOBAL);
        if (!newState->m_priorSaveFolder.empty())
        {
            saveCapturePath = newState->m_priorSaveFolder.data();
        }
        else
        {
            auto paths = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);

            if (paths.isEmpty())
            {
                paths = QStandardPaths::standardLocations(QStandardPaths::TempLocation);
            }

            if (!paths.isEmpty())
            {
                saveCapturePath = paths.first();
            }
        }

        bool success = false;

        while (!success)
        {
            QString sourcename = m_tmpCaptureFilename;

            if (!m_currentDataFilename.isEmpty())
            {
                sourcename = m_currentDataFilename;
            }

            QString fileName = QFileDialog::getSaveFileName(this, "Save Driller File As...", saveCapturePath, "Driller Files (*.drl)");
            if (!fileName.isNull())
            {
                SetCaptureDirty(false);
                if (sourcename == fileName)
                {
                    QMessageBox::warning(this, tr("Unable to save"), tr("You can't save a data file over itself ( '%1' to '%2' )").arg(sourcename).arg(fileName));
                }
                else
                {
                    (void)QFile::remove(fileName);
                    success = QFile::copy(sourcename, fileName);
                    if (success)
                    {
                        m_currentDataFilename = fileName;

                        {
                            QTemporaryFile f;
                            f.setAutoRemove(false);
                            if (f.open())
                            {
                                tempWorkspaceName = f.fileName();
                            }
                        }
                        if (!tempWorkspaceName.isEmpty())
                        {
                            OnSaveWorkspaceFile(tempWorkspaceName, true);
                        }

                        ResetCaptureControls();

                        EBUS_EVENT(Driller::DrillerDataViewMessages::Bus, EventRequestOpenWorkspace, tempWorkspaceName.toUtf8().data());
                        auto deleteResult = QFile::remove(tempWorkspaceName);
                        if (!deleteResult)
                        {
                            QMessageBox::warning(this, tr("Can't delete temp file"), tr("File = ( %1 )").arg(tempWorkspaceName));
                        }

                        return;
                    }
                    else
                    {
                        QMessageBox::warning(this, tr("Unable to save"), tr("Could not copy '%1' to '%2'").arg(sourcename).arg(fileName));
                    }
                }
            }

            if (fileName.isNull() || m_identity == 0) // close this window if no file named OR this is a LIVE channel
            {
                ResetCaptureControls();
                return;
            }
        }
    }

    QString DrillerCaptureWindow::PrepDataFileForSaving(QString filename, QString workspaceName)
    {
        // is this a TMP file?
        QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        if (filename.contains(tempPath, Qt::CaseInsensitive))
        {
            // yes := rename to match workspace
            QString newFilename(workspaceName.left(workspaceName.size() - 3) + "drl");
            // and then copy
            QFile::copy(filename, newFilename);
            m_currentDataFilename = newFilename;
            UpdateLiveControls();
            return newFilename;
        }

        return filename;
    }

    QString DrillerCaptureWindow::PrepTempFile(QString filename)
    {
        QString tmpCapturePath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        tmpCapturePath = QDir(tmpCapturePath).absoluteFilePath(filename);
        m_tmpCaptureFilename = tmpCapturePath;
        m_currentDataFilename = m_tmpCaptureFilename;
        return tmpCapturePath;
    }

    void DrillerCaptureWindow::OnAnnotationOptionsClick()
    {
        // show the annotations configure window:
        if (m_ptrConfigureAnnotationsWindow)
        {
            m_ptrConfigureAnnotationsWindow->raise();
        }
        else
        {
            m_ptrConfigureAnnotationsWindow = aznew ConfigureAnnotationsWindow(this);
            m_ptrConfigureAnnotationsWindow->Initialize(&m_AnnotationProvider);
            connect(m_ptrConfigureAnnotationsWindow, SIGNAL(destroyed(QObject*)), this, SLOT(OnAnnotationsDialogDestroyed()));
            m_ptrConfigureAnnotationsWindow->show();
        }
    }

    void DrillerCaptureWindow::OnSelectedAnnotationChannelsChanged()
    {
        // rebuild the annotations.
        RepopulateAnnotations();
        // update the views.
    }


    void DrillerCaptureWindow::OnAnnotationsDialogDestroyed()
    {
        m_ptrConfigureAnnotationsWindow = NULL;
    }

    void DrillerCaptureWindow::InformOfMouseOverAnnotation(const Annotation& annotation)
    {
        if (m_collectedAnnotations.empty())
        {
            QTimer::singleShot(0, this, SLOT(CommitAnnotationsCollected()));
        }
        m_collectedAnnotations.push_back(annotation);
    }

    void DrillerCaptureWindow::CommitAnnotationsCollected()
    {
        FrameNumberType frameCounter = -1;
        QString finalText;
        AZ::u32 priorCRC = 0;

        AZStd::sort(m_collectedAnnotations.begin(), m_collectedAnnotations.end(),
            [](const Annotation& first, const Annotation& second)
            {
                return first.GetEventIndex() < second.GetEventIndex();
            }
            );

        int numConcated = 0;
        for (auto iter = m_collectedAnnotations.begin(); iter != m_collectedAnnotations.end(); ++iter)
        {
            if (numConcated > 10)
            {
                int numRemaining = (int)m_collectedAnnotations.size() - numConcated;
                finalText += QString("... and %1 other annotations").arg(numRemaining);
                break;
            }
            ++numConcated;

            const Annotation& annot = *iter;
            if ((annot.GetFrameIndex() == frameCounter) && (priorCRC == annot.GetChannelCRC()))
            {
                finalText += QString("Event %1: '%2'<BR>").arg(annot.GetEventIndex()).arg(annot.GetText().c_str());
            }
            else if (annot.GetFrameIndex() == frameCounter)
            {
                finalText += QString("<I>%2</I><BR>Event %3: '%4'<BR>").arg(annot.GetChannel().c_str()).arg(annot.GetEventIndex()).arg(annot.GetText().c_str());
            }
            else
            {
                finalText += QString("<B>Frame %1</B><BR><I>%2</I><BR>Event %3: '%4'<BR>").arg(annot.GetFrameIndex()).arg(annot.GetChannel().c_str()).arg(annot.GetEventIndex()).arg(annot.GetText().c_str());
            }

            frameCounter = annot.GetFrameIndex();
            priorCRC = annot.GetChannelCRC();
        }

        QToolTip::showText(QCursor::pos(), finalText);
        m_collectedAnnotations.clear();
    }

    void DrillerCaptureWindow::InformOfClickAnnotation(const Annotation& annotation)
    {
        (void)annotation;
    }

    //////////////////////////////////////////////////////////////////////////
    // Target Manager Messages
    void DrillerCaptureWindow::DesiredTargetConnected(bool connected)
    {
        m_TargetConnected = connected;

        if (IsInCaptureMode(CaptureMode::Inspecting))
        {
            return;
        }

        // - have an existing capture?
        // - - ask to save it
        if (IsInCaptureMode(CaptureMode::Capturing))
        {
            OnSaveDrillerFile();
        }

        QString tmpCapturePath;

        if (connected)
        {
            SetScrubberFrame(0);
            SetFrameRangeBegin(0);
            SetFrameRangeEnd(0);
            SetCaptureDirty(false);

            tmpCapturePath = PrepTempFile(baseTempFileName);
        }
        else
        {
            SetScrubberFrame(0);
            SetFrameRangeBegin(0);
            SetFrameRangeEnd(0);
            SetCaptureDirty(false);

            m_gui->captureButton->setEnabled(false);
            m_gui->captureButton->setText(tr("Capture"));
            m_gui->captureButton->setToolTip(tr("Begin Capturing Driller Data"));

            tmpCapturePath.clear();
        }

        UpdateLiveControls();
    }

    void DrillerCaptureWindow::Reflect(AZ::ReflectContext* context)
    {
        // data container is the one place that knows about all the aggregators
        // and indeed is responsible for creating them
        DrillerDataContainer::Reflect(context);
        DrillerCaptureWindowWorkspace::Reflect(context);
        DrillerCaptureWindowSavedState::Reflect(context);
        AnnotationsProvider::Reflect(context);

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<DrillerCaptureWindow>("DrillerCaptureWindow")->
                Method("ShowWindow", &DrillerCaptureWindow::OnOpen)->
                Method("HideWindow", &DrillerCaptureWindow::OnClose);
        }
    }
}//namespace Driller
