/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef DRILLER_DrillerCaptureWindow_H
#define DRILLER_DrillerCaptureWindow_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/vector.h>

#include "DrillerNetworkMessages.h"
#include "DrillerMainWindowMessages.h"
#include "AzFramework/TargetManagement/TargetManagementAPI.h"
#include <AzToolsFramework/UI/LegacyFramework/MainWindowSavedState.h>
#include "Workspaces/Workspace.h"
#include "Annotations/Annotations.hxx"


#pragma once

class QMenu;
class QAction;
class QToolbar;
class QSettings;

#include <QMainWindow>
#include <QDockWidget>
#include <Source/Driller/DrillerMainWindowMessages.h>
#endif

namespace Ui
{
    class DrillerCaptureWindow;
}

namespace Driller
{
    class ChannelControl;
    class DrillerDataContainer;
    class CombinedEventsControl;
    class AnnotationHeaderView;
    class ConfigureAnnotationsWindow;

    /*
    This is the original guts of the singular Driller Main Window

    Home of the real commands, channels created from external aggregators when connected, and the floating control panel.
    All inputs end up here where they are interpreted and passed downwards to all channels, to maintain consistency.
    */

    //////////////////////////////////////////////////////////////////////////
    //Main Window
    class DrillerCaptureWindow
        : public QDockWidget
        , public Driller::DrillerNetworkMessages::Bus::Handler
        , public Driller::DrillerCaptureWindowRequestBus::Handler
        , private AzFramework::TargetManagerClient::Bus::Handler
    {
        Q_OBJECT;
    public:
        AZ_TYPE_INFO(DrillerCaptureWindow, "08AF3402-FCFA-4441-910D-9F994BD0D146");
        AZ_CLASS_ALLOCATOR(DrillerCaptureWindow,AZ::SystemAllocator,0);
        DrillerCaptureWindow(CaptureMode captureMode, int identity, QWidget* parent = NULL, Qt::WindowFlags flags = Qt::WindowFlags());
        DrillerCaptureWindow(const DrillerCaptureWindow&)
            : m_identity(0)
        {
            // TODO: Once AZ_NO_COPY macros is in the branch in use it!
            AZ_Assert(false, "You can't copy this class!");
        }
        virtual ~DrillerCaptureWindow(void);

        bool OnGetPermissionToShutDown();

        // DrillerCaptureWindowRequestBus
        void ScrubToFrameRequest(FrameNumberType frameType) override;

    public:

        // Driller network messages
        virtual void ConnectedToNetwork();
        virtual void NewAggregatorList( AggregatorList &theList );
        virtual void AddAggregator( Aggregator &theAggregator );
        virtual void DiscardAggregators();
        virtual void DisconnectedFromNetwork();
        virtual void EndFrame(FrameNumberType frame);
        virtual void NewAggregatorsAvailable();

        // Data Viewer request messages
        // implement AzFramework::TargetManagerClient::Bus::Handler
        void DesiredTargetConnected(bool connected);

    public:
        // MainWindow Messages and states.
        void SaveWindowState();
        AZStd::set<AZ::Uuid> m_inactiveChannels;

        void OnContractAllChannels();
        void OnExpandAllChannels();
        void OnDisableAllChannels();
        void OnEnableAllChannels();
        QString GetDataFileName() { return m_currentDataFilename; }

    private:
        // how the main window identifies us
        const int m_identity;
        CaptureMode m_captureMode;

        // internal workings
        typedef QList<ChannelControl *> SortedChannels;
        SortedChannels m_channels;        
        
        void SetCaptureMode(CaptureMode captureMode);
        void ResetCaptureControls();
        bool IsInLiveMode() const;
        bool IsInCaptureMode(CaptureMode captureMode) const;

        void ClearExistingChannels();
        void ClearChannelDisplay( bool withDeletion );
        void SortChannels();
        void PopulateChannelDisplay();
        ChannelControl* FindChannelControl(Aggregator* aggregator);
        void AddChannelDisplay(ChannelControl *);

    protected:
        // Qt Events
        virtual void closeEvent(QCloseEvent* event);
        virtual void showEvent( QShowEvent * event );
        virtual void hideEvent( QHideEvent * event );
        virtual bool event(QEvent *evt);

        //////////////////////////////////////////////////////////////////////////
        void StateReset();
        QString PrepDataFileForSaving( QString filename, QString workspaceName );
        QString PrepTempFile( QString filename );

        void ScrubberToBegin();
        void ScrubberToEnd();
        void SetScrubberFrame( FrameNumberType frame );
        FrameNumberType GetScrubberFrame() { return m_scrubberCurrentFrame; }
        void SetPlaybackLoopBegin( FrameNumberType frame );
        FrameNumberType GetPlaybackLoopBegin() { return m_playbackLoopBegin; }
        void SetPlaybackLoopEnd( FrameNumberType frame );
        FrameNumberType GetPlaybackLoopEnd() { return m_playbackLoopEnd; }
        void SetFrameRangeBegin( FrameNumberType frame );
        FrameNumberType GetFrameRangeBegin() { return m_frameRangeBegin; }
        void SetFrameRangeEnd( FrameNumberType frame);
        FrameNumberType GetFrameRangeEnd() { return m_frameRangeEnd; }

        void UpdateFrameScrubberbox();
        void ScrubberFrame(FrameNumberType frame);

        void UpdateEventScrubberbox();
        void SetScrubberEvent( EventNumberType eventIdx );

        void UpdateScrollbar( int diff = 0 );
        void FocusScrollbar( FrameNumberType focusFrame );
        void UpdatePlaybackLoopPoints();

        void ConnectChannelControl(ChannelControl *dc);

        void SetCaptureDirty( bool isDirty );

        void UpdateLiveControls();

        AZ::u32 m_windowStateCRC;

        FrameNumberType m_scrubberCurrentFrame;
        FrameNumberType m_frameRangeBegin;
        FrameNumberType m_frameRangeEnd;
        FrameNumberType m_visibleFrames;
        EventNumberType m_scrubberCurrentEvent;
        bool m_captureIsDirty;

        int m_playbackIsActive;
        FrameNumberType m_playbackLoopBegin;
        FrameNumberType m_playbackLoopEnd;
        bool m_draggingPlaybackLoopBegin;
        bool m_draggingPlaybackLoopEnd;
        bool m_draggingAnything;        
        bool m_manipulatingScrollBar;        
        DrillerDataContainer* m_data; ///< Pointer to driller data class. 
        QString m_tmpCaptureFilename;
        QString m_currentDataFilename;
        AnnotationsProvider m_AnnotationProvider;

        bool m_isLoadingFile;

        AnnotationHeaderView* m_ptrAnnotationsHeaderView;
        ConfigureAnnotationsWindow *m_ptrConfigureAnnotationsWindow;
        AZStd::vector<Annotation> m_collectedAnnotations;
        bool m_bForceNextScrub;

        int m_captureId;

        // are we viewing stored data or are we live drilling?        
        bool m_TargetConnected;
        //////////////////////////////////////////////////////////////////////////

        static const int s_availableFrameQuantities[];

    public slots:
        void RepopulateAnnotations();
        void RestoreWindowState();
        void OnMenuCloseCurrentWindow();
        void OnOpen();
        void OnClose();
        void OnCloseFile();
        void OnToBegin();
        void OnToEnd();
        void OnPlayToggled(bool toggleState);
        void OnCaptureToggled(bool toggleState);        
        void OnSliderPressed();
        void OnNewSliderValue(int newValue);
        void OnFrameScrubberboxChanged(int newValue);
        void OnQuantMenuFinal( int range );
        void HandleScrollToFrameRequest( FrameNumberType frame);
        void OnChannelControlMouseDown( Qt::MouseButton whichButton, FrameNumberType frame, FrameNumberType range, int modifiers );
        void OnChannelControlMouseMove( FrameNumberType frame, FrameNumberType range, int modifiers );
        void OnChannelControlMouseUp( Qt::MouseButton whichButton,  FrameNumberType frame, FrameNumberType range, int modifiers );
        void OnChannelControlMouseWheel( FrameNumberType frame, int wheelAmount, FrameNumberType range, int modifiers );
        void OnOpenDrillerFile();
        void OnOpenDrillerFile(QString fileName);
        void OnOpenDrillerFileForWorkspace(QString fileName, QString workspaceFileName);
        void OnOpenWorkspaceFile(QString fileName, bool openDrillerFileAlso); // just open it
        void OnApplyWorkspaceFile(QString fileName);
        void OnSaveDrillerFile();        
        void OnSaveWorkspaceFile(QString fileName, bool automated = false);

        void PlaybackTick();
        void OnUpdateScrollSize();
        void EventRequestEventFocus(EventNumberType eventIdx);
        
        // --- annotations:
        void OnSelectedAnnotationChannelsChanged();
        void OnAnnotationOptionsClick();
        void InformOfMouseOverAnnotation(const Annotation& annotation);
        void InformOfClickAnnotation(const Annotation& annotation);
        void OnAnnotationsDialogDestroyed();
        void CommitAnnotationsCollected();

        void UpdateEndFrameInControls();

        QString GetOpenFileName() const;

signals:
        void ScrubberFrameUpdate( FrameNumberType frame );
        void ShowYourself();
        void HideYourself();
        void OnCaptureModeChange(CaptureMode);
        void CaptureWindowSetToLive(bool);

    public:
        static void Reflect(AZ::ReflectContext* context);

    private:        

        Ui::DrillerCaptureWindow* m_gui;
    };
}

#endif //DRILLER_DrillerCaptureWindow_H
