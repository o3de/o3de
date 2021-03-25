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
#pragma once
#include <Include/ScriptCanvas/Libraries/Core/EBusEventHandler.generated.h>
#include <Include/ScriptCanvas/Libraries/Core/ReceiveScriptEvent.generated.h>

#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/SlotNames.h>
#include <ScriptCanvas/Libraries/Core/EBusEventHandler.h>
#include <ScriptCanvas/Libraries/Core/ReceiveScriptEvent.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            class EventHandlerTranslationHelper
            {
            public:
                // EbusEvent handler
                static SlotsOutcome GetSlotsInExecutionThreadByType(const EBusEventHandler& handler, const Slot& executionSlot, CombinedSlotType targetSlotType);
                
                // ScriptEvent handler
                static SlotsOutcome GetSlotsInExecutionThreadByType(const ReceiveScriptEvent& handler, const Slot& executionSlot, CombinedSlotType targetSlotType);

            private:
                static SlotsOutcome GetReturnValueSlotsByEventEntry(const EBusEventHandler& handler, const EBusEventEntry& eventEntry);
                static SlotsOutcome GetReturnValueSlotsByEventEntry(const ReceiveScriptEvent& handler, const Internal::ScriptEventEntry& eventEntry);
                template<typename HandlerType, typename EntryType>
                static SlotsOutcome GetReturnValueSlotsByEventEntry(const HandlerType& handler, const EntryType& eventEntry);

                static SlotsOutcome GetEventSlotsInExecutionThreadByType(const EBusEventHandler& handler, const Slot& executionSlot, CombinedSlotType targetSlotType);
                static SlotsOutcome GetEventSlotsInExecutionThreadByType(const ReceiveScriptEvent& handler, const Slot& executionSlot, CombinedSlotType targetSlotType);
                template<typename HandlerType, typename EntryType>
                static SlotsOutcome GetEventSlotsInExecutionThreadByType(const HandlerType& handler, const Slot& executionSlot, CombinedSlotType targetSlotType);

                static SlotsOutcome GetNonEventSlotsInExecutionThreadByType(const EBusEventHandler& handler, const Slot& executionSlot, CombinedSlotType targetSlotType);
                static SlotsOutcome GetNonEventSlotsInExecutionThreadByType(const ReceiveScriptEvent& handler, const Slot& executionSlot, CombinedSlotType targetSlotType);
                template<typename HandlerType, typename HandlerPropertyType>
                static SlotsOutcome GetNonEventSlotsInExecutionThreadByType(const HandlerType& handler, const Slot& executionSlot, CombinedSlotType targetSlotType);

                template<typename HandlerType>
                static SlotsOutcome GetSlotsInExecutionThreadByType(const HandlerType& handler, const Slot& executionSlot, CombinedSlotType targetSlotType);
            };

            template<typename HandlerType, typename EntryType>
            SlotsOutcome EventHandlerTranslationHelper::GetReturnValueSlotsByEventEntry(const HandlerType& handler, const EntryType& eventEntry)
            {
                if (!eventEntry.m_resultSlotId.IsValid())
                {
                    return AZ::Success(AZStd::vector<const Slot*>{});
                }
                else if (auto resultSlot = handler.GetSlot(eventEntry.m_resultSlotId))
                {
                    return AZ::Success(AZStd::vector<const Slot*>{resultSlot});
                }
                else
                {
                    return AZ::Failure(AZStd::string::format("No executionSlot found for executionSlotId %s", eventEntry.m_resultSlotId.ToString().data()));
                }
            }

            template<typename HandlerType, typename EntryType>
            SlotsOutcome EventHandlerTranslationHelper::GetEventSlotsInExecutionThreadByType(const HandlerType& handler, const Slot& executionSlot, CombinedSlotType targetSlotType)
            {
                const EntryType* eventEntry = handler.FindEventWithSlot(executionSlot);

                if (!eventEntry)
                {
                    return AZ::Failure(AZStd::string("Failure to find event with executionSlot, %s", executionSlot.GetName().data()));
                }

                // some event slots are mis-labeled
                if (executionSlot.GetType() == CombinedSlotType::LatentOut || executionSlot.GetType() == CombinedSlotType::ExecutionOut)
                {
                    if (targetSlotType == CombinedSlotType::DataIn)
                    {
                        return GetReturnValueSlotsByEventEntry(handler, *eventEntry);
                    }
                    else if (targetSlotType == CombinedSlotType::DataOut)
                    {
                        return handler.GetSlots(eventEntry->m_parameterSlotIds);
                    }
                }

                return AZ::Failure(AZStd::string("no such mapping supported"));
            }

            template<typename HandlerType, typename HandlerPropertyType>
            SlotsOutcome EventHandlerTranslationHelper::GetNonEventSlotsInExecutionThreadByType(const HandlerType& handler, const Slot& executionSlot, CombinedSlotType targetSlotType)
            {
                AZStd::vector<const Slot*> executionSlots;
                AZStd::vector<SlotId> errorSlots;

                switch (executionSlot.GetType())
                {
                case CombinedSlotType::ExecutionIn:
                    switch (targetSlotType)
                    {
                    case CombinedSlotType::DataIn:
                        if (handler.IsIDRequired() && executionSlot.GetId() == HandlerPropertyType::GetConnectSlotId(&handler))
                        {
                            auto addressSlot = handler.GetSlot(handler.GetSlotId(GetSourceSlotName()));
                            if (addressSlot)
                            {
                                executionSlots.push_back(addressSlot);
                            }
                            else
                            {
                                errorSlots.push_back(handler.GetSlotId(GetSourceSlotName()));
                            }
                        }
                        break;

                    case CombinedSlotType::ExecutionOut:
                        if (executionSlot.GetId() == HandlerPropertyType::GetConnectSlotId(&handler))
                        {
                            if (auto connectedSlot = HandlerPropertyType::GetOnConnectedSlot(&handler))
                            {
                                executionSlots.push_back(connectedSlot);
                            }
                            else
                            {
                                errorSlots.push_back(HandlerPropertyType::GetOnConnectedSlotId(&handler));
                            }
                        }
                        else if (executionSlot.GetId() == HandlerPropertyType::GetDisconnectSlotId(&handler))
                        {
                            if (auto disonnectedSlot = HandlerPropertyType::GetOnDisconnectedSlot(&handler))
                            {
                                executionSlots.push_back(disonnectedSlot);
                            }
                            else
                            {
                                errorSlots.push_back(HandlerPropertyType::GetDisconnectSlotId(&handler));
                            }
                        }
                        else
                        {
                            AZ_Assert(false, "Unsupported executionSlot %s.", executionSlot.GetName().c_str());
                        }
                        break;

                    default:
                        break;
                    }
                    break;

                default:
                    break;
                }

                if (errorSlots.empty())
                {
                    return AZ::Success(executionSlots);
                }
                else
                {
                    AZStd::string errorString = AZStd::string::format("No executionSlot found for %s", errorSlots[0].ToString().c_str());

                    for (int i = 1; i < errorSlots.size(); ++i)
                    {
                        errorString.append(", ");
                        errorString.append(errorSlots[i].ToString());
                    }

                    return AZ::Failure(errorString);
                }
            }

            template<typename HandlerType>
            SlotsOutcome EventHandlerTranslationHelper::GetSlotsInExecutionThreadByType(const HandlerType& handler, const Slot& executionSlot, CombinedSlotType targetSlotType)
            {
                if (handler.IsEventSlotId(executionSlot.GetId()))
                {
                    return GetEventSlotsInExecutionThreadByType(handler, executionSlot, targetSlotType);
                }
                else
                {
                    return GetNonEventSlotsInExecutionThreadByType(handler, executionSlot, targetSlotType);
                }
            }
        }
    }
}
