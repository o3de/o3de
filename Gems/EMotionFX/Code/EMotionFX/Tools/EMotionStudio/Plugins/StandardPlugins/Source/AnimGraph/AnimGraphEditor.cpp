/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphEditor.h>
#include <MysticQt/Source/MysticQtManager.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <QComboBox>
#include <QVBoxLayout>
#include <QVariant>
#include <QPushButton>
#include <QLabel>
#include <QPixmap>
#include <QSpacerItem>
#include <QPushButton>


namespace EMotionFX
{
    const int AnimGraphEditor::s_propertyLabelWidth = 120;
    QString AnimGraphEditor::s_lastMotionSetText = "";

    AnimGraphEditor::AnimGraphEditor(EMotionFX::AnimGraph* animGraph, AZ::SerializeContext* serializeContext, QWidget* parent)
        : QWidget(parent)
        , m_animGraph(animGraph)
        , m_propertyEditor(nullptr)
        , m_motionSetComboBox(nullptr)
    {
        QHBoxLayout* mainLayout = new QHBoxLayout();
        QLabel* iconLabel = new QLabel(this);
        iconLabel->setPixmap(QPixmap(":/EMotionFX/AnimGraphComponent.svg").scaled(QSize(32, 32), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        mainLayout->addWidget(iconLabel, 0, Qt::Alignment(Qt::AlignLeft | Qt::AlignTop));
        
        QVBoxLayout* vLayout = new QVBoxLayout();
        QHBoxLayout* filenameLayout = new QHBoxLayout();
        filenameLayout->setMargin(2);
        vLayout->addLayout(filenameLayout);
        m_filenameLabel = new QLabel();
        m_filenameLabel->setStyleSheet("font-weight: bold;");
        filenameLayout->addWidget(m_filenameLabel, 0, Qt::AlignTop);

        if (animGraph)
        {
            AZStd::string fullFilename;
            AzFramework::StringFunc::Path::GetFullFileName(animGraph->GetFileName(), fullFilename);

            if (fullFilename.empty())
            {
                fullFilename = "<Unsaved Animgraph>";
            }

            m_filenameLabel->setText(fullFilename.c_str());
        }

        m_propertyEditor = aznew AzToolsFramework::ReflectedPropertyEditor(this);
        m_propertyEditor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        m_propertyEditor->setObjectName("PropertyEditor");
        m_propertyEditor->Setup(serializeContext, nullptr, false/*enableScrollbars*/, s_propertyLabelWidth);
        m_propertyEditor->SetSizeHintOffset(QSize(0, 0));
        m_propertyEditor->SetAutoResizeLabels(false);
        m_propertyEditor->SetLeafIndentation(0);
        m_propertyEditor->setStyleSheet("QFrame, .QWidget, QSlider, QCheckBox { background-color: transparent }");
        vLayout->addWidget(m_propertyEditor, 0, Qt::AlignLeft);

        SetAnimGraph(animGraph);

        // Motion set combo box
        QHBoxLayout* motionSetLayout = new QHBoxLayout();
        motionSetLayout->setMargin(2);
        motionSetLayout->setSpacing(0);
        vLayout->addLayout(motionSetLayout);

        QLabel* motionSetLabel = new QLabel("Preview with");
        motionSetLayout->addWidget(motionSetLabel);
        motionSetLabel->setFixedWidth(120);

        m_motionSetComboBox = new QComboBox();
        m_motionSetComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        //initializes to last selection if it is there
        if (!s_lastMotionSetText.isEmpty())
        {
            m_motionSetComboBox->addItem(s_lastMotionSetText);
            m_motionSetComboBox->setCurrentIndex(0);
        }
        connect(m_motionSetComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &AnimGraphEditor::OnMotionSetChanged);
        motionSetLayout->addWidget(m_motionSetComboBox);

        mainLayout->addLayout(vLayout);
        mainLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
        setLayout(mainLayout);

        UpdateMotionSetComboBox();

        EMotionFX::AnimGraphEditorRequestBus::Handler::BusConnect();

        m_commandCallbacks.emplace_back(new UpdateMotionSetComboBoxCallback(false));
        CommandSystem::GetCommandManager()->RegisterCommandCallback("ActivateAnimGraph", m_commandCallbacks.back());
        m_commandCallbacks.emplace_back(new UpdateMotionSetComboBoxCallback(false));
        CommandSystem::GetCommandManager()->RegisterCommandCallback("RemoveAnimGraph", m_commandCallbacks.back());
        m_commandCallbacks.emplace_back(new UpdateMotionSetComboBoxCallback(false));
        CommandSystem::GetCommandManager()->RegisterCommandCallback("RemoveActorInstance", m_commandCallbacks.back());
    }


    AnimGraphEditor::~AnimGraphEditor()
    {
        for (MCore::Command::Callback* callback : m_commandCallbacks)
        {
            CommandSystem::GetCommandManager()->RemoveCommandCallback(callback, true);
        }
        m_commandCallbacks.clear();

        EMotionFX::AnimGraphEditorRequestBus::Handler::BusDisconnect();
    }


    EMotionFX::MotionSet* AnimGraphEditor::GetSelectedMotionSet()
    {
        const AZ::Outcome<size_t> motionSetIndex = GetMotionSetIndex(m_motionSetComboBox->currentIndex());
        if (motionSetIndex.IsSuccess())
        {
            return EMotionFX::GetMotionManager().GetMotionSet(motionSetIndex.GetValue());
        }

        return nullptr;
    }


    void AnimGraphEditor::UpdateMotionSetComboBox()
    {
        // block signal to avoid event when the current selected item changed
        m_motionSetComboBox->blockSignals(true);
        m_motionSetComboBox->setStyleSheet("");

        // get the current selected item to set it back in case no one actor instance is selected
        // if actor instances are selected, the used motion set of the anim graph is set if not multiple used or if no one used
        const QString currentSelectedItem = m_motionSetComboBox->currentText();

        // clear the motion set combobox
        m_motionSetComboBox->clear();

        // add each motion set name
        const size_t numMotionSets = EMotionFX::GetMotionManager().GetNumMotionSets();
        for (size_t i = 0; i < numMotionSets; ++i)
        {
            EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().GetMotionSet(i);
            if (motionSet->GetIsOwnedByRuntime())
            {
                continue;
            }

            m_motionSetComboBox->addItem(motionSet->GetName());
        }

        // get the current selection list and the number of actor instances selected
        const CommandSystem::SelectionList& selectionList = CommandSystem::GetCommandManager()->GetCurrentSelection();
        const size_t numActorInstances = selectionList.GetNumSelectedActorInstances();

        // if actor instances are selected, set the used motion set
        if (numActorInstances > 0)
        {
            // add each used motion set in the array (without duplicate)
            // this is used to check if multiple motion sets are used
            AZStd::vector<EMotionFX::MotionSet*> usedMotionSets;
            AZStd::vector<EMotionFX::AnimGraphInstance*> usedAnimGraphs;
            for (size_t i = 0; i < numActorInstances; ++i)
            {
                EMotionFX::ActorInstance* actorInstance = selectionList.GetActorInstance(i);
                if (actorInstance->GetIsOwnedByRuntime())
                {
                    continue;
                }

                EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();
                if (animGraphInstance)
                {
                    EMotionFX::MotionSet* motionSet = animGraphInstance->GetMotionSet();
                    if (motionSet && AZStd::find(usedMotionSets.begin(), usedMotionSets.end(), motionSet) == usedMotionSets.end())
                    {
                        usedMotionSets.push_back(motionSet);
                    }

                    if (animGraphInstance && AZStd::find(usedAnimGraphs.begin(), usedAnimGraphs.end(), animGraphInstance) == usedAnimGraphs.end())
                    {
                        usedAnimGraphs.push_back(animGraphInstance);
                    }
                }
            }

            // set the text based on the used motion sets
            if (usedMotionSets.empty())
            {
                // the used motion set array can be empty if no one anim graph activated on the selected actor instances
                // if the current motion set name can be found use it else use the first item
                // the current motion set name can not be found if "<multiple used>" was used
                if (m_motionSetComboBox->findText(currentSelectedItem) == -1)
                {
                    m_motionSetComboBox->setCurrentIndex(0);
                }
                else
                {
                    m_motionSetComboBox->setCurrentText(currentSelectedItem);
                }

                if (m_motionSetComboBox->count() == 1)
                {
                    m_motionSetComboBox->setCurrentIndex(0);
                }
                else if (m_motionSetComboBox->count() == 0)
                {
                    m_motionSetComboBox->addItem("Select a motion set");
                    m_motionSetComboBox->setCurrentIndex(0);
                }

                // enable the combo box in case it was disabled before
                m_motionSetComboBox->setEnabled(true);

                if (usedAnimGraphs.size() > 0)
                {
                    m_motionSetComboBox->setStyleSheet("border: 1px solid orange;");
                }
            }
            else if (usedMotionSets.size() == 1)
            {
                // if the motion set used is not valid use the first item else use the motion set name
                if (usedMotionSets[0])
                {
                    m_motionSetComboBox->setCurrentText(usedMotionSets[0]->GetName());
                }
                else
                {
                    m_motionSetComboBox->setCurrentIndex(0);

                    if (usedAnimGraphs.size() > 0)
                    {
                        m_motionSetComboBox->setStyleSheet("border: 1px solid orange;");
                    }
                }

                // enable the combo box in case it was disabled before
                m_motionSetComboBox->setEnabled(true);
            }
            else
            {
                // clear all items and add only "<multiple used"
                m_motionSetComboBox->clear();
                m_motionSetComboBox->addItem("<multiple used>");

                // disable the combobox to avoid user action
                m_motionSetComboBox->setDisabled(true);
            }
        }
        else // no one actor instance selected, set the old text
        {
            // if the current motion set name can be found use it else use the first item
            // the current motion set name can not be found if "<multiple used>" was used
            if (m_motionSetComboBox->findText(currentSelectedItem) == -1)
            {
                m_motionSetComboBox->setCurrentIndex(0);
            }
            else
            {
                m_motionSetComboBox->setCurrentText(currentSelectedItem);
            }

            if (m_motionSetComboBox->count() == 1)
            {
                m_motionSetComboBox->setCurrentIndex(0);
            }
            else if (m_motionSetComboBox->count() == 0)
            {
                m_motionSetComboBox->addItem("Select a motion set");
                m_motionSetComboBox->setCurrentIndex(0);
            }

            // enable the combo box in case it was disabled before
            m_motionSetComboBox->setEnabled(true);
        }

        // enable signals
        m_motionSetComboBox->blockSignals(false);
        // Set ComboBox Name
        m_motionSetComboBox->setObjectName("EMFX.AttributesWindowWidget.AnimGraph.MotionSetComboBox");
    }

    QComboBox* AnimGraphEditor::GetMotionSetComboBox() const
    {
        return m_motionSetComboBox;
    }

    void AnimGraphEditor::OnMotionSetChanged(int index)
    {
        // get the current selection list and the number of actor instances selected
        const CommandSystem::SelectionList& selectionList = CommandSystem::GetCommandManager()->GetCurrentSelection();
        const size_t numActorInstances = selectionList.GetNumSelectedActorInstances();

        AnimGraphEditor::s_lastMotionSetText = m_motionSetComboBox->itemText(index);
        // if no one actor instance is selected, the combo box has no effect
        if (numActorInstances == 0)
        {
            return;
        }

        const AZ::Outcome<size_t> motionSetIndex = GetMotionSetIndex(index);

        EMotionFX::MotionSet* motionSet = nullptr;
        if (motionSetIndex.IsSuccess())
        {
            motionSet = EMotionFX::GetMotionManager().GetMotionSet(motionSetIndex.GetValue());
        }

        // create the command group
        MCore::CommandGroup commandGroup("Change motion set");

        // update the motion set on each actor instance if one anim graph is activated
        AZStd::string commandString;
        for (size_t i = 0; i < numActorInstances; ++i)
        {
            // get the actor instance from the selection list and the anim graph instance
            EMotionFX::ActorInstance* actorInstance = selectionList.GetActorInstance(i);

            if (actorInstance->GetIsOwnedByRuntime())
            {
                continue;
            }

            EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();

            // check if one anim graph instance is set on the actor instance
            if (animGraphInstance)
            {
                // check if the motion set is not the same
                if (animGraphInstance->GetMotionSet() != motionSet)
                {
                    // get the current anim graph
                    EMotionFX::AnimGraph* animGraph = animGraphInstance->GetAnimGraph();

                    // add the command in the command group
                    if (motionSet)
                    {
                        commandString = AZStd::string::format("ActivateAnimGraph -actorInstanceID %d -animGraphID %d -motionSetID %d",
                            actorInstance->GetID(),
                            animGraph->GetID(),
                            motionSet->GetID());
                    }
                    else
                    {
                        commandString = AZStd::string::format("ActivateAnimGraph -actorInstanceID %d -animGraphID %d",
                            actorInstance->GetID(),
                            animGraph->GetID());
                    }

                    commandGroup.AddCommandString(commandString);
                }
            }
        }

        AZStd::string result;
        if (!CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    void AnimGraphEditor::SetAnimGraph(AnimGraph* animGraph)
    {
        if (animGraph != m_animGraph)
        {
            m_propertyEditor->ClearInstances();

            if (animGraph)
            {
                AZStd::string fullFilename;
                AzFramework::StringFunc::Path::GetFullFileName(animGraph->GetFileName(), fullFilename);

                if (fullFilename.empty())
                {
                    fullFilename = "<Unsaved Animgraph>";
                }

                m_filenameLabel->setText(fullFilename.c_str());

                const AZ::TypeId& typeId = azrtti_typeid(animGraph);
                m_propertyEditor->AddInstance(animGraph, typeId);
                m_propertyEditor->show();
                m_propertyEditor->ExpandAll();
                m_propertyEditor->InvalidateAll();
            }

            m_animGraph = animGraph;
        }
    }

    AZ::Outcome<size_t> AnimGraphEditor::GetMotionSetIndex(int comboBoxIndex) const
    {
        const size_t targetEditorMotionSetIndex = comboBoxIndex;
        size_t currentEditorMotionSet = 0;
        const size_t numMotionSets = EMotionFX::GetMotionManager().GetNumMotionSets();
        for (size_t i = 0; i < numMotionSets; ++i)
        {
            const EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().GetMotionSet(i);

            if (motionSet->GetIsOwnedByRuntime())
            {
                continue;
            }

            if (currentEditorMotionSet == targetEditorMotionSetIndex)
            {
                return AZ::Success(i);
            }

            currentEditorMotionSet++;
        }

        return AZ::Failure();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    bool AnimGraphEditor::UpdateMotionSetComboBoxCallback::Execute([[maybe_unused]] MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)
    {
        EMotionFX::AnimGraphEditorRequestBus::Broadcast(&EMotionFX::AnimGraphEditorRequests::UpdateMotionSetComboBox);
        return true;
    }

    bool AnimGraphEditor::UpdateMotionSetComboBoxCallback::Undo([[maybe_unused]] MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)
    {
        EMotionFX::AnimGraphEditorRequestBus::Broadcast(&EMotionFX::AnimGraphEditorRequests::UpdateMotionSetComboBox);
        return true;
    }
} // namespace EMotionFX
