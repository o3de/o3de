/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Asset/GenericAssetHandler.h>

#include <AzFramework/Asset/AssetSystemBus.h>

namespace AzFramework
{
    AZ::Data::AssetId GenericAssetHandlerBase::AssetMissingInCatalog(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
    {
        // GenericAssets are generally pure data, and should blockingly load unless you override this function
        // to indicate you support async loading and reloading.

        // If you override this function, consider calling this instead:
        // AssetSystemRequestBus::Broadcast(AssetSystemRequestBus::Events::EscalateAssetByUuid(asset.GetId().m_guid));
        // which will escalate your asset to the top of the list but without blocking the main thread on its loading.
        // and then instead of returning an empty assetId, return a valid AssetId of some fallback asset that you can
        // temporarily use until the real one is ready.  Once that real one is ready, you will recieve an OnAssetReloaded
        // event from the asset system and can do what you need to do in order to swap it out.

        // The default behavior, if you don't override this function, is to block the main thread until the asset has
        // been compiled by the asset system (this will not affect release builds or builds where all the assets are already
        // compiled).

        using AzFramework::AssetSystemRequestBus;
        using AzFramework::AssetSystem::AssetStatus;

        // The following call blocks until the asset is compiled or fails to compile, or is not found.
        AssetStatus status = AssetStatus::AssetStatus_Unknown;
        AssetSystemRequestBus::BroadcastResult(status, &AssetSystemRequestBus::Events::CompileAssetSyncById, asset.GetId());
        if (status == AssetStatus::AssetStatus_Compiled)
        {
            // If the asset was compiled, we can return the asset id to indicate that it is now available.
            return asset.GetId();
        }

        // in all other cases, the asset is either never going to appear, or has failed to compile, etc.  so return an empty assetId.
        // Implementors can also copy the above code and return some sort of "error asset" instead.
        // Just remember to invoke CompileAssetSync on your fallback or error asset as well!

        return AZ::Data::AssetId{};
    }
} // namespace AzFramework

