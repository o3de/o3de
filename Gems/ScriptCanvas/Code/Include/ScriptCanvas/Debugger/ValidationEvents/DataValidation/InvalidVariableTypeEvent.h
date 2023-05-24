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
#include <ScriptCanvas/Debugger/ValidationEvents/ValidationEffects/HighlightEffect.h>

namespace ScriptCanvas
{
    class InvalidVariableTypeEvent
        : public ValidationEvent
        , public HighlightVariableEffect
    {
    public:
        AZ_CLASS_ALLOCATOR(InvalidVariableTypeEvent, AZ::SystemAllocator);
        AZ_RTTI(InvalidVariableTypeEvent, "{85670BDB-ED14-44BC-A7F1-FFF0D749EF72}", ValidationEvent, HighlightVariableEffect);
        
        InvalidVariableTypeEvent(const VariableId& variableId)
            : ValidationEvent(ValidationSeverity::Error)
            , m_variableId(variableId)
        {
            SetDescription(AZStd::string::format("Variable with id %s has an invalid type.", variableId.ToString().c_str()));
        }
        
        bool CanAutoFix() const override
        {
            return true;
        }
        
        AZStd::string GetIdentifier() const override
        {
            return DataValidationIds::InvalidVariableTypeId;
        }

        const VariableId& GetVariableId() const
        {
            return m_variableId;
        }
        
        AZ::Crc32 GetIdCrc() const override
        {
            return DataValidationIds::InvalidVariableTypeCrc;
        }

        AZStd::string_view GetTooltip() const override
        {
            return "Invalid type for variable, auto fixing will remove all invalid variable nodes.";
        }

        // HighlightVariableEffect
        VariableId GetHighlightVariableId() const override
        {
            return m_variableId;
        }
        ////
        
    private:
    
        VariableId m_variableId;
    };
}
