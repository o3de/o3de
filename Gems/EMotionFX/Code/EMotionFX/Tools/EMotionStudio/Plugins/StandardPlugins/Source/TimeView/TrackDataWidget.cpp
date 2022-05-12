/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/vector.h>

#include <EMotionStudio/Plugins/StandardPlugins/Source/TimeView/TrackDataWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/TimeView/TrackHeaderWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/TimeView/TimeInfoWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/TimeView/TimeViewPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/TimeView/TimeViewShared.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/TimeView/TimeViewToolBar.h>

#include <QCheckBox>
#include <QComboBox>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QToolTip>

#include <MCore/Source/LogManager.h>
#include <MCore/Source/Algorithms.h>

#include <MysticQt/Source/MysticQtManager.h>

#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>

#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionData/MotionData.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/MotionEvent.h>
#include <EMotionFX/Source/MotionEventTrack.h>
#include <EMotionFX/Source/Recorder.h>
#include <EMotionFX/Source/MotionEventTable.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/CommandSystem/Source/MotionEventCommands.h>
#include "../MotionWindow/MotionWindowPlugin.h"
#include "../MotionEvents/MotionEventsPlugin.h"
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include "../../../../EMStudioSDK/Source/MainWindow.h"

namespace EMStudio
{
    // the constructor
    TrackDataWidget::TrackDataWidget(TimeViewPlugin* plugin, QWidget* parent)
        : QOpenGLWidget(parent)
        , QOpenGLFunctions()
        , m_brushBackground(QColor(40, 45, 50), Qt::SolidPattern)
        , m_brushBackgroundClipped(QColor(40, 40, 40), Qt::SolidPattern)
        , m_brushBackgroundOutOfRange(QColor(35, 35, 35), Qt::SolidPattern)
        , m_plugin(plugin)
        , m_mouseLeftClicked(false)
        , m_mouseMidClicked(false)
        , m_mouseRightClicked(false)
        , m_dragging(false)
        , m_resizing(false)
        , m_rectZooming(false)
        , m_isScrolling(false)
        , m_lastLeftClickedX(0)
        , m_lastMouseMoveX(0)
        , m_lastMouseX(0)
        , m_lastMouseY(0)
        , m_nodeHistoryItemHeight(20)
        , m_eventHistoryTotalHeight(0)
        , m_allowContextMenu(true)
        , m_draggingElement(nullptr)
        , m_dragElementTrack(nullptr)
        , m_resizeElement(nullptr)
        , m_graphStartHeight(0)
        , m_eventsStartHeight(0)
        , m_nodeRectsStartHeight(0)
        , m_selectStart(0, 0)
        , m_selectEnd(0, 0)
        , m_rectSelecting(false)
    {
        setObjectName("TrackDataWidget");

        m_dataFont.setPixelSize(13);

        setMouseTracking(true);
        setAcceptDrops(true);
        setAutoFillBackground(false);

        setFocusPolicy(Qt::StrongFocus);
    }


    // destructor
    TrackDataWidget::~TrackDataWidget()
    {
    }


    // init gl
    void TrackDataWidget::initializeGL()
    {
        initializeOpenGLFunctions();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    }


    // resize
    void TrackDataWidget::resizeGL(int w, int h)
    {
        MCORE_UNUSED(w);
        MCORE_UNUSED(h);
        if (m_plugin)
        {
            m_plugin->SetRedrawFlag();
        }
    }


    // calculate the selection rect
    void TrackDataWidget::CalcSelectRect(QRect& outRect)
    {
        const int32 startX = MCore::Min<int32>(m_selectStart.x(), m_selectEnd.x());
        const int32 startY = MCore::Min<int32>(m_selectStart.y(), m_selectEnd.y());
        const int32 width  = abs(m_selectEnd.x() - m_selectStart.x());
        const int32 height = abs(m_selectEnd.y() - m_selectStart.y());

        outRect = QRect(startX, startY, width, height);
    }


    void TrackDataWidget::paintGL()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // start painting
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, false);

        QRect rect(0, 0, geometry().width(), geometry().height());

        // draw a background rect
        painter.setPen(Qt::NoPen);
        painter.setBrush(m_brushBackgroundOutOfRange);
        painter.drawRect(rect);
        painter.setFont(m_dataFont);

        // if there is a recording show that, otherwise show motion tracks
        switch (m_plugin->GetMode())
        {
            case TimeViewMode::AnimGraph:
            {
                PaintRecorder(painter, rect);
                break;
            }
            case TimeViewMode::Motion:
            {
                PaintMotionTracks(painter, rect);
                break;
            }
            default:
            {
                break;
            }
        }

        painter.setRenderHint(QPainter::Antialiasing, false);

        m_plugin->RenderElementTimeHandles(painter, geometry().height(), m_plugin->m_penTimeHandles);

        DrawTimeMarker(painter, rect);

        // render selection rect
        if (m_rectSelecting)
        {
            painter.resetTransform();
            QRect selectRect;
            CalcSelectRect(selectRect);

            if (m_rectZooming)
            {
                painter.setBrush(QColor(0, 100, 200, 75));
                painter.setPen(QColor(0, 100, 255));
                painter.drawRect(selectRect);
            }
            else
            {
                if (EMotionFX::GetRecorder().GetRecordTime() < MCore::Math::epsilon && m_plugin->m_motion)
                {
                    painter.setBrush(QColor(200, 120, 0, 75));
                    painter.setPen(QColor(255, 128, 0));
                    painter.drawRect(selectRect);
                }
            }
        }
    }

    void TrackDataWidget::RemoveTrack(size_t trackIndex)
    {
        m_plugin->SetRedrawFlag();
        CommandSystem::CommandRemoveEventTrack(trackIndex);
        m_plugin->UnselectAllElements();
        ClearState();
    }

    // draw the time marker
    void TrackDataWidget::DrawTimeMarker(QPainter& painter, const QRect& rect)
    {
        // draw the current time marker
        float startHeight = 0.0f;
        const float curTimeX = aznumeric_cast<float>(m_plugin->TimeToPixel(m_plugin->m_curTime));
        painter.setPen(m_plugin->m_penCurTimeHandle);
        painter.drawLine(QPointF(curTimeX, startHeight), QPointF(curTimeX, rect.bottom()));
    }


    // paint the recorder data
    void TrackDataWidget::PaintRecorder(QPainter& painter, const QRect& rect)
    {
        painter.setRenderHint(QPainter::TextAntialiasing, true);

        EMotionFX::Recorder& recorder = EMotionFX::GetRecorder();

        QRect backgroundRect    = rect;
        QRect motionRect        = rect;

        const float animationLength = recorder.GetRecordTime();
        const double animEndPixel = m_plugin->TimeToPixel(animationLength);
        backgroundRect.setLeft(aznumeric_cast<int>(animEndPixel));
        motionRect.setRight(aznumeric_cast<int>(animEndPixel));
        motionRect.setTop(0);
        backgroundRect.setTop(0);

        // render the rects
        painter.setPen(Qt::NoPen);
        painter.setBrush(m_brushBackground);
        painter.drawRect(motionRect);
        painter.setBrush(m_brushBackgroundOutOfRange);
        painter.drawRect(backgroundRect);

        // find the selected actor instance
        EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
        if (actorInstance == nullptr)
        {
            return;
        }

        // find the actor instance data for this actor instance
        const size_t actorInstanceDataIndex = recorder.FindActorInstanceDataIndex(actorInstance);
        if (actorInstanceDataIndex == InvalidIndex) // it doesn't exist, so we didn't record anything for this actor instance
        {
            return;
        }

        // get the actor instance data for the first selected actor instance, and render the node history for that
        const EMotionFX::Recorder::ActorInstanceData* actorInstanceData = &recorder.GetActorInstanceData(actorInstanceDataIndex);

        RecorderGroup* recorderGroup = m_plugin->GetTimeViewToolBar()->GetRecorderGroup();
        const bool displayNodeActivity = recorderGroup->GetDisplayNodeActivity();
        const bool displayEvents = recorderGroup->GetDisplayMotionEvents();
        const bool displayRelativeGraph = recorderGroup->GetDisplayRelativeGraph();

        int32 startOffset = 0;
        int32 requiredHeight = 0;
        bool isTop = true;

        if (displayNodeActivity)
        {
            m_nodeRectsStartHeight = startOffset;
            PaintRecorderNodeHistory(painter, rect, actorInstanceData);
            isTop = false;
            startOffset = m_nodeHistoryRect.bottom();
            requiredHeight = m_nodeHistoryRect.bottom();
        }

        if (displayEvents)
        {
            if (isTop == false)
            {
                m_eventsStartHeight = startOffset;
                m_eventsStartHeight += PaintSeparator(painter, m_eventsStartHeight, animationLength);
                m_eventsStartHeight += 10;
                startOffset = m_eventsStartHeight;
                requiredHeight += 11;
            }
            else
            {
                startOffset += 3;
                m_eventsStartHeight = startOffset;
                requiredHeight += 3;
            }

            startOffset += m_eventHistoryTotalHeight;
            isTop = false;

            PaintRecorderEventHistory(painter, rect, actorInstanceData);
        }

        if (displayRelativeGraph)
        {
            if (isTop == false)
            {
                m_graphStartHeight = startOffset + 10;
                m_graphStartHeight += PaintSeparator(painter, m_graphStartHeight, animationLength);
                startOffset = m_graphStartHeight;
                requiredHeight += 11;
            }
            else
            {
                startOffset += 3;
                m_graphStartHeight = startOffset;
                requiredHeight += 3;
            }

            isTop = false;

            PaintRelativeGraph(painter, rect, actorInstanceData);
            requiredHeight += 200;
        }

        if (height() != requiredHeight)
        {
            QTimer::singleShot(0, this, [this, requiredHeight]() { OnRequiredHeightChanged(requiredHeight); });
        }
    }

    // paint the relative graph
    void TrackDataWidget::PaintRelativeGraph(QPainter& painter, const QRect& rect, const EMotionFX::Recorder::ActorInstanceData* actorInstanceData)
    {
        EMotionFX::Recorder& recorder = EMotionFX::GetRecorder();
        const double animationLength = recorder.GetRecordTime();
        if (animationLength < MCore::Math::epsilon)
        {
            return;
        }

        painter.setRenderHint(QPainter::Antialiasing, true);

        // get the history items shortcut
        const AZStd::vector<EMotionFX::Recorder::NodeHistoryItem*>& historyItems = actorInstanceData->m_nodeHistoryItems;
        int32 windowWidth = geometry().width();

        RecorderGroup* recorderGroup = m_plugin->GetTimeViewToolBar()->GetRecorderGroup();
        const bool useNodeColors = recorderGroup->GetUseNodeTypeColors();
        const bool limitGraphHeight = recorderGroup->GetLimitGraphHeight();
        const bool showNodeNames = m_plugin->m_trackHeaderWidget->m_nodeNamesCheckBox->isChecked();
        const bool showMotionFiles = m_plugin->m_trackHeaderWidget->m_motionFilesCheckBox->isChecked();
        const bool interpolate = recorder.GetRecordSettings().m_interpolate;

        float graphHeight = aznumeric_cast<float>(geometry().height() - m_graphStartHeight);
        float graphBottom;
        if (limitGraphHeight == false)
        {
            graphBottom = aznumeric_cast<float>(geometry().height());
        }
        else
        {
            if (graphHeight > 200)
            {
                graphHeight = 200;
            }

            graphBottom = m_graphStartHeight + graphHeight;
        }

        const uint32 graphContentsCode = m_plugin->m_trackHeaderWidget->m_graphContentsComboBox->currentIndex();

        for (EMotionFX::Recorder::NodeHistoryItem* curItem : historyItems)
        {
            double startTimePixel   = m_plugin->TimeToPixel(curItem->m_startTime);
            double endTimePixel     = m_plugin->TimeToPixel(curItem->m_endTime);

            const QRect itemRect(QPoint(aznumeric_cast<int>(startTimePixel), m_graphStartHeight), QPoint(aznumeric_cast<int>(endTimePixel), geometry().height()));
            if (rect.intersects(itemRect) == false)
            {
                continue;
            }

            const AZ::Color colorCode = (useNodeColors) ? curItem->m_typeColor : curItem->m_color;
            QColor color;
            color.setRgbF(colorCode.GetR(), colorCode.GetG(), colorCode.GetB(), colorCode.GetA());

            if (m_plugin->m_nodeHistoryItem != curItem || m_isScrolling || m_plugin->m_isAnimating)
            {
                painter.setPen(color);
                color.setAlpha(64);
                painter.setBrush(color);
            }
            else
            {
                color = QColor(255, 128, 0);
                painter.setPen(color);
                color.setAlpha(128);
                painter.setBrush(color);
            }

            QPainterPath path;
            int32 widthInPixels = aznumeric_cast<int32>(endTimePixel - startTimePixel);
            if (widthInPixels > 0)
            {
                EMotionFX::KeyTrackLinearDynamic<float, float>* keyTrack = &curItem->m_globalWeights; // init on global weights

                if (graphContentsCode == 1)
                {
                    keyTrack = &curItem->m_localWeights;
                }
                else
                if (graphContentsCode == 2)
                {
                    keyTrack = &curItem->m_playTimes;
                }

                float lastWeight = keyTrack->GetValueAtTime(0.0f, &curItem->m_cachedKey, nullptr, interpolate);
                const float keyTimeStep = (curItem->m_endTime - curItem->m_startTime) / (float)widthInPixels;

                const int32 pixelStepSize = 1;//(widthInPixels / 300.0f) + 1;

                path.moveTo(QPointF(startTimePixel, graphBottom + 1));
                path.lineTo(QPointF(startTimePixel, graphBottom - 1 - lastWeight * graphHeight));
                bool firstPixel = true;
                for (int32 w = 1; w < widthInPixels - 1; w += pixelStepSize)
                {
                    if (startTimePixel + w > windowWidth)
                    {
                        break;
                    }

                    if (firstPixel && startTimePixel < 0)
                    {
                        w = aznumeric_cast<int32>(-startTimePixel);
                        firstPixel = false;
                    }

                    const float weight = keyTrack->GetValueAtTime(w * keyTimeStep, &curItem->m_cachedKey, nullptr, interpolate);
                    const float height = graphBottom - weight * graphHeight;
                    path.lineTo(QPointF(startTimePixel + w + 1, height));
                }

                const float weight = keyTrack->GetValueAtTime(curItem->m_endTime, &curItem->m_cachedKey, nullptr, interpolate);
                const float height = graphBottom - weight * graphHeight;
                path.lineTo(QPointF(startTimePixel + widthInPixels - 1, height));
                path.lineTo(QPointF(startTimePixel + widthInPixels, graphBottom + 1));
                painter.drawPath(path);
            }
        }

        // calculate the remapped track list, based on sorted global weight, with the most influencing track on top
        recorder.ExtractNodeHistoryItems(*actorInstanceData, aznumeric_cast<float>(m_plugin->m_curTime), true, (EMotionFX::Recorder::EValueType)graphContentsCode, &m_activeItems, &m_trackRemap);

        // display the values and names
        int offset = 0;
        for (const EMotionFX::Recorder::ExtractedNodeHistoryItem& activeItem : m_activeItems)
        {
            EMotionFX::Recorder::NodeHistoryItem* curItem = activeItem.m_nodeHistoryItem;
            if (curItem == nullptr)
            {
                continue;
            }

            offset += 15;

            m_tempString.clear();
            if (showNodeNames)
            {
                m_tempString += curItem->m_name.c_str();
            }

            if (showMotionFiles && !curItem->m_motionFileName.empty())
            {
                if (!m_tempString.empty())
                {
                    m_tempString += " - ";
                }

                m_tempString += curItem->m_motionFileName.c_str();
            }

            if (!m_tempString.empty())
            {
                m_tempString += AZStd::string::format(" = %.4f", activeItem.m_value);
            }
            else
            {
                m_tempString = AZStd::string::format("%.4f", activeItem.m_value);
            }

            const AZ::Color colorCode = (useNodeColors) ? activeItem.m_nodeHistoryItem->m_typeColor : activeItem.m_nodeHistoryItem->m_color;
            QColor color;
            color.setRgbF(colorCode.GetR(), colorCode.GetG(), colorCode.GetB(), colorCode.GetA());

            painter.setPen(color);
            painter.setBrush(Qt::NoBrush);
            painter.setFont(m_dataFont);
            painter.drawText(3, offset + m_graphStartHeight, m_tempString.c_str());
        }
    }


    // paint the events
    void TrackDataWidget::PaintRecorderEventHistory(QPainter& painter, const QRect& rect, const EMotionFX::Recorder::ActorInstanceData* actorInstanceData)
    {
        EMotionFX::Recorder& recorder = EMotionFX::GetRecorder();

        const double animationLength = recorder.GetRecordTime();
        if (animationLength < MCore::Math::epsilon)
        {
            return;
        }

        // get the history items shortcut
        const AZStd::vector<EMotionFX::Recorder::EventHistoryItem*>& historyItems = actorInstanceData->m_eventHistoryItems;

        QRect clipRect = rect;
        clipRect.setRight(aznumeric_cast<int>(m_plugin->TimeToPixel(animationLength)));
        painter.setClipRect(clipRect);
        painter.setClipping(true);

        // for all event history items
        const float tickHalfWidth = 7;
        const float tickHeight = 16;

        QPointF tickPoints[6];
        for (const EMotionFX::Recorder::EventHistoryItem* curItem : historyItems)
        {
            float height = aznumeric_cast<float>((curItem->m_trackIndex * 20) + m_eventsStartHeight);
            double startTimePixel   = m_plugin->TimeToPixel(curItem->m_startTime);

            const QRect itemRect(QPoint(aznumeric_cast<int>(startTimePixel - tickHalfWidth), aznumeric_cast<int>(height)), QSize(aznumeric_cast<int>(tickHalfWidth * 2), aznumeric_cast<int>(tickHeight)));
            if (rect.intersects(itemRect) == false)
            {
                continue;
            }

            // try to locate the node based on its unique ID
            QColor borderColor(30, 30, 30);
            const AZ::Color& colorCode = curItem->m_color;
            QColor color;
            color.setRgbF(colorCode.GetR(), colorCode.GetG(), colorCode.GetB(), colorCode.GetA());
            
            if (m_isScrolling == false && m_plugin->m_isAnimating == false)
            {
                if (m_plugin->m_nodeHistoryItem && m_plugin->m_nodeHistoryItem->m_nodeId == curItem->m_emitterNodeId)
                {
                    if (curItem->m_startTime >= m_plugin->m_nodeHistoryItem->m_startTime && curItem->m_startTime <= m_plugin->m_nodeHistoryItem->m_endTime)
                    {
                        RecorderGroup* recorderGroup = m_plugin->GetTimeViewToolBar()->GetRecorderGroup();
                        if (recorderGroup->GetDisplayNodeActivity())
                        {
                            borderColor = QColor(255, 128, 0);
                            color = QColor(255, 128, 0);
                        }
                    }
                }

                if (m_plugin->m_eventHistoryItem == curItem)
                {
                    borderColor = QColor(255, 128, 0);
                    color = borderColor;
                }
            }

            const QColor gradientColor = QColor(color.red() / 2, color.green() / 2, color.blue() / 2, color.alpha());
            QLinearGradient gradient(0, height, 0, height + tickHeight);
            gradient.setColorAt(0.0f, color);
            gradient.setColorAt(1.0f, gradientColor);

            painter.setPen(Qt::red);
            painter.setBrush(Qt::black);

            tickPoints[0] = QPoint(aznumeric_cast<int>(startTimePixel),                  aznumeric_cast<int>(height));
            tickPoints[1] = QPoint(aznumeric_cast<int>(startTimePixel + tickHalfWidth),    aznumeric_cast<int>(height + tickHeight / 2));
            tickPoints[2] = QPoint(aznumeric_cast<int>(startTimePixel + tickHalfWidth),    aznumeric_cast<int>(height + tickHeight));
            tickPoints[3] = QPoint(aznumeric_cast<int>(startTimePixel - tickHalfWidth),    aznumeric_cast<int>(height + tickHeight));
            tickPoints[4] = QPoint(aznumeric_cast<int>(startTimePixel - tickHalfWidth),    aznumeric_cast<int>(height + tickHeight / 2));
            tickPoints[5] = QPoint(aznumeric_cast<int>(startTimePixel),                  aznumeric_cast<int>(height));

            painter.setPen(Qt::NoPen);
            painter.setBrush(gradient);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.drawPolygon(tickPoints, 5, Qt::WindingFill);
            painter.setRenderHint(QPainter::Antialiasing, false);

            painter.setBrush(Qt::NoBrush);
            painter.setPen(borderColor);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.drawPolyline(tickPoints, 6);
            painter.setRenderHint(QPainter::Antialiasing, false);
        }

        painter.setClipping(false);
    }


    // paint the node history
    void TrackDataWidget::PaintRecorderNodeHistory(QPainter& painter, const QRect& rect, const EMotionFX::Recorder::ActorInstanceData* actorInstanceData)
    {
        EMotionFX::Recorder& recorder = EMotionFX::GetRecorder();

        const double animationLength = recorder.GetRecordTime();
        if (animationLength < MCore::Math::epsilon)
        {
            return;
        }

        // skip the complete rendering of the node history data when its bounds are not inside view
        if (!rect.intersects(m_nodeHistoryRect))
        {
            return;
        }

        // get the history items shortcut
        const AZStd::vector<EMotionFX::Recorder::NodeHistoryItem*>&  historyItems = actorInstanceData->m_nodeHistoryItems;
        int32 windowWidth = geometry().width();

        // calculate the remapped track list, based on sorted global weight, with the most influencing track on top
        RecorderGroup* recorderGroup = m_plugin->GetTimeViewToolBar()->GetRecorderGroup();
        const bool sorted = recorderGroup->GetSortNodeActivity();
        const bool useNodeColors = recorderGroup->GetUseNodeTypeColors();

        const int graphContentsCode = m_plugin->m_trackHeaderWidget->m_nodeContentsComboBox->currentIndex();
        recorder.ExtractNodeHistoryItems(*actorInstanceData, aznumeric_cast<float>(m_plugin->m_curTime), sorted, (EMotionFX::Recorder::EValueType)graphContentsCode, &m_activeItems, &m_trackRemap);

        const bool showNodeNames = m_plugin->m_trackHeaderWidget->m_nodeNamesCheckBox->isChecked();
        const bool showMotionFiles = m_plugin->m_trackHeaderWidget->m_motionFilesCheckBox->isChecked();
        const bool interpolate = recorder.GetRecordSettings().m_interpolate;

        const int nodeContentsCode = m_plugin->m_trackHeaderWidget->m_nodeContentsComboBox->currentIndex();

        // for all history items
        QRectF itemRect;
        for (EMotionFX::Recorder::NodeHistoryItem* curItem : historyItems)
        {
            // draw the background rect
            double startTimePixel   = m_plugin->TimeToPixel(curItem->m_startTime);
            double endTimePixel     = m_plugin->TimeToPixel(curItem->m_endTime);

            const size_t trackIndex = m_trackRemap[ curItem->m_trackIndex ];

            itemRect.setLeft(startTimePixel);
            itemRect.setRight(endTimePixel - 1);
            itemRect.setTop((m_nodeRectsStartHeight + (aznumeric_cast<uint32>(trackIndex) * (m_nodeHistoryItemHeight + 3)) + 3));
            itemRect.setBottom(itemRect.top() + m_nodeHistoryItemHeight);

            if (!rect.intersects(itemRect.toRect()))
            {
                continue;
            }

            const AZ::Color colorCode = (useNodeColors) ? curItem->m_typeColor : curItem->m_color;
            QColor color;
            color.setRgbF(colorCode.GetR(), colorCode.GetG(), colorCode.GetB(), colorCode.GetA());

            bool matchesEvent = false;
            if (m_isScrolling == false && m_plugin->m_isAnimating == false)
            {
                if (m_plugin->m_nodeHistoryItem == curItem)
                {
                    color = QColor(255, 128, 0);
                }

                if (m_plugin->m_eventEmitterNode && m_plugin->m_eventEmitterNode->GetId() == curItem->m_nodeId && m_plugin->m_eventHistoryItem)
                {
                    if (m_plugin->m_eventHistoryItem->m_startTime >= curItem->m_startTime && m_plugin->m_eventHistoryItem->m_startTime <= curItem->m_endTime)
                    {
                        color = QColor(255, 128, 0);
                        matchesEvent = true;
                    }
                }
            }

            painter.setPen(color);
            color.setAlpha(128);
            painter.setBrush(color);
            painter.drawRoundedRect(itemRect, 2, 2);

            // draw weights
            //---------
            painter.setRenderHint(QPainter::Antialiasing, true);
            QPainterPath path;
            itemRect.setRight(itemRect.right() - 1);
            painter.setClipRegion(itemRect.toRect());
            painter.setClipping(true);

            int32 widthInPixels = aznumeric_cast<int32>(endTimePixel - startTimePixel);
            if (widthInPixels > 0)
            {
                const EMotionFX::KeyTrackLinearDynamic<float, float>* keyTrack = &curItem->m_globalWeights; // init on global weights
                if (nodeContentsCode == 1)
                {
                    keyTrack = &curItem->m_localWeights;
                }
                else
                if (nodeContentsCode == 2)
                {
                    keyTrack = &curItem->m_playTimes;
                }

                float lastWeight = keyTrack->GetValueAtTime(0.0f, &curItem->m_cachedKey, nullptr, interpolate);
                const float keyTimeStep = (curItem->m_endTime - curItem->m_startTime) / (float)widthInPixels;

                const int32 pixelStepSize = 1;//(widthInPixels / 300.0f) + 1;

                path.moveTo(QPointF(startTimePixel - 1, itemRect.bottom() + 1));
                path.lineTo(QPointF(startTimePixel + 1, itemRect.bottom() - 1 - lastWeight * m_nodeHistoryItemHeight));
                bool firstPixel = true;
                for (int32 w = 1; w < widthInPixels - 1; w += pixelStepSize)
                {
                    if (startTimePixel + w > windowWidth)
                    {
                        break;
                    }

                    if (firstPixel && startTimePixel < 0)
                    {
                        w = aznumeric_cast<int>(-startTimePixel);
                        firstPixel = false;
                    }

                    const float weight = keyTrack->GetValueAtTime(w * keyTimeStep, &curItem->m_cachedKey, nullptr, interpolate);
                    const float height = aznumeric_cast<float>(itemRect.bottom() - weight * m_nodeHistoryItemHeight);
                    path.lineTo(QPointF(startTimePixel + w + 1, height));
                }

                const float weight = keyTrack->GetValueAtTime(curItem->m_endTime, &curItem->m_cachedKey, nullptr, interpolate);
                const float height = aznumeric_cast<float>(itemRect.bottom() - weight * m_nodeHistoryItemHeight);
                path.lineTo(QPointF(startTimePixel + widthInPixels - 1, height));
                path.lineTo(QPointF(startTimePixel + widthInPixels, itemRect.bottom() + 1));
                painter.drawPath(path);
                painter.setRenderHint(QPainter::Antialiasing, false);
            }
            //---------

            // draw the text
            if (matchesEvent != true)
            {
                if (m_isScrolling == false && m_plugin->m_isAnimating == false)
                {
                    if (m_plugin->m_nodeHistoryItem != curItem)
                    {
                        painter.setPen(QColor(255, 255, 255, 175));
                    }
                    else
                    {
                        painter.setPen(QColor(0, 0, 0));
                    }
                }
                else
                {
                    painter.setPen(QColor(255, 255, 255, 175));
                }
            }
            else
            {
                painter.setPen(Qt::black);
            }

            m_tempString.clear();
            if (showNodeNames)
            {
                m_tempString += curItem->m_name.c_str();
            }

            if (showMotionFiles && !curItem->m_motionFileName.empty())
            {
                if (!m_tempString.empty())
                {
                    m_tempString += " - ";
                }

                m_tempString += curItem->m_motionFileName.c_str();
            }

            if (!m_tempString.empty())
            {
                painter.drawText(aznumeric_cast<int>(itemRect.left() + 3), aznumeric_cast<int>(itemRect.bottom() - 2), m_tempString.c_str());
            }

            painter.setClipping(false);
        }
    }


    // paint the motion tracks for a given motion
    void TrackDataWidget::PaintMotionTracks(QPainter& painter, const QRect& rect)
    {
        double animationLength  = 0.0;
        double clipStart        = 0.0;
        double clipEnd          = 0.0;

        // get the track over which the cursor is positioned
        QPoint localCursorPos = mapFromGlobal(QCursor::pos());
        TimeTrack* mouseCursorTrack = m_plugin->GetTrackAt(localCursorPos.y());
        if (localCursorPos.x() < 0 || localCursorPos.x() > width())
        {
            mouseCursorTrack = nullptr;
        }

        // handle highlighting
        const size_t numTracks = m_plugin->GetNumTracks();
        for (size_t i = 0; i < numTracks; ++i)
        {
            TimeTrack* track = m_plugin->GetTrack(i);

            // set the highlighting flag for the track
            if (track == mouseCursorTrack)
            {
                // highlight the track
                track->SetIsHighlighted(true);

                // get the element over which the cursor is positioned
                TimeTrackElement* mouseCursorElement = m_plugin->GetElementAt(localCursorPos.x(), localCursorPos.y());

                // get the number of elements, iterate through them and disable the highlight flag
                const size_t numElements = track->GetNumElements();
                for (size_t e = 0; e < numElements; ++e)
                {
                    TimeTrackElement* element = track->GetElement(e);

                    if (element == mouseCursorElement)
                    {
                        element->SetIsHighlighted(true);
                    }
                    else
                    {
                        element->SetIsHighlighted(false);
                    }
                }
            }
            else
            {
                track->SetIsHighlighted(false);

                // get the number of elements, iterate through them and disable the highlight flag
                const size_t numElements = track->GetNumElements();
                for (size_t e = 0; e < numElements; ++e)
                {
                    TimeTrackElement* element = track->GetElement(e);
                    element->SetIsHighlighted(false);
                }
            }
        }

        EMotionFX::Motion* motion = m_plugin->GetMotion();
        if (motion)
        {
            // get the motion length
            animationLength = motion->GetDuration();

            // get the playback info and read out the clip start/end times
            EMotionFX::PlayBackInfo* playbackInfo = motion->GetDefaultPlayBackInfo();
            clipStart   = playbackInfo->m_clipStartTime;
            clipEnd     = playbackInfo->m_clipEndTime;

            // HACK: fix this later
            clipStart = 0.0;
            clipEnd = animationLength;
        }

        // calculate the pixel index of where the animation ends and where it gets clipped
        const double animEndPixel   = m_plugin->TimeToPixel(animationLength);
        const double clipStartPixel = m_plugin->TimeToPixel(clipStart);
        const double clipEndPixel   = m_plugin->TimeToPixel(clipEnd);

        // enable anti aliassing
        //painter.setRenderHint(QPainter::Antialiasing);

        // fill with the background color
        QRectF clipStartRect    = rect;
        QRectF motionRect       = rect;
        QRectF clipEndRect      = rect;
        QRectF outOfRangeRect   = rect;

        clipEndRect.setRight(clipStartPixel);
        motionRect.setLeft(clipStartPixel);
        motionRect.setRight(clipEndPixel);
        clipEndRect.setLeft(clipEndPixel);
        clipEndRect.setRight(animEndPixel);
        outOfRangeRect.setLeft(animEndPixel);

        clipStartRect.setTop(0);
        clipEndRect.setTop(0);
        motionRect.setTop(0);
        outOfRangeRect.setTop(0);

        // render the rects
        painter.setPen(Qt::NoPen);
        painter.setBrush(m_brushBackgroundClipped);
        painter.drawRect(clipStartRect);
        painter.setBrush(m_brushBackground);
        painter.drawRect(motionRect);
        painter.setBrush(m_brushBackgroundClipped);
        painter.drawRect(clipEndRect);
        painter.setBrush(m_brushBackgroundOutOfRange);
        painter.drawRect(outOfRangeRect);

        // render the tracks
        RenderTracks(painter, rect.width(), rect.height(), animationLength, clipStart, clipEnd);
    }

    // render all tracks
    void TrackDataWidget::RenderTracks(QPainter& painter, uint32 width, uint32 height, double animationLength, double clipStartTime, double clipEndTime)
    {
        //MCore::Timer renderTimer;
        int32 yOffset = 2; // offset two pixels that used to be part of the timeline that got moved to another widget

        // calculate the start and end time range of the visible area
        double visibleStartTime, visibleEndTime;
        visibleStartTime = m_plugin->PixelToTime(0);
        visibleEndTime = m_plugin->PixelToTime(width);

        // for all tracks
        for (TimeTrack* track : m_plugin->m_tracks)
        {
            track->SetStartY(yOffset);

            // path for making the cut elements a bit transparent
            if (m_cutMode)
            {
                // disable cut mode for all elements on default
                const size_t numElements = track->GetNumElements();
                for (size_t e = 0; e < numElements; ++e)
                {
                    track->GetElement(e)->SetIsCut(false);
                }

                // get the number of copy elements and check if ours is in
                for (const CopyElement& copyElement : m_copyElements)
                {
                    // get the copy element and make sure we're in the right track
                    if (copyElement.m_trackName != track->GetName())
                    {
                        continue;
                    }

                    // set the cut mode of the elements
                    for (size_t e = 0; e < numElements; ++e)
                    {
                        TimeTrackElement* element = track->GetElement(e);
                        if (MCore::Compare<float>::CheckIfIsClose(aznumeric_cast<float>(element->GetStartTime()), copyElement.m_startTime, MCore::Math::epsilon) &&
                            MCore::Compare<float>::CheckIfIsClose(aznumeric_cast<float>(element->GetEndTime()), copyElement.m_endTime, MCore::Math::epsilon))
                        {
                            element->SetIsCut(true);
                        }
                    }
                }
            }

            // render the track
            track->RenderData(painter, width, yOffset, visibleStartTime, visibleEndTime, animationLength, clipStartTime, clipEndTime);

            // increase the offsets
            yOffset += track->GetHeight();
            yOffset += 1;
        }

        // render the element time handles
        m_plugin->RenderElementTimeHandles(painter, height, m_plugin->m_penTimeHandles);
    }


    // show the time of the currently dragging element in the time info view
    void TrackDataWidget::ShowElementTimeInfo(TimeTrackElement* element)
    {
        if (m_plugin->GetTimeInfoWidget() == nullptr)
        {
            return;
        }

        // enable overwrite mode so that the time info widget will show the custom time rather than the current time of the plugin
        m_plugin->GetTimeInfoWidget()->SetIsOverwriteMode(true);

        // calculate the dimensions
        int32 startX, startY, width, height;
        element->CalcDimensions(&startX, &startY, &width, &height);

        // show the times of the element
        m_plugin->GetTimeInfoWidget()->SetOverwriteTime(m_plugin->PixelToTime(startX), m_plugin->PixelToTime(startX + width));
    }


    void TrackDataWidget::mouseDoubleClickEvent(QMouseEvent* event)
    {
        if (event->button() != Qt::LeftButton)
        {
            return;
        }

        // if we clicked inside the node history area
        RecorderGroup* recorderGroup = m_plugin->GetTimeViewToolBar()->GetRecorderGroup();
        if (GetIsInsideNodeHistory(event->y()) && recorderGroup->GetDisplayNodeActivity())
        {
            EMotionFX::Recorder::ActorInstanceData* actorInstanceData = FindActorInstanceData();
            EMotionFX::Recorder::NodeHistoryItem* historyItem = FindNodeHistoryItem(actorInstanceData, event->x(), event->y());
            if (historyItem)
            {
                emit m_plugin->DoubleClickedRecorderNodeHistoryItem(actorInstanceData, historyItem);
            }
        }
    }

    void TrackDataWidget::SetPausedTime(float timeValue, bool emitTimeChangeStart)
    {
        m_plugin->m_curTime = timeValue;
        const AZStd::vector<EMotionFX::MotionInstance*>& motionInstances = MotionWindowPlugin::GetSelectedMotionInstances();
        if (motionInstances.size() == 1)
        {
            EMotionFX::MotionInstance* motionInstance = motionInstances[0];
            motionInstance->SetCurrentTime(timeValue);
            motionInstance->SetPause(true);
        }
        if (emitTimeChangeStart)
        {
            emit m_plugin->ManualTimeChangeStart(timeValue);
        }
        emit m_plugin->ManualTimeChange(timeValue);
    }

    // when the mouse is moving, while a button is pressed
    void TrackDataWidget::mouseMoveEvent(QMouseEvent* event)
    {
        m_plugin->SetRedrawFlag();

        QPoint mousePos = event->pos();

        const int32 deltaRelX = event->x() - m_lastMouseX;
        m_lastMouseX = event->x();
        m_plugin->m_curMouseX = event->x();
        m_plugin->m_curMouseY = event->y();

        const int32 deltaRelY = event->y() - m_lastMouseY;
        m_lastMouseY = event->y();

        const bool altPressed = event->modifiers() & Qt::AltModifier;
        const bool isZooming = m_mouseLeftClicked == false && m_mouseRightClicked && altPressed;
        const bool isPanning = m_mouseLeftClicked == false && isZooming == false && (m_mouseMidClicked || m_mouseRightClicked);

        if (deltaRelY != 0)
        {
            m_allowContextMenu = false;
        }

        // get the track over which the cursor is positioned
        TimeTrack* mouseCursorTrack = m_plugin->GetTrackAt(event->y());

        if (m_mouseRightClicked)
        {
            m_isScrolling = true;
        }

        // if the mouse left button is pressed
        if (m_mouseLeftClicked)
        {
            if (altPressed)
            {
                m_rectZooming = true;
            }
            else
            {
                m_rectZooming = false;
            }

            // rect selection: update mouse position
            if (m_rectSelecting)
            {
                m_selectEnd = mousePos;
            }

            if (m_draggingElement == nullptr && m_resizeElement == nullptr && m_rectSelecting == false)
            {
                // update the current time marker
                int newX = event->x();
                newX = MCore::Clamp<int>(newX, 0, geometry().width() - 1);
                m_plugin->m_curTime = m_plugin->PixelToTime(newX);

                EMotionFX::Recorder& recorder = EMotionFX::GetRecorder();
                if (recorder.GetRecordTime() > MCore::Math::epsilon)
                {
                    if (recorder.GetIsInPlayMode())
                    {
                        recorder.SetCurrentPlayTime(aznumeric_cast<float>(m_plugin->GetCurrentTime()));
                        recorder.SetAutoPlay(false);
                        emit m_plugin->ManualTimeChange(aznumeric_cast<float>(m_plugin->GetCurrentTime()));
                    }
                }
                else
                {
                    SetPausedTime(aznumeric_cast<float>(m_plugin->m_curTime));
                }

                m_isScrolling = true;
            }

            TimeTrack* dragElementTrack = nullptr;
            if (m_draggingElement)
            {
                dragElementTrack = m_draggingElement->GetTrack();
            }

            // calculate the delta movement
            const int32 deltaX = event->x() - m_lastLeftClickedX;
            const int32 movement = abs(deltaX);
            const bool elementTrackChanged = (mouseCursorTrack && dragElementTrack && mouseCursorTrack != dragElementTrack);
            if ((movement > 1 && !m_dragging) || elementTrackChanged)
            {
                m_dragging = true;
            }

            // handle resizing
            if (m_resizing)
            {
                if (m_plugin->FindTrackByElement(m_resizeElement) == nullptr)
                {
                    m_resizeElement = nullptr;
                }

                if (m_resizeElement)
                {
                    TimeTrack* resizeElementTrack = m_resizeElement->GetTrack();

                    // only allow resizing on enabled time tracks
                    if (resizeElementTrack->GetIsEnabled())
                    {
                        m_resizeElement->SetShowTimeHandles(true);
                        m_resizeElement->SetShowToolTip(false);

                        double resizeTime = (deltaRelX / m_plugin->m_timeScale) / m_plugin->m_pixelsPerSecond;
                        m_resizeId = m_resizeElement->HandleResize(m_resizeId, resizeTime, 0.02 / m_plugin->m_timeScale);

                        // show the time of the currently resizing element in the time info view
                        ShowElementTimeInfo(m_resizeElement);

                        // Move the current time marker along with the event resizing position.
                        float timeValue = 0.0f;
                        switch (m_resizeId)
                        {
                            case TimeTrackElement::RESIZEPOINT_START:
                            {
                                timeValue = aznumeric_cast<float>(m_resizeElement->GetStartTime());
                                break;
                            }
                            case TimeTrackElement::RESIZEPOINT_END:
                            {
                                timeValue = aznumeric_cast<float>(m_resizeElement->GetEndTime());
                                break;
                            }
                            default:
                            {
                                AZ_Warning("EMotionFX", false, "Unknown time track element resize point.");
                            }
                        }
                        SetPausedTime(timeValue);

                        setCursor(Qt::SizeHorCursor);
                    }

                    return;
                }
            }

            // if we are not dragging or no element is being dragged, there is nothing to do
            if (m_dragging == false || m_draggingElement == nullptr)
            {
                return;
            }

            // check if the mouse cursor is over another time track than the dragging element
            if (elementTrackChanged)
            {
                // if yes we need to remove the dragging element from the old time track
                dragElementTrack->RemoveElement(m_draggingElement, false);

                // and add it to the new time track where the cursor now is over
                mouseCursorTrack->AddElement(m_draggingElement);
                m_draggingElement->SetTrack(mouseCursorTrack);
            }

            // show the time of the currently dragging element in the time info view
            ShowElementTimeInfo(m_draggingElement);

            // adjust the cursor
            setCursor(Qt::ClosedHandCursor);
            m_draggingElement->SetShowToolTip(false);

            // show the time handles
            m_draggingElement->SetShowTimeHandles(true);

            const double snapThreshold = 0.02 / m_plugin->m_timeScale;

            // calculate how many pixels we moved with the mouse
            const int32 deltaMovement = event->x() - m_lastMouseMoveX;
            m_lastMouseMoveX = event->x();

            // snap the moved amount to a given time value
            double snappedTime = m_draggingElement->GetStartTime() + ((deltaMovement / m_plugin->m_pixelsPerSecond) / m_plugin->m_timeScale);

            bool startSnapped = false;
            if (abs(deltaMovement) < 2 && abs(deltaMovement) > 0) // only snap when moving the mouse very slowly
            {
                startSnapped = m_plugin->SnapTime(&snappedTime, m_draggingElement, snapThreshold);
            }

            // in case the start time didn't snap to anything
            if (startSnapped == false)
            {
                // try to snap the end time
                double snappedEndTime = m_draggingElement->GetEndTime() + ((deltaMovement / m_plugin->m_pixelsPerSecond) / m_plugin->m_timeScale);
                /*bool endSnapped = */ m_plugin->SnapTime(&snappedEndTime, m_draggingElement, snapThreshold);

                // apply the delta movement
                const double deltaTime = snappedEndTime - m_draggingElement->GetEndTime();
                m_draggingElement->MoveRelative(deltaTime);
            }
            else
            {
                // apply the snapped delta movement
                const double deltaTime = snappedTime - m_draggingElement->GetStartTime();
                m_draggingElement->MoveRelative(deltaTime);
            }

            dragElementTrack = m_draggingElement->GetTrack();
            const float timeValue = aznumeric_cast<float>(m_draggingElement->GetStartTime());
            SetPausedTime(timeValue);
        }
        else if (isPanning)
        {
            if (EMotionFX::GetRecorder().GetIsRecording() == false)
            {
                m_plugin->DeltaScrollX(-deltaRelX, false);
            }
        }
        else if (isZooming)
        {
            if (deltaRelY < 0)
            {
                setCursor(*(m_plugin->GetZoomOutCursor()));
            }
            else
            {
                setCursor(*(m_plugin->GetZoomInCursor()));
            }

            DoMouseYMoveZoom(deltaRelY, m_plugin);
        }
        else // no left mouse button is pressed
        {
            UpdateMouseOverCursor(event->x(), event->y());
        }
    }


    void TrackDataWidget::DoMouseYMoveZoom(int32 deltaY, TimeViewPlugin* plugin)
    {
        // keep the scaling speed in range so that we're not scaling insanely fast
        float movement = MCore::Min(aznumeric_cast<float>(deltaY), 9.0f);
        movement = MCore::Max<float>(movement, -9.0f);

        // scale relatively to the current scaling value, meaning when the range is bigger we scale faster than when viewing only a very small time range
        float timeScale = plugin->GetTimeScale();
        timeScale *= 1.0f - 0.01f * movement;

        // set the new scaling value
        plugin->SetScale(timeScale);
    }


    // update the mouse-over cursor, depending on its location
    void TrackDataWidget::UpdateMouseOverCursor(int32 x, int32 y)
    {
        // disable all tooltips
        m_plugin->DisableAllToolTips();

        // get the time track and return directly if we are not over a valid track with the cursor
        TimeTrack* timeTrack = m_plugin->GetTrackAt(y);
        if (timeTrack == nullptr)
        {
            setCursor(Qt::ArrowCursor);
            return;
        }

        // get the element over which the cursor is positioned
        TimeTrackElement* element = m_plugin->GetElementAt(x, y);

        // in case the cursor is over an element, show tool tips
        if (element)
        {
            element->SetShowToolTip(true);
        }
        else
        {
            m_plugin->DisableAllToolTips();
        }

        // do not allow any editing in case the track is not enabled
        if (timeTrack->GetIsEnabled() == false)
        {
            setCursor(Qt::ArrowCursor);
            return;
        }

        // check if we are hovering over a resize point
        if (m_plugin->FindResizePoint(x, y, &m_resizeElement, &m_resizeId))
        {
            setCursor(Qt::SizeHorCursor);
            m_resizeElement->SetShowToolTip(true);
        }
        else // if we're not above a resize point
        {
            if (element)
            {
                setCursor(Qt::OpenHandCursor);
            }
            else
            {
                setCursor(Qt::ArrowCursor);
            }
        }
    }


    // when the mouse is pressed
    void TrackDataWidget::mousePressEvent(QMouseEvent* event)
    {
        m_plugin->SetRedrawFlag();

        QPoint mousePos = event->pos();

        const bool ctrlPressed = event->modifiers() & Qt::ControlModifier;
        const bool shiftPressed = event->modifiers() & Qt::ShiftModifier;
        const bool altPressed = event->modifiers() & Qt::AltModifier;

        // store the last clicked position
        m_lastMouseMoveX     = event->x();
        m_allowContextMenu   = true;
        m_rectSelecting      = false;

        if (event->button() == Qt::RightButton)
        {
            m_mouseRightClicked = true;
        }

        if (event->button() == Qt::MidButton)
        {
            m_mouseMidClicked = true;
        }

        if (event->button() == Qt::LeftButton)
        {
            m_mouseLeftClicked   = true;

            EMotionFX::Recorder& recorder = EMotionFX::GetRecorder();
            if ((m_plugin->m_nodeHistoryItem == nullptr) && altPressed == false && (recorder.GetRecordTime() >= MCore::Math::epsilon))
            {
                // update the current time marker
                int newX = event->x();
                newX = MCore::Clamp<int>(newX, 0, geometry().width() - 1);
                m_plugin->m_curTime = m_plugin->PixelToTime(newX);

                if (recorder.GetRecordTime() < MCore::Math::epsilon)
                {
                    SetPausedTime(aznumeric_cast<float>(m_plugin->GetCurrentTime()), /*emitTimeChangeStart=*/true);
                }
                else
                {
                    if (recorder.GetIsInPlayMode() == false)
                    {
                        recorder.StartPlayBack();
                    }

                    recorder.SetCurrentPlayTime(aznumeric_cast<float>(m_plugin->GetCurrentTime()));
                    recorder.SetAutoPlay(false);
                    emit m_plugin->ManualTimeChangeStart(aznumeric_cast<float>(m_plugin->GetCurrentTime()));
                    emit m_plugin->ManualTimeChange(aznumeric_cast<float>(m_plugin->GetCurrentTime()));
                }
            }
            else // not inside timeline
            {
                // if we clicked inside the node history area
                RecorderGroup* recorderGroup = m_plugin->GetTimeViewToolBar()->GetRecorderGroup();
                if (GetIsInsideNodeHistory(event->y()) && recorderGroup->GetDisplayNodeActivity())
                {
                    EMotionFX::Recorder::ActorInstanceData* actorInstanceData = FindActorInstanceData();
                    EMotionFX::Recorder::NodeHistoryItem* historyItem = FindNodeHistoryItem(actorInstanceData, event->x(), event->y());
                    if (historyItem && altPressed == false)
                    {
                        emit m_plugin->ClickedRecorderNodeHistoryItem(actorInstanceData, historyItem);
                    }
                }
                {
                    // unselect all elements
                    if (ctrlPressed == false && shiftPressed == false)
                    {
                        m_plugin->UnselectAllElements();
                    }

                    // find the element we're clicking in
                    TimeTrackElement* element = m_plugin->GetElementAt(event->x(), event->y());
                    if (element)
                    {
                        // show the time of the currently dragging element in the time info view
                        ShowElementTimeInfo(element);

                        TimeTrack* timeTrack = element->GetTrack();

                        if (timeTrack->GetIsEnabled())
                        {
                            m_draggingElement    = element;
                            m_dragElementTrack   = timeTrack;
                            m_draggingElement->SetShowTimeHandles(true);
                            setCursor(Qt::ClosedHandCursor);
                        }
                        else
                        {
                            m_draggingElement    = nullptr;
                            m_dragElementTrack   = nullptr;
                        }

                        // shift select
                        if (shiftPressed)
                        {
                            // get the element number of the clicked element
                            const size_t clickedElementNr = element->GetElementNumber();

                            // get the element number of the first previously selected element
                            TimeTrackElement* firstSelectedElement = timeTrack->GetFirstSelectedElement();
                            const size_t firstSelectedNr = firstSelectedElement ? firstSelectedElement->GetElementNumber() : 0;

                            // range select
                            timeTrack->RangeSelectElements(firstSelectedNr, clickedElementNr);
                        }
                        else
                        {
                            // normal select
                            element->SetIsSelected(!element->GetIsSelected());
                        }

                        element->SetShowToolTip(false);

                        emit SelectionChanged();
                    }
                    else // no element clicked
                    {
                        m_draggingElement    = nullptr;
                        m_dragElementTrack   = nullptr;

                        // rect selection
                        m_rectSelecting  = true;
                        m_selectStart    = mousePos;
                        m_selectEnd      = m_selectStart;
                        setCursor(Qt::ArrowCursor);
                    }

                    // if we're going to resize
                    m_resizing = m_resizeElement && m_resizeId != InvalidIndex32;

                    // store the last clicked position
                    m_mouseLeftClicked   = true;
                    m_lastLeftClickedX   = event->x();
                }
            }
        }

        const bool isZooming = m_mouseLeftClicked == false && m_mouseRightClicked && altPressed;
        const bool isPanning = m_mouseLeftClicked == false && isZooming == false && (m_mouseMidClicked || m_mouseRightClicked);

        if (isPanning)
        {
            setCursor(Qt::SizeHorCursor);
        }

        if (isZooming)
        {
            setCursor(*(m_plugin->GetZoomInCursor()));
        }
    }


    // when releasing the mouse button
    void TrackDataWidget::mouseReleaseEvent(QMouseEvent* event)
    {
        m_plugin->SetRedrawFlag();

        setCursor(Qt::ArrowCursor);

        // disable overwrite mode in any case when the mouse gets released so that we display the current time from the plugin again
        if (m_plugin->GetTimeInfoWidget())
        {
            m_plugin->GetTimeInfoWidget()->SetIsOverwriteMode(false);
        }

        m_lastMouseMoveX = event->x();

        const bool ctrlPressed = event->modifiers() & Qt::ControlModifier;
        //const bool shiftPressed = event->modifiers() & Qt::ShiftModifier;

        if (event->button() == Qt::RightButton)
        {
            m_mouseRightClicked = false;
            m_isScrolling = false;
        }

        if (event->button() == Qt::MidButton)
        {
            m_mouseMidClicked = false;
        }

        if (event->button() == Qt::LeftButton)
        {
            TimeTrack* mouseCursorTrack = m_plugin->GetTrackAt(event->y());
            const bool elementTrackChanged = (mouseCursorTrack && m_dragElementTrack && mouseCursorTrack != m_dragElementTrack);

            if (m_dragging && m_mouseLeftClicked && m_draggingElement && !m_isScrolling && !m_resizing)
            {
                SetPausedTime(aznumeric_cast<float>(m_draggingElement->GetStartTime()));
            }

            if ((m_resizing || m_dragging) && elementTrackChanged == false && m_draggingElement)
            {
                emit MotionEventChanged(m_draggingElement, m_draggingElement->GetStartTime(), m_draggingElement->GetEndTime());
            }

            m_mouseLeftClicked = false;
            m_dragging = false;
            m_resizing = false;
            m_isScrolling = false;

            // rect selection
            if (m_rectSelecting)
            {
                if (m_rectZooming)
                {
                    m_rectZooming = false;

                    // calc the selection rect
                    QRect selectRect;
                    CalcSelectRect(selectRect);

                    // zoom in on the rect
                    if (selectRect.isEmpty() == false)
                    {
                        m_plugin->ZoomRect(selectRect);
                    }
                }
                else
                {
                    // calc the selection rect
                    QRect selectRect;
                    CalcSelectRect(selectRect);

                    // select things inside it
                    if (selectRect.isEmpty() == false)
                    {
                        // rect select the elements
                        const bool overwriteSelection = (ctrlPressed == false);
                        SelectElementsInRect(selectRect, overwriteSelection, true, ctrlPressed);
                    }
                }
            }

            // check if we moved an element to another track
            if (elementTrackChanged && m_draggingElement)
            {
                // lastly fire a signal so that the data can change along with
                emit ElementTrackChanged(m_draggingElement->GetElementNumber(), aznumeric_cast<float>(m_draggingElement->GetStartTime()), aznumeric_cast<float>(m_draggingElement->GetEndTime()), m_dragElementTrack->GetName(), mouseCursorTrack->GetName());
            }
            m_dragElementTrack = nullptr;

            if (m_draggingElement)
            {
                m_draggingElement->SetShowTimeHandles(false);
                m_draggingElement = nullptr;
            }

            // disable rect selection mode again
            m_rectSelecting  = false;
            return;
        }

        // disable rect selection mode again
        m_rectSelecting  = false;

        UpdateMouseOverCursor(event->x(), event->y());
    }

    void TrackDataWidget::ClearState()
    {
        m_dragElementTrack = nullptr;
        m_draggingElement = nullptr;
        m_dragging = false;
        m_resizing = false;
        m_resizeElement = nullptr;
    }

    // the mouse wheel is adjusted
    void TrackDataWidget::DoWheelEvent(QWheelEvent* event, TimeViewPlugin* plugin)
    {
        plugin->SetRedrawFlag();

        // Vertical
        {
            const int numDegrees    = event->angleDelta().y() / 8;
            const int numSteps      = numDegrees / 15;
            float delta             = numSteps / 10.0f;

            double zoomDelta = delta * 4 * MCore::Clamp(plugin->GetTimeScale() / 2.0, 1.0, 22.0);
            plugin->SetScale(plugin->GetTimeScale() + zoomDelta);
        }

        // Horizontal
        {
            const int numDegrees    = event->angleDelta().x() / 8;
            const int numSteps      = numDegrees / 15;
            float delta             = numSteps / 10.0f;

            if (EMotionFX::GetRecorder().GetIsRecording() == false)
            {
                if (delta > 0)
                {
                    delta = 1;
                }
                else
                {
                    delta = -1;
                }

                plugin->DeltaScrollX(-delta * 600);
            }
        }
    }


    // handle mouse wheel event
    void TrackDataWidget::wheelEvent(QWheelEvent* event)
    {
        DoWheelEvent(event, m_plugin);
    }


    // drag & drop support
    void TrackDataWidget::dragEnterEvent(QDragEnterEvent* event)
    {
        m_plugin->SetRedrawFlag();
        m_oldCurrentTime = m_plugin->GetCurrentTime();

        // this is needed to actually reach the drop event function
        event->acceptProposedAction();
    }


    void TrackDataWidget::dragMoveEvent(QDragMoveEvent* event)
    {
        m_plugin->SetRedrawFlag();
        QPoint mousePos = event->pos();

        double dropTime = m_plugin->PixelToTime(mousePos.x());
        m_plugin->SetCurrentTime(dropTime);

        SetPausedTime(aznumeric_cast<float>(dropTime));
    }


    void TrackDataWidget::dropEvent(QDropEvent* event)
    {
        m_plugin->SetRedrawFlag();
        // accept the drop
        event->acceptProposedAction();

        // emit drop event
        emit MotionEventPresetsDropped(event->pos());

        m_plugin->SetCurrentTime(m_oldCurrentTime);
    }


    // the context menu event
    void TrackDataWidget::contextMenuEvent(QContextMenuEvent* event)
    {
        if (m_isScrolling || m_dragging  || m_resizing || !m_allowContextMenu)
        {
            return;
        }

        m_plugin->SetRedrawFlag();

        if (EMotionFX::GetRecorder().GetRecordTime() > MCore::Math::epsilon)
        {
            DoRecorderContextMenuEvent(event);
            return;
        }

        if (m_plugin->m_motion == nullptr)
        {
            return;
        }

        QPoint point = event->pos();
        m_contextMenuX = point.x();
        m_contextMenuY = point.y();

        TimeTrack* timeTrack = m_plugin->GetTrackAt(m_contextMenuY);

        size_t numElements          = 0;
        size_t numSelectedElements  = 0;

        // calculate the number of selected and total events
        const size_t numTracks = m_plugin->GetNumTracks();
        for (size_t i = 0; i < numTracks; ++i)
        {
            // get the current time view track
            TimeTrack* track = m_plugin->GetTrack(i);
            if (track->GetIsVisible() == false)
            {
                continue;
            }

            // get the number of elements in the track and iterate through them
            const size_t numTrackElements = track->GetNumElements();
            for (size_t j = 0; j < numTrackElements; ++j)
            {
                TimeTrackElement* element = track->GetElement(j);
                numElements++;

                if (element->GetIsSelected())
                {
                    numSelectedElements++;
                }
            }
        }

        if (timeTrack)
        {
            numElements = timeTrack->GetNumElements();
            for (size_t i = 0; i < numElements; ++i)
            {
                TimeTrackElement* element = timeTrack->GetElement(i);

                // increase the counter in case the element is selected
                if (element->GetIsSelected())
                {
                    numSelectedElements++;
                }
            }
        }

        QMenu menu(this);

        if (timeTrack)
        {
            TimeTrackElement* element = m_plugin->GetElementAt(m_contextMenuX, m_contextMenuY);
            if (element == nullptr)
            {
                QAction* action = menu.addAction("Add motion event");
                connect(action, &QAction::triggered, this, &TrackDataWidget::OnAddElement);

                // add action to add a motion event which gets its param and type from the selected preset
                EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(MotionEventsPlugin::CLASS_ID);
                if (plugin)
                {
                    MotionEventsPlugin* eventsPlugin = static_cast<MotionEventsPlugin*>(plugin);
                    if (eventsPlugin->CheckIfIsPresetReadyToDrop())
                    {
                        QAction* presetAction = menu.addAction("Add preset event");
                        connect(presetAction, &QAction::triggered, this, &TrackDataWidget::OnCreatePresetEvent);
                    }
                }

                if (timeTrack->GetNumElements() > 0)
                {
                    action = menu.addAction("Cut all events in track");
                    connect(action, &QAction::triggered, this, &TrackDataWidget::OnCutTrack);

                    action = menu.addAction("Copy all events in track");
                    connect(action, &QAction::triggered, this, &TrackDataWidget::OnCopyTrack);
                }

                if (GetIsReadyForPaste())
                {
                    action = menu.addAction("Paste");
                    connect(action, &QAction::triggered, this, &TrackDataWidget::OnPaste);

                    action = menu.addAction("Paste at location");
                    connect(action, &QAction::triggered, this, &TrackDataWidget::OnPasteAtLocation);
                }
            }
            else if (element->GetIsSelected())
            {
                QAction* action = menu.addAction("Cut");
                connect(action, &QAction::triggered, this, &TrackDataWidget::OnCutElement);

                action = menu.addAction("Copy");
                connect(action, &QAction::triggered, this, &TrackDataWidget::OnCopyElement);
            }
        }
        else
        {
            QAction* action = menu.addAction("Add event track");
            connect(action, &QAction::triggered, this, &TrackDataWidget::OnAddTrack);
        }

        // menu entry for removing elements
        if (numSelectedElements > 0)
        {
            // construct the action name
            AZStd::string actionName = "Remove selected event";
            if (numSelectedElements > 1)
            {
                actionName += "s";
            }

            // add the action
            QAction* action = menu.addAction(actionName.c_str());
            connect(action, &QAction::triggered, this, &TrackDataWidget::RemoveSelectedMotionEventsInTrack);
        }

        // menu entry for removing all elements
        if (timeTrack && timeTrack->GetNumElements() > 0)
        {
            QAction* action = menu.addAction("Clear track");
            connect(action, &QAction::triggered, this, &TrackDataWidget::RemoveAllMotionEventsInTrack);
        }

        // Remove track.
        if (timeTrack)
        {
            QAction* action = menu.addAction("Remove track");
            action->setEnabled(timeTrack->GetIsDeletable());
            connect(action, &QAction::triggered, this, &TrackDataWidget::OnRemoveEventTrack);
        }

        // show the menu at the given position
        menu.exec(event->globalPos());
    }


    // propagate key events to the plugin and let it handle by a shared function
    void TrackDataWidget::keyPressEvent(QKeyEvent* event)
    {
        if (m_plugin)
        {
            m_plugin->OnKeyPressEvent(event);
        }
    }


    // propagate key events to the plugin and let it handle by a shared function
    void TrackDataWidget::keyReleaseEvent(QKeyEvent* event)
    {
        if (m_plugin)
        {
            m_plugin->OnKeyReleaseEvent(event);
        }
    }


    void TrackDataWidget::AddMotionEvent(int32 x, int32 y)
    {
        m_plugin->AddMotionEvent(x, y);
    }


    void TrackDataWidget::RemoveMotionEvent(int32 x, int32 y)
    {
        m_plugin->SetRedrawFlag();
        // get the time track on which we dropped the preset
        TimeTrack* timeTrack = m_plugin->GetTrackAt(y);
        if (timeTrack == nullptr)
        {
            return;
        }

        // get the time track on which we dropped the preset
        TimeTrackElement* element = m_plugin->GetElementAt(x, y);
        if (element == nullptr)
        {
            return;
        }

        CommandSystem::CommandHelperRemoveMotionEvent(timeTrack->GetName(), element->GetElementNumber());
    }


    // remove selected motion events in track
    void TrackDataWidget::RemoveSelectedMotionEventsInTrack()
    {
        m_plugin->SetRedrawFlag();
        // get the track where we are at the moment
        TimeTrack* timeTrack = m_plugin->GetTrackAt(m_lastMouseY);
        if (timeTrack == nullptr)
        {
            return;
        }

        AZStd::vector<size_t> eventNumbers;

        // calculate the number of selected events
        const size_t numEvents = timeTrack->GetNumElements();
        for (size_t i = 0; i < numEvents; ++i)
        {
            TimeTrackElement* element = timeTrack->GetElement(i);

            // increase the counter in case the element is selected
            if (element->GetIsSelected())
            {
                eventNumbers.emplace_back(i);
            }
        }

        // remove the motion events
        CommandSystem::CommandHelperRemoveMotionEvents(timeTrack->GetName(), eventNumbers);

        m_plugin->UnselectAllElements();
        ClearState();
    }


    // remove all motion events in track
    void TrackDataWidget::RemoveAllMotionEventsInTrack()
    {
        m_plugin->SetRedrawFlag();

        TimeTrack* timeTrack = m_plugin->GetTrackAt(m_lastMouseY);
        if (timeTrack == nullptr)
        {
            return;
        }

        AZStd::vector<size_t> eventNumbers;

        // construct an array with the event numbers
        const size_t numEvents = timeTrack->GetNumElements();
        for (size_t i = 0; i < numEvents; ++i)
        {
            eventNumbers.emplace_back(i);
        }

        // remove the motion events
        CommandSystem::CommandHelperRemoveMotionEvents(timeTrack->GetName(), eventNumbers);

        m_plugin->UnselectAllElements();
        ClearState();
    }

    void TrackDataWidget::OnRemoveEventTrack()
    {
        const TimeTrack* timeTrack = m_plugin->GetTrackAt(m_lastMouseY);
        if (!timeTrack)
        {
            return;
        }

        const AZ::Outcome<size_t> trackIndexOutcome = m_plugin->FindTrackIndex(timeTrack);
        if (trackIndexOutcome.IsSuccess())
        {
            RemoveTrack(trackIndexOutcome.GetValue());
        }
    }

    void TrackDataWidget::FillCopyElements(bool selectedItemsOnly)
    {
        // clear the array before feeding it
        m_copyElements.clear();

        // get the time track name
        const TimeTrack* timeTrack = m_plugin->GetTrackAt(m_contextMenuY);
        if (timeTrack == nullptr)
        {
            return;
        }
        const AZStd::string trackName = timeTrack->GetName();

        // check if the motion is valid and return failure in case it is not
        const EMotionFX::Motion* motion = m_plugin->GetMotion();
        if (motion == nullptr)
        {
            return;
        }

        // get the motion event table and find the track on which we will work on
        const EMotionFX::MotionEventTable* eventTable = motion->GetEventTable();
        const EMotionFX::MotionEventTrack* eventTrack = eventTable->FindTrackByName(trackName.c_str());
        if (eventTrack == nullptr)
        {
            return;
        }

        // iterate through the elements
        const size_t numElements = timeTrack->GetNumElements();
        MCORE_ASSERT(numElements == eventTrack->GetNumEvents());
        for (size_t i = 0; i < numElements; ++i)
        {
            // get the element and skip all unselected ones
            const TimeTrackElement* element = timeTrack->GetElement(i);
            if (selectedItemsOnly && !element->GetIsSelected())
            {
                continue;
            }

            // get the motion event
            const EMotionFX::MotionEvent& motionEvent = eventTrack->GetEvent(i);

            // create the copy paste element and add it to the array
            m_copyElements.emplace_back(
                motion->GetID(),
                eventTrack->GetNameString(),
                motionEvent.GetEventDatas(),
                motionEvent.GetStartTime(),
                motionEvent.GetEndTime()
            );
        }
    }


    // cut all events from a track
    void TrackDataWidget::OnCutTrack()
    {
        m_plugin->SetRedrawFlag();

        FillCopyElements(false);

        m_cutMode = true;
    }


    // copy all events from a track
    void TrackDataWidget::OnCopyTrack()
    {
        m_plugin->SetRedrawFlag();

        FillCopyElements(false);

        m_cutMode = false;
    }


    // cut motion event
    void TrackDataWidget::OnCutElement()
    {
        m_plugin->SetRedrawFlag();

        FillCopyElements(true);

        m_cutMode = true;
    }


    // copy motion event
    void TrackDataWidget::OnCopyElement()
    {
        m_plugin->SetRedrawFlag();

        FillCopyElements(true);

        m_cutMode = false;
    }


    // paste motion event at the context menu position
    void TrackDataWidget::OnPasteAtLocation()
    {
        DoPaste(true);
    }

    void TrackDataWidget::OnRequiredHeightChanged(int newHeight)
    {
        setMinimumHeight(newHeight);
    }

    // paste motion events at their original positions
    void TrackDataWidget::OnPaste()
    {
        DoPaste(false);
    }


    // paste motion events
    void TrackDataWidget::DoPaste(bool useLocation)
    {
        m_plugin->SetRedrawFlag();

        // get the time track name where we are pasting
        TimeTrack* timeTrack = m_plugin->GetTrackAt(m_contextMenuY);
        if (timeTrack == nullptr)
        {
            return;
        }
        AZStd::string trackName = timeTrack->GetName();

        // get the number of elements to copy
        const size_t numElements = m_copyElements.size();

        // create the command group
        MCore::CommandGroup commandGroup("Paste motion events");

        // find the min and maximum time values of the events to paste
        auto [minEvent, maxEvent] = AZStd::minmax_element(begin(m_copyElements), end(m_copyElements), [](const CopyElement& left, const CopyElement& right)
        {
            return left.m_startTime < right.m_startTime;
        });

        if (m_cutMode)
        {
            // iterate through the copy elements from back to front and delete the selected ones
            for (int32 i = static_cast<int32>(numElements) - 1; i >= 0; i--)
            {
                const CopyElement& copyElement = m_copyElements[i];

                // get the motion to which the original element belongs to
                EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByID(copyElement.m_motionID);
                if (motion == nullptr)
                {
                    continue;
                }

                // get the motion event table and track
                EMotionFX::MotionEventTable* eventTable  = motion->GetEventTable();
                EMotionFX::MotionEventTrack* eventTrack  = eventTable->FindTrackByName(copyElement.m_trackName.c_str());
                if (eventTrack == nullptr)
                {
                    continue;
                }

                // get the number of events and iterate through them
                size_t eventNr = InvalidIndex;
                const size_t numEvents = eventTrack->GetNumEvents();
                for (eventNr = 0; eventNr < numEvents; ++eventNr)
                {
                    EMotionFX::MotionEvent& motionEvent = eventTrack->GetEvent(eventNr);
                    if (MCore::Compare<float>::CheckIfIsClose(motionEvent.GetStartTime(), copyElement.m_startTime, MCore::Math::epsilon) &&
                        MCore::Compare<float>::CheckIfIsClose(motionEvent.GetEndTime(), copyElement.m_endTime, MCore::Math::epsilon) &&
                        copyElement.m_eventDatas == motionEvent.GetEventDatas())
                    {
                        break;
                    }
                }

                // remove event
                if (eventNr != InvalidIndex)
                {
                    CommandSystem::CommandHelperRemoveMotionEvent(copyElement.m_motionID, copyElement.m_trackName.c_str(), eventNr, &commandGroup);
                }
            }
        }

        const float offset = useLocation ? aznumeric_cast<float>(m_plugin->PixelToTime(m_contextMenuX, true)) - minEvent->m_startTime : 0.0f;

        // iterate through the elements to copy and add the new motion events
        for (const CopyElement& copyElement : m_copyElements)
        {
            float startTime = copyElement.m_startTime + offset;
            float endTime   = copyElement.m_endTime + offset;

            CommandSystem::CommandHelperAddMotionEvent(trackName.c_str(), startTime, endTime, copyElement.m_eventDatas, &commandGroup);
        }

        // execute the group command
        AZStd::string outResult;
        if (GetCommandManager()->ExecuteCommandGroup(commandGroup, outResult) == false)
        {
            MCore::LogError(outResult.c_str());
        }

        if (m_cutMode)
        {
            m_copyElements.clear();
        }
    }


    void TrackDataWidget::OnCreatePresetEvent()
    {
        m_plugin->SetRedrawFlag();
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(MotionEventsPlugin::CLASS_ID);
        if (plugin == nullptr)
        {
            return;
        }

        MotionEventsPlugin* eventsPlugin = static_cast<MotionEventsPlugin*>(plugin);

        QPoint mousePos(m_contextMenuX, m_contextMenuY);
        eventsPlugin->OnEventPresetDropped(mousePos);
    }

    void TrackDataWidget::OnAddTrack()
    {
        m_plugin->SetRedrawFlag();
        CommandSystem::CommandAddEventTrack();
    }

    // select all elements within a given rect
    void TrackDataWidget::SelectElementsInRect(const QRect& rect, bool overwriteCurSelection, bool select, bool toggleMode)
    {
        // get the number of tracks and iterate through them
        const size_t numTracks = m_plugin->GetNumTracks();
        for (size_t i = 0; i < numTracks; ++i)
        {
            // get the current time track
            TimeTrack* track = m_plugin->GetTrack(i);
            if (track->GetIsVisible() == false)
            {
                continue;
            }

            // select all elements in rect for this track
            track->SelectElementsInRect(rect, overwriteCurSelection, select, toggleMode);
        }
    }


    bool TrackDataWidget::event(QEvent* event)
    {
        if (event->type() == QEvent::ToolTip)
        {
            QHelpEvent* helpEvent = static_cast<QHelpEvent*>(event);

            QPoint localPos     = helpEvent->pos();
            QPoint tooltipPos   = helpEvent->globalPos();

            // get the position
            if (localPos.y() < 0)
            {
                return QOpenGLWidget::event(event);
            }

            // if we have a recording
            if (EMotionFX::GetRecorder().GetRecordTime() > MCore::Math::epsilon)
            {
                EMotionFX::Recorder::NodeHistoryItem* motionItem = FindNodeHistoryItem(FindActorInstanceData(), localPos.x(), localPos.y());
                if (motionItem)
                {
                    AZStd::string toolTipString;
                    BuildToolTipString(motionItem, toolTipString);

                    QRect toolTipRect(tooltipPos.x() - 4, tooltipPos.y() - 4, 8, 8);
                    QToolTip::showText(tooltipPos, toolTipString.c_str(), this, toolTipRect);
                }
                else
                {
                    EMotionFX::Recorder::EventHistoryItem* eventItem = FindEventHistoryItem(FindActorInstanceData(), localPos.x(), localPos.y());
                    if (eventItem)
                    {
                        AZStd::string toolTipString;
                        BuildToolTipString(eventItem, toolTipString);

                        QRect toolTipRect(tooltipPos.x() - 4, tooltipPos.y() - 4, 8, 8);
                        QToolTip::showText(tooltipPos, toolTipString.c_str(), this, toolTipRect);
                    }
                }
            }
            else
            {
                // get the hovered element and track
                TimeTrackElement*   element = m_plugin->GetElementAt(localPos.x(), localPos.y());
                if (element == nullptr)
                {
                    return QOpenGLWidget::event(event);
                }

                QString toolTipString;
                if (element)
                {
                    toolTipString = element->GetToolTip();
                }

                QRect toolTipRect(tooltipPos.x() - 4, tooltipPos.y() - 4, 8, 8);
                QToolTip::showText(tooltipPos, toolTipString, this, toolTipRect);
            }
        }

        return QOpenGLWidget::event(event);
    }

    // update the rects
    void TrackDataWidget::UpdateRects()
    {
        EMotionFX::Recorder& recorder = EMotionFX::GetRecorder();

        // get the actor instance data for the first selected actor instance, and render the node history for that
        const EMotionFX::Recorder::ActorInstanceData* actorInstanceData = FindActorInstanceData();

        // if we recorded node history
        m_nodeHistoryRect = QRect();
        if (actorInstanceData && !actorInstanceData->m_nodeHistoryItems.empty())
        {
            const int height = aznumeric_caster((recorder.CalcMaxNodeHistoryTrackIndex(*actorInstanceData) + 1) * (m_nodeHistoryItemHeight + 3) + m_nodeRectsStartHeight);
            m_nodeHistoryRect.setTop(m_nodeRectsStartHeight);
            m_nodeHistoryRect.setBottom(height);
            m_nodeHistoryRect.setLeft(0);
            m_nodeHistoryRect.setRight(geometry().width());
        }

        m_eventHistoryTotalHeight = 0;
        if (actorInstanceData && !actorInstanceData->m_eventHistoryItems.empty())
        {
            m_eventHistoryTotalHeight = aznumeric_caster((recorder.CalcMaxEventHistoryTrackIndex(*actorInstanceData) + 1) * 20);
        }
    }


    // find the node history item at a given mouse location
    EMotionFX::Recorder::NodeHistoryItem* TrackDataWidget::FindNodeHistoryItem(EMotionFX::Recorder::ActorInstanceData* actorInstanceData, int32 x, int32 y)
    {
        if (actorInstanceData == nullptr)
        {
            return nullptr;
        }

        if (hasFocus() == false)
        {
            return nullptr;
        }

        // make sure the m_trackRemap array is up to date
        RecorderGroup* recorderGroup = m_plugin->GetTimeViewToolBar()->GetRecorderGroup();
        const bool sorted = recorderGroup->GetSortNodeActivity();
        const int graphContentsCode = m_plugin->m_trackHeaderWidget->m_nodeContentsComboBox->currentIndex();
        EMotionFX::GetRecorder().ExtractNodeHistoryItems(*actorInstanceData, aznumeric_cast<float>(m_plugin->m_curTime), sorted, (EMotionFX::Recorder::EValueType)graphContentsCode, &m_activeItems, &m_trackRemap);


        // get the history items shortcut
        const AZStd::vector<EMotionFX::Recorder::NodeHistoryItem*>&  historyItems = actorInstanceData->m_nodeHistoryItems;

        QRect rect;
        for (EMotionFX::Recorder::NodeHistoryItem* curItem : historyItems)
        {
            // draw the background rect
            double startTimePixel   = m_plugin->TimeToPixel(curItem->m_startTime);
            double endTimePixel     = m_plugin->TimeToPixel(curItem->m_endTime);

            if (startTimePixel > x || endTimePixel < x)
            {
                continue;
            }

            rect.setLeft(aznumeric_cast<int>(startTimePixel));
            rect.setRight(aznumeric_cast<int>(endTimePixel));
            rect.setTop((m_nodeRectsStartHeight + (aznumeric_cast<uint32>(m_trackRemap[curItem->m_trackIndex]) * (m_nodeHistoryItemHeight + 3)) + 3));
            rect.setBottom(rect.top() + m_nodeHistoryItemHeight);

            if (rect.contains(x, y))
            {
                return curItem;
            }
        }

        return nullptr;
    }


    // find the actor instance data for the current selection
    EMotionFX::Recorder::ActorInstanceData* TrackDataWidget::FindActorInstanceData() const
    {
        EMotionFX::Recorder& recorder = EMotionFX::GetRecorder();

        // find the selected actor instance
        EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
        if (actorInstance == nullptr)
        {
            return nullptr;
        }

        // find the actor instance data for this actor instance
        const size_t actorInstanceDataIndex = recorder.FindActorInstanceDataIndex(actorInstance);
        if (actorInstanceDataIndex == InvalidIndex) // it doesn't exist, so we didn't record anything for this actor instance
        {
            return nullptr;
        }

        // get the actor instance data for the first selected actor instance, and render the node history for that
        return &recorder.GetActorInstanceData(actorInstanceDataIndex);
    }


    // context event when recorder has a recording
    void TrackDataWidget::DoRecorderContextMenuEvent(QContextMenuEvent* event)
    {
        QPoint point = event->pos();
        m_contextMenuX = point.x();
        m_contextMenuY = point.y();

        // create the context menu
        QMenu menu(this);

        //---------------------
        // Timeline actions
        //---------------------
        QAction* action = menu.addAction("Zoom To Fit All");
        connect(action, &QAction::triggered, m_plugin, &TimeViewPlugin::OnZoomAll);

        action = menu.addAction("Reset Timeline");
        connect(action, &QAction::triggered, m_plugin, &TimeViewPlugin::OnResetTimeline);

        //---------------------
        // Right-clicked on a motion item
        //---------------------
        EMotionFX::Recorder::NodeHistoryItem* historyItem = FindNodeHistoryItem(FindActorInstanceData(), point.x(), point.y());
        if (historyItem)
        {
            menu.addSeparator();

            action = menu.addAction("Show Node In Graph");
            connect(action, &QAction::triggered, m_plugin, &TimeViewPlugin::OnShowNodeHistoryNodeInGraph);
        }

        // show the menu at the given position
        menu.exec(event->globalPos());
    }

    // build a tooltip for a node history item
    void TrackDataWidget::BuildToolTipString(EMotionFX::Recorder::NodeHistoryItem* item, AZStd::string& outString)
    {
        outString = "<table border=\"0\">";

        // node name
        outString += AZStd::string::format("<tr><td width=\"150\"><p style=\"color:rgb(200,200,200)\"><b>Node Name:&nbsp;</b></p></td>");
        outString += AZStd::string::format("<td width=\"400\"><p style=\"color:rgb(115, 115, 115)\">%s</p></td></tr>", item->m_name.c_str());

        // build the node path string
        EMotionFX::ActorInstance* actorInstance = FindActorInstanceData()->m_actorInstance;
        EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();
        if (animGraphInstance)
        {
            EMotionFX::AnimGraph* animGraph = animGraphInstance->GetAnimGraph();
            EMotionFX::AnimGraphNode* node = animGraph->RecursiveFindNodeById(item->m_nodeId);
            if (node)
            {
                AZStd::vector<EMotionFX::AnimGraphNode*> nodePath;
                EMotionFX::AnimGraphNode* curNode = node->GetParentNode();
                while (curNode)
                {
                    nodePath.emplace(nodePath.begin(), curNode);
                    curNode = curNode->GetParentNode();
                }

                AZStd::string nodePathString;
                nodePathString.reserve(256);
                for (const EMotionFX::AnimGraphNode* parentNode : nodePath)
                {
                    if (!nodePathString.empty())
                    {
                        nodePathString += " > ";
                    }
                    nodePathString += parentNode->GetName();
                }

                outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Node Path:&nbsp;</b></p></td>");
                outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%s</p></td></tr>", nodePathString.c_str());

                outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Node Type:&nbsp;</b></p></td>");
                outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%s</p></td></tr>", node->RTTI_GetTypeName());

                outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Parent Type:&nbsp;</b></p></td>");
                outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%s</p></td></tr>", node->GetParentNode()->RTTI_GetTypeName());

                if (node->GetNumChildNodes() > 0)
                {
                    outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Child Nodes:&nbsp;</b></p></td>");
                    outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%zu</p></td></tr>", node->GetNumChildNodes());

                    outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Recursive Children:&nbsp;</b></p></td>");
                    outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%zu</p></td></tr>", node->RecursiveCalcNumNodes());
                }
            }
        }

        // motion name
        if (item->m_motionId != InvalidIndex32 && !item->m_motionFileName.empty())
        {
            outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Motion FileName:&nbsp;</b></p></td>");
            outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%s</p></td></tr>", item->m_motionFileName.c_str());

            // show motion info
            EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByID(item->m_motionId);
            if (motion)
            {
                AZStd::string path;
                AzFramework::StringFunc::Path::GetFolderPath(motion->GetFileNameString().c_str(), path);
                EMotionFX::GetEMotionFX().GetFilenameRelativeToMediaRoot(&path);

                outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Motion Path:&nbsp;</b></p></td>");
                outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%s</p></td></tr>", path.c_str());

                outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Motion Type:&nbsp;</b></p></td>");
                outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%s</p></td></tr>", motion->GetMotionData()->RTTI_GetTypeName());

                outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Motion Duration:&nbsp;</b></p></td>");
                outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%.3f seconds</p></td></tr>", motion->GetDuration());

                outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Event Tracks:&nbsp;</b></p></td>");
                outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%zu</p></td></tr>", motion->GetEventTable()->GetNumTracks());
            }
            else
            {
                outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Motion FileName:&nbsp;</b></p></td>");
                outString += AZStd::string::format("<td><p style=\"color:rgb(255, 0, 0)\">%s</p></td></tr>", "<not loaded anymore>");
            }
        }

        outString += "</table>";
    }


    // find the item at a given location
    EMotionFX::Recorder::EventHistoryItem* TrackDataWidget::FindEventHistoryItem(EMotionFX::Recorder::ActorInstanceData* actorInstanceData, int32 x, int32 y) const
    {
        if (actorInstanceData == nullptr)
        {
            return nullptr;
        }

        if (hasFocus() == false)
        {
            return nullptr;
        }

        const AZStd::vector<EMotionFX::Recorder::EventHistoryItem*>& historyItems = actorInstanceData->m_eventHistoryItems;
        const float tickHalfWidth = 7;
        const float tickHeight = 16;

        for (EMotionFX::Recorder::EventHistoryItem* curItem : historyItems)
        {
            float height = aznumeric_caster((curItem->m_trackIndex * 20) + m_eventsStartHeight);
            double startTimePixel   = m_plugin->TimeToPixel(curItem->m_startTime);

            const QRect rect(QPoint(aznumeric_cast<int>(startTimePixel - tickHalfWidth), aznumeric_cast<int>(height)), QSize(aznumeric_cast<int>(tickHalfWidth * 2), aznumeric_cast<int>(tickHeight)));
            if (rect.contains(QPoint(x, y)))
            {
                return curItem;
            }
        }

        return nullptr;
    }


    // build a tooltip for an event history item
    void TrackDataWidget::BuildToolTipString(EMotionFX::Recorder::EventHistoryItem* item, AZStd::string& outString)
    {
        outString = "<table border=\"0\">";

        const EMotionFX::MotionEvent* motionEvent = item->m_eventInfo.m_event;
        for (const EMotionFX::EventDataPtr& eventData : motionEvent->GetEventDatas())
        {
            if (eventData)
            {
                const auto& motionDataProperties = MCore::ReflectionSerializer::SerializeIntoMap(eventData.get());
                if (motionDataProperties.IsSuccess())
                {
                    for (const AZStd::pair<AZStd::string, AZStd::string>& keyValuePair : motionDataProperties.GetValue())
                    {
                        outString += AZStd::string::format(
                            "<tr><td><p style=\"color:rgb(200, 200, 200)\"><b>%s:&nbsp;</b></p></td>" // no comma, concat these 2 string literals
                            "<td><p style=\"color:rgb(115, 115, 115)\">%s</p></td></tr>",
                            keyValuePair.first.c_str(),
                            keyValuePair.second.c_str()
                        );
                    }
                }
            }
        }

        outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Event ID:&nbsp;</b></p></td>");

        outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Local Event Time:&nbsp;</b></p></td>");
        outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%.3f seconds</p></td></tr>", item->m_eventInfo.m_timeValue);

        outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Event Trigger Time:&nbsp;</b></p></td>");
        outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%.3f seconds</p></td></tr>", item->m_startTime);

        outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Is Ranged Event:&nbsp;</b></p></td>");
        outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%s</p></td></tr>", (item->m_isTickEvent == false) ? "Yes" : "No");

        if (item->m_isTickEvent == false)
        {
            const static AZStd::string eventStartText = "Event Start";
            const static AZStd::string eventActiveText = "Event Active";
            const static AZStd::string eventEndText = "Event End";
            const AZStd::string* outputEventStateText = &eventStartText;
            if (item->m_eventInfo.m_eventState == EMotionFX::EventInfo::EventState::ACTIVE)
            {
                outputEventStateText = &eventActiveText;
            }
            else if (item->m_eventInfo.m_eventState == EMotionFX::EventInfo::EventState::END)
            {
                outputEventStateText = &eventEndText;
            }
            outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Ranged Info:&nbsp;</b></p></td>");
            outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%s</p></td></tr>", outputEventStateText->c_str());
        }

        outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Global Weight:&nbsp;</b></p></td>");
        outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%.3f</p></td></tr>", item->m_eventInfo.m_globalWeight);

        outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Local Weight:&nbsp;</b></p></td>");
        outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%.3f</p></td></tr>", item->m_eventInfo.m_localWeight);

        // build the node path string
        EMotionFX::ActorInstance* actorInstance = FindActorInstanceData()->m_actorInstance;
        EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();
        if (animGraphInstance)
        {
            EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(item->m_animGraphId);//animGraphInstance->GetAnimGraph();
            EMotionFX::AnimGraphNode* node = animGraph->RecursiveFindNodeById(item->m_emitterNodeId);
            if (node)
            {
                outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Emitted By:&nbsp;</b></p></td>");
                outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%s</p></td></tr>", node->GetName());

                AZStd::vector<EMotionFX::AnimGraphNode*> nodePath;
                EMotionFX::AnimGraphNode* curNode = node->GetParentNode();
                while (curNode)
                {
                    nodePath.emplace(0, curNode);
                    curNode = curNode->GetParentNode();
                }

                AZStd::string nodePathString;
                for (const EMotionFX::AnimGraphNode* parentNode : nodePath)
                {
                    if (!nodePathString.empty())
                    {
                        nodePathString += " > ";
                    }
                    nodePathString += parentNode->GetName();
                }

                outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Node Path:&nbsp;</b></p></td>");
                outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%s</p></td></tr>", nodePathString.c_str());

                outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Node Type:&nbsp;</b></p></td>");
                outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%s</p></td></tr>", node->RTTI_GetTypeName());

                outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Parent Type:&nbsp;</b></p></td>");
                outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%s</p></td></tr>", node->GetParentNode()->RTTI_GetTypeName());

                if (node->GetNumChildNodes() > 0)
                {
                    outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Child Nodes:&nbsp;</b></p></td>");
                    outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%zu</p></td></tr>", node->GetNumChildNodes());

                    outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Recursive Children:&nbsp;</b></p></td>");
                    outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%zu</p></td></tr>", node->RecursiveCalcNumNodes());
                }

                // show the motion info
                if (azrtti_typeid(node) == azrtti_typeid<EMotionFX::AnimGraphMotionNode>())
                {
                    EMotionFX::AnimGraphMotionNode* motionNode = static_cast<EMotionFX::AnimGraphMotionNode*>(node);
                    EMotionFX::MotionInstance* motionInstance = motionNode->FindMotionInstance(animGraphInstance);
                    if (motionInstance)
                    {
                        EMotionFX::Motion* motion = motionInstance->GetMotion();
                        if (motion)
                        {
                            outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Motion FileName:&nbsp;</b></p></td>");
                            AZStd::string filename;
                            AzFramework::StringFunc::Path::GetFileName(motion->GetFileName(), filename);
                            outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%s</p></td></tr>", filename.c_str());

                            outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Motion Type:&nbsp;</b></p></td>");
                            outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%s</p></td></tr>", motion->GetMotionData()->RTTI_GetTypeName());

                            outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Motion Duration:&nbsp;</b></p></td>");
                            outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%.3f seconds</p></td></tr>", motion->GetDuration());

                            outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Event Tracks:&nbsp;</b></p></td>");
                            outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%zu</p></td></tr>", motion->GetEventTable()->GetNumTracks());
                        }
                        else
                        {
                            outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Motion FileName:&nbsp;</b></p></td>");
                            outString += AZStd::string::format("<td><p style=\"color:rgb(255, 0, 0)\">%s</p></td></tr>", "<not loaded anymore>");
                        }
                    }
                }
            }
        }

        outString += "</table>";
    }

    // paint a title bar
    uint32 TrackDataWidget::PaintSeparator(QPainter& painter, int32 heightOffset, float animationLength)
    {
        painter.setPen(QColor(60, 70, 80));
        painter.setBrush(Qt::NoBrush);
        painter.drawLine(QPoint(0, heightOffset), QPoint(aznumeric_cast<int>(m_plugin->TimeToPixel(animationLength)), heightOffset));
        return 1;
    }
}   // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/TimeView/moc_TrackDataWidget.cpp>
