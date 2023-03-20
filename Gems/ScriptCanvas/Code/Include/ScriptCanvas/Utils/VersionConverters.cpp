/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/Utils.h>

#include <ScriptCanvas/Asset/SubgraphInterfaceAsset.h>
#include <ScriptCanvas/Core/Contracts/DisallowReentrantExecutionContract.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/NodeableNode.h>
#include <ScriptCanvas/Core/SlotMetadata.h>
#include <ScriptCanvas/Internal/Nodes/BaseTimerNode.h>
#include <ScriptCanvas/Libraries/Core/EBusEventHandler.h>
#include <ScriptCanvas/Libraries/Deprecated/Time/Repeater.h>
#include <ScriptCanvas/Utils/SerializationUtils.h>
#include <ScriptCanvas/Utils/VersionConverters.h>

namespace ScriptCanvas
{
    bool VersionConverters::ContainsStringVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement)
    {
        if (rootElement.GetVersion() < 1)
        {
            auto slotContainerElements = AZ::Utils::FindDescendantElements(context, rootElement, AZStd::vector<AZ::Crc32>{AZ_CRC("BaseClass1", 0xd4925735), AZ_CRC("Slots", 0xc87435d0), AZ_CRC("element", 0x41405e39)});
            for (auto slotElement : slotContainerElements)
            {
                AZStd::string slotName;
                if (slotElement->GetChildData(AZ_CRC("slotName", 0x817c3511), slotName))
                {
                    if (slotName == "Ignore Case")
                    {
                        slotElement->RemoveElementByName(AZ_CRC("slotName", 0x817c3511));
                        slotName = "Case Sensitive";
                        if (slotElement->AddElementWithData(context, "slotName", slotName) == -1)
                        {
                            AZ_Assert(false, "Unable to add slotName data in Contains String Node version %u.", rootElement.GetVersion());
                            return false;
                        }
                        break;
                    }
                }
                else
                {
                    AZ_Assert(false, "Unable to retrieve slotName data in Contains String Node version %u.", rootElement.GetVersion());
                    return false;
                }
            }
        }

        return true;
    }

    bool VersionConverters::DelayVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement)
    {
        // Fixed issue with out pins not being correctly marked as latent.
        if (rootElement.GetVersion() < 2)
        {
            const char* slotsKey = "Slots";
            AZ::Crc32 slotsId = AZ::Crc32(slotsKey);

            AZStd::list< ScriptCanvas::Slot > nodeSlots;

            AZ::SerializeContext::DataElementNode* baseClassElement = rootElement.FindSubElement(AZ::Crc32("BaseClass1"));
            AZ::SerializeContext::DataElementNode* dataNode = baseClassElement->FindSubElement(slotsId);

            if (dataNode && dataNode->GetData(nodeSlots))
            {
                baseClassElement->RemoveElementByName(slotsId);

                for (ScriptCanvas::Slot& slot : nodeSlots)
                {
                    if (slot.GetName() == "Out")
                    {
                        slot.ConvertToLatentExecutionOut();
                    }
                }

                baseClassElement->AddElementWithData(context, slotsKey, nodeSlots);
            }
        }

        if (rootElement.GetVersion() < 3)
        {
            AZ::SerializeContext::DataElementNode* nodeElement = rootElement.FindSubElement(AZ_CRC("BaseClass1", 0xd4925735));
            if (!nodeElement)
            {
                AZ_Error("Script Canvas", false, "Unable to retrieve the node structure in Delay Node version %u.", rootElement.GetVersion());
                return false;
            }

            Node delayNode;
            if (nodeElement->GetData(delayNode))
            {
                rootElement.RemoveElementByName(AZ_CRC("BaseClass1", 0xd4925735));

                ExecutionSlotConfiguration cancelSlotConfiguration;
                cancelSlotConfiguration.m_name = "Cancel";
                cancelSlotConfiguration.m_toolTip = "Cancels the current delay.";
                cancelSlotConfiguration.SetConnectionType(ScriptCanvas::ConnectionType::Input);
                cancelSlotConfiguration.m_contractDescs = AZStd::vector<ScriptCanvas::ContractDescriptor>
                {
                    // Contract: DisallowReentrantExecutionContract
                    { []() { return aznew DisallowReentrantExecutionContract; } }
                };
                delayNode.AddSlot(cancelSlotConfiguration);

                if (rootElement.AddElementWithData(context, "BaseClass1", delayNode) == -1)
                {
                    AZ_Error("Script Canvas", false, "Unable to add the node data in Delay Node version %u.", rootElement.GetVersion());
                    return false;
                }
            }
            else
            {
                AZ_Error("Script Canvas", false, "Unable to retrieve the node data in Delay Node version %u.", rootElement.GetVersion());
                return false;
            }
        }

        return true;
    }

    bool VersionConverters::DurationVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement)
    {
        // Fixed issue with out and done pins not being correctly marked as latent.
        if (rootElement.GetVersion() < 2)
        {
            const char* slotsKey = "Slots";
            AZ::Crc32 slotsId = AZ::Crc32(slotsKey);

            AZStd::list< ScriptCanvas::Slot > nodeSlots;

            AZ::SerializeContext::DataElementNode* baseClassElement = rootElement.FindSubElement(AZ::Crc32("BaseClass1"));
            AZ::SerializeContext::DataElementNode* dataNode = baseClassElement->FindSubElement(slotsId);

            if (dataNode && dataNode->GetData(nodeSlots))
            {
                baseClassElement->RemoveElementByName(slotsId);

                for (ScriptCanvas::Slot& slot : nodeSlots)
                {
                    if (slot.GetName() == "Out"
                        || slot.GetName() == "Done")
                    {
                        slot.ConvertToLatentExecutionOut();
                    }
                }

                baseClassElement->AddElementWithData(context, slotsKey, nodeSlots);
            }
        }

        return true;
    }

    bool VersionConverters::EBusEventHandlerVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement)
    {
        if (rootElement.GetVersion() <= 3)
        {
            if (auto element = rootElement.FindSubElement(AZ_CRC("m_ebusName", 0x74a03d75)))
            {
                AZStd::string ebusName;
                element->GetData(ebusName);

                if (!ebusName.empty())
                {
                    ScriptCanvas::EBusBusId busId = ScriptCanvas::EBusBusId(ebusName.c_str());

                    if (rootElement.AddElementWithData(context, "m_busId", busId) == -1)
                    {
                        return false;
                    }
                }
            }
            else
            {
                return false;
            }
        }

        if (rootElement.GetVersion() <= 2)
        {
            // Renamed "BusId" to "Source"

            auto slotContainerElements = AZ::Utils::FindDescendantElements(context, rootElement, AZStd::vector<AZ::Crc32>{AZ_CRC("BaseClass1", 0xd4925735), AZ_CRC("Slots", 0xc87435d0)});
            if (!slotContainerElements.empty())
            {
                // This contains the pair elements stored in the SlotNameMap(Name -> Index)
                auto slotNameToIndexElements = AZ::Utils::FindDescendantElements(context, *slotContainerElements.front(), AZStd::vector<AZ::Crc32>{AZ_CRC("m_slotNameSlotMap", 0x69040afb), AZ_CRC("element", 0x41405e39)});

                // This contains the Slot class elements stored in Node class
                auto slotElements = AZ::Utils::FindDescendantElements(context, *slotContainerElements.front(), AZStd::vector<AZ::Crc32>{AZ_CRC("m_slots", 0x84838ab4), AZ_CRC("element", 0x41405e39)});

                AZStd::string slotName;
                for (auto slotNameToIndexElement : slotNameToIndexElements)
                {
                    int index = -1;
                    if (slotNameToIndexElement->GetChildData(AZ_CRC("value1", 0xa2756c5a), slotName) && slotName == "BusId" && slotNameToIndexElement->GetChildData(AZ_CRC("value2", 0x3b7c3de0), index))
                    {
                        CombinedSlotType slotType;
                        if (slotElements.size() > static_cast<size_t>(index) && slotElements[index]->GetChildData(AZ_CRC("type", 0x8cde5729), slotType) && slotType == CombinedSlotType::DataIn)
                        {
                            AZ::SerializeContext::DataElementNode& slotElement = *slotElements[index];

                            slotName = "Source";
                            slotElement.RemoveElementByName(AZ_CRC("slotName", 0x817c3511));
                            if (slotElement.AddElementWithData(context, "slotName", slotName) == -1)
                            {
                                AZ_Assert(false, "Version Converter failed. Graph contained %s node is in an invalid state", AZ::AzTypeInfo<Nodes::Core::EBusEventHandler>::Name());
                                return false;
                            }

                            if (slotNameToIndexElement->AddElementWithData(context, "value1", slotName) == -1)
                            {
                                AZ_Assert(false, "Version Converter failed. Graph contained %s node is in an invalid state", AZ::AzTypeInfo<Nodes::Core::EBusEventHandler>::Name());
                                return false;
                            }
                        }
                    }
                }
            }
        }

        if (rootElement.GetVersion() == 2)
        {
            auto element = rootElement.FindSubElement(AZ_CRC("m_eventMap", 0x8b413178));

            AZStd::unordered_map<AZ::Crc32, Nodes::Core::EBusEventEntry> entryMap;
            element->GetDataHierarchy(context, entryMap);

            Nodes::Core::EBusEventHandler::EventMap eventMap;

            for (const auto& entryElement : entryMap)
            {
                AZ_Assert(eventMap.find(entryElement.first) == eventMap.end(), "Duplicated event found while converting EBusEventHandler from version 2 to 3");

                eventMap[entryElement.first] = entryElement.second;
            }

            rootElement.RemoveElementByName(AZ_CRC("m_eventMap", 0x8b413178));
            if (rootElement.AddElementWithData(context, "m_eventMap", eventMap) == -1)
            {
                return false;
            }

            return true;
        }
        else if (rootElement.GetVersion() <= 1)
        {
            // Changed:
            //  using Events = AZStd::vector<EBusEventEntry>;
            //  to:
            //  using EventMap = AZStd::map<AZ::Crc32, EBusEventEntry>;

            auto ebusEventEntryElements = AZ::Utils::FindDescendantElements(context, rootElement, AZStd::vector<AZ::Crc32>{AZ_CRC("m_events", 0x191405b4), AZ_CRC("element", 0x41405e39)});

            Nodes::Core::EBusEventHandler::EventMap eventMap;
            for (AZ::SerializeContext::DataElementNode* ebusEventEntryElement : ebusEventEntryElements)
            {
                Nodes::Core::EBusEventEntry eventEntry;
                if (!ebusEventEntryElement->GetData(eventEntry))
                {
                    return false;
                }
                AZ::Crc32 key = AZ::Crc32(eventEntry.m_eventName.c_str());
                AZ_Assert(eventMap.find(key) == eventMap.end(), "Duplicated event found while converting EBusEventHandler from version 1 to 3.");
                eventMap[key] = eventEntry;
            }

            rootElement.RemoveElementByName(AZ_CRC("m_events", 0x191405b4));
            if (rootElement.AddElementWithData(context, "m_eventMap", eventMap) == -1)
            {
                return false;
            }

            return true;
        }

        if (rootElement.GetVersion() == 0)
        {
            auto slotContainerElements = AZ::Utils::FindDescendantElements(context, rootElement, AZStd::vector<AZ::Crc32>{AZ_CRC("BaseClass1", 0xd4925735), AZ_CRC("Slots", 0xc87435d0)});
            if (!slotContainerElements.empty())
            {
                // This contains the pair elements stored in the SlotNameMap(Name -> Index)
                auto slotNameToIndexElements = AZ::Utils::FindDescendantElements(context, *slotContainerElements.front(), AZStd::vector<AZ::Crc32>{AZ_CRC("m_slotNameSlotMap", 0x69040afb), AZ_CRC("element", 0x41405e39)});
                // This contains the Slot class elements stored in Node class
                auto slotElements = AZ::Utils::FindDescendantElements(context, *slotContainerElements.front(), AZStd::vector<AZ::Crc32>{AZ_CRC("m_slots", 0x84838ab4), AZ_CRC("element", 0x41405e39)});

                for (auto slotNameToIndexElement : slotNameToIndexElements)
                {
                    AZStd::string slotName;
                    int index = -1;
                    if (slotNameToIndexElement->GetChildData(AZ_CRC("value1", 0xa2756c5a), slotName) && slotName == "EntityId" && slotNameToIndexElement->GetChildData(AZ_CRC("value2", 0x3b7c3de0), index))
                    {
                        CombinedSlotType slotType;
                        if (slotElements.size() > static_cast<size_t>(index) && slotElements[index]->GetChildData(AZ_CRC("type", 0x8cde5729), slotType) && slotType == CombinedSlotType::DataIn)
                        {
                            AZ::SerializeContext::DataElementNode& slotElement = *slotElements[index];

                            slotName = "BusId";
                            slotElement.RemoveElementByName(AZ_CRC("slotName", 0x817c3511));
                            if (slotElement.AddElementWithData(context, "slotName", slotName) == -1)
                            {
                                AZ_Assert(false, "Version Converter failed. Graph contained %s node is in an invalid state", AZ::AzTypeInfo<Nodes::Core::EBusEventHandler>::Name());
                                return false;
                            }

                            if (slotNameToIndexElement->AddElementWithData(context, "value1", slotName) == -1)
                            {
                                AZ_Assert(false, "Version Converter failed. Graph contained %s node is in an invalid state", AZ::AzTypeInfo<Nodes::Core::EBusEventHandler>::Name());
                                return false;
                            }
                        }
                    }
                }
            }
        }

        if (rootElement.GetVersion() < 5)
        {
            if (!MarkSlotAsLatent(context, rootElement, { k_EventOutPrefix, "Handle:" }))
            {
                return false;
            }
        }

        return true;
    }

    bool VersionConverters::ForEachVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement)
    {
        if (rootElement.GetVersion() < 2)
        {
            SlotMetadata metaData;

            if (rootElement.FindSubElementAndGetData(AZ_CRC("m_sourceSlot", 0x7575d6c1), metaData))
            {
                rootElement.RemoveElementByName(AZ_CRC("m_sourceSlot", 0x7575d6c1));
                rootElement.AddElementWithData(context, "m_sourceSlot", metaData.m_slotId);
            }
        }

        return true;
    }

    bool VersionConverters::FunctionNodeVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement)
    {
        rootElement.RemoveElementByName(AZ_CRC("m_runtimeAssetId", 0x82d37299));
        rootElement.RemoveElementByName(AZ_CRC("m_sourceAssetId", 0x7a4d00c9));
        rootElement.RemoveElementByName(AZ_CRC("m_dataSlotMapping", 0x2d7c20b2));
        rootElement.RemoveElementByName(AZ_CRC("m_executionSlotMapping", 0x8da8dba8));
        rootElement.RemoveElementByName(AZ_CRC("m_savedFunctionVersion", 0x381b856b));

        if (rootElement.GetVersion() < 6)
        {
            AZ::Data::Asset<SubgraphInterfaceAsset> asset;
            if (rootElement.GetChildData(AZ_CRC("m_asset", 0x4e58e538), asset))
            {
                rootElement.RemoveElementByName(AZ_CRC("m_asset", 0x4e58e538));
                AZ::Data::AssetId interfaceAssetId(asset.GetId().m_guid, AZ_CRC("SubgraphInterface", 0xdfe6dc72));
                AZ::Data::Asset<SubgraphInterfaceAsset> correctSubId(interfaceAssetId, asset.GetType(), asset.GetHint());
                if (rootElement.AddElementWithData(context, "m_asset", correctSubId) == -1)
                {
                    AZ_Assert(false, "Unable to add m_asset data in FunctionCallNode version %u.", rootElement.GetVersion());
                    return false;
                }
            }
        }

        return true;
    }

    bool VersionConverters::HeartBeatVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement)
    {
        if (rootElement.GetVersion() < 1)
        {
            AZ::SerializeContext::DataElementNode* baseTimerElement = rootElement.FindSubElement(AZ_CRC("BaseClass1", 0xd4925735));
            if (!baseTimerElement)
            {
                AZ_Error("Script Canvas", false, "Unable to retrieve the BaseTimerNode data in Node version %u.", rootElement.GetVersion());
                return false;
            }

            if (!MarkSlotAsLatent(context, *baseTimerElement, { "Pulse" }))
            {
                return false;
            }
        }

        return true;
    }

    bool VersionConverters::InputNodeVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement)
    {
        if (rootElement.GetVersion() < 1)
        {
            if (!MarkSlotAsLatent(context, rootElement, { "Pressed", "Held", "Released" }))
            {
                return false;
            }
        }

        return true;
    }

    bool VersionConverters::LerpBetweenVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement)
    {
        if (rootElement.GetVersion() <= 1)
        {
            if (!MarkSlotAsLatent(context, rootElement, { "Tick", "Lerp Complete" }))
            {
                return false;
            }
        }

        return true;
    }

    bool VersionConverters::MarkSlotAsLatent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement, const AZStd::vector<AZStd::string>& startWith)
    {
        auto slotContainerElements = AZ::Utils::FindDescendantElements(context, rootElement, AZStd::vector<AZ::Crc32>{AZ_CRC("BaseClass1", 0xd4925735), AZ_CRC("Slots", 0xc87435d0), AZ_CRC("element", 0x41405e39)});
        for (auto slotElement : slotContainerElements)
        {
            AZStd::string slotName;
            if (slotElement->GetChildData(AZ_CRC("slotName", 0x817c3511), slotName))
            {
                for (auto& matchString : startWith)
                {
                    if (slotName.starts_with(matchString))
                    {
                        slotElement->RemoveElementByName(AZ_CRC("IsLatent", 0xaa35f29f));
                        if (slotElement->AddElementWithData(context, "IsLatent", true) == -1)
                        {
                            AZ_Assert(false, "Unable to add IsLatent data in version %u.", rootElement.GetVersion());
                            return false;
                        }
                    }
                }
            }
            else
            {
                AZ_Assert(false, "Unable to retrieve slotName data in version %u.", rootElement.GetVersion());
                return false;
            }
        }
        return true;
    }

    bool VersionConverters::OnceNodeVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement)
    {
        if (rootElement.GetVersion() == 0)
        {
            AZ::SerializeContext::DataElementNode* nodeElement = rootElement.FindSubElement(AZ_CRC("BaseClass1", 0xd4925735));
            if (!nodeElement)
            {
                AZ_Error("Script Canvas", false, "Unable to retrieve the node structure in Once Node version %u.", rootElement.GetVersion());
                return false;
            }

            Node onceNode;
            if (nodeElement->GetData(onceNode))
            {
                rootElement.RemoveElementByName(AZ_CRC("BaseClass1", 0xd4925735));

                ScriptCanvas::ExecutionSlotConfiguration outSlotConfiguration;
                outSlotConfiguration.m_name = "On Reset";
                outSlotConfiguration.m_toolTip = "Triggered when Reset";
                outSlotConfiguration.SetConnectionType(ScriptCanvas::ConnectionType::Output);
                onceNode.AddSlot(outSlotConfiguration);

                if (rootElement.AddElementWithData(context, "BaseClass1", onceNode) == -1)
                {
                    AZ_Error("Script Canvas", false, "Unable to add the node data in Once Node version %u.", rootElement.GetVersion());
                    return false;
                }
            }
            else
            {
                AZ_Error("Script Canvas", false, "Unable to retrieve the node data in Once Node version %u.", rootElement.GetVersion());
                return false;
            }
        }

        return true;
    }

    bool VersionConverters::ReceiveScriptEventVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement)
    {
        if (rootElement.GetVersion() <= 2)
        {
            AZ::SerializeContext::DataElementNode* scriptEventBase = rootElement.FindSubElement(AZ_CRC("BaseClass1", 0xd4925735));
            if (!scriptEventBase)
            {
                AZ_Error("Script Canvas", false, "Unable to retrieve the BaseTimerNode data in Node version %u.", rootElement.GetVersion());
                return false;
            }

            if (!MarkSlotAsLatent(context, *scriptEventBase, { k_EventOutPrefix }))
            {
                return false;
            }
        }

        return true;
    }

    bool VersionConverters::RepeaterVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode & rootElement)
    {
        /*
        // Old units enum. Here for reference in version conversion
        enum DelayUnits
        {
            Unknown = -1,
            Seconds,
            Ticks
        };
        */
        if (rootElement.GetVersion() < 2)
        {
            if (SerializationUtils::InsertNewBaseClass<ScriptCanvas::Nodes::Internal::BaseTimerNode>(context, rootElement))
            {
                int delayUnits = 0;
                rootElement.GetChildData(AZ_CRC("m_delayUnits", 0x41accf72), delayUnits);

                ScriptCanvas::Nodes::Internal::BaseTimerNode::TimeUnits timeUnit = ScriptCanvas::Nodes::Internal::BaseTimerNode::TimeUnits::Ticks;

                if (delayUnits == 0)
                {
                    timeUnit = ScriptCanvas::Nodes::Internal::BaseTimerNode::TimeUnits::Seconds;
                }

                AZ::SerializeContext::DataElementNode* baseTimerNode = rootElement.FindSubElement(AZ_CRC("BaseClass1", 0xd4925735));

                if (baseTimerNode)
                {
                    baseTimerNode->AddElementWithData<int>(context, "m_timeUnits", static_cast<int>(timeUnit));
                }

                rootElement.RemoveElementByName(AZ_CRC("m_delayUnits", 0x41accf72));
            }
            else
            {
                return false;
            }
        }

        if (rootElement.GetVersion() < 3)
        {
            AZ::SerializeContext::DataElementNode* baseTimerElement = rootElement.FindSubElement(AZ_CRC("BaseClass1", 0xd4925735));
            if (!baseTimerElement)
            {
                AZ_Error("Script Canvas", false, "Unable to retrieve the BaseTimerNode data in Node version %u.", rootElement.GetVersion());
                return false;
            }

            if (!MarkSlotAsLatent(context, *baseTimerElement, { "Complete", "Action" }))
            {
                return false;
            }
        }

        return true;
    }

    bool VersionConverters::TickDelayVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement)
    {
        if (rootElement.GetVersion() < 1)
        {
            if (!MarkSlotAsLatent(context, rootElement, { "Out" }))
            {
                return false;
            }
        }

        return true;
    }

    bool VersionConverters::TimeDelayVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement)
    {
        if (rootElement.GetVersion() < 1)
        {
            AZ::SerializeContext::DataElementNode* baseTimerElement = rootElement.FindSubElement(AZ_CRC("BaseClass1", 0xd4925735));
            if (!baseTimerElement)
            {
                AZ_Error("Script Canvas", false, "Unable to retrieve the BaseTimerNode data in Node version %u.", rootElement.GetVersion());
                return false;
            }

            if (!MarkSlotAsLatent(context, *baseTimerElement, { "Out" }))
            {
                return false;
            }
        }

        return true;
    }

    bool VersionConverters::TimerVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement)
    {
        if (rootElement.GetVersion() < 3)
        {
            if (!MarkSlotAsLatent(context, rootElement, { "Out" }))
            {
                return false;
            }
        }

        return true;
    }

    bool VersionConverters::ScriptEventBaseVersionConverter(AZ::SerializeContext&, AZ::SerializeContext::DataElementNode& rootElement)
    {
        if (rootElement.GetVersion() < 6)
        {
            rootElement.RemoveElementByName(AZ_CRC("m_asset", 0x4e58e538));
        }

        return true;
    }

}
