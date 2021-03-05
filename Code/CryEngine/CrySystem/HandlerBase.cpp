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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "CrySystem_precompiled.h"

#include "ProjectDefines.h"
#if defined(MAP_LOADING_SLICING)

#include "HandlerBase.h"

const char* SERVER_LOCK_NAME = "SynchronizeGameServer";
const char* CLIENT_LOCK_NAME = "SynchronizeGameClient";

HandlerBase::HandlerBase(const char* bucket, int affinity)
{
    m_serverLockName.Format("%s_%s", SERVER_LOCK_NAME, bucket);
    m_clientLockName.Format("%s_%s", CLIENT_LOCK_NAME, bucket);
    if (affinity != 0)
    {
        m_affinity = uint32(1) << (affinity - 1);
    }
    else
    {
        m_affinity = -1;
    }
    m_prevAffinity = 0;
}

HandlerBase::~HandlerBase()
{
    if (m_prevAffinity)
    {
        if (SyncSetAffinity(m_prevAffinity))
        {
            CryLogAlways("Restored affinity to %d", m_prevAffinity);
        }
        else
        {
            CryLogAlways("Failed to restore affinity to %d", m_prevAffinity);
        }
    }
}

void HandlerBase::SetAffinity()
{
    if (m_prevAffinity) //already set
    {
        return;
    }
    if (uint32 p = SyncSetAffinity(m_affinity))
    {
        CryLogAlways("Changed affinity to %d", m_affinity);
        m_prevAffinity = p;
    }
    else
    {
        CryLogAlways("Failed to change affinity to %d", m_affinity);
    }
}

#if defined(LINUX)

uint32 HandlerBase::SyncSetAffinity(uint32 cpuMask)//put -1
{
    if (cpuMask != 0)
    {
        cpu_set_t cpuSet;
        uint32 affinity = 0;
        if (!sched_getaffinity(getpid(), sizeof cpuSet, &cpuSet))
        {
            for (int cpu = 0; cpu < sizeof(cpuMask) * 8; ++cpu)
            {
                if (CPU_ISSET(cpu, &cpuSet))
                {
                    affinity |= 1 << cpu;
                }
            }
        }
        if (affinity)
        {
            CPU_ZERO(&cpuSet);
            for (int cpu = 0; cpu < sizeof(cpuMask) * 8; ++cpu)
            {
                if (cpuMask & (1 << cpu))
                {
                    CPU_SET(cpu, &cpuSet);
                }
            }

            if (!sched_setaffinity(getpid(), sizeof(cpuSet), &cpuSet))
            {
                return affinity;
            }
        }
    }
    return 0;
}

#elif AZ_LEGACY_CRYSYSTEM_TRAIT_USE_HANDLER_SYNC_AFFINITY

uint32 HandlerBase::SyncSetAffinity(uint32 cpuMask)//put -1
{
    uint32 p = (uint32)SetThreadAffinityMask(GetCurrentThread(), cpuMask);
    if (p == 0)
    {
        CryLogAlways("Error updating affinity mask to %d", cpuMask);
    }
    return p;
}

#else

uint32 HandlerBase::SyncSetAffinity(uint32 cpuMask)//put -1
{
    CryLogAlways("Updating thread affinity not supported on this platform");
    return 0;
}

#endif

#endif // defined(MAP_LOADING_SLICING)
