/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Gestures_precompiled.h"
#include <InputChannelGesture.h>
#include <AzCore/Serialization/SerializeContext.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace Gestures
{
    void InputChannelGesture::Type::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<InputChannelGesture::Type>();
        }
    }
} // namespace Gestures
