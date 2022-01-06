/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StringFormatted.h"

#include <AzCore/Component/TickBus.h>
#include <AzCore/Serialization/Utils.h>

#include <ScriptCanvas/Core/Graph.h>

AZ_DECLARE_BUDGET(ScriptCanvas);

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Internal
        {
            ////////////////////
            // StringFormatted
            ////////////////////

            void StringFormatted::OnInit()
            {
                bool addedDisplayGroup = false;

                // DISPLAY_GROUP_VERSION_CONVERTER
                for (auto slotPair : m_formatSlotMap)
                {
                    Slot* slot = GetSlot(slotPair.second);

                    if (slot)
                    {
                        // DYNAMIC_SLOT_VERSION_CONVERTER
                        if (!slot->IsDynamicSlot())
                        {
                            slot->SetDynamicDataType(DynamicDataType::Any);
                        }
                        ////

                        // DISPLAY_GROUP_VERSION_CONVERTER   
                        if (slot->GetDisplayGroup() != GetDisplayGroupId())
                        {
                            addedDisplayGroup = true;
                        }
                        ////
                    }
                }

                if (addedDisplayGroup)
                {
                    RelayoutNode();
                }

                m_stringInterface.SetPropertyReference(&m_format);
                m_stringInterface.RegisterListener(this);
            }

            void StringFormatted::OnConfigured()
            {
                // In configure, we want to parse to ensure our slots are setup for when we are added to the graph and initialized.
                if (m_formatSlotMap.empty())
                {
                    ParseFormat();
                }
            }

            void StringFormatted::OnDeserialized()
            {
                ParseFormat();
            }

            void StringFormatted::ConfigureVisualExtensions()
            {
                {
                    VisualExtensionSlotConfiguration visualExtensions(VisualExtensionSlotConfiguration::VisualExtensionType::ExtenderSlot);

                    visualExtensions.m_name = "Add Input";
                    visualExtensions.m_tooltip = "Adds an input to the current string format";
                    visualExtensions.m_displayGroup = GetDisplayGroup();
                    visualExtensions.m_connectionType = ConnectionType::Input;

                    visualExtensions.m_identifier = GetExtensionId();

                    RegisterExtension(visualExtensions);
                }

                {
                    VisualExtensionSlotConfiguration visualExtensions(VisualExtensionSlotConfiguration::VisualExtensionType::PropertySlot);

                    visualExtensions.m_name = "";
                    visualExtensions.m_tooltip = "";
                    visualExtensions.m_displayGroup = GetDisplayGroup();
                    visualExtensions.m_connectionType = ConnectionType::Input;

                    visualExtensions.m_identifier = GetPropertyId();

                    RegisterExtension(visualExtensions);
                }
            }

            bool StringFormatted::CanDeleteSlot(const SlotId& slotId) const
            {
                Slot* slot = GetSlot(slotId);

                bool canDeleteSlot = false;

                if (slot && slot->IsData() && slot->IsInput())
                {
                    canDeleteSlot = (slot->GetDisplayGroup() == GetDisplayGroupId());
                }

                return canDeleteSlot;
            }

            SlotId StringFormatted::HandleExtension(AZ::Crc32 extensionId)
            {
                if (extensionId == GetExtensionId())
                {
                    int value = 0;
                    AZStd::string name = "Value";

                    while (m_formatSlotMap.find(name) != m_formatSlotMap.end())
                    {
                        ++value;
                        name = AZStd::string::format("Value_%i", value);
                    }

                    m_format.append("{");
                    m_format.append(name);
                    m_format.append("}");

                    {
                        m_isHandlingExtension = true;
                        m_stringInterface.SignalDataChanged();
                        m_isHandlingExtension = false;
                    }

                    auto formatMapIter = m_formatSlotMap.find(name);

                    if (formatMapIter != m_formatSlotMap.end())
                    {
                        return formatMapIter->second;
                    }
                }

                return SlotId();
            }

            NodePropertyInterface* StringFormatted::GetPropertyInterface(AZ::Crc32 propertyId)
            {
                if (propertyId == GetPropertyId())
                {
                    return &m_stringInterface;
                }

                return nullptr;
            }

            void StringFormatted::OnSlotRemoved(const SlotId& slotId)
            {
                if (m_parsingFormat)
                {
                    return;
                }

                for (auto slotPair : m_formatSlotMap)
                {
                    if (slotPair.second == slotId)
                    {
                        AZStd::string name = AZStd::string::format("{%s}", slotPair.first.c_str());

                        AZStd::size_t firstInstance = m_format.find(name);

                        if (firstInstance == AZStd::string::npos)
                        {
                            return;
                        }

                        m_format.erase(firstInstance, name.size());
                        m_formatSlotMap.erase(slotPair.first);
                        break;
                    }
                }

                m_stringInterface.SignalDataChanged();
            }

            AZStd::string StringFormatted::ProcessFormat()
            {
                AZ_PROFILE_SCOPE(ScriptCanvas, "ScriptCanvas::StringFormatted::ProcessFormat");

                AZStd::string text;

                if (!m_format.empty())
                {
                    for (const auto& binding : m_arrayBindingMap)
                    {
                        SlotId slot = binding.second;
                        if (slot.IsValid())
                        {
                            const Datum* datum = FindDatum(slot);

                            // Now push the value into the formatted string at the right spot
                            AZStd::string result;
                            if (datum)
                            {
                                if (datum->GetType().IsValid() && datum->IS_A(Data::Type::Number()))
                                {
                                    m_unresolvedString[binding.first] = AZStd::string::format("%.*lf", m_numericPrecision, (*datum->GetAs<Data::NumberType>()));
                                }
                                else if (datum->GetType().IsValid() && datum->ToString(result))
                                {
                                    m_unresolvedString[binding.first] = result;
                                }
                            }
                        }
                    }

                    for (const auto& str : m_unresolvedString)
                    {
                        text.append(str);
                    }
                }

                return text;
            }

            void StringFormatted::OnPropertyChanged()
            {
                ParseFormat();
            }

            void StringFormatted::RelayoutNode()
            {
                // We have an in and an our slot. So we want to ensure we start off with 2 slots.
                int slotOrder = static_cast<int>(GetSlots().size() - m_formatSlotMap.size()) - 1;

                m_parsingFormat = true;
                for (const auto& entryPair : m_formatSlotMap)
                {
                    const bool removeConnections = false;
                    RemoveSlot(entryPair.second, removeConnections);
                }
                m_parsingFormat = false;

                AZStd::smatch match;
                
                AZStd::string format = m_format;
                while (AZStd::regex_search(format, match, GetRegex()))
                {
                    AZStd::string name = match[1].str();
                    AZStd::string tooltip = AZStd::string::format("Value which replaces instances of {%s} in the resulting string.", match[1].str().c_str());

                    auto formatSlotIter = m_formatSlotMap.find(name);

                    if (formatSlotIter == m_formatSlotMap.end())
                    {
                        continue;
                    }

                    DynamicDataSlotConfiguration dataSlotConfiguration;

                    dataSlotConfiguration.m_name = name;
                    dataSlotConfiguration.m_toolTip = tooltip;
                    dataSlotConfiguration.m_displayGroup = GetDisplayGroup();

                    dataSlotConfiguration.SetConnectionType(ConnectionType::Input);
                    dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Any;

                    dataSlotConfiguration.m_addUniqueSlotByNameAndType = true;

                    dataSlotConfiguration.m_slotId = formatSlotIter->second;

                    InsertSlot(slotOrder, dataSlotConfiguration);

                    format = match.suffix();

                    ++slotOrder;
                }

                SignalSlotsReordered();
            }

            void StringFormatted::ParseFormat()
            {
                // Used to defer the removal of slots and only remove those slots that actually need to be removed at the end of the format parsing operation.                
                AZStd::unordered_set<SlotId> slotsToRemove;
                AZStd::unordered_map<SlotId, ScriptCanvas::VariableId> slotVariableReferences;

                // When this node is duplicated. It recreates all of the slots, but discards the display type data.
                // This causes the sanity checking to fail, which in turn causes the display type in graph canvas to be orphaned.
                // Going to maintain the dynamic display type, and let the sanity check handle removing it rather then
                // it occurring through new slot creation.
                AZStd::unordered_map<SlotId, ScriptCanvas::Data::Type> displayTypes;

                m_parsingFormat = true;

                // Mark all existing slots as potential candidates for removal.
                for (const auto& entry : m_formatSlotMap)
                {
                    const Slot* slot = GetSlot(entry.second);

                    if (!slot)
                    {
                        continue;
                    }

                    if (slot->IsVariableReference())
                    {
                        slotVariableReferences[entry.second] = slot->GetVariableReference();
                    }

                    displayTypes[entry.second] = slot->GetDisplayType();

                    const bool signalRemoval = false;
                    RemoveSlot(entry.second, signalRemoval);

                    slotsToRemove.insert(entry.second);
                }

                m_parsingFormat = false;

                // Going to move around some of the other slots here. But this should at least make it consistent no mater what
                // element was using it.
                int slotOrder = aznumeric_cast<int>(GetSlots().size()) - 1;

                if (slotOrder < 0)
                {
                    slotOrder = 0;
                }

                // Clear the utility containers.
                m_arrayBindingMap.clear();
                m_unresolvedString.clear();

                static AZStd::regex formatRegex(R"(\{(\w+)\})");
                AZStd::smatch match;

                AZStd::string format = m_format;

                NamedSlotIdMap newMapping;

                while (AZStd::regex_search(format, match, formatRegex))
                {
                    m_unresolvedString.push_back(match.prefix().str());

                    AZStd::string name = match[1].str();
                    AZStd::string tooltip = AZStd::string::format("Value which replaces instances of {%s} in the resulting string.", match[1].str().c_str());

                    SlotId slotId;

                    auto mappingIter = newMapping.find(name);

                    if (mappingIter == newMapping.end())
                    {
                        auto slotIdIter = m_formatSlotMap.find(name);

                        if (GetSlotByName(name.c_str()) != nullptr)
                        {
                            const auto& slotList = GetSlots();

                            AZStd::string reservedSlotNames;

                            for (const auto& slot : slotList)
                            {
                                if (newMapping.find(slot.GetName()) == newMapping.end())
                                {
                                    if (!reservedSlotNames.empty())
                                    {
                                        reservedSlotNames.append(", ");
                                    }

                                    reservedSlotNames.append(slot.GetName());
                                }
                            }

                            format = match.suffix();

                            AZStd::string errorReport = AZStd::string::format("Attempting to use one of the reserved names '%s' in string display. Skipping input name", reservedSlotNames.c_str());
                            GetGraph()->ReportError((*this), GetNodeName(),errorReport);
                            continue;
                        }

                        // If the slot has never existed, create it
                        DynamicDataSlotConfiguration dataSlotConfiguration;

                        dataSlotConfiguration.m_name = name;
                        dataSlotConfiguration.m_toolTip = tooltip;
                        dataSlotConfiguration.m_displayGroup = GetDisplayGroup();

                        dataSlotConfiguration.SetConnectionType(ConnectionType::Input);
                        dataSlotConfiguration.m_dynamicDataType = DynamicDataType::Any;
                        dataSlotConfiguration.m_addUniqueSlotByNameAndType = true;

                        bool isNewSlot = true;

                        if (slotIdIter != m_formatSlotMap.end())
                        {
                            isNewSlot = false;
                            dataSlotConfiguration.m_slotId = slotIdIter->second;
                            slotId = slotIdIter->second;
                        }

                        auto pair = newMapping.emplace(AZStd::make_pair(name, InsertSlot(slotOrder, dataSlotConfiguration, isNewSlot)));
                        slotId = pair.first->second;

                        slotsToRemove.erase(slotId);

                        Slot* slot = GetSlot(slotId);

                        if (slot)
                        {
                            auto referenceIter = slotVariableReferences.find(slotId);

                            if (referenceIter != slotVariableReferences.end())
                            {
                                slot->SetVariableReference(referenceIter->second);
                            }

                            auto displayTypeIter = displayTypes.find(slotId);

                            if (displayTypeIter != displayTypes.end())
                            {
                                slot->SetDisplayType(displayTypeIter->second);
                            }
                        }
                    }
                    else
                    {
                        slotId = mappingIter->second;
                    }

                    m_arrayBindingMap.emplace(AZStd::make_pair(m_unresolvedString.size(), slotId));
                    m_unresolvedString.push_back(""); // blank placeholder, will be filled when the data slot is resolved.

                    format = match.suffix();

                    ++slotOrder;
                }

                for (auto slotId : slotsToRemove)
                {
                    SignalSlotRemoved(slotId);
                }

                m_formatSlotMap = AZStd::move(newMapping);

                // If there's some left over in the match make sure it gets recorded.
                if (!format.empty())
                {
                    m_unresolvedString.push_back(format);
                }

                SignalSlotsReordered();
            }

            void StringFormatted::OnFormatChanged()
            {
                m_stringInterface.SignalDataChanged();
            }
        }
    }
}
