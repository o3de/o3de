/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Driller/DrillerBus.h>

#include <AzCore/Driller/Driller.h>
#include <AzCore/Module/Environment.h>
#include <AzCore/std/parallel/mutex.h>

namespace AZ::Debug
{
    //////////////////////////////////////////////////////////////////////////
    // Globals
    // We need to synchronize all driller evens, so we have proper order, and access to the data
    // We use a global mutex which should be used for all driller operations.
    // The mutex is held in an environment variable so it works across DLLs.
    EnvironmentVariable<AZStd::recursive_mutex> s_drillerGlobalMutex;
    //////////////////////////////////////////////////////////////////////////


    //=========================================================================
    // lock
    // [4/11/2011]
    //=========================================================================
    void DrillerEBusMutex::lock()
    {
        GetMutex().lock();
    }

    //=========================================================================
    // try_lock
    // [4/11/2011]
    //=========================================================================
    bool DrillerEBusMutex::try_lock()
    {
        return GetMutex().try_lock();
    }

    //=========================================================================
    // unlock
    // [4/11/2011]
    //=========================================================================
    void DrillerEBusMutex::unlock()
    {
        GetMutex().unlock();
    }

    //=========================================================================
    // unlock
    // [4/11/2011]
    //=========================================================================
    AZStd::recursive_mutex& DrillerEBusMutex::GetMutex()
    {
        if (!s_drillerGlobalMutex)
        {
            s_drillerGlobalMutex = Environment::CreateVariable<AZStd::recursive_mutex>(AZ_FUNCTION_SIGNATURE);
        }
        return *s_drillerGlobalMutex;
    }
} // namespace AZ::Debug
