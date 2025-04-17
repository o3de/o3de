/* Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/InspectorBus.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Serialization/Locale.h>
#include "TimeViewPlugin.h"
#include "TrackDataHeaderWidget.h"
#include "TrackDataWidget.h"
#include "TrackHeaderWidget.h"
#include "TimeInfoWidget.h"
#include "TimeViewToolBar.h"

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/MotionSetsWindow/MotionSetsWindowPlugin.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/MotionEvents/MotionEventPresetsWidget.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/MotionEvents/MotionEventWidget.h>

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/MainWindow.h>

#include <QCheckBox>
#include <QDir>
#include <QDockWidget>
#include <QGridLayout>
#include <QPushButton>
#include <QScrollBar>

#include <MCore/Source/LogManager.h>
#include <MCore/Source/Compare.h>
#include <MCore/Source/ReflectionSerializer.h>
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <EMotionFX/Source/MotionEvent.h>
#include <EMotionFX/Source/MotionEventTrack.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/Recorder.h>
#include <EMotionFX/Source/MotionEventTable.h>

#include <AzQtComponents/Utilities/Conversions.h>

namespace EMStudio
{
    TimeViewPlugin::TimeViewPlugin()
        : EMStudio::DockWidgetPlugin()
    {
        m_brushCurTimeHandle = QBrush(QColor(255, 180, 0));
        m_penCurTimeHandle   = QPen(QColor(255, 180, 0));
        m_penTimeHandles     = QPen(QColor(150, 150, 150), 1, Qt::DotLine);
        m_penCurTimeHelper   = QPen(QColor(100, 100, 100), 1, Qt::DotLine);
    }

    TimeViewPlugin::~TimeViewPlugin()
    {
        if (m_motionEventWidget)
        {
            delete m_motionEventWidget;
            m_motionEventWidget = nullptr;
        }

        EMotionFX::AnimGraphEditorNotificationBus::Handler::BusDisconnect();

        for (MCore::Command::Callback* callback : m_commandCallbacks)
        {
            GetCommandManager()->RemoveCommandCallback(callback, false);
            delete callback;
        }
        m_commandCallbacks.clear();

        RemoveAllTracks();

        // get rid of the cursors
        delete m_zoomInCursor;
        delete m_zoomOutCursor;

        // get rid of the motion infos
        for (MotionInfo* motionInfo : m_motionInfos)
        {
            delete motionInfo;
        }
    }

    // get the name
    const char* TimeViewPlugin::GetName() const
    {
        return "Time View";
    }

    void EMStudio::TimeViewPlugin::Reflect(AZ::ReflectContext * context)
    {
        MotionEventPreset::Reflect(context);
        MotionEventPresetManager::Reflect(context);
    }


    // get the plugin type id
    uint32 TimeViewPlugin::GetClassID() const
    {
        return TimeViewPlugin::CLASS_ID;
    }

    // init after the parent dock window has been created
    bool TimeViewPlugin::Init()
    {
        m_commandCallbacks.emplace_back(new CommandAdjustMotionCallback(false));
        GetCommandManager()->RegisterCommandCallback("AdjustMotion", m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandSelectCallback(false));
        GetCommandManager()->RegisterCommandCallback("Select", m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandUnselectCallback(false));
        GetCommandManager()->RegisterCommandCallback("Unselect", m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandClearSelectionCallback(false));
        GetCommandManager()->RegisterCommandCallback("ClearSelection", m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandRecorderClearCallback(false));
        GetCommandManager()->RegisterCommandCallback("RecorderClear", m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new UpdateInterfaceCallback(false));
        GetCommandManager()->RegisterCommandCallback("AdjustDefaultPlayBackInfo", m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new UpdateInterfaceCallback(false));
        GetCommandManager()->RegisterCommandCallback("PlayMotion", m_commandCallbacks.back());

        // load the cursors
        m_zoomInCursor = new QCursor(QPixmap(QDir{ QString(MysticQt::GetDataDir().c_str()) }.filePath("Images/Rendering/ZoomInCursor.png")).scaled(32, 32));
        m_zoomOutCursor = new QCursor(QPixmap(QDir{ QString(MysticQt::GetDataDir().c_str()) }.filePath("Images/Rendering/ZoomOutCursor.png")).scaled(32, 32));

        m_togglePresetsView = new QAction(QIcon{":EMotionFX/List.svg"}, tr("Show/Hide Presets"), this);
        connect(m_togglePresetsView, &QAction::triggered, this, &TimeViewPlugin::toggleMotionEventPresetsPane);
        m_togglePresetsView->setCheckable(true);
        m_togglePresetsView->setChecked(true);

        // create main widget
        m_mainWidget = new QWidget(m_dock);
        m_dock->setWidget(m_mainWidget);

        // main content layout
        QGridLayout* mainLayout = new QGridLayout();
        mainLayout->setMargin(0);
        mainLayout->setSpacing(0);

        m_mainWidget->setLayout(mainLayout);
        // create widgets in the header
        QHBoxLayout* topLayout = new QHBoxLayout();
        // Top
        m_timeViewToolBar = new TimeViewToolBar(this);

        // Top-left
        m_timeInfoWidget = new TimeInfoWidget(this);
        m_timeInfoWidget->setFixedWidth(175);
        // Top-right
        auto rightSideToolbar = new QToolBar{};
        rightSideToolbar->addAction(m_togglePresetsView);
        topLayout->addWidget(m_timeInfoWidget);
        topLayout->addWidget(m_timeViewToolBar);
        topLayout->addWidget(rightSideToolbar);
        mainLayout->addLayout(topLayout, 0, 0, 1, 2);

        // Top-right
        m_trackDataHeaderWidget = new TrackDataHeaderWidget(this, m_dock);
        m_trackDataHeaderWidget->setFixedHeight(40);
        // create widgets in the body. For the body we are going to put a scroll area
        // so we can get a vertical scroll bar when we have more tracks than what the
        // view can show
        QScrollArea* bodyWidget = new QScrollArea(m_mainWidget);
        bodyWidget->setFrameShape(QFrame::NoFrame);
        bodyWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        bodyWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        bodyWidget->setWidgetResizable(true);

        // scroll areas require an inner widget to hold the layout (instead of a layout directly).
        QWidget* innerWidget = new QWidget(bodyWidget);
        QHBoxLayout* bodyLayout = new QHBoxLayout();
        bodyLayout->setMargin(0);
        bodyLayout->setSpacing(0);
        innerWidget->setLayout(bodyLayout);
        bodyWidget->setWidget(innerWidget);

        // Bottom-left
        m_trackHeaderWidget = new TrackHeaderWidget(this, m_dock);
        m_trackHeaderWidget->setFixedWidth(175);
        bodyLayout->addWidget(m_trackHeaderWidget);

        // Left
        auto* trackAndTrackDataWidget = new QWidget;
        QHBoxLayout* addTrackAndTrackDataLayout = new QHBoxLayout;
        addTrackAndTrackDataLayout->setMargin(0);
        addTrackAndTrackDataLayout->setSpacing(0);
        addTrackAndTrackDataLayout->addWidget(m_trackHeaderWidget->GetAddTrackWidget());
        m_trackHeaderWidget->GetAddTrackWidget()->setFixedWidth(175);
        addTrackAndTrackDataLayout->addWidget(m_trackDataHeaderWidget);
        trackAndTrackDataWidget->setLayout(addTrackAndTrackDataLayout);

        // bottom-right
        m_trackDataWidget = new TrackDataWidget(this, m_dock);
        bodyLayout->addWidget(m_trackDataWidget);

        auto* contentContainer = new QWidget;
        auto* contentLayout = new QVBoxLayout;
        contentContainer->setLayout(contentLayout);
        contentLayout->setMargin(0);
        contentLayout->setSpacing(0);
        contentLayout->addWidget(trackAndTrackDataWidget);
        contentLayout->addWidget(bodyWidget);

        // create the motion event presets pane
        // create a slider for motion events presets pane
        m_paneSplitter = new QSplitter(Qt::Horizontal, m_mainWidget);
        m_motionEventsPresetsWidget = new MotionEventPresetsWidget{m_paneSplitter, this};
        m_paneSplitter->addWidget(contentContainer);
        m_paneSplitter->addWidget(m_motionEventsPresetsWidget);
        m_paneSplitter->setCollapsible(0, false);
        m_paneSplitter->setStretchFactor(0, 16);
        m_paneSplitter->setStretchFactor(1, 1);
        connect(m_paneSplitter, &QSplitter::splitterMoved,
                this,
                [=]{m_togglePresetsView->setChecked(m_paneSplitter->sizes().at(1) > 0 );});
        mainLayout->addWidget(m_paneSplitter, 1, 0, 1, 2);

        // connect TrackDataWidget
        connect(m_trackDataWidget, &TrackDataWidget::SelectionChanged, this, &TimeViewPlugin::OnSelectionChanged);
        connect(m_trackDataWidget, &TrackDataWidget::ElementTrackChanged, this, &TimeViewPlugin::MotionEventTrackChanged);
        connect(m_trackDataWidget, &TrackDataWidget::MotionEventChanged, this, &TimeViewPlugin::MotionEventChanged);
        connect(this, &TimeViewPlugin::DeleteKeyPressed, this, &TimeViewPlugin::RemoveSelectedMotionEvents);
        connect(m_dock, &QDockWidget::visibilityChanged, this, &TimeViewPlugin::VisibilityChanged);

        connect(this, &TimeViewPlugin::ManualTimeChange, this, &TimeViewPlugin::OnManualTimeChange);

        connect(m_timeViewToolBar, &TimeViewToolBar::RecorderStateChanged, this, &TimeViewPlugin::RecorderStateChanged);

        SetCurrentTime(0.0f);
        SetScale(1.0f);

        SetRedrawFlag();

        m_timeViewToolBar->UpdateInterface();

        EMotionFX::AnimGraphEditorNotificationBus::Handler::BusConnect();

        // Create the motion event properties widget.
        m_motionEventWidget = new MotionEventWidget();
        m_motionEventWidget->hide();
        connect(this, &TimeViewPlugin::SelectionChanged, this, [=]
            {
                if (!m_motionEventWidget)
                {
                    return;
                }

                UpdateSelection();
                if (GetNumSelectedEvents() != 1)
                {
                    m_motionEventWidget->ReInit();
                    m_motionEventWidget->hide();
                    EMStudio::InspectorRequestBus::Broadcast(&EMStudio::InspectorRequestBus::Events::Clear); // This also gets called when just switching a motion
                }
                else
                {
                    EventSelectionItem selectionItem = GetSelectedEvent(0);
                    m_motionEventWidget->ReInit(selectionItem.m_motion, selectionItem.GetMotionEvent());
                    EMStudio::InspectorRequestBus::Broadcast(&EMStudio::InspectorRequestBus::Events::UpdateWithHeader,
                        "Motion Event",
                        MotionEventWidget::s_headerIcon,
                        m_motionEventWidget);
                }
            });

        return true;
    }


    // add a new track
    void TimeViewPlugin::AddTrack(TimeTrack* track)
    {
        m_tracks.emplace_back(track);
        SetRedrawFlag();
    }


    // delete all tracks
    void TimeViewPlugin::RemoveAllTracks()
    {
        // get the number of time tracks and iterate through them
        for (TimeTrack* track : m_tracks)
        {
            delete track;
        }

        m_tracks.clear();
        SetRedrawFlag();
    }

    TimeTrack* TimeViewPlugin::FindTrackByElement(TimeTrackElement* element) const
    {
        const auto foundTrack = AZStd::find_if(begin(m_tracks), end(m_tracks), [element](const TimeTrack* timeTrack)
        {
            // get the number of time track elements and iterate through them
            const size_t numElements = timeTrack->GetNumElements();
            for (size_t j = 0; j < numElements; ++j)
            {
                if (timeTrack->GetElement(j) == element)
                {
                    return true;
                }
            }
            return false;
        });
        return foundTrack != end(m_tracks) ? *foundTrack : nullptr;
    }

    AZ::Outcome<size_t> TimeViewPlugin::FindTrackIndex(const TimeTrack* track) const
    {
        const auto foundTrack = AZStd::find(begin(m_tracks), end(m_tracks), track);
        if (foundTrack != end(m_tracks))
        {
            return AZ::Success(static_cast<size_t>(AZStd::distance(begin(m_tracks), foundTrack)));
        }
        return AZ::Failure();
    }

    // round a double based on 0.5 (everything above is rounded up, everything else rounded down)
    double TimeViewPlugin::RoundDouble(double x) const
    {
        if ((std::numeric_limits<double>::max() - 0.5) <= x)
        {
            return std::numeric_limits<double>::max();
        }

        if ((-1 * std::numeric_limits<double>::max() + 0.5) > x)
        {
            return (-1 * std::numeric_limits<double>::max());
        }

        double intpart;
        double fractpart = modf(x, &intpart);

        if (fractpart >= 0.5)
        {
            return (intpart + 1);
        }
        else
        if (fractpart >= -0.5)
        {
            return intpart;
        }
        else
        {
            return (intpart - 1);
        }
    }


    void TimeViewPlugin::DecomposeTime(double timeValue, uint32* outMinutes, uint32* outSeconds, uint32* outMilSecs, uint32* outFrameNr) const
    {
        if (outMinutes)
        {
            *outMinutes     = aznumeric_cast<uint32>(timeValue / 60.0);
        }
        if (outSeconds)
        {
            *outSeconds     = aznumeric_cast<uint32>(fmod(timeValue, 60.0));
        }
        if (outMilSecs)
        {
            *outMilSecs     = aznumeric_cast<uint32>(fmod(RoundDouble(timeValue * 1000.0), 1000.0) / 10.0);              //fmod(pixelTime, 1.0) * 100.0;
        }
        if (outFrameNr)
        {
            *outFrameNr     = aznumeric_cast<uint32>(timeValue / (double)m_fps);
        }
    }


    // calculate time values
    void TimeViewPlugin::CalcTime(double xPixel, double* outPixelTime, uint32* outMinutes, uint32* outSeconds, uint32* outMilSecs, uint32* outFrameNr, bool scaleXPixel) const
    {
        if (scaleXPixel)
        {
            xPixel *= m_timeScale;
        }

        const double pixelTime = ((xPixel + m_scrollX) / m_pixelsPerSecond);

        if (outPixelTime)
        {
            *outPixelTime   = pixelTime;
        }
        if (outMinutes)
        {
            *outMinutes     = aznumeric_cast<uint32>(pixelTime / 60.0);
        }
        if (outSeconds)
        {
            *outSeconds     = aznumeric_cast<uint32>(fmod(pixelTime, 60.0));
        }
        if (outMilSecs)
        {
            *outMilSecs     = aznumeric_cast<uint32>(fmod(RoundDouble(pixelTime * 1000.0), 1000.0) / 10.0);             //fmod(pixelTime, 1.0) * 100.0;
        }
        if (outFrameNr)
        {
            *outFrameNr     = aznumeric_cast<uint32>(pixelTime / (double)m_fps);
        }
    }


    void TimeViewPlugin::UpdateCurrentMotionInfo()
    {
        if (EMotionFX::GetRecorder().GetRecordTime() > MCore::Math::epsilon)
        {
            return;
        }

        if (m_motion)
        {
            MotionInfo* motionInfo  = FindMotionInfo(m_motion->GetID());
            motionInfo->m_scale      = m_targetTimeScale;
            motionInfo->m_scrollX    = m_targetScrollX;
        }
    }


    // update the sub-widgets
    void TimeViewPlugin::UpdateVisualData()
    {
        ValidatePluginLinks();
        m_trackDataHeaderWidget->update();
        m_trackDataWidget->update();
        m_timeInfoWidget->update();
        m_dirty = false;
    }

    // calc the time value to a pixel value (excluding scroll)
    double TimeViewPlugin::TimeToPixel(double timeInSeconds, bool scale) const
    {
        double result = ((timeInSeconds * m_pixelsPerSecond)) - m_scrollX;
        if (scale)
        {
            return (result * m_timeScale);
        }
        else
        {
            return result;
        }
    }


    // return the element at a given pixel, or nullptr
    TimeTrackElement* TimeViewPlugin::GetElementAt(int32 x, int32 y)
    {
        // for all tracks
        for (const TimeTrack* track : m_tracks)
        {
            // check if the absolute pixel is inside
            TimeTrackElement* result = track->GetElementAt(aznumeric_cast<int32>(x + m_scrollX), y);
            if (result)
            {
                return result;
            }
        }

        return nullptr;
    }


    // return the track at a given pixel y value, or nullptr
    TimeTrack* TimeViewPlugin::GetTrackAt(int32 y)
    {
        // for all tracks
        const auto foundTrack = AZStd::find_if(begin(m_tracks), end(m_tracks), [y](const TimeTrack* track)
        {
            return track->GetIsInside(y);
        });
        return foundTrack != end(m_tracks) ? *foundTrack : nullptr;
    }


    // unselect all elements
    void TimeViewPlugin::UnselectAllElements()
    {
        // for all tracks
        for (TimeTrack* track : m_tracks)
        {
            // for all elements, deselect it
            const size_t numElems = track->GetNumElements();
            for (size_t i = 0; i < numElems; ++i)
            {
                track->GetElement(i)->SetIsSelected(false);
            }
        }
        SetRedrawFlag();

        emit SelectionChanged();
    }


    // return the time of the current time marker, in seconds
    double TimeViewPlugin::GetCurrentTime() const
    {
        return m_curTime;
    }


    double TimeViewPlugin::PixelToTime(double xPixel, bool isScaledPixel) const
    {
        if (isScaledPixel)
        {
            xPixel /= m_timeScale;
        }

        return ((xPixel + m_scrollX) / m_pixelsPerSecond);
    }


    void TimeViewPlugin::DeltaScrollX(double deltaX, bool animate)
    {
        double newTime = (m_targetScrollX + (deltaX / m_timeScale)) / m_pixelsPerSecond;
        if (newTime < m_maxTime - (1 / m_timeScale))
        {
            SetScrollX(m_targetScrollX + (deltaX / m_timeScale), animate);
        }
        else
        {
            SetScrollX((m_maxTime - ((1 / m_timeScale))) * m_pixelsPerSecond, animate);
        }
        SetRedrawFlag();
    }

    void TimeViewPlugin::SetScrollX(double scrollX, bool animate)
    {
        m_targetScrollX = scrollX;

        if (m_targetScrollX < 0)
        {
            m_targetScrollX = 0;
        }

        if (animate == false)
        {
            m_scrollX = m_targetScrollX;
        }

        // inform the motion info about the changes
        UpdateCurrentMotionInfo();
        SetRedrawFlag();
    }


    // set the current time in seconds
    void TimeViewPlugin::SetCurrentTime(double timeInSeconds)
    {
        const double oneMs = 1.0 / 1000.0;
        if (!AZ::IsClose(m_curTime, timeInSeconds, oneMs))
        {
            m_dirty = true;
        }
        m_curTime = timeInSeconds;
    }


    // snap a time value
    bool TimeViewPlugin::SnapTime(double* inOutTime, TimeTrackElement* elementToIgnore, double snapThreshold)
    {
        double inTime = *inOutTime;

        // make sure we don't go out of bounds
        if (inTime < 0.0)
        {
            inTime = 0.0;
        }

        // for all tracks
        for (TimeTrack* track : m_tracks)
        {
            if (track->GetIsVisible() == false || track->GetIsEnabled() == false)
            {
                continue;
            }

            // for all elements
            const size_t numElems = track->GetNumElements();
            for (size_t i = 0; i < numElems; ++i)
            {
                // don't snap to itself
                TimeTrackElement* element = track->GetElement(i);
                if (element == elementToIgnore)
                {
                    continue;
                }

                // snap the input time to snap locations inside the element
                element->SnapTime(&inTime, snapThreshold);
            }
        }

        // if snapping occurred
        if (MCore::Math::Abs(aznumeric_cast<float>(inTime - *inOutTime)) > 0.0001)
        {
            *inOutTime = inTime;
            return true;
        }
        else // no snapping occurred
        {
            *inOutTime = inTime;
            return false;
        }
    }


    // render the element time handles on top of everything
    void TimeViewPlugin::RenderElementTimeHandles(QPainter& painter, uint32 dataWindowHeight, const QPen& pen)
    {
        // for all tracks
        for (const TimeTrack* track : m_tracks)
        {
            if (track->GetIsVisible() == false)
            {
                continue;
            }

            // for all elements
            const size_t numElems = track->GetNumElements();
            for (size_t i = 0; i < numElems; ++i)
            {
                const TimeTrackElement* elem = track->GetElement(i);

                // if the element has to show its time handles, do it
                if (elem->GetShowTimeHandles())
                {
                    // calculate the dimensions
                    int32 startX, startY, width, height;
                    elem->CalcDimensions(&startX, &startY, &width, &height);

                    // draw the lines
                    painter.setPen(pen);
                    //painter.setRenderHint(QPainter::Antialiasing);
                    painter.drawLine(startX, 0, startX, dataWindowHeight);
                    painter.drawLine(startX + width, 0, startX + width, dataWindowHeight);
                }
            }
        }
    }

    // disables all tool tips
    void TimeViewPlugin::DisableAllToolTips()
    {
        // for all tracks
        for (const TimeTrack* track : m_tracks)
        {
             // for all elements
            const size_t numElems = track->GetNumElements();
            for (size_t i = 0; i < numElems; ++i)
            {
                TimeTrackElement* elem = track->GetElement(i);
                elem->SetShowToolTip(false);
            }
        }
        SetRedrawFlag();
    }


    // check if we're at some resize point with the mouse
    bool TimeViewPlugin::FindResizePoint(int32 x, int32 y, TimeTrackElement** outElement, uint32* outID)
    {
        // for all tracks
        for (const TimeTrack* track : m_tracks)
        {
             if (track->GetIsVisible() == false)
            {
                continue;
            }

            // for all elements
            const size_t numElems = track->GetNumElements();
            for (size_t i = 0; i < numElems; ++i)
            {
                TimeTrackElement* elem = track->GetElement(i);

                // check for a resize point in the element
                uint32 id;
                if (elem->FindResizePoint(x, y, &id))
                {
                    *outElement = elem;
                    *outID = id;
                    return true;
                }
            }
        }

        // no resize point at this location
        *outElement = nullptr;
        *outID      = MCORE_INVALIDINDEX32;

        // no resize point found
        return false;
    }


    void TimeViewPlugin::VisibilityChanged(bool visible)
    {
        MCORE_UNUSED(visible);
        ValidatePluginLinks();
        SetRedrawFlag();
    }


    // render the frame
    void TimeViewPlugin::ProcessFrame(float timePassedInSeconds)
    {
        if (GetManager()->GetAvoidRendering() || m_mainWidget->visibleRegion().isEmpty())
        {
            return;
        }

        m_totalTime += timePassedInSeconds;

        ValidatePluginLinks();

        // animate the zoom
        m_scrollX += (m_targetScrollX - m_scrollX) * 0.2;

        m_isAnimating = false;
        if (m_targetTimeScale > m_timeScale)
        {
            if (MCore::Math::Abs(aznumeric_cast<float>(m_targetScrollX - m_scrollX)) <= 1)
            {
                m_timeScale += (m_targetTimeScale - m_timeScale) * 0.1;
            }
        }
        else
        {
            m_timeScale += (m_targetTimeScale - m_timeScale) * 0.1;
        }

        if (MCore::Math::Abs(aznumeric_cast<float>(m_targetScrollX - m_scrollX)) <= 1)
        {
            m_scrollX = m_targetScrollX;
        }
        else
        {
            m_isAnimating = true;
        }

        if (MCore::Math::Abs(aznumeric_cast<float>(m_targetTimeScale - m_timeScale)) <= 0.001)
        {
            m_timeScale = m_targetTimeScale;
        }
        else
        {
            m_isAnimating = true;
        }

        // get the maximum time
        GetDataTimes(&m_maxTime, nullptr, nullptr);
        UpdateMaxHeight();
        m_trackDataWidget->UpdateRects();

        if (MCore::Math::Abs(aznumeric_cast<float>(m_maxHeight - m_lastMaxHeight)) > 0.0001)
        {
            m_lastMaxHeight = m_maxHeight;
        }

        if (m_trackDataWidget->m_dragging == false && m_trackDataWidget->m_resizing == false)
        {
            m_timeInfoWidget->SetOverwriteTime(PixelToTime(m_curMouseX), m_maxTime);
        }

        // update the hovering items
        m_eventEmitterNode = nullptr;
        m_actorInstanceData  = m_trackDataWidget->FindActorInstanceData();
        if (EMotionFX::GetRecorder().GetRecordTime() > MCore::Math::epsilon)
        {
            m_eventHistoryItem   = m_trackDataWidget->FindEventHistoryItem(m_actorInstanceData, aznumeric_cast<int32>(m_curMouseX), aznumeric_cast<int32>(m_curMouseY));
            m_nodeHistoryItem    = m_trackDataWidget->FindNodeHistoryItem(m_actorInstanceData, aznumeric_cast<int32>(m_curMouseX), aznumeric_cast<int32>(m_curMouseY));

            if (m_eventHistoryItem)
            {
                EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(m_eventHistoryItem->m_animGraphId);
                if (animGraph)
                {
                    m_eventEmitterNode = animGraph->RecursiveFindNodeById(m_eventHistoryItem->m_emitterNodeId);
                }
            }
        }
        else
        {
            m_actorInstanceData  = nullptr;
            m_nodeHistoryItem    = nullptr;
            m_eventHistoryItem   = nullptr;
        }

        switch (m_mode)
        {
            case TimeViewMode::Motion:
            {
                double newCurrentTime = m_curTime;

                if (!m_motion)
                {
                    // Use the start time when either no motion is selected.
                    newCurrentTime = 0.0f;
                }
                else
                {
                    const AZStd::vector<EMotionFX::MotionInstance*>& selectedMotionInstances = CommandSystem::GetCommandManager()->GetCurrentSelection().GetSelectedMotionInstances();
                    if (selectedMotionInstances.size() == 1 &&
                        selectedMotionInstances[0]->GetMotion() == m_motion)
                    {
                        EMotionFX::MotionInstance* motionInstance = selectedMotionInstances[0];
                        if (!AZ::IsClose(aznumeric_cast<float>(m_curTime), motionInstance->GetCurrentTime(), MCore::Math::epsilon))
                        {
                            newCurrentTime = motionInstance->GetCurrentTime();
                        }
                    }
                }

                if (!m_trackDataWidget->m_dragging && !m_trackDataWidget->m_resizing)
                {
                    SetCurrentTime(newCurrentTime);
                }

                break;
            }

            case TimeViewMode::AnimGraph:
            {
                EMotionFX::Recorder& recorder = EMotionFX::GetRecorder();
                if (recorder.GetRecordTime() > MCore::Math::epsilon)
                {
                    if (recorder.GetIsInPlayMode() && recorder.GetIsInAutoPlayMode())
                    {
                        SetCurrentTime(recorder.GetCurrentPlayTime());
                        MakeTimeVisible(m_curTime, 0.5, false);
                    }

                    if (recorder.GetIsRecording())
                    {
                        SetCurrentTime(m_maxTime);
                        MakeTimeVisible(recorder.GetRecordTime(), 0.95, false);
                    }
                }
                else
                {
                    SetCurrentTime(0.0f);
                }

                break;
            }

            default:
            {
                SetCurrentTime(0.0f);
                break;
            }
        }

        if (m_isAnimating)
        {
            m_dirty = true;
        }

        bool redraw = false;
        float fps = 15.0f;
    #ifndef MCORE_DEBUG
        if (m_isAnimating)
        {
            fps = 60.0f;
        }
        else
        {
            fps = 40.0f;
        }
    #endif

        if (m_totalTime >= 1.0f / fps)
        {
            redraw = true;
            m_totalTime = 0.0f;
        }

        if (redraw && m_dirty)
        {
            UpdateVisualData();
        }
    }


    void TimeViewPlugin::SetRedrawFlag()
    {
        m_dirty = true;
    }


    void TimeViewPlugin::UpdateViewSettings()
    {
        SetScale(m_timeScale);
    }


    void TimeViewPlugin::SetScale(double scale, bool animate)
    {
        double curTime = GetCurrentTime();

        m_targetTimeScale    = scale;
        m_targetTimeScale    = MCore::Clamp(scale, m_minScale, m_maxScale);

        if (animate == false)
        {
            m_timeScale  = m_targetTimeScale;
        }

        UpdateCurrentMotionInfo();

        SetCurrentTime(curTime);

        //  MakeTimeVisible( centerTime, 0.5 );
    }


    void TimeViewPlugin::OnKeyPressEvent(QKeyEvent* event)
    {
        // delete pressed
        if (event->key() == Qt::Key_Delete)
        {
            emit DeleteKeyPressed();
            event->accept();
            return;
        }

        if (event->key() == Qt::Key_Down)
        {
            m_trackDataWidget->scroll(0, 20);
            event->accept();
            return;
        }

        if (event->key() == Qt::Key_Up)
        {
            m_trackDataWidget->scroll(0, -20);
            event->accept();
            return;
        }

        if (event->key() == Qt::Key_Plus)
        {
            double zoomDelta = 0.1 * 3 * MCore::Clamp(m_targetTimeScale / 2.0, 1.0, 22.0);
            SetScale(m_targetTimeScale + zoomDelta);
            event->accept();
            return;
        }

        if (event->key() == Qt::Key_Minus)
        {
            double zoomDelta = 0.1 * 3 * MCore::Clamp(m_targetTimeScale / 2.0, 1.0, 22.0);
            SetScale(m_targetTimeScale - zoomDelta);
            event->accept();
            return;
        }

        if (EMotionFX::GetRecorder().GetIsRecording() == false && EMotionFX::GetRecorder().GetIsInAutoPlayMode() == false)
        {
            if (event->key() == Qt::Key_Left)
            {
                m_targetScrollX -= (m_pixelsPerSecond * 3) / m_timeScale;
                if (m_targetScrollX < 0)
                {
                    m_targetScrollX = 0;
                }
                event->accept();
                return;
            }

            if (event->key() == Qt::Key_Right)
            {
                const double newTime = (m_scrollX + ((m_pixelsPerSecond * 3) / m_timeScale)) / m_pixelsPerSecond;
                if (newTime < m_maxTime)
                {
                    m_targetScrollX += ((m_pixelsPerSecond * 3) / m_timeScale);
                }

                event->accept();
                return;
            }

            // fit the contents
            if (event->key() == Qt::Key_A)
            {
                OnZoomAll();
                event->accept();
                return;
            }

            if (event->key() == Qt::Key_Z)
            {
                OnCenterOnCurTime();
                event->accept();
                return;
            }

            if (event->key() == Qt::Key_Home)
            {
                OnGotoTimeZero();
                event->accept();
                return;
            }

            if (event->key() == Qt::Key_PageUp)
            {
                m_targetScrollX -= m_trackDataWidget->geometry().width() / m_timeScale;
                if (m_targetScrollX < 0)
                {
                    m_targetScrollX = 0;
                }
                event->accept();
                return;
            }

            if (event->key() == Qt::Key_PageDown)
            {
                const double newTime = (m_scrollX + (m_trackDataWidget->geometry().width() / m_timeScale)) / m_pixelsPerSecond;
                if (newTime < m_maxTime)
                {
                    m_targetScrollX += m_trackDataWidget->geometry().width() / m_timeScale;
                }

                event->accept();
                return;
            }
        }


        event->ignore();
    }


    void TimeViewPlugin::OnKeyReleaseEvent(QKeyEvent* event)
    {
        if (event->key() == Qt::Key_Delete)
        {
            event->accept();
            return;
        }

        event->ignore();
    }


    void TimeViewPlugin::ValidatePluginLinks()
    {
        m_motionEventsPlugin = nullptr;
        m_motionSetPlugin = nullptr;

        EMStudio::PluginManager* pluginManager = EMStudio::GetPluginManager();

        EMStudioPlugin* motionSetBasePlugin = pluginManager->FindActivePlugin(MotionSetsWindowPlugin::CLASS_ID);
        if (motionSetBasePlugin)
        {
            m_motionSetPlugin = static_cast<MotionSetsWindowPlugin*>(motionSetBasePlugin);
            connect(m_motionSetPlugin->GetMotionSetWindow(), &MotionSetWindow::MotionSelectionChanged, this, &TimeViewPlugin::MotionSelectionChanged, Qt::UniqueConnection); // UniqueConnection as we could connect multiple times.
        }
    }


    void TimeViewPlugin::MotionSelectionChanged()
    {
        ValidatePluginLinks();
        if (m_motionSetPlugin && m_motionSetPlugin->GetMotionSetWindow() &&
            m_motionSetPlugin->GetMotionSetWindow()->isVisible())
        {
            SetMode(TimeViewMode::Motion);
        }
    }


    void TimeViewPlugin::UpdateSelection()
    {
        m_selectedEvents.clear();
        if (!m_motion)
        {
            return;
        }

        // get the motion event table
        const EMotionFX::MotionEventTable* eventTable = m_motion->GetEventTable();

        // get the number of tracks in the time view and iterate through them
        const size_t numTracks = GetNumTracks();
        for (size_t trackIndex = 0; trackIndex < numTracks; ++trackIndex)
        {
            // get the current time view track
            const TimeTrack* track = GetTrack(trackIndex);
            if (track->GetIsVisible() == false)
            {
                continue;
            }

            const AZ::Outcome<size_t> trackNr = eventTable->FindTrackIndexByName(track->GetName());
            if (!trackNr.IsSuccess())
            {
                continue;
            }

            // get the number of elements in the track and iterate through them
            const size_t numTrackElements = track->GetNumElements();
            for (size_t elementIndex = 0; elementIndex < numTrackElements; ++elementIndex)
            {
                TimeTrackElement* element = track->GetElement(elementIndex);
                if (element->GetIsVisible() == false)
                {
                    continue;
                }

                if (element->GetIsSelected())
                {
                    EventSelectionItem selectionItem;
                    selectionItem.m_motion = m_motion;
                    selectionItem.m_trackNr = trackNr.GetValue();
                    selectionItem.m_eventNr = element->GetElementNumber();
                    m_selectedEvents.emplace_back(selectionItem);
                }
            }
        }
    }


    void TimeViewPlugin::ReInit()
    {
        if (EMotionFX::GetMotionManager().FindMotionIndex(m_motion) == InvalidIndex)
        {
            // set the motion first back to nullptr
            m_motion = nullptr;
        }

        // update the selection and save it
        UpdateSelection();

        ValidatePluginLinks();

        // If we are in anim graph mode and have a recording, don't init for the motions.
        if ((m_mode == TimeViewMode::AnimGraph) &&
            (EMotionFX::GetRecorder().GetIsRecording() || EMotionFX::GetRecorder().GetRecordTime() > MCore::Math::epsilon || EMotionFX::GetRecorder().GetIsInPlayMode()))
        {
            SetScrollX(0);
            m_trackHeaderWidget->ReInit();
            return;
        }

        if (m_motion)
        {
            size_t trackIndex;
            AZStd::string text;

            const EMotionFX::MotionEventTable* eventTable = m_motion->GetEventTable();

            RemoveAllTracks();

            // get the number of motion event tracks and iterate through them
            const size_t numEventTracks = eventTable->GetNumTracks();
            for (trackIndex = 0; trackIndex < numEventTracks; ++trackIndex)
            {
                const EMotionFX::MotionEventTrack* eventTrack = eventTable->GetTrack(trackIndex);

                TimeTrack* timeTrack = new TimeTrack(this);
                AddTrack(timeTrack);

                timeTrack->SetName(eventTrack->GetName());
                timeTrack->SetIsEnabled(eventTrack->GetIsEnabled());
                timeTrack->SetIsVisible(true);
                timeTrack->SetIsDeletable(eventTrack->GetIsDeletable());

                // get the number of motion events and iterate through them
                size_t eventIndex;
                const size_t numMotionEvents = eventTrack->GetNumEvents();
                if (numMotionEvents == 0)
                {
                    timeTrack->RemoveAllElements();
                }
                else
                {
                    for (eventIndex = 0; eventIndex < numMotionEvents; ++eventIndex)
                    {
                        const EMotionFX::MotionEvent& motionEvent = eventTrack->GetEvent(eventIndex);

                        TimeTrackElement* element = nullptr;
                        if (eventIndex < timeTrack->GetNumElements())
                        {
                            element = timeTrack->GetElement(eventIndex);
                        }
                        else
                        {
                            element = new TimeTrackElement("", timeTrack);
                            timeTrack->AddElement(element);
                        }

                        // Select the element if in m_selectedEvents.
                        for (const EventSelectionItem& selectionItem : m_selectedEvents)
                        {
                            if (m_motion != selectionItem.m_motion)
                            {
                                continue;
                            }
                            if (selectionItem.m_trackNr == trackIndex && selectionItem.m_eventNr == eventIndex)
                            {
                                element->SetIsSelected(true);
                                break;
                            }
                        }

                        text = '{';
                        AZStd::string delimiter;
                        const EMotionFX::EventDataSet& eventDatas = motionEvent.GetEventDatas();
                        for (const EMotionFX::EventDataPtr& data : eventDatas)
                        {
                            if (data)
                            {
                                text += delimiter + data->ToString() + "}";
                            }
                            else
                            {
                                text += "<null>}";
                            }
                            delimiter = ", {";
                        }
                        QColor qColor = AzQtComponents::toQColor(GetEventPresetManager()->GetEventColor(motionEvent.GetEventDatas()));

                        element->SetIsVisible(true);
                        element->SetName(text.c_str());
                        element->SetColor(qColor);
                        element->SetElementNumber(eventIndex);
                        element->SetStartTime(motionEvent.GetStartTime());
                        element->SetEndTime(motionEvent.GetEndTime());

                        // tooltip
                        AZStd::string tooltip;
                        AZStd::string rowName;
                        tooltip.reserve(16384);

                        tooltip = "<table border=\"0\">";

                        if (motionEvent.GetIsTickEvent())
                        {
                            // time
                            rowName = "Time";
                            tooltip += AZStd::string::format("<tr><td><p style=\"color:rgb(%i,%i,%i)\"><b>%s:&nbsp;</b></p></td>", qColor.red(), qColor.green(), qColor.blue(), rowName.c_str());
                            tooltip += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%.3f s</p></td></tr>", motionEvent.GetStartTime());
                        }
                        else // range event
                        {
                            // start time
                            rowName = "Start&nbsp;Time";
                            tooltip += AZStd::string::format("<tr><td><p style=\"color:rgb(%i,%i,%i)\"><b>%s:&nbsp;</b></p></td>", qColor.red(), qColor.green(), qColor.blue(), rowName.c_str());
                            tooltip += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%.3f s</p></td></tr>", motionEvent.GetStartTime());

                            // end time
                            rowName = "End&nbsp;Time";
                            tooltip += AZStd::string::format("<tr><td><p style=\"color:rgb(%i,%i,%i)\"><b>%s:&nbsp;</b></p></td>", qColor.red(), qColor.green(), qColor.blue(), rowName.c_str());
                            tooltip += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%.3f s</p></td></tr>", motionEvent.GetEndTime());
                        }

                        for (const EMotionFX::EventDataPtr& eventData : eventDatas)
                        {
                            if (!eventData.get())
                            {
                                continue;
                            }
                            const auto& motionDataProperties = MCore::ReflectionSerializer::SerializeIntoMap(eventData.get());
                            if (motionDataProperties.IsSuccess())
                            {
                                for (const AZStd::pair<AZStd::string, AZStd::string>& keyValuePair : motionDataProperties.GetValue())
                                {
                                    tooltip += AZStd::string::format(
                                            "<tr><td><p style=\"color:rgb(%i,%i,%i)\"><b>%s:&nbsp;</b></p></td>" // no comma, concat these 2 string literals
                                            "<td><p style=\"color:rgb(115, 115, 115)\">%s</p></td></tr>",
                                            qColor.red(),
                                            qColor.green(),
                                            qColor.blue(),
                                            keyValuePair.first.c_str(),
                                            keyValuePair.second.c_str()
                                            );
                                }
                            }
                        }

                        tooltip += "</table>";

                        // set tooltip
                        element->SetToolTip(tooltip.c_str());
                    }
                }
                timeTrack->SetElementCount(numMotionEvents);
            }
        }
        else // m_motion == nullptr
        {
            const size_t numEventTracks = GetNumTracks();
            for (size_t trackIndex = 0; trackIndex < numEventTracks; ++trackIndex)
            {
                TimeTrack* timeTrack = GetTrack(trackIndex);
                timeTrack->SetIsVisible(false);

                const size_t numMotionEvents = timeTrack->GetNumElements();
                for (size_t j = 0; j < numMotionEvents; ++j)
                {
                    TimeTrackElement* element = timeTrack->GetElement(j);
                    element->SetIsVisible(false);
                }
            }

            m_motionEventWidget->ReInit();
        }

        // update the time view plugin
        m_trackHeaderWidget->ReInit();

        if (m_motion)
        {
            MotionInfo* motionInfo = FindMotionInfo(m_motion->GetID());

            // if we already selected before, set the remembered settings
            if (motionInfo->m_initialized)
            {
                const int32 tempScroll = aznumeric_cast<int32>(motionInfo->m_scrollX);
                SetScale(motionInfo->m_scale);
                SetScrollX(tempScroll);
            }
            else
            {
                // selected the animation the first time
                motionInfo->m_initialized = true;
                m_targetTimeScale        = CalcFitScale(m_minScale, m_maxScale) * 0.8;
                motionInfo->m_scale      = m_targetTimeScale;
                motionInfo->m_scrollX    = 0.0;
            }
        }

        UpdateVisualData();
    }


    // find the motion info for the given motion id
    TimeViewPlugin::MotionInfo* TimeViewPlugin::FindMotionInfo(uint32 motionID)
    {
        const auto foundMotionInfo = AZStd::find_if(begin(m_motionInfos), end(m_motionInfos), [motionID](const MotionInfo* motionInfo)
        {
            return motionInfo->m_motionId == motionID;
        });
        if (foundMotionInfo != end(m_motionInfos))
        {
            return *foundMotionInfo;
        }

        // we haven't found a motion info for the given id yet, so create a new one
        MotionInfo* motionInfo  = new MotionInfo();
        motionInfo->m_motionId   = motionID;
        motionInfo->m_initialized = false;
        m_motionInfos.emplace_back(motionInfo);
        return motionInfo;
    }


    void TimeViewPlugin::Select(const AZStd::vector<EventSelectionItem>& selection)
    {
        m_selectedEvents = selection;

        // get the number of tracks in the time view and iterate through them
        const size_t numTracks = GetNumTracks();
        for (size_t i = 0; i < numTracks; ++i)
        {
            TimeTrack* track = GetTrack(i);

            // get the number of elements in the track and iterate through them
            const size_t numTrackElements = track->GetNumElements();
            for (size_t j = 0; j < numTrackElements; ++j)
            {
                TimeTrackElement* element = track->GetElement(j);
                element->SetIsSelected(false);
            }
        }

        for (const EventSelectionItem& selectionItem : selection)
        {
            TimeTrack*                  track           = GetTrack(selectionItem.m_trackNr);
            TimeTrackElement*           element         = track->GetElement(selectionItem.m_eventNr);

            element->SetIsSelected(true);
        }
    }



    EMotionFX::MotionEvent* EventSelectionItem::GetMotionEvent()
    {
        EMotionFX::MotionEventTrack* eventTrack = GetEventTrack();
        if (eventTrack == nullptr)
        {
            return nullptr;
        }

        if (m_eventNr >= eventTrack->GetNumEvents())
        {
            return nullptr;
        }

        return &(eventTrack->GetEvent(m_eventNr));
    }


    EMotionFX::MotionEventTrack* EventSelectionItem::GetEventTrack()
    {
        if (m_trackNr >= m_motion->GetEventTable()->GetNumTracks())
        {
            return nullptr;
        }

        EMotionFX::MotionEventTrack* eventTrack = m_motion->GetEventTable()->GetTrack(m_trackNr);
        return eventTrack;
    }



    void TimeViewPlugin::AddMotionEvent(int32 x, int32 y)
    {
        if (m_motion == nullptr)
        {
            return;
        }

        SetRedrawFlag();

        // calculate the start time for the motion event
        double dropTimeInSeconds = PixelToTime(x);
        //CalcTime( x, &dropTimeInSeconds, nullptr, nullptr, nullptr, nullptr );

        // get the time track on which we dropped the preset
        TimeTrack* timeTrack = GetTrackAt(y);
        if (timeTrack == nullptr)
        {
            return;
        }

        CommandSystem::CommandHelperAddMotionEvent(timeTrack->GetName(), aznumeric_cast<float>(dropTimeInSeconds), aznumeric_cast<float>(dropTimeInSeconds));
    }


    void TimeViewPlugin::RemoveMotionEvent(int32 x, int32 y)
    {
        // get the time track on which we dropped the preset
        TimeTrack* timeTrack = GetTrackAt(y);
        if (timeTrack == nullptr)
        {
            return;
        }

        // get the time track on which we dropped the preset
        TimeTrackElement* element = GetElementAt(x, y);
        if (element == nullptr)
        {
            return;
        }

        CommandSystem::CommandHelperRemoveMotionEvent(timeTrack->GetName(), element->GetElementNumber());
    }


    void TimeViewPlugin::MotionEventChanged(TimeTrackElement* element, double startTime, double endTime)
    {
        AZ::Locale::ScopedSerializationLocale scopedLocale; // Ensures that %f uses "." as decimal separator

        if (element == nullptr)
        {
            return;
        }

        // get the motion event number by getting the time track element number
        size_t motionEventNr = element->GetElementNumber();
        if (motionEventNr == InvalidIndex)
        {
            return;
        }

        // find the corresponding time track for the element
        TimeTrack* timeTrack = FindTrackByElement(element);
        if (timeTrack == nullptr)
        {
            return;
        }

        // get the corresponding motion event track
        EMotionFX::MotionEventTable* eventTable = m_motion->GetEventTable();
        EMotionFX::MotionEventTrack* eventTrack = eventTable->FindTrackByName(timeTrack->GetName());
        if (eventTrack == nullptr)
        {
            return;
        }

        // check if the motion event number is valid and return in case it is not
        if (motionEventNr >= eventTrack->GetNumEvents())
        {
            return;
        }

        // adjust the motion event
        AZStd::string outResult, command;
        command = AZStd::string::format("AdjustMotionEvent -motionID %i -eventTrackName \"%s\" -eventNr %zu -startTime %f -endTime %f", m_motion->GetID(), eventTrack->GetName(), motionEventNr, startTime, endTime);
        if (EMStudio::GetCommandManager()->ExecuteCommand(command.c_str(), outResult) == false)
        {
            MCore::LogError(outResult.c_str());
        }
    }


    void TimeViewPlugin::RemoveSelectedMotionEvents()
    {
        // create the command group
        AZStd::string result;
        MCore::CommandGroup commandGroup("Remove motion events");

        if (m_trackDataWidget)
        {
            m_trackDataWidget->ClearState();
        }

        if (m_motion == nullptr)
        {
            return;
        }

        if (EMotionFX::GetMotionManager().FindMotionIndex(m_motion) == InvalidIndex)
        {
            return;
        }

        // get the motion event table
        //  MotionEventTable& eventTable = m_motion->GetEventTable();
        AZStd::vector<size_t> eventNumbers;

        // get the number of tracks in the time view and iterate through them
        const size_t numTracks = GetNumTracks();
        for (size_t i = 0; i < numTracks; ++i)
        {
            // get the current time view track
            TimeTrack* track = GetTrack(i);
            if (track->GetIsVisible() == false)
            {
                continue;
            }

            eventNumbers.clear();

            // get the number of elements in the track and iterate through them
            const size_t numTrackElements = track->GetNumElements();
            for (size_t j = 0; j < numTrackElements; ++j)
            {
                TimeTrackElement* element = track->GetElement(j);

                if (element->GetIsSelected() && element->GetIsVisible())
                {
                    eventNumbers.emplace_back(j);
                }
            }

            // remove selected motion events
            CommandSystem::CommandHelperRemoveMotionEvents(track->GetName(), eventNumbers, &commandGroup);
        }

        // execute the command group
        if (EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result) == false)
        {
            MCore::LogError(result.c_str());
        }

        UnselectAllElements();
    }


    void TimeViewPlugin::RemoveAllMotionEvents()
    {
        // create the command group
        AZStd::string result;
        MCore::CommandGroup commandGroup("Remove motion events");

        if (m_motion == nullptr)
        {
            return;
        }

        if (EMotionFX::GetMotionManager().FindMotionIndex(m_motion) == InvalidIndex)
        {
            return;
        }

        // get the motion event table
        AZStd::vector<size_t> eventNumbers;

        // get the number of tracks in the time view and iterate through them
        const size_t numTracks = GetNumTracks();
        for (size_t i = 0; i < numTracks; ++i)
        {
            // get the current time view track
            TimeTrack* track = GetTrack(i);
            if (track->GetIsVisible() == false)
            {
                continue;
            }

            eventNumbers.clear();

            // get the number of elements in the track and iterate through them
            const size_t numTrackElements = track->GetNumElements();
            for (size_t j = 0; j < numTrackElements; ++j)
            {
                TimeTrackElement* element = track->GetElement(j);
                if (element->GetIsVisible())
                {
                    eventNumbers.emplace_back(j);
                }
            }

            // remove selected motion events
            CommandSystem::CommandHelperRemoveMotionEvents(track->GetName(), eventNumbers, &commandGroup);
        }

        // execute the command group
        if (EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result) == false)
        {
            MCore::LogError(result.c_str());
        }

        UnselectAllElements();
    }


    // get the data times
    void TimeViewPlugin::GetDataTimes(double* outMaxTime, double* outClipStart, double* outClipEnd) const
    {
        if (outMaxTime)
        {
            *outMaxTime     = 0.0;
        }
        if (outClipStart)
        {
            *outClipStart   = 0.0;
        }
        if (outClipEnd)
        {
            *outClipEnd     = 0.0;
        }

        switch (m_mode)
        {
            case TimeViewMode::Motion:
            {
                EMotionFX::Motion* motion = GetMotion();
                if (motion)
                {
                    EMotionFX::PlayBackInfo* playbackInfo = motion->GetDefaultPlayBackInfo();

                    if (outClipStart)
                    {
                        *outClipStart = playbackInfo->m_clipStartTime;
                    }
                    if (outClipEnd)
                    {
                        *outClipEnd = playbackInfo->m_clipEndTime;
                    }
                    if (outMaxTime)
                    {
                        *outMaxTime = motion->GetDuration();
                    }
                }

                break;
            }
            case TimeViewMode::AnimGraph:
            {
                EMotionFX::Recorder& recorder = EMotionFX::GetRecorder();
                if (recorder.GetRecordTime() > MCore::Math::epsilon)
                {
                    if (outClipEnd)
                    {
                        *outClipEnd = recorder.GetRecordTime();
                    }
                    if (outMaxTime)
                    {
                        *outMaxTime = recorder.GetRecordTime();
                    }
                }

                break;
            }
        }
    }


    // zoom to fit
    void TimeViewPlugin::ZoomToFit()
    {
        m_targetScrollX      = 0.0;
        m_targetTimeScale    = CalcFitScale(m_minScale, m_maxScale);
    }


    // calculate the scale needed to fit exactly
    double TimeViewPlugin::CalcFitScale(double minScale, double maxScale) const
    {
        // get the duration
        double maxTime;
        GetDataTimes(&maxTime, nullptr, nullptr);

        // calculate the right scale to fit it
        double scale = 1.0;
        if (maxTime > 0.0)
        {
            double width    = m_trackDataWidget->geometry().width();
            scale   = (width / m_pixelsPerSecond) / maxTime;
        }

        if (scale < minScale)
        {
            scale = minScale;
        }

        if (scale > maxScale)
        {
            scale = maxScale;
        }

        return scale;
    }


    // is the given time value visible?
    bool TimeViewPlugin::GetIsTimeVisible(double timeValue) const
    {
        const double pixel = TimeToPixel(timeValue);
        return (pixel >= 0.0 && pixel < m_trackDataWidget->geometry().width());
    }


    // make a given time value visible
    void TimeViewPlugin::MakeTimeVisible(double timeValue, double offsetFactor, bool animate)
    {
        SetRedrawFlag();

        const double pixel = TimeToPixel(timeValue, false);

        // if we need to scroll to the right
        double width = m_trackDataWidget->geometry().width() / m_timeScale;
        m_targetScrollX += (pixel - width) + width * (1.0 - offsetFactor);

        if (m_targetScrollX < 0)
        {
            m_targetScrollX = 0;
        }

        if (animate == false)
        {
            m_scrollX = m_targetScrollX;
        }
    }


    // update the maximum height
    void TimeViewPlugin::UpdateMaxHeight()
    {
        m_maxHeight = 0.0;

        // find the selected actor instance
        EMotionFX::Recorder& recorder = EMotionFX::GetRecorder();
        if (recorder.GetRecordTime() > MCore::Math::epsilon)
        {
            EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
            if (actorInstance)
            {
                // find the actor instance data for this actor instance
                const size_t actorInstanceDataIndex = recorder.FindActorInstanceDataIndex(actorInstance);
                if (actorInstanceDataIndex != InvalidIndex)
                {
                    RecorderGroup* recorderGroup = m_timeViewToolBar->GetRecorderGroup();
                    const bool displayNodeActivity = recorderGroup->GetDisplayNodeActivity();
                    const bool displayEvents = recorderGroup->GetDisplayMotionEvents();
                    const bool displayRelativeGraph = recorderGroup->GetDisplayRelativeGraph();
                    bool isTop = true;

                    const EMotionFX::Recorder::ActorInstanceData& actorInstanceData = recorder.GetActorInstanceData(actorInstanceDataIndex);
                    if (displayNodeActivity)
                    {
                        m_maxHeight += ((recorder.CalcMaxNodeHistoryTrackIndex(actorInstanceData) + 1) * (m_trackDataWidget->m_nodeHistoryItemHeight + 3));
                        isTop = false;
                    }

                    if (displayEvents)
                    {
                        if (isTop == false)
                        {
                            m_maxHeight += 10 + 10;
                        }

                        isTop = false;
                        m_maxHeight += m_trackDataWidget->m_eventHistoryTotalHeight;
                    }

                    if (displayRelativeGraph)
                    {
                        if (isTop == false)
                        {
                            m_maxHeight += 10;
                        }

                        isTop = false;
                    }
                }
            }
        }
        else
        {
            if (m_motion)
            {
                for (const TimeTrack* track : m_tracks)
                {
                     if (track->GetIsVisible() == false)
                    {
                        continue;
                    }

                    m_maxHeight += track->GetHeight();
                    m_maxHeight += 1;
                }
            }
        }
    }


    // zoom all
    void TimeViewPlugin::OnZoomAll()
    {
        ZoomToFit();
    }


    // goto time zero
    void TimeViewPlugin::OnGotoTimeZero()
    {
        m_targetScrollX = 0;
    }


    // reset timeline
    void TimeViewPlugin::OnResetTimeline()
    {
        m_targetScrollX      = 0;
        m_targetTimeScale    = 1.0;
    }


    // center on current time
    void TimeViewPlugin::OnCenterOnCurTime()
    {
        MakeTimeVisible(m_curTime, 0.5);
    }


    // center on current time
    void TimeViewPlugin::OnShowNodeHistoryNodeInGraph()
    {
        if (m_nodeHistoryItem && m_actorInstanceData)
        {
            emit DoubleClickedRecorderNodeHistoryItem(m_actorInstanceData, m_nodeHistoryItem);
        }
    }


    // center on current time
    void TimeViewPlugin::OnClickNodeHistoryNode()
    {
        if (m_nodeHistoryItem && m_actorInstanceData)
        {
            emit ClickedRecorderNodeHistoryItem(m_actorInstanceData, m_nodeHistoryItem);
        }
    }


    // zooming on rect
    void TimeViewPlugin::ZoomRect(const QRect& rect)
    {
        m_targetScrollX      = m_scrollX + (rect.left() / m_timeScale);
        m_targetTimeScale    = m_trackDataWidget->geometry().width() / (double)(rect.width() / m_timeScale);

        if (m_targetTimeScale < 1.0)
        {
            m_targetTimeScale = 1.0;
        }

        if (m_targetTimeScale > m_maxScale)
        {
            m_targetTimeScale = m_maxScale;
        }
    }


    // callbacks
    bool ReInitTimeViewPlugin()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(TimeViewPlugin::CLASS_ID);
        if (plugin == nullptr)
        {
            return false;
        }

        TimeViewPlugin* timeViewPlugin = (TimeViewPlugin*)plugin;
        timeViewPlugin->ReInit();

        return true;
    }


    bool MotionSelectionChangedTimeViewPlugin()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(TimeViewPlugin::CLASS_ID);
        if (plugin == nullptr)
        {
            return false;
        }

        TimeViewPlugin* timeViewPlugin = (TimeViewPlugin*)plugin;
        timeViewPlugin->MotionSelectionChanged();

        return true;
    }

    bool UpdateInterfaceTimeViewPlugin()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(TimeViewPlugin::CLASS_ID);
        if (plugin == nullptr)
        {
            return false;
        }

        static_cast<TimeViewPlugin*>(plugin)->GetTimeViewToolBar()->UpdateInterface();

        return true;
    }

    bool TimeViewPlugin::CommandAdjustMotionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)       { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitTimeViewPlugin(); }
    bool TimeViewPlugin::CommandAdjustMotionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)          { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitTimeViewPlugin(); }
    bool TimeViewPlugin::CommandSelectCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasMotionSelectionParameter(commandLine) == false)
        {
            UpdateInterfaceTimeViewPlugin();
            return true;
        }
        return MotionSelectionChangedTimeViewPlugin();
    }
    bool TimeViewPlugin::CommandSelectCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasMotionSelectionParameter(commandLine) == false)
        {
            UpdateInterfaceTimeViewPlugin();
            return true;
        }
        return MotionSelectionChangedTimeViewPlugin();
    }
    bool TimeViewPlugin::CommandUnselectCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasMotionSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return MotionSelectionChangedTimeViewPlugin();
    }
    bool TimeViewPlugin::CommandUnselectCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasMotionSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return MotionSelectionChangedTimeViewPlugin();
    }
    bool TimeViewPlugin::CommandClearSelectionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)     { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return MotionSelectionChangedTimeViewPlugin(); }
    bool TimeViewPlugin::CommandClearSelectionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)        { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return MotionSelectionChangedTimeViewPlugin(); }

    bool TimeViewPlugin::CommandRecorderClearCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)     { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitTimeViewPlugin(); }
    bool TimeViewPlugin::CommandRecorderClearCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)        { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitTimeViewPlugin(); }

    bool TimeViewPlugin::UpdateInterfaceCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine) { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateInterfaceTimeViewPlugin(); }
    bool TimeViewPlugin::UpdateInterfaceCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine) { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateInterfaceTimeViewPlugin(); }

    // calculate the content heights
    uint32 TimeViewPlugin::CalcContentHeight() const
    {
        RecorderGroup* recorderGroup = m_timeViewToolBar->GetRecorderGroup();
        const bool displayNodeActivity = recorderGroup->GetDisplayNodeActivity();
        const bool displayEvents = recorderGroup->GetDisplayMotionEvents();
        const bool displayRelativeGraph = recorderGroup->GetDisplayRelativeGraph();

        uint32 result = 0;
        if (displayNodeActivity)
        {
            result += m_trackDataWidget->m_nodeHistoryRect.bottom();
        }

        if (displayEvents)
        {
            result += m_trackDataWidget->m_eventHistoryTotalHeight;
        }

        if (displayRelativeGraph)
        {
        }

        //  MCore::LogInfo("maxHeight = %d", result);
        return result;
    }

    void TimeViewPlugin::OnManualTimeChange(float timeValue)
    {
        MCORE_UNUSED(timeValue);
        GetMainWindow()->OnUpdateRenderPlugins();
    }

    void TimeViewPlugin::toggleMotionEventPresetsPane()
    {
        QList<int> sizes = m_paneSplitter->sizes();
        if (sizes[1] == 0)
        {
            sizes[1] = m_motionEventsPresetsWidget->minimumSizeHint().width();
            sizes[0] = m_paneSplitter->width() - sizes[1];
        }
        else
        {
            // hide the nav pane
            sizes[0] = m_paneSplitter->width();
            sizes[1] = 0;
        }
        m_paneSplitter->setSizes(sizes);
        m_togglePresetsView->setChecked(sizes[1] != 0);
    }

    void TimeViewPlugin::OnFocusIn()
    {
        SetMode(TimeViewMode::AnimGraph);
    }

    void TimeViewPlugin::OnShow()
    {
        SetMode(TimeViewMode::AnimGraph);
    }

    void TimeViewPlugin::SetMode(TimeViewMode mode)
    {
        const bool modeChanged = (m_mode != mode);
        m_mode = mode;

        switch (mode)
        {
            case TimeViewMode::Motion:
            {
                EMotionFX::Motion* motion = GetCommandManager()->GetCurrentSelection().GetSingleMotion();
                if ((m_motion != motion) || modeChanged)
                {
                    m_motion = motion;
                    ReInit();
                }

                if (m_trackHeaderWidget)
                {
                    m_trackHeaderWidget->GetAddTrackWidget()->setEnabled(motion != nullptr);
                }

                break;
            }

            default:
            {
                m_motion = nullptr;
                ReInit();
                OnZoomAll();
                SetCurrentTime(0.0f);
            }
        }

        m_timeViewToolBar->UpdateInterface();
    }

    bool TimeViewPlugin::CheckIfMotionEventPresetReadyToDrop()
    {
        // get the motion event presets table
        QTableWidget* eventPresetsTable = m_motionEventsPresetsWidget->GetMotionEventPresetsTable();
        if (eventPresetsTable == nullptr)
        {
            return false;
        }

        // get the number of motion event presets and iterate through them
        const uint32 numRows = eventPresetsTable->rowCount();
        for (uint32 i = 0; i < numRows; ++i)
        {
            QTableWidgetItem* itemType = eventPresetsTable->item(i, 1);

            if (itemType->isSelected())
            {
                return true;
            }
        }

        return false;
    }
    void TimeViewPlugin::OnEventPresetDroppedOnTrackData(QPoint mousePoint)
    {
        EMotionFX::Motion* motion = GetCommandManager()->GetCurrentSelection().GetSingleMotion();
        if (!motion)
        {
            return;
        }

        // calculate the start time for the motion event
        double dropTimeInSeconds = PixelToTime(mousePoint.x());

        // get the time track on which we dropped the preset
        TimeTrack* timeTrack = GetTrackAt(mousePoint.y());
        if (!timeTrack)
        {
            return;
        }

        // get the corresponding motion event track
        EMotionFX::MotionEventTable* eventTable = motion->GetEventTable();
        EMotionFX::MotionEventTrack* eventTrack = eventTable->FindTrackByName(timeTrack->GetName());
        if (eventTrack == nullptr)
        {
            return;
        }

        // get the motion event presets table
        QTableWidget* eventPresetsTable = m_motionEventsPresetsWidget->GetMotionEventPresetsTable();
        if (eventPresetsTable == nullptr)
        {
            return;
        }

        // get the number of motion event presets and iterate through them
        const size_t numRows = EMStudio::GetEventPresetManager()->GetNumPresets();
        AZStd::string result;
        for (size_t i = 0; i < numRows; ++i)
        {
            const MotionEventPreset* preset = EMStudio::GetEventPresetManager()->GetPreset(i);
            QTableWidgetItem* itemName = eventPresetsTable->item(static_cast<int>(i), 1);

            if (itemName->isSelected())
            {
                CommandSystem::CommandCreateMotionEvent* createMotionEventCommand = aznew CommandSystem::CommandCreateMotionEvent();
                createMotionEventCommand->SetMotionID(motion->GetID());
                createMotionEventCommand->SetEventTrackName(eventTrack->GetName());
                createMotionEventCommand->SetStartTime(aznumeric_cast<float>(dropTimeInSeconds));
                createMotionEventCommand->SetEndTime(aznumeric_cast<float>(dropTimeInSeconds));
                createMotionEventCommand->SetEventDatas(preset->GetEventDatas());

                if (!EMStudio::GetCommandManager()->ExecuteCommand(createMotionEventCommand, result))
                {
                    AZ_Error("EMotionFX", false, result.c_str());
                }
            }
        }
    }
} // namespace EMStudio
