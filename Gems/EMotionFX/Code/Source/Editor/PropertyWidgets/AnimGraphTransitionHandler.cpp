/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphActionManager.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphHierarchyWidget.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphModel.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphWidget.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendNodeSelectionWindow.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AttributesWindow.h>
#include <Editor/AnimGraphEditorBus.h>
#include <Editor/PropertyWidgets/AnimGraphTransitionHandler.h>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QMouseEvent>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphTransitionIdPicker, AZ::SystemAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphMultiTransitionIdHandler, AZ::SystemAllocator, 0)

    const QColor AnimGraphTransitionIdSelector::s_graphWindowBorderOverwriteColor = QColor(255, 133, 0);
    const float AnimGraphTransitionIdSelector::s_graphWindowBorderOverwriteWidth = 5.0f;

    AnimGraphTransitionIdSelector::AnimGraphTransitionIdSelector()
    {
    }

    AnimGraphTransitionIdSelector::~AnimGraphTransitionIdSelector()
    {
        ResetUI();
    }

    void AnimGraphTransitionIdSelector::StartSelection(AnimGraphStateMachine* stateMachine, const AZStd::vector<AZ::u64>& transitionIds)
    {
        EMStudio::EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::AnimGraphPlugin::CLASS_ID);
        EMStudio::AnimGraphPlugin* animGraphPlugin = static_cast<EMStudio::AnimGraphPlugin*>(plugin);
        if (animGraphPlugin)
        {
            animGraphPlugin->GetAttributesWindow()->Lock();
            animGraphPlugin->SetActionFilter(EMStudio::AnimGraphActionFilter::CreateDisallowAll());

            EMStudio::AnimGraphModel& model = animGraphPlugin->GetAnimGraphModel();
            QItemSelectionModel& selectionModel = model.GetSelectionModel();
            selectionModel.clear();

            for (const AZ::u64 id : transitionIds)
            {
                AnimGraphStateTransition* transition = stateMachine->FindTransitionById(id);
                const QModelIndex transitionModelIndex = model.FindFirstModelIndex(transition);
                if (transitionModelIndex.isValid())
                {
                    selectionModel.select(transitionModelIndex, QItemSelectionModel::Rows | QItemSelectionModel::Select);
                }
            }

            EMStudio::BlendGraphWidget* graphWidget = animGraphPlugin->GetGraphWidget();
            graphWidget->EnableBorderOverwrite(s_graphWindowBorderOverwriteColor, s_graphWindowBorderOverwriteWidth);
            graphWidget->SetTitleBarText("Select interrupting transitions");
        }

        m_isSelecting = true;
    }

    void AnimGraphTransitionIdSelector::StopSelection(AnimGraphStateTransition* transition)
    {
        EMStudio::EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::AnimGraphPlugin::CLASS_ID);
        EMStudio::AnimGraphPlugin* animGraphPlugin = static_cast<EMStudio::AnimGraphPlugin*>(plugin);
        if (animGraphPlugin)
        {
            // Reset selection to the current transition before unlocking the attributes window, so that we don't update it.
            EMStudio::AnimGraphModel& model = animGraphPlugin->GetAnimGraphModel();
            QItemSelectionModel& selectionModel = model.GetSelectionModel();
            selectionModel.clear();

            const QModelIndex transitionModelIndex = model.FindFirstModelIndex(transition);
            if (transitionModelIndex.isValid())
            {
                selectionModel.select(transitionModelIndex, QItemSelectionModel::Rows | QItemSelectionModel::Select);
            }

            ResetUI();
        }

        m_isSelecting = false;
    }

    void AnimGraphTransitionIdSelector::ResetUI()
    {
        EMStudio::EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::AnimGraphPlugin::CLASS_ID);
        EMStudio::AnimGraphPlugin* animGraphPlugin = static_cast<EMStudio::AnimGraphPlugin*>(plugin);
        if (animGraphPlugin)
        {
            EMStudio::AttributesWindow* attributesWindow = animGraphPlugin->GetAttributesWindow();
            EMStudio::BlendGraphWidget* graphWidget = animGraphPlugin->GetGraphWidget();

            attributesWindow->Unlock();
            animGraphPlugin->SetActionFilter(EMStudio::AnimGraphActionFilter());
            graphWidget->DisableBorderOverwrite();
            graphWidget->SetTitleBarText(QString());
        }
    }

    AnimGraphTransitionIdPicker::AnimGraphTransitionIdPicker(QWidget* parent)
        : QWidget(parent)
    {
        m_mainLayout = new QVBoxLayout();
        setLayout(m_mainLayout);

        EMStudio::EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::AnimGraphPlugin::CLASS_ID);
        EMStudio::AnimGraphPlugin* animGraphPlugin = static_cast<EMStudio::AnimGraphPlugin*>(plugin);
        if (animGraphPlugin)
        {
            connect(&animGraphPlugin->GetAnimGraphModel(), &QAbstractItemModel::rowsAboutToBeRemoved, this, &AnimGraphTransitionIdPicker::OnAboutToBeRemoved);
        }
    }

    void AnimGraphTransitionIdPicker::OnAboutToBeRemoved(const QModelIndex& parent, int first, int last)
    {
        if (!parent.isValid())
        {
            return;
        }

        EMStudio::EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::AnimGraphPlugin::CLASS_ID);
        EMStudio::AnimGraphPlugin* animGraphPlugin = static_cast<EMStudio::AnimGraphPlugin*>(plugin);
        if (animGraphPlugin)
        {
            EMStudio::AttributesWindow* attributesWindow = animGraphPlugin->GetAttributesWindow();
            for (int i = first; i <= last; ++i)
            {
                const QModelIndex modelIndex = parent.model()->index(i, 0, parent);
                if (modelIndex == attributesWindow->GetModelIndex())
                {
                    m_transitionSelector.ResetUI();
                    attributesWindow->Init();
                }
            }
        }
    }

    AnimGraphTransitionIdPicker::~AnimGraphTransitionIdPicker()
    {
    }

    void AnimGraphTransitionIdPicker::SetTransition(AnimGraphStateTransition* transition)
    {
        m_transition = transition;
        Reinit();
    }

    QString AnimGraphTransitionIdPicker::GetTransitionNameById(const AnimGraphConnectionId transitionId)
    {
        AnimGraphStateMachine* stateMachine = GetStateMachine();
        if (!stateMachine)
        {
            AZ_Error("EMotionFX", false, "Cannot get transition name as state machine is not valid.");
            return QString();
        }

        AnimGraphStateTransition* transition = stateMachine->FindTransitionById(transitionId);
        if (!transition)
        {
            AZ_Error("EMotionFX", false, "Cannot get transition name as transition cannot be found in state machine '%s'.", stateMachine->GetName());
            return QString();
        }

        return EMStudio::AnimGraphModel::GetTransitionName(transition);
    }

    void AnimGraphTransitionIdPicker::Reinit()
    {
        if (m_widget)
        {
            m_widget->deleteLater();
        }

        m_widget = new QWidget();
        m_mainLayout->addWidget(m_widget);

        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setMargin(0);
        m_widget->setLayout(vLayout);

        m_label = new QLabel();
        vLayout->addWidget(m_label);

        QGridLayout* transitionLayout = new QGridLayout();
        transitionLayout->setMargin(0);
        vLayout->addLayout(transitionLayout);
        {
            m_removeButtons.clear();
            int row = 0;
            for (const AZ::u64 id : m_transitionIds)
            {
                QLineEdit* transitionLineEdit = new QLineEdit();
                transitionLineEdit->setText(GetTransitionNameById(id));
                transitionLineEdit->setReadOnly(true);
                transitionLayout->addWidget(transitionLineEdit, row, 0);

                QPushButton* removeTransitionButton = new QPushButton();
                EMStudio::EMStudioManager::MakeTransparentButton(removeTransitionButton, "Images/Icons/Trash.svg", "Remove transition from list");
                connect(removeTransitionButton, &QPushButton::clicked, this, [this, id]()
                    {
                        m_transitionIds.erase(AZStd::remove(m_transitionIds.begin(), m_transitionIds.end(), id), m_transitionIds.end());
                        Reinit();
                        emit SelectionChanged();
                    });
                m_removeButtons.emplace_back(removeTransitionButton);

                transitionLayout->addWidget(removeTransitionButton, row, 1);

                row++;
            }
        }

        m_pickButton = new QPushButton(this);
        connect(m_pickButton, &QPushButton::clicked, this, &AnimGraphTransitionIdPicker::OnPickClicked);
        vLayout->addWidget(m_pickButton);

        UpdateInterface();
    }

    void AnimGraphTransitionIdPicker::UpdateInterface()
    {
        const size_t numTransitions = m_transitionIds.size();
        if (numTransitions == 0)
        {
            m_label->setText("All transitions");
        }
        else
        {
            m_label->setText(QString("%1 Transition%2").arg(QString::number(numTransitions), numTransitions != 1 ? "s" : ""));
        }

        const bool isSelecting = m_transitionSelector.IsSelecting();
        if (isSelecting)
        {
            m_pickButton->setText("Leave selection mode");
        }
        else
        {
            m_pickButton->setText("Select transitions");
        }

        QString tooltip;
        for (const AZ::u64 id : m_transitionIds)
        {
            if (!tooltip.isEmpty())
            {
                tooltip += "\n";
            }

            tooltip += QString("%1").arg(GetTransitionNameById(id));
        }
        m_label->setToolTip(tooltip);
        m_pickButton->setToolTip(tooltip);

        for (QPushButton* removeTransitionButton : m_removeButtons)
        {
            removeTransitionButton->setDisabled(isSelecting);
        }
    }

    void AnimGraphTransitionIdPicker::SetTransitionIds(const AZStd::vector<AZ::u64>& transitionIds)
    {
        m_transitionIds = transitionIds;
        Reinit();
    }

    const AZStd::vector<AZ::u64>& AnimGraphTransitionIdPicker::GetTransitionIds() const
    {
        return m_transitionIds;
    }

    AnimGraphStateMachine* AnimGraphTransitionIdPicker::GetStateMachine() const
    {
        if (!m_transition)
        {
            AZ_Error("EMotionFX", false, "Expecting valid state machine.");
            return nullptr;
        }

        return m_transition->GetStateMachine();
    }

    void AnimGraphTransitionIdPicker::OnPickClicked()
    {
        if (!m_transition)
        {
            return;
        }

        EMStudio::EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::AnimGraphPlugin::CLASS_ID);
        EMStudio::AnimGraphPlugin* animGraphPlugin = static_cast<EMStudio::AnimGraphPlugin*>(plugin);
        if (!animGraphPlugin)
        {
            return;
        }

        if (!m_transitionSelector.IsSelecting())
        {
            AnimGraphStateMachine* stateMachine = GetStateMachine();
            if (stateMachine)
            {
                m_transitionSelector.StartSelection(stateMachine, m_transitionIds);
                UpdateInterface();
            }
        }
        else
        {
            m_transitionIds.clear();

            AZStd::unordered_map<EMotionFX::AnimGraph*, AZStd::vector<EMotionFX::AnimGraphStateTransition*> > selectedTransitionByAnimGraph = animGraphPlugin->GetAnimGraphModel().GetSelectedObjectsOfType<EMotionFX::AnimGraphStateTransition>();
            const AZStd::vector<EMotionFX::AnimGraphStateTransition*>& selectedTransitions = selectedTransitionByAnimGraph[m_transition->GetAnimGraph()];
            const AnimGraphNode* sourceState = m_transition->GetSourceNode();

            for (const EMotionFX::AnimGraphStateTransition* transition : selectedTransitions)
            {
                // Only add transitions that share the same source state or wildcards.
                if (transition != m_transition &&
                    (transition->GetSourceNode() == sourceState || transition->GetIsWildcardTransition() || m_transition->GetIsWildcardTransition()))
                {
                    m_transitionIds.emplace_back(transition->GetId());
                }
            }

            emit SelectionChanged();
            m_transitionSelector.StopSelection(m_transition);
            UpdateInterface();
        }

        Reinit();
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    AnimGraphMultiTransitionIdHandler::AnimGraphMultiTransitionIdHandler()
        : QObject()
        , AzToolsFramework::PropertyHandler<AZStd::vector<AZ::u64>, AnimGraphTransitionIdPicker>()
    {
    }

    AZ::u32 AnimGraphMultiTransitionIdHandler::GetHandlerName() const
    {
        return AZ_CRC("AnimGraphStateTransitionIds", 0x7b2468f7);
    }

    QWidget* AnimGraphMultiTransitionIdHandler::CreateGUI(QWidget* parent)
    {
        AnimGraphTransitionIdPicker* picker = aznew AnimGraphTransitionIdPicker(parent);

        connect(picker, &AnimGraphTransitionIdPicker::SelectionChanged, this, [picker]()
            {
                EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, picker);
            });

        return picker;
    }

    void AnimGraphMultiTransitionIdHandler::ConsumeAttribute(AnimGraphTransitionIdPicker* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, [[maybe_unused]] const char* debugName)
    {
        if (attrValue)
        {
            AnimGraphStateTransition* transition = static_cast<AnimGraphStateTransition*>(attrValue->GetInstancePointer());
            m_transition = transition;
            GUI->SetTransition(m_transition);
        }

        if (attrib == AZ::Edit::Attributes::ReadOnly)
        {
            bool value;
            if (attrValue->Read<bool>(value))
            {
                GUI->setEnabled(!value);
            }
        }
    }

    void AnimGraphMultiTransitionIdHandler::WriteGUIValuesIntoProperty([[maybe_unused]] size_t index, AnimGraphTransitionIdPicker* GUI, property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        instance = GUI->GetTransitionIds();
    }

    bool AnimGraphMultiTransitionIdHandler::ReadValuesIntoGUI([[maybe_unused]] size_t index, AnimGraphTransitionIdPicker* GUI, const property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker signalBlocker(GUI);
        GUI->SetTransitionIds(instance);
        return true;
    }
} // namespace EMotionFX
