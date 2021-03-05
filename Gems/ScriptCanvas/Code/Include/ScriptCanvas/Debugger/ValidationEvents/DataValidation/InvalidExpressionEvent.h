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

#include <ScriptCanvas/Core/Endpoint.h>
#include <ScriptCanvas/Debugger/ValidationEvents/DataValidation/DataValidationIds.h>

#include <ScriptCanvas/Debugger/ValidationEvents/ValidationEffects/FocusOnEffect.h>
#include <ScriptCanvas/Debugger/ValidationEvents/ValidationEffects/HighlightEffect.h>

namespace ScriptCanvas
{    
    class InvalidExpressionEvent
        : public ValidationEvent
        , public HighlightEntityEffect
        , public FocusOnEntityEffect
    {
    public:
        AZ_RTTI(InvalidExpressionEvent, "{85F7836A-FAAF-4BD5-A181-4E0CF9798FA0}", HighlightEntityEffect, ValidationEvent, FocusOnEntityEffect);
        AZ_CLASS_ALLOCATOR(InvalidExpressionEvent, AZ::SystemAllocator, 0);
        
        InvalidExpressionEvent(const AZ::EntityId& nodeId, const AZStd::string& parseError)
            : ValidationEvent(ValidationSeverity::Error)
            , m_nodeId(nodeId)
        {
            SetDescription(parseError);
        }
        
        AZStd::string GetIdentifier() const override
        {
            return DataValidationIds::InvalidExpressionId;
        }
        
        AZ::Crc32 GetIdCrc() const override
        {
            return DataValidationIds::InvalidExpressionCrc;
        }

        AZStd::string_view GetTooltip() const override
        {
            return "The Expression node has accounted an error during parsing. This error will need to be corrected before it can be executed";
        }

        // HighlightEffect
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

        AZ::EntityId m_nodeId;
    };
}
