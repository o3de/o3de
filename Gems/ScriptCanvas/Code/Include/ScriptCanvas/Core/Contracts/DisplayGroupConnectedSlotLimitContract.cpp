/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "DisplayGroupConnectedSlotLimitContract.h"

#include <ScriptCanvas/Core/ContractBus.h>
#include <ScriptCanvas/Core/GraphBus.h>
#include <ScriptCanvas/Core/NodeBus.h>
#include <ScriptCanvas/Core/Endpoint.h>
#include <ScriptCanvas/Core/Slot.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Graph.h>

namespace ScriptCanvas
{
    void DisplayGroupConnectedSlotLimitContract::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<DisplayGroupConnectedSlotLimitContract, Contract>()
                ->Version(0)
                ->Field("limit", &DisplayGroupConnectedSlotLimitContract::m_limit)
                ->Field("displayGroup", &DisplayGroupConnectedSlotLimitContract::m_displayGroup)
                ->Field("errorMessage", &DisplayGroupConnectedSlotLimitContract::m_customErrorMessage)
            ;
        }
    }
    
    DisplayGroupConnectedSlotLimitContract::DisplayGroupConnectedSlotLimitContract(const AZStd::string& displayGroup, AZ::u32 connectedSlotLimit)
        : m_displayGroup(displayGroup)
        , m_limit(connectedSlotLimit)
    {
    }
    
    AZ::Outcome<void, AZStd::string> DisplayGroupConnectedSlotLimitContract::OnEvaluate(const Slot& sourceSlot, [[maybe_unused]] const Slot& targetSlot) const
    {        
        // If the slot is already connected, it can have more connections
        if (sourceSlot.IsConnected())
        {
            return AZ::Success();
        }
        
        AZ::u32 connectionCount = 0;

        AZStd::vector<Slot*> slotGroup = sourceSlot.GetNode()->GetSlotsWithDisplayGroup(m_displayGroup);

        for (Slot* slot : slotGroup)
        {
            if (slot->IsConnected())
            {
                ++connectionCount;

                if (connectionCount >= m_limit)
                {
                    AZStd::string errorMessage = m_customErrorMessage;

                    if (errorMessage.empty())
                    {
                        errorMessage = AZStd::string::format("Too many connections present for DisplayGroup - %s", m_displayGroup.c_str());
                    }

                    return AZ::Failure(errorMessage);                            
                }
            }
        }

        return AZ::Success();
    }
}
