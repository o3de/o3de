/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/Asset/AssetBundleManifest.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/AssetBundle/AssetBundleComponent.h>
#include <AzToolsFramework/Archive/ArchiveAPI.h>
#include <AzToolsFramework/Asset/AssetSeedManager.h>
#include <AzToolsFramework/AssetCatalog/PlatformAddressedAssetCatalog.h>
#include <AzToolsFramework/AssetCatalog/PlatformAddressedAssetCatalogBus.h>


namespace AzToolsFramework 
{
    const char* logWindowName = "AssetBundle";
    const char tempBundleFileSuffix[] = "_temp";
    const int NumOfBytesInMB = 1024 * 1024;
    const int ManifestFileSizeBufferInBytes = 10 * 1024; // 10 KB
    const float AssetCatalogFileSizeBufferPercentage = 1.0f;
    using AssetCatalogRequestBus = AZ::Data::AssetCatalogRequestBus;

    const char AssetBundleComponent::DeltaCatalogName[] = "DeltaCatalog.xml";

    constexpr int InjectFileRetryCount = 4;


    bool MaxSizeExceeded(AZ::u64 totalFileSize, AZ::u64 bundleSize, AZ::u64 assetCatalogFileSizeBuffer, AZ::u64 maxSizeInBytes)
    {
        return ((totalFileSize + bundleSize + assetCatalogFileSizeBuffer + ManifestFileSizeBufferInBytes) > maxSizeInBytes);
    }

    bool MakePath(const AZStd::string& directory)
    {
        if (!AZ::IO::FileIOBase::GetInstance()->Exists(directory.c_str()))
        {
            auto result = AZ::IO::FileIOBase::GetInstance()->CreatePath(directory.c_str());
            if (!result)
            {
                AZ_Error(logWindowName, false, "Path creation failed. Input path: %s \n", directory.c_str());
                return false;
            }
        }
        return true;
    }

    //! This helper class can be used to create a temp folder from a filename.
    //! It strips the extension and than adds _temp token to the name and tries to create that directory on disk.
    struct TemporaryDir
    {
        explicit TemporaryDir(AZStd::string filePath)
        {
            AzFramework::StringFunc::Path::StripExtension(filePath);
            filePath += "_temp";
            if (MakePath(filePath))
            {
                m_tempFolderPath = filePath;
                m_result = true;
            }
        }

        AZ_DISABLE_COPY_MOVE(TemporaryDir)

        ~TemporaryDir()
        {
            if (m_result)
            {
                AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
                fileIO->DestroyPath(m_tempFolderPath.c_str());
            }
        }

        AZStd::string m_tempFolderPath;
        bool m_result = false;
    };

    
    void AssetBundleComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AssetBundleComponent, AZ::Component>();
        }
    }

    void AssetBundleComponent::Activate()
    {
        AssetBundleCommands::Bus::Handler::BusConnect();
    }

    void AssetBundleComponent::Deactivate()
    {
        AssetBundleCommands::Bus::Handler::BusDisconnect();
    }

    bool AssetBundleComponent::CreateDeltaCatalog(const AZStd::string& sourcePak, bool regenerate)
    {
        AZStd::string normalizedSourcePakPath = sourcePak;
        AzFramework::StringFunc::Path::Normalize(normalizedSourcePakPath);

        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        if (!fileIO)
        {
            AZ_Error("AssetBundle", false, "No FileIOBase Instance");
            return false;
        }
        if (!fileIO->Exists(normalizedSourcePakPath.c_str()))
        {
            AZ_Error("AssetBundle", false, "Invalid Arg: Source Pak does not exist at \"%s\".", normalizedSourcePakPath.c_str());
            return false;
        }

        AZStd::string outCatalogPath;
        // Scoping the fixed string to the if block to prevent the 1024 byte stack object from materializing longer than needed in this function
        if(char resolvedPath[AZ_MAX_PATH_LEN]; fileIO->ResolvePath(DeltaCatalogName, resolvedPath, AZ_ARRAY_SIZE(resolvedPath)))
        {
            outCatalogPath = resolvedPath;
        }

        AZ_TracePrintf(logWindowName, "Gathering file entries in source pak file \"%s\".\n", sourcePak.c_str());
        bool result = false;
        AZStd::vector<AZStd::string> fileEntries;
        ArchiveCommandsBus::BroadcastResult(result, &AzToolsFramework::ArchiveCommandsBus::Events::ListFilesInArchive, normalizedSourcePakPath, fileEntries);
        // This ebus currently always returns false as the result, as it is believed that the 7z process is 
        //  being terminated by the user instead of ending gracefully. Check against an empty fileList instead
        //  as a result.
        if (fileEntries.empty())
        {
            AZ_Error(logWindowName, false, "Failed to read archive \"%s\" for file entries.", sourcePak.c_str());
            return false;
        }

        // if this bundle already contains a manifest and should not regenerate it, then this bundle is good to go.
        bool manifestExists = HasManifest(fileEntries);
        if (manifestExists && !regenerate)
        {
            AZ_TracePrintf(logWindowName, "Skipping delta asset catalog creation for \"%s\", as it already contains a delta catalog.\n", normalizedSourcePakPath.c_str());
            return true;
        }

        // Load the manifest if it exists, and set the output path to the catalog in the manifest
        AzFramework::AssetBundleManifest* manifest = nullptr;
        if (manifestExists)
        {
            manifest = GetManifestFromBundle(normalizedSourcePakPath);
            if (!manifest)
            {
                return false;
            }
            outCatalogPath = manifest->GetCatalogName();
        }

        // remove non-asset entries from this file list (including the source pak itself)
        AZ_TracePrintf(logWindowName, "Removing known non-assets from the gathered list of file entries.\n");        
        if (!RemoveNonAssetFileEntries(fileEntries, normalizedSourcePakPath, manifest))
        {
            return false;
        }

        // create the new asset registry containing these files and save the catalog to a file here.
        AZ_TracePrintf(logWindowName, "Creating new asset catalog file \"%s\" containing information for assets in source pak.\n", outCatalogPath.c_str());
        bool catalogSaved = false;
        AssetCatalogRequestBus::BroadcastResult(catalogSaved, &AssetCatalogRequestBus::Events::CreateDeltaCatalog, fileEntries, outCatalogPath);
        if (!catalogSaved)
        {
            AZ_Error(logWindowName, false, "Failed to create new asset catalog \"%s\".", outCatalogPath.c_str());
            return false;
        }

        if (!InjectFile(outCatalogPath, normalizedSourcePakPath))
        {
            return false;
        }        

        // clean up the file that was created.
        if (!fileIO->Remove(outCatalogPath.c_str()))
        {
            AZ_Warning(logWindowName, false, "Failed to clean up catalog artifact at %s.", outCatalogPath.c_str());
        }

        // if the manifest already exists, we don't need to modify it, so this bundle is set.
        if (manifest)
        {
            return true;
        }

        // create the new bundle manifest and save it to a file here.
        AZ_TracePrintf(logWindowName, "Creating new asset bundle manifest file \"%s\" for source pak \"%s\".\n", AzFramework::AssetBundleManifest::s_manifestFileName, sourcePak.c_str());
        bool manifestSaved = false;
        AZStd::string manifestDirectory;
        AzFramework::StringFunc::Path::GetFullPath(sourcePak.c_str(), manifestDirectory);
        AssetCatalogRequestBus::BroadcastResult(manifestSaved, &AssetCatalogRequestBus::Events::CreateBundleManifest, outCatalogPath,
            AZStd::vector<AZStd::string>(), manifestDirectory, AzFramework::AssetBundleManifest::CurrentBundleVersion, AZStd::vector<AZ::IO::Path>{});

        AZStd::string manifestPath;
        AzFramework::StringFunc::Path::Join(manifestDirectory.c_str(), AzFramework::AssetBundleManifest::s_manifestFileName, manifestPath);
        if (!manifestSaved)
        {
            AZ_Error(logWindowName, false, "Failed to create new manifest file \"%s\" for source pak \"%s\".\n", manifestPath.c_str(), sourcePak.c_str());
            return false;
        }

        if (!InjectFile(manifestPath, sourcePak))
        {
            return false;
        }

        // clean up the file that was created.
        if (!fileIO->Remove(manifestPath.c_str()))
        {
            AZ_Warning(logWindowName, false, "Failed to clean up manifest artifact.");
        }

        return true;
    }

    AZStd::string AssetBundleComponent::CreateAssetBundleFileName(const AZStd::string& assetBundleFilePath, int bundleIndex)
    {
        AZStd::string fileName;
        AZStd::string extension;
        AzFramework::StringFunc::AssetDatabasePath::Split(assetBundleFilePath.c_str(), nullptr, nullptr, nullptr, &fileName, &extension);
        if (!bundleIndex)
        {
            return  AZStd::string::format("%s%s", fileName.c_str(), extension.c_str());
        }
        return AZStd::string::format("%s__%d%s", fileName.c_str(), bundleIndex, extension.c_str());
    }

    bool AssetBundleComponent::CreateAssetBundleFromList(const AssetBundleSettings& assetBundleSettings, const AssetFileInfoList& assetFileInfoList)
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        AZ_Assert(fileIO != nullptr, "AZ::IO::FileIOBase must be ready for use.\n");

        AZ::IO::Path bundleFilePath = AZ::IO::Path(AZStd::string_view{ AZ::Utils::GetEnginePath() }) / assetBundleSettings.m_bundleFilePath;

        AzFramework::PlatformId platformId = static_cast<AzFramework::PlatformId>(AzFramework::PlatformHelper::GetPlatformIndexFromName(assetBundleSettings.m_platform.c_str()));

        if (platformId == AzFramework::PlatformId::Invalid)
        {
            return false;
        }

        AZ::u64 maxSizeInBytes = static_cast<AZ::u64>(assetBundleSettings.m_maxBundleSizeInMB * NumOfBytesInMB);
        AZ::u64 assetCatalogFileSizeBuffer = static_cast<AZ::u64>(AssetCatalogFileSizeBufferPercentage * assetBundleSettings.m_maxBundleSizeInMB * NumOfBytesInMB) / 100;
        AZ::u64 bundleSize = 0;
        AZ::u64 totalFileSize = 0;
        int bundleIndex = 0;

        AZStd::string tempBundleFilePath = bundleFilePath.Native() + "_temp";

        AZStd::vector<AZStd::string> dependentBundleNames;
        AZStd::vector<AZ::IO::Path> levelDirs;
        AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>> bundlePathDeltaCatalogPair;
        bundlePathDeltaCatalogPair.emplace_back(tempBundleFilePath, DeltaCatalogName);

        AZStd::vector<AZStd::string> fileEntries; // this is used to add files to the archive
        AZStd::vector<AZStd::string> deltaCatalogEntries; // this is used to create the delta catalog

        AZStd::string bundleFolder;
        AzFramework::StringFunc::Path::GetFullPath(bundleFilePath.c_str(), bundleFolder);

        // Create the archive directory if it does not exist
        if (!MakePath(bundleFolder))
        {
            return  false;
        }

        AZStd::string deltaCatalogFilePath;
        AzFramework::StringFunc::Path::ConstructFull(bundleFolder.c_str(), bundlePathDeltaCatalogPair.back().second.c_str(), deltaCatalogFilePath, true);

        AZStd::string assetAlias = PlatformAddressedAssetCatalog::GetAssetRootForPlatform(platformId);

        if (fileIO->Exists(bundleFilePath.c_str()))
        {
            // This will delete both the parent bundle as well as all the dependent bundles mentioned in the manifest file of the parent bundle.
            if (!DeleteBundleFiles(bundleFilePath.Native()))
            {
                return false;
            }
        }

        for (const AzToolsFramework::AssetFileInfo& assetFileInfo : assetFileInfoList.m_fileInfoList)
        {
            AZ::u64 fileSize = 0;
            AZStd::string fullAssetFilePath;
            AzFramework::StringFunc::Path::Join(assetAlias.c_str(), assetFileInfo.m_assetRelativePath.c_str(), fullAssetFilePath);
            if (!fileIO->Size(fullAssetFilePath.c_str(), fileSize))
            {
                AZ_Error(logWindowName, false, "Unable to find size of file (%s).\n", fullAssetFilePath.c_str());
                return false;
            }

            if (fileSize > maxSizeInBytes)
            {
                AZ_Warning(logWindowName, false, "File (%s) size (%d) is bigger than the max bundle size (%d).\n", assetFileInfo.m_assetRelativePath.c_str(), fileSize, maxSizeInBytes);
            }

            totalFileSize += fileSize;

            if (!MaxSizeExceeded(totalFileSize, bundleSize, assetCatalogFileSizeBuffer, maxSizeInBytes))
            {
                // if we are here it implies that the total file size on disk is less than the max bundle size 
                // and therefore these files can be added together into the bundle.
                fileEntries.emplace_back(assetFileInfo.m_assetRelativePath);
                deltaCatalogEntries.emplace_back(assetFileInfo.m_assetRelativePath);
                continue;
            }
            else
            {
                // add all files to the archive as a batch and update the bundle size
                if (!InjectFiles(fileEntries, tempBundleFilePath, assetAlias.c_str()))
                {
                    return false;
                }

                fileEntries.clear();

                if (fileIO->Exists(tempBundleFilePath.c_str()))
                {
                    if (!fileIO->Size(tempBundleFilePath.c_str(), bundleSize))
                    {
                        AZ_Error(logWindowName, false, "Unable to find size of archive file (%s).\n", bundleFilePath.c_str());
                        return false;
                    }
                }
            }

            totalFileSize = fileSize;

            if (MaxSizeExceeded(totalFileSize, bundleSize, assetCatalogFileSizeBuffer, maxSizeInBytes))
            {
                // if we are here it implies that adding file size to the remaining increases the size over the max size 
                // and therefore we can add the pending files and the delta catalog to the bundle
                if (!AddCatalogAndFilesToBundle(deltaCatalogEntries, fileEntries, tempBundleFilePath, assetAlias.c_str(), platformId))
                {
                    return false;
                }

                fileEntries.clear();
                deltaCatalogEntries.clear();
                bundleSize = 0;
                int numOfTries = 20;
                AZStd::string dependentBundleFileName;
                do
                {
                    // we need to find a bundle which does not exist on disk;
                    bundleIndex++;
                    numOfTries--;
                    dependentBundleFileName = CreateAssetBundleFileName(bundleFilePath.Native(), bundleIndex);
                    AzFramework::StringFunc::Path::ReplaceFullName(tempBundleFilePath, (dependentBundleFileName + tempBundleFileSuffix).c_str());
                } while (numOfTries && fileIO->Exists(tempBundleFilePath.c_str()));

                if (!numOfTries)
                {
                    AZ_Error(logWindowName, false, "Unable to find a unique name for the archive in the directory (%s).\n", bundleFolder.c_str());
                    return false;
                }

                dependentBundleNames.emplace_back(dependentBundleFileName);

                AZStd::string currentDeltaCatalogName = DeltaCatalogName;
                AzFramework::StringFunc::Path::ConstructFull(bundleFolder.c_str(), currentDeltaCatalogName.c_str(), deltaCatalogFilePath, true);
                bundlePathDeltaCatalogPair.emplace_back(AZStd::make_pair(tempBundleFilePath, currentDeltaCatalogName));
            }

            fileEntries.emplace_back(assetFileInfo.m_assetRelativePath);
            deltaCatalogEntries.emplace_back(assetFileInfo.m_assetRelativePath);
        }


        if (!AddCatalogAndFilesToBundle(deltaCatalogEntries, fileEntries, tempBundleFilePath, assetAlias.c_str(), platformId))
        {
            return false;
        }

        // Create and add manifest files for all the bundles
        if (!AddManifestFileToBundles(bundlePathDeltaCatalogPair, dependentBundleNames, bundleFolder, assetBundleSettings, levelDirs))
        {
            return false;
        }

        // Rename all the temp files to the actual bundle names
        for (int idx = 0; idx < bundlePathDeltaCatalogPair.size(); ++idx)
        {
            AZStd::string destinationBundleFullPath = bundlePathDeltaCatalogPair[idx].first.c_str();

            if (destinationBundleFullPath.ends_with(tempBundleFileSuffix))
            {
                destinationBundleFullPath = destinationBundleFullPath.substr(0, destinationBundleFullPath.length() - strlen(tempBundleFileSuffix));
                int numRetries = 3;
                while(!fileIO->Rename(bundlePathDeltaCatalogPair[idx].first.c_str(), destinationBundleFullPath.c_str()))
                {
                    --numRetries;
                    AZ_Error(logWindowName, false, "Failed to rename temporary bundle file (%s) to (%s)%s", bundlePathDeltaCatalogPair[idx].first.c_str(), destinationBundleFullPath.c_str(), numRetries ? "  Retrying.." : "");
                    if (!numRetries)
                    {
                        return false;
                    }
                    constexpr auto SleepDuration = AZStd::chrono::seconds(1);
                    AZStd::this_thread::sleep_for(SleepDuration);
                }
            }
        }

        return true;
    }

    bool AssetBundleComponent::CreateAssetBundle(const AssetBundleSettings& assetBundleSettings)
    {

        if (assetBundleSettings.m_assetFileInfoListPath.empty())
        {
            AZ_Error(logWindowName, false, "AssetFileInfo List path is empty. \n");
            return false;
        }

        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();

        AZ::IO::Path assetFileInfoListPath = AZ::IO::Path{ AZStd::string_view{AZ::Utils::GetEnginePath()} } / assetBundleSettings.m_assetFileInfoListPath;

        if (!fileIO->Exists(assetFileInfoListPath.c_str()))
        {
            AZ_Error(logWindowName, false, "AssetFileInfoList file (%s) does not exist. \n", assetFileInfoListPath.c_str());
            return false;
        }

        AssetFileInfoList assetFileInfoList;

        if (!AZ::Utils::LoadObjectFromFileInPlace(assetFileInfoListPath.c_str(), assetFileInfoList))
        {
            AZ_Error(logWindowName, false, "Failed to deserialize file (%s).\n", assetFileInfoListPath.c_str());
            return false;
        }

        return CreateAssetBundleFromList(assetBundleSettings, assetFileInfoList);
    }

    bool AssetBundleComponent::AddCatalogAndFilesToBundle(const AZStd::vector<AZStd::string>& deltaCatalogEntries, const AZStd::vector<AZStd::string>& fileEntries, const AZStd::string& bundleFilePath, const char* assetAlias, const AzFramework::PlatformId& platformId)
    {
        AZStd::string bundleFolder;
        AzFramework::StringFunc::Path::GetFullPath(bundleFilePath.c_str(), bundleFolder);

        if (!MakePath(bundleFolder))
        {
            return  false;
        }

        if (fileEntries.size())
        {
            if (!InjectFiles(fileEntries, bundleFilePath, assetAlias))
            {
                return false;
            }
        }

        if(deltaCatalogEntries.size())
        {
            TemporaryDir tempDir(bundleFilePath);
            if (!tempDir.m_result)
            {
                return false;
            }

            AZStd::string tempDeltaCatalogFile;
            AzFramework::StringFunc::Path::Join(tempDir.m_tempFolderPath.c_str(), DeltaCatalogName, tempDeltaCatalogFile);

            // add the delta catalog to the bundle
            bool success = false;
            AssetCatalog::PlatformAddressedAssetCatalogRequestBus::EventResult(success, platformId, &AssetCatalog::PlatformAddressedAssetCatalogRequestBus::Events::CreateDeltaCatalog, deltaCatalogEntries, tempDeltaCatalogFile.c_str());
            if (!success)
            {
                AZ_Error(logWindowName, false, "Failed to create the delta catalog file (%s).\n", tempDeltaCatalogFile.c_str());
                return false;
            }
            if (!InjectFile(DeltaCatalogName, bundleFilePath, tempDir.m_tempFolderPath.c_str()))
            {
                AZ_Error(logWindowName, false, "Failed to add the delta catalog file (%s) in the bundle (%s).\n", tempDeltaCatalogFile.c_str(), bundleFilePath.c_str());
                return false;
            }
        }

        return true;
    }

    bool AssetBundleComponent::AddManifestFileToBundles(const AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>>& bundlePathDeltaCatalogPair, const AZStd::vector<AZStd::string>& dependentBundleNames, const AZStd::string& bundleFolder, const AzToolsFramework::AssetBundleSettings& assetBundleSettings, const AZStd::vector<AZ::IO::Path>& levelDirs)
    {
        if (!MakePath(bundleFolder))
        {
            return false;
        }

        if (bundlePathDeltaCatalogPair.empty())
        {
            AZ_Warning(logWindowName, false, "AddManifestFilesToBundle called with no bundle paths provided.  Cannot add manifest file.");

            return false;
        }

        TemporaryDir tempDir(bundlePathDeltaCatalogPair[0].first);

        if (!tempDir.m_result)
        {
            return false;
        }

        AZStd::string bundleManifestPath;
        AzFramework::StringFunc::Path::ConstructFull(tempDir.m_tempFolderPath.c_str(), AzFramework::AssetBundleManifest::s_manifestFileName, bundleManifestPath, true);

        for (int idx = 0; idx < bundlePathDeltaCatalogPair.size(); idx++)
        {
            AZStd::vector<AZStd::string> bundleNameList;
            if (!idx)
            {
                // The manifest file of the base bundle is special and contains the names of all the rollover bundle in it.
                bundleNameList = dependentBundleNames;
            }

            bool manifestSaved = false;
            AssetCatalogRequestBus::BroadcastResult(manifestSaved, &AssetCatalogRequestBus::Events::CreateBundleManifest, bundlePathDeltaCatalogPair[idx].second, bundleNameList, tempDir.m_tempFolderPath, assetBundleSettings.m_bundleVersion, levelDirs);
            if (!manifestSaved)
            {
                AZ_Error(logWindowName, false, "Failed to create manifest file (%s) for the bundle (%s).\n", AzFramework::AssetBundleManifest::s_manifestFileName, bundlePathDeltaCatalogPair[idx].first.c_str());
                return false;
            }

            AZStd::string bundleManifestFileName;
            AzFramework::StringFunc::Path::GetFullFileName(bundleManifestPath.c_str(), bundleManifestFileName);

            if (!InjectFile(bundleManifestFileName, bundlePathDeltaCatalogPair[idx].first, tempDir.m_tempFolderPath.c_str()))
            {
                AZ_Error(logWindowName, false, "Failed to add manifest file (%s) in the bundle (%s).\n", bundleManifestPath.c_str(), bundlePathDeltaCatalogPair[idx].first.c_str());
                return false;
            }

        }

        return true;
    }

    bool AssetBundleComponent::DeleteBundleFiles(const AZStd::string& assetBundleFilePath)
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        AZStd::string bundleFolder;
        AzFramework::StringFunc::Path::GetFullPath(assetBundleFilePath.c_str(), bundleFolder);
        AZStd::unique_ptr<AzFramework::AssetBundleManifest> manifest(GetManifestFromBundle(assetBundleFilePath));
        if (manifest)
        {
            for (const AZStd::string& filename : manifest->GetDependentBundleNames())
            {
                AZStd::string dependentBundlesFilePath;
                AzFramework::StringFunc::Path::ConstructFull(bundleFolder.c_str(), filename.c_str(), dependentBundlesFilePath, true);
                if (!fileIO->Remove(dependentBundlesFilePath.c_str()))
                {
                    AZ_Warning(logWindowName, false, "Failed to delete dependent bundle file (%s)", dependentBundlesFilePath.c_str());
                }
            }

        }

        // remove the parent bundle
        if (!fileIO->Remove(assetBundleFilePath.c_str()))
        {
            AZ_Error(logWindowName, false, "Failed to delete bundle file (%s)", assetBundleFilePath.c_str());
            return false;
        }

        return true;
    }

    bool AssetBundleComponent::InjectFile(const AZStd::string& filePath, const AZStd::string& archiveFilePath, const char* workingDirectory)
    {
        AZ_TracePrintf(logWindowName, "Injecting file (%s) into bundle (%s).\n", filePath.c_str(), archiveFilePath.c_str());
        bool fileAddedToArchive = false;
        std::future<bool> fileAdded;
        int retryCount = InjectFileRetryCount;

        while (!fileAddedToArchive && retryCount)
        {
            ArchiveCommandsBus::BroadcastResult(fileAdded, &AzToolsFramework::ArchiveCommandsBus::Events::AddFileToArchive, archiveFilePath, workingDirectory, filePath);
            --retryCount;
            fileAddedToArchive = fileAdded.get();
            if (!fileAddedToArchive && retryCount)
            {
                AZ_Error(logWindowName, false, "Failed to insert file (%s) into bundle (%s). Retrying.", filePath.c_str(), archiveFilePath.c_str());
            }
        }

        if (!fileAddedToArchive)
        {
            AZ_Error(logWindowName, false, "Failed to insert file (%s) into bundle (%s) after %d retries.", filePath.c_str(), archiveFilePath.c_str(), InjectFileRetryCount);
        }
        return fileAddedToArchive;
    }
    
    bool AssetBundleComponent::InjectFile(const AZStd::string& filePath, const AZStd::string& sourcePak)
    {
        // When no working directory is specified, assume that the file being injected goes into the root of the archive.
        // The filePath should be an absolute path, making the workingDirectory be the path leading up to the file.
        AZ::IO::PathView fullFilePath{ filePath, AZ::IO::PosixPathSeparator };
        AZ::IO::Path workingDir{ fullFilePath.ParentPath() };
        return InjectFile(filePath, sourcePak, workingDir.c_str());
    }

    bool AssetBundleComponent::InjectFiles(const AZStd::vector<AZStd::string>& fileEntries, const AZStd::string& sourcePak, const char* workingDirectory)
    {
        if (!fileEntries.size())
        {
            return true;
        }
        AZStd::string filesStr;

        for (const AZStd::string& file : fileEntries)
        {
            filesStr.append(AZStd::string::format("%s\n", file.c_str()));
        }

        TemporaryDir tempDir(sourcePak);

        if (!tempDir.m_result)
        {
            return false;
        }

        // Creating a list file
        AZStd::string listFilePath;
        const char listFileName[] = "ListFile.txt";
        AzFramework::StringFunc::Path::ConstructFull(tempDir.m_tempFolderPath.c_str(), listFileName, listFilePath, true);

        {
            AZ::IO::FileIOStream fileStream(listFilePath.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeText);
            if (fileStream.IsOpen())
            {
                fileStream.Write(filesStr.size(), filesStr.data());
            }
            else
            {
                AZ_Error(logWindowName, false, "Failed to create list file (%s) for adding files in the bundle (%s).\n", listFilePath.c_str(), sourcePak.c_str());
                return false;
            }
        }

        std::future<bool> filesAdded;
        AzToolsFramework::ArchiveCommandsBus::BroadcastResult(filesAdded, &AzToolsFramework::ArchiveCommands::AddFilesToArchive, sourcePak, workingDirectory, listFilePath);
        bool filesAddedToArchive = filesAdded.get();
        if (!filesAddedToArchive)
        {
            AZ_Error(logWindowName, false, "Failed to insert files into bundle (%s).\n", sourcePak.c_str());
        }

        return filesAddedToArchive;
    }

    bool AssetBundleComponent::HasManifest(const AZStd::vector<AZStd::string>& fileEntries)
    {
        auto itr = AZStd::find(fileEntries.begin(), fileEntries.end(), AZStd::string(AzFramework::AssetBundleManifest::s_manifestFileName));
        return itr != fileEntries.end();
    }

    AzFramework::AssetBundleManifest* AssetBundleComponent::GetManifestFromBundle(const AZStd::string& sourcePak)
    {
        // open the manifest and deserialize it
        bool manifestExtracted = false;

        TemporaryDir tempDir(sourcePak);
        if (!tempDir.m_result)
        {
            return nullptr;
        }

        AZStd::string manifestFilePath;
        AzFramework::StringFunc::Path::ConstructFull(tempDir.m_tempFolderPath.c_str(), AzFramework::AssetBundleManifest::s_manifestFileName, manifestFilePath, true);

        std::future<bool> extractResult;
        ArchiveCommandsBus::BroadcastResult(extractResult, &ArchiveCommandsBus::Events::ExtractFile, sourcePak, AzFramework::AssetBundleManifest::s_manifestFileName, tempDir.m_tempFolderPath);
        manifestExtracted = extractResult.get();
        if (!manifestExtracted)
        {
            AZ_Error(logWindowName, false, "Failed to extract existing manifest from archive \"%s\".", sourcePak.c_str());
            return nullptr;
        }

        // deserialize the manifest
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        AZ_Assert(serializeContext, "Unable to retrieve serialize context.");
        if (nullptr == serializeContext->FindClassData(AZ::AzTypeInfo<AzFramework::AssetBundleManifest>::Uuid()))
        {
            AzFramework::AssetBundleManifest::ReflectSerialize(serializeContext);
        }
        AzFramework::AssetBundleManifest* manifest = AZ::Utils::LoadObjectFromFile<AzFramework::AssetBundleManifest>(manifestFilePath, serializeContext);
        if (manifest == nullptr)
        {
            AZ_Error(logWindowName, false, "Failed to deserialize existing manifest from archive \"%s\".", sourcePak.c_str());
        }

        return manifest;
    }

    bool AssetBundleComponent::RemoveNonAssetFileEntries(AZStd::vector<AZStd::string>& fileEntries, const AZStd::string& normalizedSourcePakPath, const AzFramework::AssetBundleManifest* manifest)
    {
        auto sourcePakItr = AZStd::find(fileEntries.begin(), fileEntries.end(), normalizedSourcePakPath);
        if (sourcePakItr != fileEntries.end())
        {
            fileEntries.erase(sourcePakItr);
        }

        if (manifest)
        {
            sourcePakItr = AZStd::find(fileEntries.begin(), fileEntries.end(), AZStd::string(AzFramework::AssetBundleManifest::s_manifestFileName));
            if (sourcePakItr != fileEntries.end())
            {
                fileEntries.erase(sourcePakItr);
            }

            // use the catalog name stored in the manifest to determine what to exclude and overwrite
            sourcePakItr = AZStd::find(fileEntries.begin(), fileEntries.end(), manifest->GetCatalogName());
            if (sourcePakItr != fileEntries.end())
            {
                fileEntries.erase(sourcePakItr);
            }
            else
            {
                AZ_Error(logWindowName, false, "Asset catalog \"%s\" referenced in manifest doesn't exist in bundle \"%s\".", manifest->GetCatalogName().c_str(), normalizedSourcePakPath.c_str());
                return false;
            }
        }
        return true;
    }
}

