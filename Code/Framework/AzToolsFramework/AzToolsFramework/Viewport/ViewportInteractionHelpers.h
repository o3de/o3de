/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
//AZTF-SHARED
#include <AzToolsFramework/AzToolsFrameworkAPI.h>

#include <AzFramework/Input/Channels/InputChannel.h>
#include <AzToolsFramework/Viewport/ViewportTypes.h>


namespace AzToolsFramework::ViewportInteraction
{
    class AZTF_API Helpers
    {
    public:
        static MouseButton GetMouseButton(const AzFramework::InputChannel& inputChannel);
        static bool IsMouseMove(const AzFramework::InputChannel& inputChannel);
        static KeyboardModifier GetKeyboardModifier(const AzFramework::InputChannel& inputChannel);
    };
} // namespace AzToolsFramework::ViewportInteraction
