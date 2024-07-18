/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GetVariable.h"

#include <ScriptCanvas/Core/ScriptCanvasBus.h>
#include <ScriptCanvas/Grammar/ParsingUtilities.h>
#include <ScriptCanvas/Translation/GraphToLuaUtility.h>
#include <ScriptCanvas/Variable/VariableBus.h>
#include <AzCore/std/sort.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            AZ::Outcome<DependencyReport, void> GetVariableNode::GetDependencies() const
            {
                if (auto datum = GetDatum())
                {
                    return AZ::Success(DependencyReport::NativeLibrary(Data::GetName(datum->GetType())));
                }
                else
                {
                    return AZ::Failure();
                }
            }

            PropertyFields GetVariableNode::GetPropertyFields() const
            {
                PropertyFields propertyFields;
                for (auto&& propertyAccount : m_propertyAccounts)
                {
                    propertyFields.emplace_back(propertyAccount.m_propertyName, propertyAccount.m_propertySlotId);
                }

                return propertyFields;
            }

            void GetVariableNode::OnInit()
            {
                VariableNodeRequestBus::Handler::BusConnect(GetEntityId());
            }

            void GetVariableNode::OnPostActivate()
            {
                if (m_variableId.IsValid())
                {
                    RefreshPropertyFunctions();
                    PopulateNodeType();
                    GraphVariable* variable = FindGraphVariable(m_variableId);

                    if (variable)
                    {
                        m_variableName = variable->GetVariableName();
                        variable->ConfigureDatumView(m_variableView);
                    }
                }
            }

            void GetVariableNode::CollectVariableReferences(AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds) const
            {
                if (m_variableId.IsValid())
                {
                    variableIds.insert(m_variableId);
                }

                return Node::CollectVariableReferences(variableIds);
            }

            bool GetVariableNode::ContainsReferencesToVariables(const AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds) const
            {
                if (m_variableId.IsValid() && variableIds.count(m_variableId) > 0)
                {
                    return true;
                }

                return Node::ContainsReferencesToVariables(variableIds);
            }

            bool GetVariableNode::RemoveVariableReferences(const AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds)
            {
                // These nodes should just be deleted when the variable they reference is removed. Don't try to 
                // update the variable they reference.
                if (m_variableId.IsValid() && variableIds.count(m_variableId) > 0)
                {
                    return false;
                }

                return Node::RemoveVariableReferences(variableIds);
            }

            void GetVariableNode::SetId(const VariableId& variableDatumId)
            {
                if (m_variableId != variableDatumId)
                {
                    VariableId oldVariableId = m_variableId;
                    m_variableId = variableDatumId;

                    VariableNotificationBus::Handler::BusDisconnect();

                    ScriptCanvas::Data::Type oldType = ScriptCanvas::Data::Type::Invalid();

                    if (m_variableDataOutSlotId.IsValid())
                    {
                        oldType = GetSlotDataType(m_variableDataOutSlotId);
                    }

                    ScriptCanvas::Data::Type newType = ScriptCanvas::Data::Type::Invalid();
                    VariableRequestBus::EventResult(newType, GetScopedVariableId(), &VariableRequests::GetType);

                    if (oldType != newType)
                    {
                        ScopedBatchOperation scopedBatchOperation(AZ_CRC_CE("GetVariableIdChanged"));
                        RemoveOutputSlot();
                        AddOutputSlot();
                    }

                    if (m_variableId.IsValid())
                    {
                        VariableNotificationBus::Handler::BusConnect(GetScopedVariableId());
                    }

                    VariableNodeNotificationBus::Event(GetEntityId(), &VariableNodeNotifications::OnVariableIdChanged, oldVariableId, m_variableId);

                    PopulateNodeType();
                }
            }

            const VariableId& GetVariableNode::GetId() const
            {
                return m_variableId;
            }

            const SlotId& GetVariableNode::GetDataOutSlotId() const
            {
                return m_variableDataOutSlotId;
            }

            const Slot* GetVariableNode::GetVariableOutputSlot() const
            {
                return GetSlot(m_variableDataOutSlotId);
            }

            const Datum* GetVariableNode::GetDatum() const
            {
                const GraphVariable* graphVariable = FindGraphVariable(m_variableId);
                return graphVariable ? graphVariable->GetDatum() : nullptr;
            }

            void GetVariableNode::AddOutputSlot()
            {
                if (m_variableId.IsValid())
                {
                    GraphScopedVariableId scopedVariableId = GetScopedVariableId();

                    AZStd::string_view varName;
                    Data::Type varType;
                    VariableRequestBus::EventResult(varName, scopedVariableId, &VariableRequests::GetName);
                    VariableRequestBus::EventResult(varType, scopedVariableId, &VariableRequests::GetType);

                    {
                        DataSlotConfiguration slotConfiguration;

                        slotConfiguration.m_name = Data::GetName(varType);
                        slotConfiguration.SetConnectionType(ConnectionType::Output);
                        slotConfiguration.SetType(varType);

                        m_variableDataOutSlotId = AddSlot(slotConfiguration);
                    }

                    AddPropertySlots(varType);
                }
            }

            void GetVariableNode::AddPropertySlots(const Data::Type& type)
            {
                Data::GetterContainer getterFunctions = Data::ExplodeToGetters(type);
                for (const auto& getterWrapperPair : getterFunctions)
                {
                    const AZStd::string& propertyName = getterWrapperPair.first;
                    const Data::GetterWrapper& getterWrapper = getterWrapperPair.second;
                    Data::PropertyMetadata propertyAccount;
                    propertyAccount.m_propertyType = getterWrapper.m_propertyType;
                    propertyAccount.m_propertyName = propertyName;

                    {
                        DataSlotConfiguration slotConfiguration;

                        slotConfiguration.m_name = (getterWrapper.m_displayName.empty()) ? propertyName.data() : getterWrapper.m_displayName;
                        slotConfiguration.SetType(getterWrapper.m_propertyType);
                        slotConfiguration.SetConnectionType(ConnectionType::Output);

                        propertyAccount.m_propertySlotId = AddSlot(slotConfiguration);
                    }

                    propertyAccount.m_getterFunction = getterWrapper.m_getterFunction;
                    m_propertyAccounts.push_back(propertyAccount);
                }
            }

            void GetVariableNode::ClearPropertySlots()
            {
                auto oldPropertyAccounts = AZStd::move(m_propertyAccounts);
                m_propertyAccounts.clear();
                for (auto&& propertyAccount : oldPropertyAccounts)
                {
                    RemoveSlot(propertyAccount.m_propertySlotId);
                }
            }

            void GetVariableNode::RefreshPropertyFunctions()
            {
                GraphVariable* variable = FindGraphVariable(m_variableId);

                if (variable == nullptr)
                {
                    return;
                }

                Data::Type sourceType = variable->GetDataType();

                if (!sourceType.IsValid())
                {
                    return;
                }

                auto getterWrapperMap = Data::ExplodeToGetters(sourceType);

                for (auto&& propertyAccount : m_propertyAccounts)
                {
                    if (!propertyAccount.m_getterFunction)
                    {
                        auto foundPropIt = getterWrapperMap.find(propertyAccount.m_propertyName);
                        if (foundPropIt != getterWrapperMap.end() && propertyAccount.m_propertyType.IS_A(foundPropIt->second.m_propertyType))
                        {
                            propertyAccount.m_getterFunction = foundPropIt->second.m_getterFunction;
                        }
                        else
                        {
                            AZ_Error("Script Canvas", false, "Property (%s : %s) getter method could not be found in Data::PropertyTraits or the property type has changed."
                                " Output will not be pushed on the property's slot.",
                                propertyAccount.m_propertyName.c_str(), Data::GetName(propertyAccount.m_propertyType).data());
                        }
                    }
                }
            }

            void GetVariableNode::RemoveOutputSlot()
            {
                ClearPropertySlots();
                SlotId oldVariableDataOutSlotId;
                AZStd::swap(oldVariableDataOutSlotId, m_variableDataOutSlotId);

                if (oldVariableDataOutSlotId.IsValid())
                {
                    RemoveSlot(oldVariableDataOutSlotId);
                }
            }

            GraphScopedVariableId GetVariableNode::GetScopedVariableId() const
            {
                return GraphScopedVariableId(GetOwningScriptCanvasId(), m_variableId);
            }

            void GetVariableNode::OnIdChanged(const VariableId& oldVariableId)
            {
                if (m_variableId != oldVariableId)
                {
                    VariableId newVariableId = m_variableId;
                    m_variableId = oldVariableId;
                    SetId(newVariableId);
                }
            }

            AZStd::vector<AZStd::pair<VariableId, AZStd::string>> GetVariableNode::GetGraphVariables() const
            {
                AZStd::vector<AZStd::pair<VariableId, AZStd::string>> varNameToIdList;

                if (m_variableId.IsValid())
                {
                    ScriptCanvas::Data::Type baseType = ScriptCanvas::Data::Type::Invalid();
                    VariableRequestBus::EventResult(baseType, GetScopedVariableId(), &VariableRequests::GetType);

                    const GraphVariableMapping* variableMap = nullptr;
                    GraphRequestBus::EventResult(variableMap, GetOwningScriptCanvasId(), &GraphRequests::GetVariables);

                    if (variableMap && baseType.IsValid())
                    {
                        for (const auto& variablePair : *variableMap)
                        {
                            ScriptCanvas::Data::Type variableType = variablePair.second.GetDatum()->GetType();

                            if (variableType == baseType)
                            {
                                varNameToIdList.emplace_back(variablePair.first, variablePair.second.GetVariableName());
                            }
                        }

                        AZStd::sort(varNameToIdList.begin(), varNameToIdList.end(), [](const AZStd::pair<VariableId, AZStd::string>& lhs, const AZStd::pair<VariableId, AZStd::string>& rhs)
                        {
                            return lhs.second < rhs.second;
                        });
                    }
                }

                return varNameToIdList;
            }

            void GetVariableNode::OnVariableRemoved()
            {
                VariableNotificationBus::Handler::BusDisconnect();
                VariableId removedVariableId;
                AZStd::swap(removedVariableId, m_variableId);
                {
                    ScopedBatchOperation scopedBatchOperation(AZ_CRC_CE("GetVariableRemoved"));
                    RemoveOutputSlot();
                }
                VariableNodeNotificationBus::Event(GetEntityId(), &VariableNodeNotifications::OnVariableRemovedFromNode, removedVariableId);
            }

            VariableId GetVariableNode::GetVariableIdRead(const Slot*) const
            {
                return m_variableId;
            }
        }
    }
}
