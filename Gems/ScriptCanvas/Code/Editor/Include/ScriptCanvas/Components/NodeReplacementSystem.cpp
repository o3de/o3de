/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "NodeReplacementSystem.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryVisitorUtils.h>
#include <AzCore/std/string/string_view.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/GraphBus.h>
#include <ScriptCanvas/Libraries/Core/Method.h>
#include <ScriptCanvas/Utils/NodeUtils.h>
#include <ScriptCanvas/Utils/VersioningUtils.h>

namespace ScriptCanvasEditor
{
    static constexpr const int NodeReplacementMinSize = 2;
    static constexpr const char NodeReplacementRootPath[] = "/O3DE/NodeReplacement";
    static constexpr const char NodeReplacementOldNodeFieldName[] = "OldNode";
    static constexpr const char NodeReplacementNewNodeFieldName[] = "NewNode";
    static constexpr const char NodeReplacementUuidFieldName[] = "Uuid";
    static constexpr const char NodeReplacementClassFieldName[] = "Class";
    static constexpr const char NodeReplacementMethodFieldName[] = "Method";

    NodeReplacementSystem::NodeReplacementSystem()
    {
        AZ::Interface<INodeReplacementRequests>::Register(this);
        NodeReplacementRequestBus::Handler::BusConnect();
    }

    NodeReplacementSystem::~NodeReplacementSystem()
    {
        NodeReplacementRequestBus::Handler::BusDisconnect();
        AZ::Interface<INodeReplacementRequests>::Unregister(this);
    }

    NodeReplacementId NodeReplacementSystem::GenerateReplacementId(const AZ::Uuid& id, const AZStd::string& className, const AZStd::string& methodName)
    {
        NodeReplacementId result = id.ToFixedString().c_str();
        if (!className.empty() && !methodName.empty())
        {
            result += AZStd::string::format("_%s_%s", className.c_str(), methodName.c_str());
        }
        else if (!methodName.empty())
        {
            result += AZStd::string::format("_%s", methodName.c_str());
        }
        return result;
    }

    NodeReplacementId NodeReplacementSystem::GenerateReplacementId(ScriptCanvas::Node* node)
    {
        if (node)
        {
            if (auto* methodNode = azrtti_cast<ScriptCanvas::Nodes::Core::Method*>(node))
            {
                return GenerateReplacementId(methodNode->RTTI_GetType(),
                    methodNode->GetRawMethodClassName().empty() ? "" : methodNode->GetMethodClassName(),
                    methodNode->GetName());
            }
            else // custom node, which includes grammar and nodeable node
            {
                return GenerateReplacementId(node->RTTI_GetType());
            }
        }
        return "";
    }

    ScriptCanvas::NodeReplacementConfiguration NodeReplacementSystem::GetNodeReplacementConfiguration(
        const NodeReplacementId& replacementId) const
    {
        auto foundIter = m_replacementMetadata.find(replacementId);
        if (foundIter != m_replacementMetadata.end())
        {
            return foundIter->second;
        }
        return {};
    }

    ScriptCanvas::Node* NodeReplacementSystem::GetOrCreateNodeFromReplacementConfiguration(
        const ScriptCanvas::NodeReplacementConfiguration& config)
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        if (!serializeContext)
        {
            AZ_Warning("ScriptCanvas", false, "Failed to retrieve application serialize context.");
            return nullptr;
        }

        const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(config.m_type);
        if (!classData)
        {
            AZ_Warning("ScriptCanvas", false, "Failed to find replacement class with UUID %s from serialize context.", config.m_type.ToFixedString().c_str());
            return nullptr;
        }

        auto newNode = reinterpret_cast<ScriptCanvas::Node*>(classData->m_factory->Create(classData->m_name));
        AZ_Warning("ScriptCanvas", newNode != nullptr, "Failed to create replacement Node (%s).", classData->m_name);
        return newNode;
    }

    void NodeReplacementSystem::LoadReplacementMetadata()
    {
        AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get();
        AZ_Assert(settingsRegistry, "Global Settings registry must be available to retrieve replacement metadata.");

        auto RetrieveNodeReplacementFields =
            [this, &settingsRegistry](const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
        {
            auto RetrieveNodeReplacementArray =
                [this, &settingsRegistry](const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
            {
                AZStd::unordered_map<AZStd::string, ScriptCanvas::NodeReplacementConfiguration> tempReplacementMap;

                auto RetrieveNodeReplacementObject =
                    [&settingsRegistry, &tempReplacementMap](const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
                {
                    if (visitArgs.m_fieldName == NodeReplacementOldNodeFieldName ||
                        visitArgs.m_fieldName == NodeReplacementNewNodeFieldName)
                    {
                        ScriptCanvas::NodeReplacementConfiguration replacementConfig;
                        AZStd::string uuid;
                        if (settingsRegistry->Get(
                                uuid, AZStd::string::format("%s/%s", visitArgs.m_jsonKeyPath.data(), NodeReplacementUuidFieldName)))
                        {
                            replacementConfig.m_type = AZ::Uuid::CreateString(uuid.c_str());
                        }
                        else
                        {
                            // Uuid must be presented
                            return AZ::SettingsRegistryInterface::VisitResponse::Skip;
                        }

                        settingsRegistry->Get(
                            replacementConfig.m_className,
                            AZStd::string::format("%s/%s", visitArgs.m_jsonKeyPath.data(), NodeReplacementClassFieldName));
                        settingsRegistry->Get(
                            replacementConfig.m_methodName,
                            AZStd::string::format("%s/%s", visitArgs.m_jsonKeyPath.data(), NodeReplacementMethodFieldName));

                        tempReplacementMap.emplace(visitArgs.m_fieldName, replacementConfig);
                    }

                    return AZ::SettingsRegistryInterface::VisitResponse::Skip;
                };

                AZ::SettingsRegistryVisitorUtils::VisitObject(
                    *settingsRegistry, RetrieveNodeReplacementObject, visitArgs.m_jsonKeyPath.data());
                // Assume replacement metadata should be a pair
                if (tempReplacementMap.size() >= NodeReplacementMinSize)
                {
                    ScriptCanvas::NodeReplacementConfiguration oldNode = tempReplacementMap[NodeReplacementOldNodeFieldName];
                    NodeReplacementId replacementId = GenerateReplacementId(oldNode.m_type, oldNode.m_className, oldNode.m_methodName);
                    ScriptCanvas::NodeReplacementConfiguration newNode = tempReplacementMap[NodeReplacementNewNodeFieldName];
                    m_replacementMetadata.emplace(replacementId, newNode);
                }
                return AZ::SettingsRegistryInterface::VisitResponse::Skip;
            };

            AZ::SettingsRegistryVisitorUtils::VisitField(*settingsRegistry, RetrieveNodeReplacementArray, visitArgs.m_jsonKeyPath.data());
            return AZ::SettingsRegistryInterface::VisitResponse::Skip;
        };

        AZ::SettingsRegistryVisitorUtils::VisitField(*settingsRegistry, RetrieveNodeReplacementFields, NodeReplacementRootPath);
    }

    ScriptCanvas::NodeUpdateReport NodeReplacementSystem::ReplaceNodeByReplacementConfiguration(
        const AZ::EntityId& graphId, ScriptCanvas::Node* oldNode, const ScriptCanvas::NodeReplacementConfiguration& config)
    {
        ScriptCanvas::NodeUpdateReport report;

        if (!graphId.IsValid())
        {
            AZ_Warning("ScriptCanvas", false, "Graph %s is invalid to do node replacement.", graphId.ToString().c_str());
            return report;
        }

        auto nodeEntity = oldNode->GetEntity();
        if (!nodeEntity)
        {
            AZ_Warning("ScriptCanvas", false, "Could not find Node Entity for Node (%s).", oldNode->GetNodeName().c_str());
            return report;
        }

        ScriptCanvas::Node* newNode = GetOrCreateNodeFromReplacementConfiguration(config);
        if (!newNode)
        {
            AZ_Warning("ScriptCanvas", false, "Node %s does not have a valid replacement configuration.", oldNode->GetNodeName().c_str());
            return report;
        }
        report.m_newNode = newNode;

        bool rollbackRequired = false;
        nodeEntity->Deactivate();
        ScriptCanvas::GraphRequestBus::Event(graphId, &ScriptCanvas::GraphRequests::RemoveNode, oldNode->GetEntityId());

        nodeEntity->RemoveComponent(oldNode);

        nodeEntity->AddComponent(newNode);
        ScriptCanvas::GraphRequestBus::Event(graphId, &ScriptCanvas::GraphRequests::AddNode, newNode->GetEntityId());
        ScriptCanvas::NodeUtils::InitializeNode(newNode, config);

        rollbackRequired = !SanityCheckNodeReplacement(oldNode, newNode, report);
        auto& slotIdMap = report.m_oldSlotsToNewSlots;

        if (rollbackRequired)
        {
            ScriptCanvas::GraphRequestBus::Event(graphId, &ScriptCanvas::GraphRequests::RemoveNode, newNode->GetEntityId());
            nodeEntity->RemoveComponent(newNode);
            delete newNode;

            nodeEntity->AddComponent(oldNode);
            ScriptCanvas::GraphRequestBus::Event(graphId, &ScriptCanvas::GraphRequests::AddNode, oldNode->GetEntityId());
            nodeEntity->Activate();

            report.Clear();
            return report;
        }
        else
        {
            nodeEntity->Activate();
            newNode->SignalReconfigurationBegin();
            newNode->SetNodeDisabledFlag(oldNode->GetNodeDisabledFlag());

            for (auto slotIdIter : slotIdMap)
            {
                ScriptCanvas::Slot* oldSlot = oldNode->GetSlot(slotIdIter.first);
                const ScriptCanvas::Endpoint oldEndpoint{ nodeEntity->GetId(), oldSlot->GetId() };
                if (slotIdIter.second.size() == 0)
                {
                    continue;
                }

                for (auto newSlotId : slotIdIter.second)
                {
                    ScriptCanvas::Slot* newSlot = nullptr;
                    if (newSlotId.IsValid())
                    {
                        newSlot = newNode->GetSlot(newSlotId);
                    }

                    // Copy over old slot data to new slot
                    if (newSlot && newSlot->GetDescriptor().IsData() && oldSlot->GetDescriptor().IsData())
                    {
                        AZ::Crc32 oldDynamicGroup = oldSlot->GetDynamicGroup();
                        ScriptCanvas::Data::Type oldDisplayType;
                        if (oldDynamicGroup != AZ::Crc32())
                        {
                            oldDisplayType = oldNode->GetDisplayType(oldDynamicGroup);
                        }
                        else
                        {
                            oldDisplayType = oldSlot->GetDataType();
                        }

                        if (oldDisplayType.IsValid())
                        {
                            AZ::Crc32 newDynamicGroup = newSlot->GetDynamicGroup();
                            if (newDynamicGroup != AZ::Crc32())
                            {
                                newNode->SetDisplayType(newDynamicGroup, oldDisplayType);
                            }
                            else
                            {
                                newSlot->ClearDisplayType();
                                newSlot->SetDisplayType(oldDisplayType);
                            }
                        }

                        ScriptCanvas::VersioningUtils::CopyOldValueToDataSlot(
                            newSlot, oldSlot->GetVariableReference(), oldSlot->FindDatum());
                    }
                }
            }

            delete oldNode;
            newNode->SignalReconfigurationEnd();
            return report;
        }
    }

    bool NodeReplacementSystem::SanityCheckNodeReplacement(
        ScriptCanvas::Node* oldNode, ScriptCanvas::Node* newNode, ScriptCanvas::NodeUpdateReport& report)
    {
        if (!newNode)
        {
            AZ_Warning("ScriptCanvas", false, "Replacement node can not be null.");
            return false;
        }

        // Do node replace with custom logic first if there is any
        // If it fails, fall back to replace based on same topology
        return SanityCheckNodeReplacementWithCustomLogic(oldNode, newNode, report) ||
            SanityCheckNodeReplacementWithSameTopology(oldNode, newNode, report);
    }

    bool NodeReplacementSystem::SanityCheckNodeReplacementWithCustomLogic(
        ScriptCanvas::Node* oldNode, ScriptCanvas::Node* newNode, ScriptCanvas::NodeUpdateReport& report)
    {
        auto findReplacementMatch = [](const ScriptCanvas::Slot* oldSlot,
                                       const AZStd::vector<const ScriptCanvas::Slot*>& newSlots) -> ScriptCanvas::SlotId
        {
            for (auto& newSlot : newSlots)
            {
                if (newSlot->GetName() == oldSlot->GetName() && newSlot->GetType() == oldSlot->GetType() &&
                    (newSlot->IsExecution() || newSlot->GetDataType() == oldSlot->GetDataType()))
                {
                    return newSlot->GetId();
                }
            }

            return {};
        };

        // clean guard to avoid stale data if any
        auto& oldSlotsToNewSlots = report.m_oldSlotsToNewSlots;
        oldSlotsToNewSlots.clear();
        oldNode->CustomizeReplacementNode(newNode, report.m_oldSlotsToNewSlots);

        AZStd::unordered_map<AZStd::string, AZStd::vector<AZStd::string>> slotNameMap = oldNode->GetReplacementSlotsMap();

        const auto newSlots = newNode->GetAllSlots();
        const auto oldSlots = oldNode->GetAllSlots();

        for (auto oldSlot : oldSlots)
        {
            const ScriptCanvas::SlotId oldSlotId = oldSlot->GetId();
            const AZStd::string oldSlotName = oldSlot->GetName();

            auto slotIdsIter = oldSlotsToNewSlots.find(oldSlotId);
            auto slotNamesIter = slotNameMap.find(oldSlotName);
            // For old node slot remapping, we should get:
            // 1. if old slot name is not static, we should find the mapping in user provided slot id map
            // 2. if old slot name is static, we should find the mapping in codegen generated map (case 1 can override case 2)
            if (slotIdsIter != oldSlotsToNewSlots.end())
            {
                for (auto newSlotId : slotIdsIter->second)
                {
                    if (newSlotId.IsValid())
                    {
                        auto newSlot = newNode->GetSlot(newSlotId);
                        if (!newSlot)
                        {
                            AZ_Warning("ScriptCanvas", false, "Failed to find slot with id %s in replacement Node(%s).",
                                newSlotId.ToString().c_str(), newNode->GetNodeName().c_str());
                            return false;
                        }
                        else if (newSlot && oldSlot->GetType() != newSlot->GetType())
                        {
                            AZ_Warning("ScriptCanvas", false, "Failed to map old Node (%s) Slot (%s) to replacement Node (%s) Slot (%s).",
                                oldNode->GetNodeName().c_str(), oldSlot->GetName().c_str(),
                                newNode->GetNodeName().c_str(), newSlot->GetName().c_str());
                            return false;
                        }
                    }
                }
            }
            else if (slotNamesIter != slotNameMap.end())
            {
                AZStd::vector<ScriptCanvas::SlotId> newSlotIds;
                for (auto newSlotName : slotNamesIter->second)
                {
                    if (!newSlotName.empty())
                    {
                        auto newSlot = newNode->GetSlotByName(newSlotName);

                        if (!newSlot)
                        {
                            AZ_Warning("ScriptCanvas", false, "Failed to find slot with name %s in replacement Node (%s).",
                                newSlotName.c_str(), newNode->GetNodeName().c_str());
                            return false;
                        }
                        else if (newSlot && oldSlot->GetType() != newSlot->GetType())
                        {
                            AZ_Warning("ScriptCanvas", false, "Failed to map old Node (%s) Slot (%s) to replacement Node (%s) Slot (%s).",
                                oldNode->GetNodeName().c_str(), oldSlot->GetName().c_str(),
                                newNode->GetNodeName().c_str(), newSlot->GetName().c_str());
                            return false;
                        }

                        newSlotIds.push_back(newSlot->GetId());
                    }
                }
                oldSlotsToNewSlots.emplace(oldSlot->GetId(), newSlotIds);
            }
            else if (slotNameMap.empty())
            {
                auto newSlotId = findReplacementMatch(oldSlot, newSlots);

                if (newSlotId.IsValid())
                {
                    AZStd::vector<ScriptCanvas::SlotId> slotIds{ newSlotId };
                    oldSlotsToNewSlots.emplace(oldSlot->GetId(), slotIds);
                }
            }
            else
            {
                AZ_Warning("ScriptCanvas", false, "Failed to remap old Node(%s) Slot(%s).",
                    oldNode->GetNodeName().c_str(), oldSlot->GetName().c_str());
                return false;
            }
        }

        if (oldSlotsToNewSlots.size() != oldSlots.size())
        {
            AZ_Warning("ScriptCanvas", false, "Old Node(%s) slots are not fully remapped by using custom replacement, going to do replacement based on topology.",
                oldNode->GetNodeName().c_str());
            return false;
        }

        return true;
    }

    bool NodeReplacementSystem::SanityCheckNodeReplacementWithSameTopology(
        ScriptCanvas::Node* oldNode, ScriptCanvas::Node* newNode, ScriptCanvas::NodeUpdateReport& report)
    {
        // clean guard to avoid stale data if any
        auto& oldSlotsToNewSlots = report.m_oldSlotsToNewSlots;
        oldSlotsToNewSlots.clear();

        auto findTopologyMatch = [&oldSlotsToNewSlots](
                                     const AZStd::vector<const ScriptCanvas::Slot*>& oldSlots,
                                     const AZStd::vector<const ScriptCanvas::Slot*>& newSlots) -> void
        {
            if (oldSlots.size() == newSlots.size())
            {
                for (size_t index = 0; index < newSlots.size(); index++)
                {
                    auto oldSlot = oldSlots[index];
                    auto newSlot = newSlots[index];
                    if (newSlot->GetType() == oldSlot->GetType() &&
                        (newSlot->IsExecution() || newSlot->GetDataType() == oldSlot->GetDataType() ||
                         (newSlot->IsDynamicSlot() && oldSlot->IsDynamicSlot() &&
                          newSlot->GetDynamicDataType() == oldSlot->GetDynamicDataType() &&
                          newSlot->GetDynamicGroup() == oldSlot->GetDynamicGroup())))
                    {
                        oldSlotsToNewSlots.emplace(oldSlot->GetId(), AZStd::vector<ScriptCanvas::SlotId>{ newSlot->GetId() });
                    }
                }
            }
        };

        // ExecutionIn slots map
        findTopologyMatch(
            oldNode->GetSlotsByType(ScriptCanvas::CombinedSlotType::ExecutionIn),
            newNode->GetSlotsByType(ScriptCanvas::CombinedSlotType::ExecutionIn));

        // LatentOut slots map
        findTopologyMatch(
            oldNode->GetSlotsByType(ScriptCanvas::CombinedSlotType::LatentOut),
            newNode->GetSlotsByType(ScriptCanvas::CombinedSlotType::LatentOut));

        // ExecutionOut slots map
        findTopologyMatch(
            oldNode->GetSlotsByType(ScriptCanvas::CombinedSlotType::ExecutionOut),
            newNode->GetSlotsByType(ScriptCanvas::CombinedSlotType::ExecutionOut));

        // DataIn slots map
        findTopologyMatch(
            oldNode->GetSlotsByType(ScriptCanvas::CombinedSlotType::DataIn),
            newNode->GetSlotsByType(ScriptCanvas::CombinedSlotType::DataIn));

        // DataOut slots map
        findTopologyMatch(
            oldNode->GetSlotsByType(ScriptCanvas::CombinedSlotType::DataOut),
            newNode->GetSlotsByType(ScriptCanvas::CombinedSlotType::DataOut));

        if (oldSlotsToNewSlots.size() != oldNode->GetAllSlots().size())
        {
            AZ_Warning("ScriptCanvas", false,
                "Failed to remap deprecated Node(%s) topology doesn't match with replacement node, please provide custom replacement slot map.",
                oldNode->GetNodeName().c_str());
            return false;
        }

        return true;
    }

    void NodeReplacementSystem::UnloadReplacementMetadata()
    {
        m_replacementMetadata.clear();
    }
}
