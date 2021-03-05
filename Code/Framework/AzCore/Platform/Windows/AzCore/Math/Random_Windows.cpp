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

#include "Random_Windows.h"

#include <AzCore/PlatformIncl.h>
#include <Wincrypt.h>

namespace AZ
{
    BetterPseudoRandom_Windows::BetterPseudoRandom_Windows()
    {
        if (!CryptAcquireContext(&m_generatorHandle, 0, 0, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT))
        {
            AZ_Warning("System", false, "CryptAcquireContext failed with 0x%08x\n", GetLastError());
            m_generatorHandle = 0;
        }
    }

    BetterPseudoRandom_Windows::~BetterPseudoRandom_Windows()
    {
        if (m_generatorHandle)
        {
            CryptReleaseContext(m_generatorHandle, 0);
        }
    }

    bool BetterPseudoRandom_Windows::GetRandom(void* data, size_t dataSize)
    {
        if (!m_generatorHandle)
        {
            return false;
        }
        if (!CryptGenRandom(m_generatorHandle, static_cast<DWORD>(dataSize), static_cast<PBYTE>(data)))
        {
            AZ_TracePrintf("System", "Failed to call CryptGenRandom!");
            return false;
        }
        return true;
    }
}
