/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
