/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Asset/AssetBundleManifest.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AzFramework
{
    
    const int AssetBundleManifest::CurrentBundleVersion = 2;
    const char AssetBundleManifest::s_manifestFileName[] = "manifest.xml";
    void AssetBundleManifest::ReflectSerialize(AZ::SerializeContext* serializeContext)
    {
        if (serializeContext)
        {
            serializeContext->Class<AssetBundleManifest>()
                ->Version(2)
                ->Field("BundleVersion", &AssetBundleManifest::m_bundleVersion)
                ->Field("CatalogName", &AssetBundleManifest::m_catalogName)
                ->Field("DependentBundleNames", &AssetBundleManifest::m_depedendentBundleNames)
                ->Field("LevelNames", &AssetBundleManifest::m_levelDirs);
        }
    }
} // namespace AzFramework
