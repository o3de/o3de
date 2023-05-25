/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/IO/Path/Path_fwd.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    class SerializeContext;
}

namespace AzFramework
{
    constexpr const char* AssetBundleManifestTypeId = "{8628A669-7B19-4C48-A7CB-F670CC9586FD}";
    // Class to describe metadata about an AssetBundle in Open 3D Engine
    class AssetBundleManifest
    {
    public:
        AZ_TYPE_INFO(AssetBundleManifest, AssetBundleManifestTypeId);
        AZ_CLASS_ALLOCATOR(AssetBundleManifest, AZ::SystemAllocator);

        AssetBundleManifest();
        ~AssetBundleManifest();

        static void ReflectSerialize(AZ::SerializeContext* serializeContext);

        // Each AssetBundle contains a Catalog file with a unique name used to describe the list
        // of files within the AssetBundle in order to update the Asset Registry at runtime when
        // loading the bundle
        const AZStd::string& GetCatalogName() const { return m_catalogName; }
        AZStd::vector<AZStd::string> GetDependentBundleNames() const { return m_dependentBundleNames;  }
        const AZStd::vector<AZ::IO::Path>& GetLevelDirectories() const;
        int GetBundleVersion() const { return m_bundleVersion; }
        void SetCatalogName(const AZStd::string& catalogName) { m_catalogName = catalogName; }
        void SetBundleVersion(int bundleVersion) { m_bundleVersion = bundleVersion; }
        void SetDependentBundleNames(const AZStd::vector<AZStd::string>& dependentBundleNames) { m_dependentBundleNames = dependentBundleNames; }
        void SetLevelsDirectory(const AZStd::vector<AZ::IO::Path>& levelDirs);

        static const char s_manifestFileName[];
        static const int CurrentBundleVersion;

    private:
        AZStd::string m_catalogName;
        AZStd::vector<AZStd::string> m_dependentBundleNames;
        AZStd::vector<AZ::IO::Path> m_levelDirs;
        int m_bundleVersion = CurrentBundleVersion;
    };

} // namespace AzFramework
