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

#include "EventHandlerTranslationUtility.h"

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            SlotsOutcome EventHandlerTranslationHelper::GetReturnValueSlotsByEventEntry(const EBusEventHandler& handler, const EBusEventEntry& eventEntry)
            {
                return GetReturnValueSlotsByEventEntry<EBusEventHandler, EBusEventEntry>(handler, eventEntry);
            }

            SlotsOutcome EventHandlerTranslationHelper::GetReturnValueSlotsByEventEntry(const ReceiveScriptEvent& handler, const Internal::ScriptEventEntry& eventEntry)
            {
                return GetReturnValueSlotsByEventEntry<ReceiveScriptEvent, Internal::ScriptEventEntry>(handler, eventEntry);
            }

            SlotsOutcome EventHandlerTranslationHelper::GetEventSlotsInExecutionThreadByType(const EBusEventHandler& handler, const Slot& slot, CombinedSlotType targetSlotType)
            {
                return GetEventSlotsInExecutionThreadByType<EBusEventHandler, EBusEventEntry>(handler, slot, targetSlotType);
            }

            SlotsOutcome EventHandlerTranslationHelper::GetEventSlotsInExecutionThreadByType(const ReceiveScriptEvent& handler, const Slot& slot, CombinedSlotType targetSlotType)
            {
                return GetEventSlotsInExecutionThreadByType<ReceiveScriptEvent, Internal::ScriptEventEntry>(handler, slot, targetSlotType);
            }

            SlotsOutcome EventHandlerTranslationHelper::GetNonEventSlotsInExecutionThreadByType(const EBusEventHandler& handler, const Slot& slot, CombinedSlotType targetSlotType)
            {
                return GetNonEventSlotsInExecutionThreadByType<EBusEventHandler, EBusEventHandlerProperty>(handler, slot, targetSlotType);
            }

            SlotsOutcome EventHandlerTranslationHelper::GetNonEventSlotsInExecutionThreadByType(const ReceiveScriptEvent& handler, const Slot& slot, CombinedSlotType targetSlotType)
            {
                return GetNonEventSlotsInExecutionThreadByType<ReceiveScriptEvent, ReceiveScriptEventProperty>(handler, slot, targetSlotType);
            }

            SlotsOutcome EventHandlerTranslationHelper::GetSlotsInExecutionThreadByType(const EBusEventHandler& handler, const Slot& slot, CombinedSlotType targetSlotType)
            {
                return GetSlotsInExecutionThreadByType<EBusEventHandler>(handler, slot, targetSlotType);
            }

            SlotsOutcome EventHandlerTranslationHelper::GetSlotsInExecutionThreadByType(const ReceiveScriptEvent& handler, const Slot& slot, CombinedSlotType targetSlotType)
            {
                return GetSlotsInExecutionThreadByType<ReceiveScriptEvent>(handler, slot, targetSlotType);
            }
        } 
    } 
} 
