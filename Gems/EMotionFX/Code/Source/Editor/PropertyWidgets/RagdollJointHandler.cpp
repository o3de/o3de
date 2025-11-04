/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/PropertyWidgets/PropertyWidgetAllocator.h>
#include <Editor/PropertyWidgets/RagdollJointHandler.h>
#include <Editor/JointSelectionDialog.h>
#include <Editor/SkeletonSortFilterProxyModel.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(RagdollMultiJointHandler, PropertyWidgetAllocator)

    AZ::u32 RagdollMultiJointHandler::GetHandlerName() const
    {
        return AZ_CRC_CE("ActorRagdollJoints");
    }

    QWidget* RagdollMultiJointHandler::CreateGUI(QWidget* parent)
    {
        ActorJointPicker* picker = aznew ActorJointPicker(false /*singleSelection*/, "Select Ragdoll Joints", "Select the ragdoll joints to be simulated", parent);
        picker->AddDefaultFilter(SkeletonSortFilterProxyModel::s_simulationCategory, SkeletonSortFilterProxyModel::s_ragdollNodesFilterName);

        connect(picker, &ActorJointPicker::SelectionChanged, this, [picker]()
            {
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                    &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, picker);
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
