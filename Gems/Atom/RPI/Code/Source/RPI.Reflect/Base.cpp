/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Base.h>

namespace AZ
{
    namespace RPI
    {
        bool Validation::s_isEnabled = RHI::BuildOptions::IsDebugBuild || RHI::BuildOptions::IsProfileBuild;
    }
}
