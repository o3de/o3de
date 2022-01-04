/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "assetUtils.h"

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Math/Sha1.h>

#include <native/utilities/PlatformConfiguration.h>
#include <native/utilities/StatsCapture.h>
#include <native/AssetManager/FileStateCache.h>
#include <native/AssetDatabase/AssetDatabase.h>
#include <utilities/ThreadHelper.h>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QTemporaryDir>
#include <QTextStream>
#include <QTimeZone>
#include <QRandomGenerator>

#include <AzQtComponents/Utilities/RandomNumberGenerator.h>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#include <libgen.h>
#include <unistd.h>
#elif defined(AZ_PLATFORM_LINUX)
#include <libgen.h>
#endif

#include <AzCore/JSON/document.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Platform/PlatformDefaults.h>
#include <AzToolsFramework/UI/Logging/LogLine.h>
#include <xxhash/xxhash.h>

#if defined(AZ_PLATFORM_WINDOWS)
#   include <windows.h>
#else
#   include <QFile>
#endif

#if defined(AZ_PLATFORM_APPLE) || defined(AZ_PLATFORM_LINUX)
#include <fcntl.h>
#endif

#if defined(AZ_PLATFORM_LINUX)
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif // defined(AZ_PLATFORM_LINUX)

#include <sstream>

namespace AssetUtilsInternal
{
    static const unsigned int g_RetryWaitInterval = 250; // The amount of time that we are waiting for retry.
    // This is because Qt has to init random number gen on each thread.
    AZ_THREAD_LOCAL bool g_hasInitializedRandomNumberGenerator = false;

    // so that even if we do init two seeds at exactly the same msec time, theres still this extra
    // changing number
    static AZStd::atomic_int g_randomNumberSequentialSeed;

    bool FileCopyMoveWithTimeout(QString sourceFile, QString outputFile, bool isCopy, unsigned int waitTimeInSeconds)
    {
        bool failureOccurredOnce = false; // used for logging.
        bool operationSucceeded = false;
        QFile outFile(outputFile);
        QElapsedTimer timer;
        timer.start();
        do
        {
            QString normalized = AssetUtilities::NormalizeFilePath(outputFile);
            AssetProcessor::ProcessingJobInfoBus::Broadcast(
                &AssetProcessor::ProcessingJobInfoBus::Events::BeginCacheFileUpdate, normalized.toUtf8().constData());

            //Removing the old file if it exists
            if (outFile.exists())
            {
                if (!outFile.remove())
                {
                    if (!failureOccurredOnce)
                    {
                        AZ_Warning(AssetProcessor::ConsoleChannel, false, "Warning: Unable to remove file %s to copy source file %s in... (We may retry)\n", outputFile.toUtf8().constData(), sourceFile.toUtf8().constData());
                        failureOccurredOnce = true;
                    }
                    //not able to remove the file
                    if (waitTimeInSeconds != 0)
                    {
                        //Sleep only for non zero waitTime
                        QThread::msleep(AssetUtilsInternal::g_RetryWaitInterval);
                    }
                    continue;
                }
            }

            //ensure that the output dir is present
            QFileInfo outFileInfo(outputFile);
            if (!outFileInfo.absoluteDir().mkpath("."))
            {
                AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Failed to create directory (%s).\n", outFileInfo.absolutePath().toUtf8().data());
                return false;
            }

            if (isCopy && QFile::copy(sourceFile, outputFile))
            {
                //Success
                operationSucceeded = true;
                break;
            }
            else if (!isCopy && QFile::rename(sourceFile, outputFile))
            {
                //Success
                operationSucceeded = true;
                break;
            }
            else
            {
                failureOccurredOnce = true;
                if (waitTimeInSeconds != 0)
                {
                    //Sleep only for non zero waitTime
                    QThread::msleep(AssetUtilsInternal::g_RetryWaitInterval);
                }
            }
        } while (!timer.hasExpired(waitTimeInSeconds * 1000)); //We will keep retrying until the timer has expired the inputted timeout
        
        // once we're done, regardless of success or failure, we 'unlock' those files for further process.
        // if we failed, also re-trigger them to rebuild (the bool param at the end of the ebus call)
        QString normalized = AssetUtilities::NormalizeFilePath(outputFile);
        AssetProcessor::ProcessingJobInfoBus::Broadcast(
            &AssetProcessor::ProcessingJobInfoBus::Events::EndCacheFileUpdate, normalized.toUtf8().constData(), !operationSucceeded);

        if (!operationSucceeded)
        {
            //operation failed for the given timeout
            AZ_Warning(AssetProcessor::ConsoleChannel, false, "WARNING: Could not %s source from %s to %s, giving up\n",
                isCopy ? "copy" : "move (via rename)",
                sourceFile.toUtf8().constData(), outputFile.toUtf8().constData());
            return false;
        }
        else if (failureOccurredOnce)
        {
            // if we failed once, write to log to indicate that we eventually succeeded.
            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "SUCCESS:  after failure, we later succeeded to copy/move file %s\n", outputFile.toUtf8().constData());
        }

        return true;
    }

    static bool DumpAssetProcessorUserSettingsToFile(AZ::SettingsRegistryInterface& settingsRegistry,
        const AZ::IO::FixedMaxPath& setregPath)
    {
        // The AssetProcessor settings are currently under the Bootstrap object(This may change in the future
        constexpr AZStd::string_view AssetProcessorUserSettingsRootKey = AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey;
        AZStd::string apSettingsJson;
        AZ::IO::ByteContainerStream apSettingsStream(&apSettingsJson);

        AZ::SettingsRegistryMergeUtils::DumperSettings apDumperSettings;
        apDumperSettings.m_prettifyOutput = true;
        AZ_PUSH_DISABLE_WARNING(5233, "-Wunknown-warning-option") // Older versions of MSVC toolchain require to pass constexpr in the
                                                                  // capture. Newer versions issue unused warning
        apDumperSettings.m_includeFilter = [&AssetProcessorUserSettingsRootKey](AZStd::string_view path)
        AZ_POP_DISABLE_WARNING
        {
            // The AssetUtils only updates the following keys in the registry
            // Dump them all out to the setreg file
            auto allowedListKey = AZ::SettingsRegistryInterface::FixedValueString(AssetProcessorUserSettingsRootKey)
                + "/allowed_list";
            auto branchTokenKey = AZ::SettingsRegistryInterface::FixedValueString(AssetProcessorUserSettingsRootKey)
                + "/assetProcessor_branch_token";
            // The objects leading up to the keys to dump must be included in order the keys to be dumped
            return allowedListKey.starts_with(path.substr(0, allowedListKey.size()))
                || branchTokenKey.starts_with(path.substr(0, branchTokenKey.size()));
        };
        apDumperSettings.m_jsonPointerPrefix = AssetProcessorUserSettingsRootKey;

        if (AZ::SettingsRegistryMergeUtils::DumpSettingsRegistryToStream(settingsRegistry, AssetProcessorUserSettingsRootKey,
            apSettingsStream, apDumperSettings))
        {
            constexpr const char* AssetProcessorTmpSetreg = "asset_processor.setreg.tmp";
            // Write to a temporary file first before renaming it to the final file location
            // This is needed to reduce the potential of a race condition which occurs when other applications attempt to load settings registry
            // files from the project's user Registry folder while the AssetProcessor is writing the file out the asset_processor.setreg
            // at the same time
            QString tempDirValue;
            AssetUtilities::CreateTempWorkspace(tempDirValue);
            QDir tempDir(tempDirValue);
            AZ::IO::FixedMaxPath tmpSetregPath = tempDir.absoluteFilePath(QString(AssetProcessorTmpSetreg)).toUtf8().data();

            constexpr auto modeFlags = AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY | AZ::IO::SystemFile::SF_OPEN_CREATE
                | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH;
            if (AZ::IO::SystemFile apSetregFile; apSetregFile.Open(tmpSetregPath.c_str(), modeFlags))
            {
                size_t bytesWritten = apSetregFile.Write(apSettingsJson.data(), apSettingsJson.size());
                // Close the file so that it can be renamed.
                apSetregFile.Close();
                if (bytesWritten == apSettingsJson.size())
                {
                    // Create the directory to contain the moved setreg file
                    AZ::IO::SystemFile::CreateDir(AZ::IO::FixedMaxPath(setregPath.ParentPath()).c_str());
                    return AZ::IO::SystemFile::Rename(tmpSetregPath.c_str(), setregPath.c_str(), true);
                }
            }
            else
            {
                AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Unable to open AssetProcessor user setreg file (%s)\n", setregPath.c_str());
            }
        }
        else
        {
            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Dump of AssetProcessor User Settings failed at JSON pointer %.*s \n",
                aznumeric_cast<int>(AssetProcessorUserSettingsRootKey.size()), AssetProcessorUserSettingsRootKey.data());
        }

        return false;
    }
}

namespace AssetUtilities
{
    constexpr AZStd::string_view AssetProcessorUserSetregRelPath = "user/Registry/asset_processor.setreg";

    // do not place Qt objects in global scope, they allocate and refcount threaded data.
    AZ::SettingsRegistryInterface::FixedValueString s_projectPath;
    AZ::SettingsRegistryInterface::FixedValueString s_projectName;
    AZ::SettingsRegistryInterface::FixedValueString s_assetRoot;
    AZ::SettingsRegistryInterface::FixedValueString s_assetServerAddress;
    AZ::SettingsRegistryInterface::FixedValueString s_cachedEngineRoot;
    int s_truncateFingerprintTimestampPrecision{ 1 };
    AZStd::optional<bool> s_fileHashOverride{};
    AZStd::optional<bool> s_fileHashSetting{};

    void SetTruncateFingerprintTimestamp(int precision)
    {
        s_truncateFingerprintTimestampPrecision = precision;
    }

    void SetUseFileHashOverride(bool override, bool enable)
    {
        if(override)
        {
            s_fileHashOverride = enable ? 1 : 0;
        }
        else
        {
            s_fileHashOverride.reset();
        }
    }


    void ResetAssetRoot()
    {
        s_assetRoot = {};
        s_cachedEngineRoot = {};
    }

    void ResetGameName()
    {
        s_projectName = {};
    }

    bool CopyDirectory(QDir source, QDir destination)
    {
        if (!destination.exists())
        {
            bool result = destination.mkpath(".");
            if (!result)
            {
                AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Failed to create directory (%s).\n", destination.absolutePath().toUtf8().data());
                return false;
            }
        }

        QFileInfoList entries = source.entryInfoList(QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs);

        for (const QFileInfo& entry : entries)
        {
            if (entry.isDir())
            {
                //if the entry is a directory than recursively call the function
                if (!CopyDirectory(QDir(source.absolutePath() + "/" + entry.completeBaseName()), QDir(destination.absolutePath() + "/" + entry.completeBaseName())))
                {
                    return false;
                }
            }
            else
            {
                //if the entry is a file than copy it but before than ensure that the destination file is not present
                QString destinationFile = destination.absolutePath() + "/" + entry.fileName();

                if (QFile::exists(destinationFile))
                {
                    if (!QFile::remove(destinationFile))
                    {
                        AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Unable to remove file (%s).\n", destinationFile.toUtf8().data());
                        return false;
                    }
                }

                QString sourceFile = source.absolutePath() + "/" + entry.fileName();

                if (!QFile::copy(sourceFile, destinationFile))
                {
                    AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Unable to copy sourcefile (%s) to destination (%s).\n", sourceFile.toUtf8().data(), destinationFile.toUtf8().data());
                    return false;
                }
            }
        }

        return true;
    }

    bool ComputeAssetRoot(QDir& root, const QDir* rootOverride)
    {
        if (!s_assetRoot.empty())
        {
            root = QDir(QString::fromUtf8(s_assetRoot.c_str(), aznumeric_cast<int>(s_assetRoot.size())));
            return true;
        }

        // Use the override if supplied and not an empty string
        if (rootOverride && !rootOverride->path().isEmpty())
        {
            root = *rootOverride;
            s_assetRoot = root.absolutePath().toUtf8().constData();
            return true;
        }

        const AzFramework::CommandLine* commandLine = nullptr;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(commandLine, &AzFramework::ApplicationRequests::GetCommandLine);

        static const char AssetRootParam[] = "assetroot";
        if (commandLine && commandLine->HasSwitch(AssetRootParam))
        {
            s_assetRoot = commandLine->GetSwitchValue(AssetRootParam, 0).c_str();
            root = QDir(s_assetRoot.c_str());
            return true;
        }

        AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get();
        if (settingsRegistry == nullptr)
        {
            AZ_Warning("AssetProcessor", false, "Unable to retrieve Global SettingsRegistry at this time."
                " Has a ComponentApplication(or a class derived from ComponentApplication) been constructed yet?");
            return false;
        }

        AZ::IO::FixedMaxPathString engineRootFolder;
        if (settingsRegistry->Get(engineRootFolder, AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder))
        {
            root = QDir(QString::fromUtf8(engineRootFolder.c_str(), aznumeric_cast<int>(engineRootFolder.size())));
            s_assetRoot = root.absolutePath().toUtf8().constData();
            return true;
        }

        // The EngineRootFolder Key has not been found in the SettingsRegistry
        auto engineRootError = AZ::SettingsRegistryInterface::FixedValueString::format("The EngineRootFolder is not set in the SettingsRegistry at key %s.",
            AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        AssetProcessor::MessageInfoBus::Broadcast(&AssetProcessor::MessageInfoBusTraits::OnErrorMessage, engineRootError.c_str());

        return false;
    }

    //! Get the external engine root folder if the engine is external to the current root folder.
    //! If the current root folder is also the engine folder, then this behaves the same as ComputeEngineRoot
    bool ComputeEngineRoot(QDir& root, const QDir* engineRootOverride)
    {
        if (!s_cachedEngineRoot.empty())
        {
            root = QDir(QString::fromUtf8(s_cachedEngineRoot.c_str(), aznumeric_cast<int>(s_cachedEngineRoot.size())));
            return true;
        }

        // Compute the AssetRoot if it is empty as well
        if (s_assetRoot.empty())
        {
            AssetUtilities::ComputeAssetRoot(root, engineRootOverride);
        }

        AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get();
        // Use the engineRootOverride if supplied and not empty
        if (engineRootOverride && !engineRootOverride->path().isEmpty())
        {
            root = *engineRootOverride;
            s_cachedEngineRoot = root.absolutePath().toUtf8().constData();
            return true;
        }

        if (settingsRegistry == nullptr)
        {
            return false;
        }

        AZ::IO::FixedMaxPathString engineRootFolder;
        if (settingsRegistry->Get(engineRootFolder, AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder))
        {
            root = QDir(QString::fromUtf8(engineRootFolder.c_str(), aznumeric_cast<int>(engineRootFolder.size())));
            s_cachedEngineRoot = root.absolutePath().toUtf8().constData();
            return true;
        }

        return false;
    }

    bool MakeFileWritable(const QString& fileName)
    {
#if defined WIN32
        DWORD fileAttributes = GetFileAttributesA(fileName.toUtf8());
        if (fileAttributes == INVALID_FILE_ATTRIBUTES)
        {
            //file does not exist
            return false;
        }
        if ((fileAttributes & FILE_ATTRIBUTE_READONLY))
        {
            fileAttributes = fileAttributes & ~(FILE_ATTRIBUTE_READONLY);
            if (SetFileAttributesA(fileName.toUtf8(), fileAttributes))
            {
                return true;
            }

            return false;
        }
        else
        {
            //file is writeable
            return true;
        }
#else
        QFileInfo fileInfo(fileName);

        if (!fileInfo.exists())
        {
            return false;
        }
        if (fileInfo.permission(QFile::WriteUser))
        {
            // file already has the write permission
            return true;
        }
        else
        {
            QFile::Permissions filePermissions = fileInfo.permissions();
            if (QFile::setPermissions(fileName, filePermissions | QFile::WriteUser))
            {
                //write permission added
                return true;
            }

            return false;
        }
#endif
    }

    bool CheckCanLock(const QString& fileName)
    {
#if defined(AZ_PLATFORM_WINDOWS)
        AZStd::wstring usableFileName;
        usableFileName.resize(fileName.length(), 0);
        fileName.toWCharArray(usableFileName.data());

        // third parameter dwShareMode (0) prevents share access
        HANDLE fileHandle = CreateFileW(usableFileName.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, 0);

        if (fileHandle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(fileHandle);
            return true;
        }

        return false;
#else
        int open_flags = O_RDONLY | O_NONBLOCK;
#if defined(PLATFORM_APPLE)
        // O_EXLOCK only supported on APPLE
        open_flags |= O_EXLOCK
#endif
        int handle = open(fileName.toUtf8().constData(), open_flags);
        if (handle != -1)
        {
            close(handle);
            return true;
        }
        return false;
#endif
    }

    QString ComputeProjectName(QString gameNameOverride, bool force)
    {
        if (force || s_projectName.empty())
        {
            // Override Game Name if a non-empty override string has been supplied
            if (!gameNameOverride.isEmpty())
            {
                s_projectName = gameNameOverride.toUtf8().constData();
            }
            else
            {
                s_projectName = AZ::Utils::GetProjectName();
            }
        }

        return QString::fromUtf8(s_projectName.c_str(), aznumeric_cast<int>(s_projectName.size()));
    }

    QString ComputeProjectPath(bool resetCachedProjectPath/*=false*/)
    {
        if (resetCachedProjectPath)
        {
            // Clear any cached value if reset was requested
            s_projectPath.clear();
        }
        if (s_projectPath.empty())
        {
            // Check command-line args first
            QStringList args = QCoreApplication::arguments();
            for (QString arg : args)
            {
                if (arg.contains(QString("/%1=").arg(ProjectPathOverrideParameter), Qt::CaseInsensitive)
                    || arg.contains(QString("--%1=").arg(ProjectPathOverrideParameter), Qt::CaseInsensitive))
                {
                    QString rawValueString = arg.split("=")[1].trimmed();
                    if (!rawValueString.isEmpty())
                    {
                        QDir path(rawValueString);
                        if (path.isAbsolute())
                        {
                            s_projectPath = rawValueString.toUtf8().constData();
                            break;
                        }
                    }
                }
            }
        }

        if (s_projectPath.empty())
        {
            s_projectPath = AZ::Utils::GetProjectPath();
        }

        return QString::fromUtf8(s_projectPath.c_str(), aznumeric_cast<int>(s_projectPath.size()));
    }

    bool InServerMode()
    {
        static bool s_serverMode = CheckServerMode();
        return s_serverMode;
    }

    bool CheckServerMode()
    {
        QStringList args = QCoreApplication::arguments();
        for (const QString& arg : args)
        {
            if (arg.contains("/server", Qt::CaseInsensitive) || arg.contains("--server", Qt::CaseInsensitive))
            {
                bool isValid = false;
                AssetProcessor::AssetServerBus::BroadcastResult(isValid, &AssetProcessor::AssetServerBusTraits::IsServerAddressValid);
                if (isValid)
                {
                    AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Asset Processor is running in server mode.\n");
                    return true;
                }
                else
                {
                    AZ_Warning(AssetProcessor::ConsoleChannel, false, "Invalid server address, please check the AssetProcessorPlatformConfig.setreg file"
                        " to ensure that the address is correct. Asset Processor won't be running in server mode.");
                }

                break;
            }
        }

        return false;
    }


    QString ServerAddress()
    {
        if (!s_assetServerAddress.empty())
        {
            return QString::fromUtf8(s_assetServerAddress.data(), aznumeric_cast<int>(s_assetServerAddress.size()));
        }
        // QCoreApplication is not created during unit test mode and that can cause QtWarning to get emitted
        // since we need to retrieve arguments from Qt
        if (QCoreApplication::instance())
        {
            // if its been specified on the command line, then ignore AssetProcessorPlatformConfig:
            QStringList args = QCoreApplication::arguments();
            for (QString arg : args)
            {
                if (arg.contains("/serverAddress=", Qt::CaseInsensitive) || arg.contains("--serverAddress=", Qt::CaseInsensitive))
                {
                    QString serverAddress = arg.split("=")[1].trimmed();
                    if (!serverAddress.isEmpty())
                    {
                        s_assetServerAddress = serverAddress.toUtf8().constData();
                        return QString::fromUtf8(s_assetServerAddress.data(), aznumeric_cast<int>(s_assetServerAddress.size()));
                    }
                }
            }
        }

        auto settingsRegistry = AZ::SettingsRegistry::Get();
        if (settingsRegistry)
        {
            AZStd::string address;
            if (settingsRegistry->Get(address, AZ::SettingsRegistryInterface::FixedValueString(AssetProcessor::AssetProcessorSettingsKey)
                + "/Server/cacheServerAddress"))
            {
                AZ_TracePrintf(AssetProcessor::DebugChannel, "Server Address: %s\n", address.c_str());
            }
            s_assetServerAddress = address;

            return QString::fromUtf8(address.data(), aznumeric_cast<int>(address.size()));
        }

        return QString();
    }

    bool ShouldUseFileHashing()
    {
        // Check if the settings file is overridden, if so, use the override instead
        if(s_fileHashOverride)
        {
            return *s_fileHashOverride;
        }

        // Check if we read the settings file already, if so, use the cached value
        if(s_fileHashSetting)
        {
            return *s_fileHashSetting;
        }

        auto settingsRegistry = AZ::SettingsRegistry::Get();
        if (settingsRegistry)
        {
            bool curValue = true;
            settingsRegistry->Get(curValue, AZ::SettingsRegistryInterface::FixedValueString(AssetProcessor::AssetProcessorSettingsKey)
                + "/Fingerprinting/UseFileHashing");
            AZ_TracePrintf(AssetProcessor::DebugChannel, "UseFileHashing: %s\n", curValue ? "True" : "False");
            s_fileHashSetting = curValue;

            return curValue;
        }

        AZ_TracePrintf(AssetProcessor::DebugChannel, "No UseFileHashing setting found\n");
        s_fileHashSetting = true;

        return *s_fileHashSetting;
    }

    QString ReadAllowedlistFromSettingsRegistry([[maybe_unused]] QString initialFolder)
    {
        constexpr size_t BufferSize = AZ_ARRAY_SIZE(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey) + AZStd::char_traits<char>::length("/allowed_list");
        AZStd::fixed_string<BufferSize> allowedListKey{ AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey };
        allowedListKey += "/allowed_list";

        AZ::SettingsRegistryInterface::FixedValueString allowedListIp;
        if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry && settingsRegistry->Get(allowedListIp, allowedListKey))
        {
            return QString::fromUtf8(allowedListIp.c_str(), aznumeric_cast<int>(allowedListIp.size()));
        }

        return {};
    }

    QString ReadRemoteIpFromSettingsRegistry([[maybe_unused]] QString initialFolder)
    {
        constexpr size_t BufferSize = AZ_ARRAY_SIZE(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey) + AZStd::char_traits<char>::length("/remote_ip");
        AZStd::fixed_string<BufferSize> remoteIpKey{ AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey };
        remoteIpKey += "/remote_ip";

        AZ::SettingsRegistryInterface::FixedValueString remoteIp;
        if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry && settingsRegistry->Get(remoteIp, remoteIpKey))
        {
            return QString::fromUtf8(remoteIp.c_str(), aznumeric_cast<int>(remoteIp.size()));
        }

        return {};
    }

    bool WriteAllowedlistToSettingsRegistry(const QStringList& newAllowedList)
    {
        AZ::IO::FixedMaxPath assetProcessorUserSetregPath = AZ::Utils::GetProjectPath();
        assetProcessorUserSetregPath /= AssetProcessorUserSetregRelPath;

        auto settingsRegistry = AZ::SettingsRegistry::Get();
        if (!settingsRegistry)
        {
            AZ_Error(AssetProcessor::ConsoleChannel, false, "Unable access Settings Registry. Branch Token cannot be updated");
            return false;
        }

        auto allowedListKey = AZ::SettingsRegistryInterface::FixedValueString(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey)
            + "/allowed_list";
        AZStd::string currentAllowedList;
        if (settingsRegistry->Get(currentAllowedList, allowedListKey))
        {
            // Split the current allowedList into an array and compare against the new allowed list
            AZStd::vector<AZStd::string_view> allowedListArray;
            auto AppendAllowedIpTokens = [&allowedListArray](AZStd::string_view token) { allowedListArray.emplace_back(token); };
            AZ::StringFunc::TokenizeVisitor(currentAllowedList, AppendAllowedIpTokens, ',');

            auto CompareQListToAzVector = [](AZStd::string_view currentAllowedIp, const QString& newAllowedIp)
            {
                return currentAllowedIp == newAllowedIp.toUtf8().constData();
            };
            if (AZStd::equal(allowedListArray.begin(), allowedListArray.end(), newAllowedList.begin(), newAllowedList.end(), CompareQListToAzVector))
            {
                // no need to update, remote_ip already matches
                return true;
            }
        }

        // Update Settings Registry with new token
        AZStd::string azNewAllowedList{ newAllowedList.join(',').toUtf8().constData() };
        settingsRegistry->Set(allowedListKey, azNewAllowedList);

        return AssetUtilsInternal::DumpAssetProcessorUserSettingsToFile(*settingsRegistry, assetProcessorUserSetregPath);
    }

    quint16 ReadListeningPortFromSettingsRegistry(QString initialFolder /*= QString()*/)
    {
        if (initialFolder.isEmpty())
        {
            QDir engineRoot;
            if (!AssetUtilities::ComputeEngineRoot(engineRoot))
            {
                //return the default port
                return 45643;
            }
            initialFolder = engineRoot.absolutePath();
        }

        constexpr size_t BufferSize = AZ_ARRAY_SIZE(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey) + AZStd::char_traits<char>::length("/remote_port");
        AZStd::fixed_string<BufferSize> remotePortKey{ AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey };
        remotePortKey += "/remote_port";

        AZ::s64 portNumber;
        if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry && settingsRegistry->Get(portNumber, remotePortKey))
        {
            return aznumeric_cast<quint16>(portNumber);
        }

        //return the default port
        return 45643;
    }

    QStringList ReadPlatformsFromCommandLine()
    {
        QStringList args = QCoreApplication::arguments();
        for (QString arg : args)
        {
            if (arg.contains("--platforms=", Qt::CaseInsensitive) || arg.contains("/platforms=", Qt::CaseInsensitive))
            {
                QString rawPlatformString = arg.split("=")[1];
                return rawPlatformString.split(",");
            }
        }

        return QStringList();
    }

    bool CopyFileWithTimeout(QString sourceFile, QString outputFile, unsigned int waitTimeInSeconds)
    {
        return AssetUtilsInternal::FileCopyMoveWithTimeout(sourceFile, outputFile, true, waitTimeInSeconds);
    }

    bool MoveFileWithTimeout(QString sourceFile, QString outputFile, unsigned int waitTimeInSeconds)
    {
        return AssetUtilsInternal::FileCopyMoveWithTimeout(sourceFile, outputFile, false, waitTimeInSeconds);
    }

    bool CreateDirectoryWithTimeout(QDir dir, unsigned int waitTimeinSeconds)
    {
        if (dir.exists())
        {
            return true;
        }

        int retries = 0;
        QElapsedTimer timer;
        timer.start();
        do
        {
            retries++;
            // Try to create the directory path
            if (dir.mkpath("."))
            {
                return true;
            }
            else
            {
                AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Unable to create output directory path: %s retrying.\n", dir.absolutePath().toUtf8().data());
            }

            if (dir.exists())
            {
                AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Output directory: %s created by another operation.\n", dir.absolutePath().toUtf8().data());
                return true;
            }

            if (waitTimeinSeconds != 0)
            {
                QThread::msleep(AssetUtilsInternal::g_RetryWaitInterval);
            }

        } while (!timer.hasExpired(waitTimeinSeconds * 1000));

        AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Failed to create output directory: %s after %d retries.\n", dir.absolutePath().toUtf8().data(), retries);
        return false;
    }

    QString NormalizeAndRemoveAlias(QString path)
    {
        QString normalizedPath = NormalizeFilePath(path);
        if (normalizedPath.startsWith("@"))
        {
            int aliasEndIndex = normalizedPath.indexOf("@/", Qt::CaseInsensitive);
            if (aliasEndIndex != -1)
            {
                normalizedPath.remove(0, aliasEndIndex + 2);// adding two to remove both the percentage sign and the native separator
            }
            else
            {
                //try finding the second % index than,maybe the path is like @SomeAlias@somefolder/somefile.ext
                aliasEndIndex = normalizedPath.indexOf("@", 1, Qt::CaseInsensitive);
                if (aliasEndIndex != -1)
                {
                    normalizedPath.remove(0, aliasEndIndex + 1); //adding one to remove the percentage sign only
                }
            }
        }
        return normalizedPath;
    }

    bool ComputeProjectCacheRoot(QDir& projectCacheRoot)
    {
        if (auto registry = AZ::SettingsRegistry::Get(); registry != nullptr)
        {
            AZ::SettingsRegistryInterface::FixedValueString projectCacheRootValue;
            if (registry->Get(projectCacheRootValue, AZ::SettingsRegistryMergeUtils::FilePathKey_CacheProjectRootFolder);
                !projectCacheRootValue.empty())
            {
                projectCacheRoot = QDir(QString::fromUtf8(projectCacheRootValue.c_str(), aznumeric_cast<int>(projectCacheRootValue.size())));
                return true;
            }
        }

        return false;
    }

    bool ComputeFenceDirectory(QDir& fenceDir)
    {
        QDir cacheRoot;
        if (!AssetUtilities::ComputeProjectCacheRoot(cacheRoot))
        {
            return false;
        }
        fenceDir = QDir(cacheRoot.filePath("fence"));
        return true;
    }

    QString StripAssetPlatform(AZStd::string_view relativeProductPath)
    {
        // Skip over the assetPlatform path segment if it is matches one of the platform defaults
        // Otherwise return the path unchanged
        AZStd::string_view strippedProductPath{ relativeProductPath };
        if (AZStd::optional pathSegment = AZ::StringFunc::TokenizeNext(strippedProductPath, AZ_CORRECT_AND_WRONG_FILESYSTEM_SEPARATOR);
            pathSegment.has_value())
        {
            AZ::IO::FixedMaxPathString assetPlatformSegmentLower{ *pathSegment };
            AZStd::to_lower(assetPlatformSegmentLower.begin(), assetPlatformSegmentLower.end());
            if (AzFramework::PlatformHelper::GetPlatformIdFromName(assetPlatformSegmentLower) != AzFramework::PlatformId::Invalid)
            {
                return QString::fromUtf8(strippedProductPath.data(), aznumeric_cast<int>(strippedProductPath.size()));
            }
        }

        return QString::fromUtf8(relativeProductPath.data(), aznumeric_cast<int>(relativeProductPath.size()));
    }

    QString NormalizeFilePath(const QString& filePath)
    {
        // do NOT convert to absolute paths here, we just want to manipulate the string itself.

        // note that according to the Qt Documentation, in QDir::toNativeSeparators,
        // "The returned string may be the same as the argument on some operating systems, for example on Unix.".
        // in other words, what we need here is a custom normalization - we want always the same
        // direction of slashes on all platforms.s

        QString returnString = filePath;
        returnString.replace(QChar('\\'), QChar('/'));
        returnString = QDir::cleanPath(returnString);

#if defined(AZ_PLATFORM_WINDOWS)
        // windows has an additional idiosyncrasy - it returns upper and lower case drive letters
        // from various APIs differently.  we will settle on upper case as the standard.
        if ((returnString.length() > 1) && (returnString[1] == ':'))
        {
            returnString[0] = returnString[0].toUpper();
        }
#endif

        return returnString;
    }

    QString NormalizeDirectoryPath(const QString& directoryPath)
    {
        QString dirPath(NormalizeFilePath(directoryPath));
        while ((dirPath.endsWith('/')))
        {
            dirPath.resize(dirPath.length() - 1);
        }
        return dirPath;
    }

    AZ::Uuid CreateSafeSourceUUIDFromName(const char* sourceName, bool caseInsensitive)
    {
        AZStd::string lowerVersion(sourceName);
        if (caseInsensitive)
        {
            AZStd::to_lower(lowerVersion.begin(), lowerVersion.end());
        }
        AzFramework::StringFunc::Replace(lowerVersion, '\\', '/');
        return AZ::Uuid::CreateName(lowerVersion.c_str());
    }

    void NormalizeFilePaths(QStringList& filePaths)
    {
        for (int pathIdx = 0; pathIdx < filePaths.size(); ++pathIdx)
        {
            filePaths[pathIdx] = NormalizeFilePath(filePaths[pathIdx]);
        }
    }

    unsigned int ComputeCRC32(const char* inString, unsigned int priorCRC)
    {
        AZ::Crc32 crc(priorCRC != -1 ? priorCRC : 0U);
        crc.Add(inString, ::strlen(inString), false);
        return crc;
    }

    unsigned int ComputeCRC32(const char* data, size_t dataSize, unsigned int priorCRC)
    {
        AZ::Crc32 crc(priorCRC != -1 ? priorCRC : 0U);
        crc.Add(data, dataSize, false);
        return crc;
    }

    unsigned int ComputeCRC32Lowercase(const char* inString, unsigned int priorCRC)
    {
        AZ::Crc32 crc(priorCRC != -1 ? priorCRC : 0U);
        crc.Add(inString); // note that the char* version of Add() sets lowercase to be true by default.
        return crc;
    }

    unsigned int ComputeCRC32Lowercase(const char* data, size_t dataSize, unsigned int priorCRC)
    {
        AZ::Crc32 crc(priorCRC != -1 ? priorCRC : 0U);
        crc.Add(data, dataSize, true);
        return crc;
    }

    bool UpdateBranchToken()
    {
        AZ::IO::FixedMaxPath assetProcessorUserSetregPath = AZ::Utils::GetProjectPath();
        assetProcessorUserSetregPath /= AssetProcessorUserSetregRelPath;

        AZStd::string appBranchToken;
        AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::CalculateBranchTokenForEngineRoot, appBranchToken);

        auto settingsRegistry = AZ::SettingsRegistry::Get();
        if (!settingsRegistry)
        {
            AZ_Error(AssetProcessor::ConsoleChannel, false, "Unable access Settings Registry. Branch Token cannot be updated");
            return false;
        }

        AZStd::string registryBranchToken;
        auto branchTokenKey = AZ::SettingsRegistryInterface::FixedValueString(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey) + "/assetProcessor_branch_token";
        if (settingsRegistry->Get(registryBranchToken, branchTokenKey))
        {
            if (appBranchToken == registryBranchToken)
            {
                // no need to update, branch token match
                AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Branch token (%s) is already correct in (%s)\n", appBranchToken.c_str(), assetProcessorUserSetregPath.c_str());
                return true;
            }

            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Updating branch token (%s) in (%s)\n", appBranchToken.c_str(), assetProcessorUserSetregPath.c_str());
        }
        else
        {
            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Adding branch token (%s) in (%s)\n", appBranchToken.c_str(), assetProcessorUserSetregPath.c_str());
        }

        // Update Settings Registry with new token
        settingsRegistry->Set(branchTokenKey, appBranchToken);

        return AssetUtilsInternal::DumpAssetProcessorUserSettingsToFile(*settingsRegistry, assetProcessorUserSetregPath);
    }

    QString ComputeJobDescription(const AssetProcessor::AssetRecognizer* recognizer)
    {
        return recognizer->m_name.toLower();
    }

    AZStd::string ComputeJobLogFolder()
    {
        return AZStd::string::format("@log@/JobLogs");
    }

    AZStd::string ComputeJobLogFileName(const AzToolsFramework::AssetSystem::JobInfo& jobInfo)
    {
        return AZStd::string::format("%s-%u-%llu.log", jobInfo.m_sourceFile.c_str(), jobInfo.GetHash(), jobInfo.m_jobRunKey);
    }

    AZStd::string ComputeJobLogFileName(const AssetBuilderSDK::CreateJobsRequest& createJobsRequest)
    {
        return AZStd::string::format("%s-%s_createJobs.log", createJobsRequest.m_sourceFile.c_str(), createJobsRequest.m_builderid.ToString<AZStd::string>(false).c_str());
    }

    ReadJobLogResult ReadJobLog(AzToolsFramework::AssetSystem::JobInfo& jobInfo, AzToolsFramework::AssetSystem::AssetJobLogResponse& response)
    {
        AZStd::string logFile = AssetUtilities::ComputeJobLogFolder() + "/" + AssetUtilities::ComputeJobLogFileName(jobInfo);
        return ReadJobLog(logFile.c_str(), response);
    }

    ReadJobLogResult ReadJobLog(const char* absolutePath, AzToolsFramework::AssetSystem::AssetJobLogResponse& response)
    {
        response.m_isSuccess = false;
        AZ::IO::HandleType handle = AZ::IO::InvalidHandle;
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        if (!fileIO)
        {
            AZ_TracePrintf("AssetProcessorManager", "Error: AssetProcessorManager: FileIO is unavailable\n", absolutePath);
            response.m_jobLog = "FileIO is unavailable";
            response.m_isSuccess = false;
            return ReadJobLogResult::MissingFileIO;
        }

        if (!fileIO->Open(absolutePath, AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary, handle))
        {
            AZ_TracePrintf("AssetProcessorManager", "Error: AssetProcessorManager: Failed to find the log file %s for a request.\n", absolutePath);

            response.m_jobLog.append(AZStd::string::format("Error: No log file found for the given log (%s)", absolutePath).c_str());
            response.m_isSuccess = false;

            return ReadJobLogResult::MissingLogFile;
        }

        AZ::u64 actualSize = 0;
        fileIO->Size(handle, actualSize);

        if (actualSize == 0)
        {
            AZ_TracePrintf("AssetProcessorManager", "Error: AssetProcessorManager: Log File %s is empty.\n", absolutePath);
            response.m_jobLog.append(AZStd::string::format("Error: Log is empty (%s)", absolutePath).c_str());
            response.m_isSuccess = false;
            fileIO->Close(handle);
            return ReadJobLogResult::EmptyLogFile;
        }

        size_t currentResponseSize = response.m_jobLog.size();
        response.m_jobLog.resize(currentResponseSize + actualSize);

        fileIO->Read(handle, response.m_jobLog.data() + currentResponseSize, actualSize);
        fileIO->Close(handle);
        response.m_isSuccess = true;
        return ReadJobLogResult::Success;
    }

    unsigned int GenerateFingerprint(const AssetProcessor::JobDetails& jobDetail)
    {
        // it is assumed that m_fingerprintFilesList contains the original file and all dependencies, and is in a stable order without duplicates
        // CRC32 is not an effective hash for this purpose, so we will build a string and then use SHA1 on it.

        // to avoid resizing and copying repeatedly we will keep track of the largest reserved capacity ever needed for this function, and reserve that much data
        static size_t s_largestFingerprintCapacitySoFar = 1;
        AZStd::string fingerprintString;
        fingerprintString.reserve(s_largestFingerprintCapacitySoFar);

        // in general, we'll build a string which is:
        // (version):[Array of individual file fingerprints][Array of individual job fingerprints]
        // with each element of the arrays seperated by colons.

        fingerprintString.append(jobDetail.m_extraInformationForFingerprinting);

        for (const auto& fingerprintFile : jobDetail.m_fingerprintFiles)
        {
            fingerprintString.append(":");
            fingerprintString.append(GetFileFingerprint(fingerprintFile.first, fingerprintFile.second));
        }
        // now the other jobs, which this job depends on:
        for (const AssetProcessor::JobDependencyInternal& jobDependencyInternal : jobDetail.m_jobDependencyList)
        {
            if (jobDependencyInternal.m_jobDependency.m_type == AssetBuilderSDK::JobDependencyType::OrderOnce)
            {
                // we do not want to include the fingerprint of dependent jobs if the job dependency type is OrderOnce.
                continue;
            }
            AssetProcessor::JobDesc jobDesc(jobDependencyInternal.m_jobDependency.m_sourceFile.m_sourceFileDependencyPath,
                jobDependencyInternal.m_jobDependency.m_jobKey, jobDependencyInternal.m_jobDependency.m_platformIdentifier);

            for (auto builderIter = jobDependencyInternal.m_builderUuidList.begin(); builderIter != jobDependencyInternal.m_builderUuidList.end(); ++builderIter)
            {
                AZ::u32 dependentJobFingerprint;
                AssetProcessor::ProcessingJobInfoBus::BroadcastResult(dependentJobFingerprint, &AssetProcessor::ProcessingJobInfoBusTraits::GetJobFingerprint, AssetProcessor::JobIndentifier(jobDesc, *builderIter));
                if (dependentJobFingerprint != 0)
                {
                    fingerprintString.append(AZStd::string::format(":%u", dependentJobFingerprint));
                }
            }
        }
        s_largestFingerprintCapacitySoFar = AZStd::GetMax(fingerprintString.capacity(), s_largestFingerprintCapacitySoFar);

        if (fingerprintString.empty())
        {
            AZ_Assert(false, "GenerateFingerprint was called but no input files were requested for fingerprinting.");
            return 0;
        }

        AZ::Sha1 sha;
        sha.ProcessBytes(fingerprintString.data(), fingerprintString.size());
        AZ::u32 digest[5];
        sha.GetDigest(digest);

        return digest[0]; // we only currently use 32-bit hashes.  This could be extended if collisions still occur.
    }

    std::uint64_t AdjustTimestamp(QDateTime timestamp, int overridePrecision)
    {
        if (timestamp.isDaylightTime())
        {
            int offsetTimeinSecs = timestamp.timeZone().daylightTimeOffset(timestamp);
            timestamp = timestamp.addSecs(-1 * offsetTimeinSecs);
        }

        timestamp = timestamp.toUTC();

        auto timeMilliseconds = timestamp.toMSecsSinceEpoch();

        int checkPrecision = (overridePrecision ? overridePrecision : s_truncateFingerprintTimestampPrecision);
        // Reduce the precision from milliseconds to the specified precision (default is 1, so no change)
        timeMilliseconds /= checkPrecision;
        timeMilliseconds *= checkPrecision;

        return timeMilliseconds;
    }

    AZ::u64 GetFileHash(const char* filePath, bool force, AZ::IO::SizeType* bytesReadOut, int hashMsDelay)
    {
#ifndef AZ_TESTS_ENABLED
        // Only used for unit tests, speed is critical for GetFileHash.
        hashMsDelay = 0;
#endif
        bool useFileHashing = ShouldUseFileHashing();

        if(!useFileHashing || !filePath)
        {
            return 0;
        }

        AZ::u64 hash = 0;
        if(!force)
        {
            auto* fileStateInterface = AZ::Interface<AssetProcessor::IFileStateRequests>::Get();

            if (fileStateInterface && fileStateInterface->GetHash(filePath, &hash))
            {
                return hash;
            }
        }

        // keep track of how much time we spend actually hashing files.
        AZStd::string statName = AZStd::string::format("HashFile,%s", filePath);
        AssetProcessor::StatsCapture::BeginCaptureStat(statName.c_str());
        hash = AssetBuilderSDK::GetFileHash(filePath, bytesReadOut, hashMsDelay);
        AssetProcessor::StatsCapture::EndCaptureStat(statName.c_str());
        return hash;
    }

    AZ::u64 AdjustTimestamp(QDateTime timestamp)
    {
        timestamp = timestamp.toUTC();

        auto timeMilliseconds = timestamp.toMSecsSinceEpoch();

        // Reduce the precision from milliseconds to the specified precision (default is 1, so no change)
        timeMilliseconds /= s_truncateFingerprintTimestampPrecision;
        timeMilliseconds *= s_truncateFingerprintTimestampPrecision;

        return timeMilliseconds;
    }

    AZStd::string GetFileFingerprint(const AZStd::string& absolutePath, const AZStd::string& nameToUse)
    {
        bool fileFound = false;
        AssetProcessor::FileStateInfo fileStateInfo;

        auto* fileStateInterface = AZ::Interface<AssetProcessor::IFileStateRequests>::Get();
        if (fileStateInterface)
        {
            fileFound = fileStateInterface->GetFileInfo(QString::fromUtf8(absolutePath.c_str()), &fileStateInfo);
        }

        QDateTime lastModifiedTime = fileStateInfo.m_modTime;
        if (!fileFound || !lastModifiedTime.isValid())
        {
            // we still use the name here so that when missing files change, it still counts as a change.
            // we also don't use '0' as the placeholder, so that there is a difference between files that do not exist
            // and files which have 0 bytes size.
            return AZStd::string::format("-:-:%s", nameToUse.c_str());
        }
        else
        {
            bool useHash = ShouldUseFileHashing();
            AZ::u64 fileIdentifier;
            if(useHash)
            {
                fileIdentifier = GetFileHash(absolutePath.c_str());
            }
            else
            {
                fileIdentifier = AdjustTimestamp(lastModifiedTime);
            }

            // its possible that the dependency has moved to a different file with the same modtime/hash
            // so we add the size of it too.
            // its also possible that it moved to a different file with the same modtime/hash AND size,
            // but with a different name.  So we add that too.
            return AZStd::string::format("%llX:%llu:%s", fileIdentifier, fileStateInfo.m_fileSize, nameToUse.c_str());
        }
    }

    AZStd::string ComputeJobLogFileName(const AssetProcessor::JobEntry& jobEntry)
    {
        return AZStd::string::format("%s-%u-%llu.log", jobEntry.m_databaseSourceName.toUtf8().constData(), jobEntry.GetHash(), jobEntry.m_jobRunKey);
    }

    bool CreateTempRootFolder(QString startFolder, QDir& tempRoot)
    {
        tempRoot.setPath(startFolder);

        if (!tempRoot.exists("AssetProcessorTemp"))
        {
            if (!tempRoot.mkpath("AssetProcessorTemp"))
            {
                AZ_WarningOnce("Asset Utils", false, "Could not create a temp folder at %s", startFolder.toUtf8().constData());
                return false;
            }
        }

        if (!tempRoot.cd("AssetProcessorTemp"))
        {
            AZ_WarningOnce("Asset Utils", false, "Could not access temp folder at %s/AssetProcessorTemp", startFolder.toUtf8().constData());
            return false;
        }

        return true;
    }

    bool CreateTempWorkspace(QString startFolder, QString& result)
    {
        if (!AssetUtilsInternal::g_hasInitializedRandomNumberGenerator)
        {
            AssetUtilsInternal::g_hasInitializedRandomNumberGenerator = true;
            // seed the random number generator a different seed as the main thread.  random numbers are thread-specific.
            // note that 0 is an invalid random seed.
            AzQtComponents::GetRandomGenerator()->seed(QTime::currentTime().msecsSinceStartOfDay() + AssetUtilsInternal::g_randomNumberSequentialSeed.fetch_add(1) + 1);
        }

        QDir tempRoot;

        if (!CreateTempRootFolder(startFolder, tempRoot))
        {
            result.clear();
            return false;
        }

        // try multiple times in the very low chance that its going to be a collision:

        for (int attempts = 0; attempts < 3; ++attempts)
        {
            QTemporaryDir tempDir(tempRoot.absoluteFilePath("JobTemp-XXXXXX"));
            tempDir.setAutoRemove(false);

            if ((tempDir.path().isEmpty()) || (!QDir(tempDir.path()).exists()))
            {
                QByteArray errorData = tempDir.errorString().toUtf8();
                AZ_WarningOnce("Asset Utils", false, "Could not create new temp folder in %s - error from OS is '%s'", tempRoot.absolutePath().toUtf8().constData(), errorData.constData());
                result.clear();
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(100));
                continue;
            }

            result = tempDir.path();
            break;
        }
        return !result.isEmpty();
    }

    bool CreateTempWorkspace(QString& result)
    {
        // Use the project user folder as a temp workspace folder
        // The benefits are
        // * It's on the same drive as the Cache/ so we will be moving files instead of copying from drive to drive
        // * It is discoverable by the user and thus deletable and we can also tell people to send us that folder without them having to go digging for it

        QDir rootDir;
        bool foundValidPath{};
        if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
        {
            if (AZ::IO::Path userPath; settingsRegistry->Get(userPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectUserPath))
            {
                rootDir.setPath(QString::fromUtf8(userPath.c_str(), aznumeric_cast<int>(userPath.Native().size())));
                foundValidPath = true;
            }
        }

        if (!foundValidPath)
        {
            foundValidPath = ComputeAssetRoot(rootDir);
        }

        if (foundValidPath)
        {
            QString tempPath = rootDir.absolutePath();
            return CreateTempWorkspace(tempPath, result);
        }
        result.clear();
        return false;
    }

    QString GuessProductNameInDatabase(QString path, QString platform, AssetProcessor::AssetDatabaseConnection* databaseConnection)
    {
        QString productName;
        QString inputName;
        QString platformName;
        QString jobDescription;

        using namespace AzToolsFramework::AssetDatabase;

        productName = AssetUtilities::NormalizeAndRemoveAlias(path);

        // most of the time, the incoming request will be for an actual product name, so optimize this by assuming that is the case
        // and do an optimized query for it
        if (platform.isEmpty())
        {
            platform = AzToolsFramework::AssetSystem::GetHostAssetPlatform();
        }

        QString platformPrepend = QString("%1/").arg(platform);
        QString productNameWithPlatform = productName;

        if (!productName.startsWith(platformPrepend, Qt::CaseInsensitive))
        {
            productNameWithPlatform = productName = QString("%1/%2").arg(platform, productName);
        }

        ProductDatabaseEntryContainer products;
        if (databaseConnection->GetProductsByProductName(productNameWithPlatform, products))
        {
            // if we find stuff, then return immediately, productName is already a productName.
            return productName;
        }

        // if that fails, see at least if it starts with the given product name.

        if (databaseConnection->GetProductsLikeProductName(productName, AssetDatabaseConnection::LikeType::StartsWith, products))
        {
            return productName;
        }

        if (!databaseConnection->GetProductsLikeProductName(productNameWithPlatform, AssetDatabaseConnection::LikeType::StartsWith, products))
        {
            return {};
        }
        return productName.toLower();
    }

    static void CollectDependenciesRecursively(AssetProcessor::AssetDatabaseConnection& databaseConnection, const AZ::Uuid& assetId,
        AZStd::unordered_set<AZ::Uuid>& uuidSet, AZStd::vector<AZ::Uuid>& dependecyList)
    {
        if (uuidSet.count(assetId))
        {
            return;
        }
        dependecyList.push_back(assetId);
        uuidSet.insert(assetId);
        AzToolsFramework::AssetDatabase::SourceDatabaseEntry entry;
        if (!databaseConnection.GetSourceBySourceGuid(assetId, entry))
        {
            return;
        }

        AzToolsFramework::AssetDatabase::SourceFileDependencyEntryContainer container;
        if (!databaseConnection.GetDependsOnSourceBySource(entry.m_sourceName.c_str(), AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_Any, container))
        {
            return;
        }
        for (const auto& sourceFileEntry : container)
        {
            databaseConnection.QuerySourceBySourceName(sourceFileEntry.m_dependsOnSource.c_str(), [&](AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry)
                {
                    CollectDependenciesRecursively(databaseConnection, entry.m_sourceGuid, uuidSet, dependecyList);
                    return true;
                });
        }
    }

    AZStd::vector<AZ::Uuid> CollectAssetAndDependenciesRecursively(AssetProcessor::AssetDatabaseConnection& databaseConnection, const AZStd::vector<AZ::Uuid>& assetList)
    {
        AZStd::unordered_set<AZ::Uuid> uuidSet; // Used to guarantee uniqueness and prevent infinite recursion.
        AZStd::vector<AZ::Uuid> completeAssetList;
        for (const AZ::Uuid& assetId : assetList)
        {
            CollectDependenciesRecursively(databaseConnection, assetId, uuidSet, completeAssetList);
        }
        return completeAssetList;
    }

    bool UpdateToCorrectCase(const QString& rootPath, QString& relativePathFromRoot)
    {
        // normalize the input string:
        relativePathFromRoot = NormalizeFilePath(relativePathFromRoot);

#if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_MAC)
        // these operating systems use File Systems which are generally not case sensitive, and so we can make this function "early out" a lot faster.
        // by quickly determining the case where it does not exist at all.  Without this assumption, it can be hundreds of times slower.
        bool exists = false;
        auto* fileStateInterface = AZ::Interface<AssetProcessor::IFileStateRequests>::Get();
        if (fileStateInterface)
        {
            exists = fileStateInterface->Exists(QDir(rootPath).absoluteFilePath(relativePathFromRoot));
        }

        if (!exists)
        {
            return false;
        }
#endif

        AZStd::string relPathFromRoot = relativePathFromRoot.toUtf8().constData();
        if(AzToolsFramework::AssetUtils::UpdateFilePathToCorrectCase(rootPath.toUtf8().constData(), relPathFromRoot))
        {
            relativePathFromRoot = QString::fromUtf8(relPathFromRoot.c_str(), aznumeric_cast<int>(relPathFromRoot.size()));
            return true;
        }
        return false;
    }

    BuilderFilePatternMatcher::BuilderFilePatternMatcher(const AssetBuilderSDK::AssetBuilderPattern& pattern, const AZ::Uuid& builderDescID)
        : AssetBuilderSDK::FilePatternMatcher(pattern)
        , m_builderDescID(builderDescID)
    {
    }

    BuilderFilePatternMatcher::BuilderFilePatternMatcher(const BuilderFilePatternMatcher& copy)
        : AssetBuilderSDK::FilePatternMatcher(copy)
        , m_builderDescID(copy.m_builderDescID)
    {
    }

    const AZ::Uuid& BuilderFilePatternMatcher::GetBuilderDescID() const
    {
        return this->m_builderDescID;
    };

    QuitListener::QuitListener()
        : m_requestedQuit(false)
    {
    }

    QuitListener::~QuitListener()
    {
        BusDisconnect();
    }

    void QuitListener::ApplicationShutdownRequested()
    {
        m_requestedQuit = true;
    }

    bool QuitListener::WasQuitRequested() const
    {
        return m_requestedQuit;
    }

    JobLogTraceListener::JobLogTraceListener(const AZStd::string& logFileName, AZ::s64 jobKey, bool overwriteLogFile /* = false */)
    {
        m_logFileName = AssetUtilities::ComputeJobLogFolder() + "/" + logFileName;
        m_runKey = jobKey;
        m_forceOverwriteLog = overwriteLogFile;
        AZ::Debug::TraceMessageBus::Handler::BusConnect();
    }

    JobLogTraceListener::JobLogTraceListener(const AzToolsFramework::AssetSystem::JobInfo& jobInfo, bool overwriteLogFile /* = false */)
    {
        m_logFileName = AssetUtilities::ComputeJobLogFolder() + "/" + AssetUtilities::ComputeJobLogFileName(jobInfo);
        m_runKey = jobInfo.m_jobRunKey;
        m_forceOverwriteLog = overwriteLogFile;
        AZ::Debug::TraceMessageBus::Handler::BusConnect();
    }

    JobLogTraceListener::JobLogTraceListener(const AssetProcessor::JobEntry& jobEntry, bool overwriteLogFile /* = false */)
    {
        m_logFileName = AssetUtilities::ComputeJobLogFolder() + "/" + AssetUtilities::ComputeJobLogFileName(jobEntry);
        m_runKey = jobEntry.m_jobRunKey;
        m_forceOverwriteLog = overwriteLogFile;
        AZ::Debug::TraceMessageBus::Handler::BusConnect();
    }

    JobLogTraceListener::~JobLogTraceListener()
    {
        BusDisconnect();
    }

    bool JobLogTraceListener::OnAssert(const char* message)
    {
        if (AssetProcessor::GetThreadLocalJobId() == m_runKey)
        {
            AppendLog(AzFramework::LogFile::SEV_ASSERT, "ASSERT", message);
            return true;
        }

        return false;
    }

    bool JobLogTraceListener::OnException(const char* message)
    {
        if (AssetProcessor::GetThreadLocalJobId() == m_runKey)
        {
            m_inException = true;
            AppendLog(AzFramework::LogFile::SEV_EXCEPTION, "EXCEPTION", message);
            // we return false here so that the main app can also trace it, exceptions are bad enough
            // that we want them to show up in all the logs.
        }

        return false;
    }

    // we want no trace of errors to show up from jobs, inside the console app
    // only in explicit usages, so we return true for pre-error here too
    bool JobLogTraceListener::OnPreError(const char* window, const char* /*file*/, int /*line*/, const char* /*func*/, const char* message)
    {
        if (AssetProcessor::GetThreadLocalJobId() == m_runKey)
        {
            AppendLog(m_inException ? AzFramework::LogFile::SEV_EXCEPTION : AzFramework::LogFile::SEV_ERROR, window, message);
            return true;
        }

        return false;
    }

    bool JobLogTraceListener::OnWarning(const char* window, const char* message)
    {
        if (AssetProcessor::GetThreadLocalJobId() == m_runKey)
        {
            AppendLog(m_inException ? AzFramework::LogFile::SEV_EXCEPTION : AzFramework::LogFile::SEV_WARNING, window, message);
            return true;
        }

        return false;
    }

    bool JobLogTraceListener::OnPrintf(const char* window, const char* message)
    {
        if (AssetProcessor::GetThreadLocalJobId() == m_runKey)
        {
            if(azstrnicmp(message, "S: ", 3) == 0)
            {
                std::string dummy;
                std::istringstream stream(message);

                stream >> dummy >> m_errorCount >> dummy >> m_warningCount;
            }

            if (azstrnicmp(window, "debug", 5) == 0)
            {
                AppendLog(AzFramework::LogFile::SEV_DEBUG, window, message);
            }
            else
            {
                AppendLog(m_inException ? AzFramework::LogFile::SEV_EXCEPTION : AzFramework::LogFile::SEV_NORMAL, window, message);
            }

            return true;
        }

        return false;
    }

    void JobLogTraceListener::AppendLog(AzFramework::LogFile::SeverityLevel severity, const char* window, const char* message)
    {
        if (m_isLogging)
        {
            return;
        }

        m_isLogging = true;

        if (!m_logFile)
        {
            m_logFile.reset(new AzFramework::LogFile(m_logFileName.c_str(), m_forceOverwriteLog));
        }
        m_logFile->AppendLog(severity, window, message);
        m_isLogging = false;
    }

    void JobLogTraceListener::AppendLog(AzToolsFramework::Logging::LogLine& logLine)
    {
        using namespace AzToolsFramework;
        using namespace AzFramework;

        if (m_isLogging)
        {
            return;
        }

        m_isLogging = true;

        if (!m_logFile)
        {
            m_logFile.reset(new LogFile(m_logFileName.c_str(), m_forceOverwriteLog));
        }

        LogFile::SeverityLevel severity;

        switch (logLine.GetLogType())
        {
        case Logging::LogLine::TYPE_MESSAGE:
            severity = LogFile::SEV_NORMAL;
            break;
        case Logging::LogLine::TYPE_WARNING:
            severity = LogFile::SEV_WARNING;
            break;
        case Logging::LogLine::TYPE_ERROR:
            severity = LogFile::SEV_ERROR;
            break;
        default:
            severity = LogFile::SEV_DEBUG;
        }

        m_logFile->AppendLog(severity, logLine.GetLogMessage().c_str(), (int)logLine.GetLogMessage().length(),
            logLine.GetLogWindow().c_str(), (int)logLine.GetLogWindow().length(), logLine.GetLogThreadId(), logLine.GetLogTime());
        m_isLogging = false;
    }

    AZ::s64 JobLogTraceListener::GetErrorCount() const
    {
        return m_errorCount;
    }

    AZ::s64 JobLogTraceListener::GetWarningCount() const
    {
        return m_warningCount;
    }

    void JobLogTraceListener::AddError()
    {
        ++m_errorCount;
    }

    void JobLogTraceListener::AddWarning()
    {
        ++m_warningCount;
    }
} // namespace AssetUtilities
