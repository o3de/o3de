/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PhysX_precompiled.h>
#include <NameConstants.h>

namespace PhysX
{
    namespace UXNameConstants
    {
        const AZStd::string& GetPhysXDocsRoot()
        {
            static const AZStd::string val = "https://docs.o3de.org/docs/user-guide/interactivity/physics/";
            return val;
        }
    } // namespace UXNameConstants
} // namespace PhysX
