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
