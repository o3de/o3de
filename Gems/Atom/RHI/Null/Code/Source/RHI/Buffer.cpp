/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "Atom_RHI_Null_precompiled.h"
#include <RHI/Buffer.h>

namespace AZ
{
    namespace Null
    {
        RHI::Ptr<Buffer> Buffer::Create()
        {
            return aznew Buffer();
        }
    }
}
