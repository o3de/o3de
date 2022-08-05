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

namespace MaterialCanvas
{
    class MaterialCanvasDocumentRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::Uuid BusIdType;

        // Get the graph canvas scene ID for this document.
        virtual GraphCanvas::GraphId GetGraphId() const = 0;

        // Get a list of all of the generated files from the last time this graph was compiled.
        virtual const AZStd::vector<AZStd::string>& GetGeneratedFilePaths() const = 0;

        // Convert the document file name into one that can be used as a symbol in graph template files.
        virtual AZStd::string GetGraphName() const = 0;

        // Evaluate the graph nodes, slots, values, and settings to generate and export shaders, material types, and materials.
        virtual bool CompileGraph() const = 0;

        // Schedule the graph to be compiled on the next system tick.
        virtual void QueueCompileGraph() const = 0;

        // Returns true if graph compilation has already been scheduled.
        virtual bool IsCompileGraphQueued() const = 0;
    };

    using MaterialCanvasDocumentRequestBus = AZ::EBus<MaterialCanvasDocumentRequests>;
} // namespace MaterialCanvas
