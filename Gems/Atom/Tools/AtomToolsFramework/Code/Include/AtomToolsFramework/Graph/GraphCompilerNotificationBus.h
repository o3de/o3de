/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

namespace AtomToolsFramework
{
    //! Bus interface with notification about the status of an individual compile emitted from a graph compiler
    class GraphCompilerNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::Crc32 BusIdType;

        // This notification is sent whenever graph compilation has started.
        virtual void OnCompileGraphStarted([[maybe_unused]] const AZ::Uuid& documentId){};

        // This notification is sent whenever graph compilation has completed.
        virtual void OnCompileGraphCompleted([[maybe_unused]] const AZ::Uuid& documentId){};

        // This notification is sent whenever graph compilation has failed.
        virtual void OnCompileGraphFailed([[maybe_unused]] const AZ::Uuid& documentId){};
    };

    using GraphCompilerNotificationBus = AZ::EBus<GraphCompilerNotifications>;
} // namespace AtomToolsFramework
