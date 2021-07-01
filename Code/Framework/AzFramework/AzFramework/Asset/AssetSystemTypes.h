/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AzFramework
{
    namespace AssetSystem
    {
        // Note: this must be kept in sync with the engine's AssetStatus enum
        enum AssetStatus
        {
            AssetStatus_Unknown,
            AssetStatus_Missing,
            AssetStatus_Queued,
            AssetStatus_Compiling,
            AssetStatus_Compiled,
            AssetStatus_Failed,
        };

        enum NegotiationInfo
        {
            NegotiationInfo_ProcessId,
            NegotiationInfo_Platform,
            NegotiationInfo_BranchIndentifier,
            NegotiationInfo_ProjectName,
        };

        const unsigned int DEFAULT_SERIAL = 0;
        const unsigned int NEGOTIATION_SERIAL = 0x0fffffff;
        const unsigned int RESPONSE_SERIAL_FLAG = (1U << 31);

        //This enum should have the list of all asset system errors
        enum AssetSystemErrors
        {
            ASSETSYSTEM_FAILED_TO_LAUNCH_ASSETPROCESSOR,        // not able to launch the AssetProcessor
            ASSETSYSTEM_FAILED_TO_CONNECT_TO_ASSETPROCESSOR,    // not able to connect to the AssetProcessor
        };
    } // namespace AssetMessage
} // namespace AzFramework
