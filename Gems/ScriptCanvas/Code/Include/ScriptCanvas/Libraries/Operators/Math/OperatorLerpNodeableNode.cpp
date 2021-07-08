/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "OperatorLerpNodeableNode.h"

#include <ScriptCanvas/Core/Slot.h>
#include <ScriptCanvas/Core/Core.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        void NodeableNodeOverloadedLerp::Reflect(AZ::ReflectContext* reflectContext)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
            {
                serializeContext->Class<NodeableNodeOverloadedLerp, NodeableNodeOverloaded>()
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<NodeableNodeOverloadedLerp>("Lerp Between", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Math")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ;
                }
            }
        }

        void NodeableNodeOverloadedLerp::ConfigureSlots()
        {
            SlotExecution::Ins ins;
            {
                {
                    SlotExecution::In in;
                    ScriptCanvas::ExecutionSlotConfiguration inSlotConfiguration;
                    inSlotConfiguration.m_name = "In";
                    inSlotConfiguration.m_displayGroup = "In";
                    inSlotConfiguration.m_toolTip = "Starts the lerp action from the beginning.";
                    inSlotConfiguration.SetConnectionType(ScriptCanvas::ConnectionType::Input);
                    inSlotConfiguration.m_isLatent = false;
                    SlotId inSlotId = AddSlot(inSlotConfiguration);
                    AZ_Assert(inSlotId.IsValid(), "In slot is not created successfully.");
                    in.slotId = inSlotId;

                    {
                        ScriptCanvas::DynamicDataSlotConfiguration dataSlotConfiguration;
                        dataSlotConfiguration.m_name = "Start";
                        dataSlotConfiguration.m_toolTip = "The initial value of linear interpolation";
                        dataSlotConfiguration.m_displayGroup = "In";
                        dataSlotConfiguration.m_dynamicGroup = GetDataDynamicGroup();

                        dataSlotConfiguration.SetConnectionType(ScriptCanvas::ConnectionType::Input);
                        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Value;

                        // Since this contract will check in with the underlying overload to enforce
                        // the typing, we don't need to have one of these contracts on each slot, as each
                        // group assignment will trigger this contract to confirm the typing.
                        dataSlotConfiguration.m_contractDescs = AZStd::vector<ScriptCanvas::ContractDescriptor>
                        {
                            { []() { return aznew ScriptCanvas::OverloadContract(); } }
                        };

                        SlotId slotId = AddSlot(dataSlotConfiguration);
                        AZ_Assert(slotId.IsValid(), "Data slot is not created successfully.");
                        in.inputs.push_back(slotId);
                    }

                    {
                        ScriptCanvas::DynamicDataSlotConfiguration dataSlotConfiguration;
                        dataSlotConfiguration.m_name = "Stop";
                        dataSlotConfiguration.m_toolTip = "The final value of linear interpolation";
                        dataSlotConfiguration.m_displayGroup = "In";
                        dataSlotConfiguration.m_dynamicGroup = GetDataDynamicGroup();

                        dataSlotConfiguration.SetConnectionType(ScriptCanvas::ConnectionType::Input);
                        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Value;

                        SlotId slotId = AddSlot(dataSlotConfiguration);
                        AZ_Assert(slotId.IsValid(), "Data slot is not created successfully.");
                        in.inputs.push_back(slotId);
                    }

                    {
                        ScriptCanvas::DynamicDataSlotConfiguration  dataSlotConfiguration;
                        dataSlotConfiguration.m_name = "Speed";
                        dataSlotConfiguration.m_toolTip = "The speed at which to lerp between the start and stop. Zero entries.";
                        dataSlotConfiguration.m_displayGroup = "In";
                        dataSlotConfiguration.m_dynamicGroup = GetDataDynamicGroup();

                        dataSlotConfiguration.SetConnectionType(ScriptCanvas::ConnectionType::Input);
                        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Value;

                        SlotId slotId = AddSlot(dataSlotConfiguration);
                        AZ_Assert(slotId.IsValid(), "Data slot is not created successfully.");
                        in.inputs.push_back(slotId);
                    }

                    {
                        ScriptCanvas::DataSlotConfiguration dataSlotConfiguration;
                        dataSlotConfiguration.m_name = "Maximum Duration";
                        dataSlotConfiguration.m_toolTip = "The time, in seconds, it will take to complete the specified lerp. Negative value implies no limit, 0 implies instant.";
                        dataSlotConfiguration.m_displayGroup = "In";
                        dataSlotConfiguration.SetConnectionType(ScriptCanvas::ConnectionType::Input);
                        dataSlotConfiguration.SetDefaultValue<float>(0);
                        SlotId slotId = AddSlot(dataSlotConfiguration);
                        AZ_Assert(slotId.IsValid(), "Data slot is not created successfully.");
                        in.inputs.push_back(slotId);
                    }

                    {
                        SlotExecution::Out out;
                        ScriptCanvas::ExecutionSlotConfiguration outSlotConfiguration;
                        outSlotConfiguration.m_name = "Out";
                        outSlotConfiguration.m_displayGroup = "In";
                        outSlotConfiguration.m_toolTip = "Executes immediately after the lerp action is started.";
                        outSlotConfiguration.SetConnectionType(ScriptCanvas::ConnectionType::Output);
                        outSlotConfiguration.m_isLatent = false;
                        SlotId outSlotId = AddSlot(outSlotConfiguration);
                        AZ_Assert(outSlotId.IsValid(), "Out slot is not created successfully.");
                        out.name = outSlotConfiguration.m_name;
                        out.slotId = outSlotId;
                        in.outs.push_back(out);
                    }

                    ins.push_back(in);
                }

                {
                    SlotExecution::In cancel;
                    ScriptCanvas::ExecutionSlotConfiguration inSlotConfiguration;
                    inSlotConfiguration.m_name = "Cancel";
                    inSlotConfiguration.m_displayGroup = "Cancel";
                    inSlotConfiguration.m_toolTip = "Stops the lerp action immediately.";
                    inSlotConfiguration.SetConnectionType(ScriptCanvas::ConnectionType::Input);
                    inSlotConfiguration.m_isLatent = false;
                    SlotId inSlotId = AddSlot(inSlotConfiguration);
                    AZ_Assert(inSlotId.IsValid(), "Cancel slot is not created successfully.");
                    cancel.slotId = inSlotId;

                    {
                        SlotExecution::Out out;
                        ScriptCanvas::ExecutionSlotConfiguration outSlotConfiguration;
                        outSlotConfiguration.m_name = "Canceled";
                        outSlotConfiguration.m_displayGroup = "Cancel";
                        outSlotConfiguration.m_toolTip = "Executes immediately after the operation is canceled.";
                        outSlotConfiguration.SetConnectionType(ScriptCanvas::ConnectionType::Output);
                        outSlotConfiguration.m_isLatent = false;
                        SlotId outSlotId = AddSlot(outSlotConfiguration);
                        AZ_Assert(outSlotId.IsValid(), "Cancel slot is not created successfully.");
                        out.name = outSlotConfiguration.m_name;
                        out.slotId = outSlotId;
                        cancel.outs.push_back(out);
                    }

                    ins.push_back(cancel);
                }
            }

            SlotExecution::Outs latentOuts;
            {
                // on tick
                {
                    SlotExecution::Out latentOut;

                    ScriptCanvas::ExecutionSlotConfiguration latentOutSlotConfiguration;
                    latentOutSlotConfiguration.m_name = "Tick";
                    latentOutSlotConfiguration.m_toolTip = "Signaled at each step of the lerp.";
                    latentOutSlotConfiguration.m_displayGroup = "Tick";
                    latentOutSlotConfiguration.SetConnectionType(ScriptCanvas::ConnectionType::Output);
                    latentOutSlotConfiguration.m_isLatent = true;
                    SlotId slotIdTick = AddSlot(latentOutSlotConfiguration);
                    AZ_Assert(slotIdTick.IsValid(), "LatentOutput slot is not created successfully.");
                    latentOut.name = latentOutSlotConfiguration.m_name;
                    latentOut.slotId = slotIdTick;

                    {
                        ScriptCanvas::DynamicDataSlotConfiguration dataSlotConfiguration;
                        dataSlotConfiguration.m_name = "Step";
                        dataSlotConfiguration.m_toolTip = "The value of the current step of the lerp";
                        dataSlotConfiguration.m_displayGroup = "Tick";
                        dataSlotConfiguration.m_dynamicGroup = GetDataDynamicGroup();

                        dataSlotConfiguration.SetConnectionType(ScriptCanvas::ConnectionType::Output);
                        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Value;

                        SlotId slotIdStep = AddSlot(dataSlotConfiguration);
                        AZ_Assert(slotIdStep.IsValid(), "Data slot is not created successfully.");
                        latentOut.outputs.push_back(slotIdStep);
                    }

                    {
                        ScriptCanvas::DataSlotConfiguration dataSlotConfiguration;
                        dataSlotConfiguration.m_name = "Percent";
                        dataSlotConfiguration.m_toolTip = "The percentage of the way the through the lerp on this tick.";
                        dataSlotConfiguration.m_displayGroup = "Tick";
                        dataSlotConfiguration.SetConnectionType(ScriptCanvas::ConnectionType::Output);
                        dataSlotConfiguration.SetDefaultValue<float>(0);
                        SlotId slotIdPercent = AddSlot(dataSlotConfiguration);
                        AZ_Assert(slotIdPercent.IsValid(), "Data slot is not created successfully.");
                        latentOut.outputs.push_back(slotIdPercent);
                    }

                    latentOuts.push_back(latentOut);
                }

                // lerp complete
                {
                    SlotExecution::Out latentOut;

                    ScriptCanvas::ExecutionSlotConfiguration latentOutSlotConfiguration;
                    latentOutSlotConfiguration.m_name = "Lerp Complete";
                    latentOutSlotConfiguration.m_toolTip = "Signaled after the last Tick, when the lerp is complete";
                    latentOutSlotConfiguration.m_displayGroup = "Lerp Complete";
                    latentOutSlotConfiguration.SetConnectionType(ScriptCanvas::ConnectionType::Output);
                    latentOutSlotConfiguration.m_isLatent = true;
                    SlotId slotId = AddSlot(latentOutSlotConfiguration);
                    AZ_Assert(slotId.IsValid(), "LatentOutput slot is not created successfully.");
                    latentOut.name = latentOutSlotConfiguration.m_name;
                    latentOut.slotId = slotId;

                    latentOuts.push_back(latentOut);
                }
            }

            SetSlotExecutionMap(AZStd::move(SlotExecution::Map(AZStd::move(ins), AZStd::move(latentOuts))));
        }

        AZStd::vector<AZStd::unique_ptr<Nodeable>> NodeableNodeOverloadedLerp::GetInitializationNodeables() const
        {
            AZStd::vector<AZStd::unique_ptr<Nodeable>> nodeables;
            nodeables.push_back(AZStd::make_unique<LerpBetweenNodeable<float>>());
            nodeables.push_back(AZStd::make_unique<LerpBetweenNodeable<ScriptCanvas::Data::Vector2Type>>());
            nodeables.push_back(AZStd::make_unique<LerpBetweenNodeable<ScriptCanvas::Data::Vector3Type>>());
            nodeables.push_back(AZStd::make_unique<LerpBetweenNodeable<ScriptCanvas::Data::Vector4Type>>());
            return nodeables;
        }
    }
}
