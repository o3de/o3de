/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Mappings/InputMapping.h>
#include <AzFramework/Input/Contexts/InputContext.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputMapping::InputMapping(const InputChannelId& inputChannelId, const InputContext& inputContext)
        : InputChannel(inputChannelId, inputContext)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputMapping::ProcessPotentialSourceInputEvent(const InputChannel& inputChannel)
    {
        return IsSourceInput(inputChannel) ? OnSourceInputEvent(inputChannel) : false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputMapping::OnTick()
    {
        // The 'Ended' state for any input channel only lasts for one tick, but we will not receive
        // any events when a source input transitions from Ended->Idle, and we need to do that here.
        if (IsStateEnded())
        {
            ResetState();
        }
    }
} // namespace AzFramework
