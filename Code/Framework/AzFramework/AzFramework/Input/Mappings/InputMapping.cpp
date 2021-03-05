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
