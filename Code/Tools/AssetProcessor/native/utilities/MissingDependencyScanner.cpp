/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MissingDependencyScanner.h"

AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option")
AZ_POP_DISABLE_WARNING

#include "LineByLineDependencyScanner.h"
#include "PotentialDependencies.h"
#include "native/AssetDatabase/AssetDatabase.h"
#include "native/assetprocessor.h"
#include <AzCore/Component/TickBus.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/wildcard.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/XML/rapidxml.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/FileTag/FileTag.h>
#include <AzFramework/FileTag/FileTagBus.h>

namespace AssetProcessor
{
    constexpr auto EngineFolder = AZ::IO::FixedMaxPath("Assets") / "Engine";

    AZStd::string GetXMLDependenciesFile(const AZStd::string& fullPath, const AZStd::vector<AzFramework::GemInfo>& gemInfoList, AZStd::string& tokenName)
    {
        AZ::IO::Path xmlDependenciesFileFullPath;
        tokenName = EngineFolder.String();
        for (const AzFramework::GemInfo& gemElement : gemInfoList)
        {
            for (const AZ::IO::Path& absoluteSourcePath : gemElement.m_absoluteSourcePaths)
            {
                if (AZ::IO::PathView(fullPath).IsRelativeTo(absoluteSourcePath))
                {
                    xmlDependenciesFileFullPath = absoluteSourcePath;
                    xmlDependenciesFileFullPath /= AzFramework::GemInfo::GetGemAssetFolder();
                    xmlDependenciesFileFullPath /= AZStd::string::format("%s_Dependencies.xml", gemElement.m_gemName.c_str());;
                    if (AZ::IO::FileIOBase::GetInstance()->Exists(xmlDependenciesFileFullPath.c_str()))
                    {
                        tokenName = gemElement.m_gemName;
                        return xmlDependenciesFileFullPath.Native();
                    }
                }
            }
        }

        // if we are here than either the %gemName%_Dependencies.xml file does not exists or the user inputted path is not inside a gems folder,
        // in both the cases we will return the engine dependencies file
        xmlDependenciesFileFullPath = AZ::Utils::GetEnginePath();
        xmlDependenciesFileFullPath /= EngineFolder;
        xmlDependenciesFileFullPath /= "Engine_Dependencies.xml";

        return xmlDependenciesFileFullPath.Native();
    }

    const int MissingDependencyScanner::DefaultMaxScanIteration = 800;
    class MissingDependency
    {
    public:
        MissingDependency(const AZ::Data::AssetId& assetId, const PotentialDependencyMetaData& metaData) :
            m_assetId(assetId),
            m_metaData(metaData)
        {

        }

        // Allows MissingDependency to be in a sorted container, which stabilizes log output.
        bool operator<(const MissingDependency& rhs) const
        {
            return m_assetId < rhs.m_assetId;
        }

        AZ::Data::AssetId m_assetId;
        PotentialDependencyMetaData m_metaData;
    };

    MissingDependencyScanner::MissingDependencyScanner()
    {
        m_defaultScanner = AZStd::make_shared<LineByLineDependencyScanner>();
        ApplicationManagerNotifications::Bus::Handler::BusConnect();
        MissingDependencyScannerRequestBus::Handler::BusConnect();
    }

    MissingDependencyScanner::~MissingDependencyScanner()
    {
        MissingDependencyScannerRequestBus::Handler::BusDisconnect();
        ApplicationManagerNotifications::Bus::Handler::BusDisconnect();
    }

    void MissingDependencyScanner::ApplicationShutdownRequested()
    {
        // Do not add any new functions to the SystemTickBus queue
        m_shutdownRequested = true;

        // Finish up previously queued work
        AZ::SystemTickBus::ExecuteQueuedEvents();
    }

    void MissingDependencyScanner::ScanFile(const AZStd::string& fullPath, int maxScanIteration, AZStd::shared_ptr<AssetDatabaseConnection> databaseConnection, const AZStd::string& dependencyTokenName, bool queueDbCommandsOnMainThread, scanFileCallback callback)
    {
        AZ::s64 productPK = -1;
        AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer dependencies = {};

        ScanFile(fullPath, maxScanIteration, productPK, dependencies, databaseConnection, dependencyTokenName, ScannerMatchType::ExtensionOnlyFirstMatch, nullptr, queueDbCommandsOnMainThread, callback);
    }

    void MissingDependencyScanner::ScanFile(
        const AZStd::string& fullPath,
        int maxScanIteration,
        AZ::s64 productPK,
        const AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer& dependencies,
        AZStd::shared_ptr<AssetDatabaseConnection> databaseConnection,
        bool queueDbCommandsOnMainThread,
        scanFileCallback callback)
    {
        ScanFile(fullPath, maxScanIteration, productPK, dependencies, databaseConnection, "", ScannerMatchType::ExtensionOnlyFirstMatch, nullptr, queueDbCommandsOnMainThread, callback);
    }

    void MissingDependencyScanner::ScanFile(
        const AZStd::string& fullPath,
        int maxScanIteration,
        AZ::s64 productPK,
        const AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer& dependencies,
        AZStd::shared_ptr<AssetDatabaseConnection> databaseConnection,
        AZStd::string dependencyTokenName,
        ScannerMatchType matchType,
        AZ::Crc32* forceScanner,
        bool queueDbCommandsOnMainThread,
        scanFileCallback callback)
    {
        using namespace AzFramework::FileTag;
        AZ_Printf(AssetProcessor::ConsoleChannel, "Scanning for missing dependencies:\t%s\n", fullPath.c_str());
        AzToolsFramework::AssetDatabase::SourceDatabaseEntry sourceEntry;
        if (productPK != -1)
        {
            AZStd::vector<AZStd::vector<AZStd::string>> excludedTagsList = {
            {
                FileTags[static_cast<unsigned int>(FileTagsIndex::EditorOnly)]
            },
            {
                FileTags[static_cast<unsigned int>(FileTagsIndex::Shader)]
            } };
            databaseConnection->GetSourceByProductID(productPK, sourceEntry);

            for (const AZStd::vector<AZStd::string>& tags : excludedTagsList)
            {
                bool shouldIgnore = false;
                QueryFileTagsEventBus::EventResult(shouldIgnore, FileTagType::Exclude,
                    &QueryFileTagsEventBus::Events::Match, fullPath.c_str(), tags);
                if (shouldIgnore)
                {
                    // Record that this file was ignored in the database, so the asset tab can display this information.
                    AZStd::string ignoredByTagText("File matches EditorOnly or Shader tag, ignoring for missing dependencies search.");
                    AZ_Printf(AssetProcessor::ConsoleChannel, "\t%s\n", ignoredByTagText.c_str());
                    SetDependencyScanResultStatus(
                        ignoredByTagText,
                        productPK,
                        sourceEntry.m_analysisFingerprint,
                        databaseConnection,
                        queueDbCommandsOnMainThread,
                        callback);
                    return;
                }
            }
        }
        else
        {
            // if we are here than it implies that this file is not an asset
            AZStd::vector<AZStd::string> tags{
            FileTags[static_cast<unsigned int>(FileTagsIndex::Ignore)],
            FileTags[static_cast<unsigned int>(FileTagsIndex::ProductDependency)] };
            bool shouldIgnore = false;
            QueryFileTagsEventBus::EventResult(shouldIgnore, FileTagType::Exclude, &QueryFileTagsEventBus::Events::Match, fullPath.c_str(), tags);
            if (shouldIgnore)
            {
                AZ_Printf(AssetProcessor::ConsoleChannel, "File ( %s ) will be skipped by the missing dependency scanner.\n", fullPath.c_str());
                return;
            }
        }

        AZ::IO::FileIOStream fileStream;
        if (!fileStream.Open(fullPath.c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary))
        {
            AZ_Error(AssetProcessor::ConsoleChannel, false, "File at path %s could not be opened.", fullPath.c_str());

            // Record that this file was ignored in the database, so the asset tab can display this information.
            SetDependencyScanResultStatus(
                "The file could not be opened.",
                productPK,
                sourceEntry.m_analysisFingerprint,
                databaseConnection,
                queueDbCommandsOnMainThread,
                callback);
            return;
        }

        PotentialDependencies potentialDependencies;
        bool scanSuccessful = RunScan(fullPath, maxScanIteration, fileStream, potentialDependencies, matchType, forceScanner);
        fileStream.Close();

        if (!scanSuccessful)
        {
            // RunScan will report an error on what caused the scan to fail.
            SetDependencyScanResultStatus(
                "An error occured, see log for details.",
                productPK,
                sourceEntry.m_analysisFingerprint,
                databaseConnection,
                queueDbCommandsOnMainThread,
                callback);
            return;
        }

        MissingDependencies missingDependencies;
        PopulateMissingDependencies(productPK, databaseConnection, dependencies, missingDependencies, potentialDependencies);

        if (queueDbCommandsOnMainThread && !m_shutdownRequested)
        {
            AZ::SystemTickBus::QueueFunction([=]()
            {
                ReportMissingDependencies(productPK, databaseConnection, dependencyTokenName, missingDependencies, callback);
            });
        }
        else
        {
            ReportMissingDependencies(productPK, databaseConnection, dependencyTokenName, missingDependencies, callback);
        }
    }

    void MissingDependencyScanner::SetDependencyScanResultStatus(
        AZStd::string status,
        AZ::s64 productPK,
        const AZStd::string& analysisFingerprint,
        AZStd::shared_ptr<AssetDatabaseConnection> databaseConnection,
        bool queueDbCommandsOnMainThread,
        scanFileCallback callback)
    {
        QDateTime currentTime = QDateTime::currentDateTime();
        auto finalizeMissingDependency = [=]() {
            AzToolsFramework::AssetDatabase::MissingProductDependencyDatabaseEntry missingDependencyEntry(
                productPK,
                /*Scanner*/ "",
                /*Scanner Version*/ "",
                analysisFingerprint,
                AZ::Uuid::CreateNull(),
                /*Product ID*/ 0,
                status,
                currentTime.toString().toUtf8().constData(),
                currentTime.toSecsSinceEpoch());
            databaseConnection->SetMissingProductDependency(missingDependencyEntry);
            callback(missingDependencyEntry.m_missingDependencyString);
        };

        if (queueDbCommandsOnMainThread && !m_shutdownRequested)
        {
            AZ::SystemTickBus::QueueFunction(finalizeMissingDependency);
        }
        else
        {
            finalizeMissingDependency();
        }
    }

    void MissingDependencyScanner::RegisterSpecializedScanner(AZStd::shared_ptr<SpecializedDependencyScanner> scanner)
    {
        m_specializedScanners.insert(AZStd::pair<AZ::Crc32, AZStd::shared_ptr<SpecializedDependencyScanner>>(scanner->GetScannerCRC(), scanner));
    }

    bool MissingDependencyScanner::RunScan(
        const AZStd::string& fullPath,
        int maxScanIteration,
        AZ::IO::GenericStream& fileStream,
        PotentialDependencies& potentialDependencies,
        ScannerMatchType matchType,
        AZ::Crc32* forceScanner)
    {
        // If a scanner is given to specifically use, then use that scanner and only that scanner.
        if (forceScanner)
        {
            AZ_Printf(
                AssetProcessor::ConsoleChannel,
                "\tForcing scanner with CRC %d\n",
                *forceScanner);
            DependencyScannerMap::iterator scannerToUse = m_specializedScanners.find(*forceScanner);
            if (scannerToUse != m_specializedScanners.end())
            {
                scannerToUse->second->ScanFileForPotentialDependencies(fileStream, potentialDependencies, maxScanIteration);
                return true;
            }
            else
            {
                AZ_Error(AssetProcessor::ConsoleChannel, false, "Attempted to force dependency scan using CRC %d, which is not registered.",
                    *forceScanner);
                return false;
            }
        }

        // Check if a specialized scanner should be used, based on the given scanner matching type rule.
        for (const AZStd::pair<AZ::Crc32, AZStd::shared_ptr<SpecializedDependencyScanner>>&
            scanner : m_specializedScanners)
        {
            switch (matchType)
            {
            case ScannerMatchType::ExtensionOnlyFirstMatch:
                if (scanner.second->DoesScannerMatchFileExtension(fullPath))
                {
                    return scanner.second->ScanFileForPotentialDependencies(fileStream, potentialDependencies, maxScanIteration);
                }
                break;
            case ScannerMatchType::FileContentsFirstMatch:
                if (scanner.second->DoesScannerMatchFileData(fileStream))
                {
                    return scanner.second->ScanFileForPotentialDependencies(fileStream, potentialDependencies, maxScanIteration);
                }
                break;
            case ScannerMatchType::Deep:
                // A deep scan has every matching scanner scan the file, and uses the default scan.
                if (scanner.second->DoesScannerMatchFileData(fileStream))
                {
                    scanner.second->ScanFileForPotentialDependencies(fileStream, potentialDependencies, maxScanIteration);
                }
                break;
            default:
                AZ_Error(AssetProcessor::ConsoleChannel, false, "Scan match type %d is not available.", matchType);
                break;
            };
        }

        // No specialized scanner was found (or a deep scan is being performed), so use the default scanner.
        return m_defaultScanner->ScanFileForPotentialDependencies(fileStream, potentialDependencies, maxScanIteration);
    }

    void MissingDependencyScanner::PopulateMissingDependencies(
        AZ::s64 productPK,
        AZStd::shared_ptr<AssetDatabaseConnection> databaseConnection,
        const AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer& dependencies,
        MissingDependencies& missingDependencies,
        const PotentialDependencies& potentialDependencies)
    {
        // If a file references itself, don't report it.
        AzToolsFramework::AssetDatabase::SourceDatabaseEntry fileWithPotentialMissingDependencies;
        databaseConnection->GetSourceByProductID(productPK, fileWithPotentialMissingDependencies);

        AZStd::map<AZ::Uuid, PotentialDependencyMetaData> uuids(potentialDependencies.m_uuids);
        AZStd::map<AZ::Data::AssetId, PotentialDependencyMetaData> assetIds(potentialDependencies.m_assetIds);

        // Check if any products exist for the given job, and those products have a sub ID that matches
        // the expected sub ID.
        AzToolsFramework::AssetDatabase::ProductDatabaseEntry productWithPotentialMissingDependencies;
        databaseConnection->GetProductByProductID(productPK, productWithPotentialMissingDependencies);
        QString scannedProductPath( productWithPotentialMissingDependencies.m_productName.c_str() );
        auto lastSeparatorIndex = scannedProductPath.lastIndexOf(AZ_CORRECT_DATABASE_SEPARATOR_STRING);
        scannedProductPath = scannedProductPath.remove(lastSeparatorIndex + 1, scannedProductPath.length());

        // Check the existing product dependency list for the file that is being scanned, remove
        // any potential UUIDs that match dependencies already being emitted.
        for (const AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry&
            existingDependency : dependencies)
        {
            AZStd::map<AZ::Uuid, PotentialDependencyMetaData>::iterator matchingDependency =
                uuids.find(existingDependency.m_dependencySourceGuid);
            if (matchingDependency != uuids.end())
            {
                uuids.erase(matchingDependency);
            }
        }

        // Remove all UUIDs that don't match an asset in the database.
        for (AZStd::map<AZ::Uuid, PotentialDependencyMetaData>::iterator uuidIter = uuids.begin();
            uuidIter != uuids.end();
            ++uuidIter)
        {
            if (fileWithPotentialMissingDependencies.m_sourceGuid == uuidIter->first)
            {
                // This product references itself, or the source it comes from. Don't report it as a missing dependency.
                continue;
            }

            AzToolsFramework::AssetDatabase::SourceDatabaseEntry sourceEntry;
            if (!databaseConnection->GetSourceBySourceGuid(uuidIter->first, sourceEntry))
            {
                // The UUID isn't in the asset database, don't add it to the list of missing dependencies.
                continue;
            }

            AzToolsFramework::AssetDatabase::JobDatabaseEntryContainer jobs;
            if (!databaseConnection->GetJobsBySourceID(sourceEntry.m_sourceID, jobs))
            {
                // No jobs existed for that source asset, so there are no products for this asset.
                // With no products, there is no way there can be a missing product dependency.
                continue;
            }

            // The dependency only referenced the source UUID, so add all products as missing dependencies.
            for (const AzToolsFramework::AssetDatabase::JobDatabaseEntry& job : jobs)
            {
                AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer products;
                if (!databaseConnection->GetProductsByJobID(job.m_jobID, products))
                {
                    continue;
                }
                for (const AzToolsFramework::AssetDatabase::ProductDatabaseEntry& product : products)
                {
                    // This match was for a UUID with no product ID, so add all products as missing dependencies.
                    MissingDependency missingDependency(
                        AZ::Data::AssetId(uuidIter->first, product.m_subID),
                        uuidIter->second);
                    missingDependencies.insert(missingDependency);
                }
            }
        }

        // Validate the asset ID list, removing anything that is already a dependency, or does not exist in the asset database.
        for (AZStd::map<AZ::Data::AssetId, PotentialDependencyMetaData>::iterator assetIdIter = assetIds.begin();
            assetIdIter != assetIds.end();
            ++assetIdIter)
        {
            bool foundUUID = false;
            // Strip out all existing, matching dependencies
            for (const AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry&
                existingDependency : dependencies)
            {
                if (existingDependency.m_dependencySourceGuid == assetIdIter->first.m_guid &&
                    existingDependency.m_dependencySubID == assetIdIter->first.m_subId)
                {
                    foundUUID = true;
                    break;
                }
            }

            // There is already a dependency with this UUID, so it's not a missing dependency.
            if (foundUUID)
            {
                continue;
            }

            AzToolsFramework::AssetDatabase::SourceDatabaseEntry sourceEntry;
            if (!databaseConnection->GetSourceBySourceGuid(assetIdIter->first.m_guid, sourceEntry))
            {
                // The UUID isn't in the asset database. Don't report it as a missing dependency
                // because UUIDs are used for tracking many things that are not assets.
                continue;
            }

            AzToolsFramework::AssetDatabase::JobDatabaseEntryContainer jobs;
            if (!databaseConnection->GetJobsBySourceID(sourceEntry.m_sourceID, jobs))
            {
                // No jobs existed for that source asset, so there are no products for this asset.
                // With no products, there is no way there can be a missing product dependency.
                continue;
            }


            bool isProductOfFileWithPotentialMissingDependencies = fileWithPotentialMissingDependencies.m_sourceGuid == assetIdIter->first.m_guid;

            bool foundMatchingProduct = false;
            for (const AzToolsFramework::AssetDatabase::JobDatabaseEntry& job : jobs)
            {
                AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer products;
                if (!databaseConnection->GetProductsByJobID(job.m_jobID, products))
                {
                    continue;
                }
                for (const AzToolsFramework::AssetDatabase::ProductDatabaseEntry& product : products)
                {
                    if (product.m_subID == assetIdIter->first.m_subId)
                    {
                        // This product references itself. Don't report it as a missing dependency.
                        // If the product references a different product of the same source and that isn't
                        // a dependency, then do report that.
                        // We have to check against more than the productPK to catch identical products across multiple
                        // platforms. 
                        if (productPK == product.m_productID ||
                            (isProductOfFileWithPotentialMissingDependencies && productWithPotentialMissingDependencies.m_subID == product.m_subID))
                        {
                            continue;
                        }

                        MissingDependency missingDependency(
                            assetIdIter->first,
                            assetIdIter->second);
                        missingDependencies.insert(missingDependency);
                        foundMatchingProduct = true;
                        break;
                    }
                }
                if (foundMatchingProduct)
                {
                    break;
                }
            }
        }

        for (const PotentialDependencyMetaData& path : potentialDependencies.m_paths)
        {
            AzToolsFramework::AssetDatabase::SourceDatabaseEntryContainer searchSources;
            QString searchName(path.m_sourceString.c_str());
            // The paths in the file may have had slashes in either direction, or double slashes.
            searchName.replace(AZ_WRONG_DATABASE_SEPARATOR_STRING, AZ_CORRECT_DATABASE_SEPARATOR_STRING);
            searchName.replace(AZ_DOUBLE_CORRECT_DATABASE_SEPARATOR, AZ_CORRECT_DATABASE_SEPARATOR_STRING);
            
            if (databaseConnection->GetSourcesBySourceName(searchName, searchSources))
            {
                // A source matched the path, look up products and add them as resolved path dependencies.
                for (const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& source : searchSources)
                {
                    if (fileWithPotentialMissingDependencies.m_sourceGuid == source.m_sourceGuid)
                    {
                        // This product references itself, or the source it comes from. Don't report it as a missing dependency.
                        continue;
                    }

                    bool dependencyExistsForSource = false;
                    for (const AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry&
                        existingDependency : dependencies)
                    {
                        if (existingDependency.m_dependencySourceGuid == source.m_sourceGuid)
                        {
                            dependencyExistsForSource = true;
                            break;
                        }
                    }

                    if (dependencyExistsForSource)
                    {
                        continue;
                    }

                    AzToolsFramework::AssetDatabase::JobDatabaseEntryContainer jobs;
                    if (!databaseConnection->GetJobsBySourceID(source.m_sourceID, jobs))
                    {
                        // No jobs exist for this source, which means there is no matching product dependency.
                        continue;
                    }

                    for (const AzToolsFramework::AssetDatabase::JobDatabaseEntry& job : jobs)
                    {
                        AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer products;
                        if (!databaseConnection->GetProductsByJobID(job.m_jobID, products))
                        {
                            // No products, no product dependencies.
                            continue;
                        }

                        for (const AzToolsFramework::AssetDatabase::ProductDatabaseEntry& product : products)
                        {
                            MissingDependency missingDependency(
                                AZ::Data::AssetId(source.m_sourceGuid, product.m_subID),
                                path);
                            missingDependencies.insert(missingDependency);
                        }
                    }
                }
            }
            else
            {
                // Product paths in the asset database include the platform and additional pathing information that
                // makes this check more complex than the source path check.
                // Examples:
                //      pc/usersettings.xml
                //      pc/ProjectName/file.xml
                // Taking all results from this EndsWith check can lead to an over-emission of potential missing dependencies.
                // For example, if a file has a comment like "Something about .dds files", then EndsWith would return
                // every single dds file in the database.
                AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer products;
                if (!databaseConnection->GetProductsLikeProductName(searchName, AssetDatabaseConnection::LikeType::EndsWith, products))
                {
                    continue;
                }
                for (const AzToolsFramework::AssetDatabase::ProductDatabaseEntry& product : products)
                {
                    if (productPK == product.m_productID)
                    {
                        // Don't report if a file has a reference to itself.
                        continue;
                    }

                    // Cull the platform from the product path to perform a more confident comparison against the given path.
                    QString culledProductPath = QString(product.m_productName.c_str());

                    // If this appears to be a valid path to a product relative to the product being checked
                    if (culledProductPath.compare(scannedProductPath.append(searchName), Qt::CaseInsensitive) != 0)
                    {

                        culledProductPath = culledProductPath.remove(0, culledProductPath.indexOf(AZ_CORRECT_DATABASE_SEPARATOR_STRING) + 1);

                        // This first check will catch paths that include the project name, as well as references to assets that include a scan folder in the path.
                        if (culledProductPath.compare(searchName, Qt::CaseInsensitive) != 0)
                        {
                            int nextFolderIndex = culledProductPath.indexOf(AZ_CORRECT_DATABASE_SEPARATOR_STRING);
                            if (nextFolderIndex == -1)
                            {
                                continue;
                            }
                            // Perform a second check with the scan folder removed. Many asset references are relevant to scan folder roots.
                            // For example, a material may have a relative path reference to a texture as "textures/SomeTexture.dds".
                            // This relative path resolves in many systems based on scan folder root, so if this file is in "platform/project/textures/SomeTexture.dds",
                            // this check is intended to find that reference.
                            culledProductPath = culledProductPath.remove(0, nextFolderIndex + 1);
                            if (culledProductPath.compare(searchName, Qt::CaseInsensitive) != 0)
                            {
                                continue;
                            }
                        }
                    }

                    AzToolsFramework::AssetDatabase::SourceDatabaseEntryContainer productSources;
                    if (!databaseConnection->GetSourcesByProductName(QString(product.m_productName.c_str()), productSources))
                    {
                        AZ_Error(
                            AssetProcessor::ConsoleChannel,
                            false,
                            "Product %s does not have a matching source. Your database may be corrupted.", product.m_productName.c_str());
                        continue;
                    }

                    for (const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& source : productSources)
                    {
                        bool dependencyExistsForProduct = false;
                        for (const AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry&
                            existingDependency : dependencies)
                        {
                            if (existingDependency.m_dependencySourceGuid == source.m_sourceGuid &&
                                existingDependency.m_dependencySubID == product.m_subID)
                            {
                                dependencyExistsForProduct = true;
                                break;
                            }
                        }
                        if (!dependencyExistsForProduct)
                        {
                            AZ::Data::AssetId assetId(source.m_sourceGuid, product.m_subID);
                            MissingDependency missingDependency(
                                AZ::Data::AssetId(source.m_sourceGuid, product.m_subID),
                                path);

                            missingDependencies.insert(missingDependency);
                        }
                    }
                }

            }
        }
    }

    void MissingDependencyScanner::ReportMissingDependencies(
        AZ::s64 productPK,
        AZStd::shared_ptr<AssetDatabaseConnection> databaseConnection,
        const AZStd::string& dependencyTokenName,
        const MissingDependencies& missingDependencies,
        scanFileCallback callback)
    {
        using namespace AzFramework::FileTag;
        AzToolsFramework::AssetDatabase::SourceDatabaseEntry sourceEntry;
        databaseConnection->GetSourceByProductID(productPK, sourceEntry);

        AZStd::vector<AZStd::string> tags{ 
            FileTags[static_cast<unsigned int>(FileTagsIndex::Ignore)],
            FileTags[static_cast<unsigned int>(FileTagsIndex::ProductDependency)] };

        QDateTime currentTime = QDateTime::currentDateTime();

        // If there were no missing dependencies, add a row to the table so we know it was scanned.
        if (productPK != -1 && missingDependencies.empty())
        {
            SetDependencyScanResultStatus(
                "No missing dependencies found",
                productPK,
                sourceEntry.m_analysisFingerprint,
                databaseConnection,
                /*queueDbCommandsOnMainThread*/ false, // ReportMissingDependencies was already queued to run on the main thread.
                callback);
            return;
        }

        for (const MissingDependency& missingDependency : missingDependencies)
        {
            bool shouldIgnore = false;
            QueryFileTagsEventBus::EventResult(shouldIgnore, FileTagType::Exclude,
                &QueryFileTagsEventBus::Events::Match, missingDependency.m_metaData.m_sourceString.c_str(), tags);

            if (!shouldIgnore && !dependencyTokenName.empty())
            {
                // if one of the rules in the xml dependency file match then skip the missing dependency
                auto rulesFound = m_dependenciesRulesMap.find(dependencyTokenName);
                if (rulesFound != m_dependenciesRulesMap.end())
                {
                    for (const auto& rule : rulesFound->second)
                    {
                        if (AZStd::wildcard_match(rule, missingDependency.m_metaData.m_sourceString))
                        {
                            shouldIgnore = true;
                            break;
                        }
                    }
                }
            }
            if (!shouldIgnore)
            {
                AZStd::string assetIdStr = missingDependency.m_assetId.ToString<AZStd::string>();
                AZ_Printf(
                    AssetProcessor::ConsoleChannel,
                    "\t\tMissing dependency: String \"%s\" matches asset: %s\n",
                    missingDependency.m_metaData.m_sourceString.c_str(),
                    assetIdStr.c_str());

                if (productPK != -1)
                {
                    AzToolsFramework::AssetDatabase::MissingProductDependencyDatabaseEntry missingDependencyEntry(
                        productPK,
                        missingDependency.m_metaData.m_scanner->GetName(),
                        missingDependency.m_metaData.m_scanner->GetVersion(),
                        sourceEntry.m_analysisFingerprint,
                        missingDependency.m_assetId.m_guid,
                        missingDependency.m_assetId.m_subId,
                        missingDependency.m_metaData.m_sourceString,
                        currentTime.toString().toUtf8().constData(),
                        currentTime.toSecsSinceEpoch());

                    databaseConnection->SetMissingProductDependency(missingDependencyEntry);
                }

                callback(missingDependency.m_metaData.m_sourceString);
            }
        }
    }

    bool MissingDependencyScanner::PopulateRulesForScanFolder(const AZStd::string& scanFolderPath, const AZStd::vector<AzFramework::GemInfo>& gemInfoList, AZStd::string& dependencyTokenName)
    {
        AZStd::string xmlDependenciesFullFilePath = GetXMLDependenciesFile(scanFolderPath, gemInfoList, dependencyTokenName);
        if (xmlDependenciesFullFilePath.empty())
        {
            AZ_Printf(AssetProcessor::ConsoleChannel, "Unable to find xml dependency file for the directory scan %s\n", scanFolderPath.c_str());
        }

        auto found = m_dependenciesRulesMap.find(dependencyTokenName);
        if (found != m_dependenciesRulesMap.end())
        {
            // this imply that we have already parsed this file and populated the rules,
            // therefore we can exit early.
            return true;
        }

        if (!AZ::IO::FileIOBase::GetInstance()->Exists(xmlDependenciesFullFilePath.c_str()))
        {
            AZ_Printf(AssetProcessor::ConsoleChannel, "Unable to find xml dependency file (%s). \n", xmlDependenciesFullFilePath.c_str());
            return false;
        }

        AZ::IO::FileIOStream fileStream;
        AZStd::vector<AZStd::string> dependenciesRuleList;
        if (fileStream.Open(xmlDependenciesFullFilePath.c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary))
        {
            if (!fileStream.CanRead())
            {
                return false;
            }

            AZ::IO::SizeType length = fileStream.GetLength();

            if (length == 0)
            {
                return false;
            }

            AZStd::vector<char> charBuffer;

            charBuffer.resize_no_construct(length + 1);
            fileStream.Read(length, charBuffer.data());
            charBuffer.back() = 0;

            AZ::rapidxml::xml_document<char> xmlDoc;
            xmlDoc.parse<AZ::rapidxml::parse_no_data_nodes>(charBuffer.data());
            auto engineDependenciesNode = xmlDoc.first_node("EngineDependencies");

            if (!engineDependenciesNode)
            {
                return false;
            }

            auto dependencyNode = engineDependenciesNode->first_node("Dependency");

            while (dependencyNode)
            {
                auto pathAttr = dependencyNode->first_attribute("path");

                if (pathAttr)
                {
                    dependenciesRuleList.emplace_back(AZStd::string(pathAttr->value()));
                }

                dependencyNode = dependencyNode->next_sibling();
            }

            m_dependenciesRulesMap[dependencyTokenName] = dependenciesRuleList;

            return true;
        }

        return false;


    }
}
