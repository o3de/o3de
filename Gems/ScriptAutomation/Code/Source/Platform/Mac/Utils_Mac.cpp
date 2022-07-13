/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Utils/Utils.h>

#include <AzCore/PlatformIncl.h>
#include <Atom/RHI.Edit/Utils.h>
#include <iostream>
#include <errno.h>

namespace ScriptAutomation
{
    namespace Utils
    {
        bool SupportsResizeClientArea()
        {
            return true;
        }
    } // namespace Utils
} // namespace ScriptAutomation
