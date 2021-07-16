/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(AZCORE_EXCLUDE_LUA)

#include <AzCore/Script/ScriptAsset.h>
#include <AzCore/Math/Crc.h>

namespace AZ
{
    //=========================================================================
    // ScriptAsset
    //=========================================================================
    ScriptAsset::ScriptAsset(const Data::AssetId& assetId)
        : Data::AssetData(assetId)
    {
    }
}   // namespace AZ

#endif // #if !defined(AZCORE_EXCLUDE_LUA)
