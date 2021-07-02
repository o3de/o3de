/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/PlatformIncl.h>
#include <EMotionFX/CommandSystem/Source/MotionEventCommands.h>
#include <EMotionFX/Source/Recorder.h>
#include <Editor/AnimGraphEditorBus.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/DockWidgetPlugin.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/TimeView/TimeTrack.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/TimeView/TimeViewShared.h>
#include <QScrollArea>
#endif

namespace EMStudio
{
    // forward declarations
    class TrackDataHeaderWidget;
    class TrackDataHeaderWidget;
    class TrackDataWidget;
    class TrackHeaderWidget;
    class TimeInfoWidget;
    class TimeViewToolBar;
    class MotionWindowPlugin;
    class MotionEventsPlugin;
    class MotionListWindow;
    class MotionEventPresetsWidget;
    class MotionSetsWindowPlugin;

    struct EventSelectionItem
    {
        MCORE_MEMORYOBJECTCATEGORY(EventSelectionItem, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

        EMotionFX::MotionEvent* GetMotionEvent();
        EMotionFX::MotionEventTrack* GetEventTrack();

        uint32                          mEventNr;// the motion event index in its track
        size_t                          mTrackNr;// the corresponding track in which the event is in
        EMotionFX::Motion*              mMotion;// the parent motion of the event track
    };

    class TimeViewPlugin
        : public EMStudio::DockWidgetPlugin
        , private EMotionFX::AnimGraphEditorNotificationBus::Handler
    {
        Q_OBJECT // AUTOMOC
        MCORE_MEMORYOBJECTCATEGORY(TimeViewPlugin, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);
        
        friend class TrackDataHeaderWidget;
        friend class TrackDataWidget;
        friend class TimeViewWidget;
        friend class TimeViewToolBar;
        friend class TimeInfoWidget;
        friend class TrackHeaderWidget;

    public:
        enum
        {
            CLASS_ID = 0x00fab009
        };

        TimeViewPlugin();
        ~TimeViewPlugin();

        // overloaded
        const char* GetCompileDate() const override;
        const char* GetName() const override;
        uint32 GetClassID() const override;
        const char* GetCreatorName() const override;
        float GetVersion() const override;
        bool GetIsClosable() const override             { return true; }
        bool GetIsFloatable() const override            { return true; }
        bool GetIsVertical() const override             { return false; }

        // overloaded main init function
        bool Init() override;
        EMStudioPlugin* Clone() override;

        void OnBeforeRemovePlugin(uint32 classID) override;

        void ProcessFrame(float timePassedInSeconds) override;

        void SetMode(TimeViewMode mode);
        TimeViewMode GetMode() const { return m_mode; }

        double GetScrollX() const       { return mScrollX; }

        void DeltaScrollX(double deltaX, bool animate = true);

        void SetScrollX(double scrollX, bool animate = true);

        void CalcTime(double xPixel, double* outPixelTime, uint32* outMinutes, uint32* outSeconds, uint32* outMilSecs, uint32* outFrameNr, bool scaleXPixel = true) const;
        void DecomposeTime(double timeValue, uint32* outMinutes, uint32* outSeconds, uint32* outMilSecs, uint32* outFrameNr) const;
        double PixelToTime(double xPixel, bool isScaledPixel = true) const;
        double TimeToPixel(double timeInSeconds, bool scale = true) const;
        double GetCurrentTime() const;  // returns time in seconds
        void SetCurrentTime(double timeInSeconds);  // set time in seconds, auto updates visuals (redraw)
        void UpdateVisualData();
        bool SnapTime(double* inOutTime, TimeTrackElement* elementToIgnore, double snapThreshold);

        void ZoomToFit();
        void GetDataTimes(double* outMaxTime, double* outClipStart, double* outClipEnd) const;
        double CalcFitScale(double minScale = 1.0, double maxScale = 100.0) const;
        void MakeTimeVisible(double timeValue, double offsetFactor = 0.95, bool animate = true);
        bool GetIsTimeVisible(double timeValue) const;
        float GetTimeScale() const                  { return aznumeric_cast<float>(mTimeScale); }

        void RenderElementTimeHandles(QPainter& painter, uint32 dataWindowHeight, const QPen& pen);
        void DisableAllToolTips();

        void AddTrack(TimeTrack* track);
        void RemoveAllTracks();
        TimeTrack* GetTrack(uint32 index)           { return mTracks[index]; }
        uint32 GetNumTracks() const                 { return mTracks.GetLength(); }
        AZ::Outcome<AZ::u32> FindTrackIndex(const TimeTrack* track) const;
        TimeTrack* FindTrackByElement(TimeTrackElement* element) const;

        void UnselectAllElements();

        double RoundDouble(double x) const;

        TimeTrackElement* GetElementAt(int32 x, int32 y);
        TimeTrack* GetTrackAt(int32 y);

        void UpdateViewSettings();
        void SetScale(double scale, bool animate = true);

        bool FindResizePoint(int32 x, int32 y, TimeTrackElement** outElement, uint32* outID);

        QCursor* GetZoomInCursor() const            { return mZoomInCursor; }
        QCursor* GetZoomOutCursor() const           { return mZoomOutCursor; }

        // some getters
        TrackDataHeaderWidget* GetTrackDataHeaderWidget() { return mTrackDataHeaderWidget; }
        TrackDataWidget*    GetTrackDataWidget()    { return mTrackDataWidget; }
        TrackHeaderWidget*  GetTrackHeaderWidget()  { return mTrackHeaderWidget; }
        TimeInfoWidget*     GetTimeInfoWidget()     { return mTimeInfoWidget; }
        TimeViewToolBar*    GetTimeViewToolBar()    { return mTimeViewToolBar; }

        void OnKeyPressEvent(QKeyEvent* event);
        void OnKeyReleaseEvent(QKeyEvent* event);

        void ValidatePluginLinks();
        void UpdateSelection();
        void UpdateMaxHeight();

        void ZoomRect(const QRect& rect);

        uint32 GetNumSelectedEvents()                                                   { return mSelectedEvents.GetLength(); }
        EventSelectionItem GetSelectedEvent(uint32 index) const                         { return mSelectedEvents[index]; }

        void Select(const MCore::Array<EventSelectionItem>& selection);

        MCORE_INLINE EMotionFX::Motion* GetMotion() const                               { return mMotion; }
        void SetRedrawFlag();

        uint32 CalcContentHeight() const;

    public slots:
        void ReInit();

    public slots:
        void VisibilityChanged(bool visible);
        void OnSelectionChanged()                   { emit SelectionChanged(); }
        void MotionSelectionChanged();

        void AddMotionEvent(int32 x, int32 y);
        void RemoveMotionEvent(int32 x, int32 y);
        void MotionEventChanged(TimeTrackElement* element, double startTime, double endTime);
        void RemoveSelectedMotionEvents();
        void RemoveAllMotionEvents();
        void OnZoomAll();
        void OnGotoTimeZero();
        void OnResetTimeline();
        void OnCenterOnCurTime();
        void OnShowNodeHistoryNodeInGraph();
        void OnClickNodeHistoryNode();
        void MotionEventTrackChanged(uint32 eventNr, float startTime, float endTime, const char* oldTrackName, const char* newTrackName)            { UnselectAllElements(); CommandSystem::CommandHelperMotionEventTrackChanged(eventNr, startTime, endTime, oldTrackName, newTrackName); }
        void OnManualTimeChange(float timeValue);

    signals:
        void SelectionChanged();
        void DeleteKeyPressed();
        void ManualTimeChangeStart(float newTime);  // single click scrub in time line (on mouse down)
        void ManualTimeChange(float newTime);       // scrubbing in the time line (moving with the mouse while mouse button clicked)
        void DoubleClickedRecorderNodeHistoryItem(EMotionFX::Recorder::ActorInstanceData* actorInstanceData, EMotionFX::Recorder::NodeHistoryItem* item);       // double clicked an item
        void ClickedRecorderNodeHistoryItem(EMotionFX::Recorder::ActorInstanceData* actorInstanceData, EMotionFX::Recorder::NodeHistoryItem* item);             // left clicked an item
        void RecorderStateChanged();

    private:
        // AnimGraphEditorNotificationBus
        void OnFocusIn() override;
        void OnShow() override;

        MCORE_DEFINECOMMANDCALLBACK(CommandAdjustMotionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandSelectCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandUnselectCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandClearSelectionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandRecorderClearCallback);
        MCORE_DEFINECOMMANDCALLBACK(UpdateInterfaceCallback);
        AZStd::vector<MCore::Command::Callback*> m_commandCallbacks;

        TrackDataHeaderWidget* mTrackDataHeaderWidget;
        TrackDataWidget*    mTrackDataWidget;
        TrackHeaderWidget*  mTrackHeaderWidget;
        TimeInfoWidget*     mTimeInfoWidget;
        TimeViewToolBar*    mTimeViewToolBar;
        QWidget*            mMainWidget;

        TimeViewMode m_mode = TimeViewMode::None;
        EMotionFX::Motion*                  mMotion;
        MotionWindowPlugin*                 mMotionWindowPlugin;
        MotionEventsPlugin*                 mMotionEventsPlugin;
        MotionListWindow*                   mMotionListWindow;
        MotionSetsWindowPlugin*             m_motionSetPlugin;
        MCore::Array<EventSelectionItem>    mSelectedEvents;

        EMotionFX::Recorder::ActorInstanceData* mActorInstanceData;
        EMotionFX::Recorder::NodeHistoryItem*   mNodeHistoryItem;
        EMotionFX::Recorder::EventHistoryItem*  mEventHistoryItem;
        EMotionFX::AnimGraphNode*              mEventEmitterNode;

        struct MotionInfo
        {
            uint32      mMotionID;
            bool        mInitialized;
            double      mScale;
            double      mScrollX;
        };

        MotionInfo* FindMotionInfo(uint32 motionID);
        void UpdateCurrentMotionInfo();

        MCore::Array<MotionInfo*>   mMotionInfos;
        MCore::Array<TimeTrack*>    mTracks;

        double              mPixelsPerSecond;   // pixels per second
        double              mScrollX;           // horizontal scroll offset
        double              mCurTime;           // current time
        double              mFPS;               // the frame rate, used to snap time values to and to calculate frame numbers
        double              mCurMouseX;
        double              mCurMouseY;
        double              mMaxTime;           // the end time of the full time bar
        double              mMaxHeight;
        double              mLastMaxHeight;
        double              mTimeScale;         // the time zoom scale factor
        double              mMaxScale;
        double              mMinScale;
        float               mTotalTime;

        double              mTargetTimeScale;
        double              mTargetScrollX;

        bool                mIsAnimating;
        bool                mDirty;

        QCursor*            mZoomInCursor;
        QCursor*            mZoomOutCursor;

        QPen                mPenCurTimeHandle;
        QPen                mPenTimeHandles;
        QPen                mPenCurTimeHelper;
        QBrush              mBrushCurTimeHandle;
    };
}   // namespace EMStudio
