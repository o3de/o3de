/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/EntityId.h>

#include <ScriptCanvas/Debugger/ValidationEvents/ValidationEvent.h>

#include <ScriptCanvas/Debugger/ValidationEvents/ExecutionValidation/ExecutionValidationIds.h>

#include <ScriptCanvas/Debugger/ValidationEvents/ValidationEffects/FocusOnEffect.h>
#include <ScriptCanvas/Debugger/ValidationEvents/ValidationEffects/GreyOutEffect.h>
#include <ScriptCanvas/Debugger/ValidationEvents/ValidationEffects/HighlightEffect.h>

namespace ScriptCanvas
{
    // UnusedNodeEvent
    // An event that indicated that a node that is not a start point, does not have an execution in.
    // And thus will never execute.
    class UnusedNodeEvent
        : public ValidationEvent
        , public HighlightEntityEffect
        , public GreyOutNodeEffect
        , public FocusOnEntityEffect
    {
    public:
        AZ_RTTI(UnusedNodeEvent, "{EC6933F8-0D50-49A7-BCA2-BB4B4534AA8C}", HighlightEntityEffect, GreyOutNodeEffect, ValidationEvent, FocusOnEntityEffect);
        AZ_CLASS_ALLOCATOR(UnusedNodeEvent, AZ::SystemAllocator);
        
        UnusedNodeEvent(const AZ::EntityId& nodeId)
            : ValidationEvent(ValidationSeverity::Warning)
            , m_nodeId(nodeId)
        {
            SetDescription("Node is not marked as an entry point to the graph, and has no incoming connections. Node will not be executed.");
        }
        
        AZStd::string GetIdentifier() const override
        {
            return ExecutionValidationIds::UnusedNodeId;
        }
        
        AZ::Crc32 GetIdCrc() const override
        {
            return AZ_CRC_CE(ExecutionValidationIds::UnusedNodeId);
        }
        
        const AZ::EntityId& GetNodeId() const
        {
            return m_nodeId;
        }

        AZStd::string_view GetTooltip() const override
        {
            return "Unused Node";
        }

        // HighlightEntityEffect
        AZ::EntityId GetHighlightTarget() const override
        {
            return m_nodeId;
        }
        ////

        // GreyOutNodeEffect
        AZ::EntityId GetGreyOutNodeId() const override
        {
            return m_nodeId;
        }
        ////

        // FocusOnEntityEffect
        AZ::EntityId GetFocusTarget() const override
        {
            return m_nodeId;
        }
        ////
        
    private:
    
        AZ::EntityId m_nodeId;
    };
}
