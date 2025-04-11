/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/PlatformIncl.h>
#include <cstdlib> // for size_t
#include <QString>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzFramework/Logging/LogFile.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include "native/assetprocessor.h"
#include "native/utilities/AssetUtilEBusHelper.h"
#include "native/utilities/ApplicationManagerAPI.h"
#include <AzToolsFramework/Asset/AssetProcessorMessages.h>
#include <AzCore/IO/Path/Path.h>
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>
#include <AssetManager/SourceAssetReference.h>

namespace AzToolsFramework
{
    namespace AssetSystem
    {
        struct JobInfo;
    }
    namespace Logging
    {
        class LogLine;
    }
}

class QStringList;
class QDir;

namespace AssetProcessor
{
    class PlatformConfiguration;
    struct AssetRecognizer;
    class JobEntry;
    class AssetDatabaseConnection;
    struct BuilderParams;
}

namespace AssetUtilities
{
    inline constexpr char ProjectPathOverrideParameter[] = "project-path";

    //! Set precision fingerprint timestamps will be truncated to avoid mismatches across systems/packaging with different file timestamp precisions
    //! Timestamps default to milliseconds.  A value of 1 will keep the default millisecond precision.  A value of 1000 will reduce the precision to seconds
    void SetTruncateFingerprintTimestamp(int precision);

    //! Sets an override for using file hashing.  If override is true, the value of enable will be used instead of the settings file
    void SetUseFileHashOverride(bool override, bool enable);

    //! Compute the root asset folder by scanning for marker files such as root.ini
    //! By Default, queries the EngineRootFolder value from within the SettingsRegistry
    bool ComputeAssetRoot(QDir& root, const QDir* assetRootOverride = nullptr);

    //! Get the engine root folder by looking up the EngineRootFolder key from the Settings Registry
    bool ComputeEngineRoot(QDir& root, const QDir* engineRootOverride = nullptr);

    //! Reset the asset root to not be cached anymore.  Generally only useful for tests
    void ResetAssetRoot();

    //! Reset the game name to not be cached anymore. Generally only useful for tests
    void ResetGameName();

    //! Copy all files from  the source directory to the destination directory, returns true if successfull, else return false
    bool CopyDirectory(QDir source, QDir destination);

    //! makes the file writable
    //! return true if operation is successful, otherwise return false
    bool MakeFileWritable(const QString& filename);

    //! Check to see if we can Lock the file
    bool CheckCanLock(const QString& filename);

    //! Updates the branch token in the bootstrap file
    bool UpdateBranchToken();

    bool ShouldUseFileHashing();

    //! Determine the name of the current project - for example, AutomatedTesting
    //! Can be overridden by passing in a non-empty projectNameOverride
    //! The override will persist if the project name wasn't set previously or
    //! force=true is supplied
    QString ComputeProjectName(QString projectNameOverride = QString(), bool force = false);

    //! Determine the absolute path of the current project
    //! The path computed path will be cached on subsequent calls unless resetCachedProjectPath=true
    QString ComputeProjectPath(bool resetCachedProjectPath = false);

    //! Reads the allowed list directly from the bootstrap file
    QString ReadAllowedlistFromSettingsRegistry(QString initialFolder = QString());

    //! Reads the allowed list directly from the bootstrap file
    QString ReadRemoteIpFromSettingsRegistry(QString initialFolder = QString());

    //! Writes the allowed list directly to the bootstrap file
    bool WriteAllowedlistToSettingsRegistry(const QStringList& allowedList);

    //! Reads the listening port from the bootstrap file
    //! By default the listening port is 45643
    quint16 ReadListeningPortFromSettingsRegistry(QString initialFolder = QString());

    //! Reads platforms from command line
    QStringList ReadPlatformsFromCommandLine();

    //! Copies the sourceFile to the outputFile,returns true if the copy operation succeeds otherwise return false
    //! This function will try deleting the outputFile first,if it exists, before doing the copy operation
    bool CopyFileWithTimeout(QString sourceFile, QString outputFile, unsigned int waitTimeinSeconds = 0);
    //! Moves the sourceFile to the outputFile,returns true if the move operation succeeds otherwise return false
    //! This function will try deleting the outputFile first,if it exists, before doing the move operation
    bool MoveFileWithTimeout(QString sourceFile, QString outputFile, unsigned int waitTimeinSeconds = 0);

    //! Create directory with retries, returns true if the create operation succeeds otherwise return false
    bool CreateDirectoryWithTimeout(QDir dir, unsigned int waitTimeinSeconds = 0);

    //! Normalize and removes any alias from the path
    QString NormalizeAndRemoveAlias(QString path);

    //! Determine the Job Description for a job, for now it is the name of the recognizer
    QString ComputeJobDescription(const AssetProcessor::AssetRecognizer* recognizer);

    //! Compute the root of the cache for the current project.
    //! This is generally the "<Project>/Cache" folder
    bool ComputeProjectCacheRoot(QDir& projectCacheRoot);

    //! Compute the folder that will be used for fence files.
    bool ComputeFenceDirectory(QDir& fenceDir);

    //! Strips the first "asset platform" from the first path segment of a relative product path
    //! This is meant for removing the asset platform for paths such as "pc/MyAssetFolder/MyAsset.asset"
    //! Therefore the result here becomes "MyAssetFolder/MyAsset"
    //!
    //! Similarly invoking this function on relative path that begins with the "server" platform
    //! "server/AssetFolder/Server.asset2" -> "AssetFolder/Server.asset2"
    //! This function does not strip an asset platform from anywhere, but the first path segment
    //! Therefore invoking strip Asset on "MyProject/Cache/pc/MyAsset/MyAsset.asset"
    //! would return a copy of the relative path
    QString StripAssetPlatform(AZStd::string_view relativeProductPath);

    //! Same as StripAssetPlatform, but does not perform any string copies
    //! The return result is only valid for as long as the original input is valid
    AZStd::string_view StripAssetPlatformNoCopy(AZStd::string_view relativeProductPath, AZStd::string_view* outputPlatform = nullptr);

    //! Converts all slashes to forward slashes, removes double slashes,
    //! replaces all indirections such as '.' or '..' as appropriate.
    //! On windows, the drive letter (if present) is converted to uppercase.
    //! Besides that, all case is preserved.
    QString NormalizeFilePath(const QString& filePath);
    void NormalizeFilePaths(QStringList& filePaths);

    //! given a directory name, normalize it the same way as the above file path normalizer
    //! does not convert into absolute path - do that yourself before calling this if you want that
    QString NormalizeDirectoryPath(const QString& directoryPath);

    // UUID generation defaults to lowercase SHA1 of the source name, this does normalization and such
    AZ::Uuid CreateSafeSourceUUIDFromName(const char* sourceName, bool caseInsensitive = true);

    AZ::Outcome<AZ::Uuid, AZStd::string> GetSourceUuid(const AssetProcessor::SourceAssetReference& sourceAsset);
    AZ::Outcome<AZStd::unordered_set<AZ::Uuid>, AZStd::string> GetLegacySourceUuids(const AssetProcessor::SourceAssetReference& sourceAsset);

    //! Compute a CRC given a null-terminated string
    //! @param[in] priorCRC     If supplied, continues an existing CRC by feeding it more data
    unsigned int ComputeCRC32(const char* inString, unsigned int priorCRC = 0xFFFFFFFF);

    //! Compute a CRC given data and a size
    //! @param[in] priorCRC     If supplied, continues an existing CRC by feeding it more data
    unsigned int ComputeCRC32(const char* data, size_t dataSize, unsigned int priorCRC = 0xFFFFFFFF);

    //! Compute a CRC given data and a size
    //! @param[in] priorCRC     If supplied, continues an existing CRC by feeding it more data
    template <typename T>
    unsigned int ComputeCRC32(const T* data, size_t dataSize, unsigned int priorCRC = 0xFFFFFFFF)
    {
        return ComputeCRC32(reinterpret_cast<const char*>(data), dataSize, priorCRC);
    }

    //! Compute a CRC given a null-terminated string
    //! @param[in] priorCRC     If supplied, continues an existing CRC by feeding it more data
    unsigned int ComputeCRC32Lowercase(const char* inString, unsigned int priorCRC = 0xFFFFFFFF);

    //! Compute a CRC given data and a size
    //! @param[in] priorCRC     If supplied, continues an existing CRC by feeding it more data
    unsigned int ComputeCRC32Lowercase(const char* data, size_t dataSize, unsigned int priorCRC = 0xFFFFFFFF);

    //! Compute a CRC given data and a size
    //! @param[in] priorCRC     If supplied, continues an existing CRC by feeding it more data
    template <typename T>
    unsigned int ComputeCRC32Lowercase(const T* data, size_t dataSize, unsigned int priorCRC = 0xFFFFFFFF)
    {
        return ComputeCRC32Lowercase(reinterpret_cast<const char*>(data), dataSize, priorCRC);
    }

    //! attempt to create a workspace for yourself to use as scratch-space, at that starting root folder.
    //! If it succeeds, it will return true and set the result to the final absolute folder name.
    //! this includes creation of temp folder with numbered/lettered temp characters in it.
    //! Note that its up to you to clean this temp workspace up.  It will not automatically be deleted!
    //! If you fail to delete the temp workspace, it will eventually fill the folder up and cause problems.
    bool CreateTempWorkspace(QString startFolder, QString& result);

    //! Create a temp workspace in a default location
    //! If it succeeds, it will return true and set the result to the final absolute folder name.
    //! If it fails, it will return false and result will be an empty string
    //! Note that its up to you to clean this temp workspace up.  It will not automatically be deleted!
    //! If you fail to delete the temp workspace, it will eventually fill the folder up and cause problems.
    bool CreateTempWorkspace(QString& result);

    bool CreateTempRootFolder(QString startFolder, QDir& tempRoot);

    AZStd::string ComputeJobLogFolder();
    AZStd::string ComputeJobLogFileName(const AzToolsFramework::AssetSystem::JobInfo& jobInfo);
    AZStd::string ComputeJobLogFileName(const AssetProcessor::JobEntry& jobEntry);
    AZStd::string ComputeJobLogFileName(const AssetBuilderSDK::CreateJobsRequest& createJobsRequest);

    enum class ReadJobLogResult
    {
        Success,
        MissingFileIO,
        MissingLogFile,
        EmptyLogFile,
    };

    ReadJobLogResult ReadJobLog(AzToolsFramework::AssetSystem::JobInfo& jobInfo, AzToolsFramework::AssetSystem::AssetJobLogResponse& response);
    ReadJobLogResult ReadJobLog(const char* absolutePath, AzToolsFramework::AssetSystem::AssetJobLogResponse& response);

    //! interrogate a given file, which is specified as a full path name, and generate a fingerprint for it.
    unsigned int GenerateFingerprint(const AssetProcessor::JobDetails& jobDetail);

    //! Returns a hash of the contents of the specified file
    // hashMsDelay is only for automated tests to test that writing to a file while it's hashing does not cause a crash.
    // hashMsDelay is not used in non-unit test builds.
    AZ::u64 GetFileHash(const char* filePath, bool force = false, AZ::IO::SizeType* bytesReadOut = nullptr, int hashMsDelay = 0);

    //! Adjusts a timestamp to fix timezone settings and account for any precision adjustment needed
    AZ::u64 AdjustTimestamp(QDateTime timestamp);

    // Generates a fingerprint string based on details of the file, will return the string "0" if the file does not exist.
    // note that the 'name to use' can be blank, but it used to disambiguate between files that have the same
    // modtime and size.
    AZStd::string GetFileFingerprint(const AZStd::string& absolutePath, const AZStd::string& nameToUse);

    QString GuessProductNameInDatabase(QString path, QString platform, AssetProcessor::AssetDatabaseConnection* databaseConnection);

    //! A utility function which checks the given path starting at the root and updates the relative path to be the actual case correct path.
    //! Set checkEntirePath to false if the caller is absolutely sure the path is correct and only the last element (file name or extension)
    //! is potentially wrong. This can happen when for example taking a real file found from a real file directory that is already correct
    //! and modifying just the file path or extension.  It is significantly faster to avoid checking the entire path.
    bool UpdateToCorrectCase(const QString& rootPath, QString& relativePathFromRoot, bool checkEntirePath = true);

    //! Returns true if the path is in the cachePath and *not* in the intermediate assets folder.
    //! If cachePath is empty, it will be computed using ComputeProjectCacheRoot.
    bool IsInCacheFolder(AZ::IO::PathView path, AZ::IO::Path cachePath = "");

    //! Returns true if the path is in the intermediate assets folder.
    //! If cachePath is empty, it will be computed using ComputeProjectCacheRoot.
    bool IsInIntermediateAssetsFolder(AZ::IO::PathView path, AZ::IO::PathView cachePath = "");

    //! Returns the absolute path of the intermediate assets folder
    AZ::IO::FixedMaxPath GetIntermediateAssetsFolder(AZ::IO::PathView cachePath);

    //! Appends the platform prefix for an intermediate asset to get the database name used for products
    AZStd::string GetIntermediateAssetDatabaseName(AZ::IO::PathView relativePath);

    //! Finds the top level source that produced an intermediate product.  If the source is not yet recorded in the database or has no top level source, this will return nothing
    AZStd::optional<AzToolsFramework::AssetDatabase::SourceDatabaseEntry> GetTopLevelSourceForIntermediateAsset(const AssetProcessor::SourceAssetReference& sourceAsset, AZStd::shared_ptr<AssetProcessor::AssetDatabaseConnection> db);

    //! Gets the absolute path to the top level source that produced an intermediate product. Returns nothing if the source is not yet recorded, there is no top level source, or other issues are encountered.
    //! Does not check if the file exists.
    AZStd::optional<AZ::IO::Path> GetTopLevelSourcePathForIntermediateAsset(
        const AssetProcessor::SourceAssetReference& sourceAsset, AZStd::shared_ptr<AssetProcessor::AssetDatabaseConnection> db);

    //! Finds all the sources (up and down) in an intermediate output chain
    AZStd::vector<AssetProcessor::SourceAssetReference> GetAllIntermediateSources(
        const AssetProcessor::SourceAssetReference& sourceAsset, AZStd::shared_ptr<AssetProcessor::AssetDatabaseConnection> db);

    //! Given a source path for an intermediate asset, constructs the product path.
    //! This does not verify either exist, it just manipulates the string.
    AZStd::string GetRelativeProductPathForIntermediateSourcePath(AZStd::string_view relativeSourcePath);

    //! Helper class that provides various paths related to a single output asset.
    //! Files are not guaranteed to exist at the given path.
    struct ProductPath
    {
        ProductPath(AZStd::string scanfolderRelativeProductPath, AZStd::string platformIdentifier);

        static ProductPath FromDatabasePath(AZStd::string_view databasePath, AZStd::string_view* platformOut = nullptr);
        static ProductPath FromAbsoluteProductPath(AZ::IO::PathView absolutePath, AZStd::string& outPlatform);

        //! Absolute path for the product in the intermediate asset folder.  Not guaranteed to exist, this is just the path the file would be at
        AZStd::string GetIntermediatePath() const { return m_intermediatePath.StringAsPosix(); }
        //! Absolute path for the product in the cache folder.  Not guaranteed to exist, this is just the path the file would be at
        AZStd::string GetCachePath() const { return m_cachePath.StringAsPosix(); }
        //! Relative path of the product for the database, this includes the platform prefix and is lowercased
        AZStd::string GetDatabasePath() const { return m_databasePath.StringAsPosix(); }
        //! Scanfolder relative path of the product.  This is lowercased and does not include the platform prefix
        AZStd::string GetRelativePath() const { return m_relativePath; }

    protected:
        AZStd::string m_relativePath;
        AZ::IO::Path m_intermediatePath, m_cachePath, m_databasePath;
    };

    class BuilderFilePatternMatcher
        : public AssetBuilderSDK::FilePatternMatcher
    {
    public:
        AZ_CLASS_ALLOCATOR(BuilderFilePatternMatcher, AZ::SystemAllocator)
        BuilderFilePatternMatcher() = default;
        BuilderFilePatternMatcher(const BuilderFilePatternMatcher& copy);
        BuilderFilePatternMatcher(const AssetBuilderSDK::AssetBuilderPattern& pattern, const AZ::Uuid& builderDescID);

        const AZ::Uuid& GetBuilderDescID() const;
    protected:
        AZ::Uuid    m_builderDescID;
    };

    //! QuitListener is an utility class that can be used to listen for application quit notification
    class QuitListener
        : public AssetProcessor::ApplicationManagerNotifications::Bus::Handler
    {
    public:

        QuitListener();
        ~QuitListener();
        /// ApplicationManagerNotifications::Bus::Handler
        void ApplicationShutdownRequested() override;

        bool WasQuitRequested() const;

    private:
        AZStd::atomic<bool> m_requestedQuit;
    };

    //! JobLogTraceListener listens for job messages
    class JobLogTraceListener
        : public AZ::Debug::TraceMessageBus::Handler
    {
    public:

        JobLogTraceListener(const AZStd::string& logFileName, AZ::s64 jobKey, bool overwriteLogFile = false);

        JobLogTraceListener(const AzToolsFramework::AssetSystem::JobInfo& jobInfo, bool overwriteLogFile = false);

        JobLogTraceListener(const AssetProcessor::JobEntry& jobEntry, bool overwriteLogFile = false);

        ~JobLogTraceListener();

        //////////////////////////////////////////////////////////////////////////
        // AZ::Debug::TraceMessagesBus - we actually ignore all outputs except those for our ID.

        bool OnAssert(const char* message) override;
        bool OnException(const char* message) override;
        bool OnPreError(const char* window, const char* file, int line, const char* func, const char* message) override;
        bool OnWarning(const char* window, const char* message) override;
        //////////////////////////////////////////////////////////////////////////

        bool OnPrintf(const char* window, const char* message) override;
        //////////////////////////////////////////////////////////////////////////

        void AppendLog(AzToolsFramework::Logging::LogLine& logLine);

        AZ::s64 GetErrorCount() const;
        AZ::s64 GetWarningCount() const;

        void AddError();
        void AddWarning();

    private:
        AZStd::unique_ptr<AzFramework::LogFile> m_logFile;
        AZStd::string m_logFileName;
        AZ::s64 m_runKey = 0;
        // using m_isLogging bool to prevent an infinite loop which can happen if an error/warning happens when trying to create an invalid logFile,
        // because it will cause the appendLog function to be called again, which will again try to create that log file.
        bool m_isLogging = false;
        bool m_inException = false;
        //! If true, log file will be overwritten instead of appended
        bool m_forceOverwriteLog = false;
        AZ::s64 m_errorCount = 0;
        AZ::s64 m_warningCount = 0;

        void AppendLog(AzFramework::LogFile::SeverityLevel severity, const char* window, const char* message);
    };
} // namespace AssetUtilities
