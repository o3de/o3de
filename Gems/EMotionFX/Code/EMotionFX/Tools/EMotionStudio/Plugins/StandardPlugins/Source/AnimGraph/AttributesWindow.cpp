/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzQtComponents/Components/Widgets/Card.h>
#include <AzQtComponents/Components/Widgets/CardHeader.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphMotionCondition.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphObjectFactory.h>
#include <EMotionFX/Source/AnimGraphPlayTimeCondition.h>
#include <EMotionFX/Source/AnimGraphStateCondition.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphTriggerAction.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphConnectionCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphConditionCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphTriggerActionCommands.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphEditor.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphModel.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AttributesWindow.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphWidget.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/GraphNodeFactory.h>
#include <MCore/Source/LogManager.h>
#include <MCore/Source/ReflectionSerializer.h>
#include <QCheckBox>
#include <QContextMenuEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <Editor/InspectorBus.h>
#include <Source/Editor/ObjectEditor.h>


namespace EMStudio
{
    AttributesWindow::AttributesWindow(AnimGraphPlugin* plugin, QWidget* parent)
        : QWidget(parent)
    {
        m_plugin                 = plugin;
        m_pasteConditionsWindow  = nullptr;
        m_scrollArea             = new QScrollArea();

        QVBoxLayout* mainLayout = new QVBoxLayout();
        mainLayout->setMargin(0);
        mainLayout->setSpacing(1);
        setLayout(mainLayout);

        mainLayout->addWidget(m_scrollArea);
        m_scrollArea->setWidgetResizable(true);

        // The main reflected widget will contain the non-custom attribute version of the
        // attribute widget. The intention is to reuse the Reflected Property Editor and
        // Cards
        {
            m_mainReflectedWidget = new QWidget();
            m_mainReflectedWidget->setVisible(false);

            QVBoxLayout* verticalLayout = new QVBoxLayout();
            m_mainReflectedWidget->setLayout(verticalLayout);
            verticalLayout->setAlignment(Qt::AlignTop);
            verticalLayout->setMargin(0);
            verticalLayout->setSpacing(0);
            verticalLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);

            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            if (!serializeContext)
            {
                AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
            }
            else
            {
                // 1. Create anim graph card.
                m_animGraphEditor = new EMotionFX::AnimGraphEditor(nullptr, serializeContext, m_mainReflectedWidget);
                verticalLayout->addWidget(m_animGraphEditor);
                m_animGraphEditor->setVisible(false);

                // 2. Create object card
                m_objectEditor = new EMotionFX::ObjectEditor(serializeContext, m_mainReflectedWidget);
                m_objectEditor->setObjectName("EMFX.AttributesWindow.ObjectEditor");

                m_objectCard = new AzQtComponents::Card(m_mainReflectedWidget);
                m_objectCard->setTitle("");
                m_objectCard->setContentWidget(m_objectEditor);
                m_objectCard->setExpanded(true);

                AzQtComponents::CardHeader* cardHeader = m_objectCard->header();
                cardHeader->setHasContextMenu(false);
                cardHeader->setHelpURL("");

                verticalLayout->addWidget(m_objectCard);
                m_objectCard->setVisible(false);
            }
            {
                m_conditionsWidget = new QWidget();
                QVBoxLayout* conditionsVerticalLayout = new QVBoxLayout();
                m_conditionsWidget->setLayout(conditionsVerticalLayout);
                m_conditionsWidget->setObjectName("EMFX.AttributesWindowWidget.NodeTransition.ConditionsWidget");
                conditionsVerticalLayout->setAlignment(Qt::AlignTop);
                conditionsVerticalLayout->setMargin(0);
                conditionsVerticalLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);

                m_conditionsLayout = new QVBoxLayout();
                m_conditionsLayout->setAlignment(Qt::AlignTop);
                m_conditionsLayout->setMargin(0);
                m_conditionsLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
                conditionsVerticalLayout->addLayout(m_conditionsLayout);

                m_addConditionButton = new AddConditionButton(m_plugin, m_conditionsWidget);
                m_addConditionButton->setObjectName("EMFX.AttributesWindowWidget.NodeTransition.AddConditionsWidget");
                connect(m_addConditionButton, &AddConditionButton::ObjectTypeChosen, this, [=](AZ::TypeId conditionType)
                    {
                        AddCondition(conditionType);
                    });

                conditionsVerticalLayout->addWidget(m_addConditionButton);

                verticalLayout->addWidget(m_conditionsWidget);
                m_conditionsWidget->setVisible(false);
            }
            {
                m_actionsWidget = new QWidget();
                QVBoxLayout* actionVerticalLayout = new QVBoxLayout();
                m_actionsWidget->setLayout(actionVerticalLayout);
                actionVerticalLayout->setAlignment(Qt::AlignTop);
                actionVerticalLayout->setMargin(0);
                actionVerticalLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);

                m_actionsLayout = new QVBoxLayout();
                m_actionsLayout->setAlignment(Qt::AlignTop);
                m_actionsLayout->setMargin(0);
                m_actionsLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
                actionVerticalLayout->addLayout(m_actionsLayout);

                AddActionButton* addActionButton = new AddActionButton(m_plugin, m_actionsWidget);
                connect(addActionButton, &AddActionButton::ObjectTypeChosen, this, [=](AZ::TypeId actionType)
                    {
                        const AnimGraphModel::ModelItemType itemType = m_displayingModelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>();
                        if (itemType == AnimGraphModel::ModelItemType::TRANSITION)
                        {
                            AddTransitionAction(actionType);
                        }
                        else
                        {
                            AddStateAction(actionType);
                        }
                    });
                actionVerticalLayout->addWidget(addActionButton);

                verticalLayout->addWidget(m_actionsWidget);
                m_actionsWidget->setVisible(false);
            }
        }

        connect(&plugin->GetAnimGraphModel().GetSelectionModel(), &QItemSelectionModel::selectionChanged, this, [=]([[maybe_unused]] const QItemSelection& selected, [[maybe_unused]] const QItemSelection& deselected)
            {
                UpdateAndShowInInspector();
            });
        connect(&plugin->GetAnimGraphModel(), &AnimGraphModel::dataChanged, this, &AttributesWindow::OnDataChanged);

        Init(QModelIndex(), true);

        AttributesWindowRequestBus::Handler::BusConnect();
    }

    AttributesWindow::~AttributesWindow()
    {
        // Clear the inspector in case this window is currently shown.
        EMStudio::InspectorRequestBus::Broadcast(&EMStudio::InspectorRequestBus::Events::ClearIfShown, this);

        AttributesWindowRequestBus::Handler::BusDisconnect();

        if (m_mainReflectedWidget)
        {
            if (m_scrollArea->widget() == m_mainReflectedWidget)
            {
                m_scrollArea->takeWidget();
            }
            delete m_mainReflectedWidget;
        }
    }

    void AttributesWindow::Init(const QModelIndex& modelIndex, bool forceUpdate)
    {
        if (m_isLocked)
        {
            return;
        }

        if (!modelIndex.isValid())
        {
            m_objectEditor->ClearInstances(false);
            for (const CachedWidgets& widget : m_conditionsCachedWidgets)
            {
                widget.m_objectEditor->ClearInstances(false);
            }
            for (const CachedWidgets& widget : m_actionsCachedWidgets)
            {
                widget.m_objectEditor->ClearInstances(false);
            }
        }

        // This only works on TRANSITIONS and NODES
        AnimGraphModel::ModelItemType itemType = modelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>();
        if (itemType != AnimGraphModel::ModelItemType::NODE && itemType != AnimGraphModel::ModelItemType::TRANSITION)
        {
            return;
        }

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!serializeContext)
        {
            AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
            return;
        }

        EMotionFX::AnimGraphObject* object = modelIndex.data(AnimGraphModel::ROLE_ANIM_GRAPH_OBJECT_PTR).value<EMotionFX::AnimGraphObject*>();

        QWidget* attributeWidget = m_plugin->GetGraphNodeFactory()->CreateAttributeWidget(azrtti_typeid(object));
        if (attributeWidget)
        {
            // In the case we have a custom attribute widget, we cannot reuse the widget, so we just replace it
            if (m_scrollArea->widget() == m_mainReflectedWidget)
            {
                m_scrollArea->takeWidget();
            }
            m_scrollArea->setWidget(attributeWidget);
        }
        else
        {
            EMotionFX::AnimGraph* animGraph = nullptr;
            if (object)
            {
                animGraph = object->GetAnimGraph();
            }
            else
            {
                animGraph = m_plugin->GetActiveAnimGraph();
            }

            m_animGraphEditor->SetAnimGraph(animGraph);
            m_animGraphEditor->setVisible(animGraph);

            if (object)
            {
                m_objectCard->setTitle(object->GetPaletteName());

                AzQtComponents::CardHeader* cardHeader = m_objectCard->header();
                cardHeader->setHelpURL(object->GetHelpUrl());

                if (!forceUpdate && object == m_objectEditor->GetObject())
                {
                    m_objectEditor->InvalidateValues();
                }
                else
                {
                    m_objectEditor->ClearInstances(false);
                    m_objectEditor->AddInstance(object, azrtti_typeid(object));
                }

                UpdateConditions(object, serializeContext, forceUpdate);
                UpdateActions(object, serializeContext, forceUpdate);
            }
            else
            {
                // In case the previous selected object was showing any of these
                m_conditionsWidget->setVisible(false);
                m_actionsWidget->setVisible(false);
            }

            m_objectCard->setVisible(object);

            if (m_scrollArea->widget() != m_mainReflectedWidget)
            {
                m_scrollArea->setWidget(m_mainReflectedWidget);
            }
        }

        m_displayingModelIndex = modelIndex;
    }

    void AttributesWindow::UpdateConditions(EMotionFX::AnimGraphObject* object, AZ::SerializeContext* serializeContext, bool forceUpdate)
    {
        if (azrtti_typeid(object) == azrtti_typeid<EMotionFX::AnimGraphStateTransition>())
        {
            const EMotionFX::AnimGraphStateTransition* stateTransition = static_cast<EMotionFX::AnimGraphStateTransition*>(object);

            const size_t numConditions = stateTransition->GetNumConditions();
            const size_t numConditionsWidgets = m_conditionsCachedWidgets.size();
            const size_t numConditionsAlreadyWithWidgets = AZStd::min(numConditions, numConditionsWidgets);
            for (size_t c = 0; c < numConditionsAlreadyWithWidgets; ++c)
            {
                EMotionFX::AnimGraphTransitionCondition* condition = stateTransition->GetCondition(c);
                CachedWidgets& conditionWidgets = m_conditionsCachedWidgets[c];

                conditionWidgets.m_card->setTitle(condition->GetPaletteName());
                AzQtComponents::CardHeader* cardHeader = conditionWidgets.m_card->header();
                cardHeader->setHelpURL(condition->GetHelpUrl());

                if (!forceUpdate && conditionWidgets.m_objectEditor->GetObject() == condition)
                {
                    conditionWidgets.m_objectEditor->InvalidateValues();
                }
                else
                {
                    conditionWidgets.m_objectEditor->ClearInstances(false);
                    conditionWidgets.m_objectEditor->AddInstance(condition, azrtti_typeid(condition));
                }
            }

            if (numConditions > numConditionsWidgets)
            {
                for (size_t c = numConditionsWidgets; c < numConditions; ++c)
                {
                    EMotionFX::AnimGraphTransitionCondition* condition = stateTransition->GetCondition(c);

                    EMotionFX::ObjectEditor* conditionEditor = new EMotionFX::ObjectEditor(serializeContext, this);
                    conditionEditor->AddInstance(condition, azrtti_typeid(condition));

                    // Create the card and put the editor widget in it.
                    AzQtComponents::Card* card = new AzQtComponents::Card(m_conditionsWidget);
                    connect(card, &AzQtComponents::Card::contextMenuRequested, this, &AttributesWindow::OnConditionContextMenu);

                    card->setTitle(condition->GetPaletteName());
                    card->setContentWidget(conditionEditor);
                    card->setProperty("conditionIndex", static_cast<unsigned int>(c));
                    card->setExpanded(true);

                    AzQtComponents::CardHeader* cardHeader = card->header();
                    cardHeader->setHelpURL(condition->GetHelpUrl());

                    m_conditionsLayout->addWidget(card);

                    m_conditionsCachedWidgets.emplace_back(card, conditionEditor);
                } // for all conditions
            }
            else if (numConditionsWidgets > numConditions)
            {
                // remove all the widgets that are no longer valid
                for (size_t w = numConditions; w < numConditionsWidgets; ++w)
                {
                    CachedWidgets& conditionWidgets = m_conditionsCachedWidgets[w];

                    // Just the card needs to be removed.
                    conditionWidgets.m_card->setVisible(false);
                    m_conditionsLayout->removeWidget(conditionWidgets.m_card);
                }
                m_conditionsCachedWidgets.erase(m_conditionsCachedWidgets.begin() + numConditions, m_conditionsCachedWidgets.end());
            }

            m_conditionsWidget->setVisible(true);
        }
        else
        {
            m_conditionsWidget->setVisible(false);
        }
    }

    void AttributesWindow::UpdateActions(EMotionFX::AnimGraphObject* object, AZ::SerializeContext* serializeContext, bool forceUpdate)
    {
        const EMotionFX::TriggerActionSetup* actionSetup = nullptr;

        if (azrtti_istypeof<EMotionFX::AnimGraphNode>(object))
        {
            const EMotionFX::AnimGraphNode* node = static_cast<EMotionFX::AnimGraphNode*>(object);
            const EMotionFX::AnimGraphNode* parent = node->GetParentNode();
            if (node->GetCanActAsState() && parent && azrtti_istypeof<EMotionFX::AnimGraphStateMachine>(parent))
            {
                actionSetup = &node->GetTriggerActionSetup();
            }
        }
        else if (azrtti_typeid(object) == azrtti_typeid<EMotionFX::AnimGraphStateTransition>())
        {
            const EMotionFX::AnimGraphStateTransition* stateTransition = static_cast<EMotionFX::AnimGraphStateTransition*>(object);
            actionSetup = &stateTransition->GetTriggerActionSetup();
        }

        if (actionSetup)
        {
            const size_t numActions = actionSetup->GetNumActions();
            const size_t numActionWidgets = m_actionsCachedWidgets.size();
            const size_t numActionsAlreadyWithWidgets = AZStd::min(numActions, numActionWidgets);
            for (size_t a = 0; a < numActionsAlreadyWithWidgets; ++a)
            {
                EMotionFX::AnimGraphTriggerAction* action = actionSetup->GetAction(a);
                CachedWidgets& actionWidgets = m_actionsCachedWidgets[a];

                actionWidgets.m_card->setTitle(action->GetPaletteName());
                AzQtComponents::CardHeader* cardHeader = actionWidgets.m_card->header();
                cardHeader->setHelpURL(action->GetHelpUrl());

                if (!forceUpdate && actionWidgets.m_objectEditor->GetObject() == action)
                {
                    actionWidgets.m_objectEditor->InvalidateValues();
                }
                else
                {
                    actionWidgets.m_objectEditor->ClearInstances(false);
                    actionWidgets.m_objectEditor->AddInstance(action, azrtti_typeid(action));
                }
            }

            if (numActions > numActionWidgets)
            {
                for (size_t a = numActionWidgets; a < numActions; ++a)
                {
                    EMotionFX::AnimGraphTriggerAction* action = actionSetup->GetAction(a);

                    EMotionFX::ObjectEditor* actionEditor = new EMotionFX::ObjectEditor(serializeContext, this);
                    actionEditor->AddInstance(action, azrtti_typeid(action));

                    // Create the card and put the editor widget in it.
                    AzQtComponents::Card* card = new AzQtComponents::Card(m_actionsWidget);
                    connect(card, &AzQtComponents::Card::contextMenuRequested, this, &AttributesWindow::OnActionContextMenu);

                    card->setTitle(action->GetPaletteName());
                    card->setContentWidget(actionEditor);
                    card->setProperty("actionIndex", static_cast<unsigned int>(a));
                    card->setExpanded(true);

                    AzQtComponents::CardHeader* cardHeader = card->header();
                    cardHeader->setHelpURL(action->GetHelpUrl());

                    m_actionsLayout->addWidget(card);

                    m_actionsCachedWidgets.emplace_back(card, actionEditor);
                } // for all actions
            }
            else if (numActionWidgets > numActions)
            {
                // remove all the widgets that are no longer valid
                for (size_t w = numActions; w < numActionWidgets; ++w)
                {
                    CachedWidgets& actionWidgets = m_actionsCachedWidgets[w];

                    // Just the card needs to be removed
                    actionWidgets.m_card->setVisible(false);
                    m_actionsLayout->removeWidget(actionWidgets.m_card);
                }
                m_actionsCachedWidgets.erase(m_actionsCachedWidgets.begin() + numActions, m_actionsCachedWidgets.end());
            }

            m_actionsWidget->setVisible(true);
        }
        else
        {
            m_actionsWidget->setVisible(false);
        }
    }


    void AttributesWindow::OnConditionContextMenu(const QPoint& position)
    {
        const AzQtComponents::Card* card = static_cast<AzQtComponents::Card*>(sender());
        const int conditionIndex = card->property("conditionIndex").toInt();

        QMenu* contextMenu = new QMenu(this);

        QAction* deleteAction = contextMenu->addAction("Delete condition");
        deleteAction->setProperty("conditionIndex", conditionIndex);
        connect(deleteAction, &QAction::triggered, this, &AttributesWindow::OnRemoveCondition);

        AddTransitionCopyPasteMenuEntries(contextMenu);

        if (!contextMenu->isEmpty())
        {
            contextMenu->popup(position);
        }

        connect(contextMenu, &QMenu::triggered, contextMenu, &QMenu::deleteLater);
    }

    void AttributesWindow::AddTransitionCopyPasteMenuEntries(QMenu* menu)
    {
        const NodeGraph* activeGraph = m_plugin->GetGraphWidget()->GetActiveGraph();
        if (!activeGraph)
        {
            return;
        }

        QAction* copyAction = menu->addAction("Copy transition");
        connect(copyAction, &QAction::triggered, this, &AttributesWindow::OnCopy);

        if (!activeGraph->IsInReferencedGraph())
        {
            if (m_copyPasteClipboard.m_transition.IsSuccess())
            {
                QAction* pasteAction = menu->addAction("Paste transition properties including conditions");
                connect(pasteAction, &QAction::triggered, this, &AttributesWindow::OnPasteFullTransition);
            }

            if (!m_copyPasteClipboard.m_conditions.empty())
            {
                QAction* pasteAction = menu->addAction("Paste conditions only");
                connect(pasteAction, &QAction::triggered, this, &AttributesWindow::OnPasteConditions);

                QAction* pasteSelectiveAction = menu->addAction("Paste conditions selective");
                connect(pasteSelectiveAction, &QAction::triggered, this, &AttributesWindow::OnPasteConditionsSelective);
            }
        }
    }

    void AttributesWindow::OnActionContextMenu(const QPoint& position)
    {
        const AnimGraphModel::ModelItemType itemType = m_displayingModelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>();

        const AzQtComponents::Card* card = static_cast<AzQtComponents::Card*>(sender());
        const int actionIndex = card->property("actionIndex").toInt();

        QMenu contextMenu(this);

        QAction* deleteAction = contextMenu.addAction("Delete action");
        deleteAction->setProperty("actionIndex", actionIndex);
        if (itemType == AnimGraphModel::ModelItemType::TRANSITION)
        {
            connect(deleteAction, &QAction::triggered, this, &AttributesWindow::OnRemoveTransitionAction);
        }
        else
        {
            connect(deleteAction, &QAction::triggered, this, &AttributesWindow::OnRemoveStateAction);
        }

        if (!contextMenu.isEmpty())
        {
            contextMenu.exec(position);
        }
    }

    void AttributesWindow::UpdateAndShowInInspector()
    {
        const QModelIndexList modelIndexes = m_plugin->GetAnimGraphModel().GetSelectionModel().selectedRows();
        if (!modelIndexes.empty())
        {
            Init(modelIndexes.front());
        }
        else
        {
            Init();
        }

        EMStudio::InspectorRequestBus::Broadcast(&EMStudio::InspectorRequestBus::Events::Update, this);
    }

    void AttributesWindow::OnDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
    {
        QItemSelection changes(topLeft, bottomRight);
        if (changes.contains(m_displayingModelIndex))
        {
            if (roles.empty())
            {
                Init(m_displayingModelIndex);
            }
            else
            {
                if (roles.contains(AnimGraphModel::ROLE_TRANSITION_CONDITIONS))
                {
                    AZ::SerializeContext* serializeContext = nullptr;
                    AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
                    if (!serializeContext)
                    {
                        AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
                        return;
                    }

                    EMotionFX::AnimGraphObject* object = m_displayingModelIndex.data(AnimGraphModel::ROLE_ANIM_GRAPH_OBJECT_PTR).value<EMotionFX::AnimGraphObject*>();
                    UpdateConditions(object, serializeContext);
                }
                else if (roles.contains(AnimGraphModel::ROLE_TRIGGER_ACTIONS))
                {
                    AZ::SerializeContext* serializeContext = nullptr;
                    AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
                    if (!serializeContext)
                    {
                        AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
                        return;
                    }

                    EMotionFX::AnimGraphObject* object = m_displayingModelIndex.data(AnimGraphModel::ROLE_ANIM_GRAPH_OBJECT_PTR).value<EMotionFX::AnimGraphObject*>();
                    UpdateActions(object, serializeContext);
                }
            }
        }
    }

    EMotionFX::AnimGraphEditor* AttributesWindow::GetAnimGraphEditor() const
    {
        return m_animGraphEditor;
    }

    void AttributesWindow::AddCondition(const AZ::TypeId& conditionType)
    {
        AZ_Assert(m_displayingModelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>() == AnimGraphModel::ModelItemType::TRANSITION,
            "Expected a transition");

        const EMotionFX::AnimGraphStateTransition* transition = m_displayingModelIndex.data(AnimGraphModel::ROLE_TRANSITION_POINTER).value<EMotionFX::AnimGraphStateTransition*>();

        const EMotionFX::AnimGraphNode* sourceNode = transition->GetSourceNode();
        const EMotionFX::AnimGraphNode* targetNode = transition->GetTargetNode();
        const EMotionFX::AnimGraphNode* parentNode = targetNode->GetParentNode();

        if (azrtti_typeid(parentNode) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
        {
            AZStd::optional<AZStd::string> contents;
            if (conditionType == azrtti_typeid<EMotionFX::AnimGraphMotionCondition>() &&
                sourceNode && azrtti_typeid(sourceNode) == azrtti_typeid<EMotionFX::AnimGraphMotionNode>())
            {
                EMotionFX::AnimGraphMotionCondition motionCondition;
                motionCondition.SetMotionNodeId(sourceNode->GetId());

                AZ::Outcome<AZStd::string> serializeOutcome = MCore::ReflectionSerializer::Serialize(&motionCondition);
                if (serializeOutcome.IsSuccess())
                {
                    contents = serializeOutcome.GetValue();
                }
            }
            else if (conditionType == azrtti_typeid<EMotionFX::AnimGraphStateCondition>() &&
                     sourceNode && azrtti_typeid(sourceNode) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
            {
                EMotionFX::AnimGraphStateCondition stateCondition;
                stateCondition.SetStateId(sourceNode->GetId());

                AZ::Outcome<AZStd::string> serializeOutcome = MCore::ReflectionSerializer::Serialize(&stateCondition);
                if (serializeOutcome.IsSuccess())
                {
                    contents = serializeOutcome.GetValue();
                }
            }
            else if (conditionType == azrtti_typeid<EMotionFX::AnimGraphPlayTimeCondition>() &&
                     sourceNode)
            {
                EMotionFX::AnimGraphPlayTimeCondition playTimeCondition;
                playTimeCondition.SetNodeId(sourceNode->GetId());

                AZ::Outcome<AZStd::string> serializeOutcome = MCore::ReflectionSerializer::Serialize(&playTimeCondition);
                if (serializeOutcome.IsSuccess())
                {
                    contents = serializeOutcome.GetValue();
                }
            }

            CommandSystem::CommandAddTransitionCondition* addConditionCommand = aznew CommandSystem::CommandAddTransitionCondition(
                transition->GetAnimGraph()->GetID(),
                transition->GetId(),
                conditionType,
                /*insertAt=*/AZStd::nullopt,
                contents);

            AZStd::string commandResult;
            if (!GetCommandManager()->ExecuteCommand(addConditionCommand, commandResult))
            {
                AZ_Error("EMotionFX", false, commandResult.c_str());
            }
        }
    }


    // when we press the remove condition button
    void AttributesWindow::OnRemoveCondition()
    {
        AZ_Assert(m_displayingModelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>() == AnimGraphModel::ModelItemType::TRANSITION,
            "Expected a transition");

        QAction* action = static_cast<QAction*>(sender());
        const int conditionIndex = action->property("conditionIndex").toInt();

        // convert the object into a state transition
        EMotionFX::AnimGraphStateTransition* transition = m_displayingModelIndex.data(AnimGraphModel::ROLE_TRANSITION_POINTER).value<EMotionFX::AnimGraphStateTransition*>();

        CommandSystem::CommandRemoveTransitionCondition* removeConditionCommand = aznew CommandSystem::CommandRemoveTransitionCondition(
            transition->GetAnimGraph()->GetID(), transition->GetId(), conditionIndex);

        AZStd::string commandResult;
        if (!EMStudio::GetCommandManager()->ExecuteCommand(removeConditionCommand, commandResult))
        {
            AZ_Error("EMotionFX", false, commandResult.c_str());
        }
    }


    void AttributesWindow::contextMenuEvent(QContextMenuEvent* event)
    {
        if (!m_displayingModelIndex.isValid()
            || m_displayingModelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>() != AnimGraphModel::ModelItemType::TRANSITION)
        {
            return;
        }

        QMenu menu(this);

        AddTransitionCopyPasteMenuEntries(&menu);

        // show the menu at the given position
        if (menu.isEmpty() == false)
        {
            menu.exec(event->globalPos());
        }
    }

    void AttributesWindow::OnCopy()
    {
        m_copyPasteClipboard.Clear();

        if (!m_displayingModelIndex.isValid()
            || m_displayingModelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>() != AnimGraphModel::ModelItemType::TRANSITION)
        {
            return;
        }

        EMotionFX::AnimGraphStateTransition* transition = m_displayingModelIndex.data(AnimGraphModel::ROLE_TRANSITION_POINTER).value<EMotionFX::AnimGraphStateTransition*>();

        // Serialize all attributes that can be manipulated in the RPE.
        m_copyPasteClipboard.m_transition = MCore::ReflectionSerializer::SerializeMembersExcept(transition, {
            "conditions", "actionSetup",
            "id", "sourceNodeId", "targetNodeId", "isWildcard",
            "startOffsetX", "startOffsetY",
            "endOffsetX", "endOffsetY"});

        // iterate through the conditions and put them into the clipboard
        const size_t numConditions = transition->GetNumConditions();
        for (size_t i = 0; i < numConditions; ++i)
        {
            EMotionFX::AnimGraphTransitionCondition* condition = transition->GetCondition(i);

            // construct the copy & paste object and put it into the clipboard
            AZ::Outcome<AZStd::string> contents = MCore::ReflectionSerializer::Serialize(condition);
            if (contents.IsSuccess())
            {
                CopyPasteConditionObject copyPasteObject;
                copyPasteObject.m_contents = contents.GetValue();
                copyPasteObject.m_conditionType = azrtti_typeid(condition);
                condition->GetSummary(&copyPasteObject.m_summary);
                m_copyPasteClipboard.m_conditions.push_back(copyPasteObject);
            }
        }
    }

    void AttributesWindow::OnPasteConditions()
    {
        PasteTransition(/*pasteTransitionProperties=*/false, /*pasteConditions=*/true);
    }

    void AttributesWindow::OnPasteFullTransition()
    {
        PasteTransition(/*pasteTransitionProperties=*/true, /*pasteConditions=*/true);
    }

    void AttributesWindow::PasteTransition(bool pasteTransitionProperties, bool pasteConditions)
    {
        if (!m_displayingModelIndex.isValid()
            || m_displayingModelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>() != AnimGraphModel::ModelItemType::TRANSITION)
        {
            return;
        }
        EMotionFX::AnimGraphStateTransition* transition = m_displayingModelIndex.data(AnimGraphModel::ROLE_TRANSITION_POINTER).value<EMotionFX::AnimGraphStateTransition*>();
        MCore::CommandGroup commandGroup;

        pasteTransitionProperties = (pasteTransitionProperties && m_copyPasteClipboard.m_transition.IsSuccess());
        if (pasteTransitionProperties)
        {
            CommandSystem::AdjustTransition(transition,
                /*isDisabled=*/AZStd::nullopt,
                /*sourceNode=*/AZStd::nullopt, /*targetNode=*/AZStd::nullopt,
                /*startOffsetX=*/AZStd::nullopt, /*startOffsetY=*/AZStd::nullopt,
                /*endOffsetX=*/AZStd::nullopt, /*endOffsetY=*/AZStd::nullopt,
                /*attributesString=*/AZStd::nullopt, /*serializedMembers=*/m_copyPasteClipboard.m_transition.GetValue(),
                &commandGroup);
        }

        if (pasteConditions)
        {
            for (const CopyPasteConditionObject& copyPasteObject : m_copyPasteClipboard.m_conditions)
            {
                CommandSystem::CommandAddTransitionCondition* addConditionCommand = aznew CommandSystem::CommandAddTransitionCondition(
                    transition->GetAnimGraph()->GetID(),
                    transition->GetId(),
                    copyPasteObject.m_conditionType,
                    /*insertAt=*/AZStd::nullopt,
                    copyPasteObject.m_contents);
                commandGroup.AddCommand(addConditionCommand);
            }
        }

        const AZStd::string groupName = AZStd::string::format("Pasted transition %s%s%s",
            pasteTransitionProperties ? "properties " : "",
            pasteTransitionProperties && pasteConditions ? "and " : "",
            pasteConditions ? "conditions" : "");
        commandGroup.SetGroupName(groupName);

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }

    void AttributesWindow::OnPasteConditionsSelective()
    {
        if (!m_displayingModelIndex.isValid()
            || m_displayingModelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>() != AnimGraphModel::ModelItemType::TRANSITION)
        {
            return;
        }

        delete m_pasteConditionsWindow;
        m_pasteConditionsWindow = nullptr;

        EMotionFX::AnimGraphStateTransition* transition = m_displayingModelIndex.data(AnimGraphModel::ROLE_TRANSITION_POINTER).value<EMotionFX::AnimGraphStateTransition*>();

        // Open the select conditions window and return if the user canceled it.
        m_pasteConditionsWindow = new PasteConditionsWindow(this);
        if (m_pasteConditionsWindow->exec() == QDialog::Rejected)
        {
            return;
        }

        MCore::CommandGroup commandGroup;
        commandGroup.SetGroupName("Pasted transition conditions");

        const size_t numConditions = m_copyPasteClipboard.m_conditions.size();
        for (size_t i = 0; i < numConditions; ++i)
        {
            // check if the condition was selected in the window, if not skip it
            if (!m_pasteConditionsWindow->GetIsConditionSelected(i))
            {
                continue;
            }

            CommandSystem::CommandAddTransitionCondition* addConditionCommand = aznew CommandSystem::CommandAddTransitionCondition(
                transition->GetAnimGraph()->GetID(),
                transition->GetId(),
                m_copyPasteClipboard.m_conditions[i].m_conditionType,
                /*insertAt=*/AZStd::nullopt,
                m_copyPasteClipboard.m_conditions[i].m_contents);
            commandGroup.AddCommand(addConditionCommand);

        }

        if (!commandGroup.IsEmpty())
        {
            AZStd::string result;
            if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }
        }
    }

    void AttributesWindow::AddTransitionAction(const AZ::TypeId& actionType)
    {
        AZ_Assert(m_displayingModelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>() == AnimGraphModel::ModelItemType::TRANSITION,
            "Expected a transition");

        const EMotionFX::AnimGraphStateTransition* transition = m_displayingModelIndex.data(AnimGraphModel::ROLE_TRANSITION_POINTER).value<EMotionFX::AnimGraphStateTransition*>();
        CommandSystem::AddTransitionAction(transition, actionType);
    }

    void AttributesWindow::AddStateAction(const AZ::TypeId& actionType)
    {
        AZ_Assert(m_displayingModelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>() == AnimGraphModel::ModelItemType::NODE,
            "StateAction must added on an anim graph node");

        const EMotionFX::AnimGraphNode* node = m_displayingModelIndex.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
        CommandSystem::AddStateAction(node, actionType);
    }

    // when we press the remove condition button
    void AttributesWindow::OnRemoveTransitionAction()
    {
        AZ_Assert(m_displayingModelIndex.isValid(), "Object shouldn't be null.");

        QAction* action = static_cast<QAction*>(sender());
        const int actionIndex = action->property("actionIndex").toInt();
        AZ_Assert(m_displayingModelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>() == AnimGraphModel::ModelItemType::TRANSITION,
            "Expected a transition");

        EMotionFX::AnimGraphStateTransition* transition = m_displayingModelIndex.data(AnimGraphModel::ROLE_TRANSITION_POINTER).value<EMotionFX::AnimGraphStateTransition*>();
        CommandSystem::RemoveTransitionAction(transition, actionIndex);
    }

    // when we press the remove condition button
    void AttributesWindow::OnRemoveStateAction()
    {
        AZ_Assert(m_displayingModelIndex.isValid(), "Object shouldn't be null.");

        QAction* action = static_cast<QAction*>(sender());
        const int actionIndex = action->property("actionIndex").toInt();

        AZ_Assert(m_displayingModelIndex.data(AnimGraphModel::ROLE_MODEL_ITEM_TYPE).value<AnimGraphModel::ModelItemType>() == AnimGraphModel::ModelItemType::NODE,
            "StateAction must added on an anim graph node");

        EMotionFX::AnimGraphNode* node = m_displayingModelIndex.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
        CommandSystem::RemoveStateAction(node, actionIndex);
    }


    // constructor
    PasteConditionsWindow::PasteConditionsWindow(AttributesWindow* attributeWindow)
        : QDialog(attributeWindow)
    {
        // update title of the about dialog
        setWindowTitle("Paste Transition Conditions");

        // create the about dialog's layout
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setSizeConstraint(QLayout::SetFixedSize);

        layout->addWidget(new QLabel("Please select the conditions you want to paste:"));

        m_checkboxes.clear();
        const AttributesWindow::CopyPasteClipboard& copyPasteClipboard = attributeWindow->GetCopyPasteConditionClipboard();
        for (const AttributesWindow::CopyPasteConditionObject& copyPasteObject : copyPasteClipboard.m_conditions)
        {
            QCheckBox* checkbox = new QCheckBox(copyPasteObject.m_summary.c_str());
            m_checkboxes.push_back(checkbox);
            checkbox->setCheckState(Qt::Checked);
            layout->addWidget(checkbox);
        }

        // create the ok and cancel buttons
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        m_okButton = new QPushButton("OK");
        m_cancelButton = new QPushButton("Cancel");
        buttonLayout->addWidget(m_okButton);
        buttonLayout->addWidget(m_cancelButton);

        layout->addLayout(buttonLayout);
        setLayout(layout);

        connect(m_okButton, &QPushButton::clicked, this, &PasteConditionsWindow::accept);
        connect(m_cancelButton, &QPushButton::clicked, this, &PasteConditionsWindow::reject);
    }


    // destructor
    PasteConditionsWindow::~PasteConditionsWindow()
    {
    }


    // check if the condition is selected
    bool PasteConditionsWindow::GetIsConditionSelected(size_t index) const
    {
        return m_checkboxes[index]->checkState() == Qt::Checked;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    AddConditionButton::AddConditionButton(AnimGraphPlugin* plugin, QWidget* parent)
        : EMotionFX::TypeChoiceButton("Add condition", "", parent)
    {
        const AZStd::vector<EMotionFX::AnimGraphObject*>& objectPrototypes = plugin->GetAnimGraphObjectFactory()->GetUiObjectPrototypes();
        AZStd::unordered_map<AZ::TypeId, AZStd::string> types;
        m_types.reserve(objectPrototypes.size());

        for (const EMotionFX::AnimGraphObject* objectPrototype : objectPrototypes)
        {
            if (azrtti_istypeof<EMotionFX::AnimGraphTransitionCondition>(objectPrototype))
            {
                m_types.emplace(azrtti_typeid(objectPrototype), objectPrototype->GetPaletteName());
            }
        }
    }

    AddActionButton::AddActionButton(AnimGraphPlugin* plugin, QWidget* parent)
        : EMotionFX::TypeChoiceButton("Add action", "", parent)
    {
        const AZStd::vector<EMotionFX::AnimGraphObject*>& objectPrototypes = plugin->GetAnimGraphObjectFactory()->GetUiObjectPrototypes();
        AZStd::unordered_map<AZ::TypeId, AZStd::string> types;
        types.reserve(objectPrototypes.size());

        for (const EMotionFX::AnimGraphObject* objectPrototype : objectPrototypes)
        {
            if (azrtti_istypeof<EMotionFX::AnimGraphTriggerAction>(objectPrototype))
            {
                types.emplace(azrtti_typeid(objectPrototype), objectPrototype->GetPaletteName());
            }
        }

        SetTypes(types);
    }

    void AttributesWindow::CopyPasteClipboard::Clear()
    {
        m_conditions.clear();
        m_transition = AZ::Failure();
    }

} // namespace EMStudio
