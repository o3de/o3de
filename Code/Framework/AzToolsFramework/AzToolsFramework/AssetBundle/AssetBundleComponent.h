/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/IO/FileIOEventBus.h>
#include <AzToolsFramework/AssetBundle/AssetBundleAPI.h>

namespace AZ
{
    class ReflectContext;
}

namespace AzFramework
{
    class AssetBundleManifest;
}

namespace AzToolsFramework
{
    // System component to handle requests to the AssetBundleCommands bus.
    class AssetBundleComponent
        : public AZ::Component
        , private AssetBundleCommands::Bus::Handler
    {
    public:
        AZ_COMPONENT(AssetBundleComponent, "{4DF6D9CF-2393-4C86-8A35-8F85543E8B8A}")

        AssetBundleComponent() = default;
        ~AssetBundleComponent() override = default;

        static const char DeltaCatalogName[];
        //////////////////////////////////////////////////////////////////////////
        // AZ::Component overrides
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////


        //! Returns true if the given list of entries contains an Asset Bundle Manifest
        static bool HasManifest(const AZStd::vector<AZStd::string>& fileEntries);
        
        //! Extracts and deserializes an Asset Bundle manifest from the given bundle
        //! Returns the deserialized Asset Bundle Manifest
        static AzFramework::AssetBundleManifest* GetManifestFromBundle(const AZStd::string& sourcePak);

        //! Inject the file at filePath into the bundle at sourcePak
        //! Returns true if the file at filePath was successfully injected into the bundle at sourcePak
        static bool InjectFile(const AZStd::string& filePath, const AZStd::string& sourcePak);

        //! Inject the file with relative filePath with respect to the working directory into the bundle at sourcePak
        //! Returns true if the file at filePath was successfully injected into the bundle at sourcePak
        static bool InjectFile(const AZStd::string& filePath, const AZStd::string& sourcePak, const char* workingDirectory);

        //! Inject the files with relative filePaths with respect to the working directory into the bundle at sourcePak
        //! Returns true if the file at filePath was successfully injected into the bundle at sourcePak
        static bool InjectFiles(const AZStd::vector<AZStd::string>& fileEntries, const AZStd::string& sourcePak, const char* workingDirectory);

        //! Removes any known non-asset entries from a list of files that exist in a bundle. 
        //! Currently removes entries such as a Delta Asset Catalog if one exists, and the bundle itself.
        //! If a manifest exists within the bundle, we remove the manifest and the catalog it references.
        //! Returns whether all known non-asset entries were removed from the list
        static bool RemoveNonAssetFileEntries(AZStd::vector<AZStd::string>& fileEntries, const AZStd::string& normalizedSourcePakPath, const AzFramework::AssetBundleManifest* manifest);

    private:
        static void Reflect(AZ::ReflectContext* context);

        //////////////////////////////////////////////////////////////////////////
        // AssetBundleCommands::Bus::Handler overrides
        bool CreateDeltaCatalog(const AZStd::string& sourcePak, bool regenerate) override;
        bool CreateAssetBundle(const AssetBundleSettings& assetBundleSettings) override;
        bool CreateAssetBundleFromList(const AssetBundleSettings& assetBundleSettings, const AssetFileInfoList& assetFileInfoList) override;
        ///////////////////////////////////////////////////////////////////////////

        //! Given an asset bundle file path and an index creates a unique bundle filename
        //! this will be use to save dependent bundle files.
        AZStd::string CreateAssetBundleFileName(const AZStd::string& assetBundleFilePath, int bundleIndex);

        //! This will delete both the parent bundle as well as all the dependent bundles mentioned in the manifest file of the parent bundle.
        bool DeleteBundleFiles(const AZStd::string& assetBundleFilePath);

        //! Adds the manifest file to all the bundles
        //! The parent bundle manifest file is special since it will contain information of all dependent bundles names.
        bool AddManifestFileToBundles(const AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>>& bundlePathDeltaCatalogPair, const AZStd::vector<AZStd::string>& dependentBundleNames, const AZStd::string& bundleFolder, const AzToolsFramework::AssetBundleSettings& assetBundleSettings, const AZStd::vector<AZStd::string>& levelDirs);

        //! Adds the delta catalog and any remaining files to the bundle
        //! We only create the delta catalog once we are sure about what all the files that will go in it. 
        bool AddCatalogAndFilesToBundle(const AZStd::vector<AZStd::string>& deltaCatalogEntries, const AZStd::vector<AZStd::string>& fileEntries, const AZStd::string& bundleFilePath, const char* assetAlias, const AzFramework::PlatformId& platformId);
    };

    class ScopedIOEventBusHandler :
        public AZ::IO::FileIOEventBus::Handler
    {
    public:
        ScopedIOEventBusHandler();
        ~ScopedIOEventBusHandler();

        void OnError(const AZ::IO::SystemFile* file, const char* fileName, int errorCode) override;
    };
}
