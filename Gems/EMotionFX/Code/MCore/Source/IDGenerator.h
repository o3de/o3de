/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        size_t GenerateID();

    private:
        AZStd::atomic<size_t>    m_nextId;    /**< The id used for the next GenerateID() call. */

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
