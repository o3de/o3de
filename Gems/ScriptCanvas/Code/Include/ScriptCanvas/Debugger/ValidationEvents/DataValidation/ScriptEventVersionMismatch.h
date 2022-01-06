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

#include <ScriptCanvas/Debugger/ValidationEvents/DataValidation/DataValidationIds.h>
#include <ScriptEvents/ScriptEventDefinition.h>

#include <ScriptCanvas/Debugger/ValidationEvents/ValidationEffects/FocusOnEffect.h>
#include <ScriptCanvas/Debugger/ValidationEvents/ValidationEffects/HighlightEffect.h>

namespace ScriptCanvas
{
    class ScriptEventVersionMismatch
        : public ValidationEvent
        , public HighlightEntityEffect
        , public FocusOnEntityEffect
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptEventVersionMismatch, AZ::SystemAllocator, 0);
        AZ_RTTI(ScriptEventVersionMismatch, "{4968A689-B45A-40B6-BB3C-B1D35557D692}", ValidationEvent, HighlightEntityEffect, FocusOnEntityEffect);
        
        ScriptEventVersionMismatch(AZ::u32 nodeVersion, const ScriptEvents::ScriptEvent& definition, AZ::EntityId nodeId)
            : ValidationEvent(ValidationSeverity::Error)
            , m_definition(definition)
            , m_nodeVersion(nodeVersion)
            , m_nodeId(nodeId)
        {
            SetDescription("The Script Event asset this node uses has changed. This node is no longer valid. You can fix this by deleting this node, re-adding it and reconnecting it.");
        }
        
        bool CanAutoFix() const override
        {
            return false;
        }
        
        AZStd::string GetIdentifier() const override
        {
            return DataValidationIds::ScriptEventVersionMismatchId;
        }
        
        AZ::Crc32 GetIdCrc() const override
        {
            return DataValidationIds::ScriptEventVersionMismatchCrc;
        }
        
        const ScriptEvents::ScriptEvent& GetVariableId() const
        {
            return m_definition;
        }

        AZStd::string_view GetTooltip() const override
        {
            return "The Script Event asset has changed, you can fix this problem by deleting the out of date node and re-adding it to your graph.";
        }

        AZ::EntityId GetNodeId() const
        {
            return m_nodeId;
        }

        // HighlightEntityEffect
        AZ::EntityId GetHighlightTarget() const override
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
    
        ScriptEvents::ScriptEvent m_definition;
        AZ::u32 m_nodeVersion;
        AZ::EntityId m_nodeId;
    };
}
