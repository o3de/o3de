/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <ScriptCanvas/Core/ExecutionNotificationsBus.h>

#include <ScriptCanvas/Core/Core.h>

namespace ScriptCanvas
{
    class Node;
    struct SlotId;
    
    GraphInfo CreateGraphInfo(ScriptCanvasId executionId, const GraphIdentifier& graphIdentifier);
    DatumValue CreateVariableDatumValue(ScriptCanvasId scriptCanvasId, const GraphVariable& graphVariable);
    DatumValue CreateVariableDatumValue(ScriptCanvasId scriptCanvasId, const Datum& variableDatum, const VariableId& variableId);

    //! Execution Request Bus that triggers graph Execution and supports adding nodes
    //! to the execution stack
    class ExecutionRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        //! BusIdType represents a unique id for the execution component
        //! Because multiple Script Canvas graphs can execute on the same entity
        //! this is not an "EntityId" in the sense that it uniquely identifies an entity.
        using BusIdType = ScriptCanvasId;

        // Adds the node to the Execution stack where the slotId represents the slot that triggered the execution
        virtual void AddToExecutionStack(Node& node, const SlotId& slotId) = 0;
        // Starts or resumes execution of the graph
        virtual void Execute() = 0;
        virtual void ExecuteUntilNodeIsTopOfStack(Node&) = 0;
        virtual bool IsExecuting() const = 0;
    };

    using ExecutionRequestBus = AZ::EBus<ExecutionRequests>;
}
