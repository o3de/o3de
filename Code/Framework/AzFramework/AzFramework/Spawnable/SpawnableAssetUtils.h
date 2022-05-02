/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetManager.h>

namespace AzFramework::SpawnableAssetUtils
{
    void ResolveEntityAliases(
        class Spawnable* spawnable,
        const AZStd::string& spawnableName,
        AZStd::chrono::milliseconds streamingDeadline = {},
        AZ::IO::IStreamerTypes::Priority streamingPriority = {},
        const AZ::Data::AssetFilterCB& assetLoadFilterCB = {});
} // AzFramework::SpawnableAssetUtils
