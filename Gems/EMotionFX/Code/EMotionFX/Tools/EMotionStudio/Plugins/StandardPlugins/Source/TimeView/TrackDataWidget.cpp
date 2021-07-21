/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        , mBrushBackground(QColor(40, 45, 50), Qt::SolidPattern)
        , mBrushBackgroundClipped(QColor(40, 40, 40), Qt::SolidPattern)
        , mBrushBackgroundOutOfRange(QColor(35, 35, 35), Qt::SolidPattern)
        , mPlugin(plugin)
        , mMouseLeftClicked(false)
        , mMouseMidClicked(false)
        , mMouseRightClicked(false)
        , mDragging(false)
        , mResizing(false)
        , mRectZooming(false)
        , mIsScrolling(false)
        , mLastLeftClickedX(0)
        , mLastMouseMoveX(0)
        , mLastMouseX(0)
        , mLastMouseY(0)
        , mNodeHistoryItemHeight(20)
        , mEventHistoryTotalHeight(0)
        , mAllowContextMenu(true)
        , mDraggingElement(nullptr)
        , mDragElementTrack(nullptr)
        , mResizeElement(nullptr)
        , mGraphStartHeight(0)
        , mEventsStartHeight(0)
        , mNodeRectsStartHeight(0)
        , mSelectStart(0, 0)
        , mSelectEnd(0, 0)
        , mRectSelecting(false)
    {
        setObjectName("TrackDataWidget");

        mDataFont.setPixelSize(13);

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
        if (mPlugin)
        {
            mPlugin->SetRedrawFlag();
        }
    }


    // calculate the selection rect
    void TrackDataWidget::CalcSelectRect(QRect& outRect)
    {
        const int32 startX = MCore::Min<int32>(mSelectStart.x(), mSelectEnd.x());
        const int32 startY = MCore::Min<int32>(mSelectStart.y(), mSelectEnd.y());
        const int32 width  = abs(mSelectEnd.x() - mSelectStart.x());
        const int32 height = abs(mSelectEnd.y() - mSelectStart.y());

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
        painter.setBrush(mBrushBackgroundOutOfRange);
        painter.drawRect(rect);
        painter.setFont(mDataFont);

        // if there is a recording show that, otherwise show motion tracks
        switch (mPlugin->GetMode())
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

        mPlugin->RenderElementTimeHandles(painter, geometry().height(), mPlugin->mPenTimeHandles);

        DrawTimeMarker(painter, rect);

        // render selection rect
        if (mRectSelecting)
        {
            painter.resetTransform();
            QRect selectRect;
            CalcSelectRect(selectRect);

            if (mRectZooming)
            {
                painter.setBrush(QColor(0, 100, 200, 75));
                painter.setPen(QColor(0, 100, 255));
                painter.drawRect(selectRect);
            }
            else
            {
                if (EMotionFX::GetRecorder().GetRecordTime() < MCore::Math::epsilon && mPlugin->mMotion)
                {
                    painter.setBrush(QColor(200, 120, 0, 75));
                    painter.setPen(QColor(255, 128, 0));
                    painter.drawRect(selectRect);
                }
            }
        }
    }

    void TrackDataWidget::RemoveTrack(AZ::u32 trackIndex)
    {
        mPlugin->SetRedrawFlag();
        CommandSystem::CommandRemoveEventTrack(trackIndex);
        mPlugin->UnselectAllElements();
        ClearState();
    }

    // draw the time marker
    void TrackDataWidget::DrawTimeMarker(QPainter& painter, const QRect& rect)
    {
        // draw the current time marker
        float startHeight = 0.0f;
        const float curTimeX = aznumeric_cast<float>(mPlugin->TimeToPixel(mPlugin->mCurTime));
        painter.setPen(mPlugin->mPenCurTimeHandle);
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
        const double animEndPixel = mPlugin->TimeToPixel(animationLength);
        backgroundRect.setLeft(aznumeric_cast<int>(animEndPixel));
        motionRect.setRight(aznumeric_cast<int>(animEndPixel));
        motionRect.setTop(0);
        backgroundRect.setTop(0);

        // render the rects
        painter.setPen(Qt::NoPen);
        painter.setBrush(mBrushBackground);
        painter.drawRect(motionRect);
        painter.setBrush(mBrushBackgroundOutOfRange);
        painter.drawRect(backgroundRect);

        // find the selected actor instance
        EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
        if (actorInstance == nullptr)
        {
            return;
        }

        // find the actor instance data for this actor instance
        const uint32 actorInstanceDataIndex = recorder.FindActorInstanceDataIndex(actorInstance);
        if (actorInstanceDataIndex == MCORE_INVALIDINDEX32) // it doesn't exist, so we didn't record anything for this actor instance
        {
            return;
        }

        // get the actor instance data for the first selected actor instance, and render the node history for that
        const EMotionFX::Recorder::ActorInstanceData* actorInstanceData = &recorder.GetActorInstanceData(actorInstanceDataIndex);

        RecorderGroup* recorderGroup = mPlugin->GetTimeViewToolBar()->GetRecorderGroup();
        const bool displayNodeActivity = recorderGroup->GetDisplayNodeActivity();
        const bool displayEvents = recorderGroup->GetDisplayMotionEvents();
        const bool displayRelativeGraph = recorderGroup->GetDisplayRelativeGraph();

        int32 startOffset = 0;
        int32 requiredHeight = 0;
        bool isTop = true;

        if (displayNodeActivity)
        {
            mNodeRectsStartHeight = startOffset;
            PaintRecorderNodeHistory(painter, rect, actorInstanceData);
            isTop = false;
            startOffset = mNodeHistoryRect.bottom();
            requiredHeight = mNodeHistoryRect.bottom();
        }

        if (displayEvents)
        {
            if (isTop == false)
            {
                mEventsStartHeight = startOffset;
                mEventsStartHeight += PaintSeparator(painter, mEventsStartHeight, animationLength);
                mEventsStartHeight += 10;
                startOffset = mEventsStartHeight;
                requiredHeight += 11;
            }
            else
            {
                startOffset += 3;
                mEventsStartHeight = startOffset;
                requiredHeight += 3;
            }

            startOffset += mEventHistoryTotalHeight;
            isTop = false;

            PaintRecorderEventHistory(painter, rect, actorInstanceData);
        }

        if (displayRelativeGraph)
        {
            if (isTop == false)
            {
                mGraphStartHeight = startOffset + 10;
                mGraphStartHeight += PaintSeparator(painter, mGraphStartHeight, animationLength);
                startOffset = mGraphStartHeight;
                requiredHeight += 11;
            }
            else
            {
                startOffset += 3;
                mGraphStartHeight = startOffset;
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
        const MCore::Array<EMotionFX::Recorder::NodeHistoryItem*>& historyItems = actorInstanceData->mNodeHistoryItems;
        int32 windowWidth = geometry().width();

        RecorderGroup* recorderGroup = mPlugin->GetTimeViewToolBar()->GetRecorderGroup();
        const bool useNodeColors = recorderGroup->GetUseNodeTypeColors();
        const bool limitGraphHeight = recorderGroup->GetLimitGraphHeight();
        const bool showNodeNames = mPlugin->mTrackHeaderWidget->mNodeNamesCheckBox->isChecked();
        const bool showMotionFiles = mPlugin->mTrackHeaderWidget->mMotionFilesCheckBox->isChecked();
        const bool interpolate = recorder.GetRecordSettings().mInterpolate;

        float graphHeight = aznumeric_cast<float>(geometry().height() - mGraphStartHeight);
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

            graphBottom = mGraphStartHeight + graphHeight;
        }

        const uint32 graphContentsCode = mPlugin->mTrackHeaderWidget->mGraphContentsComboBox->currentIndex();

        const uint32 numItems = historyItems.GetLength();
        for (uint32 i = 0; i < numItems; ++i)
        {
            EMotionFX::Recorder::NodeHistoryItem* curItem = historyItems[i];

            double startTimePixel   = mPlugin->TimeToPixel(curItem->mStartTime);
            double endTimePixel     = mPlugin->TimeToPixel(curItem->mEndTime);

            const QRect itemRect(QPoint(aznumeric_cast<int>(startTimePixel), mGraphStartHeight), QPoint(aznumeric_cast<int>(endTimePixel), geometry().height()));
            if (rect.intersects(itemRect) == false)
            {
                continue;
            }

            const AZ::Color colorCode = (useNodeColors) ? curItem->mTypeColor : curItem->mColor;
            QColor color;
            color.setRgbF(colorCode.GetR(), colorCode.GetG(), colorCode.GetB(), colorCode.GetA());

            if (mPlugin->mNodeHistoryItem != curItem || mIsScrolling || mPlugin->mIsAnimating)
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
                EMotionFX::KeyTrackLinearDynamic<float, float>* keyTrack = &curItem->mGlobalWeights; // init on global weights

                if (graphContentsCode == 1)
                {
                    keyTrack = &curItem->mLocalWeights;
                }
                else
                if (graphContentsCode == 2)
                {
                    keyTrack = &curItem->mPlayTimes;
                }

                float lastWeight = keyTrack->GetValueAtTime(0.0f, &curItem->mCachedKey, nullptr, interpolate);
                const float keyTimeStep = (curItem->mEndTime - curItem->mStartTime) / (float)widthInPixels;

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

                    const float weight = keyTrack->GetValueAtTime(w * keyTimeStep, &curItem->mCachedKey, nullptr, interpolate);
                    const float height = graphBottom - weight * graphHeight;
                    path.lineTo(QPointF(startTimePixel + w + 1, height));
                }

                const float weight = keyTrack->GetValueAtTime(curItem->mEndTime, &curItem->mCachedKey, nullptr, interpolate);
                const float height = graphBottom - weight * graphHeight;
                path.lineTo(QPointF(startTimePixel + widthInPixels - 1, height));
                path.lineTo(QPointF(startTimePixel + widthInPixels, graphBottom + 1));
                painter.drawPath(path);
            }
        }

        // calculate the remapped track list, based on sorted global weight, with the most influencing track on top
        recorder.ExtractNodeHistoryItems(*actorInstanceData, aznumeric_cast<float>(mPlugin->mCurTime), true, (EMotionFX::Recorder::EValueType)graphContentsCode, &mActiveItems, &mTrackRemap);

        // display the values and names
        uint32 offset = 0;
        const uint32 numActiveItems = mActiveItems.GetLength();
        for (uint32 i = 0; i < numActiveItems; ++i)
        {
            EMotionFX::Recorder::NodeHistoryItem* curItem = mActiveItems[i].mNodeHistoryItem;
            if (curItem == nullptr)
            {
                continue;
            }

            offset += 15;

            mTempString.clear();
            if (showNodeNames)
            {
                mTempString += curItem->mName.c_str();
            }

            if (showMotionFiles && curItem->mMotionFileName.size() > 0)
            {
                if (!mTempString.empty())
                {
                    mTempString += " - ";
                }

                mTempString += curItem->mMotionFileName.c_str();
            }

            if (!mTempString.empty())
            {
                mTempString += AZStd::string::format(" = %.4f", mActiveItems[i].mValue);
            }
            else
            {
                mTempString = AZStd::string::format("%.4f", mActiveItems[i].mValue);
            }

            const AZ::Color colorCode = (useNodeColors) ? mActiveItems[i].mNodeHistoryItem->mTypeColor : mActiveItems[i].mNodeHistoryItem->mColor;
            QColor color;
            color.setRgbF(colorCode.GetR(), colorCode.GetG(), colorCode.GetB(), colorCode.GetA());

            painter.setPen(color);
            painter.setBrush(Qt::NoBrush);
            painter.setFont(mDataFont);
            painter.drawText(3, offset + mGraphStartHeight, mTempString.c_str());
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
        const MCore::Array<EMotionFX::Recorder::EventHistoryItem*>& historyItems = actorInstanceData->mEventHistoryItems;

        QRect clipRect = rect;
        clipRect.setRight(aznumeric_cast<int>(mPlugin->TimeToPixel(animationLength)));
        painter.setClipRect(clipRect);
        painter.setClipping(true);

        // for all event history items
        const float tickHalfWidth = 7;
        const float tickHeight = 16;

        QPointF tickPoints[6];
        const uint32 numItems = historyItems.GetLength();
        for (uint32 i = 0; i < numItems; ++i)
        {
            EMotionFX::Recorder::EventHistoryItem* curItem = historyItems[i];

            float height = aznumeric_cast<float>((curItem->mTrackIndex * 20) + mEventsStartHeight);
            double startTimePixel   = mPlugin->TimeToPixel(curItem->mStartTime);

            const QRect itemRect(QPoint(aznumeric_cast<int>(startTimePixel - tickHalfWidth), aznumeric_cast<int>(height)), QSize(aznumeric_cast<int>(tickHalfWidth * 2), aznumeric_cast<int>(tickHeight)));
            if (rect.intersects(itemRect) == false)
            {
                continue;
            }

            // try to locate the node based on its unique ID
            QColor borderColor(30, 30, 30);
            const AZ::Color& colorCode = curItem->mColor;
            QColor color;
            color.setRgbF(colorCode.GetR(), colorCode.GetG(), colorCode.GetB(), colorCode.GetA());
            
            if (mIsScrolling == false && mPlugin->mIsAnimating == false)
            {
                if (mPlugin->mNodeHistoryItem && mPlugin->mNodeHistoryItem->mNodeId == curItem->mEmitterNodeId)
                {
                    if (curItem->mStartTime >= mPlugin->mNodeHistoryItem->mStartTime && curItem->mStartTime <= mPlugin->mNodeHistoryItem->mEndTime)
                    {
                        RecorderGroup* recorderGroup = mPlugin->GetTimeViewToolBar()->GetRecorderGroup();
                        if (recorderGroup->GetDisplayNodeActivity())
                        {
                            borderColor = QColor(255, 128, 0);
                            color = QColor(255, 128, 0);
                        }
                    }
                }

                if (mPlugin->mEventHistoryItem == curItem)
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
        if (!rect.intersects(mNodeHistoryRect))
        {
            return;
        }

        // get the history items shortcut
        const MCore::Array<EMotionFX::Recorder::NodeHistoryItem*>&  historyItems = actorInstanceData->mNodeHistoryItems;
        int32 windowWidth = geometry().width();

        // calculate the remapped track list, based on sorted global weight, with the most influencing track on top
        RecorderGroup* recorderGroup = mPlugin->GetTimeViewToolBar()->GetRecorderGroup();
        const bool sorted = recorderGroup->GetSortNodeActivity();
        const bool useNodeColors = recorderGroup->GetUseNodeTypeColors();

        const uint32 graphContentsCode = mPlugin->mTrackHeaderWidget->mNodeContentsComboBox->currentIndex();
        recorder.ExtractNodeHistoryItems(*actorInstanceData, aznumeric_cast<float>(mPlugin->mCurTime), sorted, (EMotionFX::Recorder::EValueType)graphContentsCode, &mActiveItems, &mTrackRemap);

        const bool showNodeNames = mPlugin->mTrackHeaderWidget->mNodeNamesCheckBox->isChecked();
        const bool showMotionFiles = mPlugin->mTrackHeaderWidget->mMotionFilesCheckBox->isChecked();
        const bool interpolate = recorder.GetRecordSettings().mInterpolate;

        const uint32 nodeContentsCode = mPlugin->mTrackHeaderWidget->mNodeContentsComboBox->currentIndex();

        // for all history items
        QRectF itemRect;
        const uint32 numItems = historyItems.GetLength();
        for (uint32 i = 0; i < numItems; ++i)
        {
            EMotionFX::Recorder::NodeHistoryItem* curItem = historyItems[i];

            // draw the background rect
            double startTimePixel   = mPlugin->TimeToPixel(curItem->mStartTime);
            double endTimePixel     = mPlugin->TimeToPixel(curItem->mEndTime);

            const uint32 trackIndex = mTrackRemap[ curItem->mTrackIndex ];

            itemRect.setLeft(startTimePixel);
            itemRect.setRight(endTimePixel - 1);
            itemRect.setTop((mNodeRectsStartHeight + (trackIndex * (mNodeHistoryItemHeight + 3)) + 3) /* - mPlugin->mScrollY*/);
            itemRect.setBottom(itemRect.top() + mNodeHistoryItemHeight);

            if (!rect.intersects(itemRect.toRect()))
            {
                continue;
            }

            const AZ::Color colorCode = (useNodeColors) ? curItem->mTypeColor : curItem->mColor;
            QColor color;
            color.setRgbF(colorCode.GetR(), colorCode.GetG(), colorCode.GetB(), colorCode.GetA());

            bool matchesEvent = false;
            if (mIsScrolling == false && mPlugin->mIsAnimating == false)
            {
                if (mPlugin->mNodeHistoryItem == curItem)
                {
                    color = QColor(255, 128, 0);
                }

                if (mPlugin->mEventEmitterNode && mPlugin->mEventEmitterNode->GetId() == curItem->mNodeId && mPlugin->mEventHistoryItem)
                {
                    if (mPlugin->mEventHistoryItem->mStartTime >= curItem->mStartTime && mPlugin->mEventHistoryItem->mStartTime <= curItem->mEndTime)
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
                EMotionFX::KeyTrackLinearDynamic<float, float>* keyTrack = &curItem->mGlobalWeights; // init on global weights
                if (nodeContentsCode == 1)
                {
                    keyTrack = &curItem->mLocalWeights;
                }
                else
                if (nodeContentsCode == 2)
                {
                    keyTrack = &curItem->mPlayTimes;
                }

                float lastWeight = keyTrack->GetValueAtTime(0.0f, &curItem->mCachedKey, nullptr, interpolate);
                const float keyTimeStep = (curItem->mEndTime - curItem->mStartTime) / (float)widthInPixels;

                const int32 pixelStepSize = 1;//(widthInPixels / 300.0f) + 1;

                path.moveTo(QPointF(startTimePixel - 1, itemRect.bottom() + 1));
                path.lineTo(QPointF(startTimePixel + 1, itemRect.bottom() - 1 - lastWeight * mNodeHistoryItemHeight));
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

                    const float weight = keyTrack->GetValueAtTime(w * keyTimeStep, &curItem->mCachedKey, nullptr, interpolate);
                    const float height = aznumeric_cast<float>(itemRect.bottom() - weight * mNodeHistoryItemHeight);
                    path.lineTo(QPointF(startTimePixel + w + 1, height));
                }

                const float weight = keyTrack->GetValueAtTime(curItem->mEndTime, &curItem->mCachedKey, nullptr, interpolate);
                const float height = aznumeric_cast<float>(itemRect.bottom() - weight * mNodeHistoryItemHeight);
                path.lineTo(QPointF(startTimePixel + widthInPixels - 1, height));
                path.lineTo(QPointF(startTimePixel + widthInPixels, itemRect.bottom() + 1));
                painter.drawPath(path);
                painter.setRenderHint(QPainter::Antialiasing, false);
            }
            //---------

            // draw the text
            if (matchesEvent != true)
            {
                if (mIsScrolling == false && mPlugin->mIsAnimating == false)
                {
                    if (mPlugin->mNodeHistoryItem != curItem)
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

            mTempString.clear();
            if (showNodeNames)
            {
                mTempString += curItem->mName.c_str();
            }

            if (showMotionFiles && curItem->mMotionFileName.size() > 0)
            {
                if (!mTempString.empty())
                {
                    mTempString += " - ";
                }

                mTempString += curItem->mMotionFileName.c_str();
            }

            if (!mTempString.empty())
            {
                painter.drawText(aznumeric_cast<int>(itemRect.left() + 3), aznumeric_cast<int>(itemRect.bottom() - 2), mTempString.c_str());
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
        TimeTrack* mouseCursorTrack = mPlugin->GetTrackAt(localCursorPos.y());
        if (localCursorPos.x() < 0 || localCursorPos.x() > width())
        {
            mouseCursorTrack = nullptr;
        }

        // handle highlighting
        const uint32 numTracks = mPlugin->GetNumTracks();
        for (uint32 i = 0; i < numTracks; ++i)
        {
            TimeTrack* track = mPlugin->GetTrack(i);

            // set the highlighting flag for the track
            if (track == mouseCursorTrack)
            {
                // highlight the track
                track->SetIsHighlighted(true);

                // get the element over which the cursor is positioned
                TimeTrackElement* mouseCursorElement = mPlugin->GetElementAt(localCursorPos.x(), localCursorPos.y());

                // get the number of elements, iterate through them and disable the highlight flag
                const uint32 numElements = track->GetNumElements();
                for (uint32 e = 0; e < numElements; ++e)
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
                const uint32 numElements = track->GetNumElements();
                for (uint32 e = 0; e < numElements; ++e)
                {
                    TimeTrackElement* element = track->GetElement(e);
                    element->SetIsHighlighted(false);
                }
            }
        }

        EMotionFX::Motion* motion = mPlugin->GetMotion();
        if (motion)
        {
            // get the motion length
            animationLength = motion->GetDuration();

            // get the playback info and read out the clip start/end times
            EMotionFX::PlayBackInfo* playbackInfo = motion->GetDefaultPlayBackInfo();
            clipStart   = playbackInfo->mClipStartTime;
            clipEnd     = playbackInfo->mClipEndTime;

            // HACK: fix this later
            clipStart = 0.0;
            clipEnd = animationLength;
        }

        // calculate the pixel index of where the animation ends and where it gets clipped
        const double animEndPixel   = mPlugin->TimeToPixel(animationLength);
        const double clipStartPixel = mPlugin->TimeToPixel(clipStart);
        const double clipEndPixel   = mPlugin->TimeToPixel(clipEnd);

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
        painter.setBrush(mBrushBackgroundClipped);
        painter.drawRect(clipStartRect);
        painter.setBrush(mBrushBackground);
        painter.drawRect(motionRect);
        painter.setBrush(mBrushBackgroundClipped);
        painter.drawRect(clipEndRect);
        painter.setBrush(mBrushBackgroundOutOfRange);
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
        visibleStartTime = mPlugin->PixelToTime(0); //mPlugin->CalcTime( 0, &visibleStartTime, nullptr, nullptr, nullptr, nullptr );
        visibleEndTime = mPlugin->PixelToTime(width); //mPlugin->CalcTime( width, &visibleEndTime, nullptr, nullptr, nullptr, nullptr );

        // for all tracks
        const uint32 numTracks = mPlugin->mTracks.GetLength();
        for (uint32 i = 0; i < numTracks; ++i)
        {
            TimeTrack* track = mPlugin->mTracks[i];
            track->SetStartY(yOffset);

            // path for making the cut elements a bit transparent
            if (mCutMode)
            {
                // disable cut mode for all elements on default
                const uint32 numElements = track->GetNumElements();
                for (uint32 e = 0; e < numElements; ++e)
                {
                    track->GetElement(e)->SetIsCut(false);
                }

                // get the number of copy elements and check if ours is in
                const size_t numCopyElements = mCopyElements.size();
                for (size_t c = 0; c < numCopyElements; ++c)
                {
                    // get the copy element and make sure we're in the right track
                    const CopyElement& copyElement = mCopyElements[c];
                    if (copyElement.m_trackName != track->GetName())
                    {
                        continue;
                    }

                    // set the cut mode of the elements
                    for (uint32 e = 0; e < numElements; ++e)
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
        mPlugin->RenderElementTimeHandles(painter, height, mPlugin->mPenTimeHandles);
    }


    // show the time of the currently dragging element in the time info view
    void TrackDataWidget::ShowElementTimeInfo(TimeTrackElement* element)
    {
        if (mPlugin->GetTimeInfoWidget() == nullptr)
        {
            return;
        }

        // enable overwrite mode so that the time info widget will show the custom time rather than the current time of the plugin
        mPlugin->GetTimeInfoWidget()->SetIsOverwriteMode(true);

        // calculate the dimensions
        int32 startX, startY, width, height;
        element->CalcDimensions(&startX, &startY, &width, &height);

        // show the times of the element
        mPlugin->GetTimeInfoWidget()->SetOverwriteTime(mPlugin->PixelToTime(startX), mPlugin->PixelToTime(startX + width));
    }


    void TrackDataWidget::mouseDoubleClickEvent(QMouseEvent* event)
    {
        if (event->button() != Qt::LeftButton)
        {
            return;
        }

        // if we clicked inside the node history area
        RecorderGroup* recorderGroup = mPlugin->GetTimeViewToolBar()->GetRecorderGroup();
        if (GetIsInsideNodeHistory(event->y()) && recorderGroup->GetDisplayNodeActivity())
        {
            EMotionFX::Recorder::ActorInstanceData* actorInstanceData = FindActorInstanceData();
            EMotionFX::Recorder::NodeHistoryItem* historyItem = FindNodeHistoryItem(actorInstanceData, event->x(), event->y());
            if (historyItem)
            {
                emit mPlugin->DoubleClickedRecorderNodeHistoryItem(actorInstanceData, historyItem);
            }
        }
    }

    void TrackDataWidget::SetPausedTime(float timeValue, bool emitTimeChangeStart)
    {
        mPlugin->mCurTime = timeValue;
        const AZStd::vector<EMotionFX::MotionInstance*>& motionInstances = MotionWindowPlugin::GetSelectedMotionInstances();
        if (motionInstances.size() == 1)
        {
            EMotionFX::MotionInstance* motionInstance = motionInstances[0];
            motionInstance->SetCurrentTime(timeValue);
            motionInstance->SetPause(true);
        }
        if (emitTimeChangeStart)
        {
            emit mPlugin->ManualTimeChangeStart(timeValue);
        }
        emit mPlugin->ManualTimeChange(timeValue);
    }

    // when the mouse is moving, while a button is pressed
    void TrackDataWidget::mouseMoveEvent(QMouseEvent* event)
    {
        mPlugin->SetRedrawFlag();

        QPoint mousePos = event->pos();

        const int32 deltaRelX = event->x() - mLastMouseX;
        mLastMouseX = event->x();
        mPlugin->mCurMouseX = event->x();
        mPlugin->mCurMouseY = event->y();

        const int32 deltaRelY = event->y() - mLastMouseY;
        mLastMouseY = event->y();

        const bool altPressed = event->modifiers() & Qt::AltModifier;
        const bool isZooming = mMouseLeftClicked == false && mMouseRightClicked && altPressed;
        const bool isPanning = mMouseLeftClicked == false && isZooming == false && (mMouseMidClicked || mMouseRightClicked);

        if (deltaRelY != 0)
        {
            mAllowContextMenu = false;
        }

        // get the track over which the cursor is positioned
        TimeTrack* mouseCursorTrack = mPlugin->GetTrackAt(event->y());

        if (mMouseRightClicked)
        {
            mIsScrolling = true;
        }

        // if the mouse left button is pressed
        if (mMouseLeftClicked)
        {
            if (altPressed)
            {
                mRectZooming = true;
            }
            else
            {
                mRectZooming = false;
            }

            // rect selection: update mouse position
            if (mRectSelecting)
            {
                mSelectEnd = mousePos;
            }

            if (mDraggingElement == nullptr && mResizeElement == nullptr && mRectSelecting == false)
            {
                // update the current time marker
                int newX = event->x();
                newX = MCore::Clamp<int>(newX, 0, geometry().width() - 1);
                mPlugin->mCurTime = mPlugin->PixelToTime(newX);

                EMotionFX::Recorder& recorder = EMotionFX::GetRecorder();
                if (recorder.GetRecordTime() > MCore::Math::epsilon)
                {
                    if (recorder.GetIsInPlayMode())
                    {
                        recorder.SetCurrentPlayTime(aznumeric_cast<float>(mPlugin->GetCurrentTime()));
                        recorder.SetAutoPlay(false);
                        emit mPlugin->ManualTimeChange(aznumeric_cast<float>(mPlugin->GetCurrentTime()));
                    }
                }
                else
                {
                    SetPausedTime(aznumeric_cast<float>(mPlugin->mCurTime));
                }

                mIsScrolling = true;
            }

            TimeTrack* dragElementTrack = nullptr;
            if (mDraggingElement)
            {
                dragElementTrack = mDraggingElement->GetTrack();
            }

            // calculate the delta movement
            const int32 deltaX = event->x() - mLastLeftClickedX;
            const int32 movement = abs(deltaX);
            const bool elementTrackChanged = (mouseCursorTrack && dragElementTrack && mouseCursorTrack != dragElementTrack);
            if ((movement > 1 && !mDragging) || elementTrackChanged)
            {
                mDragging = true;
            }

            // handle resizing
            if (mResizing)
            {
                if (mPlugin->FindTrackByElement(mResizeElement) == nullptr)
                {
                    mResizeElement = nullptr;
                }

                if (mResizeElement)
                {
                    TimeTrack* resizeElementTrack = mResizeElement->GetTrack();

                    // only allow resizing on enabled time tracks
                    if (resizeElementTrack->GetIsEnabled())
                    {
                        mResizeElement->SetShowTimeHandles(true);
                        mResizeElement->SetShowToolTip(false);

                        double resizeTime = (deltaRelX / mPlugin->mTimeScale) / mPlugin->mPixelsPerSecond;
                        mResizeID = mResizeElement->HandleResize(mResizeID, resizeTime, 0.02 / mPlugin->mTimeScale);

                        // show the time of the currently resizing element in the time info view
                        ShowElementTimeInfo(mResizeElement);

                        // Move the current time marker along with the event resizing position.
                        float timeValue = 0.0f;
                        switch (mResizeID)
                        {
                            case TimeTrackElement::RESIZEPOINT_START:
                            {
                                timeValue = aznumeric_cast<float>(mResizeElement->GetStartTime());
                                break;
                            }
                            case TimeTrackElement::RESIZEPOINT_END:
                            {
                                timeValue = aznumeric_cast<float>(mResizeElement->GetEndTime());
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
            if (mDragging == false || mDraggingElement == nullptr)
            {
                return;
            }

            // check if the mouse cursor is over another time track than the dragging element
            if (elementTrackChanged)
            {
                // if yes we need to remove the dragging element from the old time track
                dragElementTrack->RemoveElement(mDraggingElement, false);

                // and add it to the new time track where the cursor now is over
                mouseCursorTrack->AddElement(mDraggingElement);
                mDraggingElement->SetTrack(mouseCursorTrack);
            }

            // show the time of the currently dragging element in the time info view
            ShowElementTimeInfo(mDraggingElement);

            // adjust the cursor
            setCursor(Qt::ClosedHandCursor);
            mDraggingElement->SetShowToolTip(false);

            // show the time handles
            mDraggingElement->SetShowTimeHandles(true);

            const double snapThreshold = 0.02 / mPlugin->mTimeScale;

            // calculate how many pixels we moved with the mouse
            const int32 deltaMovement = event->x() - mLastMouseMoveX;
            mLastMouseMoveX = event->x();

            // snap the moved amount to a given time value
            double snappedTime = mDraggingElement->GetStartTime() + ((deltaMovement / mPlugin->mPixelsPerSecond) / mPlugin->mTimeScale);

            bool startSnapped = false;
            if (abs(deltaMovement) < 2 && abs(deltaMovement) > 0) // only snap when moving the mouse very slowly
            {
                startSnapped = mPlugin->SnapTime(&snappedTime, mDraggingElement, snapThreshold);
            }

            // in case the start time didn't snap to anything
            if (startSnapped == false)
            {
                // try to snap the end time
                double snappedEndTime = mDraggingElement->GetEndTime() + ((deltaMovement / mPlugin->mPixelsPerSecond) / mPlugin->mTimeScale);
                /*bool endSnapped = */ mPlugin->SnapTime(&snappedEndTime, mDraggingElement, snapThreshold);

                // apply the delta movement
                const double deltaTime = snappedEndTime - mDraggingElement->GetEndTime();
                mDraggingElement->MoveRelative(deltaTime);
            }
            else
            {
                // apply the snapped delta movement
                const double deltaTime = snappedTime - mDraggingElement->GetStartTime();
                mDraggingElement->MoveRelative(deltaTime);
            }

            dragElementTrack = mDraggingElement->GetTrack();
            const float timeValue = aznumeric_cast<float>(mDraggingElement->GetStartTime());
            SetPausedTime(timeValue);
        }
        else if (isPanning)
        {
            if (EMotionFX::GetRecorder().GetIsRecording() == false)
            {
                mPlugin->DeltaScrollX(-deltaRelX, false);
            }
        }
        else if (isZooming)
        {
            if (deltaRelY < 0)
            {
                setCursor(*(mPlugin->GetZoomOutCursor()));
            }
            else
            {
                setCursor(*(mPlugin->GetZoomInCursor()));
            }

            DoMouseYMoveZoom(deltaRelY, mPlugin);
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
        mPlugin->DisableAllToolTips();

        // get the time track and return directly if we are not over a valid track with the cursor
        TimeTrack* timeTrack = mPlugin->GetTrackAt(y);
        if (timeTrack == nullptr)
        {
            setCursor(Qt::ArrowCursor);
            return;
        }

        // get the element over which the cursor is positioned
        TimeTrackElement* element = mPlugin->GetElementAt(x, y);

        // in case the cursor is over an element, show tool tips
        if (element)
        {
            element->SetShowToolTip(true);
        }
        else
        {
            mPlugin->DisableAllToolTips();
        }

        // do not allow any editing in case the track is not enabled
        if (timeTrack->GetIsEnabled() == false)
        {
            setCursor(Qt::ArrowCursor);
            return;
        }

        // check if we are hovering over a resize point
        if (mPlugin->FindResizePoint(x, y, &mResizeElement, &mResizeID))
        {
            setCursor(Qt::SizeHorCursor);
            mResizeElement->SetShowToolTip(true);
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
        mPlugin->SetRedrawFlag();

        QPoint mousePos = event->pos();

        const bool ctrlPressed = event->modifiers() & Qt::ControlModifier;
        const bool shiftPressed = event->modifiers() & Qt::ShiftModifier;
        const bool altPressed = event->modifiers() & Qt::AltModifier;

        // store the last clicked position
        mLastMouseMoveX     = event->x();
        mAllowContextMenu   = true;
        mRectSelecting      = false;

        if (event->button() == Qt::RightButton)
        {
            mMouseRightClicked = true;
        }

        if (event->button() == Qt::MidButton)
        {
            mMouseMidClicked = true;
        }

        if (event->button() == Qt::LeftButton)
        {
            mMouseLeftClicked   = true;

            EMotionFX::Recorder& recorder = EMotionFX::GetRecorder();
            if ((mPlugin->mNodeHistoryItem == nullptr) && altPressed == false && (recorder.GetRecordTime() >= MCore::Math::epsilon))
            {
                // update the current time marker
                int newX = event->x();
                newX = MCore::Clamp<int>(newX, 0, geometry().width() - 1);
                mPlugin->mCurTime = mPlugin->PixelToTime(newX);

                if (recorder.GetRecordTime() < MCore::Math::epsilon)
                {
                    SetPausedTime(aznumeric_cast<float>(mPlugin->GetCurrentTime()), /*emitTimeChangeStart=*/true);
                }
                else
                {
                    if (recorder.GetIsInPlayMode() == false)
                    {
                        recorder.StartPlayBack();
                    }

                    recorder.SetCurrentPlayTime(aznumeric_cast<float>(mPlugin->GetCurrentTime()));
                    recorder.SetAutoPlay(false);
                    emit mPlugin->ManualTimeChangeStart(aznumeric_cast<float>(mPlugin->GetCurrentTime()));
                    emit mPlugin->ManualTimeChange(aznumeric_cast<float>(mPlugin->GetCurrentTime()));
                }
            }
            else // not inside timeline
            {
                // if we clicked inside the node history area
                RecorderGroup* recorderGroup = mPlugin->GetTimeViewToolBar()->GetRecorderGroup();
                if (GetIsInsideNodeHistory(event->y()) && recorderGroup->GetDisplayNodeActivity())
                {
                    EMotionFX::Recorder::ActorInstanceData* actorInstanceData = FindActorInstanceData();
                    EMotionFX::Recorder::NodeHistoryItem* historyItem = FindNodeHistoryItem(actorInstanceData, event->x(), event->y());
                    if (historyItem && altPressed == false)
                    {
                        emit mPlugin->ClickedRecorderNodeHistoryItem(actorInstanceData, historyItem);
                    }
                }
                {
                    // unselect all elements
                    if (ctrlPressed == false && shiftPressed == false)
                    {
                        mPlugin->UnselectAllElements();
                    }

                    // find the element we're clicking in
                    TimeTrackElement* element = mPlugin->GetElementAt(event->x(), event->y());
                    if (element)
                    {
                        // show the time of the currently dragging element in the time info view
                        ShowElementTimeInfo(element);

                        TimeTrack* timeTrack = element->GetTrack();

                        if (timeTrack->GetIsEnabled())
                        {
                            mDraggingElement    = element;
                            mDragElementTrack   = timeTrack;
                            mDraggingElement->SetShowTimeHandles(true);
                            setCursor(Qt::ClosedHandCursor);
                        }
                        else
                        {
                            mDraggingElement    = nullptr;
                            mDragElementTrack   = nullptr;
                        }

                        // shift select
                        if (shiftPressed)
                        {
                            // get the element number of the clicked element
                            const uint32 clickedElementNr = element->GetElementNumber();

                            // get the element number of the first previously selected element
                            TimeTrackElement* firstSelectedElement = timeTrack->GetFirstSelectedElement();
                            const uint32 firstSelectedNr = firstSelectedElement ? firstSelectedElement->GetElementNumber() : 0;

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
                        mDraggingElement    = nullptr;
                        mDragElementTrack   = nullptr;

                        // rect selection
                        mRectSelecting  = true;
                        mSelectStart    = mousePos;
                        mSelectEnd      = mSelectStart;
                        setCursor(Qt::ArrowCursor);
                    }

                    // if we're going to resize
                    if (mResizeElement && mResizeID != MCORE_INVALIDINDEX32)
                    {
                        mResizing = true;
                    }
                    else
                    {
                        mResizing = false;
                    }

                    // store the last clicked position
                    mMouseLeftClicked   = true;
                    mLastLeftClickedX   = event->x();
                }
            }
        }

        const bool isZooming = mMouseLeftClicked == false && mMouseRightClicked && altPressed;
        const bool isPanning = mMouseLeftClicked == false && isZooming == false && (mMouseMidClicked || mMouseRightClicked);

        if (isPanning)
        {
            setCursor(Qt::SizeHorCursor);
        }

        if (isZooming)
        {
            setCursor(*(mPlugin->GetZoomInCursor()));
        }
    }


    // when releasing the mouse button
    void TrackDataWidget::mouseReleaseEvent(QMouseEvent* event)
    {
        mPlugin->SetRedrawFlag();

        setCursor(Qt::ArrowCursor);

        // disable overwrite mode in any case when the mouse gets released so that we display the current time from the plugin again
        if (mPlugin->GetTimeInfoWidget())
        {
            mPlugin->GetTimeInfoWidget()->SetIsOverwriteMode(false);
        }

        mLastMouseMoveX = event->x();

        const bool ctrlPressed = event->modifiers() & Qt::ControlModifier;
        //const bool shiftPressed = event->modifiers() & Qt::ShiftModifier;

        if (event->button() == Qt::RightButton)
        {
            mMouseRightClicked = false;
            mIsScrolling = false;
        }

        if (event->button() == Qt::MidButton)
        {
            mMouseMidClicked = false;
        }

        if (event->button() == Qt::LeftButton)
        {
            TimeTrack* mouseCursorTrack = mPlugin->GetTrackAt(event->y());
            const bool elementTrackChanged = (mouseCursorTrack && mDragElementTrack && mouseCursorTrack != mDragElementTrack);

            if (mDragging && mMouseLeftClicked && mDraggingElement && !mIsScrolling && !mResizing)
            {
                SetPausedTime(aznumeric_cast<float>(mDraggingElement->GetStartTime()));
            }

            if ((mResizing || mDragging) && elementTrackChanged == false && mDraggingElement)
            {
                emit MotionEventChanged(mDraggingElement, mDraggingElement->GetStartTime(), mDraggingElement->GetEndTime());
            }

            mMouseLeftClicked = false;
            mDragging = false;
            mResizing = false;
            mIsScrolling = false;

            // rect selection
            if (mRectSelecting)
            {
                if (mRectZooming)
                {
                    mRectZooming = false;

                    // calc the selection rect
                    QRect selectRect;
                    CalcSelectRect(selectRect);

                    // zoom in on the rect
                    if (selectRect.isEmpty() == false)
                    {
                        mPlugin->ZoomRect(selectRect);
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
                        //selectRect = mActiveGraph->GetTransform().inverted().mapRect( selectRect );

                        // rect select the elements
                        const bool overwriteSelection = (ctrlPressed == false);
                        SelectElementsInRect(selectRect, overwriteSelection, true, ctrlPressed);
                    }
                }
            }

            // check if we moved an element to another track
            if (elementTrackChanged && mDraggingElement)
            {
                // lastly fire a signal so that the data can change along with
                emit ElementTrackChanged(mDraggingElement->GetElementNumber(), aznumeric_cast<float>(mDraggingElement->GetStartTime()), aznumeric_cast<float>(mDraggingElement->GetEndTime()), mDragElementTrack->GetName(), mouseCursorTrack->GetName());
            }
            mDragElementTrack = nullptr;

            if (mDraggingElement)
            {
                mDraggingElement->SetShowTimeHandles(false);
                mDraggingElement = nullptr;
            }

            // disable rect selection mode again
            mRectSelecting  = false;
            return;
        }

        // disable rect selection mode again
        mRectSelecting  = false;

        UpdateMouseOverCursor(event->x(), event->y());
    }

    void TrackDataWidget::ClearState()
    {
        mDragElementTrack = nullptr;
        mDraggingElement = nullptr;
        mDragging = false;
        mResizing = false;
        mResizeElement = nullptr;
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
        DoWheelEvent(event, mPlugin);
    }


    // drag & drop support
    void TrackDataWidget::dragEnterEvent(QDragEnterEvent* event)
    {
        mPlugin->SetRedrawFlag();
        mOldCurrentTime = mPlugin->GetCurrentTime();

        // this is needed to actually reach the drop event function
        event->acceptProposedAction();
    }


    void TrackDataWidget::dragMoveEvent(QDragMoveEvent* event)
    {
        mPlugin->SetRedrawFlag();
        QPoint mousePos = event->pos();

        double dropTime = mPlugin->PixelToTime(mousePos.x());
        mPlugin->SetCurrentTime(dropTime);

        SetPausedTime(aznumeric_cast<float>(dropTime));
    }


    void TrackDataWidget::dropEvent(QDropEvent* event)
    {
        mPlugin->SetRedrawFlag();
        // accept the drop
        event->acceptProposedAction();

        // emit drop event
        emit MotionEventPresetsDropped(event->pos());

        mPlugin->SetCurrentTime(mOldCurrentTime);
    }


    // the context menu event
    void TrackDataWidget::contextMenuEvent(QContextMenuEvent* event)
    {
        if (mIsScrolling || mDragging  || mResizing || !mAllowContextMenu)
        {
            return;
        }

        mPlugin->SetRedrawFlag();

        if (EMotionFX::GetRecorder().GetRecordTime() > MCore::Math::epsilon)
        {
            DoRecorderContextMenuEvent(event);
            return;
        }

        if (mPlugin->mMotion == nullptr)
        {
            return;
        }

        QPoint point = event->pos();
        mContextMenuX = point.x();
        mContextMenuY = point.y();

        TimeTrack* timeTrack = mPlugin->GetTrackAt(mContextMenuY);

        uint32 numElements          = 0;
        uint32 numSelectedElements  = 0;

        // calculate the number of selected and total events
        const uint32 numTracks = mPlugin->GetNumTracks();
        for (uint32 i = 0; i < numTracks; ++i)
        {
            // get the current time view track
            TimeTrack* track = mPlugin->GetTrack(i);
            if (track->GetIsVisible() == false)
            {
                continue;
            }

            // get the number of elements in the track and iterate through them
            const uint32 numTrackElements = track->GetNumElements();
            for (uint32 j = 0; j < numTrackElements; ++j)
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
            for (uint32 i = 0; i < numElements; ++i)
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
            TimeTrackElement* element = mPlugin->GetElementAt(mContextMenuX, mContextMenuY);
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
        if (mPlugin)
        {
            mPlugin->OnKeyPressEvent(event);
        }
    }


    // propagate key events to the plugin and let it handle by a shared function
    void TrackDataWidget::keyReleaseEvent(QKeyEvent* event)
    {
        if (mPlugin)
        {
            mPlugin->OnKeyReleaseEvent(event);
        }
    }


    void TrackDataWidget::AddMotionEvent(int32 x, int32 y)
    {
        mPlugin->AddMotionEvent(x, y);
    }


    void TrackDataWidget::RemoveMotionEvent(int32 x, int32 y)
    {
        mPlugin->SetRedrawFlag();
        // get the time track on which we dropped the preset
        TimeTrack* timeTrack = mPlugin->GetTrackAt(y);
        if (timeTrack == nullptr)
        {
            return;
        }

        // get the time track on which we dropped the preset
        TimeTrackElement* element = mPlugin->GetElementAt(x, y);
        if (element == nullptr)
        {
            return;
        }

        CommandSystem::CommandHelperRemoveMotionEvent(timeTrack->GetName(), element->GetElementNumber());
    }


    // remove selected motion events in track
    void TrackDataWidget::RemoveSelectedMotionEventsInTrack()
    {
        mPlugin->SetRedrawFlag();
        // get the track where we are at the moment
        TimeTrack* timeTrack = mPlugin->GetTrackAt(mLastMouseY);
        if (timeTrack == nullptr)
        {
            return;
        }

        MCore::Array<uint32> eventNumbers;

        // calculate the number of selected events
        const uint32 numEvents = timeTrack->GetNumElements();
        for (uint32 i = 0; i < numEvents; ++i)
        {
            TimeTrackElement* element = timeTrack->GetElement(i);

            // increase the counter in case the element is selected
            if (element->GetIsSelected())
            {
                eventNumbers.Add(i);
            }
        }

        // remove the motion events
        CommandSystem::CommandHelperRemoveMotionEvents(timeTrack->GetName(), eventNumbers);

        mPlugin->UnselectAllElements();
        ClearState();
    }


    // remove all motion events in track
    void TrackDataWidget::RemoveAllMotionEventsInTrack()
    {
        mPlugin->SetRedrawFlag();

        TimeTrack* timeTrack = mPlugin->GetTrackAt(mLastMouseY);
        if (timeTrack == nullptr)
        {
            return;
        }

        MCore::Array<uint32> eventNumbers;

        // construct an array with the event numbers
        const uint32 numEvents = timeTrack->GetNumElements();
        for (uint32 i = 0; i < numEvents; ++i)
        {
            eventNumbers.Add(i);
        }

        // remove the motion events
        CommandSystem::CommandHelperRemoveMotionEvents(timeTrack->GetName(), eventNumbers);

        mPlugin->UnselectAllElements();
        ClearState();
    }

    void TrackDataWidget::OnRemoveEventTrack()
    {
        const TimeTrack* timeTrack = mPlugin->GetTrackAt(mLastMouseY);
        if (!timeTrack)
        {
            return;
        }

        const AZ::Outcome<AZ::u32> trackIndexOutcome = mPlugin->FindTrackIndex(timeTrack);
        if (trackIndexOutcome.IsSuccess())
        {
            RemoveTrack(trackIndexOutcome.GetValue());
        }
    }

    void TrackDataWidget::FillCopyElements(bool selectedItemsOnly)
    {
        // clear the array before feeding it
        mCopyElements.clear();

        // get the time track name
        const TimeTrack* timeTrack = mPlugin->GetTrackAt(mContextMenuY);
        if (timeTrack == nullptr)
        {
            return;
        }
        const AZStd::string trackName = timeTrack->GetName();

        // check if the motion is valid and return failure in case it is not
        const EMotionFX::Motion* motion = mPlugin->GetMotion();
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
        const uint32 numElements = timeTrack->GetNumElements();
        MCORE_ASSERT(numElements == eventTrack->GetNumEvents());
        for (uint32 i = 0; i < numElements; ++i)
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
            mCopyElements.emplace_back(
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
        mPlugin->SetRedrawFlag();

        FillCopyElements(false);

        mCutMode = true;
    }


    // copy all events from a track
    void TrackDataWidget::OnCopyTrack()
    {
        mPlugin->SetRedrawFlag();

        FillCopyElements(false);

        mCutMode = false;
    }


    // cut motion event
    void TrackDataWidget::OnCutElement()
    {
        mPlugin->SetRedrawFlag();

        FillCopyElements(true);

        mCutMode = true;
    }


    // copy motion event
    void TrackDataWidget::OnCopyElement()
    {
        mPlugin->SetRedrawFlag();

        FillCopyElements(true);

        mCutMode = false;
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
        mPlugin->SetRedrawFlag();

        // get the time track name where we are pasting
        TimeTrack* timeTrack = mPlugin->GetTrackAt(mContextMenuY);
        if (timeTrack == nullptr)
        {
            return;
        }
        AZStd::string trackName = timeTrack->GetName();

        // get the number of elements to copy
        const size_t numElements = mCopyElements.size();

        // create the command group
        MCore::CommandGroup commandGroup("Paste motion events");

        // find the min and maximum time values of the events to paste
        auto [minEvent, maxEvent] = AZStd::minmax_element(begin(mCopyElements), end(mCopyElements), [](const CopyElement& left, const CopyElement& right)
        {
            return left.m_startTime < right.m_startTime;
        });

        if (mCutMode)
        {
            // iterate through the copy elements from back to front and delete the selected ones
            for (int32 i = static_cast<int32>(numElements) - 1; i >= 0; i--)
            {
                const CopyElement& copyElement = mCopyElements[i];

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
                size_t eventNr = MCORE_INVALIDINDEX32;
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
                if (eventNr != MCORE_INVALIDINDEX32)
                {
                    CommandSystem::CommandHelperRemoveMotionEvent(copyElement.m_motionID, copyElement.m_trackName.c_str(), static_cast<uint32>(eventNr), &commandGroup);
                }
            }
        }

        const float offset = useLocation ? aznumeric_cast<float>(mPlugin->PixelToTime(mContextMenuX, true)) - minEvent->m_startTime : 0.0f;

        // iterate through the elements to copy and add the new motion events
        for (uint32 i = 0; i < numElements; ++i)
        {
            const CopyElement& copyElement = mCopyElements[i];

            float startTime = copyElement.m_startTime + offset;
            float endTime   = copyElement.m_endTime + offset;

            // calculate the duration of the motion event
            float duration  = 0.0f;
            if (MCore::Compare<float>::CheckIfIsClose(startTime, endTime, MCore::Math::epsilon) == false)
            {
                duration = endTime - startTime;
            }

            CommandSystem::CommandHelperAddMotionEvent(trackName.c_str(), startTime, endTime, copyElement.m_eventDatas, &commandGroup);
        }

        // execute the group command
        AZStd::string outResult;
        if (GetCommandManager()->ExecuteCommandGroup(commandGroup, outResult) == false)
        {
            MCore::LogError(outResult.c_str());
        }

        if (mCutMode)
        {
            mCopyElements.clear();
        }
    }


    void TrackDataWidget::OnCreatePresetEvent()
    {
        mPlugin->SetRedrawFlag();
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(MotionEventsPlugin::CLASS_ID);
        if (plugin == nullptr)
        {
            return;
        }

        MotionEventsPlugin* eventsPlugin = static_cast<MotionEventsPlugin*>(plugin);

        QPoint mousePos(mContextMenuX, mContextMenuY);
        eventsPlugin->OnEventPresetDropped(mousePos);
    }

    void TrackDataWidget::OnAddTrack()
    {
        mPlugin->SetRedrawFlag();
        CommandSystem::CommandAddEventTrack();
    }

    // select all elements within a given rect
    void TrackDataWidget::SelectElementsInRect(const QRect& rect, bool overwriteCurSelection, bool select, bool toggleMode)
    {
        // get the number of tracks and iterate through them
        const uint32 numTracks = mPlugin->GetNumTracks();
        for (uint32 i = 0; i < numTracks; ++i)
        {
            // get the current time track
            TimeTrack* track = mPlugin->GetTrack(i);
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
                TimeTrackElement*   element = mPlugin->GetElementAt(localPos.x(), localPos.y());
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
        mNodeHistoryRect = QRect();
        if (actorInstanceData && actorInstanceData->mNodeHistoryItems.GetLength() > 0)
        {
            const uint32 height = (recorder.CalcMaxNodeHistoryTrackIndex(*actorInstanceData) + 1) * (mNodeHistoryItemHeight + 3) + mNodeRectsStartHeight;
            mNodeHistoryRect.setTop(mNodeRectsStartHeight);
            mNodeHistoryRect.setBottom(height);
            mNodeHistoryRect.setLeft(0);
            mNodeHistoryRect.setRight(geometry().width());
        }

        mEventHistoryTotalHeight = 0;
        if (actorInstanceData && actorInstanceData->mEventHistoryItems.GetLength() > 0)
        {
            mEventHistoryTotalHeight = (recorder.CalcMaxEventHistoryTrackIndex(*actorInstanceData) + 1) * 20;
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

        // make sure the mTrackRemap array is up to date
        RecorderGroup* recorderGroup = mPlugin->GetTimeViewToolBar()->GetRecorderGroup();
        const bool sorted = recorderGroup->GetSortNodeActivity();
        const uint32 graphContentsCode = mPlugin->mTrackHeaderWidget->mNodeContentsComboBox->currentIndex();
        EMotionFX::GetRecorder().ExtractNodeHistoryItems(*actorInstanceData, aznumeric_cast<float>(mPlugin->mCurTime), sorted, (EMotionFX::Recorder::EValueType)graphContentsCode, &mActiveItems, &mTrackRemap);


        // get the history items shortcut
        const MCore::Array<EMotionFX::Recorder::NodeHistoryItem*>&  historyItems = actorInstanceData->mNodeHistoryItems;

        QRect rect;
        const uint32 numItems = historyItems.GetLength();
        for (uint32 i = 0; i < numItems; ++i)
        {
            EMotionFX::Recorder::NodeHistoryItem* curItem = historyItems[i];

            // draw the background rect
            double startTimePixel   = mPlugin->TimeToPixel(curItem->mStartTime);
            double endTimePixel     = mPlugin->TimeToPixel(curItem->mEndTime);

            if (startTimePixel > x || endTimePixel < x)
            {
                continue;
            }

            rect.setLeft(aznumeric_cast<int>(startTimePixel));
            rect.setRight(aznumeric_cast<int>(endTimePixel));
            rect.setTop((mNodeRectsStartHeight + (mTrackRemap[curItem->mTrackIndex] * (mNodeHistoryItemHeight + 3)) + 3));
            rect.setBottom(rect.top() + mNodeHistoryItemHeight);

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
        const uint32 actorInstanceDataIndex = recorder.FindActorInstanceDataIndex(actorInstance);
        if (actorInstanceDataIndex == MCORE_INVALIDINDEX32) // it doesn't exist, so we didn't record anything for this actor instance
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
        mContextMenuX = point.x();
        mContextMenuY = point.y();

        // create the context menu
        QMenu menu(this);

        //---------------------
        // Timeline actions
        //---------------------
        QAction* action = menu.addAction("Zoom To Fit All");
        connect(action, &QAction::triggered, mPlugin, &TimeViewPlugin::OnZoomAll);

        action = menu.addAction("Reset Timeline");
        connect(action, &QAction::triggered, mPlugin, &TimeViewPlugin::OnResetTimeline);

        //---------------------
        // Right-clicked on a motion item
        //---------------------
        EMotionFX::Recorder::NodeHistoryItem* historyItem = FindNodeHistoryItem(FindActorInstanceData(), point.x(), point.y());
        if (historyItem)
        {
            menu.addSeparator();

            action = menu.addAction("Show Node In Graph");
            connect(action, &QAction::triggered, mPlugin, &TimeViewPlugin::OnShowNodeHistoryNodeInGraph);
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
        outString += AZStd::string::format("<td width=\"400\"><p style=\"color:rgb(115, 115, 115)\">%s</p></td></tr>", item->mName.c_str());

        // build the node path string
        EMotionFX::ActorInstance* actorInstance = FindActorInstanceData()->mActorInstance;
        EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();
        if (animGraphInstance)
        {
            EMotionFX::AnimGraph* animGraph = animGraphInstance->GetAnimGraph();
            EMotionFX::AnimGraphNode* node = animGraph->RecursiveFindNodeById(item->mNodeId);
            if (node)
            {
                MCore::Array<EMotionFX::AnimGraphNode*> nodePath;
                EMotionFX::AnimGraphNode* curNode = node->GetParentNode();
                while (curNode)
                {
                    nodePath.Insert(0, curNode);
                    curNode = curNode->GetParentNode();
                }

                AZStd::string nodePathString;
                nodePathString.reserve(256);
                for (uint32 i = 0; i < nodePath.GetLength(); ++i)
                {
                    nodePathString += nodePath[i]->GetName();
                    if (i != nodePath.GetLength() - 1)
                    {
                        nodePathString += " > ";
                    }
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
                    outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%d</p></td></tr>", node->GetNumChildNodes());

                    outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Recursive Children:&nbsp;</b></p></td>");
                    outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%d</p></td></tr>", node->RecursiveCalcNumNodes());
                }
            }
        }

        // motion name
        if (item->mMotionID != MCORE_INVALIDINDEX32 && item->mMotionFileName.size() > 0)
        {
            outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Motion FileName:&nbsp;</b></p></td>");
            outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%s</p></td></tr>", item->mMotionFileName.c_str());

            // show motion info
            EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByID(item->mMotionID);
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

        const MCore::Array<EMotionFX::Recorder::EventHistoryItem*>& historyItems = actorInstanceData->mEventHistoryItems;
        const float tickHalfWidth = 7;
        const float tickHeight = 16;

        const uint32 numItems = historyItems.GetLength();
        for (uint32 i = 0; i < numItems; ++i)
        {
            EMotionFX::Recorder::EventHistoryItem* curItem = historyItems[i];

            float height = aznumeric_cast<float>((curItem->mTrackIndex * 20) + mEventsStartHeight);
            double startTimePixel   = mPlugin->TimeToPixel(curItem->mStartTime);
            //double endTimePixel       = mPlugin->TimeToPixel( curItem->mEndTime );

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

        const EMotionFX::MotionEvent* motionEvent = item->mEventInfo.mEvent;
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
        outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%.3f seconds</p></td></tr>", item->mEventInfo.mTimeValue);

        outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Event Trigger Time:&nbsp;</b></p></td>");
        outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%.3f seconds</p></td></tr>", item->mStartTime);

        outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Is Ranged Event:&nbsp;</b></p></td>");
        outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%s</p></td></tr>", (item->mIsTickEvent == false) ? "Yes" : "No");

        if (item->mIsTickEvent == false)
        {
            const static AZStd::string eventStartText = "Event Start";
            const static AZStd::string eventActiveText = "Event Active";
            const static AZStd::string eventEndText = "Event End";
            const AZStd::string* outputEventStateText = &eventStartText;
            if (item->mEventInfo.m_eventState == EMotionFX::EventInfo::EventState::ACTIVE)
            {
                outputEventStateText = &eventActiveText;
            }
            else if (item->mEventInfo.m_eventState == EMotionFX::EventInfo::EventState::END)
            {
                outputEventStateText = &eventEndText;
            }
            outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Ranged Info:&nbsp;</b></p></td>");
            outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%s</p></td></tr>", outputEventStateText->c_str());
        }

        outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Global Weight:&nbsp;</b></p></td>");
        outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%.3f</p></td></tr>", item->mEventInfo.mGlobalWeight);

        outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Local Weight:&nbsp;</b></p></td>");
        outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%.3f</p></td></tr>", item->mEventInfo.mLocalWeight);

        // build the node path string
        EMotionFX::ActorInstance* actorInstance = FindActorInstanceData()->mActorInstance;
        EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();
        if (animGraphInstance)
        {
            EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(item->mAnimGraphID);//animGraphInstance->GetAnimGraph();
            EMotionFX::AnimGraphNode* node = animGraph->RecursiveFindNodeById(item->mEmitterNodeId);
            if (node)
            {
                outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Emitted By:&nbsp;</b></p></td>");
                outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%s</p></td></tr>", node->GetName());

                MCore::Array<EMotionFX::AnimGraphNode*> nodePath;
                EMotionFX::AnimGraphNode* curNode = node->GetParentNode();
                while (curNode)
                {
                    nodePath.Insert(0, curNode);
                    curNode = curNode->GetParentNode();
                }

                AZStd::string nodePathString;
                nodePathString.reserve(256);
                for (uint32 i = 0; i < nodePath.GetLength(); ++i)
                {
                    nodePathString += nodePath[i]->GetName();
                    if (i != nodePath.GetLength() - 1)
                    {
                        nodePathString += " > ";
                    }
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
                    outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%d</p></td></tr>", node->GetNumChildNodes());

                    outString += AZStd::string::format("<tr><td><p style=\"color:rgb(200,200,200)\"><b>Recursive Children:&nbsp;</b></p></td>");
                    outString += AZStd::string::format("<td><p style=\"color:rgb(115, 115, 115)\">%d</p></td></tr>", node->RecursiveCalcNumNodes());
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
        painter.drawLine(QPoint(0, heightOffset), QPoint(aznumeric_cast<int>(mPlugin->TimeToPixel(animationLength)), heightOffset));
        return 1;
    }
}   // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/TimeView/moc_TrackDataWidget.cpp>
