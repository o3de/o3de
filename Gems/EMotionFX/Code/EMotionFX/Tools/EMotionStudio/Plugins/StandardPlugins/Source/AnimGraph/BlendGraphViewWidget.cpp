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
#include <Editor/AnimGraphEditorBus.h>
#include <QKeyEvent>
#include <QPushButton>
#include <QShowEvent>
#include <QSplitter>
#include <QToolBar>
#include <QVBoxLayout>

namespace EMStudio
{
    BlendGraphViewWidget::BlendGraphViewWidget(AnimGraphPlugin* plugin, QWidget* parentWidget)
        : QWidget(parentWidget)
        , m_parentPlugin(plugin)
    {
        for (uint32 i = 0; i < NUM_OPTIONS; ++i)
        {
            m_actions[i] = nullptr;
        }

        EMotionFX::ActorEditorRequestBus::Handler::BusConnect();
    }

    QToolBar* BlendGraphViewWidget::CreateTopToolBar()
    {
        QToolBar* toolBar = new QToolBar(this);
        toolBar->setObjectName("EMFX.BlendGraphViewWidget.TopToolBar");
        // Create new anim graph
        {
            QAction* action = toolBar->addAction(QIcon(":/EMotionFX/Plus.svg"),
                tr("Create a new anim graph"),
                this, &BlendGraphViewWidget::OnCreateAnimGraph);
            action->setObjectName("EMFX.BlendGraphViewWidget.NewButton");
            
            //action->setShortcut(QKeySequence::New);
            m_actions[FILE_NEW] = action;
        }

        // Open anim graph
        {
            QAction* action = toolBar->addAction(
                QIcon(":/EMotionFX/Open.svg"),
                tr("Open anim graph asset"));
            m_actions[FILE_OPEN] = action;

            QToolButton* toolButton = qobject_cast<QToolButton*>(toolBar->widgetForAction(action));
            AZ_Assert(toolButton, "The action widget must be a tool button.");
            toolButton->setPopupMode(QToolButton::InstantPopup);

            m_openMenu = new QMenu(toolBar);
            action->setMenu(m_openMenu);
            BuildOpenMenu();
            connect(m_openMenu, &QMenu::aboutToShow, this, &BlendGraphViewWidget::BuildOpenMenu);
        }

        // Save anim graph
        {
            QAction* saveMenuAction = toolBar->addAction(
                QIcon(":/EMotionFX/Save.svg"),
                tr("Save anim graph"));

            QToolButton* toolButton = qobject_cast<QToolButton*>(toolBar->widgetForAction(saveMenuAction));
            AZ_Assert(toolButton, "The action widget must be a tool button.");
            toolButton->setPopupMode(QToolButton::InstantPopup);

            QMenu* contextMenu = new QMenu(toolBar);

            m_actions[FILE_SAVE] = contextMenu->addAction(tr("Save"), m_parentPlugin, &AnimGraphPlugin::OnFileSave);
            m_actions[FILE_SAVEAS] = contextMenu->addAction(tr("Save as..."), m_parentPlugin, &AnimGraphPlugin::OnFileSaveAs);

            saveMenuAction->setMenu(contextMenu);
        }

        toolBar->addSeparator();

        m_actions[ACTIVATE_ANIMGRAPH] = toolBar->addAction(QIcon(":/EMotionFX/PlayForward.svg"),
            tr("Activate Animgraph/State"),
            &m_parentPlugin->GetActionManager(), &AnimGraphActionManager::ActivateAnimGraph);
        toolBar->addSeparator();

        m_actions[SELECTION_ZOOMSELECTION] = toolBar->addAction(QIcon(":/EMotionFX/ZoomSelected.svg"),
            tr("Zoom Selection"),
            this, &BlendGraphViewWidget::ZoomSelected);

        // Visualization options
        {
            QAction* menuAction = toolBar->addAction(
                QIcon(":/EMotionFX/Visualization.svg"),
                tr("Visualization"));

            QToolButton* toolButton = qobject_cast<QToolButton*>(toolBar->widgetForAction(menuAction));
            AZ_Assert(toolButton, "The action widget must be a tool button.");
            toolButton->setPopupMode(QToolButton::InstantPopup);

            QMenu* contextMenu = new QMenu(toolBar);

            m_actions[VISUALIZATION_PLAYSPEEDS] = contextMenu->addAction(tr("Display Play Speeds"), this, &BlendGraphViewWidget::OnDisplayPlaySpeeds);
            m_actions[VISUALIZATION_GLOBALWEIGHTS] = contextMenu->addAction(tr("Display Global Weights"), this, &BlendGraphViewWidget::OnDisplayGlobalWeights);
            m_actions[VISUALIZATION_SYNCSTATUS] = contextMenu->addAction(tr("Display Sync Status"), this, &BlendGraphViewWidget::OnDisplaySyncStatus);
            m_actions[VISUALIZATION_PLAYPOSITIONS] = contextMenu->addAction(tr("Display Play Positions"), this, &BlendGraphViewWidget::OnDisplayPlayPositions);

            m_actions[VISUALIZATION_PLAYSPEEDS]->setCheckable(true);
            m_actions[VISUALIZATION_GLOBALWEIGHTS]->setCheckable(true);
            m_actions[VISUALIZATION_SYNCSTATUS]->setCheckable(true);
            m_actions[VISUALIZATION_PLAYPOSITIONS]->setCheckable(true);

            menuAction->setMenu(contextMenu);
        }

        toolBar->addSeparator();

        // Alignment Options
        m_actions[SELECTION_ALIGNLEFT] = toolBar->addAction(QIcon(":/EMotionFX/AlignLeft.svg"),
            tr("Align left"),
            &m_parentPlugin->GetActionManager(), &AnimGraphActionManager::AlignLeft);

        m_actions[SELECTION_ALIGNRIGHT] = toolBar->addAction(QIcon(":/EMotionFX/AlignRight.svg"),
            tr("Align right"),
            &m_parentPlugin->GetActionManager(), &AnimGraphActionManager::AlignRight);

        m_actions[SELECTION_ALIGNTOP] = toolBar->addAction(QIcon(":/EMotionFX/AlignTop.svg"),
            tr("Align top"),
            &m_parentPlugin->GetActionManager(), &AnimGraphActionManager::AlignTop);

        m_actions[SELECTION_ALIGNBOTTOM] = toolBar->addAction(QIcon(":/EMotionFX/AlignBottom.svg"),
            tr("Align bottom"),
            &m_parentPlugin->GetActionManager(), &AnimGraphActionManager::AlignBottom);

        return toolBar;
    }

    QToolBar* BlendGraphViewWidget::CreateNavigationToolBar()
    {
        QToolBar* toolBar = new QToolBar(this);

        m_actions[NAVIGATION_BACK] = toolBar->addAction(QIcon(":/EMotionFX/Backward.svg"),
            tr("Back"),
            this, [=] {
                m_parentPlugin->GetNavigationHistory()->StepBackward();
                UpdateNavigation();
            });

        m_actions[NAVIGATION_FORWARD] = toolBar->addAction(QIcon(":/EMotionFX/Forward.svg"),
            tr("Forward"),
            this, [=] {
                m_parentPlugin->GetNavigationHistory()->StepForward();
                UpdateNavigation();
            });

        mNavigationLink = new NavigationLinkWidget(m_parentPlugin, this);
        mNavigationLink->setMinimumHeight(28);
        toolBar->addWidget(mNavigationLink);

        m_actions[NAVIGATION_NAVPANETOGGLE] = toolBar->addAction(QIcon(":/EMotionFX/List.svg"),
            tr("Show/hide navigation pane"),
            this, &BlendGraphViewWidget::ToggleNavigationPane);

        return toolBar;
    }

    void BlendGraphViewWidget::Init(BlendGraphWidget* blendGraphWidget)
    {
        connect(&m_parentPlugin->GetAnimGraphModel(), &AnimGraphModel::FocusChanged, this, &BlendGraphViewWidget::OnFocusChanged);
        connect(&m_parentPlugin->GetAnimGraphModel().GetSelectionModel(), &QItemSelectionModel::selectionChanged, this, &BlendGraphViewWidget::UpdateSelection);
        connect(m_parentPlugin->GetNavigationHistory(), &NavigationHistory::ChangedSteppingLimits, this, &BlendGraphViewWidget::UpdateNavigation);

        // create the vertical layout with the menu and the graph widget as entries
        QVBoxLayout* verticalLayout = new QVBoxLayout(this);
        verticalLayout->setSizeConstraint(QLayout::SetNoConstraint);
        verticalLayout->setSpacing(0);
        verticalLayout->setMargin(2);

        // Create toolbars
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

        UpdateNavigation();
        UpdateAnimGraphOptions();
        UpdateSelection();
    }

    BlendGraphViewWidget::~BlendGraphViewWidget()
    {
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

    void BlendGraphViewWidget::UpdateSelection()
    {
        // do we have any selection?
        const bool anySelection = m_parentPlugin->GetAnimGraphModel().GetSelectionModel().hasSelection();
        SetOptionEnabled(SELECTION_ZOOMSELECTION, anySelection);

        QModelIndex firstSelectedNode;
        bool atLeastTwoNodes = false;
        const QModelIndexList selectedIndexes = m_parentPlugin->GetAnimGraphModel().GetSelectionModel().selectedRows();
        for (const QModelIndex& selected : selectedIndexes)
        {
            const AnimGraphModel::ModelItemType itemType = selected.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>();
            if (itemType == AnimGraphModel::ModelItemType::NODE)
            {
                if (firstSelectedNode.isValid())
                {
                    atLeastTwoNodes = true;
                    break;
                }
                else
                {
                    firstSelectedNode = selected;
                }
            }
        }

        if (m_parentPlugin->GetActionFilter().m_editNodes &&
            atLeastTwoNodes)
        {
            SetOptionEnabled(SELECTION_ALIGNLEFT, true);
            SetOptionEnabled(SELECTION_ALIGNRIGHT, true);
            SetOptionEnabled(SELECTION_ALIGNTOP, true);
            SetOptionEnabled(SELECTION_ALIGNBOTTOM, true);
        }
        else
        {
            SetOptionEnabled(SELECTION_ALIGNLEFT, false);
            SetOptionEnabled(SELECTION_ALIGNRIGHT, false);
            SetOptionEnabled(SELECTION_ALIGNTOP, false);
            SetOptionEnabled(SELECTION_ALIGNBOTTOM, false);
        }
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

    void BlendGraphViewWidget::SetOptionFlag(EOptionFlag option, bool isEnabled)
    {
        const uint32 optionIndex = (uint32)option;
        if (m_actions[optionIndex])
        {
            m_actions[optionIndex]->setChecked(isEnabled);
        }
    }

    void BlendGraphViewWidget::SetOptionEnabled(EOptionFlag option, bool isEnabled)
    {
        const uint32 optionIndex = (uint32)option;
        if (m_actions[optionIndex])
        {
            m_actions[optionIndex]->setEnabled(isEnabled);
        }
    }

    void BlendGraphViewWidget::BuildOpenMenu()
    {
        m_openMenu->clear();

        m_actions[FILE_OPEN] = m_openMenu->addAction(tr("Open..."));
        connect(m_actions[FILE_OPEN], &QAction::triggered, m_parentPlugin, &AnimGraphPlugin::OnFileOpen);

        const uint32 numAnimGraphs = EMotionFX::GetAnimGraphManager().GetNumAnimGraphs();
        if (numAnimGraphs > 0)
        {
            m_openMenu->addSeparator();
            for (uint32 i = 0; i < numAnimGraphs; ++i)
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
        const uint32 numActorInstances = selectionList.GetNumSelectedActorInstances();

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
                for (uint32 i = 0; i < numActorInstances; ++i)
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


    void BlendGraphViewWidget::NavigateToParent()
    {
        const QModelIndex parentFocus = m_parentPlugin->GetAnimGraphModel().GetParentFocus();
        if (parentFocus.isValid())
        {
            QModelIndex newParentFocus = parentFocus.model()->parent(parentFocus);
            if (newParentFocus.isValid())
            {
                m_parentPlugin->GetAnimGraphModel().Focus(newParentFocus);
            }
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
    }

    void BlendGraphViewWidget::NavigateToNode()
    {
        const QModelIndexList currentModelIndexes = m_parentPlugin->GetAnimGraphModel().GetSelectionModel().selectedRows();
        if (!currentModelIndexes.empty())
        {
            const QModelIndex currentModelIndex = currentModelIndexes.front();
            m_parentPlugin->GetAnimGraphModel().Focus(currentModelIndex);
        }
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

    void BlendGraphViewWidget::keyPressEvent(QKeyEvent* event)
    {
        switch (event->key())
        {
        case Qt::Key_Backspace:
        {
            m_parentPlugin->GetNavigationHistory()->StepBackward();
            event->accept();
            break;
        }

        default:
            event->ignore();
        }
    }


    // on key release
    void BlendGraphViewWidget::keyReleaseEvent(QKeyEvent* event)
    {
        switch (event->key())
        {
        case Qt::Key_Backspace:
        {
            event->accept();
            break;
        }

        default:
            event->ignore();
        }
    }
} // namespace EMStudio
