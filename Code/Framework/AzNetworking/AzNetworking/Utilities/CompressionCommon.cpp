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

#include <AzCore/Console/IConsole.h>
#include <AzNetworking/Framework/ICompressor.h>
#include <AzNetworking/Utilities/CompressionCommon.h>

#ifdef ENABLE_MULTIPLAYER_COMPRESSION
// Requires the MultiplayerCompression Gem be enabled and set as a dependency of the Multiplayer Gem
#include <MultiplayerCompression/MultiplayerCompressionBus.h>
#endif

namespace AzNetworking
{
    AZStd::unique_ptr<ICompressor> CreateCompressor(AZStd::string_view compressorName)
    {
        CompressorType compressorType = aznumeric_cast<AzNetworking::CompressorType>(static_cast<AZ::u32>(AZ::Crc32(compressorName)));

#ifdef ENABLE_MULTIPLAYER_COMPRESSION
        // Requires the MultiplayerCompression Gem be enabled and set as a dependency of the Multiplayer Gem
        CompressorType lz4Type;
        MultiplayerCompression::MultiplayerCompressionRequestBus::BroadcastResult(lz4Type, &MultiplayerCompression::MultiplayerCompressionRequests::GetType);
        if (lz4Type == compressorType)
        {
            AZStd::shared_ptr<ICompressorFactory> compressorFactory;
            MultiplayerCompression::MultiplayerCompressionRequestBus::BroadcastResult(compressorFactory, &MultiplayerCompression::MultiplayerCompressionRequests::GetCompressionFactory);
            return compressorFactory->Create();
        }
#endif

        AZ_Warning("CompressionCommon", false, "No compressor was found matching %.*s, check that related Gems are enabled.", 
            aznumeric_cast<int>(compressorName.size()), compressorName.data());
        return nullptr;
    }
}
