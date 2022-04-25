/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MultiplayerCompressionFactory.h"
#include "LZ4Compressor.h"

#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace MultiplayerCompression
{
    AZStd::unique_ptr<AzNetworking::ICompressor> MultiplayerCompressionFactory::Create()
    {
        return AZStd::make_unique<LZ4Compressor>();
    }

    const AZStd::string_view MultiplayerCompressionFactory::GetFactoryName() const
    {
        return s_compressorName;
    }
}
