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

#pragma once

// include the required headers
#include "StandardHeaders.h"
#include "MemoryManager.h"
#include "MultiThreadManager.h"


namespace MCore
{
    class MCORE_API IDGenerator
    {
        MCORE_MEMORYOBJECTCATEGORY(IDGenerator, MCORE_DEFAULT_ALIGNMENT, MCORE_MEMCATEGORY_IDGENERATOR);
        friend class Initializer;
        friend class MCoreSystem;

    public:
        /**
         * Generate a unique id.
         * This is thread safe.
         * @return The unique id.
         */
        uint32 GenerateID();

    private:
        AtomicUInt32    mNextID;    /**< The id used for the next GenerateID() call. */

        /**
         * Default constructor.
         */
        IDGenerator();

        /**
         * Destructor.
         */
        ~IDGenerator();
    };
} // namespace MCore
