/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <System/PhysXSdkCallbacks.h>

#include <AzCore/Debug/Profiler.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Math/Crc.h>

namespace PhysX
{
    void PxAzErrorCallback::reportError(physx::PxErrorCode::Enum code, [[maybe_unused]] const char* message, [[maybe_unused]] const char* file, [[maybe_unused]] int line)
    {
        switch (code)
        {
        case physx::PxErrorCode::eDEBUG_INFO: [[fallthrough]];
        case physx::PxErrorCode::eNO_ERROR:
            AZ_TracePrintf("PhysX", "PxErrorCode %i: %s (line %i in %s)", code, message, line, file);
            break;

        case physx::PxErrorCode::eDEBUG_WARNING: [[fallthrough]];
        case physx::PxErrorCode::ePERF_WARNING:
            AZ_Warning("PhysX", false, "PxErrorCode %i: %s (line %i in %s)", code, message, line, file);
            break;

        case physx::PxErrorCode::eINVALID_OPERATION: [[fallthrough]];
        case physx::PxErrorCode::eINTERNAL_ERROR: [[fallthrough]];
        case physx::PxErrorCode::eOUT_OF_MEMORY: [[fallthrough]];
        case physx::PxErrorCode::eABORT:
            AZ_Assert(false, "PhysX - PxErrorCode %i: %s (line %i in %s)", code, message, line, file)
                break;

        case physx::PxErrorCode::eINVALID_PARAMETER: [[fallthrough]];
        default:
            AZ_Error("PhysX", false, "PxErrorCode %i: %s (line %i in %s)", code, message, line, file);
            break;
        }
    }

    void* PxAzProfilerCallback::zoneStart([[maybe_unused]] const char* eventName, bool detached, [[maybe_unused]] uint64_t contextId)
    {
        if (!detached)
        {
            AZ_PROFILE_BEGIN(Physics, eventName);
        }
        else
        {
            AZ_PROFILE_INTERVAL_START(Physics, AZ::Crc32(eventName), eventName);
        }
        return nullptr;
    }

    void PxAzProfilerCallback::zoneEnd([[maybe_unused]]void* profilerData,
        [[maybe_unused]] const char* eventName, bool detached, [[maybe_unused]] uint64_t contextId)
    {
        if (!detached)
        {
            AZ_PROFILE_END(Physics);
        }
        else
        {
            AZ_PROFILE_INTERVAL_END(Physics, AZ::Crc32(eventName));
        }
    }
}
