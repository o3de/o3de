/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/TypeInfoSimple.h>
#include <AzCore/RTTI/RTTIMacros.h>

namespace AzToolsFramework
{
    //! HotKeyManagerInternalInterface
    //! Internal Interface to query implementation details for hotkeys.
    class HotKeyManagerInternalInterface
    {
    public:
        AZ_RTTI(HotKeyManagerInternalInterface, "{9AD5D5E3-C2A6-448B-8CA2-84888A896008}");

        //! Completely reset the HotKey Manager from all items registered after initialization.
        //! Clears all HotKeys.
        //! Used in Unit tests to allow clearing the environment between runs.
        virtual void Reset() = 0;

    };

} // namespace AzToolsFramework
