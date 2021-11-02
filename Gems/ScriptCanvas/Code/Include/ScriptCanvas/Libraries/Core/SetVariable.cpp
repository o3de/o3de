/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SetVariable.h"

#include <Core/ExecutionNotificationsBus.h>
#include <Libraries/Core/MethodUtility.h>
#include <ScriptCanvas/Core/ScriptCanvasBus.h>
#include <ScriptCanvas/Debugger/ValidationEvents/DataValidation/DataValidationIds.h>
#include <ScriptCanvas/Grammar/ParsingUtilities.h>
#include <ScriptCanvas/Translation/GraphToLuaUtility.h>
#include <ScriptCanvas/Variable/VariableBus.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            AZ::Outcome<DependencyReport, void> SetVariableNode::GetDependencies() const
            {
                if (auto datum = ModVariable()->GetDatum())
                {
                    return AZ::Success(DependencyReport::NativeLibrary(Data::GetName(datum->GetType()).c_str()));
                }
                else
                {
                    return AZ::Failure();
                }
            }

            PropertyFields SetVariableNode::GetPropertyFields() const
            {
                PropertyFields propertyFields;
                for (auto&& propertyAccount : m_propertyAccounts)
                {
                    propertyFields.emplace_back(propertyAccount.m_propertyName, propertyAccount.m_propertySlotId);
                }

                return propertyFields;
            }

            void SetVariableNode::OnInit()
            {
                VariableNodeRequestBus::Handler::BusConnect(GetEntityId());
            }

            void SetVariableNode::OnPostActivate()
            {
                if (m_variableId.IsValid())
                {
                    RefreshPropertyFunctions();
                    PopulateNodeType();
                    VariableNotificationBus::Handler::BusConnect(GetScopedVariableId());
                }
            }

            VariableId SetVariableNode::GetVariableIdRead(const Slot*) const
            {
                return m_variableId;
            }

            VariableId SetVariableNode::GetVariableIdWritten(const Slot*) const
            {
                return m_variableId;
            }

            const Slot* SetVariableNode::GetVariableOutputSlot() const
            {
                return GetSlot(m_variableDataOutSlotId);
            }

            void SetVariableNode::CollectVariableReferences(AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds) const
            {
                if (m_variableId.IsValid())
                {
                    variableIds.insert(m_variableId);
                }

                return Node::CollectVariableReferences(variableIds);
            }

            bool SetVariableNode::ContainsReferencesToVariables(const AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds) const
            {
                if (m_variableId.IsValid() && variableIds.count(m_variableId) > 0)
                {
                    return true;
                }

                return Node::ContainsReferencesToVariables(variableIds);
            }

            bool SetVariableNode::RemoveVariableReferences(const AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds)
            {
                // These nodes should just be deleted when the variable they reference is removed. Don't try to 
                // update the variable they reference.
                if (m_variableId.IsValid() && variableIds.count(m_variableId) > 0)
                {
                    return false;
                }

                return Node::RemoveVariableReferences(variableIds);
            }

            void SetVariableNode::SetId(const VariableId& variableDatumId)
            {
                if (m_variableId != variableDatumId)
                {
                    VariableId oldVariableId = m_variableId;
                    m_variableId = variableDatumId;

                    VariableNotificationBus::Handler::BusDisconnect();

                    ScriptCanvas::Data::Type oldType = ScriptCanvas::Data::Type::Invalid();

                    if (m_variableDataInSlotId.IsValid())
                    {
                        oldType = GetSlotDataType(m_variableDataInSlotId);
                    }

                    ScriptCanvas::Data::Type newType = ScriptCanvas::Data::Type::Invalid();
                    VariableRequestBus::EventResult(newType, GetScopedVariableId(), &VariableRequests::GetType);

                    if (oldType != newType)
                    {
                        ScopedBatchOperation scopedBatchOperation(AZ_CRC("SetVariableIdChanged", 0xc072e633));
                        RemoveSlots();
                        AddSlots();
                    }

                    if (m_variableId.IsValid())
                    {
                        VariableNotificationBus::Handler::BusConnect(GetScopedVariableId());
                    }

                    VariableNodeNotificationBus::Event(GetEntityId(), &VariableNodeNotifications::OnVariableIdChanged, oldVariableId, m_variableId);

                    PopulateNodeType();
                }
            }

            const VariableId& SetVariableNode::GetId() const
            {
                return m_variableId;
            }

            const SlotId& SetVariableNode::GetDataInSlotId() const
            {
                return m_variableDataInSlotId;
            }

            const SlotId& SetVariableNode::GetDataOutSlotId() const
            {
                return m_variableDataOutSlotId;
            }

            GraphVariable* SetVariableNode::ModVariable() const
            {
                GraphVariable* graphVariable = FindGraphVariable(m_variableId);

                AZ_Warning("ScriptCanvas", graphVariable != nullptr, "Unknown variable referenced by Id - %s", m_variableId.ToString().data());
                AZ_Warning("ScriptCanvas", (graphVariable == nullptr || graphVariable->GetVariableId() == m_variableId), "Mismatch in SetVariableNode: VariableId %s requested but found VariableId %s", m_variableId.ToString().data(), graphVariable->GetVariableId().ToString().data());

                return graphVariable;
            }

            void SetVariableNode::AddSlots()
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
                        slotConfiguration.SetConnectionType(ConnectionType::Input);
                        slotConfiguration.ConfigureDatum(AZStd::move(Datum(varType, Datum::eOriginality::Copy)));

                        m_variableDataInSlotId = AddSlot(slotConfiguration);
                    }

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

            void SetVariableNode::RemoveSlots()
            {
                ClearPropertySlots();

                SlotId oldVariableDataInSlotId;
                AZStd::swap(oldVariableDataInSlotId, m_variableDataInSlotId);

                if (oldVariableDataInSlotId.IsValid())
                {
                    RemoveSlot(oldVariableDataInSlotId);
                }

                SlotId oldVariableDataOutSlotId;
                AZStd::swap(oldVariableDataOutSlotId, m_variableDataOutSlotId);

                if (oldVariableDataOutSlotId.IsValid())
                {
                    RemoveSlot(oldVariableDataOutSlotId);
                }
            }

            void SetVariableNode::OnIdChanged(const VariableId& oldVariableId)
            {
                if (m_variableId != oldVariableId)
                {
                    VariableId newVariableId = m_variableId;
                    m_variableId = oldVariableId;
                    SetId(newVariableId);
                }
            }

            AZStd::vector<AZStd::pair<VariableId, AZStd::string>> SetVariableNode::GetGraphVariables() const
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

            void SetVariableNode::OnVariableRemoved()
            {
                VariableNotificationBus::Handler::BusDisconnect();
                VariableId removedVariableId;
                AZStd::swap(removedVariableId, m_variableId);
                {
                    ScopedBatchOperation scopedBatchOperation(AZ_CRC("SetVariableRemoved", 0xd7da59f5));
                    RemoveSlots();
                }
                VariableNodeNotificationBus::Event(GetEntityId(), &VariableNodeNotifications::OnVariableRemovedFromNode, removedVariableId);
            }

            void SetVariableNode::AddPropertySlots(const Data::Type& type)
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

                        slotConfiguration.m_name = propertyName.data();
                        slotConfiguration.SetType(getterWrapper.m_propertyType);
                        slotConfiguration.SetConnectionType(ConnectionType::Output);

                        propertyAccount.m_propertySlotId = AddSlot(slotConfiguration);

                    }

                    propertyAccount.m_getterFunction = getterWrapper.m_getterFunction;
                    m_propertyAccounts.push_back(propertyAccount);
                }
            }

            void SetVariableNode::ClearPropertySlots()
            {
                auto oldPropertyAccounts = AZStd::move(m_propertyAccounts);
                m_propertyAccounts.clear();
                for (auto&& propertyAccount : oldPropertyAccounts)
                {
                    RemoveSlot(propertyAccount.m_propertySlotId);
                }
            }

            void SetVariableNode::RefreshPropertyFunctions()
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

            GraphScopedVariableId SetVariableNode::GetScopedVariableId() const
            {
                return GraphScopedVariableId(GetOwningScriptCanvasId(), m_variableId);
            }

        }
    }
}
