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
