/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Endpoint.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string_view.h>

namespace ScriptCanvas
{
    class Node;
    //! Execution RequestBus for interfacing with a running graph
    class ErrorReporter : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        //! BusIdType represents a unique id for the execution component
        //! Because multiple Script Canvas graphs can execute on the same entity
        //! this is not an "EntityId" in the sense that it uniquely identifies an entity.
        using BusIdType = ScriptCanvasId;

        virtual AZStd::string_view GetLastErrorDescription() const = 0;
        virtual void HandleError(const Node& callStackTop) = 0;
        virtual bool IsInErrorState() const = 0;
        virtual bool IsInIrrecoverableErrorState() const = 0;
        virtual void ReportError(const Node& reporter, const char* format, ...) = 0;
    };

    using ErrorReporterBus = AZ::EBus<ErrorReporter>;
}

#define SCRIPTCANVAS_HANDLE_ERROR(node)\
    bool inErrorState = false;\
    ScriptCanvas::ErrorReporterBus::EventResult(inErrorState, node.GetGraphId(), &ScriptCanvas::ErrorReporter::IsInErrorState);\
    if (inErrorState)\
    {\
        ScriptCanvas::ErrorReporterBus::Event(node.GetOwningScriptCanvasId(), &ScriptCanvas::ErrorReporter::HandleError, (node));\
    }

#define SCRIPTCANVAS_REPORT_ERROR(node, ...)\
    ScriptCanvas::ErrorReporterBus::Event(node.GetOwningScriptCanvasId(), &ScriptCanvas::ErrorReporter::ReportError, (node), __VA_ARGS__)

#define SCRIPTCANVAS_RETURN_IF_ERROR_STATE(node)\
    bool inErrorState = false;\
    ScriptCanvas::ErrorReporterBus::EventResult(inErrorState, node.GetOwningScriptCanvasId(), &ScriptCanvas::ErrorReporter::IsInErrorState);\
    if (inErrorState) { return; }  
