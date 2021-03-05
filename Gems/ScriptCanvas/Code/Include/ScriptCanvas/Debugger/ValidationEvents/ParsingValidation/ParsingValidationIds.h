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

namespace ScriptCanvas
{
    constexpr const char* NotYetImplementedId = "SC-0000";
    static const AZ::Crc32 NotYetImplementedCrc = AZ_CRC(NotYetImplementedId);

    namespace ParsingValidationIds
    {
        constexpr const char* InactiveGraph = "MA-0001";
        static const AZ::Crc32 InactiveGraphCrc = AZ_CRC(InactiveGraph);

        constexpr const char* MultipleExecutionOutConnections = "MA-0002";
        static const AZ::Crc32 MultipleExecutionOutConnectionsCrc = AZ_CRC(MultipleExecutionOutConnections);

        constexpr const char* MultipleStartNodes = "MA-0003";
        static const AZ::Crc32 MultipleStartNodesCrc = AZ_CRC(MultipleStartNodes);       
    }

    namespace Internal
    {
        namespace ParsingValidationIds
        {
            constexpr const char* DuplicateInputProcessed = "MI-0001";
            static const AZ::Crc32 DuplicateInputProcessedCrc = AZ_CRC(DuplicateInputProcessed);

            constexpr const char* NullEntityInGraph = "MI-0002";
            static const AZ::Crc32 NullEntityInGraphCrc = AZ_CRC(NullEntityInGraph);

            constexpr const char* NullNodeInGraph = "MI-0003";
            static const AZ::Crc32 NullNodeInGraphCrc = AZ_CRC(NullNodeInGraph);
        }
    }

    namespace NodeCompatiliblity
    {
        namespace ParsingValidationIds
        {
            constexpr const char* DependencyRetrievalFailiure = "MN-0001";
            static const AZ::Crc32 DependencyRetrievalFailiureCrc = AZ_CRC(DependencyRetrievalFailiure);
        }
    }
}