/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Channels/InputChannelAxis1D.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelAxis1D::InputChannelAxis1D(const InputChannelId& inputChannelId,
                                           const InputDevice& inputDevice)
        : InputChannelAnalog(inputChannelId, inputDevice)
    {
    }
} // namespace AzFramework
