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
            SELECTION_ALIGNLEFT                 = 0,
            SELECTION_ALIGNRIGHT                = 1,
            SELECTION_ALIGNTOP                  = 2,
            SELECTION_ALIGNBOTTOM               = 3,
            FILE_NEW                            = 4,
            FILE_OPENFILE                       = 5,
            FILE_OPEN                           = 6,
            FILE_SAVE                           = 7,
            FILE_SAVEAS                         = 8,
            NAVIGATION_FORWARD                  = 9,
            NAVIGATION_BACK                     = 10,
            NAVIGATION_NAVPANETOGGLE            = 11,
            SELECTION_ZOOMSELECTION             = 12,
            ACTIVATE_ANIMGRAPH                  = 13,
            WINDOWS_NODEGROUPWINDOW             = 14,
            VISUALIZATION_PLAYSPEEDS            = 15,
            VISUALIZATION_GLOBALWEIGHTS         = 16,
            VISUALIZATION_SYNCSTATUS            = 17,
            VISUALIZATION_PLAYPOSITIONS         = 18,

            NUM_OPTIONS //automatically gets the next number assigned
        };

        BlendGraphViewWidget(AnimGraphPlugin* plugin, QWidget* parentWidget);
        ~BlendGraphViewWidget();

        bool GetOptionFlag(EOptionFlag option) const { return m_actions[(uint32)option]->isChecked(); }
        void SetOptionFlag(EOptionFlag option, bool isEnabled);
        void SetOptionEnabled(EOptionFlag option, bool isEnabled);

        void Init(BlendGraphWidget* blendGraphWidget);
        void UpdateAnimGraphOptions();
        void UpdateSelection();

        // If there is a specific widget to handle this node returns that.
        // Else, returns nullptr.
        AnimGraphNodeWidget* GetWidgetForNode(const EMotionFX::AnimGraphNode* node);

        // Get Actions (used for testing purposes)
        QAction* GetAction(EOptionFlag option) const { return m_actions[option]; }

    public slots:
        void OnFocusChanged(const QModelIndex& newFocusIndex, const QModelIndex& newFocusParent, const QModelIndex& oldFocusIndex, const QModelIndex& oldFocusParent);

        void OnCreateAnimGraph();

        void NavigateToRoot();
        void NavigateToNode();
        void NavigateToParent();
        void ToggleNavigationPane();

        void ZoomSelected();

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

        void keyReleaseEvent(QKeyEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;

    private:
        QToolBar* CreateTopToolBar();
        QToolBar* CreateNavigationToolBar();

        QMenuBar* m_menu = nullptr;
        QMenu* m_openMenu = nullptr;
        QHBoxLayout* m_toolbarLayout = nullptr;
        QAction* m_actions[NUM_OPTIONS];
        AnimGraphPlugin* m_parentPlugin = nullptr;
        NavigationLinkWidget* mNavigationLink = nullptr;
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
