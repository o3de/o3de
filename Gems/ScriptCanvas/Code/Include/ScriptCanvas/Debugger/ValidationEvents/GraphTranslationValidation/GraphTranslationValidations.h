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
#include <ScriptCanvas/Debugger/ValidationEvents/GraphTranslationValidation/GraphTranslationValidationIds.h>

namespace ScriptCanvas
{
    class InvalidFunctionCallNameValidation
        : public ValidationEvent
    {
    public:
        AZ_RTTI(InvalidFunctionCallNameValidation, "{928CF581-2561-4AC2-9623-A9572A43A0DD}", ValidationEvent);
        AZ_CLASS_ALLOCATOR(InvalidFunctionCallNameValidation, AZ::SystemAllocator, 0);

        InvalidFunctionCallNameValidation(AZ::EntityId nodeId, SlotId slotId)
            : ValidationEvent(ValidationSeverity::Error)
        {
            SetDescription("This node has no implementation for getting a function call name on this slot");
        }

        AZStd::string GetIdentifier() const override
        {
            return GraphTranslationValidationIds::InvalidFunctionCallNameId;
        }

        AZ::Crc32 GetIdCrc() const override
        {
            return GraphTranslationValidationIds::InvalidFunctionCallNameCrc;
        }

        AZStd::string_view GetTooltip() const override
        {
            return GetDescription();
        }
    };

} // namespace ScriptCanvas