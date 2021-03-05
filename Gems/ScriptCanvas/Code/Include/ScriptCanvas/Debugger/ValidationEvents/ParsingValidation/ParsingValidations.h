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
#include <ScriptCanvas/Debugger/ValidationEvents/ParsingValidation/ParsingValidationIds.h>

namespace ScriptCanvas
{
    // \todo this must be removed before it goes even into preview
    class NotYetImplemented
        : public ValidationEvent
    {
    public:
        AZ_RTTI(NotYetImplemented, "{9439177C-DDFA-4B90-A6A4-8F9BEF8E6E0C}", ValidationEvent);
        AZ_CLASS_ALLOCATOR(NotYetImplemented, AZ::SystemAllocator, 0);

        NotYetImplemented(const AZStd::string_view& description)
            : ValidationEvent(ValidationSeverity::Error)
        {
            SetDescription(AZStd::string::format("%s is not yet implemented", description.data()));
        }

        AZStd::string GetIdentifier() const override
        {
            return NotYetImplementedId;
        }

        AZ::Crc32 GetIdCrc() const override
        {
            return NotYetImplementedCrc;
        }

        AZStd::string_view GetTooltip() const override
        {
            return GetDescription();
        }
    };

    class InactiveGraph
        : public ValidationEvent
    {
    public:
        AZ_RTTI(InactiveGraph, "{315F5191-D990-40DA-9E92-1ADBA72CC00E}", ValidationEvent);
        AZ_CLASS_ALLOCATOR(InactiveGraph, AZ::SystemAllocator, 0);

        InactiveGraph()
            : ValidationEvent(ValidationSeverity::Warning)
        {
            SetDescription("This graph is not a library, but it is never activated and will never execute. Add a Start node or connect an event handler.");
        }

        AZStd::string GetIdentifier() const override
        {
            return ParsingValidationIds::InactiveGraph;
        }

        AZ::Crc32 GetIdCrc() const override
        {
            return ParsingValidationIds::InactiveGraphCrc;
        }

        AZStd::string_view GetTooltip() const override
        {
            return GetDescription();
        }
    };

    class MultipleExecutionOutConnections
        : public ValidationEvent
    {
    public:
        AZ_RTTI(MultipleExecutionOutConnections, "{2C7D74F0-382D-4C99-B2E8-A76C351B21DA}", ValidationEvent);
        AZ_CLASS_ALLOCATOR(MultipleExecutionOutConnections, AZ::SystemAllocator, 0);

        MultipleExecutionOutConnections(AZ::EntityId nodeId)
            : ValidationEvent(ValidationSeverity::Error)
        {
            SetDescription("This node has multiple, unordered Excution Out connections");
        }

        AZStd::string GetIdentifier() const override
        {
            return ParsingValidationIds::MultipleExecutionOutConnections;
        }

        AZ::Crc32 GetIdCrc() const override
        {
            return ParsingValidationIds::MultipleExecutionOutConnectionsCrc;
        }

        AZStd::string_view GetTooltip() const override
        {
            return GetDescription();
        }
    };
    
    class MultipleStartNodes
        : public ValidationEvent
    {
    public:
        AZ_RTTI(MultipleStartNodes, "{C6623D43-1D8F-4932-A426-E243A3C85A93}", ValidationEvent);
        AZ_CLASS_ALLOCATOR(MultipleStartNodes, AZ::SystemAllocator, 0);

        MultipleStartNodes(AZ::EntityId nodeId)
            : ValidationEvent(ValidationSeverity::Error)
        {
            SetDescription("Multiple Start nodes in a single graph. Only one is allowed.");
        }

        AZStd::string GetIdentifier() const override
        {
            return ParsingValidationIds::MultipleStartNodes;
        }

        AZ::Crc32 GetIdCrc() const override
        {
            return ParsingValidationIds::MultipleStartNodesCrc;
        }

        AZStd::string_view GetTooltip() const override
        {
            return GetDescription();
        }
    };

    namespace NodeCompatiliblity
    {
        class DependencyRetrievalFailiure
            : public ValidationEvent
        {
        public:
            AZ_RTTI(DependencyRetrievalFailiure, "{5EDBD642-2EC8-402E-AC9D-DA0DF444A208}", ValidationEvent);
            AZ_CLASS_ALLOCATOR(DependencyRetrievalFailiure, AZ::SystemAllocator, 0);

            DependencyRetrievalFailiure(AZ::EntityId nodeId)
                : ValidationEvent(ValidationSeverity::Error)
            {
                SetDescription("A node failed to retrieve its dependencies.");
            }

            AZStd::string GetIdentifier() const override
            {
                return ParsingValidationIds::DependencyRetrievalFailiure;
            }

            AZ::Crc32 GetIdCrc() const override
            {
                return ParsingValidationIds::DependencyRetrievalFailiureCrc;
            }

            AZStd::string_view GetTooltip() const override
            {
                return GetDescription();
            }
        };
    }

    namespace Internal
    {
        class DuplicateInputProcessed
            : public ValidationEvent
        {
        public:
            AZ_RTTI(DuplicateInputProcessed, "{69B056F5-7E10-4067-A50E-BDCE26222BD7}", ValidationEvent);
            AZ_CLASS_ALLOCATOR(DuplicateInputProcessed, AZ::SystemAllocator, 0);

            DuplicateInputProcessed(AZ::EntityId nodeId)
                : ValidationEvent(ValidationSeverity::Error)
            {
                SetDescription("input to the slot at this execution has already been found");
            }

            AZStd::string GetIdentifier() const override
            {
                return ParsingValidationIds::DuplicateInputProcessed;
            }

            AZ::Crc32 GetIdCrc() const override
            {
                return ParsingValidationIds::DuplicateInputProcessedCrc;
            }

            AZStd::string_view GetTooltip() const override
            {
                return GetDescription();
            }
        };

        class NullEntityInGraph
            : public ValidationEvent
        {
        public:
            AZ_RTTI(NullEntityInGraph, "{920C0FBE-ADC0-45FF-A0C1-84ABF050FCFC}", ValidationEvent);
            AZ_CLASS_ALLOCATOR(NullEntityInGraph, AZ::SystemAllocator, 0);

            NullEntityInGraph()
                : ValidationEvent(ValidationSeverity::Error)
            {
                SetDescription("null entity pointer in graph");
            }

            AZStd::string GetIdentifier() const override
            {
                return ParsingValidationIds::NullEntityInGraph;
            }

            AZ::Crc32 GetIdCrc() const override
            {
                return ParsingValidationIds::NullEntityInGraphCrc;
            }

            AZStd::string_view GetTooltip() const override
            {
                return GetDescription();
            }
        };

        class NullNodeInGraph
            : public ValidationEvent
        {
        public:
            AZ_RTTI(NullNodeInGraph, "{D5945CEF-149B-4065-9E60-58C17CD11864}", ValidationEvent);
            AZ_CLASS_ALLOCATOR(NullNodeInGraph, AZ::SystemAllocator, 0);

            NullNodeInGraph(AZ::EntityId nodeId)
                : ValidationEvent(ValidationSeverity::Error)
            {
                SetDescription("null node pointer in graph");
            }

            AZStd::string GetIdentifier() const override
            {
                return ParsingValidationIds::NullNodeInGraph;
            }

            AZ::Crc32 GetIdCrc() const override
            {
                return ParsingValidationIds::NullNodeInGraphCrc;
            }

            AZStd::string_view GetTooltip() const override
            {
                return GetDescription();
            }
        };
    }

} // namespace ScriptCanvas