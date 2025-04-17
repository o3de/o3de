/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ActionManager/HotKey/HotKeyManagerInterface.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphModel.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphNodeWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphActionManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphViewWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendSpace1DNodeWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendSpace2DNodeWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NavigateWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NavigationHistory.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NavigationLinkWidget.h>
#include <MysticQt/Source/KeyboardShortcutManager.h>
#include <Editor/AnimGraphEditorBus.h>
#include <QKeyEvent>
#include <QPushButton>
#include <QShowEvent>
#include <QSplitter>
#include <QToolBar>
#include <QVBoxLayout>

namespace EMStudio
{
    constexpr AZStd::string_view AnimationEditorAnimGraphActionContextIdentifier = "o3de.context.animationEditor.animGraph";

    BlendGraphViewWidget::BlendGraphViewWidget(AnimGraphPlugin* plugin, QWidget* parentWidget)
        : QWidget(parentWidget)
        , m_parentPlugin(plugin)
    {
        EMotionFX::ActorEditorRequestBus::Handler::BusConnect();
    }

    void BlendGraphViewWidget::CreateActions()
    {
        MysticQt::KeyboardShortcutManager* shortcutManager = GetMainWindow()->GetShortcutManager();

        m_actions[SELECTION_ALIGNLEFT] = new QAction(
            QIcon(":/EMotionFX/AlignLeft.svg"),
            FromStdString(AnimGraphPlugin::s_alignLeftShortcutName),
            this);
        m_actions[SELECTION_ALIGNLEFT]->setShortcut(Qt::Key_L | Qt::ControlModifier);
        shortcutManager->RegisterKeyboardShortcut(m_actions[SELECTION_ALIGNLEFT], AnimGraphPlugin::s_animGraphWindowShortcutGroupName, true);
        connect(m_actions[SELECTION_ALIGNLEFT], &QAction::triggered, &m_parentPlugin->GetActionManager(), &AnimGraphActionManager::AlignLeft);

        m_actions[SELECTION_ALIGNRIGHT] = new QAction(
            QIcon(":/EMotionFX/AlignRight.svg"),
            FromStdString(AnimGraphPlugin::s_alignRightShortcutName),
            this);
        m_actions[SELECTION_ALIGNRIGHT]->setShortcut(Qt::Key_R | Qt::ControlModifier);
        shortcutManager->RegisterKeyboardShortcut(m_actions[SELECTION_ALIGNRIGHT], AnimGraphPlugin::s_animGraphWindowShortcutGroupName, true);
        connect(m_actions[SELECTION_ALIGNRIGHT], &QAction::triggered, &m_parentPlugin->GetActionManager(), &AnimGraphActionManager::AlignRight);

        m_actions[SELECTION_ALIGNTOP] = new QAction(
            QIcon(":/EMotionFX/AlignTop.svg"),
            FromStdString(AnimGraphPlugin::s_alignTopShortcutName),
            this);
        m_actions[SELECTION_ALIGNTOP]->setShortcut(Qt::Key_T | Qt::ControlModifier);
        shortcutManager->RegisterKeyboardShortcut(m_actions[SELECTION_ALIGNTOP], AnimGraphPlugin::s_animGraphWindowShortcutGroupName, true);
        connect(m_actions[SELECTION_ALIGNTOP], &QAction::triggered, &m_parentPlugin->GetActionManager(), &AnimGraphActionManager::AlignTop);

        m_actions[SELECTION_ALIGNBOTTOM] = new QAction(
            QIcon(":/EMotionFX/AlignBottom.svg"),
            FromStdString(AnimGraphPlugin::s_alignBottomShortcutName),
            this);
        m_actions[SELECTION_ALIGNBOTTOM]->setShortcut(Qt::Key_B | Qt::ControlModifier);
        shortcutManager->RegisterKeyboardShortcut(m_actions[SELECTION_ALIGNBOTTOM], AnimGraphPlugin::s_animGraphWindowShortcutGroupName, true);
        connect(m_actions[SELECTION_ALIGNBOTTOM], &QAction::triggered, &m_parentPlugin->GetActionManager(), &AnimGraphActionManager::AlignBottom);

        m_actions[SELECTION_SELECTALL] = new QAction(
            FromStdString(AnimGraphPlugin::s_selectAllShortcutName),
            this
        );
        m_actions[SELECTION_SELECTALL]->setShortcut(Qt::Key_A | Qt::ControlModifier);
        shortcutManager->RegisterKeyboardShortcut(m_actions[SELECTION_SELECTALL], AnimGraphPlugin::s_animGraphWindowShortcutGroupName, true);
        connect(m_actions[SELECTION_SELECTALL], &QAction::triggered, [this]
        {
            NodeGraph* activeGraph = m_parentPlugin->GetGraphWidget()->GetActiveGraph();
            if (activeGraph)
            {
                activeGraph->SelectAllNodes();
            }
        });

        m_actions[SELECTION_UNSELECTALL] = new QAction(
            FromStdString(AnimGraphPlugin::s_unselectAllShortcutName),
            this
        );
        m_actions[SELECTION_UNSELECTALL]->setShortcut(Qt::Key_D | Qt::ControlModifier);
        shortcutManager->RegisterKeyboardShortcut(m_actions[SELECTION_UNSELECTALL], AnimGraphPlugin::s_animGraphWindowShortcutGroupName, true);
        connect(m_actions[SELECTION_UNSELECTALL], &QAction::triggered, [this]
        {
            NodeGraph* activeGraph = m_parentPlugin->GetGraphWidget()->GetActiveGraph();
            if (activeGraph)
            {
                activeGraph->UnselectAllNodes();
            }
        });

        m_actions[FILE_NEW] = new QAction(
            QIcon(":/EMotionFX/Plus.svg"),
            tr("Create a new anim graph"),
            this);
        m_actions[FILE_NEW]->setObjectName("EMFX.BlendGraphViewWidget.NewButton");
        connect(m_actions[FILE_NEW], &QAction::triggered, this, &BlendGraphViewWidget::OnCreateAnimGraph);

        m_actions[FILE_OPEN] = new QAction(
            tr("Open..."),
            this);
        connect(m_actions[FILE_OPEN], &QAction::triggered, m_parentPlugin, &AnimGraphPlugin::OnFileOpen);

        m_actions[FILE_SAVE] = new QAction(
            tr("Save"),
            this);
        connect(m_actions[FILE_SAVE], &QAction::triggered, m_parentPlugin, &AnimGraphPlugin::OnFileSave);

        m_actions[FILE_SAVEAS] = new QAction(
            tr("Save as..."),
            this);
        connect(m_actions[FILE_SAVEAS], &QAction::triggered, m_parentPlugin, &AnimGraphPlugin::OnFileSaveAs);

        m_actions[NAVIGATION_FORWARD] = new QAction(
            QIcon(":/EMotionFX/Forward.svg"),
            FromStdString(AnimGraphPlugin::s_historyForwardShortcutName),
            this);
        m_actions[NAVIGATION_FORWARD]->setShortcut(Qt::Key_Right);
        shortcutManager->RegisterKeyboardShortcut(m_actions[NAVIGATION_FORWARD], AnimGraphPlugin::s_animGraphWindowShortcutGroupName, true);
        connect(m_actions[NAVIGATION_FORWARD], &QAction::triggered, [this]
        {
            m_parentPlugin->GetNavigationHistory()->StepForward();
            UpdateNavigation();
        });

        m_actions[NAVIGATION_BACK] = new QAction(
            QIcon(":/EMotionFX/Backward.svg"),
            FromStdString(AnimGraphPlugin::s_historyBackShortcutName),
            this);
        m_actions[NAVIGATION_BACK]->setShortcut(Qt::Key_Left);
        shortcutManager->RegisterKeyboardShortcut(m_actions[NAVIGATION_BACK], AnimGraphPlugin::s_animGraphWindowShortcutGroupName, true);
        connect(m_actions[NAVIGATION_BACK], &QAction::triggered, [this]
        {
            m_parentPlugin->GetNavigationHistory()->StepBackward();
            UpdateNavigation();
        });

        m_actions[NAVIGATION_NAVPANETOGGLE] = new QAction(
            QIcon(":/EMotionFX/List.svg"),
            tr("Show/hide navigation pane"),
            this);
        m_actions[NAVIGATION_NAVPANETOGGLE]->setCheckable(true);
        connect(m_actions[NAVIGATION_NAVPANETOGGLE], &QAction::triggered, this, &BlendGraphViewWidget::ToggleNavigationPane);

        m_actions[NAVIGATION_OPEN_SELECTED] = new QAction(
            FromStdString(AnimGraphPlugin::s_openSelectedNodeShortcutName),
            this);
        m_actions[NAVIGATION_OPEN_SELECTED]->setShortcut(Qt::Key_Down);
        shortcutManager->RegisterKeyboardShortcut(m_actions[NAVIGATION_OPEN_SELECTED], AnimGraphPlugin::s_animGraphWindowShortcutGroupName, true);
        connect(m_actions[NAVIGATION_OPEN_SELECTED], &QAction::triggered, &m_parentPlugin->GetActionManager(), &AnimGraphActionManager::NavigateToNode);

        m_actions[NAVIGATION_TO_PARENT] = new QAction(
            FromStdString(AnimGraphPlugin::s_openParentNodeShortcutName),
            this);
        m_actions[NAVIGATION_TO_PARENT]->setShortcut(Qt::Key_Up);
        shortcutManager->RegisterKeyboardShortcut(m_actions[NAVIGATION_TO_PARENT], AnimGraphPlugin::s_animGraphWindowShortcutGroupName, true);
        connect(m_actions[NAVIGATION_TO_PARENT], &QAction::triggered, &m_parentPlugin->GetActionManager(), &AnimGraphActionManager::NavigateToParent);

        m_actions[NAVIGATION_FRAME_ALL] = new QAction(
            QIcon(":/EMotionFX/ZoomSelected.svg"),
            FromStdString(AnimGraphPlugin::s_fitEntireGraphShortcutName),
            this);
        m_actions[NAVIGATION_FRAME_ALL]->setShortcut(Qt::Key_A);
        shortcutManager->RegisterKeyboardShortcut(m_actions[NAVIGATION_FRAME_ALL], AnimGraphPlugin::s_animGraphWindowShortcutGroupName, true);
        connect(m_actions[NAVIGATION_FRAME_ALL], &QAction::triggered, this, &BlendGraphViewWidget::ZoomToAll);

        m_actions[NAVIGATION_ZOOMSELECTION] = new QAction(
            QIcon(":/EMotionFX/ZoomSelected.svg"),
            FromStdString(AnimGraphPlugin::s_zoomOnSelectedNodesShortcutName),
            this);
        m_actions[NAVIGATION_ZOOMSELECTION]->setShortcut(Qt::Key_Z);
        shortcutManager->RegisterKeyboardShortcut(m_actions[NAVIGATION_ZOOMSELECTION], AnimGraphPlugin::s_animGraphWindowShortcutGroupName, true);
        connect(m_actions[NAVIGATION_ZOOMSELECTION], &QAction::triggered, this, &BlendGraphViewWidget::ZoomSelected);

        m_actions[ACTIVATE_ANIMGRAPH] = new QAction(
            QIcon(":/EMotionFX/PlayForward.svg"),
            tr("Activate Animgraph/State"),
            this);
        connect(m_actions[ACTIVATE_ANIMGRAPH], &QAction::triggered, &m_parentPlugin->GetActionManager(), &AnimGraphActionManager::ActivateAnimGraph);

        m_actions[VISUALIZATION_PLAYSPEEDS] = new QAction(
            tr("Display Play Speeds"),
            this);
        m_actions[VISUALIZATION_PLAYSPEEDS]->setCheckable(true);
        connect(m_actions[VISUALIZATION_PLAYSPEEDS], &QAction::triggered, this, &BlendGraphViewWidget::OnDisplayPlaySpeeds);

        m_actions[VISUALIZATION_GLOBALWEIGHTS] = new QAction(
            tr("Display Global Weights"),
            this);
        m_actions[VISUALIZATION_GLOBALWEIGHTS]->setCheckable(true);
        connect(m_actions[VISUALIZATION_GLOBALWEIGHTS], &QAction::triggered, this, &BlendGraphViewWidget::OnDisplayGlobalWeights);

        m_actions[VISUALIZATION_SYNCSTATUS] = new QAction(
            tr("Display Sync Status"),
            this);
        m_actions[VISUALIZATION_SYNCSTATUS]->setCheckable(true);
        connect(m_actions[VISUALIZATION_SYNCSTATUS], &QAction::triggered, this, &BlendGraphViewWidget::OnDisplaySyncStatus);

        m_actions[VISUALIZATION_PLAYPOSITIONS] = new QAction(
            tr("Display Play Positions"),
            this);
        m_actions[VISUALIZATION_PLAYPOSITIONS]->setCheckable(true);
        connect(m_actions[VISUALIZATION_PLAYPOSITIONS], &QAction::triggered, this, &BlendGraphViewWidget::OnDisplayPlayPositions);

#if defined(EMFX_ANIMGRAPH_PROFILER_ENABLED)
        m_actions[VISUALIZATION_PROFILING_NONE] = new QAction(tr("None"), this);
        m_actions[VISUALIZATION_PROFILING_NONE]->setCheckable(false);
        connect(m_actions[VISUALIZATION_PROFILING_NONE], &QAction::triggered, this, [this]{ OnDisplayAllProfiling(false); });
        
        m_actions[VISUALIZATION_PROFILING_ALL] = new QAction(tr("All"), this);
        m_actions[VISUALIZATION_PROFILING_ALL]->setCheckable(false);
        connect(m_actions[VISUALIZATION_PROFILING_ALL], &QAction::triggered, this, [this]{ OnDisplayAllProfiling(true); });

        AddProfilingAction("Update", VISUALIZATION_PROFILING_UPDATE);
        AddProfilingAction("TopDownUpdate", VISUALIZATION_PROFILING_TOPDOWN);
        AddProfilingAction("PostUpdate", VISUALIZATION_PROFILING_POSTUPDATE);
        AddProfilingAction("Output", VISUALIZATION_PROFILING_OUTPUT);
#endif

        m_actions[EDIT_CUT] = new QAction(
            FromStdString(AnimGraphPlugin::s_cutShortcutName),
            this
        );
        m_actions[EDIT_CUT]->setShortcut(Qt::Key_X | Qt::ControlModifier);
        shortcutManager->RegisterKeyboardShortcut(m_actions[EDIT_CUT], AnimGraphPlugin::s_animGraphWindowShortcutGroupName, true);
        connect(m_actions[EDIT_CUT], &QAction::triggered, this, [this]
        {
            m_parentPlugin->GetActionManager().Cut();
        });

        m_actions[EDIT_COPY] = new QAction(
            FromStdString(AnimGraphPlugin::s_copyShortcutName),
            this
        );
        m_actions[EDIT_COPY]->setShortcut(Qt::Key_C | Qt::ControlModifier);
        shortcutManager->RegisterKeyboardShortcut(m_actions[EDIT_COPY], AnimGraphPlugin::s_animGraphWindowShortcutGroupName, true);
        connect(m_actions[EDIT_COPY], &QAction::triggered, this, [this]
        {
            m_parentPlugin->GetActionManager().Copy();
        });

        m_actions[EDIT_PASTE] = new QAction(
            FromStdString(AnimGraphPlugin::s_pasteShortcutName),
            this
        );
        m_actions[EDIT_PASTE]->setShortcut(Qt::Key_V | Qt::ControlModifier);
        shortcutManager->RegisterKeyboardShortcut(m_actions[EDIT_PASTE], AnimGraphPlugin::s_animGraphWindowShortcutGroupName, true);
        connect(m_actions[EDIT_PASTE], &QAction::triggered, this, [this]
        {
            const BlendGraphWidget* graphWidget = m_parentPlugin->GetGraphWidget();
            const NodeGraph* activeGraph = graphWidget->GetActiveGraph();
            if (!activeGraph)
            {
                return;
            }
            const QPoint pastePosition = graphWidget->underMouse()
                ? graphWidget->SnapLocalToGrid(graphWidget->LocalToGlobal(graphWidget->mapFromGlobal(QCursor::pos())))
                : graphWidget->SnapLocalToGrid(graphWidget->LocalToGlobal(graphWidget->rect().center()));
            m_parentPlugin->GetActionManager().Paste(activeGraph->GetModelIndex(), pastePosition);
        });

        m_actions[EDIT_DELETE] = new QAction(
            FromStdString(AnimGraphPlugin::s_deleteSelectedNodesShortcutName),
            this
        );
        m_actions[EDIT_DELETE]->setShortcut(Qt::Key_Delete);
        shortcutManager->RegisterKeyboardShortcut(m_actions[EDIT_DELETE], AnimGraphPlugin::s_animGraphWindowShortcutGroupName, true);
        connect(m_actions[EDIT_DELETE], &QAction::triggered, this, [this]
        {
            m_parentPlugin->GetGraphWidget()->DeleteSelectedItems();
        });

        for (QAction* action : m_actions)
        {
            action->setShortcutContext(Qt::WidgetShortcut);
            addAction(action);
        }

        GetMainWindow()->LoadKeyboardShortcuts();
    }

    QToolBar* BlendGraphViewWidget::CreateTopToolBar()
    {
        QToolBar* toolBar = new QToolBar(this);
        toolBar->setObjectName("EMFX.BlendGraphViewWidget.TopToolBar");

        toolBar->addAction(m_actions[FILE_NEW]);

        // Open anim graph
        {
            m_openMenu = new QMenu(this);
            connect(m_openMenu, &QMenu::aboutToShow, this, &BlendGraphViewWidget::BuildOpenMenu);

            QAction* action = new QAction(
                QIcon(":/EMotionFX/Open.svg"),
                tr("Open"),
                this);
            action->setMenu(m_openMenu);

            QToolButton* button = new QToolButton();
            button->setDefaultAction(action);
            button->setPopupMode(QToolButton::InstantPopup);

            toolBar->addWidget(button);
        }


        // Save anim graph
        {
            QMenu* contextMenu = new QMenu(toolBar);
            contextMenu->addAction(m_actions[FILE_SAVE]);
            contextMenu->addAction(m_actions[FILE_SAVEAS]);

            QAction* saveMenuAction = new QAction(
                QIcon(":/EMotionFX/Save.svg"),
                tr("Save anim graph"),
                this);
            saveMenuAction->setMenu(contextMenu);

            QToolButton* button = new QToolButton();
            button->setDefaultAction(saveMenuAction);
            button->setPopupMode(QToolButton::InstantPopup);

            toolBar->addWidget(button);
        }

        toolBar->addSeparator();

        toolBar->addAction(m_actions[ACTIVATE_ANIMGRAPH]);

        toolBar->addSeparator();

        toolBar->addAction(m_actions[NAVIGATION_ZOOMSELECTION]);

        // Visualization options
        {
            QAction* menuAction = toolBar->addAction(
                QIcon(":/EMotionFX/Visualization.svg"),
                tr("Visualization"));

            QToolButton* toolButton = qobject_cast<QToolButton*>(toolBar->widgetForAction(menuAction));
            AZ_Assert(toolButton, "The action widget must be a tool button.");
            toolButton->setPopupMode(QToolButton::InstantPopup);

            QMenu* contextMenu = new QMenu(toolBar);

            contextMenu->addAction(m_actions[VISUALIZATION_PLAYSPEEDS]);
            contextMenu->addAction(m_actions[VISUALIZATION_GLOBALWEIGHTS]);
            contextMenu->addAction(m_actions[VISUALIZATION_SYNCSTATUS]);
            contextMenu->addAction(m_actions[VISUALIZATION_PLAYPOSITIONS]);

            menuAction->setMenu(contextMenu);

#if defined(EMFX_ANIMGRAPH_PROFILER_ENABLED)
            // Profiler options
            {
                QMenu* profilerMenu = new QMenu("Profiler", toolBar);
                profilerMenu->addAction(m_actions[VISUALIZATION_PROFILING_NONE]);
                profilerMenu->addAction(m_actions[VISUALIZATION_PROFILING_UPDATE]);
                profilerMenu->addAction(m_actions[VISUALIZATION_PROFILING_TOPDOWN]);
                profilerMenu->addAction(m_actions[VISUALIZATION_PROFILING_POSTUPDATE]);
                profilerMenu->addAction(m_actions[VISUALIZATION_PROFILING_OUTPUT]);
                profilerMenu->addAction(m_actions[VISUALIZATION_PROFILING_ALL]);
                contextMenu->addMenu(profilerMenu);
            }
#endif
        }

        toolBar->addSeparator();

        // Alignment Options
        toolBar->addAction(m_actions[SELECTION_ALIGNLEFT]);
        toolBar->addAction(m_actions[SELECTION_ALIGNRIGHT]);
        toolBar->addAction(m_actions[SELECTION_ALIGNTOP]);
        toolBar->addAction(m_actions[SELECTION_ALIGNBOTTOM]);

        return toolBar;
    }

    QToolBar* BlendGraphViewWidget::CreateNavigationToolBar()
    {
        QToolBar* toolBar = new QToolBar(this);

        toolBar->addAction(m_actions[NAVIGATION_BACK]);

        toolBar->addAction(m_actions[NAVIGATION_FORWARD]);

        m_navigationLink = new NavigationLinkWidget(m_parentPlugin, this);
        m_navigationLink->setMinimumHeight(28);
        toolBar->addWidget(m_navigationLink);

        toolBar->addAction(m_actions[NAVIGATION_NAVPANETOGGLE]);

        return toolBar;
    }

    void BlendGraphViewWidget::Init(BlendGraphWidget* blendGraphWidget)
    {
        connect(&m_parentPlugin->GetAnimGraphModel(), &AnimGraphModel::FocusChanged, this, &BlendGraphViewWidget::OnFocusChanged);
        connect(&m_parentPlugin->GetAnimGraphModel().GetSelectionModel(), &QItemSelectionModel::selectionChanged, this, &BlendGraphViewWidget::UpdateEnabledActions);
        connect(m_parentPlugin->GetNavigationHistory(), &NavigationHistory::ChangedSteppingLimits, this, &BlendGraphViewWidget::UpdateNavigation);
        connect(m_parentPlugin->GetGraphWidget(), &NodeGraphWidget::ActiveGraphChanged, this, &BlendGraphViewWidget::UpdateEnabledActions);
        connect(m_parentPlugin, &AnimGraphPlugin::ActionFilterChanged, this, &BlendGraphViewWidget::UpdateEnabledActions);
        connect(&m_parentPlugin->GetActionManager(), &AnimGraphActionManager::PasteStateChanged, this, &BlendGraphViewWidget::UpdateEnabledActions);

        // create the vertical layout with the menu and the graph widget as entries
        QVBoxLayout* verticalLayout = new QVBoxLayout(this);
        verticalLayout->setSizeConstraint(QLayout::SetNoConstraint);
        verticalLayout->setSpacing(0);
        verticalLayout->setMargin(2);

        // Create toolbars
        CreateActions();
        verticalLayout->addWidget(CreateTopToolBar());
        verticalLayout->addWidget(CreateNavigationToolBar());

        /////////////////////////////////////////////////////////////////////////////////////
        // Anim graph viewport
        /////////////////////////////////////////////////////////////////////////////////////

        verticalLayout->addWidget(&m_viewportStack);

        m_viewportSplitter = new QSplitter(Qt::Horizontal, this);
        m_viewportSplitter->addWidget(blendGraphWidget);
        m_viewportSplitter->addWidget(m_parentPlugin->GetNavigateWidget());
        m_viewportSplitter->setCollapsible(0, false);
        QList<int> sizes = { this->width(), 0 };
        m_viewportSplitter->setSizes(sizes);
        m_viewportStack.addWidget(m_viewportSplitter);
        connect(m_viewportSplitter, &QSplitter::splitterMoved,
                this,
                [=]{m_actions[NAVIGATION_NAVPANETOGGLE]->setChecked(m_viewportSplitter->sizes().at(1) > 0 );});

        UpdateNavigation();
        UpdateAnimGraphOptions();
        UpdateEnabledActions();

        // Register this window as the widget for the Animation Editor Action Context.
        if (auto hotKeyManagerInterface = AZ::Interface<AzToolsFramework::HotKeyManagerInterface>::Get())
        {
            hotKeyManagerInterface->AssignWidgetToActionContext(AnimationEditorAnimGraphActionContextIdentifier, this);
        }
    }

    BlendGraphViewWidget::~BlendGraphViewWidget()
    {
        // Unregister this window as the widget for the Animation Editor Action Context.
        if (auto hotKeyManagerInterface = AZ::Interface<AzToolsFramework::HotKeyManagerInterface>::Get())
        {
            hotKeyManagerInterface->RemoveWidgetFromActionContext(AnimationEditorAnimGraphActionContextIdentifier, this);
        }

        for (auto entry : m_nodeTypeToWidgetMap)
        {
            AnimGraphNodeWidget* widget = entry.second;
            delete widget;
        }
    }

    void BlendGraphViewWidget::UpdateNavigation()
    {
        NavigationHistory* navigationHistory = m_parentPlugin->GetNavigationHistory();
        m_actions[NAVIGATION_BACK]->setEnabled(navigationHistory->CanStepBackward());
        m_actions[NAVIGATION_FORWARD]->setEnabled(navigationHistory->CanStepForward());
    }

    void BlendGraphViewWidget::UpdateAnimGraphOptions()
    {
        // get the anim graph that is currently selected in the resource widget
        EMotionFX::AnimGraph* animGraph = m_parentPlugin->GetActiveAnimGraph();
        if (animGraph == nullptr)
        {
            SetOptionEnabled(FILE_SAVE, false);
            SetOptionEnabled(FILE_SAVEAS, false);
        }
        else
        {
            SetOptionEnabled(FILE_SAVE, true);
            SetOptionEnabled(FILE_SAVEAS, true);
        }
    }

    void BlendGraphViewWidget::UpdateEnabledActions()
    {
        // do we have any selection?
        const bool anySelection = m_parentPlugin->GetAnimGraphModel().GetSelectionModel().hasSelection();
        SetOptionEnabled(NAVIGATION_ZOOMSELECTION, anySelection);

        const auto isNodeSelected = [](const QModelIndex& index)
        {
            return index.isValid()
                && index.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>() == AnimGraphModel::ModelItemType::NODE;
        };
        const QModelIndexList selectedIndexes = m_parentPlugin->GetAnimGraphModel().GetSelectionModel().selectedRows();
        const auto firstSelectedNode = AZStd::find_if(selectedIndexes.begin(), selectedIndexes.end(), isNodeSelected);
        const auto secondSelectedNode = AZStd::find_if(firstSelectedNode, selectedIndexes.end(), isNodeSelected);
        const bool atLeastTwoNodes = secondSelectedNode != selectedIndexes.end();

        const bool enableAlignActions = m_parentPlugin->GetActionFilter().m_editNodes && atLeastTwoNodes;
        SetOptionEnabled(SELECTION_ALIGNLEFT, enableAlignActions);
        SetOptionEnabled(SELECTION_ALIGNRIGHT, enableAlignActions);
        SetOptionEnabled(SELECTION_ALIGNTOP, enableAlignActions);
        SetOptionEnabled(SELECTION_ALIGNBOTTOM, enableAlignActions);

        const bool isEditable = m_parentPlugin->GetGraphWidget()->GetActiveGraph() && !m_parentPlugin->GetGraphWidget()->GetActiveGraph()->IsInReferencedGraph();
        const AnimGraphActionFilter& actionFilter = m_parentPlugin->GetActionFilter();

        SetOptionEnabled(EDIT_CUT, actionFilter.m_copyAndPaste && anySelection && isEditable);
        SetOptionEnabled(EDIT_COPY, actionFilter.m_copyAndPaste && anySelection);
        SetOptionEnabled(EDIT_PASTE, actionFilter.m_copyAndPaste && isEditable && m_parentPlugin->GetActionManager().GetIsReadyForPaste());
        SetOptionEnabled(EDIT_DELETE, actionFilter.m_copyAndPaste && anySelection && isEditable);
    }

    AnimGraphNodeWidget* BlendGraphViewWidget::GetWidgetForNode(const EMotionFX::AnimGraphNode* node)
    {
        if (!node)
        {
            return nullptr;
        }

        const AZ::TypeId nodeType = azrtti_typeid(node);
        AnimGraphNodeWidget* widget = nullptr;

        AZStd::unordered_map<AZ::TypeId, AnimGraphNodeWidget*>::iterator it =
            m_nodeTypeToWidgetMap.find(nodeType);
        if (it != m_nodeTypeToWidgetMap.end())
        {
            widget = it->second;
        }
        else
        {
            if (nodeType == azrtti_typeid<EMotionFX::BlendSpace2DNode>())
            {
                widget = new BlendSpace2DNodeWidget(m_parentPlugin);
                m_nodeTypeToWidgetMap[nodeType] = widget;
            }
            else if (nodeType == azrtti_typeid<EMotionFX::BlendSpace1DNode>())
            {
                widget = new BlendSpace1DNodeWidget(m_parentPlugin);
                m_nodeTypeToWidgetMap[nodeType] = widget;
            }
        }
        return widget;
    }

    void BlendGraphViewWidget::OnFocusChanged(const QModelIndex& newFocusIndex, const QModelIndex& newFocusParent, const QModelIndex& oldFocusIndex, const QModelIndex& oldFocusParent)
    {
        AZ_UNUSED(newFocusIndex);
        AZ_UNUSED(oldFocusIndex);

        if (newFocusParent == oldFocusParent && newFocusParent.isValid())
        {
            // Not interested if the parent didn't change, and the parent is still a valid model index.
            return;
        }

        // Reset the viewports to avoid dangling pointers.
        for (auto& item : m_nodeTypeToWidgetMap)
        {
            AnimGraphNodeWidget* viewport = item.second;
            viewport->SetCurrentNode(nullptr);
        }

        if (newFocusParent.isValid())
        {
            EMotionFX::AnimGraphNode* node = newFocusParent.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
            AnimGraphNodeWidget* widget = GetWidgetForNode(node);
            if (widget)
            {
                int index = m_viewportStack.indexOf(widget);
                if (index == -1)
                {
                    m_viewportStack.addWidget(widget);
                    m_viewportStack.setCurrentIndex(m_viewportStack.count() - 1);
                }
                else
                {
                    m_viewportStack.setCurrentIndex(index);
                }

                widget->SetCurrentNode(node);
                widget->SetCurrentModelIndex(newFocusParent);
            }
            else
            {
                // Show the default widget.
                m_viewportStack.setCurrentIndex(0);
            }
        }
    }

#if defined(EMFX_ANIMGRAPH_PROFILER_ENABLED)
    void BlendGraphViewWidget::AddProfilingAction(const char* actionName, EOptionFlag optionFlag)
    {
        m_actions[optionFlag] = new QAction(
            tr(actionName),
            this);
        m_actions[optionFlag]->setCheckable(true);
        connect(m_actions[optionFlag], &QAction::triggered, this, [this, optionFlag]{ OnDisplayProfiling(optionFlag); });
    }

    void BlendGraphViewWidget::OnDisplayProfiling(EOptionFlag profileOption)
    {
        bool show = GetOptionFlag(profileOption);

        uint32 displayFlags = m_parentPlugin->GetDisplayFlags();
        switch (profileOption)
        {
        case VISUALIZATION_PROFILING_UPDATE:
            displayFlags = AnimGraphPlugin::DISPLAYFLAG_PROFILING_UPDATE;
            break;
        case VISUALIZATION_PROFILING_TOPDOWN:
            displayFlags = AnimGraphPlugin::DISPLAYFLAG_PROFILING_TOPDOWN;
            break;
        case VISUALIZATION_PROFILING_POSTUPDATE:
            displayFlags = AnimGraphPlugin::DISPLAYFLAG_PROFILING_POSTUPDATE;
            break;
        case VISUALIZATION_PROFILING_OUTPUT:
            displayFlags = AnimGraphPlugin::DISPLAYFLAG_PROFILING_OUTPUT;
            break;
        default:
            AZ_Error("EMotionFX", true,
                     "Undefined profile option flags.");
        };
        m_parentPlugin->SetDisplayFlagEnabled(displayFlags, show);
    }

    void BlendGraphViewWidget::OnDisplayAllProfiling(bool enabledAll)
    {
        m_parentPlugin->SetDisplayFlagEnabled(AnimGraphPlugin::DISPLAYFLAG_PROFILING_UPDATE, enabledAll);
        m_parentPlugin->SetDisplayFlagEnabled(AnimGraphPlugin::DISPLAYFLAG_PROFILING_TOPDOWN, enabledAll);
        m_parentPlugin->SetDisplayFlagEnabled(AnimGraphPlugin::DISPLAYFLAG_PROFILING_POSTUPDATE, enabledAll);
        m_parentPlugin->SetDisplayFlagEnabled(AnimGraphPlugin::DISPLAYFLAG_PROFILING_OUTPUT, enabledAll);
        SetOptionFlag(VISUALIZATION_PROFILING_UPDATE, enabledAll);
        SetOptionFlag(VISUALIZATION_PROFILING_TOPDOWN, enabledAll);
        SetOptionFlag(VISUALIZATION_PROFILING_POSTUPDATE, enabledAll);
        SetOptionFlag(VISUALIZATION_PROFILING_OUTPUT, enabledAll);
    }
#endif

    void BlendGraphViewWidget::SetOptionFlag(EOptionFlag option, bool isEnabled)
    {
        if (m_actions[option])
        {
            m_actions[option]->setChecked(isEnabled);
        }
    }

    void BlendGraphViewWidget::SetOptionEnabled(EOptionFlag option, bool isEnabled)
    {
        if (m_actions[option])
        {
            m_actions[option]->setEnabled(isEnabled);
        }
    }

    void BlendGraphViewWidget::BuildOpenMenu()
    {
        m_openMenu->clear();

        m_openMenu->addAction(m_actions[FILE_OPEN]);

        const size_t numAnimGraphs = EMotionFX::GetAnimGraphManager().GetNumAnimGraphs();
        if (numAnimGraphs > 0)
        {
            m_openMenu->addSeparator();
            for (size_t i = 0; i < numAnimGraphs; ++i)
            {
                EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(i);
                if (animGraph->GetIsOwnedByRuntime() == false)
                {
                    QString itemName = "<Unsaved Animgraph>";
                    if (!animGraph->GetFileNameString().empty())
                    {
                        // convert full absolute paths to friendlier relative paths + folder they're found in.
                        // GetSourceInfoBySourcePath works on relative paths and absolute paths and doesn't need to wait for
                        // cached products to exist in order to function, so it is orders of magnitude faster than asking about product files.
                        itemName = QString::fromUtf8(animGraph->GetFileName());
                        bool success = false;
                        AZStd::string watchFolder;
                        AZ::Data::AssetInfo assetInfo;
                        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(success, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, animGraph->GetFileName(), assetInfo, watchFolder);
                        if (success)
                        {
                            itemName = QString::fromUtf8(assetInfo.m_relativePath.c_str());
                        }
                    }
                    QAction* openItem = m_openMenu->addAction(itemName);
                    connect(openItem, &QAction::triggered, [this, animGraph]() { OpenAnimGraph(animGraph); });
                    openItem->setData(animGraph->GetID());
                }
            }
        }
    }

    void BlendGraphViewWidget::OpenAnimGraph(EMotionFX::AnimGraph* animGraph)
    {
        if (animGraph)
        {
            EMotionFX::MotionSet* motionSet = nullptr;
            EMotionFX::AnimGraphEditorRequestBus::BroadcastResult(motionSet, &EMotionFX::AnimGraphEditorRequests::GetSelectedMotionSet);
            m_parentPlugin->GetActionManager().ActivateGraphForSelectedActors(animGraph, motionSet);
        }
    }

    void BlendGraphViewWidget::OnCreateAnimGraph()
    {
        // get the current selection list and the number of actor instances selected
        const CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
        const size_t numActorInstances = selectionList.GetNumSelectedActorInstances();

        // Activate the new anim graph automatically (The shown anim graph should always be the activated one).
        if (numActorInstances > 0)
        {
            // create the command group
            MCore::CommandGroup commandGroup("Create an anim graph");

            // add the create anim graph command
            commandGroup.AddCommandString("CreateAnimGraph");

            // get the correct motion set
            // nullptr can only be <no motion set> because it's the first anim graph so no one is activated
            // if no motion set selected but one is possible, use the first possible
            // if no motion set selected and no one created, use no motion set
            // if one already selected, use the already selected
            EMotionFX::MotionSet* motionSet = nullptr;
            EMotionFX::AnimGraphEditorRequestBus::BroadcastResult(motionSet, &EMotionFX::AnimGraphEditorRequests::GetSelectedMotionSet);
            if (!motionSet)
            {
                if (EMotionFX::GetMotionManager().GetNumMotionSets() > 0)
                {
                    motionSet = EMotionFX::GetMotionManager().GetMotionSet(0);
                }
            }

            if (motionSet)
            {
                // Activate anim graph on all actor instances in case there is a motion set.
                for (size_t i = 0; i < numActorInstances; ++i)
                {
                    EMotionFX::ActorInstance* actorInstance = selectionList.GetActorInstance(i);
                    commandGroup.AddCommandString(AZStd::string::format("ActivateAnimGraph -actorInstanceID %d -animGraphID %%LASTRESULT%% -motionSetID %d", actorInstance->GetID(), motionSet->GetID()));
                }
            }

            AZStd::string result;
            if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }
        }
        else
        {
            AZStd::string result;
            if (!EMStudio::GetCommandManager()->ExecuteCommand("CreateAnimGraph", result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }
        }
    }

    void BlendGraphViewWidget::ZoomSelected()
    {
        BlendGraphWidget* blendGraphWidget = m_parentPlugin->GetGraphWidget();
        if (blendGraphWidget)
        {
            NodeGraph* nodeGraph = blendGraphWidget->GetActiveGraph();
            if (nodeGraph)
            {
                // Try zooming on the selection rect
                const QRect selectionRect = nodeGraph->CalcRectFromSelection(true);
                if (!selectionRect.isEmpty())
                {
                    // Always use the blend graph widget size in the viewport splitter, so it will center correctly if navigateWidget is open.
                    QList<int> sizes = m_viewportSplitter->sizes();
                    nodeGraph->ZoomOnRect(selectionRect, sizes[0], blendGraphWidget->geometry().height(), true);
                }
                else // Zoom on the full scene
                {
                    nodeGraph->FitGraphOnScreen(blendGraphWidget->geometry().width(), blendGraphWidget->geometry().height(), blendGraphWidget->GetMousePos(), true);
                }
            }
        }
    }

    void BlendGraphViewWidget::ZoomToAll()
    {
        BlendGraphWidget* blendGraphWidget = m_parentPlugin->GetGraphWidget();
        if (blendGraphWidget)
        {
            NodeGraph* nodeGraph = blendGraphWidget->GetActiveGraph();
            if (nodeGraph)
            {
                nodeGraph->FitGraphOnScreen(geometry().width(), geometry().height(), blendGraphWidget->GetMousePos());
            }
        }
    }

    void BlendGraphViewWidget::OnActivateState()
    {
        // Transition to the selected state.
        const QModelIndexList currentModelIndexes = m_parentPlugin->GetAnimGraphModel().GetSelectionModel().selectedRows();
        if (!currentModelIndexes.empty())
        {
            const QModelIndex currentModelIndex = currentModelIndexes.front();
            const AnimGraphModel::ModelItemType itemType = currentModelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>();
            if (itemType == AnimGraphModel::ModelItemType::NODE)
            {
                EMotionFX::AnimGraphNode* selectedNode = currentModelIndex.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
                EMotionFX::AnimGraphNode* parentNode = selectedNode->GetParentNode();
                if (parentNode && azrtti_typeid(parentNode) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
                {
                    EMotionFX::AnimGraphStateMachine* stateMachine = static_cast<EMotionFX::AnimGraphStateMachine*>(parentNode);
                    EMotionFX::AnimGraphInstance* animGraphInstance = currentModelIndex.data(AnimGraphModel::ROLE_ANIM_GRAPH_INSTANCE).value<EMotionFX::AnimGraphInstance*>();
                    if (animGraphInstance)
                    {
                        stateMachine->TransitionToState(animGraphInstance, selectedNode);
                    }
                }
            }
        }
    }

    void BlendGraphViewWidget::NavigateToRoot()
    {
        const QModelIndex nodeModelIndex = m_parentPlugin->GetGraphWidget()->GetActiveGraph()->GetModelIndex();
        if (nodeModelIndex.isValid())
        {
            m_parentPlugin->GetAnimGraphModel().Focus(nodeModelIndex);
        }
    }

    void BlendGraphViewWidget::ToggleNavigationPane()
    {
        QList<int> sizes = m_viewportSplitter->sizes();
        if (sizes[1] == 0)
        {
            // the nav pane is hidden if the width is 0, so set the width to 25%
            sizes[0] = (this->width() * 75) / 100;
            sizes[1] = this->width() - sizes[0];
        }
        else
        {
            // hide the nav pane
            sizes[0] = this->width();
            sizes[1] = 0;
        }
        m_viewportSplitter->setSizes(sizes);
        m_actions[NAVIGATION_NAVPANETOGGLE]->setChecked(m_viewportSplitter->sizes().at(1) > 0 );
    }

    // toggle playspeed viz
    void BlendGraphViewWidget::OnDisplayPlaySpeeds()
    {
        const bool showPlaySpeeds = !m_parentPlugin->GetIsDisplayFlagEnabled(AnimGraphPlugin::DISPLAYFLAG_PLAYSPEED);
        m_parentPlugin->SetDisplayFlagEnabled(AnimGraphPlugin::DISPLAYFLAG_PLAYSPEED, showPlaySpeeds);
        SetOptionFlag(VISUALIZATION_PLAYSPEEDS, showPlaySpeeds);
    }


    // toggle sync status viz
    void BlendGraphViewWidget::OnDisplaySyncStatus()
    {
        const bool show = !m_parentPlugin->GetIsDisplayFlagEnabled(AnimGraphPlugin::DISPLAYFLAG_SYNCSTATUS);
        m_parentPlugin->SetDisplayFlagEnabled(AnimGraphPlugin::DISPLAYFLAG_SYNCSTATUS, show);
        SetOptionFlag(VISUALIZATION_SYNCSTATUS, show);
    }


    // toggle global weights viz
    void BlendGraphViewWidget::OnDisplayGlobalWeights()
    {
        const bool displayGlobalWeights = !m_parentPlugin->GetIsDisplayFlagEnabled(AnimGraphPlugin::DISPLAYFLAG_GLOBALWEIGHT);
        m_parentPlugin->SetDisplayFlagEnabled(AnimGraphPlugin::DISPLAYFLAG_GLOBALWEIGHT, displayGlobalWeights);
        SetOptionFlag(VISUALIZATION_GLOBALWEIGHTS, displayGlobalWeights);
    }


    // toggle play position viz
    void BlendGraphViewWidget::OnDisplayPlayPositions()
    {
        const bool show = !m_parentPlugin->GetIsDisplayFlagEnabled(AnimGraphPlugin::DISPLAYFLAG_PLAYPOSITION);
        m_parentPlugin->SetDisplayFlagEnabled(AnimGraphPlugin::DISPLAYFLAG_PLAYPOSITION, show);
        SetOptionFlag(VISUALIZATION_PLAYPOSITIONS, show);
    }

    void BlendGraphViewWidget::showEvent([[maybe_unused]] QShowEvent* showEvent)
    {
        EMotionFX::AnimGraphEditorNotificationBus::Broadcast(&EMotionFX::AnimGraphEditorNotificationBus::Events::OnShow);
    }

} // namespace EMStudio
