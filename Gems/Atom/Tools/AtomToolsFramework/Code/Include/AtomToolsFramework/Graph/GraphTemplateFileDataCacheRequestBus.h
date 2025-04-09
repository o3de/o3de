/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/Graph/GraphTemplateFileData.h>
#include <AzCore/EBus/EBus.h>

namespace AtomToolsFramework
{
    //! Interface for class managing a cache of template file data structures
    class GraphTemplateFileDataCacheRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::Crc32 BusIdType;
        using MutexType = AZStd::mutex;

        //! Attempts to load or retrieve a previously loaded template file data structure
        virtual GraphTemplateFileData Load(const AZStd::string& path) = 0;
    };

    using GraphTemplateFileDataCacheRequestBus = AZ::EBus<GraphTemplateFileDataCacheRequests>;
} // namespace AtomToolsFramework
