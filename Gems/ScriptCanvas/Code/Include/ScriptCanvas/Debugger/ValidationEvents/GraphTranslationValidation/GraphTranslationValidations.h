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
#include <ScriptCanvas/Debugger/ValidationEvents/GraphTranslationValidation/GraphTranslationValidationIds.h>

namespace ScriptCanvas
{
    class InvalidFunctionCallNameValidation
        : public ValidationEvent
    {
    public:
        AZ_RTTI(InvalidFunctionCallNameValidation, "{928CF581-2561-4AC2-9623-A9572A43A0DD}", ValidationEvent);
        AZ_CLASS_ALLOCATOR(InvalidFunctionCallNameValidation, AZ::SystemAllocator);

        InvalidFunctionCallNameValidation(AZ::EntityId, SlotId)
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

} 
