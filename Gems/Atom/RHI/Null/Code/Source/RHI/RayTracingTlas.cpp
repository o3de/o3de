/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "Atom_RHI_Null_precompiled.h"
#include <Atom/RHI/Buffer.h>
#include <RHI/RayTracingTlas.h>

namespace AZ
{
    namespace Null
    {
        RHI::Ptr<RayTracingTlas> RayTracingTlas::Create()
        {
            return aznew RayTracingTlas;
        }
    }
}
