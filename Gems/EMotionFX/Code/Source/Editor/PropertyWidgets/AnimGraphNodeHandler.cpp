/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/PropertyWidgets/AnimGraphNodeHandler.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphHierarchyWidget.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendNodeSelectionWindow.h>
#include <Editor/AnimGraphEditorBus.h>
#include <QHBoxLayout>
#include <QMessageBox>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphNodeIdPicker, EditorAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphNodeIdHandler, EditorAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphMotionNodeIdHandler, EditorAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphStateIdHandler, EditorAllocator)

    AnimGraphNodeIdPicker::AnimGraphNodeIdPicker(QWidget* parent)
        : QWidget(parent)
        , m_animGraph(nullptr)
        , m_nodeFilterType(AZ::TypeId::CreateNull())
        , m_showStatesOnly(false)
    {
        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->setMargin(0);

        m_pickButton = new QPushButton(this);
        connect(m_pickButton, &QPushButton::clicked, this, &AnimGraphNodeIdPicker::OnPickClicked);
        hLayout->addWidget(m_pickButton);

        setLayout(hLayout);
    }


    void AnimGraphNodeIdPicker::SetAnimGraph(AnimGraph* animGraph)
    {
        m_animGraph = animGraph;
        UpdateInterface();
    }


    void AnimGraphNodeIdPicker::UpdateInterface()
    {
        if (!m_nodeId.IsValid())
        {
            m_pickButton->setText("Select node");
        }
        else
        {
            if (m_animGraph)
            {
                AnimGraphNode* node = m_animGraph->RecursiveFindNodeById(m_nodeId);
                if (node)
                {
                    m_pickButton->setText(node->GetName());
                }
            }
        }
    }


    void AnimGraphNodeIdPicker::SetNodeId(AnimGraphNodeId nodeId)
    {
        m_nodeId = nodeId;
        UpdateInterface();
    }


    AnimGraphNodeId AnimGraphNodeIdPicker::GetNodeId() const
    {
        return m_nodeId;
    }


    void AnimGraphNodeIdPicker::SetShowStatesOnly(bool showStatesOnly)
    {
        m_showStatesOnly = showStatesOnly;
    }


    void AnimGraphNodeIdPicker::SetNodeTypeFilter(const AZ::TypeId& nodeFilterType)
    {
        m_nodeFilterType = nodeFilterType;
    }


    void AnimGraphNodeIdPicker::OnPickClicked()
    {
        if (!m_animGraph)
        {
            AZ_Error("EMotionFX", false, "Cannot open anim graph node selection window. No valid anim graph.");
            return;
        }

        // create and show the node picker window
        EMStudio::BlendNodeSelectionWindow dialog(this);
        dialog.GetAnimGraphHierarchyWidget().SetSingleSelectionMode(true);
        dialog.GetAnimGraphHierarchyWidget().SetFilterNodeType(m_nodeFilterType);
        dialog.GetAnimGraphHierarchyWidget().SetFilterStatesOnly(m_showStatesOnly);
        dialog.GetAnimGraphHierarchyWidget().SetRootAnimGraph(m_animGraph);
        dialog.setModal(true);

        if (dialog.exec() != QDialog::Rejected)
        {
            const AZStd::vector<AnimGraphSelectionItem>& selectedNodes = dialog.GetAnimGraphHierarchyWidget().GetSelectedItems();
            if (!selectedNodes.empty())
            {
                AnimGraphNode* selectedNode = m_animGraph->RecursiveFindNodeByName(selectedNodes[0].m_nodeName.c_str());
                if (selectedNode)
                {
                    m_nodeId = selectedNode->GetId();
                    UpdateInterface();
                    emit SelectionChanged();
                }
            }
        }
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    AnimGraphNodeIdHandler::AnimGraphNodeIdHandler()
        : QObject()
        , AzToolsFramework::PropertyHandler<AZ::u64, AnimGraphNodeIdPicker>()
        , m_animGraph(nullptr)
        , m_nodeFilterType(AZ::TypeId::CreateNull())
        , m_showStatesOnly(false)
    {
    }

    AZ::u32 AnimGraphNodeIdHandler::GetHandlerName() const
    {
        return AZ_CRC_CE("AnimGraphNodeId");
    }


    QWidget* AnimGraphNodeIdHandler::CreateGUI(QWidget* parent)
    {
        AnimGraphNodeIdPicker* picker = aznew AnimGraphNodeIdPicker(parent);
        picker->SetShowStatesOnly(m_showStatesOnly);
        picker->SetNodeTypeFilter(m_nodeFilterType);

        connect(picker, &AnimGraphNodeIdPicker::SelectionChanged, this, [picker]()
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, picker);
        });

        return picker;
    }


    void AnimGraphNodeIdHandler::ConsumeAttribute(AnimGraphNodeIdPicker* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, [[maybe_unused]] const char* debugName)
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


    void AnimGraphNodeIdHandler::WriteGUIValuesIntoProperty([[maybe_unused]] size_t index, AnimGraphNodeIdPicker* GUI, property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        instance = GUI->GetNodeId();
    }


    bool AnimGraphNodeIdHandler::ReadValuesIntoGUI([[maybe_unused]] size_t index, AnimGraphNodeIdPicker* GUI, const property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker signalBlocker(GUI);
        GUI->SetNodeId(instance);
        return true;
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    AnimGraphMotionNodeIdHandler::AnimGraphMotionNodeIdHandler()
        : AnimGraphNodeIdHandler()
    {
        m_nodeFilterType = azrtti_typeid<AnimGraphMotionNode>();
    }


    AZ::u32 AnimGraphMotionNodeIdHandler::GetHandlerName() const
    {
        return AZ_CRC_CE("AnimGraphMotionNodeId");
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    AnimGraphStateIdHandler::AnimGraphStateIdHandler()
        : AnimGraphNodeIdHandler()
    {
        m_showStatesOnly = true;
    }


    AZ::u32 AnimGraphStateIdHandler::GetHandlerName() const
    {
        return AZ_CRC_CE("AnimGraphStateId");
    }
} // namespace EMotionFX

#include <Source/Editor/PropertyWidgets/moc_AnimGraphNodeHandler.cpp>
