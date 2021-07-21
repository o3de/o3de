/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Channels/InputChannelDigitalWithSharedPosition2D.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelDigitalWithSharedPosition2D::InputChannelDigitalWithSharedPosition2D(
        const AzFramework::InputChannelId& inputChannelId,
        const InputDevice& inputDevice,
        const SharedPositionData2D& sharedPositionData)
    : InputChannelDigital(inputChannelId, inputDevice)
    , m_sharedPositionData(sharedPositionData)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputChannel::CustomData* InputChannelDigitalWithSharedPosition2D::GetCustomData() const
    {
        return m_sharedPositionData.get();
    }
} // namespace LmbrCentral
