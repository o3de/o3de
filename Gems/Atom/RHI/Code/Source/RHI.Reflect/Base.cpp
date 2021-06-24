/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/Base.h>

namespace AZ
{
    namespace RHI
    {
        bool Validation::s_isEnabled = BuildOptions::IsDebugBuild || BuildOptions::IsProfileBuild;
    }
}
