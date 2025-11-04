/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Crc.h>

namespace ScriptCanvas
{
    namespace DataValidationIds
    {
        // Used to self signal from the graph that an element has pushed its own internal error onto the stack
        constexpr const char* InternalValidationErrorId = "DV-0000";
        static const AZ::Crc32 InternalValidationErrorCrc = AZ_CRC_CE(InternalValidationErrorId);

        constexpr const char* ScopedDataConnectionId = "DV-0001";
        static const AZ::Crc32 ScopedDataConnectionCrc = AZ_CRC_CE(ScopedDataConnectionId);

        constexpr const char* UnknownTargetEndpointId = "DV-0002";
        static const AZ::Crc32 UnknownTargetEndpointCrc = AZ_CRC_CE(UnknownTargetEndpointId);

        constexpr const char* UnknownSourceEndpointId = "DV-0003";
        static const AZ::Crc32 UnknownSourceEndpointCrc = AZ_CRC_CE(UnknownSourceEndpointId);

        constexpr const char* InvalidVariableTypeId = "DV-0004";
        static const AZ::Crc32 InvalidVariableTypeCrc = AZ_CRC_CE(InvalidVariableTypeId);

        constexpr const char* ScriptEventVersionMismatchId = "DV-0005";
        static const AZ::Crc32 ScriptEventVersionMismatchCrc = AZ_CRC_CE(ScriptEventVersionMismatchId);

        constexpr const char* InvalidExpressionId = "DV-0006";
        static const AZ::Crc32 InvalidExpressionCrc = AZ_CRC_CE(InvalidExpressionId);

        constexpr const char* UnknownDataTypeId = "DV-0007";
        static const AZ::Crc32 UnknownDataTypeCrc = AZ_CRC_CE(UnknownDataTypeId);

        constexpr const char* InvalidReferenceId = "DV-0008";
        static const AZ::Crc32 InvalidReferenceCrc = AZ_CRC_CE(InvalidReferenceId);
        
        constexpr const char* InvalidRandomSignalId = "DV-0009";
        static const AZ::Crc32 InvalidRandomSignalCrc = AZ_CRC_CE(InvalidRandomSignalId);

        constexpr const char* InvalidPropertyId = "DV-0010";
        static const AZ::Crc32 InvalidPropertyCrc = AZ_CRC_CE(InvalidPropertyId);
    }
}
