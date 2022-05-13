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
    //! Called as part of the creation of a spawnable asset from loaded file data or in-memory data.
    //! Queues required spawnable aliases for loading after a callback is called that allows the
    //! entity aliases in a spawnable to be adjusted based on runtime requirements
    void ResolveEntityAliases(
        class Spawnable* spawnable,
        const AZStd::string& spawnableName,
        AZStd::chrono::milliseconds streamingDeadline = {},
        AZ::IO::IStreamerTypes::Priority streamingPriority = {},
        const AZ::Data::AssetFilterCB& assetLoadFilterCB = {});
} // AzFramework::SpawnableAssetUtils
