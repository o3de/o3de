/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "StandardHeaders.h"
#include "IDGenerator.h"


namespace MCore
{
    // constructor
    IDGenerator::IDGenerator()
    {
        mNextID.SetValue(0);
    }


    // destructor
    IDGenerator::~IDGenerator()
    {
    }


    // get a unique id
    uint32 IDGenerator::GenerateID()
    {
        const uint32 result = mNextID.Increment();
        MCORE_ASSERT(result != MCORE_INVALIDINDEX32); // reached the limit
        return result;
    }
} // namespace MCore
