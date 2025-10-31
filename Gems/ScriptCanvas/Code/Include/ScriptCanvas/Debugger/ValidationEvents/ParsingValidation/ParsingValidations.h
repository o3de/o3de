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
#include <ScriptCanvas/Debugger/ValidationEvents/ValidationEffects/HighlightEffect.h>
#include <ScriptCanvas/Debugger/ValidationEvents/ValidationEffects/FocusOnEffect.h>
#include <ScriptCanvas/Debugger/ValidationEvents/ParsingValidation/ParsingValidationIds.h>

#include <ScriptCanvas/Results/ErrorText.h>

namespace ScriptCanvas
{
    constexpr const char* InternalValidationErrorId = "DV-0000";
    static const AZ::Crc32 InternalValidationErrorCrc = AZ_CRC_CE(InternalValidationErrorId);


    //! Base class for all parser validation events, they will all share the same
    //! behavior
    class ParserValidation
        : public ValidationEvent
        , public HighlightEntityEffect
        , public FocusOnEntityEffect
    {
    public:

        AZ_RTTI(ParserValidation, "{1B91C6DC-B258-463C-B7EE-05338F6635E2}", ValidationEvent, HighlightEntityEffect, FocusOnEntityEffect);
        AZ_CLASS_ALLOCATOR(ParserValidation, AZ::SystemAllocator);

        ParserValidation
            ( AZ::EntityId nodeId
            , ValidationSeverity severity
            , const AZStd::string_view& description
            , AZ::Crc32 idCRC = {}
            , const AZStd::string& id = {})
            : ValidationEvent(severity)
            , m_nodeId(nodeId)
            , m_idCrc(idCRC)
            , m_identifier(id)
        {
            SetDescription(description);
        }

        AZStd::string GetIdentifier() const override
        {
            return m_identifier;
        }

        AZ::Crc32 GetIdCrc() const override
        {
            return m_idCrc;
        }

        AZStd::string_view GetTooltip() const override
        {
            return GetDescription();
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
        AZStd::string m_identifier;
        AZ::Crc32 m_idCrc;
    };

    // \todo this must be removed before it goes even into preview
    class NotYetImplemented
        : public ParserValidation
    {
    public:
        AZ_RTTI(NotYetImplemented, "{9439177C-DDFA-4B90-A6A4-8F9BEF8E6E0C}", ParserValidation);
        AZ_CLASS_ALLOCATOR(NotYetImplemented, AZ::SystemAllocator);

        NotYetImplemented(AZ::EntityId nodeId, const AZStd::string_view& description)
            : ParserValidation(nodeId, ValidationSeverity::Error, description)
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

    };

    class InactiveGraph
        : public ParserValidation
    {
    public:
        AZ_RTTI(InactiveGraph, "{315F5191-D990-40DA-9E92-1ADBA72CC00E}", ParserValidation);
        AZ_CLASS_ALLOCATOR(InactiveGraph, AZ::SystemAllocator);

        InactiveGraph()
            : ParserValidation(AZ::EntityId(), ValidationSeverity::Warning, ParseErrors::InactiveGraph)
        {
        }

        AZStd::string GetIdentifier() const override
        {
            return ParsingValidationIds::InactiveGraph;
        }

        AZ::Crc32 GetIdCrc() const override
        {
            return ParsingValidationIds::InactiveGraphCrc;
        }

    };

    class MultipleExecutionOutConnections
        : public ParserValidation
    {
    public:
        AZ_RTTI(MultipleExecutionOutConnections, "{2C7D74F0-382D-4C99-B2E8-A76C351B21DA}", ParserValidation);
        AZ_CLASS_ALLOCATOR(MultipleExecutionOutConnections, AZ::SystemAllocator);

        MultipleExecutionOutConnections(AZ::EntityId nodeId)
            : ParserValidation(nodeId, ValidationSeverity::Error, ParseErrors::MultipleExecutionOutConnections)
        {
        }

        AZStd::string GetIdentifier() const override
        {
            return ParsingValidationIds::MultipleExecutionOutConnections;
        }

        AZ::Crc32 GetIdCrc() const override
        {
            return ParsingValidationIds::MultipleExecutionOutConnectionsCrc;
        }

    };
    
    class MultipleStartNodes
        : public ParserValidation
    {
    public:
        AZ_RTTI(MultipleStartNodes, "{C6623D43-1D8F-4932-A426-E243A3C85A93}", ParserValidation);
        AZ_CLASS_ALLOCATOR(MultipleStartNodes, AZ::SystemAllocator);

        MultipleStartNodes(AZ::EntityId nodeId)
            : ParserValidation(nodeId, ValidationSeverity::Error, ParseErrors::MultipleStartNodes)
        {
        }

        AZStd::string GetIdentifier() const override
        {
            return ParsingValidationIds::MultipleStartNodes;
        }

        AZ::Crc32 GetIdCrc() const override
        {
            return ParsingValidationIds::MultipleStartNodesCrc;
        }

    };

    namespace NodeCompatiliblity
    {
        // TODO: fill link when page is ready
        static const AZStd::string k_newBackendMigrationGuideLink = "";

        class DependencyRetrievalFailiure
            : public ParserValidation
        {
        public:
            AZ_RTTI(DependencyRetrievalFailiure, "{5EDBD642-2EC8-402E-AC9D-DA0DF444A208}", ParserValidation);
            AZ_CLASS_ALLOCATOR(DependencyRetrievalFailiure, AZ::SystemAllocator);

            DependencyRetrievalFailiure(AZ::EntityId nodeId)
                : ParserValidation(nodeId, ValidationSeverity::Error, ParseErrors::DependencyRetrievalFailiure)
            {
            }

            AZStd::string GetIdentifier() const override
            {
                return ParsingValidationIds::DependencyRetrievalFailiure;
            }

            AZ::Crc32 GetIdCrc() const override
            {
                return ParsingValidationIds::DependencyRetrievalFailiureCrc;
            }
        };

        class NodeOutOfDate
            : public ParserValidation
        {
        public:
            AZ_RTTI(NodeOutOfDate, "{A4051A2D-E471-41C7-9D2C-A54418747AF8}", ParserValidation);
            AZ_CLASS_ALLOCATOR(NodeOutOfDate, AZ::SystemAllocator);

            NodeOutOfDate(AZ::EntityId nodeId, const AZStd::string_view& nodeName)
                : ParserValidation(nodeId, ValidationSeverity::Error, nodeName)
            {
                SetDescription(AZStd::string::format("Node (%s) is out of date.", nodeName.data()));
            }

            AZStd::string GetIdentifier() const override
            {
                return ParsingValidationIds::NodeOutOfDate;
            }

            AZ::Crc32 GetIdCrc() const override
            {
                return ParsingValidationIds::NodeOutOfDateCrc;
            }

        };

        class NewBackendUnsupportedNode
            : public ValidationEvent
        {
        public:
            AZ_RTTI(NewBackendUnsupportedNode, "{A4051A2D-E471-41C7-9D2C-A54418747AF8}", ValidationEvent);
            AZ_CLASS_ALLOCATOR(NewBackendUnsupportedNode, AZ::SystemAllocator);

            NewBackendUnsupportedNode(const AZ::EntityId& nodeId, const AZStd::string_view& nodeName)
                : ValidationEvent(ValidationSeverity::Error)
                , m_nodeId(nodeId)
            {
                SetDescription(AZStd::string::format("Node (%s) is not supported by new backend, please convert/remove it. %s", nodeName.data(), k_newBackendMigrationGuideLink.c_str()));
            }

            AZStd::string GetIdentifier() const override
            {
                return ParsingValidationIds::NewBackendUnsupportedNode;
            }

            AZ::Crc32 GetIdCrc() const override
            {
                return ParsingValidationIds::NewBackendUnsupportedNodeCrc;
            }

            const AZ::EntityId& GetNodeId() const
            {
                return m_nodeId;
            }

            AZStd::string_view GetTooltip() const override
            {
                return GetDescription();
            }

        private:
            AZ::EntityId m_nodeId;
        };
    }

    namespace Internal
    {
        class ParseError
            : public ParserValidation
        {
        public:
            AZ_RTTI(ParseError, "{1C36835A-2BAE-483A-BE13-5D1BEABB659B}", ParserValidation);
            AZ_CLASS_ALLOCATOR(ParseError, AZ::SystemAllocator);

            ParseError(AZ::EntityId nodeId, const AZStd::string_view& description)
                : ParserValidation(nodeId, ValidationSeverity::Error, description)
            {
            }

            AZStd::string GetIdentifier() const override
            {
                return ParsingValidationIds::ParseError;
            }

            AZ::Crc32 GetIdCrc() const override
            {
                return ParsingValidationIds::ParseErrorCrc;
            }

        };

        class AddOutputNameFailure
            : public ParserValidation
        {
        public:
            AZ_RTTI(AddOutputNameFailure, "{45BE27AA-A80B-45B1-BBD7-A174A5791764}", ParserValidation);
            AZ_CLASS_ALLOCATOR(AddOutputNameFailure, AZ::SystemAllocator);

            AddOutputNameFailure(AZ::EntityId nodeId, const AZStd::string_view&)
                : ParserValidation(nodeId, ValidationSeverity::Error, ParseErrors::AddOutputNameFailure)
            {
            }

            AZStd::string GetIdentifier() const override
            {
                return ParsingValidationIds::AddOutputNameFailure;
            }

            AZ::Crc32 GetIdCrc() const override
            {
                return ParsingValidationIds::AddOutputNameFailureCrc;
            }
        };

        class DuplicateInputProcessed
            : public ParserValidation
        {
        public:
            AZ_RTTI(DuplicateInputProcessed, "{69B056F5-7E10-4067-A50E-BDCE26222BD7}", ParserValidation);
            AZ_CLASS_ALLOCATOR(DuplicateInputProcessed, AZ::SystemAllocator);

            DuplicateInputProcessed(AZ::EntityId nodeId, const AZStd::string_view&)
                : ParserValidation(nodeId, ValidationSeverity::Error, ParseErrors::DuplicateInputProcessed)
            {
            }

            AZStd::string GetIdentifier() const override
            {
                return ParsingValidationIds::DuplicateInputProcessed;
            }

            AZ::Crc32 GetIdCrc() const override
            {
                return ParsingValidationIds::DuplicateInputProcessedCrc;
            }
        };

        class NullEntityInGraph
            : public ParserValidation
        {
        public:
            AZ_RTTI(NullEntityInGraph, "{920C0FBE-ADC0-45FF-A0C1-84ABF050FCFC}", ParserValidation);
            AZ_CLASS_ALLOCATOR(NullEntityInGraph, AZ::SystemAllocator);

            NullEntityInGraph()
                : ParserValidation(AZ::EntityId(), ValidationSeverity::Error, ParseErrors::NullEntityInGraph)
            {
            }

            AZStd::string GetIdentifier() const override
            {
                return ParsingValidationIds::NullEntityInGraph;
            }

            AZ::Crc32 GetIdCrc() const override
            {
                return ParsingValidationIds::NullEntityInGraphCrc;
            }
        };

        class NullNodeInGraph
            : public ParserValidation
        {
        public:
            AZ_RTTI(NullNodeInGraph, "{D5945CEF-149B-4065-9E60-58C17CD11864}", ParserValidation);
            AZ_CLASS_ALLOCATOR(NullNodeInGraph, AZ::SystemAllocator);

            NullNodeInGraph(AZ::EntityId nodeId, AZStd::string_view nodeName)
                : ParserValidation(nodeId, ValidationSeverity::Error, nodeName)
            {
                SetDescription(AZStd::string::format("null node pointer in graph: %s", nodeName.data()));
            }

            AZStd::string GetIdentifier() const override
            {
                return ParsingValidationIds::NullNodeInGraph;
            }

            AZ::Crc32 GetIdCrc() const override
            {
                return ParsingValidationIds::NullNodeInGraphCrc;
            }
        };
    }

} 
