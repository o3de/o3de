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
