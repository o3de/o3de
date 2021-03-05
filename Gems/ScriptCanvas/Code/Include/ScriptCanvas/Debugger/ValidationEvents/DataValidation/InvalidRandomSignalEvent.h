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
    class InvalidRandomSignalEvent
        : public ValidationEvent
        , public HighlightEntityEffect
        , public FocusOnEntityEffect
    {
    public:
        AZ_RTTI(InvalidRandomSignalEvent, "{79B8E967-3852-4A8E-A0B9-22BFA68A04F1}", HighlightEntityEffect, ValidationEvent, FocusOnEntityEffect);
        AZ_CLASS_ALLOCATOR(InvalidRandomSignalEvent, AZ::SystemAllocator, 0);
        
        InvalidRandomSignalEvent(const AZ::EntityId& nodeId)
            : ValidationEvent(ValidationSeverity::Error)
            , m_nodeId(nodeId)
        {
            SetDescription("The Random Signal Node will not execute correctly since all weights are 0.");
        }
        
        AZStd::string GetIdentifier() const override
        {
            return DataValidationIds::InvalidRandomSignalId;
        }
        
        AZ::Crc32 GetIdCrc() const override
        {
            return DataValidationIds::InvalidRandomSignalCrc;
        }

        AZStd::string_view GetTooltip() const override
        {
            return "All outs from this Random Signal have a weight at 0. No out result will be triggered because of this.";
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
