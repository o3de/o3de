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
    //! Interface for handling graph document status notifications
    class GraphDocumentNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::Crc32 BusIdType;

        // This notification is sent whenever graph compilation has started.
        virtual void OnCompileGraphStarted([[maybe_unused]] const AZ::Uuid& documentId){};

        // This notification is sent whenever graph compilation has completed.
        //! The graph compiler has written every generated file and is about to wait for the Asset Processor to build them.
        //!
        //! Raised between OnCompileGraphStarted and OnCompileGraphCompleted, and the gap between this and completion is the wait
        //! rather than any work: measured at roughly 600 ms of a 900 ms compile, against 287 ms for a compile that had nothing to
        //! wait for ("Skipping asset status wait: no generated file changed").
        //!
        //! For anything that only needs the generated files themselves, this is the point to start. GetGeneratedFilePaths is
        //! already populated. Anything that needs the built assets has to wait for OnCompileGraphCompleted as before.
        virtual void OnCompileGraphProcessing([[maybe_unused]] const AZ::Uuid& documentId){};

        virtual void OnCompileGraphCompleted([[maybe_unused]] const AZ::Uuid& documentId){};

        // This notification is sent whenever graph compilation has failed.
        virtual void OnCompileGraphFailed([[maybe_unused]] const AZ::Uuid& documentId){};
    };

    using GraphDocumentNotificationBus = AZ::EBus<GraphDocumentNotifications>;
} // namespace AtomToolsFramework
