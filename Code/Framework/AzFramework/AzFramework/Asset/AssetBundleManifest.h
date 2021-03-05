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

#pragma once

#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    class SerializeContext;
}

namespace AzFramework
{
    // Class to describe metadata about an AssetBundle in Lumberyard
    class AssetBundleManifest
    {
    public:
        AZ_TYPE_INFO(AssetBundleManifest, "{8628A669-7B19-4C48-A7CB-F670CC9586FD}");
        AZ_CLASS_ALLOCATOR(AssetBundleManifest, AZ::SystemAllocator, 0);

        AssetBundleManifest() = default;

        static void ReflectSerialize(AZ::SerializeContext* serializeContext);
        
        // Each AssetBundle contains a Catalog file with a unique name used to describe the list
        // of files within the AssetBundle in order to update the Asset Registry at runtime when
        // loading the bundle
        const AZStd::string& GetCatalogName() const { return m_catalogName; }
        AZStd::vector<AZStd::string> GetDependentBundleNames() const { return m_depedendentBundleNames;  }
        AZStd::vector<AZStd::string> GetLevelDirectories() const { return m_levelDirs; }
        int GetBundleVersion() const { return m_bundleVersion; }
        void SetCatalogName(const AZStd::string& catalogName) { m_catalogName = catalogName; }
        void SetBundleVersion(int bundleVersion) { m_bundleVersion = bundleVersion; }
        void SetDependentBundleNames(const AZStd::vector<AZStd::string>& dependentBundleNames) { m_depedendentBundleNames = dependentBundleNames; }
        void SetLevelsDirectory(const AZStd::vector<AZStd::string>& levelDirs) { m_levelDirs = levelDirs; }

        static const char s_manifestFileName[];
        static const int CurrentBundleVersion;
        
    private: 
        AZStd::string m_catalogName;
        AZStd::vector<AZStd::string> m_depedendentBundleNames;
        AZStd::vector<AZStd::string> m_levelDirs;
        int m_bundleVersion = CurrentBundleVersion;
    };

} // namespace AzFramework
