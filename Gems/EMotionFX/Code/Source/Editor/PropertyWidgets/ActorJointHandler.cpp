/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/PropertyWidgets/ActorJointHandler.h>
#include <Editor/ActorEditorBus.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <Editor/AnimGraphEditorBus.h>
#include <Editor/JointSelectionDialog.h>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(ActorJointPicker, EditorAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(ActorSingleJointHandler, EditorAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(ActorMultiJointHandler, EditorAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(ActorMultiWeightedJointHandler, EditorAllocator)
    AZ_CLASS_ALLOCATOR_IMPL_TEMPLATE(ActorJointElementHandler, EditorAllocator)
    AZ_CLASS_ALLOCATOR_IMPL_TEMPLATE(ActorWeightedJointElementHandler, EditorAllocator)

    ActorJointPicker::ActorJointPicker(bool singleSelection, const QString& dialogTitle, const QString& dialogDescriptionLabelText, QWidget* parent)
        : QWidget(parent)
        , m_dialogTitle(dialogTitle)
        , m_dialogDescriptionLabelText(dialogDescriptionLabelText)
        , m_label(new QLabel())
        , m_pickButton(new QPushButton(QIcon(":/SceneUI/Manifest/TreeIcon.png"), ""))
        , m_resetButton(new QPushButton())
        , m_singleSelection(singleSelection)
    {
        connect(m_pickButton, &QPushButton::clicked, this, &ActorJointPicker::OnPickClicked);

        EMStudio::EMStudioManager::MakeTransparentButton(m_resetButton, "Images/Icons/Clear.svg", "Reset selection");
        connect(m_resetButton, &QPushButton::clicked, this, &ActorJointPicker::OnResetClicked);

        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->setMargin(0);
        hLayout->addWidget(m_label);
        hLayout->addStretch();
        hLayout->addWidget(m_pickButton);
        hLayout->addWidget(m_resetButton);
        setLayout(hLayout);
    }

    void ActorJointPicker::AddDefaultFilter(const QString& category, const QString& displayName)
    {
        m_defaultFilters.emplace_back(category, displayName);
    }

    void ActorJointPicker::OnResetClicked()
    {
        SetWeightedJointNames(AZStd::vector<AZStd::pair<AZStd::string, float> >());
        emit SelectionChanged();
    }

    void ActorJointPicker::SetJointName(const AZStd::string& jointName)
    {
        AZStd::vector<AZStd::pair<AZStd::string, float> > weightedJointNames;

        if (!jointName.empty())
        {
            weightedJointNames.emplace_back(jointName, 0.0f);
        }

        SetWeightedJointNames(weightedJointNames);
    }

    AZStd::string ActorJointPicker::GetJointName() const
    {
        if (m_weightedJointNames.empty())
        {
            return AZStd::string();
        }

        return m_weightedJointNames[0].first;
    }

    void ActorJointPicker::SetJointNames(const AZStd::vector<AZStd::string>& jointNames)
    {
        AZStd::vector<AZStd::pair<AZStd::string, float> > weightedJointNames;

        const size_t numJointNames = jointNames.size();
        weightedJointNames.resize(numJointNames);

        for (size_t i = 0; i < numJointNames; ++i)
        {
            weightedJointNames[i] = AZStd::make_pair(jointNames[i], 0.0f);
        }

        SetWeightedJointNames(weightedJointNames);
    }

    AZStd::vector<AZStd::string> ActorJointPicker::GetJointNames() const
    {
        AZStd::vector<AZStd::string> result;

        const size_t numJoints = m_weightedJointNames.size();
        result.resize(numJoints);

        for (size_t i = 0; i < numJoints; ++i)
        {
            result[i] = m_weightedJointNames[i].first;
        }

        return result;
    }

    void ActorJointPicker::UpdateInterface()
    {
        const size_t numJoints = m_weightedJointNames.size();
        if (numJoints == 1)
        {
            m_label->setText(QString::fromUtf8(m_weightedJointNames[0].first.c_str()));
        }
        else
        {
            m_label->setText(QString("%1 joint%2 selected").arg(QString::number(numJoints), numJoints == 1 ? QString() : "s"));
        }
        m_resetButton->setVisible(numJoints > 0);

        // Build and set the tooltip containing all joints.
        QString tooltip;
        for (size_t i = 0; i < numJoints; ++i)
        {
            tooltip += m_weightedJointNames[i].first.c_str();
            if (i != numJoints - 1)
            {
                tooltip += "\n";
            }
        }
        m_label->setToolTip(tooltip);
    }

    void ActorJointPicker::SetWeightedJointNames(const AZStd::vector<AZStd::pair<AZStd::string, float> >& weightedJointNames)
    {
        m_weightedJointNames = weightedJointNames;
        UpdateInterface();
    }

    AZStd::vector<AZStd::pair<AZStd::string, float> > ActorJointPicker::GetWeightedJointNames() const
    {
        return m_weightedJointNames;
    }

    void ActorJointPicker::OnPickClicked()
    {
        EMotionFX::ActorInstance* actorInstance = nullptr;
        ActorEditorRequestBus::BroadcastResult(actorInstance, &ActorEditorRequests::GetSelectedActorInstance);
        if (!actorInstance)
        {
            QMessageBox::warning(this, "No Actor Instance", "Cannot open joint selection window. No valid actor instance selected.");
            return;
        }

        // Create and show the joint picker window
        JointSelectionDialog jointSelectionWindow(m_singleSelection, m_dialogTitle, m_dialogDescriptionLabelText, this);

        for (const auto& filterPair : m_defaultFilters)
        {
            jointSelectionWindow.SetFilterState(filterPair.first, filterPair.second, true);
        }

        jointSelectionWindow.HideIcons();
        jointSelectionWindow.SelectByJointNames(GetJointNames());
        jointSelectionWindow.setModal(true);

        if (jointSelectionWindow.exec() != QDialog::Rejected)
        {
            SetJointNames(jointSelectionWindow.GetSelectedJointNames());
            emit SelectionChanged();
        }
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    template<class T>
    AZ::u32 ActorJointElementHandlerImpl<T>::GetHandlerName() const
    {
        return AZ_CRC_CE("ActorJointElement");
    }

    template<>
    AZ::u32 ActorJointElementHandlerImpl<AZStd::pair<AZStd::string, float>>::GetHandlerName() const
    {
        return AZ_CRC_CE("ActorWeightedJointElement");
    }

    template<class T>
    QWidget* ActorJointElementHandlerImpl<T>::CreateGUI(QWidget* parent)
    {
        AZ_UNUSED(parent);
        return nullptr;
    }

    template<class T>
    void ActorJointElementHandlerImpl<T>::WriteGUIValuesIntoProperty([[maybe_unused]] size_t index, [[maybe_unused]] QWidget* GUI, [[maybe_unused]] T& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
    }

    template<class T>
    bool ActorJointElementHandlerImpl<T>::ReadValuesIntoGUI([[maybe_unused]] size_t index, [[maybe_unused]] QWidget* GUI, [[maybe_unused]] const T& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        return true;
    }

    template class ActorJointElementHandlerImpl<AZStd::string>;
    template class ActorJointElementHandlerImpl<AZStd::pair<AZStd::string, float>>;

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    AZ::u32 ActorSingleJointHandler::GetHandlerName() const
    {
        return AZ_CRC_CE("ActorNode");
    }

    QWidget* ActorSingleJointHandler::CreateGUI(QWidget* parent)
    {
        ActorJointPicker* picker = aznew ActorJointPicker(true /*singleSelection*/, "Select Joint", "Select or double-click a joint", parent);

        connect(picker, &ActorJointPicker::SelectionChanged, this, [picker]()
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                    [picker](AzToolsFramework::PropertyEditorGUIMessages* handler)
                    {
                        handler->RequestWrite(picker);
                        handler->OnEditingFinished(picker);
                    });
        });

        return picker;
    }

    void ActorSingleJointHandler::ConsumeAttribute(ActorJointPicker* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, [[maybe_unused]] const char* debugName)
    {
        if (attrib == AZ::Edit::Attributes::ReadOnly)
        {
            bool value;
            if (attrValue->Read<bool>(value))
            {
                GUI->setEnabled(!value);
            }
        }
    }

    void ActorSingleJointHandler::WriteGUIValuesIntoProperty([[maybe_unused]] size_t index, ActorJointPicker* GUI, property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        instance = GUI->GetJointName();
    }

    bool ActorSingleJointHandler::ReadValuesIntoGUI([[maybe_unused]] size_t index, ActorJointPicker* GUI, const property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker signalBlocker(GUI);
        GUI->SetJointName(instance);
        return true;
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    AZ::u32 ActorMultiJointHandler::GetHandlerName() const
    {
        return AZ_CRC_CE("ActorNodes");
    }

    QWidget* ActorMultiJointHandler::CreateGUI(QWidget* parent)
    {
        ActorJointPicker* picker = aznew ActorJointPicker(false /*singleSelection*/, "Select Joints", "Select one or more joints from the skeleton", parent);

        connect(picker, &ActorJointPicker::SelectionChanged, this, [picker]()
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                    [picker](AzToolsFramework::PropertyEditorGUIMessages* handler)
                    {
                        handler->RequestWrite(picker);
                        handler->OnEditingFinished(picker);
                        handler->RequestRefresh(AzToolsFramework::Refresh_EntireTree);
                    });
        });

        return picker;
    }

    void ActorMultiJointHandler::ConsumeAttribute(ActorJointPicker* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, [[maybe_unused]] const char* debugName)
    {
        if (attrib == AZ::Edit::Attributes::ReadOnly)
        {
            bool value;
            if (attrValue->Read<bool>(value))
            {
                GUI->setEnabled(!value);
            }
        }
    }

    void ActorMultiJointHandler::WriteGUIValuesIntoProperty([[maybe_unused]] size_t index, ActorJointPicker* GUI, property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        instance = GUI->GetJointNames();
    }

    bool ActorMultiJointHandler::ReadValuesIntoGUI([[maybe_unused]] size_t index, ActorJointPicker* GUI, const property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker signalBlocker(GUI);
        GUI->SetJointNames(instance);
        return true;
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    AZ::u32 ActorMultiWeightedJointHandler::GetHandlerName() const
    {
        return AZ_CRC_CE("ActorWeightedNodes");
    }

    QWidget* ActorMultiWeightedJointHandler::CreateGUI(QWidget* parent)
    {
        ActorJointPicker* picker = aznew ActorJointPicker(false /*singleSelection*/, "Joint Selection Dialog", "Select one or more joints from the skeleton", parent);

        connect(picker, &ActorJointPicker::SelectionChanged, this, [picker]()
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                [picker](AzToolsFramework::PropertyEditorGUIMessages* handler)
                {
                    handler->RequestWrite(picker);
                    handler->OnEditingFinished(picker);
                    handler->RequestRefresh(AzToolsFramework::Refresh_EntireTree);
                });
        });

        return picker;
    }

    void ActorMultiWeightedJointHandler::ConsumeAttribute(ActorJointPicker* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, [[maybe_unused]] const char* debugName)
    {
        if (attrib == AZ::Edit::Attributes::ReadOnly)
        {
            bool value;
            if (attrValue->Read<bool>(value))
            {
                GUI->setEnabled(!value);
            }
        }
    }

    void ActorMultiWeightedJointHandler::WriteGUIValuesIntoProperty([[maybe_unused]] size_t index, ActorJointPicker* GUI, property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        instance = GUI->GetWeightedJointNames();
    }

    bool ActorMultiWeightedJointHandler::ReadValuesIntoGUI([[maybe_unused]] size_t index, ActorJointPicker* GUI, const property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker signalBlocker(GUI);
        GUI->SetWeightedJointNames(instance);
        return true;
    }
} // namespace EMotionFX
