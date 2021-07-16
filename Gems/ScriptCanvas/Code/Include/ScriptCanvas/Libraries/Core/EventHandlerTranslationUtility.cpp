/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EventHandlerTranslationUtility.h"

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            ConstSlotsOutcome  EventHandlerTranslationHelper::GetReturnValueSlotsByEventEntry(const EBusEventHandler& handler, const EBusEventEntry& eventEntry)
            {
                return GetReturnValueSlotsByEventEntry<EBusEventHandler, EBusEventEntry>(handler, eventEntry);
            }

            ConstSlotsOutcome  EventHandlerTranslationHelper::GetReturnValueSlotsByEventEntry(const ReceiveScriptEvent& handler, const Internal::ScriptEventEntry& eventEntry)
            {
                return GetReturnValueSlotsByEventEntry<ReceiveScriptEvent, Internal::ScriptEventEntry>(handler, eventEntry);
            }

            ConstSlotsOutcome  EventHandlerTranslationHelper::GetEventSlotsInExecutionThreadByType(const EBusEventHandler& handler, const Slot& slot, CombinedSlotType targetSlotType)
            {
                return GetEventSlotsInExecutionThreadByType<EBusEventHandler, EBusEventEntry>(handler, slot, targetSlotType);
            }

            ConstSlotsOutcome  EventHandlerTranslationHelper::GetEventSlotsInExecutionThreadByType(const ReceiveScriptEvent& handler, const Slot& slot, CombinedSlotType targetSlotType)
            {
                return GetEventSlotsInExecutionThreadByType<ReceiveScriptEvent, Internal::ScriptEventEntry>(handler, slot, targetSlotType);
            }

            ConstSlotsOutcome  EventHandlerTranslationHelper::GetNonEventSlotsInExecutionThreadByType(const EBusEventHandler& handler, const Slot& slot, CombinedSlotType targetSlotType)
            {
                return GetNonEventSlotsInExecutionThreadByType<EBusEventHandler, EBusEventHandlerProperty>(handler, slot, targetSlotType);
            }

            ConstSlotsOutcome  EventHandlerTranslationHelper::GetNonEventSlotsInExecutionThreadByType(const ReceiveScriptEvent& handler, const Slot& slot, CombinedSlotType targetSlotType)
            {
                return GetNonEventSlotsInExecutionThreadByType<ReceiveScriptEvent, ReceiveScriptEventProperty>(handler, slot, targetSlotType);
            }

            ConstSlotsOutcome  EventHandlerTranslationHelper::GetSlotsInExecutionThreadByType(const EBusEventHandler& handler, const Slot& slot, CombinedSlotType targetSlotType)
            {
                return GetSlotsInExecutionThreadByType<EBusEventHandler>(handler, slot, targetSlotType);
            }

            ConstSlotsOutcome EventHandlerTranslationHelper::GetSlotsInExecutionThreadByType(const ReceiveScriptEvent& handler, const Slot& slot, CombinedSlotType targetSlotType)
            {
                return GetSlotsInExecutionThreadByType<ReceiveScriptEvent>(handler, slot, targetSlotType);
            }
        } 
    } 
} 
