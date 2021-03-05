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

#include <Editor/PropertyWidgets/PropertyWidgetAllocator.h>
#include <Editor/PropertyWidgets/RagdollJointHandler.h>
#include <Editor/JointSelectionDialog.h>
#include <Editor/SkeletonSortFilterProxyModel.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(RagdollMultiJointHandler, PropertyWidgetAllocator, 0)

    AZ::u32 RagdollMultiJointHandler::GetHandlerName() const
    {
        return AZ_CRC("ActorRagdollJoints", 0xed1cae00);
    }

    QWidget* RagdollMultiJointHandler::CreateGUI(QWidget* parent)
    {
        ActorJointPicker* picker = aznew ActorJointPicker(false /*singleSelection*/, "Select Ragdoll Joints", "Select the ragdoll joints to be simulated", parent);
        picker->AddDefaultFilter(SkeletonSortFilterProxyModel::s_simulationCategory, SkeletonSortFilterProxyModel::s_ragdollNodesFilterName);

        connect(picker, &ActorJointPicker::SelectionChanged, this, [picker]()
            {
                EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, picker);
            });

        return picker;
    }

    void RagdollMultiJointHandler::ConsumeAttribute(ActorJointPicker* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, [[maybe_unused]] const char* debugName)
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

    void RagdollMultiJointHandler::WriteGUIValuesIntoProperty([[maybe_unused]] size_t index, ActorJointPicker* GUI, property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        instance = GUI->GetJointNames();
    }

    bool RagdollMultiJointHandler::ReadValuesIntoGUI([[maybe_unused]] size_t index, ActorJointPicker* GUI, const property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker signalBlocker(GUI);
        GUI->SetJointNames(instance);
        return true;
    }
} // namespace EMotionFX