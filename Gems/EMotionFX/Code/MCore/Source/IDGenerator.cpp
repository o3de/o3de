/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        : m_nextId{0}
    {
    }


    // destructor
    IDGenerator::~IDGenerator()
    {
    }


    // get a unique id
    size_t IDGenerator::GenerateID()
    {
        const size_t result = m_nextId++;
        MCORE_ASSERT(result != InvalidIndex); // reached the limit
        return result;
    }
} // namespace MCore
