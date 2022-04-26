/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AzToolsFramework
{
    //! HotkeyManagerInterface
    //! Interface to register and manage hotkeys in the Editor.
    class HotkeyManagerInterface
    {
    public:
        AZ_RTTI(HotkeyManagerInterface, "{6548627D-2D95-46BB-A6C8-9C9C59ED74EC}");
    };

} // namespace AzToolsFramework
