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

#include <ScriptCanvas/Core/Endpoint.h>
#include <ScriptCanvas/Debugger/ValidationEvents/DataValidation/DataValidationIds.h>

#include <ScriptCanvas/Debugger/ValidationEvents/ValidationEffects/FocusOnEffect.h>
#include <ScriptCanvas/Debugger/ValidationEvents/ValidationEffects/HighlightEffect.h>

namespace ScriptCanvas
{    
    class InvalidPropertyEvent
        : public ValidationEvent
        , public HighlightEntityEffect
        , public FocusOnEntityEffect
    {
    public:
        AZ_RTTI(InvalidPropertyEvent, "{85F7836A-FAAF-4BD5-A181-4E0CF9798FA0}", HighlightEntityEffect, ValidationEvent, FocusOnEntityEffect);
        AZ_CLASS_ALLOCATOR(InvalidPropertyEvent, AZ::SystemAllocator);
        
        InvalidPropertyEvent(const AZ::EntityId& nodeId, const AZStd::string& parseError)
            : ValidationEvent(ValidationSeverity::Error)
            , m_nodeId(nodeId)
        {
            SetDescription(parseError);
        }
        
        AZStd::string GetIdentifier() const override
        {
            return DataValidationIds::InvalidPropertyId;
        }
        
        AZ::Crc32 GetIdCrc() const override
        {
            return DataValidationIds::InvalidPropertyCrc;
        }
        
        void SetTooltip(const AZStd::string& tooltip)
        {
            m_tooltip = tooltip;
        }

        AZStd::string_view GetTooltip() const override
        {
            return m_tooltip;
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

        AZStd::string m_tooltip;
        AZ::EntityId m_nodeId;
    };
}
