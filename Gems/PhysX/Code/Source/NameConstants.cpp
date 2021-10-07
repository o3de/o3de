/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <NameConstants.h>

namespace PhysX
{
    namespace UXNameConstants
    {
        const AZStd::string& GetPhysXDocsRoot()
        {
            static const AZStd::string val = "https://o3de.org/docs/user-guide/interactivity/physics/nvidia-physx/";
            return val;
        }
    } // namespace UXNameConstants
} // namespace PhysX
