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
    //! Bus interface containing common graph compiler functions for queuing and checking the results of graph builds
    class GraphCompilerRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::Uuid BusIdType;

        //! Get a list of all of the generated files from the last time this graph was compiled.
        virtual const AZStd::vector<AZStd::string>& GetGeneratedFilePaths() const = 0;

        //! Get the graph export path based on the document path or default export path.
        virtual AZStd::string GetGraphPath() const = 0;

        //! Evaluate the graph nodes, slots, values, and settings to generate and export data.
        virtual bool CompileGraph() = 0;

        //! Schedule the graph to be compiled on the next system tick.
        virtual void QueueCompileGraph() = 0;

        //! Returns true if graph compilation has already been scheduled.
        virtual bool IsCompileGraphQueued() const = 0;

        //! Requests and reports job status of generated files from the AP.
        //! Return true if generation and processing is complete. Otherwise, return falss.
        virtual bool ReportGeneratedFileStatus() = 0;
    };

    using GraphCompilerRequestBus = AZ::EBus<GraphCompilerRequests>;
} // namespace AtomToolsFramework
