/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SourceFileRelocator.h"
#include "FileStateCache.h"
#include <AzCore/IO/SystemFile.h>
#include <QDir>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzCore/Component/TickBus.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <native/utilities/BatchApplicationManager.h>
#include <cinttypes>

namespace AssetProcessor
{
    // note that this will be true if EITHER the source control is valid, connected, and working
    // OR if source control is completely disabled and thus will just transparently pass thru to the file system
    // it will only return false if the source control plugin is ACTIVE but is failing to function due to a configuration
    // problem, or the executable being missing.
    bool IsSourceControlValid()
    {
        using SCRequest = AzToolsFramework::SourceControlConnectionRequestBus;
        AzToolsFramework::SourceControlState state = AzToolsFramework::SourceControlState::Disabled;
        SCRequest::BroadcastResult(state, &SCRequest::Events::GetSourceControlState);
        return (state != AzToolsFramework::SourceControlState::ConfigurationInvalid);
    }

    bool WaitForSourceControl(AZStd::binary_semaphore& waitSignal)
    {
        constexpr int MaxWaitTimeMS = 10000;
        constexpr int SleepTimeMS = 50;
        int retryCount = MaxWaitTimeMS / SleepTimeMS;

        AZ::TickBus::ExecuteQueuedEvents();

        while (!waitSignal.try_acquire_for(AZStd::chrono::milliseconds(SleepTimeMS)) && retryCount >= 0)
        {
            --retryCount;
            AZ::TickBus::ExecuteQueuedEvents();
        }

        if (retryCount < 0)
        {
            AZ_Error("SourceFileRelocator", false, "Timed out waiting for response from source control component.");
            return false;
        }

        return true;
    }


    void WildcardHelper(AZStd::string& path)
    {
        path.replace(path.length() - 1, 1, "...");
    }

    void AdjustWildcardForPerforce(AZStd::string& source)
    {
        if (source.ends_with('*'))
        {
            WildcardHelper(source);
        }
    }

    void AdjustWildcardForPerforce(AZStd::string& source, AZStd::string& destination)
    {
        if (source.ends_with('*') && destination.ends_with('*'))
        {
            WildcardHelper(source);
            WildcardHelper(destination);
        }
    }

    SourceFileRelocator::SourceFileRelocator(AZStd::shared_ptr<AzToolsFramework::AssetDatabase::AssetDatabaseConnection> stateData, PlatformConfiguration* platformConfiguration)
        : m_stateData(AZStd::move(stateData))
        , m_platformConfig(platformConfiguration)
    {
        AZ::Interface<ISourceFileRelocation>::Register(this);
        m_additionalHelpTextMap[AZStd::string("seed")] = AZStd::string("\t\tPlease note that path hints in the seed file might not be correct as a result of this file reference fixup,\
you can update the path hints by running the AssetBundlerBatch. Please run AssetBundlerBatch --help to find the correct command for updating the path hints for seed files.\
Please note that only those seed files will get updated that are active for your current game project. If there are seed files that are not active for your current game project and does contain\
 references to files that are being moved, then asset processor won't be able to catch these references and perform the fixup and the user would have to update them manually.\n");
    }

    SourceFileRelocator::~SourceFileRelocator()
    {
        AZ::Interface<ISourceFileRelocation>::Unregister(this);
    }

    AZ::Outcome<void, AZStd::string> SourceFileRelocator::GetScanFolderAndRelativePath(const AZStd::string& normalizedSource, bool allowNonexistentPath, const ScanFolderInfo*& scanFolderInfo, AZStd::string& relativePath) const
    {
        scanFolderInfo = nullptr;
        bool isRelative = AzFramework::StringFunc::Path::IsRelative(normalizedSource.c_str());

        if (isRelative)
        {
            // Relative paths can match multiple files/folders, search each scan folder for a valid match
            QString matchedPath;
            QString tempRelativeName(normalizedSource.c_str());

            for(int i = 0; i < m_platformConfig->GetScanFolderCount(); ++i)
            {
                const AssetProcessor::ScanFolderInfo* scanFolderInfoCheck = &m_platformConfig->GetScanFolderAt(i);

                if ((!scanFolderInfoCheck->RecurseSubFolders()) && (tempRelativeName.contains('/')))
                {
                    // the name is a deeper relative path, but we don't recurse this scan folder, so it can't win
                    continue;
                }
                QDir rooted(scanFolderInfoCheck->ScanPath());
                QString absolutePath = rooted.absoluteFilePath(tempRelativeName);
                bool fileExists = false;
                auto* fileStateInterface = AZ::Interface<IFileStateRequests>::Get();
                if(fileStateInterface)
                {
                    fileExists = fileStateInterface->Exists(absolutePath);
                }

                if (fileExists)
                {
                    if (matchedPath.isEmpty())
                    {
                        matchedPath = AssetUtilities::NormalizeFilePath(absolutePath);
                        scanFolderInfo = scanFolderInfoCheck;
                    }
                    else
                    {
                        return AZ::Failure(AZStd::string::format("Relative path matched multiple files/folders.  Please narrow your query by using an absolute path or, if using wildcards, try making your query path more specific.\nMatch 1: %s\nMatch 2: %s\n",
                            matchedPath.toUtf8().constData(),
                            absolutePath.toUtf8().constData()));
                    }
                }
            }

            if(allowNonexistentPath && !scanFolderInfo)
            {
                // We didn't find a match, so assume the path refers to a folder in the highest priority scanfolder
                scanFolderInfo = &m_platformConfig->GetScanFolderAt(0);
            }
        }
        else
        {
            scanFolderInfo = m_platformConfig->GetScanFolderForFile(normalizedSource.c_str());
        }

        if (!scanFolderInfo)
        {
            return AZ::Failure(AZStd::string::format("Path %s points to a file outside the current project's scan folders.\n", normalizedSource.c_str()));
        }

        if (isRelative)
        {
            relativePath = normalizedSource;
        }
        else
        {
            QString relativePathQString;
            if (!PlatformConfiguration::ConvertToRelativePath(normalizedSource.c_str(), scanFolderInfo, relativePathQString))
            {
                return AZ::Failure(AZStd::string::format("Failed to convert path to relative path. %s\n", normalizedSource.c_str()));
            }

            relativePath = relativePathQString.toUtf8().constData();
        }

        return AZ::Success();
    }

    QHash<QString, int> SourceFileRelocator::GetSources(QStringList pathMatches, const ScanFolderInfo* scanFolderInfo,
        SourceFileRelocationContainer& sources,
        bool allowNonDatabaseFiles) const
    {
        auto* uuidInterface = AZ::Interface<AssetProcessor::IUuidRequests>::Get();
        AZ_Assert(uuidInterface, "Programmer Error - IUuidRequests interface is not available.");

        QHash<QString, int> sourceIndexMap;
        QSet<QString> filesNotInAssetDatabase;
        for (auto& file : pathMatches)
        {
            QString databaseSourceName;
            int sourcesSize = aznumeric_cast<int>(sources.size());
            PlatformConfiguration::ConvertToRelativePath(file, scanFolderInfo, databaseSourceName);
            filesNotInAssetDatabase.insert(databaseSourceName);
            m_stateData->QuerySourceBySourceNameScanFolderID(
                databaseSourceName.toUtf8().constData(),
                scanFolderInfo->ScanFolderID(),
                [this, &sources, &scanFolderInfo, &sourceIndexMap, &databaseSourceName, &filesNotInAssetDatabase, uuidInterface](
                    const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry)
                {
                    const bool isMetadataType = uuidInterface->IsGenerationEnabledForFile(databaseSourceName.toUtf8().constData());
                    sources.emplace_back(
                        entry, GetProductMapForSource(entry.m_sourceID), entry.m_sourceName, scanFolderInfo, isMetadataType);
                    filesNotInAssetDatabase.remove(databaseSourceName);
                    sourceIndexMap[databaseSourceName] = aznumeric_cast<int>(sources.size() - 1);

                    return true;
                });
            // If allowNonDatabaseFiles is true add any source files that have no database entry so that they may be moved/deleted
            if (sources.size() == sourcesSize && allowNonDatabaseFiles)
            {
                AZStd::unordered_map<int, AzToolsFramework::AssetDatabase::ProductDatabaseEntry> products;
                AzToolsFramework::AssetDatabase::SourceDatabaseEntry entry;
                entry.m_scanFolderPK = scanFolderInfo->ScanFolderID();
                entry.m_sourceName = databaseSourceName.toUtf8().constData();
                sources.emplace_back(entry, products, entry.m_sourceName, scanFolderInfo, false);
                filesNotInAssetDatabase.remove(databaseSourceName);
                sourceIndexMap[databaseSourceName] = aznumeric_cast<int>(sources.size() - 1);
            }
        }

        for (const QString& file : filesNotInAssetDatabase)
        {
            AZ_Printf("AssetProcessor", "File `%s` was found/matched but is not a source asset.  Skipping.\n", file.toUtf8().constData());
        }

        return sourceIndexMap;
    }

    void SourceFileRelocator::HandleMetaDataFiles(QStringList pathMatches, QHash<QString, int>& sourceIndexMap, const ScanFolderInfo* scanFolderInfo, SourceFileRelocationContainer& metadataFiles, bool excludeMetaDataFiles) const
    {
        QSet<QString> metaDataFileEntries;

        // Remove all the metadata files
        if (excludeMetaDataFiles)
        {
            pathMatches.erase(AZStd::remove_if(pathMatches.begin(), pathMatches.end(), [this](const QString& file)
                {
                    for (int idx = 0; idx < m_platformConfig->MetaDataFileTypesCount(); idx++)
                    {
                        const auto& [metadataType, extension] = m_platformConfig->GetMetaDataFileTypeAt(idx);
                        if (file.endsWith("." + metadataType, Qt::CaseInsensitive))
                        {
                            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Metadata file %s will be ignored because --excludeMetadataFiles was specified in the command line.\n",
                                file.toUtf8().constData());
                            return true;
                        }
                    }
                    return false;
                }),
                pathMatches.end());
        }

        for (const QString& file : pathMatches)
        {
            for (int idx = 0; idx < m_platformConfig->MetaDataFileTypesCount(); idx++)
            {
                const auto& [metadataType, extension] = m_platformConfig->GetMetaDataFileTypeAt(idx);
                if (file.endsWith("." + metadataType, Qt::CaseInsensitive))
                {
                    const QString normalizedFilePath = AssetUtilities::NormalizeFilePath(file);
                    if (!metaDataFileEntries.contains(normalizedFilePath))
                    {
                        SourceFileRelocationInfo metaDataFile(file.toUtf8().data(), scanFolderInfo);
                        metaDataFile.m_metadataIndex = idx;
                        metadataFiles.emplace_back(metaDataFile);
                        metaDataFileEntries.insert(normalizedFilePath);
                    }
                }
                else if (!excludeMetaDataFiles && (file.endsWith("." + extension, Qt::CaseInsensitive) || extension.isEmpty()))
                {
                    // if we are here it implies that a metadata file might exists for this source file,
                    // add metadata file only if it exists and is not added already
                    AZStd::string metadataFilePath(file.toUtf8().data());
                    if (extension.isEmpty())
                    {
                        metadataFilePath.append(AZStd::string::format(".%s", metadataType.toUtf8().data()));
                    }
                    else
                    {
                        AZ::StringFunc::Path::ReplaceExtension(metadataFilePath, metadataType.toUtf8().data());
                    };

                    // The metadata file can have a different case than the source file,
                    // We are trying to finding the correct case for the file here

                    QString metadaFileCorrectCase;
                    QFileInfo fileInfo(metadataFilePath.c_str());
                    QStringList fileEntries = fileInfo.absoluteDir().entryList(QDir::Files);
                    for (const QString& fileEntry : fileEntries)
                    {
                        if (QString::compare(fileEntry, fileInfo.fileName(), Qt::CaseInsensitive) == 0)
                        {
                            metadaFileCorrectCase = AssetUtilities::NormalizeFilePath(fileInfo.absoluteDir().filePath(fileEntry));
                            break;
                        }
                    }

                    if (QFile::exists(metadataFilePath.c_str()) && metaDataFileEntries.find(metadaFileCorrectCase) == metaDataFileEntries.end())
                    {
                        QString databaseSourceName;
                        PlatformConfiguration::ConvertToRelativePath(file, scanFolderInfo, databaseSourceName);
                        auto sourceFileIndex = sourceIndexMap.find(databaseSourceName);
                        if (sourceFileIndex != sourceIndexMap.end())
                        {
                            SourceFileRelocationInfo metaDataFile(metadaFileCorrectCase.toUtf8().data(), scanFolderInfo);
                            metaDataFile.m_metadataIndex = idx;
                            metaDataFile.m_sourceFileIndex = sourceFileIndex.value();
                            metadataFiles.emplace_back(metaDataFile);
                            metaDataFileEntries.insert(metadaFileCorrectCase);
                        }
                    }
                }
            }
        }
    }

    AZStd::unordered_map<int, AzToolsFramework::AssetDatabase::ProductDatabaseEntry> SourceFileRelocator::GetProductMapForSource(AZ::s64 sourceId) const
    {
        AZStd::unordered_map<int, AzToolsFramework::AssetDatabase::ProductDatabaseEntry> products;

        m_stateData->QueryProductBySourceID(sourceId, [&products](const AzToolsFramework::AssetDatabase::ProductDatabaseEntry& entry)
            {
                products.emplace(entry.m_subID, entry);
                return true;
            });

        return products;
    }

    bool SourceFileRelocator::GetFilesFromSourceControl(
        SourceFileRelocationContainer& sources,
        const ScanFolderInfo* scanFolderInfo,
        QString absolutePath,
        bool excludeMetaDataFiles,
        bool allowNonDatabaseFiles) const
    {
        QStringList pathMatches;
        AZStd::binary_semaphore waitSignal;

        AzToolsFramework::SourceControlResponseCallbackBulk filesInfoCallback = [&](bool success, AZStd::vector<AzToolsFramework::SourceControlFileInfo> filesInfo)
        {
            if (success)
            {
                for (const auto& fileInfo : filesInfo)
                {
                    if (AZ::IO::SystemFile::Exists(fileInfo.m_filePath.c_str()))
                    {
                        pathMatches.append(fileInfo.m_filePath.c_str());
                    }
                }

                QHash<QString, int> sourceIndexMap = GetSources(pathMatches, scanFolderInfo, sources, allowNonDatabaseFiles);
                HandleMetaDataFiles(pathMatches, sourceIndexMap, scanFolderInfo, sources, excludeMetaDataFiles);
            }

            waitSignal.release();
        };

        AZStd::string adjustedPerforceSearchString(absolutePath.toUtf8().data());
        AdjustWildcardForPerforce(adjustedPerforceSearchString);

        AZStd::unordered_set<AZStd::string> normalizedDir = { adjustedPerforceSearchString };

        AzToolsFramework::SourceControlCommandBus::Broadcast(&AzToolsFramework::SourceControlCommands::GetBulkFileInfo, normalizedDir, filesInfoCallback);
        WaitForSourceControl(waitSignal);

        return !pathMatches.isEmpty();
    }

    AZ::Outcome<void, AZStd::string> SourceFileRelocator::GetSourcesByPath(
        const AZStd::string& normalizedSource,
        SourceFileRelocationContainer& sources,
        const ScanFolderInfo*& scanFolderInfoOut,
        bool excludeMetaDataFiles,
        bool allowNonDatabaseFiles) const
    {
        // nothing below will succeed if source control is active but invalid, so early out with a clear
        // warning to the user.
        if (!IsSourceControlValid())
        {
            // no point in continuing, it will always fail.
            return AZ::Failure(AZStd::string("The Source Control plugin is active but the configuration is invalid.\n" 
                                             "Either disable it by right-clicking the source control icon in the editor status bar,\n"
                                             "or fix the configuration of it in that same right-click menu.\n"));
        }

        if(normalizedSource.find("**") != AZStd::string::npos)
        {
            return AZ::Failure(AZStd::string("Consecutive wildcards are not allowed.  Please remove extra wildcards from your query.\n"));
        }

        bool fileExists = false;
        bool isWildcard = normalizedSource.find('*') != AZStd::string_view::npos || normalizedSource.find('?') != AZStd::string_view::npos;

        if (isWildcard)
        {
            bool isRelative = AzFramework::StringFunc::Path::IsRelative(normalizedSource.c_str());
            scanFolderInfoOut = nullptr;

            if (isRelative)
            {
                // For relative wildcard paths, we'll test the source path in each scan folder to see if we find any matches
                bool foundMatch = false;
                bool sourceContainsSlash = normalizedSource.find('/') != normalizedSource.npos;

                for (int i = 0; i < m_platformConfig->GetScanFolderCount(); ++i)
                {
                    const ScanFolderInfo* scanFolderInfo = &m_platformConfig->GetScanFolderAt(i);

                    if(!scanFolderInfo->RecurseSubFolders() && sourceContainsSlash)
                    {
                        continue;
                    }

                    QString relativeFileName(normalizedSource.c_str());

                    QDir rooted(scanFolderInfo->ScanPath());
                    QString absolutePath = rooted.absoluteFilePath(relativeFileName);

                    if (GetFilesFromSourceControl(sources, scanFolderInfo, absolutePath, excludeMetaDataFiles, allowNonDatabaseFiles))
                    {
                        if (foundMatch)
                        {
                            return AZ::Failure(AZStd::string::format("Wildcard query %s matched files in multiple scanfolders.  Files can only be moved from one scanfolder at a time.  Please narrow your query.\nMatch 1: %s\nMatch 2: %s\n",
                                normalizedSource.c_str(),
                                scanFolderInfoOut->ScanPath().toUtf8().constData(),
                                scanFolderInfo->ScanPath().toUtf8().constData()));
                        }

                        foundMatch = true;
                        fileExists = true;
                        scanFolderInfoOut = scanFolderInfo;
                    }

                }
            }
            else
            {
                if (!normalizedSource.ends_with('*') && AZ::IO::FileIOBase::GetInstance()->IsDirectory(normalizedSource.c_str()))
                {
                    return AZ::Failure(AZStd::string("Cannot operate on directories.  Please specify a file or use a wildcard to select all files within a directory.\n"));
                }

                // Absolute path: just look up the scanfolder and convert to a relative path
                AZStd::string pathOnly;
                AzFramework::StringFunc::Path::GetFullPath(normalizedSource.c_str(), pathOnly);

                scanFolderInfoOut = m_platformConfig->GetScanFolderForFile(pathOnly.c_str());

                if (!scanFolderInfoOut)
                {
                    return AZ::Failure(AZStd::string::format("Path %s points to a folder outside the current project's scan folders.\n", pathOnly.c_str()));
                }

                fileExists = GetFilesFromSourceControl(
                    sources, scanFolderInfoOut, normalizedSource.c_str(), excludeMetaDataFiles, allowNonDatabaseFiles);
            }

            if(sources.empty())
            {
                if (fileExists)
                {
                    return AZ::Failure(AZStd::string("Wildcard search matched one or more files but none are source assets.  This utility only handles source assets.\n"));
                }
                else
                {
                    return AZ::Failure(AZStd::string("Wildcard search did not match any files.\n"));
                }
            }
        }
        else // Non-wildcard search
        {
            AZStd::string relativePath;
            auto result = GetScanFolderAndRelativePath(normalizedSource, false, scanFolderInfoOut, relativePath);

            if (!result.IsSuccess())
            {
                return result;
            }

            AZStd::string absoluteSourcePath;
            AzFramework::StringFunc::Path::Join(scanFolderInfoOut->ScanPath().toUtf8().constData(), relativePath.c_str(), absoluteSourcePath);

            if (AZ::IO::FileIOBase::GetInstance()->IsDirectory(absoluteSourcePath.c_str()))
            {
                return AZ::Failure(AZStd::string("Cannot operate on directories.  Please specify a file or use a wildcard to select all files within a directory.\n"));
            }

            fileExists = GetFilesFromSourceControl(
                sources, scanFolderInfoOut, absoluteSourcePath.c_str(), excludeMetaDataFiles, allowNonDatabaseFiles);

            if (sources.empty())
            {
                if (fileExists)
                {
                    return AZ::Failure(AZStd::string("Search matched an existing file but it is not a source asset.  This utility only handles source assets.\n"));
                }
                else
                {
                    return AZ::Failure(AZStd::string("File not found.\n"));
                }
            }
        }

        return AZ::Success();
    }

    void SourceFileRelocator::PopulateDependencies(SourceFileRelocationContainer& relocationContainer) const
    {
        for (auto& relocationInfo : relocationContainer)
        {
            if (relocationInfo.m_isMetadataEnabledType)
            {
                // Metadata enabled files do not use dependency fixup system
                continue;
            }

            m_stateData->QuerySourceDependencyByDependsOnSource(relocationInfo.m_sourceEntry.m_sourceGuid, relocationInfo.m_sourceEntry.m_sourceName.c_str(), relocationInfo.m_oldAbsolutePath.c_str(),
                AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_Any, [&relocationInfo](AzToolsFramework::AssetDatabase::SourceFileDependencyEntry& dependencyEntry)
                {
                    relocationInfo.m_sourceDependencyEntries.push_back(dependencyEntry);
                    relocationInfo.m_hasPathDependencies |= !dependencyEntry.m_fromAssetId;
                    return true;
                });

            m_stateData->QueryProductDependenciesThatDependOnProductBySourceId(relocationInfo.m_sourceEntry.m_sourceID, [this, &relocationInfo](const AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry& entry)
                {
                    AzToolsFramework::AssetDatabase::SourceDatabaseEntry sourceEntry;

                    m_stateData->QuerySourceByProductID(entry.m_productPK, [&sourceEntry](const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry)
                        {
                            sourceEntry = entry;
                            return false;
                        });

                    // Don't count self-referencing product dependencies as there shouldn't be a case where a file has a hardcoded reference to itself.
                    if(sourceEntry.m_sourceID != relocationInfo.m_sourceEntry.m_sourceID)
                    {
                        relocationInfo.m_productDependencyEntries.push_back(entry);
                        relocationInfo.m_hasPathDependencies |= !entry.m_fromAssetId;
                    }

                    return true;
                });
        }
    }

    void SourceFileRelocator::MakePathRelative(const AZStd::string& parentPath, const AZStd::string& childPath, AZStd::string& parentRelative, AZStd::string& childRelative)
    {
        auto parentItr = parentPath.begin();
        auto childItr = childPath.begin();

        while (*parentItr == *childItr && parentItr != parentPath.end() && childItr != childPath.end())
        {
            ++parentItr;
            ++childItr;
        }

        parentRelative = AZStd::string(parentItr, parentPath.end());
        childRelative = AZStd::string(childItr, childPath.end());
    }

    AZ::Outcome<AZStd::string, AZStd::string> SourceFileRelocator::HandleWildcard(AZStd::string_view absFile, AZStd::string_view absSearch, AZStd::string destination)
    {
        AZStd::string searchAsRegex = absSearch;
        AZStd::regex specialCharacters(R"([.?^$+(){}[\]-])");

        // Escape the regex special characters
        searchAsRegex = AZStd::regex_replace(searchAsRegex, specialCharacters, R"(\$0)");
        // Replace * with .*
        searchAsRegex = AZStd::regex_replace(searchAsRegex, AZStd::regex(R"(\*)"), R"((.*))");

        AZStd::smatch result;

        // Match absSearch against absFile to find what each * expands to
        if(AZStd::regex_search(absFile.begin(), absFile.end(), result, AZStd::regex(searchAsRegex, AZStd::regex::icase)))
        {
            // For each * expansion, replace the * in the destination with the expanded result
            for (size_t i = 1; i < result.size(); ++i)
            {
                auto matchedString = result[i].str();

                // Only the last match can match across directory levels
                if(matchedString.find('/') != matchedString.npos && i < result.size() - 1)
                {
                    return AZ::Failure(AZStd::string("Wildcard cannot match across directory levels.  Please simplify your search or put a wildcard at the end of the search to match across directories.\n"));
                }

                destination.replace(destination.find('*'), 1, result[i].str().c_str());
            }
        }

        return AZ::Success(destination);
    }

    void SourceFileRelocator::FixDestinationMissingFilename(AZStd::string& destination, const AZStd::string& source)
    {
        if (destination.ends_with(AZ_CORRECT_DATABASE_SEPARATOR))
        {
            size_t lastSlash = source.find_last_of(AZ_CORRECT_DATABASE_SEPARATOR);

            if (lastSlash == source.npos)
            {
                lastSlash = 0;
            }
            else
            {
                ++lastSlash; // Skip the slash itself
            }

            AZStd::string filename = source.substr(lastSlash);
            destination = destination + filename;
        }
    }

    AZ::Outcome<void, AZStd::string> SourceFileRelocator::ComputeDestination(SourceFileRelocationContainer& relocationContainer, const ScanFolderInfo* sourceScanFolder, const AZStd::string& source, AZStd::string destination, const ScanFolderInfo*& destinationScanFolderOut) const
    {
        if(destination.find("..") != AZStd::string::npos)
        {
            return AZ::Failure(AZStd::string("Destination cannot contain any path navigation.  Please specify an absolute or relative path that does not contain ..\n"));
        }

        FixDestinationMissingFilename(destination, source);

        if(destination.find_first_of("<|>?\"") != destination.npos)
        {
            return AZ::Failure(AZStd::string("Destination string contains invalid characters.\n"));
        }

        if (!AzFramework::StringFunc::Path::IsRelative(destination.c_str()))
        {
            destinationScanFolderOut = m_platformConfig->GetScanFolderForFile(destination.c_str());

            if (!destinationScanFolderOut)
            {
                return AZ::Failure(AZStd::string("Destination must exist within a scanfolder.\n"));
            }
        }

        AZ::s64 sourceWildcardCount = std::count(source.begin(), source.end(), '*');
        AZ::s64 destinationWildcardCount = std::count(destination.begin(), destination.end(), '*');

        if(sourceWildcardCount != destinationWildcardCount)
        {
            return AZ::Failure(AZStd::string("Source and destination paths must have the same number of wildcards.\n"));
        }

        AZStd::string lastError;

        for (SourceFileRelocationInfo& relocationInfo : relocationContainer)
        {
            AZStd::string newDestinationPath;
            // A valid sourceFile Index ( i.e non negative) indicates that it is a metadafile and therefore
            // we would have to determine the destination info from the source file itself.
            if (relocationInfo.m_sourceFileIndex == AssetProcessor::SourceFileRelocationInvalidIndex)
            {
                AZStd::string oldAbsolutePath, selectionSourceAbsolutePath;
                AzFramework::StringFunc::Path::ConstructFull(sourceScanFolder->ScanPath().toUtf8().constData(), relocationInfo.m_oldRelativePath.c_str(), oldAbsolutePath, true);

                if (AzFramework::StringFunc::Path::IsRelative(source.c_str()))
                {
                    AzFramework::StringFunc::Path::Join(sourceScanFolder->ScanPath().toUtf8().constData(), source.c_str(), selectionSourceAbsolutePath, true, false);
                }
                else
                {
                    selectionSourceAbsolutePath = source;
                }

                AZStd::replace(selectionSourceAbsolutePath.begin(), selectionSourceAbsolutePath.end(), AZ_WRONG_DATABASE_SEPARATOR, AZ_CORRECT_DATABASE_SEPARATOR);
                AZStd::replace(oldAbsolutePath.begin(), oldAbsolutePath.end(), AZ_WRONG_DATABASE_SEPARATOR, AZ_CORRECT_DATABASE_SEPARATOR);

                auto result = HandleWildcard(oldAbsolutePath, selectionSourceAbsolutePath, destination);

                if (!result.IsSuccess())
                {
                    lastError = result.TakeError();
                    continue;
                }

                newDestinationPath = AssetUtilities::NormalizeFilePath(result.TakeValue().c_str()).toUtf8().constData();
            }
            else
            {
                const auto& metadataInfo = m_platformConfig->GetMetaDataFileTypeAt(relocationInfo.m_metadataIndex);
                newDestinationPath = relocationContainer[relocationInfo.m_sourceFileIndex].m_newAbsolutePath;

                if (!metadataInfo.second.isEmpty())
                {
                    // Replace extension
                    newDestinationPath = AZ::IO::FixedMaxPath(newDestinationPath).ReplaceExtension(metadataInfo.first.toUtf8().constData()).c_str();
                }
                else
                {
                    // Append extension
                    newDestinationPath += ".";
                    newDestinationPath += metadataInfo.first.toUtf8().constData();
                }
            }

            if (!AzFramework::StringFunc::Path::IsRelative(newDestinationPath.c_str()))
            {
                QString relativePath;
                QString scanFolderName;
                m_platformConfig->ConvertToRelativePath(newDestinationPath.c_str(), relativePath, scanFolderName);

                relocationInfo.m_newAbsolutePath = newDestinationPath;
                relocationInfo.m_newRelativePath = relativePath.toUtf8().constData();
            }
            else
            {
                if (!destinationScanFolderOut)
                {
                    destinationScanFolderOut = sourceScanFolder;
                }

                relocationInfo.m_newRelativePath = newDestinationPath;
                AzFramework::StringFunc::Path::ConstructFull(destinationScanFolderOut->ScanPath().toUtf8().constData(), relocationInfo.m_newRelativePath.c_str(), relocationInfo.m_newAbsolutePath, true);
            }

            relocationInfo.m_newRelativePath = AssetUtilities::NormalizeFilePath(relocationInfo.m_newRelativePath.c_str()).toUtf8().constData();
            relocationInfo.m_newAbsolutePath = AssetUtilities::NormalizeFilePath(relocationInfo.m_newAbsolutePath.c_str()).toUtf8().constData();

            relocationInfo.m_newUuid = AssetUtilities::CreateSafeSourceUUIDFromName(relocationInfo.m_newRelativePath.c_str());
        }

        if(!destinationScanFolderOut)
        {
            return AZ::Failure(lastError);
        }

        return AZ::Success();
    }

    AZStd::string BuildTaskFailureReport(const FileUpdateTasks& updateTasks)
    {
        AZStd::string report;
        AZStd::string skippedReport;

        for (const FileUpdateTask& task : updateTasks)
        {
            if (task.m_skipTask)
            {
                if (skippedReport.empty())
                {
                    skippedReport.append("UPDATE SKIP REPORT:\nThe following files have a dependency on file(s) that failed to move.  These files were not updated:\n");
                }

                skippedReport.append(AZStd::string::format("\t%s\n", task.m_absPathFileToUpdate.c_str()));
            }
            else if (!task.m_succeeded)
            {
                if (report.empty())
                {
                    report.append("UPDATE FAILURE REPORT:\nThe following files have a dependency on file(s) that were moved and failed to be updated automatically.  They will need to be updated manually to fix broken references to moved files:\n");
                }

                report.append(AZStd::string::format("\tFILE: %s\n", task.m_absPathFileToUpdate.c_str()));

                for(int i = 0; i < task.m_oldStrings.size(); ++i)
                {
                    report.append(AZStd::string::format("\t\tPOSSIBLE REFERENCE: %s -> UPDATE TO: %s\n", task.m_oldStrings[i].c_str(), task.m_newStrings[i].c_str()));
                }
            }
        }

        if(!report.empty())
        {
            report.append("\n");
        }

        report.append(skippedReport);

        return report;
    }

    AZStd::string SourceFileRelocator::BuildChangeReport(const SourceFileRelocationContainer& relocationEntries, const FileUpdateTasks& updateTasks) const
    {
        AZStd::string report;

        for (const SourceFileRelocationInfo& relocationInfo : relocationEntries)
        {
            if (!relocationInfo.m_sourceDependencyEntries.empty())
            {
                AZStd::string reportString = AZStd::string::format("%s:\n", "The following files have a source / job dependency on this file, we will attempt to fix the references but they may still break.");
                report.append(reportString);

                for (const auto& sourceDependency : relocationInfo.m_sourceDependencyEntries)
                {
                    AzToolsFramework::AssetDatabase::SourceDatabaseEntry sourceEntry;
                    m_stateData->QuerySourceBySourceGuid(
                        sourceDependency.m_sourceGuid,
                        [&sourceEntry](AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry)
                        {
                            sourceEntry = entry;
                            return false;
                        });

                    report = AZStd::string::format(
                        "%s\nPATH: %s, TYPE: %d, %s\n",
                        report.c_str(),
                        sourceEntry.m_sourceName.c_str(),
                        sourceDependency.m_typeOfDependency,
                        sourceDependency.m_fromAssetId ? "AssetId-based" : "Path-based");
                    AZStd::string fileExtension;
                    AZ::StringFunc::Path::GetExtension(sourceEntry.m_sourceName.c_str(), fileExtension, false);

                    auto found = m_additionalHelpTextMap.find(fileExtension);
                    if (found != m_additionalHelpTextMap.end())
                    {
                        report.append(found->second);
                    }
                }
            }

            if (!relocationInfo.m_productDependencyEntries.empty())
            {
                AZStd::string reportString = AZStd::string::format("%s:\n", "The following files have a product dependency on one or more of the products generated by this file, we will attempt to fix the references but they may still break");
                report.append(reportString);

                for (const auto& productDependency : relocationInfo.m_productDependencyEntries)
                {
                    AzToolsFramework::AssetDatabase::SourceDatabaseEntry sourceEntry;
                    AzToolsFramework::AssetDatabase::ProductDatabaseEntry productEntry;
                    AzToolsFramework::AssetDatabase::ProductDatabaseEntry thisFilesProductEntry;

                    m_stateData->QuerySourceByProductID(productDependency.m_productPK, [&sourceEntry](const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry)
                        {
                            sourceEntry = entry;
                            return false;
                        });

                    m_stateData->QueryProductByProductID(productDependency.m_productPK, [&productEntry](const AzToolsFramework::AssetDatabase::ProductDatabaseEntry& entry)
                        {
                            productEntry = entry;
                            return false;
                        });

                    m_stateData->QueryCombinedBySourceGuidProductSubId(productDependency.m_dependencySourceGuid, productDependency.m_dependencySubID, [&thisFilesProductEntry](const AzToolsFramework::AssetDatabase::CombinedDatabaseEntry& entry)
                        {
                            thisFilesProductEntry = static_cast<AzToolsFramework::AssetDatabase::ProductDatabaseEntry>(entry);
                            return false;
                        }, AZ::Uuid::CreateNull(), nullptr, productDependency.m_platform.c_str());

                    report = AZStd::string::format("%s\nPATH: %s, DEPENDS ON PRODUCT: %s, ASSETID: %s, TYPE: %d, %s\n", report.c_str(), productEntry.m_productName.c_str(), thisFilesProductEntry.m_productName.c_str(), AZ::Data::AssetId(sourceEntry.m_sourceGuid, productEntry.m_subID).ToString<AZStd::string>().c_str(), productDependency.m_dependencyType, productDependency.m_fromAssetId ? "AssetId-based" : "Path-based");
                }
            }
        }

        report.append(BuildTaskFailureReport(updateTasks));

        return report;
    }

    AZStd::string SourceFileRelocator::BuildReport(const SourceFileRelocationContainer& relocationEntries, const FileUpdateTasks& updateTasks, bool isMove, bool updateReference) const
    {
        AZStd::string report;

        report.append("FILE REPORT:\n");

        for (const SourceFileRelocationInfo& relocationInfo : relocationEntries)
        {
            if (relocationInfo.m_metadataIndex != SourceFileRelocationInvalidIndex)
            {
                report.append(AZStd::string::format(
                    "Metadata file CURRENT PATH: %s, NEW PATH: %s\n",
                    relocationInfo.m_oldRelativePath.c_str(),
                    relocationInfo.m_newRelativePath.c_str()));
            }
            else
            {
                if (isMove)
                {
                    report.append(AZStd::string::format(
                        "SOURCEID: %lld, CURRENT PATH: %s, NEW PATH: %s, CURRENT GUID: %s, NEW GUID: %s\n",
                        relocationInfo.m_sourceEntry.m_sourceID,
                        relocationInfo.m_oldRelativePath.c_str(),
                        relocationInfo.m_newRelativePath.c_str(),
                        relocationInfo.m_sourceEntry.m_sourceGuid.ToString<AZStd::string>().c_str(),
                        relocationInfo.m_newUuid.ToString<AZStd::string>().c_str()));
                }
                else
                {
                    report.append(AZStd::string::format(
                        "SOURCEID: %lld, CURRENT PATH: %s, CURRENT GUID: %s\n",
                        relocationInfo.m_sourceEntry.m_sourceID,
                        relocationInfo.m_oldRelativePath.c_str(),
                        relocationInfo.m_sourceEntry.m_sourceGuid.ToString<AZStd::string>().c_str()));
                }
            }

            if (!relocationInfo.m_sourceDependencyEntries.empty())
            {
                AZStd::string reportString = AZStd::string::format("\t%s:\n", updateReference ? " The following files have a source / job dependency on this file, we will attempt to fix the references but they may still break"
                    : "The following files have a source / job dependency on this file and will break");
                report.append(reportString);

                for (const auto& sourceDependency : relocationInfo.m_sourceDependencyEntries)
                {
                    AzToolsFramework::AssetDatabase::SourceDatabaseEntry sourceEntry;
                    m_stateData->QuerySourceBySourceGuid(
                        sourceDependency.m_sourceGuid,
                        [&sourceEntry](AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry)
                        {
                            sourceEntry = entry;
                            return false;
                        });

                    report = AZStd::string::format("%s\t\tUUID: %s, TYPE: %d, %s\n", report.c_str(), sourceEntry.m_sourceName.c_str(), sourceDependency.m_typeOfDependency, sourceDependency.m_fromAssetId ? "AssetId-based" : "Path-based");
                    AZStd::string fileExtension;
                    AZ::StringFunc::Path::GetExtension(sourceEntry.m_sourceName.c_str(), fileExtension, false);

                    auto found = m_additionalHelpTextMap.find(fileExtension);
                    if (found != m_additionalHelpTextMap.end())
                    {
                        report.append(found->second);
                    }
                }
            }

            if (!relocationInfo.m_productDependencyEntries.empty())
            {
                AZStd::string reportString = AZStd::string::format("\t%s:\n", updateReference ? " The following files have a product dependency on one or more of the products generated by this file, we will attempt to fix the references but they may still break"
                    : "The following files have a product dependency on one or more of the products generated by this file and will break");
                report.append(reportString);

                for (const auto& productDependency : relocationInfo.m_productDependencyEntries)
                {
                    AzToolsFramework::AssetDatabase::SourceDatabaseEntry sourceEntry;
                    AzToolsFramework::AssetDatabase::ProductDatabaseEntry productEntry;
                    AzToolsFramework::AssetDatabase::ProductDatabaseEntry thisFilesProductEntry;

                    m_stateData->QuerySourceByProductID(productDependency.m_productPK, [&sourceEntry](const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry)
                        {
                            sourceEntry = entry;
                            return false;
                        });

                    m_stateData->QueryProductByProductID(productDependency.m_productPK, [&productEntry](const AzToolsFramework::AssetDatabase::ProductDatabaseEntry& entry)
                        {
                            productEntry = entry;
                            return false;
                        });

                    m_stateData->QueryCombinedBySourceGuidProductSubId(productDependency.m_dependencySourceGuid, productDependency.m_dependencySubID, [&thisFilesProductEntry](const AzToolsFramework::AssetDatabase::CombinedDatabaseEntry& entry)
                        {
                            thisFilesProductEntry = static_cast<AzToolsFramework::AssetDatabase::ProductDatabaseEntry>(entry);
                            return false;
                        }, AZ::Uuid::CreateNull(), nullptr, productDependency.m_platform.c_str());

                    report = AZStd::string::format("%s\t\tPATH: %s, DEPENDS ON PRODUCT: %s, ASSETID: %s, TYPE: %d, %s\n", report.c_str(), productEntry.m_productName.c_str(), thisFilesProductEntry.m_productName.c_str(), AZ::Data::AssetId(sourceEntry.m_sourceGuid, productEntry.m_subID).ToString<AZStd::string>().c_str(), productDependency.m_dependencyType, productDependency.m_fromAssetId ? "AssetId-based" : "Path-based");
                }
            }
        }

        report.append(BuildTaskFailureReport(updateTasks));

        return report;
    }

    AZ::Outcome<RelocationSuccess, MoveFailure> SourceFileRelocator::Move(const AZStd::string& source, const AZStd::string& destination, int flags )
    {
        AZStd::string normalizedSource = source;
        AZStd::string normalizedDestination = destination;
        bool previewOnly = (flags & RelocationParameters_PreviewOnlyFlag) != 0 ? true : false;
        bool allowDependencyBreaking = (flags & RelocationParameters_AllowDependencyBreakingFlag) != 0 ? true : false;
        bool removeEmptyFolders = (flags & RelocationParameters_RemoveEmptyFoldersFlag) != 0 ? true : false;
        bool updateReferences = (flags & RelocationParameters_UpdateReferencesFlag) != 0 ? true : false;
        bool excludeMetaDataFiles = (flags & RelocationParameters_ExcludeMetaDataFilesFlag) != 0 ? true : false;
        bool allowNonDatabaseFiles = (flags & RelocationParameters_AllowNonDatabaseFilesFlag) != 0 ? true : false;

        // Just make sure we have uniform slashes, don't normalize because we need to keep slashes at the end of the path and wildcards, etc, which tend to get stripped out by normalize functions
        AZStd::replace(normalizedSource.begin(), normalizedSource.end(), AZ_WRONG_DATABASE_SEPARATOR, AZ_CORRECT_DATABASE_SEPARATOR);
        AZStd::replace(normalizedDestination.begin(), normalizedDestination.end(), AZ_WRONG_DATABASE_SEPARATOR, AZ_CORRECT_DATABASE_SEPARATOR);

        SourceFileRelocationContainer relocationContainer;
        const ScanFolderInfo* sourceScanFolderInfo{};
        const ScanFolderInfo* destinationScanFolderInfo{};

        auto result =
            GetSourcesByPath(normalizedSource, relocationContainer, sourceScanFolderInfo, excludeMetaDataFiles, allowNonDatabaseFiles);

        if (!result.IsSuccess())
        {
            return AZ::Failure(MoveFailure(result.TakeError(), false));
        }

        // If no files were found, just early out
        if(relocationContainer.empty())
        {
            return AZ::Success(RelocationSuccess());
        }

        PopulateDependencies(relocationContainer);
        result = ComputeDestination(relocationContainer, sourceScanFolderInfo, normalizedSource, normalizedDestination, destinationScanFolderInfo);

        if(!result.IsSuccess())
        {
            return AZ::Failure(MoveFailure(result.TakeError(), false));
        }

        int errorCount = 0;
        FileUpdateTasks updateTasks;

        if(!previewOnly)
        {
            if(!updateReferences && !allowDependencyBreaking)
            {
                for (const SourceFileRelocationInfo& relocationInfo : relocationContainer)
                {
                    if(!relocationInfo.m_productDependencyEntries.empty() || !relocationInfo.m_sourceDependencyEntries.empty())
                    {
                        return AZ::Failure(MoveFailure(AZStd::string("Move failed.  There are files that have dependencies that may break as a result of being moved/renamed.\n"), true));
                    }
                }
            }

            bool sourceControlEnabled = false;
            AzToolsFramework::SourceControlConnectionRequestBus::BroadcastResult(sourceControlEnabled, &AzToolsFramework::SourceControlConnectionRequestBus::Events::IsActive);

            errorCount = DoSourceControlMoveFiles(normalizedSource, normalizedDestination, relocationContainer, sourceScanFolderInfo, destinationScanFolderInfo, removeEmptyFolders);

            if (updateReferences)
            {
                updateTasks = UpdateReferences(relocationContainer, sourceControlEnabled);
            }
        }

        int relocationCount = aznumeric_caster(relocationContainer.size());
        int updateTotalCount = aznumeric_caster(updateTasks.size());
        int updateSuccessCount = 0;

        for (const auto& task : updateTasks)
        {
            if (task.m_succeeded)
            {
                ++updateSuccessCount;
            }
        }

        int updateFailureCount = updateTotalCount - updateSuccessCount;

        return AZ::Success(RelocationSuccess(
            relocationCount - errorCount, errorCount, relocationCount,
            updateSuccessCount, updateFailureCount, updateTotalCount,
            AZStd::move(relocationContainer),
            AZStd::move(updateTasks)));
    }

    AZ::Outcome<RelocationSuccess, AZStd::string> SourceFileRelocator::Delete(const AZStd::string& source, int flags)
    {
        bool previewOnly = (flags & RelocationParameters_PreviewOnlyFlag) != 0 ? true : false;
        bool allowDependencyBreaking = (flags & RelocationParameters_AllowDependencyBreakingFlag) != 0 ? true : false;
        bool removeEmptyFolders = (flags & RelocationParameters_RemoveEmptyFoldersFlag) != 0 ? true : false;
        bool excludeMetaDataFiles = (flags & RelocationParameters_ExcludeMetaDataFilesFlag) != 0 ? true : false;
        bool allowNonDatabaseFiles = (flags & RelocationParameters_AllowNonDatabaseFilesFlag) != 0 ? true : false;
        AZStd::string normalizedSource = AssetUtilities::NormalizeFilePath(source.c_str()).toUtf8().constData();

        SourceFileRelocationContainer relocationContainer;
        const ScanFolderInfo* scanFolderInfo{};

        auto result = GetSourcesByPath(normalizedSource, relocationContainer, scanFolderInfo, excludeMetaDataFiles, allowNonDatabaseFiles);

        if (!result.IsSuccess())
        {
            return AZ::Failure(result.TakeError());
        }

        // If no files were found, just early out
        if (relocationContainer.empty())
        {
            return AZ::Success(RelocationSuccess());
        }

        PopulateDependencies(relocationContainer);

        int errorCount = 0;

        if (!previewOnly)
        {
            if (!allowDependencyBreaking)
            {
                for (const SourceFileRelocationInfo& relocationInfo : relocationContainer)
                {
                    if (!relocationInfo.m_productDependencyEntries.empty() || !relocationInfo.m_sourceDependencyEntries.empty())
                    {
                        return AZ::Failure(AZStd::string("Delete failed.  There are files that have dependencies that may break as a result of being deleted.\n"));
                    }
                }
            }

            errorCount = DoSourceControlDeleteFiles(normalizedSource, relocationContainer, scanFolderInfo, removeEmptyFolders);
        }

        int relocationCount = aznumeric_caster(relocationContainer.size());
        return AZ::Success(RelocationSuccess(
            relocationCount - errorCount, errorCount, relocationCount,
            0, 0, 0,
            AZStd::move(relocationContainer),
            {}));
    }

    void RemoveEmptyFolders(const SourceFileRelocationContainer& relocationContainer)
    {
        for (const SourceFileRelocationInfo& info : relocationContainer)
        {
            AZStd::string oldParentFolder;
            AzFramework::StringFunc::Path::GetFullPath(info.m_oldAbsolutePath.c_str(), oldParentFolder);

            // Not checking the return value since non-empty folders will fail, we just want to delete empty folders.
            AZ::IO::SystemFile::DeleteDir(oldParentFolder.c_str());
        }
    }

    void HandleSourceControlResult(SourceFileRelocationContainer& relocationContainer, AZStd::binary_semaphore& waitSignal, int& errorCount,
        AzToolsFramework::SourceControlFlags checkFlag, bool checkNewPath, bool /*success*/, AZStd::vector<AzToolsFramework::SourceControlFileInfo> info)
    {
        for (SourceFileRelocationInfo& entry : relocationContainer)
        {
            bool found = false;
            bool readOnly = false;

            for (const AzToolsFramework::SourceControlFileInfo& scInfo : info)
            {
                const char* checkPath = checkNewPath ? entry.m_newAbsolutePath.c_str() : entry.m_oldAbsolutePath.c_str();

                if (AssetUtilities::NormalizeFilePath(scInfo.m_filePath.c_str()) == AssetUtilities::NormalizeFilePath(checkPath))
                {
                    found = true;
                    readOnly = !scInfo.HasFlag(AzToolsFramework::SourceControlFlags::SCF_Writeable);
                    entry.m_operationStatus = (scInfo.m_status == AzToolsFramework::SCS_OpSuccess && scInfo.HasFlag(checkFlag)) ?
                        SourceFileRelocationStatus::Succeeded : SourceFileRelocationStatus::Failed;
                    break;
                }
            }

            if (!found && entry.m_sourceFileIndex != AssetProcessor::SourceFileRelocationInvalidIndex)
            {
                // if we are here it implies that it is an meta data file.
                // it is very much possible that the source control won't be able to find all the meta data files
                // from the source search path and therefore we will handle them separately.
                continue;
            }

            if(entry.m_operationStatus == SourceFileRelocationStatus::Failed)
            {
                ++errorCount;

                if (!found)
                {
                    AZ_Printf("SourceFileRelocator", "Error: file is not tracked by source control %s\n", entry.m_oldAbsolutePath.c_str());
                }
                else
                {
                    AZ_Printf("SourceFileRelocator", "Error: operation failed for file %s.  Note: File is %s.\n", entry.m_oldAbsolutePath.c_str(), readOnly ? "read-only" : "writable (this is not the source of the error)");
                }
            }
        }

        waitSignal.release();
    }

    AZStd::string ToAbsolutePath(AZStd::string& normalizedPath, const ScanFolderInfo* scanFolderInfo)
    {
        AZStd::string absolutePath;

        if (AzFramework::StringFunc::Path::IsRelative(normalizedPath.c_str()))
        {
            AzFramework::StringFunc::AssetDatabasePath::Join(scanFolderInfo->ScanPath().toUtf8().constData(), normalizedPath.c_str(), absolutePath, true, false);
        }
        else
        {
            absolutePath = normalizedPath;
        }

        return absolutePath;
    }

    int SourceFileRelocator::DoSourceControlMoveFiles(AZStd::string normalizedSource, AZStd::string normalizedDestination, SourceFileRelocationContainer& relocationContainer, const ScanFolderInfo* sourceScanFolderInfo, const ScanFolderInfo* destinationScanFolderInfo, bool removeEmptyFolders) const
    {
        using namespace AzToolsFramework;

        AZ_Assert(sourceScanFolderInfo, "sourceScanFolderInfo cannot be null");
        AZ_Assert(destinationScanFolderInfo, "destinationScanFolderInfo cannot be null");

        for(const auto& relocationInfo : relocationContainer)
        {
            const char* oldPath = relocationInfo.m_oldAbsolutePath.c_str();
            const char* newPath = relocationInfo.m_newAbsolutePath.c_str();

            if(AZ::StringFunc::Equal(newPath, oldPath, true))
            {
                // It's not really an error to rename a file to the same thing.  This can happen (unintentionally) with wildcard renames
                continue;
            }

            if(AZ::StringFunc::Equal(newPath, oldPath, false))
            {
                AZ_Printf("SourceFileRelocator", "Error: Changing the case of a filename is not supported due to potential source control restrictions.  OldPath: %s, NewPath: %s\n", oldPath, newPath);
                return 1;
            }

            bool newExists = AZ::IO::SystemFile::Exists(newPath);

            if (newExists)
            {
                AZ_Printf("SourceFileRelocator", "Warning: Destination file %s already exists, rename will fail\n", newPath);
            }
        }

        FixDestinationMissingFilename(normalizedDestination, normalizedSource);
        AdjustWildcardForPerforce(normalizedSource, normalizedDestination);

        AZStd::string absoluteSource = ToAbsolutePath(normalizedSource, sourceScanFolderInfo);
        AZStd::string absoluteDestination = ToAbsolutePath(normalizedDestination, destinationScanFolderInfo);

        AZ_Printf("SourceFileRelocator", "From: %s, To: %s\n", absoluteSource.c_str(), absoluteDestination.c_str());

        AZStd::binary_semaphore waitSignal;
        int errorCount = 0;

        AzToolsFramework::SourceControlResponseCallbackBulk callback = [&](bool success, AZStd::vector<SourceControlFileInfo> info)
            {
                HandleSourceControlResult(
                    relocationContainer, waitSignal, errorCount,
                    SCF_OpenByUser, // If a file is moved from A -> B and then again from B -> A, the result is just an "edit", so we're just going
                                         // to assume success if the file is checked out, regardless of state
                    true, success, info);
            };


        AzToolsFramework::SourceControlCommandBus::Broadcast(&AzToolsFramework::SourceControlCommandBus::Events::RequestRenameBulkExtended,
            absoluteSource.c_str(), absoluteDestination.c_str(), true, callback);

        if(!WaitForSourceControl(waitSignal))
        {
            return errorCount + 1;
        }

        // Source control rename operation by source path might not result in moving metadata files,
        // therefore we will have to move all those metadata files individually whose associated source files got renamed.
        for (const auto& relocationInfo : relocationContainer)
        {
            if (relocationInfo.m_operationStatus == SourceFileRelocationStatus::Succeeded || relocationInfo.m_sourceFileIndex == AssetProcessor::SourceFileRelocationInvalidIndex)
            {
                // we do not want to retry if the move operation already succeeded or if it is a source file
                continue;
            }

            AzToolsFramework::SourceControlCommandBus::Broadcast(&AzToolsFramework::SourceControlCommandBus::Events::RequestRenameBulkExtended,
                relocationInfo.m_oldAbsolutePath.c_str(), relocationInfo.m_newAbsolutePath.c_str(), true, callback);
            if (!WaitForSourceControl(waitSignal))
            {
                return errorCount + 1;
            }
        }

        if (removeEmptyFolders)
        {
            RemoveEmptyFolders(relocationContainer);
        }

        return errorCount;
    }

    int SourceFileRelocator::DoSourceControlDeleteFiles(AZStd::string normalizedSource, SourceFileRelocationContainer& relocationContainer, const ScanFolderInfo* sourceScanFolderInfo, bool removeEmptyFolders) const
    {
        using namespace AzToolsFramework;

        AdjustWildcardForPerforce(normalizedSource);
        AZStd::string absoluteSource = ToAbsolutePath(normalizedSource, sourceScanFolderInfo);

        AZ_Printf("SourceFileRelocator", "Delete %s\n", absoluteSource.c_str());

        bool sourceControlEnabled = false;
        AzToolsFramework::SourceControlConnectionRequestBus::BroadcastResult(sourceControlEnabled, &AzToolsFramework::SourceControlConnectionRequestBus::Events::IsActive);

        SourceControlFlags checkFlag = SCF_PendingDelete;

        if(!sourceControlEnabled)
        {
            // When using the local SC component, the only flag set is the writable flag when a file is deleted
            checkFlag = SCF_Writeable;
        }

        AZStd::binary_semaphore waitSignal;
        int errorCount = 0;

        AzToolsFramework::SourceControlResponseCallbackBulk callback = AZStd::bind(&HandleSourceControlResult, AZStd::ref(relocationContainer), AZStd::ref(waitSignal), AZStd::ref(errorCount),
            checkFlag, false, AZStd::placeholders::_1, AZStd::placeholders::_2);

        AzToolsFramework::SourceControlCommandBus::Broadcast(&AzToolsFramework::SourceControlCommandBus::Events::RequestDeleteBulkExtended, absoluteSource.c_str(), true, callback);

        if(!WaitForSourceControl(waitSignal))
        {
            return errorCount + 1;
        }

        // Source control delete operation by source path might not result in deleting metadata files,
        // therefore we will have to delete all those metadata files individually whose associated source files got removed.
        for (auto& entry : relocationContainer)
        {
            if (entry.m_operationStatus == SourceFileRelocationStatus::Succeeded || entry.m_sourceFileIndex == AssetProcessor::SourceFileRelocationInvalidIndex)
            {
                // we do not want to retry if the move operation already succeeded or if it is a source file
                continue;
            }

            AzToolsFramework::SourceControlCommandBus::Broadcast(&AzToolsFramework::SourceControlCommandBus::Events::RequestDeleteBulkExtended, entry.m_oldAbsolutePath.c_str(), true, callback);
            if (!WaitForSourceControl(waitSignal))
            {
                return errorCount + 1;
            }
        }

        if (!sourceControlEnabled)
        {
            // Do an extra check to make sure the files were deleted since the flags provided aren't very informative
            for(auto& relocationInfo : relocationContainer)
            {
                if(relocationInfo.m_operationStatus == SourceFileRelocationStatus::Succeeded && AZ::IO::SystemFile::Exists(relocationInfo.m_oldAbsolutePath.c_str()))
                {
                    relocationInfo.m_operationStatus = SourceFileRelocationStatus::Failed;
                    ++errorCount;
                }
            }
        }

        if (removeEmptyFolders)
        {
            RemoveEmptyFolders(relocationContainer);
        }

        return errorCount;
    }

    AZStd::string FileToString(const char* fullPath)
    {
        AZ::IO::FileIOStream fileStream(fullPath, AZ::IO::OpenMode::ModeRead);

        if (!fileStream.IsOpen())
        {
            AZ_Error("SourceFileRelocator", false, "Failed to open file for read %s", fullPath);
            return {};
        }

        AZ::IO::SizeType length = fileStream.GetLength();

        if (length == 0)
        {
            return {};
        }

        AZStd::vector<char> charBuffer;
        charBuffer.resize_no_construct(length);
        fileStream.Read(length, charBuffer.data());

        return AZStd::string(charBuffer.data(), charBuffer.data() + length);
    }

    void StringToFile(const char* fullPath, const AZStd::string& str)
    {
        AZ::IO::FileIOStream fileStream(fullPath, AZ::IO::OpenMode::ModeWrite);

        if(!fileStream.IsOpen())
        {
            AZ_Error("SourceFileRelocator", false, "Failed to open file for write %s", fullPath);
            return;
        }

        fileStream.Write(str.size(), str.data());
    }

    bool ReplaceAll(AZStd::string& str, const AZStd::string& oldStr, const AZStd::string& newStr)
    {
        return AzFramework::StringFunc::Replace(str, oldStr.c_str(), newStr.c_str());
    }

    bool SourceFileRelocator::UpdateFileReferences(const FileUpdateTask& updateTask)
    {
        if(updateTask.m_skipTask)
        {
            return false;
        }

        const char* fullPath = updateTask.m_absPathFileToUpdate.c_str();
        AZStd::string fileAsString = FileToString(fullPath);

        if(fileAsString.empty())
        {
            return false;
        }

        bool didReplace = false;

        for (int i = 0; i < updateTask.m_oldStrings.size(); ++i)
        {
            didReplace |= ReplaceAll(fileAsString, updateTask.m_oldStrings[i], updateTask.m_newStrings[i]);
        }

        if (didReplace)
        {
            StringToFile(fullPath, fileAsString);
        }

        AZ_TracePrintf("SourceFileRelocator", "Updated %s - %s\n", fullPath, didReplace ? "SUCCESS" : "FAIL");

        return didReplace;
    }

    bool SourceFileRelocator::ComputeProductDependencyUpdatePaths(const SourceFileRelocationInfo& relocationInfo, const AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry& productDependency,
        AZStd::vector<AZStd::string>& oldPaths, AZStd::vector<AZStd::string>& newPaths, AZStd::string& absPathFileToUpdate) const
    {
        AZStd::string sourceName;
        AZStd::string scanPath;

        // Look up the source file and scanfolder of the product (productPK) that references this file
        m_stateData->QuerySourceByProductID(productDependency.m_productPK, [this, &sourceName, &scanPath](AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry)
        {
            sourceName = entry.m_sourceName;

            m_stateData->QueryScanFolderByScanFolderID(entry.m_scanFolderPK, [&scanPath](AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry& entry)
            {
                scanPath = entry.m_scanFolder;
                return false;
            });

            return false;
        });

        // Find the product this dependency refers to
        auto iterator = relocationInfo.m_products.find(productDependency.m_dependencySubID);

        if(iterator == relocationInfo.m_products.end())
        {
            AZ_Warning("SourceFileRelocator", false, "Can't automatically update references to product, failed to find product with subId %d in product list for file %s", productDependency.m_dependencySubID, relocationInfo.m_oldAbsolutePath.c_str());
            return false;
        }

        // See if the product filename and source file name are the same (not including extension), if so, we can proceed.
        // The names need to be the same because if the source is renamed, we have no way of knowing how that will affect the product name.
        const AZStd::string& productName = iterator->second.m_productName;
        AZStd::string productFileName, sourceFileName;

        AzFramework::StringFunc::Path::GetFileName(productName.c_str(), productFileName);
        AzFramework::StringFunc::Path::GetFileName(relocationInfo.m_oldAbsolutePath.c_str(), sourceFileName);

        if(!AzFramework::StringFunc::Equal(sourceFileName.c_str(), productFileName.c_str(), false))
        {
            AZ_Warning("SourceFileRelocator", false, "Can't automatically update references to product because product name (%s) is different from source name (%s)", productFileName.c_str(), sourceFileName.c_str());
            return false;
        }

        // The names are the same, so just take the source path and replace the extension
        // We're computing the old path as well because the productName includes the platform and gamename which shouldn't be included in hardcoded references
        AZStd::string productExtension;
        AZStd::string oldProductPath = relocationInfo.m_oldRelativePath;
        AZStd::string newProductPath = relocationInfo.m_newRelativePath;

        // Product paths should be all lowercase
        AZStd::to_lower(oldProductPath.begin(), oldProductPath.end());
        AZStd::to_lower(newProductPath.begin(), newProductPath.end());

        AzFramework::StringFunc::Path::GetExtension(productName.c_str(), productExtension);
        AzFramework::StringFunc::Path::ReplaceExtension(oldProductPath, productExtension.c_str());
        AzFramework::StringFunc::Path::ReplaceExtension(newProductPath, productExtension.c_str());

        // This is the full path to the file we need to fix up
        AzFramework::StringFunc::Path::ConstructFull(scanPath.c_str(), sourceName.c_str(), absPathFileToUpdate);
        absPathFileToUpdate = AssetUtilities::NormalizeFilePath(absPathFileToUpdate.c_str()).toUtf8().constData();

        oldPaths.push_back(oldProductPath);
        oldPaths.push_back(relocationInfo.m_oldRelativePath); // If we fail to find a reference to the product, we'll try to find a reference to the source

        newPaths.push_back(newProductPath);
        newPaths.push_back(relocationInfo.m_newRelativePath);

        return true;
    }

    FileUpdateTasks SourceFileRelocator::UpdateReferences(const SourceFileRelocationContainer& relocationContainer,  bool useSourceControl) const
    {
        FileUpdateTasks updateTasks;
        AZStd::unordered_set<AZStd::string> filesToEdit;
        AZStd::unordered_map<AZStd::string, AZStd::string> movedFileMap;

        // Make a record of all moved files.  We might need to edit some of them and if they've already moved,
        // we need to make sure we edit the files at the new location
        for(const SourceFileRelocationInfo& relocationInfo : relocationContainer)
        {
            if (relocationInfo.m_operationStatus == SourceFileRelocationStatus::Succeeded)
            {
                movedFileMap.insert(AZStd::make_pair(
                    AssetUtilities::NormalizeFilePath(relocationInfo.m_oldAbsolutePath.c_str()).toUtf8().constData(),
                    AssetUtilities::NormalizeFilePath(relocationInfo.m_newAbsolutePath.c_str()).toUtf8().constData()));
            }
        }

        auto pathFixupFunc = [&movedFileMap](const char* filePath) -> AZStd::string
        {
            auto iterator = movedFileMap.find(AssetUtilities::NormalizeFilePath(filePath).toUtf8().constData());

            if (iterator != movedFileMap.end())
            {
                return iterator->second;
            }

            return filePath;
        };

        // Gather a list of all the files to edit and the edits that need to be made
        for(const SourceFileRelocationInfo& relocationInfo : relocationContainer)
        {
            bool skipTask = relocationInfo.m_operationStatus == SourceFileRelocationStatus::Failed;

            for (const auto& sourceDependency : relocationInfo.m_sourceDependencyEntries)
            {
                AzToolsFramework::AssetDatabase::SourceDatabaseEntry sourceEntry;
                m_stateData->QuerySourceBySourceGuid(
                    sourceDependency.m_sourceGuid,
                    [&sourceEntry](AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry)
                    {
                        sourceEntry = entry;
                        return false;
                    });

                AZStd::string fullPath = m_platformConfig->FindFirstMatchingFile(sourceEntry.m_sourceName.c_str()).toUtf8().constData();

                fullPath = pathFixupFunc(fullPath.c_str());

                updateTasks.emplace(AZStd::vector<AZStd::string>{relocationInfo.m_sourceEntry.m_sourceGuid.ToString<AZStd::string>(), relocationInfo.m_oldRelativePath},
                    AZStd::vector<AZStd::string>{relocationInfo.m_newUuid.ToString<AZStd::string>(), relocationInfo.m_newRelativePath}, fullPath, sourceDependency.m_fromAssetId, skipTask);
                filesToEdit.insert(AZStd::move(fullPath));
            }

            for (const auto& productDependency : relocationInfo.m_productDependencyEntries)
            {
                AZStd::string fullPath;
                AZStd::vector<AZStd::string> oldPaths, newPaths;

                if (ComputeProductDependencyUpdatePaths(relocationInfo, productDependency, oldPaths, newPaths, fullPath))
                {
                    fullPath = pathFixupFunc(fullPath.c_str());

                    oldPaths.push_back(relocationInfo.m_sourceEntry.m_sourceGuid.ToString<AZStd::string>());
                    newPaths.push_back(relocationInfo.m_newUuid.ToString<AZStd::string>());

                    updateTasks.emplace(oldPaths, newPaths, fullPath, productDependency.m_fromAssetId, skipTask);
                    filesToEdit.insert(AZStd::move(fullPath));
                }
            }
        }

        // No work to do? Early out
        if(filesToEdit.empty())
        {
            return {};
        }

        if (useSourceControl)
        {
            // Mark all the files for edit
            AZStd::binary_semaphore waitSignal;

            AzToolsFramework::SourceControlResponseCallbackBulk callback = [&waitSignal](bool /*success*/, const AZStd::vector<AzToolsFramework::SourceControlFileInfo>& /*info*/)
            {
                waitSignal.release();
            };

            AzToolsFramework::SourceControlCommandBus::Broadcast(&AzToolsFramework::SourceControlCommandBus::Events::RequestEditBulk, filesToEdit, true, callback);

            // Wait for the edit command to finish before trying to actually edit the files
            if(!WaitForSourceControl(waitSignal))
            {
                return updateTasks;
            }
        }

        // Update all the files
        for (FileUpdateTask& updateTask : updateTasks)
        {
            updateTask.m_succeeded = UpdateFileReferences(updateTask);
        }

        return updateTasks;
    }
} // namespace AssetProcessor
