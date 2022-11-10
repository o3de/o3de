/*
* Copyright (c) Contributors to the Open 3D Engine Project.
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#pragma once

#include <AzCore/Console/IConsole.h>
#include <AzCore/Interface/Interface.h>

namespace AZ
{
    namespace Debug
    {
        class PerformanceCollector;
    }

    namespace RPI
    {
        class GpuPassProfiler;

        // A simple helper function.
        AZ::Debug::PerformanceCollector::DataLogType GetDataLogTypeFromCVar(const AZ::CVarFixedString& newCaptureType);

        //! This is a simple interface to help get a reference to an AZ::Debug::PerformanceCollector
        //! so we can feed CVar values into it.
        class IPerformanceCollectorOwner
        {
        public:
            AZ_CLASS_ALLOCATOR(IPerformanceCollectorOwner, AZ::SystemAllocator, 0);
            AZ_RTTI(IPerformanceCollectorOwner, "{D157F48E-8D9C-4F4F-93CE-961860371965}");

            IPerformanceCollectorOwner() = default;
            virtual ~IPerformanceCollectorOwner() = default;

        protected:
            friend class PerformanceCvarManager;
            virtual AZ::Debug::PerformanceCollector* GetPerformanceCollector() { return nullptr; }
            virtual GpuPassProfiler* GetGpuPassProfiler() { return nullptr;  }
        };
        using PerformanceCollectorOwner = AZ::Interface<IPerformanceCollectorOwner>;

    } // namespace RPI
} // namespace AZ
