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

namespace ScriptCanvas
{
    class UnknownEndpointEvent
        : public ValidationEvent
    {
    public:
        AZ_RTTI(UnknownEndpointEvent, "{F1987F1F-E335-4C76-AA00-AD30EA5A51B3}", ValidationEvent);
        AZ_CLASS_ALLOCATOR(UnknownEndpointEvent, AZ::SystemAllocator, 0);

    protected:
        UnknownEndpointEvent(ValidationSeverity validationType, const Endpoint& endpoint)
            : ValidationEvent(validationType)
            , m_endpoint(endpoint)
        {
            SetDescription("Unknown Endpoint specified for use.");
        }

    private:
    
        Endpoint m_endpoint;
    };
    
    class UnknownTargetEndpointEvent
        : public UnknownEndpointEvent
    {
    public:
        AZ_RTTI(UnknownTargetEndpointEvent, "{0C6D8D73-A174-4548-BE06-00962A601668}", UnknownEndpointEvent);
        AZ_CLASS_ALLOCATOR(UnknownTargetEndpointEvent, AZ::SystemAllocator, 0);
        
        UnknownTargetEndpointEvent(const Endpoint& endpoint)
            : UnknownEndpointEvent(ValidationSeverity::Warning, endpoint)
        {
        }
        
        AZStd::string GetIdentifier() const override
        {
            return DataValidationIds::UnknownTargetEndpointId;
        }
        
        AZ::Crc32 GetIdCrc() const override
        {
            return DataValidationIds::UnknownTargetEndpointCrc;
        }

        AZStd::string_view GetTooltip() const override
        {
            return "Unknown Target Endpoint";
        }
    };
    
    class UnknownSourceEndpointEvent
        : public UnknownEndpointEvent
    {
    public:
        AZ_RTTI(UnknownSourceEndpointEvent, "{7794A870-E64D-47FE-A73A-CA38378E3C4D}", UnknownEndpointEvent);
        AZ_CLASS_ALLOCATOR(UnknownSourceEndpointEvent, AZ::SystemAllocator, 0);
        
        UnknownSourceEndpointEvent(const Endpoint& endpoint)
            : UnknownEndpointEvent(ValidationSeverity::Warning, endpoint)
        {
        }
        
        AZStd::string GetIdentifier() const override
        {
            return DataValidationIds::UnknownSourceEndpointId;
        }
        
        AZ::Crc32 GetIdCrc() const override
        {
            return DataValidationIds::UnknownSourceEndpointCrc;
        }

        AZStd::string_view GetTooltip() const
        {
            return "Unknown Source Endpoint";
        }
    };
}