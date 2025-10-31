/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <GraphCanvas/Editor/EditorTypes.h>
#include <GraphModel/Model/DataType.h>

namespace AtomToolsFramework
{
    //! GraphDocumentRequests establishes a common interface for graph model graphs managed by the document system
    class GraphDocumentRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::Uuid BusIdType;

        // Get the graph model graph pointer for this document.
        virtual GraphModel::GraphPtr GetGraph() const = 0;

        // Get the graph canvas scene ID for this document.
        virtual GraphCanvas::GraphId GetGraphId() const = 0;

        // Convert the document file name into one that can be used as a symbol.
        virtual AZStd::string GetGraphName() const = 0;

        //! Set a list of all of the generated files from the last time this graph was compiled.
        virtual void SetGeneratedFilePaths(const AZStd::vector<AZStd::string>& pathas) = 0;

        //! Get a list of all of the generated files from the last time this graph was compiled.
        virtual const AZStd::vector<AZStd::string>& GetGeneratedFilePaths() const = 0;

        //! Evaluate the graph nodes, slots, values, and settings to generate and export data.
        virtual bool CompileGraph() = 0;

        //! Schedule the graph to be compiled on the next system tick.
        virtual void QueueCompileGraph() = 0;

        //! Returns true if graph compilation has already been scheduled.
        virtual bool IsCompileGraphQueued() const = 0;
    };

    using GraphDocumentRequestBus = AZ::EBus<GraphDocumentRequests>;
} // namespace AtomToolsFramework
