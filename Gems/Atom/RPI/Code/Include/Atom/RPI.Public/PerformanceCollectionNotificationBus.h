/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/string/string.h>

#include <Atom/RPI.Public/Configuration.h>

namespace AZ::RPI
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Connect to this EBus to be notified whenever the current RPI performance collection job
    //! is completed. For details on how to start an RPI performance collection job see:
    //! "o3de/Gems/Atom/RPI/Code/Source/RPI.Private/PerformanceCVarManager.cpp".
    //! This EBus is available for C++, Game Client BehaviorContext (Lua) and Editor BehaviorContext (Python).
    class ATOM_RPI_PUBLIC_API PerformaceCollectionNotification : public AZ::EBusTraits
    {
    public:
        virtual ~PerformaceCollectionNotification() = default;

        static void Reflect(AZ::ReflectContext* context);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Performance collection job is completed and the results are available
        //! in @outputfilePath.
        virtual void OnPerformanceCollectionJobFinished(const AZStd::string& outputfilePath) = 0;

    };
    using PerformaceCollectionNotificationBus = AZ::EBus<PerformaceCollectionNotification>;

} // namespace AZ::RPI

DECLARE_EBUS_EXTERN_DLL_SINGLE_ADDRESS(RPI::PerformaceCollectionNotification);
