/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/Graph/AssetStatusReporterState.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace AtomToolsFramework
{
    //! Interface for class processing a queue of job status requests
    class AssetStatusReporterSystemRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::Crc32 BusIdType;
        using MutexType = AZStd::mutex;

        //! Start reporting job status for one or more source files
        //! @return Unique ID used to reference this specific request, check status, or stop reporting
        virtual void StartReporting(const AZ::Uuid& requestId, const AZStd::vector<AZStd::string>& sourcePaths) = 0;

        //! Stop reporting job status for files corresponding to the request ID
        virtual void StopReporting(const AZ::Uuid& requestId) = 0;

        //! Stop reporting status for all requests
        virtual void StopReportingAll() = 0;

        //! Return the overall status for files corresponding to the request ID
        virtual AssetStatusReporterState GetStatus(const AZ::Uuid& requestId) const = 0;
    };

    using AssetStatusReporterSystemRequestBus = AZ::EBus<AssetStatusReporterSystemRequests>;
} // namespace AtomToolsFramework
