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

#include <AzFramework/Input/Channels/InputChannelDeltaWithSharedPosition2D.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelDeltaWithSharedPosition2D::InputChannelDeltaWithSharedPosition2D(
        const AzFramework::InputChannelId& inputChannelId,
        const InputDevice& inputDevice,
        const SharedPositionData2D& sharedPositionData)
    : InputChannelDelta(inputChannelId, inputDevice)
    , m_sharedPositionData(sharedPositionData)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputChannel::CustomData* InputChannelDeltaWithSharedPosition2D::GetCustomData() const
    {
        return m_sharedPositionData.get();
    }
} // namespace LmbrCentral
