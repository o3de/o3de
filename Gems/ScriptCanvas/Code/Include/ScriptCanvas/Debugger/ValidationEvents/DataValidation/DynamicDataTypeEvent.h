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
#include <AzCore/std/containers/vector.h>

#include <ScriptCanvas/Debugger/ValidationEvents/ValidationEvent.h>

#include <ScriptCanvas/Core/Endpoint.h>
#include <ScriptCanvas/Debugger/ValidationEvents/DataValidation/DataValidationIds.h>

#include <ScriptCanvas/Debugger/ValidationEvents/ValidationEffects/FocusOnEffect.h>
#include <ScriptCanvas/Debugger/ValidationEvents/ValidationEffects/HighlightEffect.h>

namespace ScriptCanvas
{
    class UnspecifiedDynamicDataTypeEvent
        : public ValidationEvent
        , public HighlightEntityEffect
        , public FocusOnEntityEffect
    {
    public:
        AZ_RTTI(UnspecifiedDynamicDataTypeEvent, "{429702EE-E08A-47C3-A489-1029A7F27DD9}", ValidationEvent, HighlightEntityEffect, FocusOnEntityEffect);
        AZ_CLASS_ALLOCATOR(UnspecifiedDynamicDataTypeEvent, AZ::SystemAllocator, 0);

        UnspecifiedDynamicDataTypeEvent(const AZ::EntityId& nodeId, const AZStd::vector<SlotId>& slots)
            : ValidationEvent(ValidationSeverity::Error)
            , m_nodeId(nodeId)
            , m_slots(slots)
        {
            SetDescription("Data type not set for Dynamic Data Slots. Node cannot execute properly due to missing data.");
        }

        AZStd::string GetIdentifier() const override
        {
            return DataValidationIds::UnknownDataTypeId;
        }

        AZ::Crc32 GetIdCrc() const override
        {
            return DataValidationIds::UnknownDataTypeCrc;
        }

        AZStd::string_view GetTooltip() const override
        {
            return "Unspecified Dynamic Data Type";
        }

        const AZ::EntityId& GetNodeId() const
        {
            return m_nodeId;
        }

        const AZStd::vector<SlotId>& GetSlots() const
        {
            return m_slots;
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

        AZ::EntityId m_nodeId;
        AZStd::vector<SlotId> m_slots;
    };
}
