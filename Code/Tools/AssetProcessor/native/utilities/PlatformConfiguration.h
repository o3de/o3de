/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef PLATFORMCONFIGURATION_H
#define PLATFORMCONFIGURATION_H

#if !defined(Q_MOC_RUN)
#include <QList>
#include <QString>
#include <QObject>
#include <QHash>
#include <QRegExp>
#include <QPair>
#include <QVector>
#include <QSet>

#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryVisitorUtils.h>
#include <AzCore/std/string/string.h>
#include <native/utilities/assetUtils.h>
#include <native/AssetManager/assetScanFolderInfo.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzToolsFramework/Asset/AssetUtils.h>
#endif
#include "IPathConversion.h"


namespace AZ
{
    class SettingsRegistryInterface;
}

namespace AssetProcessor
{
    inline constexpr const char* AssetProcessorSettingsKey{ "/Amazon/AssetProcessor/Settings" };
    inline constexpr const char* AssetProcessorServerKey{ "/O3DE/AssetProcessor/Settings/Server" };
    class PlatformConfiguration;
    class ScanFolderInfo;
    extern const char AssetConfigPlatformDir[];
    extern const char AssetProcessorPlatformConfigFileName[];

    //! Information for a given recognizer, on a specific platform
    //! essentially a plain data holder, but with helper funcs
    enum class AssetInternalSpec
    {
        Copy,
        Skip
    };

    //! The data about a particular recognizer, including all platform specs.
    //! essentially a plain data holder, but with helper funcs
    struct AssetRecognizer
    {
        AZ_CLASS_ALLOCATOR(AssetRecognizer, AZ::SystemAllocator);
        AZ_TYPE_INFO(AssetRecognizer, "{29B7A73A-4D7F-4C19-AEAC-6D6750FB1156}");

        AssetRecognizer() = default;

        AssetRecognizer(
            const AZStd::string& name,
            bool testLockSource,
            int priority,
            bool critical,
            bool supportsCreateJobs,
            AssetBuilderSDK::FilePatternMatcher patternMatcher,
            const AZStd::string& version,
            const AZ::Data::AssetType& productAssetType,
            bool outputProductDependencies,
            bool checkServer = false)
            : m_name(name)
            , m_testLockSource(testLockSource)
            , m_priority(priority)
            , m_isCritical(critical)
            , m_supportsCreateJobs(supportsCreateJobs)
            , m_patternMatcher(patternMatcher)
            , m_version(version)
            , m_productAssetType(productAssetType) // if specified, it allows you to assign a UUID for the type of products directly.
            , m_outputProductDependencies(outputProductDependencies)
            , m_checkServer(checkServer)
        {}

        AZStd::string m_name;
        AssetBuilderSDK::FilePatternMatcher  m_patternMatcher;
        AZStd::string m_version = {};

        // the QString is the Platform Identifier ("pc")
        // the AssetInternalSpec specifies the type of internal job to process
        AZStd::unordered_map<AZStd::string, AssetInternalSpec> m_platformSpecs;

        // an optional parameter which is a UUID of types to assign to the output asset(s)
        // if you don't specify one, then a heuristic will be used
        AZ::Uuid m_productAssetType = AZ::Uuid::CreateNull();

        int m_priority = 0; // used in order to sort these jobs vs other jobs when no other priority is applied (such as platform connected)
        bool m_testLockSource = false;
        bool m_isCritical = false;
        bool m_checkServer = false;
        bool m_supportsCreateJobs = false; // used to indicate a recognizer that can respond to a createJobs request
        bool m_outputProductDependencies = false;
    };
    //! Dictionary of Asset Recognizers based on name
    typedef AZStd::unordered_map<AZStd::string, AssetRecognizer> RecognizerContainer;
    typedef QList<const AssetRecognizer*> RecognizerPointerContainer;

    //! The structure holds information about a particular exclude recognizer
    struct ExcludeAssetRecognizer
    {
        QString                             m_name;
        AssetBuilderSDK::FilePatternMatcher  m_patternMatcher;
    };
    typedef QHash<QString, ExcludeAssetRecognizer> ExcludeRecognizerContainer;

    //! Interface to get constant references to asset and exclude recognizers
    struct RecognizerConfiguration
    {
        AZ_RTTI(RecognizerConfiguration, "{2E4DD73E-8D1E-42BC-A3E3-1A671D636DAC}");

        virtual const RecognizerContainer& GetAssetRecognizerContainer() const = 0;
        virtual const RecognizerContainer& GetAssetCacheRecognizerContainer() const = 0;
        virtual const ExcludeRecognizerContainer& GetExcludeAssetRecognizerContainer() const = 0;
        virtual bool AddAssetCacheRecognizerContainer(const RecognizerContainer& recognizerContainer) = 0;
    };

    /** Reads the platform ini configuration file to determine
    * platforms for which assets needs to be build
    */
    class PlatformConfiguration
        : public QObject
        , public RecognizerConfiguration
        , public AZ::Interface<IPathConversion>::Registrar
    {
        Q_OBJECT
    public:
        AZ_RTTI(PlatformConfiguration, "{9F0C465D-A3A6-417E-B69C-62CBD22FD950}", RecognizerConfiguration, IPathConversion);

        typedef QPair<QRegExp, QString> RCSpec;
        typedef QVector<RCSpec> RCSpecList;

    public:
        explicit PlatformConfiguration(QObject* pParent = nullptr);
        virtual ~PlatformConfiguration() = default;

        static void Reflect(AZ::ReflectContext* context);

        /** Use this function to parse the set of config files and the gem file to set up the platform config.
        * This should be about the only function that is required to be called in order to end up with
        * a full configuration.
        * Note that order of the config files is relevant - later files override settings in
        * files that are earlier.
        **/
        bool InitializeFromConfigFiles(const QString& absoluteSystemRoot, const QString& absoluteAssetRoot, const QString& projectPath, bool addPlatformConfigs = true, bool addGemsConfigs = true);

        //! Merge an AssetProcessor*Config.ini path to the Settings Registry
        //! The settings are anchored underneath the AssetProcessor::AssetProcessorSettingsKey JSON pointer
        static bool MergeConfigFileToSettingsRegistry(AZ::SettingsRegistryInterface& settingsRegistry, const AZ::IO::PathView& filePathView);

        const AZStd::vector<AssetBuilderSDK::PlatformInfo>& GetEnabledPlatforms() const;
        const AssetBuilderSDK::PlatformInfo* const GetPlatformByIdentifier(const char* identifier) const;

        //! Add AssetProcessor config files from platform specific folders
        bool AddPlatformConfigFilePaths(AZStd::vector<AZ::IO::Path>& configList);

        int MetaDataFileTypesCount() const { return m_metaDataFileTypes.count(); }
        // Metadata file types are (meta file extension, original file extension - or blank if its tacked on the end instead of replacing).
        // so for example if its
        // blah.tif + blah.tif.metadata, then its ("metadata", "")
        // but if its blah.tif + blah.metadata (replacing tif, data is lost) then its ("metadata", "tif")
        QPair<QString, QString> GetMetaDataFileTypeAt(int pos) const;

        // Metadata extensions can also be a real file, to create a dependency on file types if a specific file changes
        // so for example, a Metadata file type pair ("Animations/SkeletonList.xml", "i_caf")
        // would cause all i_caf files to be re-evaluated when Animations/SkeletonList.xml is modified.
        bool IsMetaDataTypeRealFile(QString relativeName) const;

        void EnablePlatform(const AssetBuilderSDK::PlatformInfo& platform, bool enable = true);

        //! Gets the minumum jobs specified in the configuration file
        int GetMinJobs() const;
        int GetMaxJobs() const;

        void EnableCommonPlatform();
        void AddIntermediateScanFolder();

        //! Return how many scan folders there are
        int GetScanFolderCount() const;

        //! Return the gems info list
        AZStd::vector<AzFramework::GemInfo> GetGemsInformation() const;

        //! Retrieve the scan folder at a given index.
        AssetProcessor::ScanFolderInfo& GetScanFolderAt(int index);

        //! Retrieve the scan folder at a given index.
        const AssetProcessor::ScanFolderInfo& GetScanFolderAt(int index) const;
        //! Retrieve the scan folder found by a boolean predicate function, when the predicate returns true, the current scan folder info is returned.
        const AssetProcessor::ScanFolderInfo* FindScanFolder(AZStd::function<bool(const AssetProcessor::ScanFolderInfo&)> predicate) const;
        const AssetProcessor::ScanFolderInfo* GetScanFolderById(AZ::s64 id) const override;
        const AZ::s64 GetIntermediateAssetScanFolderId() const;

        //!  Manually add a scan folder.  Also used for testing.
        void AddScanFolder(const AssetProcessor::ScanFolderInfo& source, bool isUnitTesting = false);


        //!  Manually add a recognizer.  Used for testing.
        void AddRecognizer(const AssetRecognizer& source);

        //!  Manually remove a recognizer.  Used for testing.
        void RemoveRecognizer(QString name);

        //!  Manually add an exclude recognizer.  Used for testing.
        void AddExcludeRecognizer(const ExcludeAssetRecognizer& recogniser);

        //!  Manually remove an exclude recognizer.  Used for testing.
        void RemoveExcludeRecognizer(QString name);

        //!  Manually add a metadata type.  Used for testing.
        //!  The originalextension, if specified, means this metafile type REPLACES the given extension
        //!  If not specified (blank) it means that the metafile extension is added onto the end instead and does
        //!  not remove the original file extension
        void AddMetaDataType(const QString& type, const QString& originalExtension);

        // ------------------- utility functions --------------------
        //! Checks to see whether the input file is an excluded file, assumes input is absolute path.
        bool IsFileExcluded(QString fileName) const;
        //! If you already have a relative path, this is a cheaper function to call:
        bool IsFileExcludedRelPath(QString relPath) const;

        //! Given a file name, return a container that contains all matching recognizers
        //!
        //! Returns false if there were no matches, otherwise returns true
        bool GetMatchingRecognizers(QString fileName, RecognizerPointerContainer& output) const;

        //! given a fileName (as a relative and which scan folder it was found in)
        //! Return either an empty string, or the canonical path to a file which overrides it
        //! because of folder priority.
        //! Note that scanFolderName is only used to exit quickly
        //! If its found in any scan folder before it arrives at scanFolderName it will be considered a hit
        QString GetOverridingFile(QString relativeName, QString scanFolderName) const;

        //! given a relative name, loop over folders and resolve it to a full path with the first existing match.
        QString FindFirstMatchingFile(QString relativeName, bool skipIntermediateScanFolder = false, const AssetProcessor::ScanFolderInfo** scanFolderInfo = nullptr) const;

        //! given a relative name with wildcard characters (* allowed) find a set of matching files or optionally folders
        QStringList FindWildcardMatches(const QString& sourceFolder, QString relativeName, bool includeFolders = false,
            bool recursiveSearch = true) const;

        //! given a relative name with wildcard characters (* allowed) find a set of matching files or optionally folders
        QStringList FindWildcardMatches(
            const QString& sourceFolder,
            QString relativeName,
            const AZStd::unordered_set<AZStd::string>& excludedFolders,
            bool includeFolders = false,
            bool recursiveSearch = true) const;

        //! given a fileName (as a full path), return the database source name which includes the output prefix.
        //!
        //! for example
        //! c:/dev/mygame/textures/texture1.tga
        //! ----> [textures/texture1.tga] found under [c:/dev/mygame]
        //! c:/dev/engine/models/box01.mdl
        //! ----> [models/box01.mdl] found under[c:/dev/engine]
        //! note that this does return a database source path by default
        bool ConvertToRelativePath(QString fullFileName, QString& databaseSourceName, QString& scanFolderName) const override;
        static bool ConvertToRelativePath(const QString& fullFileName, const ScanFolderInfo* scanFolderInfo, QString& databaseSourceName);

        //! given a full file name (assumed already fed through the normalization funciton), return the first matching scan folder
        const AssetProcessor::ScanFolderInfo* GetScanFolderForFile(const QString& fullFileName) const override;

        //! Given a scan folder path, get its complete info
        const AssetProcessor::ScanFolderInfo* GetScanFolderByPath(const QString& scanFolderPath) const;

        const RecognizerContainer& GetAssetRecognizerContainer() const override;

        const RecognizerContainer& GetAssetCacheRecognizerContainer() const override;

        const ExcludeRecognizerContainer& GetExcludeAssetRecognizerContainer() const override;

        bool AddAssetCacheRecognizerContainer(const RecognizerContainer& recognizerContainer) override;

        static bool ConvertToJson(const RecognizerContainer& recognizerContainer, AZStd::string& jsonText);
        static bool ConvertFromJson(const AZStd::string& jsonText, RecognizerContainer& recognizerContainer);

        /** returns true if the config is valid.
        * configs are considered invalid if critical information is missing.
        * for example, if no recognizers are given, or no platforms are enabled.
        * They can also be considered invalid if a critical parse error occurred during load.
        */
        bool IsValid() const;

        /** If IsValid is false, this will contain the full error string to show to the user.
        * Note that IsValid will automatically write this error string to stderror as part of checking
        * So this function is there for those wishing to use a GUI.
        */
        const AZStd::string& GetError() const;

        void PopulatePlatformsForScanFolder(AZStd::vector<AssetBuilderSDK::PlatformInfo>& platformsList, QStringList includeTagsList = QStringList(), QStringList excludeTagsList = QStringList());

        // uses const + mutability since its a cache.
        void CacheIntermediateAssetsScanFolderId() const;
        AZStd::optional<AZ::s64> GetIntermediateAssetsScanFolderId() const;
        void ReadMetaDataFromSettingsRegistry();

    protected:

        // call this first, to populate the list of platform informations
        void ReadPlatformInfosFromSettingsRegistry();
        // call this next, in order to find out what platforms are enabled
        void PopulateEnabledPlatforms();
        // finally, call this, in order to delete the platforminfos for non-enabled platforms
        void FinalizeEnabledPlatforms();

        // iterate over all the gems and add their folders to the "scan folders" list as appropriate.
        void AddGemScanFolders(const AZStd::vector<AzFramework::GemInfo>& gemInfoList);

        void ReadEnabledPlatformsFromSettingsRegistry();
        bool ReadRecognizersFromSettingsRegistry(const QString& assetRoot, bool skipScanFolders = false, QStringList scanFolderPatterns = QStringList() );

        int GetProjectScanFolderOrder() const;

    private:
        AZStd::vector<AssetBuilderSDK::PlatformInfo> m_enabledPlatforms;
        RecognizerContainer m_assetRecognizers;
        RecognizerContainer m_assetCacheServerRecognizers;
        ExcludeRecognizerContainer m_excludeAssetRecognizers;
        AZStd::vector<AssetProcessor::ScanFolderInfo> m_scanFolders;
        QList<QPair<QString, QString> > m_metaDataFileTypes;
        QSet<QString> m_metaDataRealFiles;
        AZStd::vector<AzFramework::GemInfo> m_gemInfoList;
        mutable AZ::s64 m_intermediateAssetScanFolderId = -1; // Cached ID for intermediate scanfolder, for quick lookups

        int m_minJobs = 1;
        int m_maxJobs = 3;

        // used only during file read, keeps the total running list of all the enabled platforms from all config files and command lines
        AZStd::vector<AZStd::string> m_tempEnabledPlatforms;

        ///! if non-empty, fatalError contains the error that occurred during read.
        ///! it will be printed out to the log when
        mutable AZStd::string m_fatalError;
    };
} // end namespace AssetProcessor

#endif // PLATFORMCONFIGURATION_H
