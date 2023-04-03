/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/containers/unordered_map.h>
#include <Editor/ActorEditorBus.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/StandardPluginsConfig.h>
#include <QStackedWidget>
#include <QAction>
#include <QWidget>
#include <QAction>
#endif

QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QMenuBar)
QT_FORWARD_DECLARE_CLASS(QHBoxLayout)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QShowEvent)
QT_FORWARD_DECLARE_CLASS(QSplitter)
QT_FORWARD_DECLARE_CLASS(QToolBar)
QT_FORWARD_DECLARE_CLASS(QLayout)

namespace EMotionFX { class AnimGraphNode; }

namespace EMStudio
{
    // forward declarations
    class AnimGraphNodeWidget;
    class AnimGraphPlugin;
    class BlendGraphWidget;
    class NavigationHistory;
    class NavigationLinkWidget;

    class BlendGraphViewWidget
        : public QWidget
        , public EMotionFX::ActorEditorRequestBus::Handler
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendGraphViewWidget, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);
        Q_OBJECT // AUTOMOC

    public:
        enum EOptionFlag
        {
            SELECTION_ALIGNLEFT,
            SELECTION_ALIGNRIGHT,
            SELECTION_ALIGNTOP,
            SELECTION_ALIGNBOTTOM,
            SELECTION_SELECTALL,
            SELECTION_UNSELECTALL,
            FILE_NEW,
            FILE_OPEN,
            FILE_SAVE,
            FILE_SAVEAS,
            NAVIGATION_FORWARD,
            NAVIGATION_BACK,
            NAVIGATION_NAVPANETOGGLE,
            NAVIGATION_OPEN_SELECTED,
            NAVIGATION_TO_PARENT,
            NAVIGATION_FRAME_ALL,
            NAVIGATION_ZOOMSELECTION,
            ACTIVATE_ANIMGRAPH,
            VISUALIZATION_PLAYSPEEDS,
            VISUALIZATION_GLOBALWEIGHTS,
            VISUALIZATION_SYNCSTATUS,
            VISUALIZATION_PLAYPOSITIONS,
#if defined(EMFX_ANIMGRAPH_PROFILER_ENABLED)
            VISUALIZATION_PROFILING_NONE,
            VISUALIZATION_PROFILING_UPDATE,
            VISUALIZATION_PROFILING_TOPDOWN,
            VISUALIZATION_PROFILING_POSTUPDATE,
            VISUALIZATION_PROFILING_OUTPUT,
            VISUALIZATION_PROFILING_ALL,
#endif
            EDIT_CUT,
            EDIT_COPY,
            EDIT_PASTE,
            EDIT_DELETE,

            NUM_OPTIONS //automatically gets the next number assigned
        };

        BlendGraphViewWidget(AnimGraphPlugin* plugin, QWidget* parentWidget);
        ~BlendGraphViewWidget();

        bool GetOptionFlag(EOptionFlag option) const { return m_actions[(uint32)option]->isChecked(); }
        void SetOptionFlag(EOptionFlag option, bool isEnabled);
        void SetOptionEnabled(EOptionFlag option, bool isEnabled);

        void Init(BlendGraphWidget* blendGraphWidget);
        void UpdateAnimGraphOptions();
        void UpdateEnabledActions();

        // If there is a specific widget to handle this node returns that.
        // Else, returns nullptr.
        AnimGraphNodeWidget* GetWidgetForNode(const EMotionFX::AnimGraphNode* node);

        QAction* GetAction(EOptionFlag option) const { return m_actions[option]; }

    public slots:
        void OnFocusChanged(const QModelIndex& newFocusIndex, const QModelIndex& newFocusParent, const QModelIndex& oldFocusIndex, const QModelIndex& oldFocusParent);

        void OnCreateAnimGraph();

        void NavigateToRoot();
        void ToggleNavigationPane();

        void ZoomSelected();
        void ZoomToAll();

        void OnActivateState();

        void OnDisplayPlaySpeeds();
        void OnDisplayGlobalWeights();
        void OnDisplaySyncStatus();
        void OnDisplayPlayPositions();

    protected:
        void UpdateNavigation();

        void OpenAnimGraph(EMotionFX::AnimGraph* animGraph);

        void BuildOpenMenu();

        void showEvent(QShowEvent* showEvent);

#if defined(EMFX_ANIMGRAPH_PROFILER_ENABLED)
        void AddProfilingAction(const char* actionName, EOptionFlag optionFlag);
        void OnDisplayProfiling(EOptionFlag profileOption);
        void OnDisplayAllProfiling(bool enableAll); // enabledAll = false -> disableAllProfiling
#endif

    private:
        void CreateActions();
        QToolBar* CreateTopToolBar();
        QToolBar* CreateNavigationToolBar();

        QMenuBar* m_menu = nullptr;
        QMenu* m_openMenu = nullptr;
        QHBoxLayout* m_toolbarLayout = nullptr;
        AZStd::array<QAction*, NUM_OPTIONS> m_actions{};
        AnimGraphPlugin* m_parentPlugin = nullptr;
        NavigationLinkWidget* m_navigationLink = nullptr;
        QStackedWidget m_viewportStack;
        QSplitter* m_viewportSplitter = nullptr;

        // This maps a node's UUID to a widget that will be used to
        // display the "contents" of that node type.  If no entry for a given
        // node type is found, then a BlendGraphWidget is used by default.
        // For normal blend trees and state machines, the BlendGraphWidget is
        // shown to draw the nodes inside the tree.  For special types like a
        // blendspace, a separate widget is registered to handle the drawing for that
        // node.
        AZStd::unordered_map<AZ::TypeId, AnimGraphNodeWidget*> m_nodeTypeToWidgetMap;
    };
}   // namespace EMStudio
