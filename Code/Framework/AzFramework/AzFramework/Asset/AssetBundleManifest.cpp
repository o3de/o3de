/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Asset/AssetBundleManifest.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AzFramework
{
    static bool BundleManifestVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement);

    const int AssetBundleManifest::CurrentBundleVersion = 3;
    const char AssetBundleManifest::s_manifestFileName[] = "manifest.xml";

    AssetBundleManifest::AssetBundleManifest() = default;
    AssetBundleManifest::~AssetBundleManifest() = default;

    void AssetBundleManifest::ReflectSerialize(AZ::SerializeContext* serializeContext)
    {
        if (serializeContext)
        {
            serializeContext->Class<AssetBundleManifest>()
                ->Version(CurrentBundleVersion, &BundleManifestVersionConverter)
                ->Field("BundleVersion", &AssetBundleManifest::m_bundleVersion)
                ->Field("CatalogName", &AssetBundleManifest::m_catalogName)
                ->Field("DependentBundleNames", &AssetBundleManifest::m_depedendentBundleNames)
                ->Field("LevelNames", &AssetBundleManifest::m_levelDirs);

            // Make sure the AZStd::vector<AZStd::string> type is reflected so that it can be read
            // using DataElement::GetChildData
            serializeContext->RegisterGenericType<AZStd::vector<AZStd::string>>();
        }
    }

    bool BundleManifestVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement)
    {
        if (rootElement.GetVersion() < 3)
        {
            static constexpr AZ::u32 levelNamesCrc = AZ_CRC_CE("LevelNames");
            AZStd::vector<AZ::IO::Path> newLevelDirs;
            if (AZStd::vector<AZStd::string> oldLevelNames; rootElement.GetChildData(levelNamesCrc, oldLevelNames))
            {
                newLevelDirs.insert(newLevelDirs.end(),
                    AZStd::make_move_iterator(oldLevelNames.begin()), AZStd::make_move_iterator(oldLevelNames.end()));
            }
            else
            {
                AZ_Error("AssetBundleManifest", false, R"(Unable to read "levelNames" from AssetBundleManifest version %u )",
                    rootElement.GetVersion());
            }

            rootElement.RemoveElementByName(levelNamesCrc);
            rootElement.AddElementWithData(context, "LevelNames", newLevelDirs);
        }
        return true;
    }

    const AZStd::vector<AZ::IO::Path>& AssetBundleManifest::GetLevelDirectories() const
    {
        return m_levelDirs;
    }
    void AssetBundleManifest::SetLevelsDirectory(const AZStd::vector<AZ::IO::Path>& levelDirs)
    {
        m_levelDirs = levelDirs;
    }
} // namespace AzFramework
