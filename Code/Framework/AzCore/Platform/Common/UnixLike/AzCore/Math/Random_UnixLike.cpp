/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Random_UnixLike.h"

#include <AzCore/PlatformIncl.h>
#include <AzCore/Debug/Trace.h>

namespace AZ
{
    BetterPseudoRandom_UnixLike::BetterPseudoRandom_UnixLike()
    {
        m_generatorHandle = fopen("/dev/urandom", "r");
    }

    BetterPseudoRandom_UnixLike::~BetterPseudoRandom_UnixLike()
    {
        if (m_generatorHandle)
        {
            fclose(m_generatorHandle);
        }
    }

    bool BetterPseudoRandom_UnixLike::GetRandom(void* data, size_t dataSize)
    {
        if (!m_generatorHandle)
        {
            return false;
        }
        if (fread(data, 1, dataSize, m_generatorHandle) != dataSize)
        {
            AZ_TracePrintf("System", "Failed to read %d bytes from /dev/urandom!", dataSize);
            fclose(m_generatorHandle);
            m_generatorHandle = nullptr;
            return false;
        }
        return true;
    }
}
