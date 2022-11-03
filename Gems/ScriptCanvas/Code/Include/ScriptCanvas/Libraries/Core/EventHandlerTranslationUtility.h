/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
                static ConstSlotsOutcome  GetSlotsInExecutionThreadByType(const EBusEventHandler& handler, const Slot& executionSlot, CombinedSlotType targetSlotType);
                
                // ScriptEvent handler
                static ConstSlotsOutcome  GetSlotsInExecutionThreadByType(const ReceiveScriptEvent& handler, const Slot& executionSlot, CombinedSlotType targetSlotType);

            private:
                static ConstSlotsOutcome  GetReturnValueSlotsByEventEntry(const EBusEventHandler& handler, const EBusEventEntry& eventEntry);
                static ConstSlotsOutcome  GetReturnValueSlotsByEventEntry(const ReceiveScriptEvent& handler, const Internal::ScriptEventEntry& eventEntry);
                template<typename HandlerType, typename EntryType>
                static ConstSlotsOutcome  GetReturnValueSlotsByEventEntry(const HandlerType& handler, const EntryType& eventEntry);

                static ConstSlotsOutcome  GetEventSlotsInExecutionThreadByType(const EBusEventHandler& handler, const Slot& executionSlot, CombinedSlotType targetSlotType);
                static ConstSlotsOutcome  GetEventSlotsInExecutionThreadByType(const ReceiveScriptEvent& handler, const Slot& executionSlot, CombinedSlotType targetSlotType);
                template<typename HandlerType, typename EntryType>
                static ConstSlotsOutcome  GetEventSlotsInExecutionThreadByType(const HandlerType& handler, const Slot& executionSlot, CombinedSlotType targetSlotType);

                static ConstSlotsOutcome  GetNonEventSlotsInExecutionThreadByType(const EBusEventHandler& handler, const Slot& executionSlot, CombinedSlotType targetSlotType);
                static ConstSlotsOutcome  GetNonEventSlotsInExecutionThreadByType(const ReceiveScriptEvent& handler, const Slot& executionSlot, CombinedSlotType targetSlotType);
                template<typename HandlerType, typename HandlerPropertyType>
                static ConstSlotsOutcome  GetNonEventSlotsInExecutionThreadByType(const HandlerType& handler, const Slot& executionSlot, CombinedSlotType targetSlotType);

                template<typename HandlerType>
                static ConstSlotsOutcome  GetSlotsInExecutionThreadByType(const HandlerType& handler, const Slot& executionSlot, CombinedSlotType targetSlotType);
            };

            template<typename HandlerType, typename EntryType>
            ConstSlotsOutcome  EventHandlerTranslationHelper::GetReturnValueSlotsByEventEntry(const HandlerType& handler, const EntryType& eventEntry)
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
            ConstSlotsOutcome  EventHandlerTranslationHelper::GetEventSlotsInExecutionThreadByType(const HandlerType& handler, const Slot& executionSlot, CombinedSlotType targetSlotType)
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
            ConstSlotsOutcome  EventHandlerTranslationHelper::GetNonEventSlotsInExecutionThreadByType(const HandlerType& handler, const Slot& executionSlot, CombinedSlotType targetSlotType)
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
                            if (auto disconnectedSlot = HandlerPropertyType::GetOnDisconnectedSlot(&handler))
                            {
                                executionSlots.push_back(disconnectedSlot);
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
            ConstSlotsOutcome  EventHandlerTranslationHelper::GetSlotsInExecutionThreadByType(const HandlerType& handler, const Slot& executionSlot, CombinedSlotType targetSlotType)
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
