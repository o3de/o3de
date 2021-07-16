/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace ScriptCanvas
{
    constexpr const char* NotYetImplementedId = "SC-0000";
    static const AZ::Crc32 NotYetImplementedCrc = AZ_CRC(NotYetImplementedId);

    namespace ParsingValidationIds
    {
        constexpr const char* InactiveGraph = "PA-0001";
        static const AZ::Crc32 InactiveGraphCrc = AZ_CRC(InactiveGraph);

        constexpr const char* MultipleExecutionOutConnections = "PA-0002";
        static const AZ::Crc32 MultipleExecutionOutConnectionsCrc = AZ_CRC(MultipleExecutionOutConnections);

        constexpr const char* MultipleStartNodes = "PA-0003";
        static const AZ::Crc32 MultipleStartNodesCrc = AZ_CRC(MultipleStartNodes);       
        
    }

    namespace Internal
    {
        namespace ParsingValidationIds
        {
            constexpr const char* ParseError = "P0-0000";
            static const AZ::Crc32 ParseErrorCrc = AZ_CRC(ParseError);

            constexpr const char* DuplicateInputProcessed = "PI-0001";
            static const AZ::Crc32 DuplicateInputProcessedCrc = AZ_CRC(DuplicateInputProcessed);

            constexpr const char* NullEntityInGraph = "PI-0002";
            static const AZ::Crc32 NullEntityInGraphCrc = AZ_CRC(NullEntityInGraph);

            constexpr const char* NullNodeInGraph = "PI-0003";
            static const AZ::Crc32 NullNodeInGraphCrc = AZ_CRC(NullNodeInGraph);

            constexpr const char* AddOutputNameFailure = "PI-0004";
            static const AZ::Crc32 AddOutputNameFailureCrc = AZ_CRC(AddOutputNameFailure);
        }
    }

    namespace NodeCompatiliblity
    {
        namespace ParsingValidationIds
        {
            constexpr const char* DependencyRetrievalFailiure = "PN-0001";
            static const AZ::Crc32 DependencyRetrievalFailiureCrc = AZ_CRC(DependencyRetrievalFailiure);

            constexpr const char* NodeOutOfDate = "PN-0002";
            static const AZ::Crc32 NodeOutOfDateCrc = AZ_CRC(NodeOutOfDate);

            constexpr const char* NewBackendUnsupportedNode = "PN-0003";
            static const AZ::Crc32 NewBackendUnsupportedNodeCrc = AZ_CRC(NewBackendUnsupportedNode);
        }
    }
}
