
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/vector.h>
#include <AzFramework/Asset/AssetRegistry.h>
#include <AzFramework/Asset/AssetSeedList.h>
#include <AzFramework/Platform/PlatformDefaults.h>

namespace AZ
{
    class ReflectContext;
}

namespace AzToolsFramework
{
    class AssetFileDebugInfo;
    class AssetFileDebugInfoList;

    // AssetFileInfo class will be used to store information
    // related to the asset file including modification time
    // and the the hash of the entire file content.
    class AssetFileInfo
    {
    public:

        constexpr static int s_arraySize = 5;
        AZ_TYPE_INFO(AssetFileInfo, "{F1616D53-9A20-4C04-854E-D458A334B86B}");
        AZ_CLASS_ALLOCATOR(AssetFileInfo, AZ::SystemAllocator);

        AssetFileInfo(AZ::Data::AssetId assetId, AZStd::string assetRelativePath, uint64_t modTime, AZStd::array<AZ::u32, s_arraySize> hash);
        AssetFileInfo() = default;
        static void Reflect(AZ::ReflectContext* context);


        AZ::Data::AssetId m_assetId;
        AZStd::string m_assetRelativePath;
        uint64_t m_modificationTime = 0;
        AZStd::array<AZ::u32, s_arraySize> m_hash;// hash of the file content
    };

    class AssetFileInfoList
    {
    public:

        AZ_TYPE_INFO(AssetFileInfoList, "{61F16042-E381-47E4-8AAA-91BC532F4101}");
        AZ_CLASS_ALLOCATOR(AssetFileInfoList, AZ::SystemAllocator);
        static void Reflect(AZ::ReflectContext* context);

        static bool Save(const AssetFileInfoList& assetFileInfoList, const AZStd::string& destinationFilePath);

        //! Validates that the input path has the proper file extension for an Asset List file.
        //! Input path can be relative or absolute.
        //! Returns void on success, error message on failure.
        static AZ::Outcome<void, AZStd::string> ValidateAssetListFileExtension(const AZStd::string& path);

        AZStd::vector<AssetFileInfo> m_fileInfoList;
    };

    /*
   * Implements an Asset Seed Manager that can be used to handle seed assets.
   * Given a list of seed assets, this class can retrieve a complete
   * list of all the product dependencies.
   */
    class AssetSeedManager
    {
    public:
        AZ_TYPE_INFO(AssetSeedManager, "{0DD7913A-EAD2-43DF-9A30-8A6FA6111E98}");
        AZ_CLASS_ALLOCATOR(AssetSeedManager, AZ::SystemAllocator);

        using AssetsInfoList = AZStd::vector<AZ::Data::AssetInfo>;

        ~AssetSeedManager();

        bool AddSeedAsset(AZ::Data::AssetId assetId, AzFramework::PlatformFlags platformFlags, AZStd::string path = AZStd::string(), const AZStd::string& seedListFilePath = AZStd::string());
        bool AddSeedAsset(const AZStd::string& assetPath, AzFramework::PlatformFlags platformFlags, const AZStd::string& seedListFilePath = AZStd::string());

        //! Attempts to add all platform-specific instances of the given Asset to the Seed List, if they exist.
        //! Returns the Asset Id of the added Asset, and all of the Platforms that were found on-disk and added to the Seed List.
        AZStd::pair<AZ::Data::AssetId, AzFramework::PlatformFlags> AddSeedAssetForValidPlatforms(const AZStd::string& assetPath, AzFramework::PlatformFlags platformFlags);

        void RemoveSeedAsset(AZ::Data::AssetId assetId, AzFramework::PlatformFlags platformFlags);
        //! Removes the seed from the seed list.
        //! AssetKey can either be an assetid, assetpath or the path hint.
        //! If you are providing an assetId than the format has to be "Uuid:SubID", i.e source Uuid and sub ID are separated by a colon
        //! and subId needs to be in the decimal format.
        //! Also important to note is that the asset path is relative to the second project name in the cache
        //! for example if your asset absolute path is <absolute_path_to_cachefolder>/<game_name>/<platform>/<game_name_lowercase>/foo/dummy.txt
        //! than the assetPath should be foo/dummy.txt
        void RemoveSeedAsset(const AZStd::string& assetKey, AzFramework::PlatformFlags platformFlags);

        void AddPlatformToAllSeeds(AzFramework::PlatformId platform);
        void RemovePlatformFromAllSeeds(AzFramework::PlatformId platform);

        //! Save the asset seed list to the destination path
        bool Save(const AZStd::string& destinationFilePath);

        //! Updates the seed path for all seeds.
        //! If a seed is enabled for multiple platforms, it will update the path
        //! with the information provided by the first asset catalog in which that seed exists.
        void UpdateSeedPath();

        //! Removes seed path hint for all existing seeds.
        void RemoveSeedPath();

        //! Loads the asset seed file from the source path and
        //! adds root assets from the file to the seed list
        bool Load(const AZStd::string& sourceFilePath);

        const AzFramework::AssetSeedList& GetAssetSeedList() const;

        AZ::Outcome<void, AZStd::string> SetSeedPlatformFlags(int index, AzFramework::PlatformFlags platformFlags);

        // Using the entries in the seed list retrieves a list of product dependencies and their AssetInfo
        AssetsInfoList GetDependenciesInfo(AzFramework::PlatformId platformIndex, const AZStd::unordered_set<AZ::Data::AssetId>& exclusionList, AssetFileDebugInfoList* optionalDebugList = nullptr, const AZStd::vector<AZStd::string>& wildcardPatternExclusionList = AZStd::vector<AZStd::string>()) const;

        // Creates a AssetFileInfoList comprising of all known product dependencies from the seed list.
        AssetFileInfoList GetDependencyList(AzFramework::PlatformId platformIndex, const AZStd::unordered_set<AZ::Data::AssetId>& exclusionList = {}, AssetFileDebugInfoList* optionalDebugList = nullptr, const AZStd::vector<AZStd::string>& wildcardPatternExclusionList = AZStd::vector<AZStd::string>()) const;

        // Expands the current seed list to gather all dependencies based on the given platform. If given a debugFilePath,
        // also stores information that is useful for understanding what is in the asset list info file and why, but not necessary to generate bundles.
        bool SaveAssetFileInfo(const AZStd::string& destinationFilePath, AzFramework::PlatformFlags platformFlags, const AZStd::unordered_set<AZ::Data::AssetId>& exclusionList = {}, const AZStd::string& debugFilePath = AZStd::string(), const AZStd::vector<AZStd::string>& wildcardPatternExclusionList = AZStd::vector<AZStd::string>());

        AZ::Outcome<AssetFileInfoList, AZStd::string> LoadAssetFileInfo(const AZStd::string& assetListFileAbsolutePath);

        //! Returns the Seed List file extension
        static const char* GetSeedFileExtension();

        //! Validates that the input path has the proper file extension for a Seed List file.
        //! Input path can be relative or absolute.
        //! Returns void on success, error message on failure.
        static AZ::Outcome<void, AZStd::string> ValidateSeedFileExtension(const AZStd::string& path);

        //! Returns the Asset List file extension
        static const char* GetAssetListFileExtension();

        const AZStd::string& GetReadablePlatformList(const AzFramework::SeedInfo& seed);

        static void Reflect(AZ::ReflectContext* context);

        AZ::Data::AssetId FindAssetIdByPathHint(const AZStd::string& pathHint) const;
        AZ::Data::AssetId GetAssetIdByPath(const AZStd::string& assetPath, const AzFramework::PlatformFlags& platformFlags) const;
        AZ::Data::AssetId GetAssetIdByAssetKey(const AZStd::string& assetKey, const AzFramework::PlatformFlags& platformFlags) const;
        static AZ::Data::AssetInfo GetAssetInfoById(const AZ::Data::AssetId& assetId, const AzFramework::PlatformId& platformIndex, const AZStd::string& seedListfilePath = AZStd::string(), const AZStd::string& asetHintPath = AZStd::string());
    private:
        AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> GetAllProductDependencies(
            const AZ::Data::AssetId& assetId,
            const AzFramework::PlatformId& platformIndex,
            const AZStd::unordered_set<AZ::Data::AssetId>& exclusionList,
            AssetFileDebugInfoList* optionalDebugList = nullptr,
            AZStd::unordered_set<AZ::Data::AssetId>* cyclicalDependencySet = nullptr,
            const AZStd::vector<AZStd::string>& wildcardPatternExclusionList = AZStd::vector<AZStd::string>()) const;

        AZStd::string AddAssetToSeedListHelper(const AZStd::string& assetPath);

        void PopulateAssetTypeMap();

        AzFramework::AssetSeedList m_assetSeedList;

        AZStd::unordered_map<AZStd::string, AZStd::string> m_sourceAssetTypeToRuntimeAssetTypeMap;

        AZStd::unordered_map<AzFramework::PlatformFlags, AZStd::string> m_platformFlagsToReadablePlatformList;
    };

   /*
   * Provides a mechanism to interface with the AssetSeedManager
   */
    class AssetSeedManagerRequests : public AZ::EBusTraits
    {
    public:

        using Bus = AZ::EBus<AssetSeedManagerRequests>;

        using AssetTypePairs = AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>>;

        //! Systems may provide source and destination asset mapping by file extensions
        virtual AssetTypePairs GetAssetTypeMapping()
        {
            return {};
        }
    };

} // namespace AzToolsFramework

