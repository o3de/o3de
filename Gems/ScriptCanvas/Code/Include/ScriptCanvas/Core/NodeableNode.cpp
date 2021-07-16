/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "NodeableNode.h"

#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Serialization/IdUtils.h>
#include <ScriptCanvas/Core/Attributes.h>
#include <ScriptCanvas/Libraries/Core/Method.h>
#include <ScriptCanvas/Libraries/Core/MethodUtility.h>

namespace NodeableNodeCpp
{
    AZ::BehaviorContext* GetBehaviorContext()
    {
        AZ::BehaviorContext* behaviorContext(nullptr);
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
        AZ_Assert(behaviorContext, "BehaviorContext is required");
        return behaviorContext;
    }
}

namespace ScriptCanvas
{
    namespace Nodes
    {
        const AZ::Crc32 NodeableNode::c_onVariableHandlingGroup = AZ::Crc32("OnInputChangeMethod");

        void NodeableNode::AddOut(const Grammar::Out& out, const AZStd::string& name, const AZStd::string& outputPrefix, bool isLatent, SlotExecution::Outs& outs)
        {
            ExecutionSlotConfiguration outSlotConfig(name, ConnectionType::Output);
            outSlotConfig.m_isLatent = isLatent;
            auto outSlotId = AddSlot(outSlotConfig);
            AZ_Error("ScriptCanvas", outSlotId.IsValid(), "Failed to add branching out slot to node.");
            
            SlotExecution::Out outSlotMapEntry;
            // static_assert(false, "check here for why the name NewLevel and DangerZoneEntered get shrunk but the others did not.");
            outSlotMapEntry.name = out.displayName;
            outSlotMapEntry.slotId = outSlotId;

            const AZStd::string outPrefix = out.displayName + ":";

            for (const Grammar::Output& output : out.outputs)
            {
                DataSlotConfiguration slotConfiguration;
                slotConfiguration.m_name = outputPrefix;
                slotConfiguration.m_name += outPrefix;
                slotConfiguration.m_name += output.displayName;
                slotConfiguration.SetType(output.type);
                slotConfiguration.SetConnectionType(ConnectionType::Output);
                auto outputSlotId = AddSlot(slotConfiguration);
                AZ_Error("ScriptCanvas", outSlotId.IsValid(), "Failed to add output slot to branching slot");
                outSlotMapEntry.outputs.emplace_back(outputSlotId);
            }


            for (const auto& returnValue : out.returnValues)
            {
                if (returnValue.datum.GetType().IsValid())
                {
                    DataSlotConfiguration slotConfiguration;
                    slotConfiguration.m_name = outputPrefix;
                    slotConfiguration.m_name += outPrefix;
                    slotConfiguration.m_name += returnValue.displayName;
                    slotConfiguration.SetType(returnValue.datum.GetType());
                    slotConfiguration.SetConnectionType(ConnectionType::Input);
                    auto returnValueSlotId = AddSlot(slotConfiguration);
                    AZ_Error("ScriptCanvas", returnValueSlotId.IsValid(), "Failed to add return value slot slot");
                    outSlotMapEntry.returnValues.values.push_back(returnValueSlotId);
                }
            }

            outs.push_back(outSlotMapEntry);
        }

        void NodeableNode::ConfigureSlots()
        {
            AZ_Error("ScriptCanvas", m_nodeable, "null Nodeable in NodeableNode::ConfigureSlots");
        }

        AZ::Outcome<const AZ::BehaviorClass*, AZStd::string> NodeableNode::GetBehaviorContextClass() const
        {
            AZ::BehaviorContext* behaviorContext = NodeableNodeCpp::GetBehaviorContext();

            auto classIter(behaviorContext->m_typeToClassMap.find(GetNodeableType()));
            if (classIter == behaviorContext->m_typeToClassMap.end())
            {
                return AZ::Failure(AZStd::string::format("Nodeable type not found in BehaviorContext %s", GetNodeableType().ToString<AZStd::string>().data()));
            }

            const AZ::BehaviorClass* behaviorClass(classIter->second);
            if (!behaviorClass)
            {
                return AZ::Failure(AZStd::string::format("BehaviorContext Class entry %s has no class pointer", GetNodeableType().ToString<AZStd::string>().data()));
            }

            return AZ::Success(behaviorClass);
        }

        ConstSlotsOutcome NodeableNode::GetBehaviorContextOutName(const Slot& inSlot) const
        {
            auto behaviorClassOutcome = GetBehaviorContextClass();
            if (!behaviorClassOutcome)
            {
                return AZ::Failure(behaviorClassOutcome.TakeError());
            }

            const AZ::BehaviorClass* behaviorClass = behaviorClassOutcome.GetValue();

            auto methodIter(behaviorClass->m_methods.find(inSlot.GetName()));
            if (methodIter == behaviorClass->m_methods.end())
            {
                return AZ::Failure(AZStd::string::format("BehaviorContext Class %s has no method by name %s", behaviorClass->m_name.data(), inSlot.GetName().c_str()));
            }

            // consider checking here that the method doesn't branch, either declared (eg, an outcome) or undeclared.
            // ^^ or move such a check before the branching in the parser
            auto outSlot = GetSlotByNameAndType(inSlot.GetName(), CombinedSlotType::ExecutionOut);
            if (!outSlot)
            {
                return AZ::Failure(AZStd::string::format("No out slot by name of %s was found in the node %s", inSlot.GetName().data(), behaviorClass->m_name.data()));
            }

            AZStd::vector<const Slot*> outSlots;
            outSlots.push_back(outSlot);
            return AZ::Success(outSlots);
        }

        AZ::Outcome<DependencyReport, void> NodeableNode::GetDependencies() const
        {
            // Nodeable nodes shouldn't need to introduce any dependencies since they are managed through behavior context
            return AZ::Success(DependencyReport{});
        }
        
        AZ::Outcome<AZStd::string, void> NodeableNode::GetFunctionCallName(const Slot* slot) const
        {
            return AZ::Success(AZStd::string(slot->GetName()));
        }

        AZ::Outcome<Grammar::LexicalScope, void> NodeableNode::GetFunctionCallLexicalScope(const Slot* /*slot*/) const
        {
            if (!GetBehaviorContextClass())
            {
                return AZ::Failure();
            }

            return AZ::Success(Grammar::LexicalScope::Variable());
        }

        AZ::TypeId NodeableNode::GetNodeableType() const
        {
            if (auto nodeable = GetNodeable())
            {
                return azrtti_typeid(*nodeable);
            }
            else
            {
                return {};
            }
        }

        AZStd::vector<const Slot*> NodeableNode::GetOnVariableHandlingDataSlots() const
        {
            AZStd::vector<const Slot*> result;
            for (auto slotConstPtr : GetSlotsByType(CombinedSlotType::DataIn))
            {
                if (slotConstPtr->GetDisplayGroup() == c_onVariableHandlingGroup)
                {
                    result.push_back(slotConstPtr);
                }
            }
            return result;
        }

        AZStd::vector<const Slot*> NodeableNode::GetOnVariableHandlingExecutionSlots() const
        {
            AZStd::vector<const Slot*> result;
            for (auto slotConstPtr : GetSlotsByType(CombinedSlotType::ExecutionIn))
            {
                if (slotConstPtr->GetDisplayGroup() == c_onVariableHandlingGroup)
                {
                    result.push_back(slotConstPtr);
                }
            }
            return result;
        }

        NodePropertyInterface* NodeableNode::GetPropertyInterface(AZ::Crc32 propertyId)
        {
            if (m_nodeable)
            {
                return m_nodeable->GetPropertyInterface(propertyId);
            }

            return nullptr;
        }

        Nodeable* NodeableNode::GetMutableNodeable() const
        {
            return m_nodeable.get();
        }

        const Nodeable* NodeableNode::GetNodeable() const
        {
            return GetMutableNodeable();
        }
        
        const SlotExecution::Map* NodeableNode::GetSlotExecutionMap() const
        {
            return &m_slotExecutionMap;
        }

        bool NodeableNode::IsNodeableNode() const
        {
            return true;
        }

        void NodeableNode::Reflect(AZ::ReflectContext* reflectContext)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
            {
                serializeContext->Class<NodeableNode, Node>()
                    ->Field("nodeable", &NodeableNode::m_nodeable)
                    ->Field("slotExecutionMap", &NodeableNode::m_slotExecutionMap)
                    ;

                if (auto editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<NodeableNode>("NodeableNode", "NodeableNode")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                            ->DataElement(AZ::Edit::UIHandlers::Default, &NodeableNode::m_nodeable, "Nodeable", "")
                        ;
                }
            }
        }

        Nodeable* NodeableNode::ReleaseNodeable()
        {
            return m_nodeable.release();
        }

        void NodeableNode::SetSlotExecutionMap(SlotExecution::Map&& map)
        {
            m_slotExecutionMap = map;
        }

        void NodeableNode::SetNodeable(AZStd::unique_ptr<Nodeable> nodeable)
        {
            AZ_Assert(!m_nodeable, "nodeable is already set");
            m_nodeable = AZStd::move(nodeable);
        }

    }
}
