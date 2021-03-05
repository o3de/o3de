/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#ifndef __EMSTUDIO_ANIMGRAPHPLUGIN_H
#define __EMSTUDIO_ANIMGRAPHPLUGIN_H

// include MCore
#if !defined(Q_MOC_RUN)
#include "../StandardPluginsConfig.h"

#include "../../../../EMStudioSDK/Source/DockWidgetPlugin.h"
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"

#include <MCore/Source/Random.h>
#include <MCore/Source/Array.h>

#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/EventHandler.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/AnimGraphTransitionCondition.h>
#include <EMotionFX/Source/Recorder.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphActionManager.h>

#include <QDockWidget>
#include <QStackedWidget>

#include "AnimGraphOptions.h"
#include "NodeGraph.h"
#include "StateGraphNode.h"
#endif

QT_FORWARD_DECLARE_CLASS(QSettings)
QT_FORWARD_DECLARE_CLASS(QMenu)

namespace EMotionFX
{
    class AnimGraphObjectFactory;
}


namespace EMStudio
{
    // forward declarations
    class AnimGraphModel;
    class BlendGraphWidget;
    class NodeGraph;
    class GraphNode;
    class NodePaletteWidget;
    class NavigateWidget;
    class BlendTreeVisualNode;
    class AttributesWindow;
    class GameControllerWindow;
    class GraphNodeFactory;
    class ParameterWindow;
    class NodeGroupWindow;
    class BlendGraphViewWidget;
    class AnimGraphPlugin;
    class TimeViewPlugin;
    class SaveDirtyAnimGraphFilesCallback;
    class NavigationHistory;

    // our anim graph event handler
    class AnimGraphEventHandler
        : public EMotionFX::EventHandler
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        AnimGraphEventHandler(AnimGraphPlugin* plugin);

        const AZStd::vector<EMotionFX::EventTypes> GetHandledEventTypes() const override { return { EMotionFX::EVENT_TYPE_ON_SET_VISUAL_MANIPULATOR_OFFSET, EMotionFX::EVENT_TYPE_ON_INPUT_PORTS_CHANGED, EMotionFX::EVENT_TYPE_ON_OUTPUT_PORTS_CHANGED, EMotionFX::EVENT_TYPE_ON_RAY_INTERSECTION_TEST, EMotionFX::EVENT_TYPE_ON_DELETE_ANIM_GRAPH, EMotionFX::EVENT_TYPE_ON_DELETE_ANIM_GRAPH_INSTANCE }; }
        void OnSetVisualManipulatorOffset(EMotionFX::AnimGraphInstance* animGraphInstance, uint32 paramIndex, const AZ::Vector3& offset) override;
        void OnInputPortsChanged(EMotionFX::AnimGraphNode* node, const AZStd::vector<AZStd::string>& newInputPorts, const AZStd::string& memberName, const AZStd::vector<AZStd::string>& memberValue) override;
        void OnOutputPortsChanged(EMotionFX::AnimGraphNode* node, const AZStd::vector<AZStd::string>& newOutputPorts, const AZStd::string& memberName, const AZStd::vector<AZStd::string>& memberValue) override;
        bool OnRayIntersectionTest(const AZ::Vector3& start, const AZ::Vector3& end, EMotionFX::IntersectionInfo* outIntersectInfo) override;
        void OnDeleteAnimGraph(EMotionFX::AnimGraph* animGraph) override;
        void OnDeleteAnimGraphInstance(EMotionFX::AnimGraphInstance* animGraphInstance) override;

    private:
        AnimGraphPlugin* mPlugin;
    };

    class AnimGraphPerFrameCallback
    {
    public:
        virtual void ProcessFrame(float timePassedInSeconds) = 0;
    };

    /**
     *
     *
     */
    class AnimGraphPlugin
        : public EMStudio::DockWidgetPlugin
    {
        MCORE_MEMORYOBJECTCATEGORY(AnimGraphPlugin, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);
        Q_OBJECT //AUTOMOC

    public:
        enum
        {
            CLASS_ID = 0x06def5df
        };

        enum
        {
            DISPLAYFLAG_PLAYSPEED       = 1 << 0,
            DISPLAYFLAG_GLOBALWEIGHT    = 1 << 1,
            DISPLAYFLAG_SYNCSTATUS      = 1 << 2,
            DISPLAYFLAG_PLAYPOSITION    = 1 << 3
        };

        AnimGraphPlugin();
        ~AnimGraphPlugin();

        // overloaded
        const char* GetCompileDate() const override;
        const char* GetName() const override;
        uint32 GetClassID() const override;
        const char* GetCreatorName() const override;
        float GetVersion() const override;
        bool GetIsClosable() const override             { return true; }
        bool GetIsFloatable() const override            { return true; }
        bool GetIsVertical() const override             { return false; }
        uint32 GetProcessFramePriority() const override { return 200; }
        void AddWindowMenuEntries(QMenu* parent) override;

        void SetActiveAnimGraph(EMotionFX::AnimGraph* animGraph);
        EMotionFX::AnimGraph* GetActiveAnimGraph()           { return mActiveAnimGraph; }

        void SaveAnimGraph(const char* filename, uint32 animGraphIndex, MCore::CommandGroup* commandGroup = nullptr);
        void SaveAnimGraph(EMotionFX::AnimGraph* animGraph, MCore::CommandGroup* commandGroup = nullptr);
        void SaveAnimGraphAs(EMotionFX::AnimGraph* animGraph, MCore::CommandGroup* commandGroup = nullptr);
        int SaveDirtyAnimGraph(EMotionFX::AnimGraph* animGraph, MCore::CommandGroup* commandGroup, bool askBeforeSaving, bool showCancelButton = true);
        int OnSaveDirtyAnimGraphs();

        PluginOptions* GetOptions() override { return &mOptions; }

        void LoadOptions();
        void SaveOptions();

        void RegisterKeyboardShortcuts() override;

        bool CheckIfCanCreateObject(EMotionFX::AnimGraphObject* parentObject, const EMotionFX::AnimGraphObject* object, EMotionFX::AnimGraphObject::ECategory category) const;

        void ProcessFrame(float timePassedInSeconds) override;

        TimeViewPlugin* FindTimeViewPlugin() const;

        void RegisterPerFrameCallback(AnimGraphPerFrameCallback* callback);
        void UnregisterPerFrameCallback(AnimGraphPerFrameCallback* callback);

        void OnMainWindowClosed() override;

        AnimGraphModel& GetAnimGraphModel() { return *m_animGraphModel; }

        AnimGraphActionManager& GetActionManager() { return *m_actionManager; }

        /// Is the given anim graph running on any selected actor instance?
        bool IsAnimGraphActive(EMotionFX::AnimGraph* animGraph) const;

    public slots:
        void OnFileOpen();
        void OnFileSave();
        void OnFileSaveAs();
        void OnDoubleClickedRecorderNodeHistoryItem(EMotionFX::Recorder::ActorInstanceData* actorInstanceData, EMotionFX::Recorder::NodeHistoryItem* historyItem);
        void OnClickedRecorderNodeHistoryItem(EMotionFX::Recorder::ActorInstanceData* actorInstanceData, EMotionFX::Recorder::NodeHistoryItem* historyItem);

    public:
        BlendGraphWidget* GetGraphWidget()                     { return mGraphWidget; }
        NavigateWidget* GetNavigateWidget()                    { return mNavigateWidget; }
        NodePaletteWidget* GetPaletteWidget()                  { return mPaletteWidget; }
        AttributesWindow* GetAttributesWindow()                { return mAttributesWindow; }
        ParameterWindow* GetParameterWindow()                  { return mParameterWindow; }
        NodeGroupWindow* GetNodeGroupWidget()                  { return mNodeGroupWindow; }
        BlendGraphViewWidget* GetViewWidget()                  { return mViewWidget; }
        NavigationHistory* GetNavigationHistory() const        { return m_navigationHistory; }

        QDockWidget* GetAttributeDock()                        { return mAttributeDock; }
        QDockWidget* GetNodePaletteDock()                      { return mNodePaletteDock; }
        QDockWidget* GetParameterDock()                        { return mParameterDock; }
        QDockWidget* GetNodeGroupDock()                        { return mNodeGroupDock; }

#if AZ_TRAIT_EMOTIONFX_HAS_GAME_CONTROLLER
        GameControllerWindow* GetGameControllerWindow()        { return mGameControllerWindow; }
        QDockWidget* GetGameControllerDock()                   { return mGameControllerDock; }
#endif

        void SetDisplayFlagEnabled(uint32 flags, bool enabled)
        {
            if (enabled)
            {
                mDisplayFlags |= flags;
            }
            else
            {
                mDisplayFlags &= ~flags;
            }
        }
        bool GetIsDisplayFlagEnabled(uint32 flags) const                                   { return (mDisplayFlags & flags); }
        uint32 GetDisplayFlags() const                                                     { return mDisplayFlags; }

        const EMotionFX::AnimGraphObjectFactory* GetAnimGraphObjectFactory() const         { return m_animGraphObjectFactory; }
        GraphNodeFactory* GetGraphNodeFactory()                                            { return mGraphNodeFactory; }

        // overloaded main init function
        void Reflect(AZ::ReflectContext* serializeContext) override;
        bool Init() override;
        void OnAfterLoadLayout() override;
        EMStudioPlugin* Clone() override;

        const AnimGraphOptions& GetAnimGraphOptions() const                                { return mOptions; }

        void SetDisableRendering(bool flag)                                                { mDisableRendering = flag; }
        bool GetDisableRendering() const                                                   { return mDisableRendering; }

        void SetActionFilter(const AnimGraphActionFilter& actionFilter);
        const AnimGraphActionFilter& GetActionFilter() const;

    private:
        enum EDockWindowOptionFlag
        {
            WINDOWS_PARAMETERWINDOW = 1,
            WINDOWS_ATTRIBUTEWINDOW = 2,
            WINDOWS_NODEGROUPWINDOW = 3,
            WINDOWS_PALETTEWINDOW = 4,
            WINDOWS_GAMECONTROLLERWINDOW = 5,

            NUM_DOCKWINDOW_OPTIONS //automatically gets the next number assigned
        };

        MCORE_DEFINECOMMANDCALLBACK(CommandActivateAnimGraphCallback);

        MCORE_DEFINECOMMANDCALLBACK(CommandSelectCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandUnselectCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandClearSelectionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandRecorderClearCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandPlayMotionCallback);

        AZStd::vector<MCore::Command::Callback*>    m_commandCallbacks;
        AZStd::vector<AnimGraphPerFrameCallback*>   mPerFrameCallbacks;

        bool                                        mDisableRendering;

        AnimGraphEventHandler                       mEventHandler;

        BlendGraphWidget*                           mGraphWidget;
        NavigateWidget*                             mNavigateWidget;
        NodePaletteWidget*                          mPaletteWidget;
        AttributesWindow*                           mAttributesWindow;
        ParameterWindow*                            mParameterWindow;
        NodeGroupWindow*                            mNodeGroupWindow;
        BlendGraphViewWidget*                       mViewWidget;
        NavigationHistory*                          m_navigationHistory;

        SaveDirtyAnimGraphFilesCallback*            mDirtyFilesCallback;

        QDockWidget*                                mAttributeDock;
        QDockWidget*                                mNodePaletteDock;
        QDockWidget*                                mParameterDock;
        QDockWidget*                                mNodeGroupDock;
        QAction*                                    mDockWindowActions[NUM_DOCKWINDOW_OPTIONS];
        EMotionFX::AnimGraph*                       mActiveAnimGraph;

#if AZ_TRAIT_EMOTIONFX_HAS_GAME_CONTROLLER
        GameControllerWindow*                       mGameControllerWindow;
        QPointer<QDockWidget>                       mGameControllerDock;
#endif

        float                                       mLastPlayTime;
        float                                       mTotalTime;

        uint32                                      mDisplayFlags;

        AnimGraphOptions                            mOptions;

        EMotionFX::AnimGraphObjectFactory*          m_animGraphObjectFactory;
        GraphNodeFactory*                           mGraphNodeFactory;

        // Model used for the MVC pattern
        AnimGraphModel*                             m_animGraphModel;

        // Helper class to handle copy/cut/paste
        AnimGraphActionManager*                     m_actionManager;
        AnimGraphActionFilter                       m_actionFilter;

        void InitForAnimGraph(EMotionFX::AnimGraph* setup);
        bool GetOptionFlag(EDockWindowOptionFlag option) { return mDockWindowActions[(uint32)option]->isChecked(); }
        void SetOptionFlag(EDockWindowOptionFlag option, bool isEnabled);
        void SetOptionEnabled(EDockWindowOptionFlag option, bool isEnabled);

    private slots:
        void UpdateWindowVisibility(EDockWindowOptionFlag option, bool checked);
        void UpdateWindowActionsCheckState();
    };
}   // namespace EMStudio


#endif
