/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include MCore
#if !defined(Q_MOC_RUN)
#include "../StandardPluginsConfig.h"

#include "../../../../EMStudioSDK/Source/DockWidgetPlugin.h"
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"

#include <MCore/Source/Random.h>
#include <AzCore/std/containers/vector.h>

#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/EventHandler.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/AnimGraphTransitionCondition.h>
#include <EMotionFX/Source/Recorder.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphActionManager.h>

#include <GraphCanvas/Editor/EditorTypes.h>

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
    class GraphNodeFactory;
    class ParameterWindow;
    class BlendGraphViewWidget;
    class AnimGraphPlugin;
    class TimeViewPlugin;
    class NavigationHistory;

    static constexpr GraphCanvas::EditorId AnimGraphEditorId = AZ_CRC_CE("AnimGraphEditor");

    // our anim graph event handler
    class AnimGraphEventHandler
        : public EMotionFX::EventHandler
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        AnimGraphEventHandler(AnimGraphPlugin* plugin);

        const AZStd::vector<EMotionFX::EventTypes> GetHandledEventTypes() const override { return { EMotionFX::EVENT_TYPE_ON_SET_VISUAL_MANIPULATOR_OFFSET, EMotionFX::EVENT_TYPE_ON_INPUT_PORTS_CHANGED, EMotionFX::EVENT_TYPE_ON_OUTPUT_PORTS_CHANGED, EMotionFX::EVENT_TYPE_ON_RAY_INTERSECTION_TEST, EMotionFX::EVENT_TYPE_ON_DELETE_ANIM_GRAPH, EMotionFX::EVENT_TYPE_ON_DELETE_ANIM_GRAPH_INSTANCE }; }
        void OnSetVisualManipulatorOffset(EMotionFX::AnimGraphInstance* animGraphInstance, size_t paramIndex, const AZ::Vector3& offset) override;
        void OnInputPortsChanged(EMotionFX::AnimGraphNode* node, const AZStd::vector<AZStd::string>& newInputPorts, const AZStd::string& memberName, const AZStd::vector<AZStd::string>& memberValue) override;
        void OnOutputPortsChanged(EMotionFX::AnimGraphNode* node, const AZStd::vector<AZStd::string>& newOutputPorts, const AZStd::string& memberName, const AZStd::vector<AZStd::string>& memberValue) override;
        bool OnRayIntersectionTest(const AZ::Vector3& start, const AZ::Vector3& end, EMotionFX::IntersectionInfo* outIntersectInfo) override;
        void OnDeleteAnimGraph(EMotionFX::AnimGraph* animGraph) override;
        void OnDeleteAnimGraphInstance(EMotionFX::AnimGraphInstance* animGraphInstance) override;

    private:
        AnimGraphPlugin* m_plugin;
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
        const char* GetName() const override;
        uint32 GetClassID() const override;
        bool GetIsClosable() const override             { return true; }
        bool GetIsFloatable() const override            { return true; }
        bool GetIsVertical() const override             { return false; }
        uint32 GetProcessFramePriority() const override { return 200; }
        void AddWindowMenuEntries(QMenu* parent) override;

        void SetActiveAnimGraph(EMotionFX::AnimGraph* animGraph);
        EMotionFX::AnimGraph* GetActiveAnimGraph()           { return m_activeAnimGraph; }

        int OnSaveDirtyAnimGraphs();

        PluginOptions* GetOptions() override { return &m_options; }

        void LoadOptions();
        void SaveOptions();

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

        inline static constexpr AZStd::string_view s_animGraphWindowShortcutGroupName = "Anim Graph Window";
        inline static constexpr AZStd::string_view s_fitEntireGraphShortcutName = "Fit Entire Graph";
        inline static constexpr AZStd::string_view s_zoomOnSelectedNodesShortcutName = "Zoom On Selected Nodes";
        inline static constexpr AZStd::string_view s_openParentNodeShortcutName = "Open Parent Node";
        inline static constexpr AZStd::string_view s_openSelectedNodeShortcutName = "Open Selected Node";
        inline static constexpr AZStd::string_view s_historyBackShortcutName = "History Back";
        inline static constexpr AZStd::string_view s_historyForwardShortcutName = "History Forward";
        inline static constexpr AZStd::string_view s_alignLeftShortcutName = "Align Left";
        inline static constexpr AZStd::string_view s_alignRightShortcutName = "Align Right";
        inline static constexpr AZStd::string_view s_alignTopShortcutName = "Align Top";
        inline static constexpr AZStd::string_view s_alignBottomShortcutName = "Align Bottom";
        inline static constexpr AZStd::string_view s_cutShortcutName = "Cut";
        inline static constexpr AZStd::string_view s_copyShortcutName = "Copy";
        inline static constexpr AZStd::string_view s_pasteShortcutName = "Paste";
        inline static constexpr AZStd::string_view s_selectAllShortcutName = "Select All";
        inline static constexpr AZStd::string_view s_unselectAllShortcutName = "Unselect All";
        inline static constexpr AZStd::string_view s_deleteSelectedNodesShortcutName = "Delete Selected Nodes";

    signals:
        void ActionFilterChanged();

    public slots:
        void OnFileOpen();
        void OnFileSave();
        void OnFileSaveAs();
        void OnDoubleClickedRecorderNodeHistoryItem(EMotionFX::Recorder::ActorInstanceData* actorInstanceData, EMotionFX::Recorder::NodeHistoryItem* historyItem);
        void OnClickedRecorderNodeHistoryItem(EMotionFX::Recorder::ActorInstanceData* actorInstanceData, EMotionFX::Recorder::NodeHistoryItem* historyItem);

    public:
        BlendGraphWidget* GetGraphWidget()                     { return m_graphWidget; }
        NavigateWidget* GetNavigateWidget()                    { return m_navigateWidget; }
        NodePaletteWidget* GetPaletteWidget()                  { return m_paletteWidget; }
        AttributesWindow* GetAttributesWindow()                { return m_attributesWindow; }
        ParameterWindow* GetParameterWindow()                  { return m_parameterWindow; }
        BlendGraphViewWidget* GetViewWidget()                  { return m_viewWidget; }
        NavigationHistory* GetNavigationHistory() const        { return m_navigationHistory; }

        QDockWidget* GetNodePaletteDock()                      { return m_nodePaletteDock; }
        QDockWidget* GetParameterDock()                        { return m_parameterDock; }

        void SetDisplayFlagEnabled(uint32 flags, bool enabled)
        {
            if (enabled)
            {
                m_displayFlags |= flags;
            }
            else
            {
                m_displayFlags &= ~flags;
            }
        }
        bool GetIsDisplayFlagEnabled(uint32 flags) const                                   { return (m_displayFlags & flags); }
        uint32 GetDisplayFlags() const                                                     { return m_displayFlags; }

        const EMotionFX::AnimGraphObjectFactory* GetAnimGraphObjectFactory() const         { return m_animGraphObjectFactory; }
        GraphNodeFactory* GetGraphNodeFactory()                                            { return m_graphNodeFactory; }

        // overloaded main init function
        void Reflect(AZ::ReflectContext* serializeContext) override;
        bool Init() override;
        void OnAfterLoadLayout() override;
        EMStudioPlugin* Clone() const override { return new AnimGraphPlugin(); }

        const AnimGraphOptions& GetAnimGraphOptions() const                                { return m_options; }

        void SetDisableRendering(bool flag)                                                { m_disableRendering = flag; }
        bool GetDisableRendering() const                                                   { return m_disableRendering; }

        void SetActionFilter(const AnimGraphActionFilter& actionFilter);
        const AnimGraphActionFilter& GetActionFilter() const;
        void FileOpen(AZStd::string filename);

    private:
        enum EDockWindowOptionFlag
        {
            WINDOWS_PARAMETERWINDOW = 1,
            WINDOWS_PALETTEWINDOW = 2,

            NUM_DOCKWINDOW_OPTIONS //automatically gets the next number assigned
        };

        MCORE_DEFINECOMMANDCALLBACK(CommandActivateAnimGraphCallback);

        MCORE_DEFINECOMMANDCALLBACK(CommandSelectCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandUnselectCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandClearSelectionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandRecorderClearCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandPlayMotionCallback);

        AZStd::vector<MCore::Command::Callback*>    m_commandCallbacks;
        AZStd::vector<AnimGraphPerFrameCallback*>   m_perFrameCallbacks;

        bool                                        m_disableRendering;

        AnimGraphEventHandler                       m_eventHandler;

        BlendGraphWidget*                           m_graphWidget;
        NavigateWidget*                             m_navigateWidget;
        NodePaletteWidget*                          m_paletteWidget;
        AttributesWindow*                           m_attributesWindow;
        ParameterWindow*                            m_parameterWindow;
        BlendGraphViewWidget*                       m_viewWidget;
        NavigationHistory*                          m_navigationHistory;

        QDockWidget*                                m_nodePaletteDock;
        QDockWidget*                                m_parameterDock;
        QAction*                                    m_dockWindowActions[NUM_DOCKWINDOW_OPTIONS];
        EMotionFX::AnimGraph*                       m_activeAnimGraph;

        float                                       m_lastPlayTime;
        float                                       m_totalTime;

        uint32                                      m_displayFlags;

        AnimGraphOptions                            m_options;

        EMotionFX::AnimGraphObjectFactory*          m_animGraphObjectFactory;
        GraphNodeFactory*                           m_graphNodeFactory;

        // Model used for the MVC pattern
        AnimGraphModel*                             m_animGraphModel;

        // Helper class to handle copy/cut/paste
        AnimGraphActionManager*                     m_actionManager;
        AnimGraphActionFilter                       m_actionFilter;

        void InitForAnimGraph(EMotionFX::AnimGraph* setup);
        bool GetOptionFlag(EDockWindowOptionFlag option) { return m_dockWindowActions[(uint32)option]->isChecked(); }
        void SetOptionFlag(EDockWindowOptionFlag option, bool isEnabled);
        void SetOptionEnabled(EDockWindowOptionFlag option, bool isEnabled);

    private slots:
        void UpdateWindowVisibility(EDockWindowOptionFlag option, bool checked);
        void UpdateWindowActionsCheckState();
    };
}   // namespace EMStudio
