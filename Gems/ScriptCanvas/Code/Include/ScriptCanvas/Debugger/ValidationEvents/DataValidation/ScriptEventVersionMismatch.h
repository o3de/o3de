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
        
        bool CanAutoFix() const
        {
            return false;
        }
        
        AZStd::string GetIdentifier() const
        {
            return DataValidationIds::ScriptEventVersionMismatchId;
        }
        
        AZ::Crc32 GetIdCrc() const
        {
            return DataValidationIds::ScriptEventVersionMismatchCrc;
        }
        
        const ScriptEvents::ScriptEvent& GetVariableId() const
        {
            return m_definition;
        }

        AZStd::string_view GetTooltip() const
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
