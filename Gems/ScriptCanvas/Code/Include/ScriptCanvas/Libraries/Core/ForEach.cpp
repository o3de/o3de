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

#include <ScriptCanvas/Libraries/Core/ForEach.h>
#include <ScriptCanvas/Libraries/Core/BehaviorContextObjectNode.h>

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

            bool ForEach::VersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement)
            {
                if (rootElement.GetVersion() <= 1)
                {
                    SlotMetadata metaData;

                    if (rootElement.FindSubElementAndGetData(AZ_CRC("m_sourceSlot", 0x7575d6c1), metaData))
                    {
                        rootElement.RemoveElementByName(AZ_CRC("m_sourceSlot", 0x7575d6c1));
                        rootElement.AddElementWithData(serializeContext, "m_sourceSlot", metaData.m_slotId);
                    }
                }

                return true;
            }

            void ForEach::OnInit()
            {
                ResetLoop();

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

            void ForEach::OnInputSignal(const SlotId& slotId)
            {
                auto inSlotId = ForEachProperty::GetInSlotId(this);
                if (slotId == inSlotId || slotId == SlotId{})
                {
                    if (slotId == inSlotId)
                    {
                        if (!InitializeLoop())
                        {
                            // Loop initialization failed
                            SignalOutput(ForEachProperty::GetFinishedSlotId(this));
                            return;
                        }
                    }

                    if (!m_breakCalled)
                    {
                        Iterate();
                    }
                }
                else if (slotId == ForEachProperty::GetBreakSlotId(this))
                {
                    m_breakCalled = true;
                    SignalOutput(ForEachProperty::GetFinishedSlotId(this));
                }
            }

            bool ForEach::InitializeLoop()
            {
                ResetLoop();

                const Datum* input = FindDatum(m_sourceSlot);

                if (input && !input->Empty())
                {
                    if (!Data::IsContainerType(input->GetType()))
                    {
                        SCRIPTCANVAS_REPORT_ERROR((*this), "Iteration not supported on this type: %s", Data::GetName(m_sourceContainer.GetType()).c_str());
                        return false;
                    }

                    // Make a copy of the source datum
                    m_sourceContainer = *input;

                    // Get the size of the container
                    auto sizeOutcome = BehaviorContextMethodHelper::CallMethodOnDatum(m_sourceContainer, "Size");

                    if (!sizeOutcome)
                    {
                        SCRIPTCANVAS_REPORT_ERROR((*this), "Failed to get size of container: %s", sizeOutcome.GetError().c_str());
                        return false;
                    }

                    Datum sizeResult = sizeOutcome.TakeValue();
                    const size_t* sizePtr = sizeResult.GetAs<size_t>();
                    m_size = sizePtr ? *sizePtr : 0;

                    if (Data::IsSetContainerType(m_sourceContainer.GetType()) || Data::IsMapContainerType(m_sourceContainer.GetType()))
                    {
                        // If it's a map or set, get the vector of keys
                        auto keysVectorOutcome = BehaviorContextMethodHelper::CallMethodOnDatum(m_sourceContainer, "GetKeys");

                        if (!keysVectorOutcome)
                        {
                            SCRIPTCANVAS_REPORT_ERROR((*this), "Failed to get vector of keys: %s", keysVectorOutcome.GetError().c_str());
                            return false;
                        }

                        m_keysVector = keysVectorOutcome.TakeValue();

                        // Check size of vector of keys for safety
                        auto keysSizeOutcome = BehaviorContextMethodHelper::CallMethodOnDatum(m_keysVector, "Size");

                        if (!keysSizeOutcome)
                        {
                            SCRIPTCANVAS_REPORT_ERROR((*this), "Failed to get size of vector of keys: %s", keysSizeOutcome.GetError().c_str());
                            return false;
                        }

                        Datum keysSizeResult = keysSizeOutcome.TakeValue();
                        const size_t* keysSizePtr = keysSizeResult.GetAs<size_t>();
                        size_t keysSize = keysSizePtr ? *keysSizePtr : 0;

                        if (m_size != keysSize)
                        {
                            // This shouldn't happen
                            SCRIPTCANVAS_REPORT_ERROR((*this), "Container size and vector of keys size mismatch.");
                            return false;
                        }
                    }

                    return true;
                }

                return false;
            }

            void ForEach::Iterate()
            {
                if (m_sourceContainer.Empty() || m_index >= m_size)
                {
                    SignalOutput(ForEachProperty::GetFinishedSlotId(this));
                    return;
                }

                Datum& container = Data::IsVectorContainerType(m_sourceContainer.GetType()) ? m_sourceContainer : m_keysVector;

                auto keyAtOutcome = BehaviorContextMethodHelper::CallMethodOnDatumUnpackOutcomeSuccess(container, "At", m_index);

                if (!keyAtOutcome)
                {
                    SCRIPTCANVAS_REPORT_ERROR((*this), "Failed to get key in container: %s", keyAtOutcome.GetError().c_str());
                    return;
                }

                Datum keyAtResult = keyAtOutcome.TakeValue();

                if (!SetPropertySlotData(keyAtResult, k_keySlotIndex))
                {
                    // Unable to set property slot
                    SCRIPTCANVAS_REPORT_ERROR((*this), "Unable to set one of the property slots on this node.");
                    SignalOutput(ForEachProperty::GetFinishedSlotId(this));
                    return;
                }

                if (Data::IsMapContainerType(m_sourceContainer.GetType()))
                {
                    // If the container is a map, we want to get the value for the current key
                    auto valueAtOutcome = BehaviorContextMethodHelper::CallMethodOnDatumUnpackOutcomeSuccess(m_sourceContainer, "At", keyAtResult);

                    if (!valueAtOutcome)
                    {
                        SCRIPTCANVAS_REPORT_ERROR((*this), "Failed to get value for key in container: %s", valueAtOutcome.GetError().c_str());
                        return;
                    }

                    Datum valueAtResult = valueAtOutcome.TakeValue();

                    if (!SetPropertySlotData(valueAtResult, k_valueSlotIndex))
                    {
                        // Unable to set property slot
                        SCRIPTCANVAS_REPORT_ERROR((*this), "Unable to set one of the property slots on this node.");
                        SignalOutput(ForEachProperty::GetFinishedSlotId(this));
                        return;
                    }
                }

                ++m_index;
                GetExecutionBus()->AddToExecutionStack((*this), SlotId());
                SignalOutput(ForEachProperty::GetEachSlotId(this));
            }

            bool ForEach::SetPropertySlotData(Datum& atResult, size_t propertyIndex)
            {
                if (atResult.Empty())
                {
                    // Something went wrong with the Behavior Context call
                    SCRIPTCANVAS_REPORT_ERROR((*this), "Behavior Context call failed; unable to retrieve element from container.");
                    return false;
                }

                if (m_propertySlots.size() <= propertyIndex)
                {
                    // Missing a property slot
                    SCRIPTCANVAS_REPORT_ERROR((*this), "Node in invalid state; missing a property slot.");
                    return false;
                }

                PushOutput(atResult, *GetSlot(m_propertySlots[propertyIndex].m_propertySlotId));
                return true;
            }

            void ForEach::ResetLoop()
            {
                // Reset node state
                m_index = 0;
                m_size = 0;
                m_breakCalled = false;
                m_sourceContainer = Datum();
                m_keysVector = Datum();
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
                        ClearPropertySlots();

                        m_previousTypeId = newType;

                        AZStd::vector<Data::Type> types = Data::GetContainedTypes(dataType);
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
