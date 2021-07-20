/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptCanvas/Libraries/Core/ForEach.h>
#include <ScriptCanvas/Libraries/Core/BehaviorContextObjectNode.h>
#include <ScriptCanvas/Utils/VersionConverters.h>

#include <Libraries/Core/MethodUtility.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            // Slot indices for standard containers
            const size_t ForEach::k_keySlotIndex = 0;
            const size_t ForEach::k_valueSlotIndex = 1;

            AZ::Outcome<DependencyReport, void> ForEach::GetDependencies() const
            {
                return AZ::Success(DependencyReport{});
            }

            SlotId ForEach::GetLoopBreakSlotId() const
            {
                return ForEachProperty::GetBreakSlotId(const_cast<ForEach*>(this));
            }

            SlotId ForEach::GetLoopFinishSlotId() const
            {
                return ForEachProperty::GetFinishedSlotId(const_cast<ForEach*>(this));
            }

            SlotId ForEach::GetLoopSlotId() const
            {
                return ForEachProperty::GetEachSlotId(const_cast<ForEach*>(this));
            }

            Data::Type ForEach::GetKeySlotDataType() const
            {
                return m_propertySlots[k_keySlotIndex].m_propertyType;
            }

            SlotId ForEach::GetKeySlotId() const
            {
                AZ_Error("ScriptCanvas", m_propertySlots.size() == 2, "not enough property slots for a key slot");
                return m_propertySlots.size() == 2 ? m_propertySlots[k_keySlotIndex].m_propertySlotId : SlotId();
            }

            ExecutionNameMap ForEach::GetExecutionNameMap() const
            {
                return { { "In", "Each" }, { "In", "Finished" },  { "Break", "Finished" } };
            }

            Data::Type ForEach::GetValueSlotDataType() const
            {
                return m_propertySlots.size() == 2 ? m_propertySlots[k_valueSlotIndex].m_propertyType : m_propertySlots[k_keySlotIndex].m_propertyType;
            }

            SlotId ForEach::GetValueSlotId() const
            {
                return m_propertySlots.size() == 2 ? m_propertySlots[k_valueSlotIndex].m_propertySlotId : m_propertySlots[k_keySlotIndex].m_propertySlotId;
            }

            bool ForEach::IsFormalLoop() const
            {
                return true;
            }

            bool ForEach::IsBreakSlot(const SlotId& checkSlotId) const
            {
                auto breakSlot = this->GetSlotByName("Break");
                return breakSlot ? breakSlot->GetId() == checkSlotId : false;
            }

            bool ForEach::IsOutOfDate(const VersionData& /*graphVersion*/) const
            {
                if (GetSlotByNameAndType("Continue", CombinedSlotType::ExecutionIn))
                {
                    return true;
                }
                return false;
            }

            void ForEach::OnInit()
            {
                if (!m_sourceSlot.IsValid())
                {
                    DynamicDataSlotConfiguration slotConfiguration;

                    slotConfiguration.m_name = GetSourceSlotName();
                    slotConfiguration.m_dynamicDataType = DynamicDataType::Container;
                    slotConfiguration.m_dynamicGroup = GetContainerGroupId();
                    slotConfiguration.SetConnectionType(ConnectionType::Input);                    

                    m_sourceSlot = AddSlot(slotConfiguration);
                }
                // DYNAMIC_SLOT_VERSION_CONVERTER
                
                {
                    Slot* slot = GetSlot(m_sourceSlot);

                    if (slot)
                    {
                        if (!slot->IsDynamicSlot())
                        {
                            slot->SetDynamicDataType(DynamicDataType::Container);
                        }

                        if (slot->GetDynamicGroup() == AZ::Crc32())
                        {
                            SetDynamicGroup(slot->GetId(), GetContainerGroupId());
                        }
                    }
                }
                ////

                EndpointNotificationBus::Handler::BusConnect({ GetEntityId(), m_sourceSlot });
            }

            UpdateResult ForEach::OnUpdateNode()
            {
                if (auto continueSlot = GetSlotByNameAndType("Continue", CombinedSlotType::ExecutionIn))
                {
                    RemoveSlot(continueSlot->GetId());
                }

                return UpdateResult::DirtyGraph;
            }

            void ForEach::OnDynamicGroupDisplayTypeChanged(const AZ::Crc32& dynamicGroup, const Data::Type& dataType)
            {
                if (dynamicGroup == GetContainerGroupId() && dataType.IsValid())
                {
                    AddPropertySlotsFromType(dataType);
                }
            }

            void ForEach::ClearPropertySlots()
            {
                for (auto&& propertyAccount : m_propertySlots)
                {
                    RemoveSlot(propertyAccount.m_propertySlotId);
                }
                m_propertySlots.clear();
            }

            void ForEach::AddPropertySlotsFromType(const Data::Type& dataType)
            {
                if (Data::IsContainerType(dataType))
                {
                    AZ::TypeId newType = ScriptCanvas::Data::ToAZType(dataType);

                    if (newType != m_previousTypeId)
                    {
                        AZStd::vector<Data::Type> types = Data::GetContainedTypes(dataType);

                        if (types.size() == m_propertySlots.size())
                        {
                            bool allTypesMatch = true;
                            for (size_t slotIndex = 0; slotIndex < m_propertySlots.size(); ++slotIndex)
                            {
                                if (m_propertySlots[slotIndex].m_propertyType != types[slotIndex])
                                {
                                    allTypesMatch = false;
                                    break;
                                }
                            }

                            if (allTypesMatch)
                            {
                                return;
                            }
                        }

                        ClearPropertySlots();

                        m_previousTypeId = newType;

                        for (size_t i = 0; i < types.size(); ++i)
                        {
                            Data::PropertyMetadata propertyAccount;
                            propertyAccount.m_propertyType = types[i];
                            propertyAccount.m_propertyName = Data::GetName(types[i]);

                            DataSlotConfiguration slotConfiguration;

                            slotConfiguration.m_name = propertyAccount.m_propertyName;
                            slotConfiguration.m_toolTip = "";
                            slotConfiguration.SetConnectionType(ConnectionType::Output);
                            slotConfiguration.m_addUniqueSlotByNameAndType = false;                            

                            slotConfiguration.SetType(types[i]);

                            propertyAccount.m_propertySlotId = AddSlot(slotConfiguration);
                            m_propertySlots.push_back(propertyAccount);
                        }
                    }
                }
            }
        }
    }
}
