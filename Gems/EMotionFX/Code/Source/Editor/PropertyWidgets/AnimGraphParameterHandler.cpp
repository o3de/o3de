/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/PropertyWidgets/AnimGraphParameterHandler.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/ObjectAffectedByParameterChanges.h>
#include <EMotionFX/Source/Parameter/BoolParameter.h>
#include <EMotionFX/Source/Parameter/FloatSliderParameter.h>
#include <EMotionFX/Source/Parameter/FloatSpinnerParameter.h>
#include <EMotionFX/Source/Parameter/IntSliderParameter.h>
#include <EMotionFX/Source/Parameter/IntSpinnerParameter.h>
#include <EMotionFX/Source/Parameter/TagParameter.h>
#include <EMotionFX/Source/Parameter/Vector2Parameter.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterSelectionWindow.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <QHBoxLayout>
#include <QMessageBox>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphParameterPicker, EditorAllocator);
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphSingleParameterHandler, EditorAllocator);
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphMultipleParameterHandler, EditorAllocator);

    AnimGraphParameterPicker::AnimGraphParameterPicker(QWidget* parent, bool singleSelection, bool parameterMaskMode)
        : QWidget(parent)
        , m_animGraph(nullptr)
        , m_affectedByParameterChanges(nullptr)
        , m_pickButton(nullptr)
        , m_resetButton(nullptr)
        , m_shrinkButton(nullptr)
        , m_singleSelection(singleSelection)
        , m_parameterMaskMode(parameterMaskMode)
    {
        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->setMargin(0);

        m_pickButton = new QPushButton(this);
        connect(m_pickButton, &QPushButton::clicked, this, &AnimGraphParameterPicker::OnPickClicked);
        hLayout->addWidget(m_pickButton);

        m_resetButton = new QPushButton(this);
        EMStudio::EMStudioManager::MakeTransparentButton(m_resetButton, "Images/Icons/Clear.svg", "Reset selection");
        connect(m_resetButton, &QPushButton::clicked, this, &AnimGraphParameterPicker::OnResetClicked);
        hLayout->addWidget(m_resetButton);

        if (m_parameterMaskMode)
        {
            m_shrinkButton = new QPushButton();
            EMStudio::EMStudioManager::MakeTransparentButton(m_shrinkButton, "Images/Icons/Cut.svg", "Shrink the parameter mask to the ports that are actually connected.");
            connect(m_shrinkButton, &QPushButton::clicked, this, &AnimGraphParameterPicker::OnShrinkClicked);
            hLayout->addWidget(m_shrinkButton);
        }

        setLayout(hLayout);
        UpdateInterface();
    }

    void AnimGraphParameterPicker::OnResetClicked()
    {
        if (m_parameterNames.empty())
        {
            return;
        }
        UpdateParameterNames(AZStd::vector<AZStd::string>());
    }


    void AnimGraphParameterPicker::OnShrinkClicked()
    {
        if (!m_affectedByParameterChanges)
        {
            AZ_Error("EMotionFX", false, "Cannot shrink parameter mask. No valid parameter picker rule.");
            return;
        }

        AZStd::vector<AZStd::string> parameterNames;
        if (m_affectedByParameterChanges)
        {
            m_affectedByParameterChanges->AddRequiredParameters(parameterNames);
        }
        UpdateParameterNames(parameterNames);
    }


    void AnimGraphParameterPicker::UpdateInterface()
    {
        // Set the picker button name based on the number of nodes.
        const size_t numParameters = m_parameterNames.size();
        if (numParameters == 0)
        {
            if (m_singleSelection)
            {
                m_pickButton->setText("Select parameter");
            }
            else
            {
                m_pickButton->setText("Select parameters");
            }

            m_resetButton->setVisible(false);
        }
        else if (numParameters == 1)
        {
            m_pickButton->setText(m_parameterNames[0].c_str());
            m_resetButton->setVisible(true);
        }
        else
        {
            m_pickButton->setText(QString("%1 parameters").arg(numParameters));
            m_resetButton->setVisible(true);
        }

        // Build and set the tooltip containing all nodes.
        QString tooltip;
        for (size_t i = 0; i < numParameters; ++i)
        {
            tooltip += m_parameterNames[i].c_str();
            if (i != numParameters - 1)
            {
                tooltip += "\n";
            }
        }
        m_pickButton->setToolTip(tooltip);
    }

    void AnimGraphParameterPicker::SetObjectAffectedByParameterChanges(ObjectAffectedByParameterChanges* affectedObject)
    {
        m_affectedByParameterChanges = affectedObject;
        m_parameterNames = m_affectedByParameterChanges->GetParameters();
        UpdateInterface();
    }

    void AnimGraphParameterPicker::InitializeParameterNames(const AZStd::vector<AZStd::string>& parameterNames)
    {
        if (m_parameterNames != parameterNames)
        {
            m_parameterNames = parameterNames;
            UpdateInterface();
        }
    }

    void AnimGraphParameterPicker::UpdateParameterNames(const AZStd::vector<AZStd::string>& parameterNames)
    {
        if (m_parameterNames != parameterNames)
        {
            m_parameterNames = parameterNames;
            
            if (m_affectedByParameterChanges)
            {
                m_affectedByParameterChanges->ParameterMaskChanged(m_parameterNames);
            }
            emit ParametersChanged(m_parameterNames);
            
            UpdateInterface();
        }
    }

    const AZStd::vector<AZStd::string>& AnimGraphParameterPicker::GetParameterNames() const
    {
        return m_parameterNames;
    }


    void AnimGraphParameterPicker::SetSingleParameterName(const AZStd::string& parameterName)
    {
        AZStd::vector<AZStd::string> parameterNames;

        if (!parameterName.empty())
        {
            parameterNames.emplace_back(parameterName);   
        }

        InitializeParameterNames(parameterNames);
    }


    const AZStd::string AnimGraphParameterPicker::GetSingleParameterName() const
    {
        if (m_parameterNames.empty())
        {
            return AZStd::string();
        }

        return m_parameterNames[0];
    }


    void AnimGraphParameterPicker::OnPickClicked()
    {
        AnimGraph* animGraph = m_animGraph;
        if (m_affectedByParameterChanges)
        {
            animGraph = m_affectedByParameterChanges->GetParameterAnimGraph();
        }

        if (!animGraph)
        {
            AZ_Error("EMotionFX", false, "Cannot open anim graph parameter selection window. No valid anim graph.");
            return;
        }

        // Create and show the node picker window
        EMStudio::ParameterSelectionWindow selectionWindow(this, m_singleSelection);
        selectionWindow.GetParameterWidget()->SetFilterTypes(m_filterTypes);
        selectionWindow.Update(animGraph, m_parameterNames);
        selectionWindow.setModal(true);

        if (selectionWindow.exec() != QDialog::Rejected)
        {
            AZStd::vector<AZStd::string> parameterNames = selectionWindow.GetParameterWidget()->GetSelectedParameters();

            if (m_parameterMaskMode)
            {
                if (m_affectedByParameterChanges)
                {
                    m_affectedByParameterChanges->AddRequiredParameters(parameterNames);
                }
            }

            UpdateParameterNames(parameterNames);
        }
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    AnimGraphSingleParameterHandler::AnimGraphSingleParameterHandler()
        : QObject()
        , AzToolsFramework::PropertyHandler<AZStd::string, AnimGraphParameterPicker>()
        , m_animGraph(nullptr)
    {
    }

    AZ::u32 AnimGraphSingleParameterHandler::GetHandlerName() const
    {
        return AZ_CRC_CE("AnimGraphParameter");
    }


    QWidget* AnimGraphSingleParameterHandler::CreateGUI(QWidget* parent)
    {
        AnimGraphParameterPicker* picker = aznew AnimGraphParameterPicker(parent, true);

        connect(picker, &AnimGraphParameterPicker::ParametersChanged, this, [picker]([[maybe_unused]] const AZStd::vector<AZStd::string>& newParameters)
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, picker);
        });

        return picker;
    }


    void AnimGraphSingleParameterHandler::ConsumeAttribute(AnimGraphParameterPicker* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, [[maybe_unused]] const char* debugName)
    {
        if (attrib == AZ::Edit::Attributes::ReadOnly)
        {
            bool value;
            if (attrValue->Read<bool>(value))
            {
                GUI->setEnabled(!value);
            }
        }

        if (attrib == AZ_CRC_CE("AnimGraph"))
        {
            attrValue->Read<AnimGraph*>(m_animGraph);
            GUI->SetAnimGraph(m_animGraph);
        }
    }


    void AnimGraphSingleParameterHandler::WriteGUIValuesIntoProperty([[maybe_unused]] size_t index, AnimGraphParameterPicker* GUI, property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        instance = GUI->GetSingleParameterName();
    }


    bool AnimGraphSingleParameterHandler::ReadValuesIntoGUI([[maybe_unused]] size_t index, AnimGraphParameterPicker* GUI, const property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker signalBlocker(GUI);
        GUI->SetSingleParameterName(instance);
        return true;
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphSingleNumberParameterHandler, EditorAllocator);

    AnimGraphSingleNumberParameterHandler::AnimGraphSingleNumberParameterHandler()
        : AnimGraphSingleParameterHandler()
    {
    }

    AZ::u32 AnimGraphSingleNumberParameterHandler::GetHandlerName() const
    {
        return AZ_CRC_CE("AnimGraphNumberParameter");
    }

    QWidget* AnimGraphSingleNumberParameterHandler::CreateGUI(QWidget* parent)
    {
        AnimGraphParameterPicker* picker = aznew AnimGraphParameterPicker(parent, true);
        picker->SetFilterTypes({
            azrtti_typeid<BoolParameter>(),
            azrtti_typeid<FloatSliderParameter>(),
            azrtti_typeid<FloatSpinnerParameter>(),
            azrtti_typeid<IntSliderParameter>(),
            azrtti_typeid<IntSpinnerParameter>(),
            azrtti_typeid<TagParameter>()});

        connect(picker, &AnimGraphParameterPicker::ParametersChanged, this, [picker]([[maybe_unused]] const AZStd::vector<AZStd::string>& newParameters)
            {
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                    &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, picker);
            });

        return picker;
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphSingleVector2ParameterHandler, EditorAllocator);

    AnimGraphSingleVector2ParameterHandler::AnimGraphSingleVector2ParameterHandler()
        : AnimGraphSingleParameterHandler()
    {
    }

    AZ::u32 AnimGraphSingleVector2ParameterHandler::GetHandlerName() const
    {
        return AZ_CRC_CE("AnimGraphVector2Parameter");
    }

    QWidget* AnimGraphSingleVector2ParameterHandler::CreateGUI(QWidget* parent)
    {
        AnimGraphParameterPicker* picker = aznew AnimGraphParameterPicker(parent, true);
        picker->SetFilterTypes({azrtti_typeid<Vector2Parameter>()});

        connect(picker, &AnimGraphParameterPicker::ParametersChanged, this, [picker]([[maybe_unused]] const AZStd::vector<AZStd::string>& newParameters) {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, picker);
        });

        return picker;
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    AnimGraphMultipleParameterHandler::AnimGraphMultipleParameterHandler()
        : QObject()
        , AzToolsFramework::PropertyHandler<AZStd::vector<AZStd::string>, AnimGraphParameterPicker>()
        , m_animGraph(nullptr)
    {
    }

    AZ::u32 AnimGraphMultipleParameterHandler::GetHandlerName() const
    {
        return AZ_CRC_CE("AnimGraphMultipleParameter");
    }


    QWidget* AnimGraphMultipleParameterHandler::CreateGUI(QWidget* parent)
    {
        AnimGraphParameterPicker* picker = aznew AnimGraphParameterPicker(parent, false);

        connect(picker, &AnimGraphParameterPicker::ParametersChanged, this, [picker]([[maybe_unused]] const AZStd::vector<AZStd::string>& newParameters)
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, picker);
        });

        return picker;
    }


    void AnimGraphMultipleParameterHandler::ConsumeAttribute(AnimGraphParameterPicker* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, [[maybe_unused]] const char* debugName)
    {
        if (attrib == AZ::Edit::Attributes::ReadOnly)
        {
            bool value;
            if (attrValue->Read<bool>(value))
            {
                GUI->setEnabled(!value);
            }
        }

        if (attrib == AZ_CRC_CE("AnimGraph"))
        {
            attrValue->Read<AnimGraph*>(m_animGraph);
            GUI->SetAnimGraph(m_animGraph);
        }
    }


    void AnimGraphMultipleParameterHandler::WriteGUIValuesIntoProperty([[maybe_unused]] size_t index, AnimGraphParameterPicker* GUI, property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        instance = GUI->GetParameterNames();
    }


    bool AnimGraphMultipleParameterHandler::ReadValuesIntoGUI([[maybe_unused]] size_t index, AnimGraphParameterPicker* GUI, const property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker signalBlocker(GUI);
        GUI->InitializeParameterNames(instance);
        return true;
    }
} // namespace EMotionFX
