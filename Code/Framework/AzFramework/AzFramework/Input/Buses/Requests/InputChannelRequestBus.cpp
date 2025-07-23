/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Buses/Requests/InputChannelRequestBus.h>

AZ_INSTANTIATE_EBUS_MULTI_ADDRESS(AZF_API, AzFramework::InputChannelRequests);

namespace AzFramework
{
    const InputChannel* InputChannelRequests::FindInputChannel(const InputChannelId& channelId, AZ::u32 deviceIndex)
    {
        const InputChannel* inputChannel = nullptr;
        const BusIdType inputChannelRequestId(channelId, deviceIndex);
        InputChannelRequestBus::EventResult(inputChannel,
                                            inputChannelRequestId,
                                            &InputChannelRequests::GetInputChannel);
        return inputChannel;
    }
}
