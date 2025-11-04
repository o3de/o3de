/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Builder/TestAssetBuilderComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Slice/SliceAsset.h>
#include <AzCore/RTTI/TypeInfo.h>

namespace TestAssetBuilder
{
    bool failedNetworkConnectionTest = true;

    AZ::Data::AssetStreamInfo TestDependentAssetCatalog::GetStreamInfoForLoad(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& type) 
    {
        if (AZ::AzTypeInfo<TestDependentAsset>::Uuid() != type)
        {
            AZ_Error("TestDependentAssetCatalog", false, "Invalid asset type %s", assetId.ToString<AZStd::string>().c_str());
            return {};
        }
        AZ::Data::AssetInfo assetInfo;
        AZStd::string rootFilePath;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, assetId);
        if (assetInfo.m_assetId.IsValid())
        {
            AZ::Data::AssetStreamInfo streamInfo;
            streamInfo.m_dataOffset = 0;
            streamInfo.m_dataLen = assetInfo.m_sizeBytes;
            streamInfo.m_streamName = assetInfo.m_relativePath;
            streamInfo.m_streamFlags = AZ::IO::OpenMode::ModeRead;
            return streamInfo;
        }
        return {};
    }


    TestAssetBuilderComponent::TestAssetBuilderComponent()
    {
    }

    TestAssetBuilderComponent::~TestAssetBuilderComponent()
    {
    }

    void TestAssetBuilderComponent::Init()
    {
    }

    void TestAssetBuilderComponent::Activate()
    {
        AssetBuilderSDK::AssetBuilderDesc builderDescriptor;
        builderDescriptor.m_name = "Test Asset Builder";
        builderDescriptor.m_version = 2;
        builderDescriptor.m_patterns.emplace_back(AssetBuilderSDK::AssetBuilderPattern("*.source", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_patterns.emplace_back(AssetBuilderSDK::AssetBuilderPattern("*.dependent", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_patterns.emplace_back(AssetBuilderSDK::AssetBuilderPattern("*.slicetest", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_patterns.emplace_back(AssetBuilderSDK::AssetBuilderPattern("*.foldertest", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_busId = azrtti_typeid<TestAssetBuilderComponent>();
        builderDescriptor.m_createJobFunction = AZStd::bind(&TestAssetBuilderComponent::CreateJobs, this, AZStd::placeholders::_1, AZStd::placeholders::_2);
        builderDescriptor.m_processJobFunction = AZStd::bind(&TestAssetBuilderComponent::ProcessJob, this, AZStd::placeholders::_1, AZStd::placeholders::_2);

        bool success = false;
        AZStd::vector<AZStd::string> assetSafeFolders;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(success, &AzToolsFramework::AssetSystemRequestBus::Events::GetAssetSafeFolders, assetSafeFolders);

        failedNetworkConnectionTest = !success || assetSafeFolders.empty();

        BusConnect(builderDescriptor.m_busId);

        m_dependentCatalog.reset(aznew TestDependentAssetCatalog());
        AZ::Data::AssetManager::Instance().RegisterCatalog(m_dependentCatalog.get(), AZ::AzTypeInfo<TestDependentAsset>::Uuid());
        AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBusTraits::RegisterBuilderInformation, builderDescriptor);
    }

    void TestAssetBuilderComponent::Deactivate()
    {
        BusDisconnect();
        AZ::Data::AssetManager::Instance().UnregisterCatalog(m_dependentCatalog.get());
        m_dependentCatalog.reset();
    }

    void TestAssetBuilderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<TestAssetBuilderComponent, AZ::Component>()
                ->Version(0)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }))
            ;
        }
    }

    void TestAssetBuilderComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("TestAssetBuilderPluginService"));
    }

    void TestAssetBuilderComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("TestAssetBuilderPluginService"));
    }

    void TestAssetBuilderComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    void TestAssetBuilderComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void TestAssetBuilderComponent::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
    {
        if (m_isShuttingDown)
        {
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
            return;
        }

        if(failedNetworkConnectionTest)
        {
            AZ_Assert(false, "GetAssetSafeFolders API failed to respond or responded with an empty list.  The network connection to AssetProcessor must be established before builder activation.");
            return;
        }

        AZStd::string ext;
        AzFramework::StringFunc::Path::GetExtension(request.m_sourceFile.c_str(), ext, false);

        if (AzFramework::StringFunc::Equal(ext.c_str(), "dependent"))
        {
            // Since we're a source file, we also add a job to do the actual compilation (for each enabled platform)
            for (const AssetBuilderSDK::PlatformInfo& platformInfo : request.m_enabledPlatforms)
            {
                AssetBuilderSDK::JobDescriptor descriptor;
                descriptor.m_jobKey = "Compile Example";
                descriptor.SetPlatformIdentifier(platformInfo.m_identifier.c_str());
                response.m_createJobOutputs.push_back(descriptor);
            }

            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
            return;
        }
        else if (AzFramework::StringFunc::Equal(ext.c_str(), "source"))
        {
            for (const AssetBuilderSDK::PlatformInfo& platformInfo : request.m_enabledPlatforms)
            {
                AssetBuilderSDK::JobDescriptor descriptor;
                descriptor.m_jobKey = "Compile Example";
                descriptor.SetPlatformIdentifier(platformInfo.m_identifier.c_str());

                // add a dependency on the other job:
                AssetBuilderSDK::SourceFileDependency sourceFile;
                sourceFile.m_sourceFileDependencyPath = request.m_sourceFile.c_str();
                AzFramework::StringFunc::Path::ReplaceExtension(sourceFile.m_sourceFileDependencyPath, "dependent");
                descriptor.m_jobDependencyList.push_back({ "Compile Example", platformInfo.m_identifier.c_str(), AssetBuilderSDK::JobDependencyType::Order, sourceFile });

                response.m_createJobOutputs.push_back(descriptor);
            }
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
            return;
        }
        else if (AzFramework::StringFunc::Equal(ext.c_str(), "foldertest"))
        {
            for (const AssetBuilderSDK::PlatformInfo& platformInfo : request.m_enabledPlatforms)
            {
                AssetBuilderSDK::JobDescriptor descriptor;
                descriptor.m_jobKey = "Compile Example";
                descriptor.SetPlatformIdentifier(platformInfo.m_identifier.c_str());

                AZStd::string folderName;
                AzFramework::StringFunc::Path::GetFileName(request.m_sourceFile.c_str(), folderName);
                AZStd::string baseFolder;
                AzFramework::StringFunc::Path::GetFolderPath(request.m_sourceFile.c_str(), baseFolder);
                AZStd::string outFolder;
                AzFramework::StringFunc::Path::Join(baseFolder.c_str(), folderName.c_str(), outFolder);

                AZStd::string folderDep{ outFolder + "/*.dependent" };
                // add a dependency on the other job:
                AssetBuilderSDK::SourceFileDependency sourceFile;
                sourceFile.m_sourceFileDependencyPath = folderDep.c_str();
                sourceFile.m_sourceDependencyType = AssetBuilderSDK::SourceFileDependency::SourceFileDependencyType::Wildcards;

                descriptor.m_jobDependencyList.push_back({ "Compile Example", platformInfo.m_identifier.c_str(), AssetBuilderSDK::JobDependencyType::Order, sourceFile });
                response.m_createJobOutputs.push_back(descriptor);
            }
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
            return;
        }
        else if (AzFramework::StringFunc::Equal(ext.c_str(), "slicetest"))
        {
            for (const AssetBuilderSDK::PlatformInfo& platformInfo : request.m_enabledPlatforms)
            {
                AssetBuilderSDK::JobDescriptor descriptor;
                descriptor.m_jobKey = "Compile Example";
                descriptor.SetPlatformIdentifier(platformInfo.m_identifier.c_str());

                // add a dependency on the other job:
                AssetBuilderSDK::SourceFileDependency sourceFile;
                sourceFile.m_sourceFileDependencyPath = request.m_sourceFile.c_str();
                AzFramework::StringFunc::Path::ReplaceExtension(sourceFile.m_sourceFileDependencyPath, "slice");
                descriptor.m_jobDependencyList.push_back({ "Editor Slice Copy", platformInfo.m_identifier.c_str(), AssetBuilderSDK::JobDependencyType::Order, sourceFile });

                response.m_createJobOutputs.push_back(descriptor);
            }
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
            return;
        }

        AZ_Assert(false, "Unhandled extension type in TestAssetBuilderWorker.");
        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Failed;
    }

    void TestAssetBuilderComponent::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
    {
        AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);

        AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "Starting Job.\n");

        AZ::IO::FileIOBase* fileIO = AZ::IO::LocalFileIO::GetInstance();
        AZStd::string outputData;

        AZ::IO::HandleType sourcefileHandle;
        if (!fileIO->Open(request.m_fullPath.c_str(), AZ::IO::OpenMode::ModeRead, sourcefileHandle))
        {
            AZ_Error("AssetBuilder", false, " Unable to open file ( %s ).", request.m_fullPath.c_str());
            return;
        }

        AZ::u64 sourceSizeBytes = 0;
        if (fileIO->Size(sourcefileHandle, sourceSizeBytes))
        {
            outputData.resize(sourceSizeBytes);
            if (!fileIO->Read(sourcefileHandle, outputData.data(), sourceSizeBytes))
            {
                AZ_Error("AssetBuilder", false, " Unable to read file ( %s ).", request.m_fullPath.c_str());
                fileIO->Close(sourcefileHandle);
                return;
            }
        }

        fileIO->Close(sourcefileHandle);

        AZStd::string fileName = request.m_sourceFile.c_str();
        AZStd::string ext;
        AzFramework::StringFunc::Path::GetExtension(request.m_sourceFile.c_str(), ext, false);
        AZ::Data::AssetType outputAssetType = AZ::Data::AssetType::CreateNull();

        const AZ::u32 DependentSubID = 2222;
        AZ::u32 outputSubId = 0;

        if (AzFramework::StringFunc::Equal(ext.c_str(), "source"))
        {
            AZStd::string sourcePath{ request.m_fullPath };
            AZStd::string dependentFile{ fileName };
            AzFramework::StringFunc::Path::ReplaceExtension(dependentFile, "dependentprocessed");
            // By default fileIO uses @asset@ alias therefore if we give fileIO a filename 
            // it will try to check in the cache instead of the source folder.
            AZ::IO::HandleType dependentfileHandle;
            if (!fileIO->Open(dependentFile.c_str(), AZ::IO::OpenMode::ModeRead, dependentfileHandle))
            {
                AZ_Error("AssetBuilder", false, " Unable to open file in cache ( %s ) while processing source ( %s ) ", dependentFile.c_str(), request.m_sourceFile.c_str());
                return;
            }

            AZ::u64 dependentsizeBytes = 0;
            if (fileIO->Size(dependentfileHandle, dependentsizeBytes))
            {
                outputData.resize(outputData.size() + dependentsizeBytes);
                if (!fileIO->Read(dependentfileHandle, outputData.data() + sourceSizeBytes, dependentsizeBytes))
                {
                    AZ_Error("AssetBuilder", false, " Unable to read file data from cache ( %s ).", dependentFile.c_str());
                    fileIO->Close(dependentfileHandle);
                    return;
                }
            }

            fileIO->Close(dependentfileHandle);
            // Validating AssetCatalogRequest API's here which operates on assetpath and assetid 
            AZ::Data::AssetId depAssetId;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(depAssetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, dependentFile.c_str(), AZ::Data::s_invalidAssetType, false);
            if (!depAssetId.IsValid())
            {
                AZ_Error("AssetBuilder", false, "GetAssetIdByPath - Asset id should be valid for this asset ( %s ).", dependentFile.c_str());
                return;
            }

            AZ::Data::AssetInfo depAssetInfo;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(depAssetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, depAssetId);
            if (!depAssetInfo.m_assetId.IsValid())
            {
                AZ_Error("AssetBuilder", false, "GetAssetInfoById - Asset info should be valid for this asset ( %s ).", depAssetId.ToString<AZStd::string>().c_str());
                return;
            }
            if (depAssetInfo.m_assetType != AZ::AzTypeInfo<TestDependentAsset>::Uuid())
            {
                AZ_Error("AssetBuilder", false, "GetAssetInfoById - Asset type not valid for asset ( %s ).", depAssetId.ToString<AZStd::string>().c_str());
                return;
            }
            AZ::Data::AssetStreamInfo resultInfo = AZ::Data::AssetManager::Instance().GetLoadStreamInfoForAsset(depAssetId, AZ::AzTypeInfo<TestDependentAsset>::Uuid());
            if (!resultInfo.IsValid())
            {
                AZ_Error("AssetBuilder", false, "GetLoadStreamInfoForAsset - AssetStreamInfo should be valid for this asset ( %s ).", depAssetId.ToString<AZStd::string>().c_str());
                return;
            }
            if (!AzFramework::StringFunc::Path::IsRelative(resultInfo.m_streamName.c_str()))
            {
                AZ_Error("AssetBuilder", false, "GetLoadStreamInfoForAsset - Source AssetStreamInfo streamName  %s isn't a relative path.", resultInfo.m_streamName.c_str());
                return;
            }

            bool gotSourceInfo{ false };
            AZStd::string watchFolder;
            AZ::Data::AssetInfo sourcePathAssetInfo;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(gotSourceInfo, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath, sourcePath.c_str(), sourcePathAssetInfo, watchFolder);
            if (!gotSourceInfo)
            {
                AZ_Error("AssetBuilder", false, "GetSourceInfoBySourcePath - Failed to get source info for source ( %s ).", sourcePath.c_str());
                return;
            }
            if (!sourcePathAssetInfo.m_assetId.IsValid())
            {
                AZ_Error("AssetBuilder", false, "GetSourceInfoBySourcePath - Asset info should be valid for asset at source path ( %s ).", sourcePath.c_str());
                return;
            }
            if (watchFolder.empty())
            {
                AZ_Error("AssetBuilder", false, "GetSourceInfoBySourcePath - Got empty watch folder for asset at source path ( %s ).", sourcePath.c_str());
                return;
            }
            if (AzFramework::StringFunc::Path::IsRelative(watchFolder.c_str()))
            {
                AZ_Error("AssetBuilder", false, "GetSourceInfoBySourcePath - Got relative path %s for source asset ( %s ).", watchFolder.c_str(), sourcePath.c_str());
                return;
            }

            bool gotAssetInfo{ false };
            AZ::Data::AssetInfo assetSystemDepInfo;
            AZStd::string depRootFolder;
            const AZStd::string platformName = ""; // Empty for default
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(gotAssetInfo, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetAssetInfoById, depAssetId, AZ::AzTypeInfo<TestDependentAsset>::Uuid(), platformName, assetSystemDepInfo, depRootFolder);
            if (!gotAssetInfo)
            {
                AZ_Error("AssetBuilder", false, "GetAssetInfoById - Failed to get info for asset ( %s ).", depAssetId.ToString<AZStd::string>().c_str());
                return;
            }
            if (assetSystemDepInfo.m_assetType != AZ::AzTypeInfo<TestDependentAsset>::Uuid())
            {
                AZ_Error("AssetBuilder", false, "GetAssetInfoById - Asset type not valid for asset ( %s ).", depAssetId.ToString<AZStd::string>().c_str());
                return;
            }

            // Validating AssetCatalogRequest API's here which operates on assetpath and assetid 
            AZ::Data::AssetId assetId;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, dependentFile.c_str(), AZ::Data::s_invalidAssetType, false);
            if (!assetId.IsValid())
            {
                AZ_Error("AssetBuilder", false, "GetAssetIdByPath - Asset id should be valid for this asset ( %s ).", dependentFile.c_str());
                return;
            }

            AZ::Data::AssetInfo assetInfo;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, assetId);
            if (!assetInfo.m_assetId.IsValid())
            {
                AZ_Error("AssetBuilder", false, "GetAssetInfoById - Asset info should be valid for this asset ( %s ).", assetId.ToString<AZStd::string>().c_str());
                return;
            }
            if (!AzFramework::StringFunc::Path::IsRelative(assetInfo.m_relativePath.c_str()))
            {
                AZ_Error("AssetBuilder", false, "GetAssetInfoById - assetInfo m_relativePath  %s isn't a relative path.", assetInfo.m_relativePath.c_str());
                return;
            }
            if (assetId.m_subId != DependentSubID)
            {
                AZ_Error("AssetBuilder", false, "GetAssetInfoById - Asset Info m_subId for %s should be %d.", assetId.ToString<AZStd::string>().c_str(), DependentSubID);
                return;
            }
            if (assetInfo.m_assetId.m_subId != assetId.m_subId)
            {
                AZ_Error("AssetBuilder", false, "GetAssetInfoById - Asset Info m_subId for %s should be %d.", assetInfo.m_relativePath.c_str(), assetId.m_subId);
                return;
            }

            AZStd::string assetpath;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetpath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, assetId);
            if (assetpath.empty())
            {
                AZ_Error("AssetBuilder", false, "Asset path should not be empty for this assetid ( %s ) ( %s )", assetId.ToString<AZStd::string>().c_str(), dependentFile.c_str());
                return;
            }
            if (!AzFramework::StringFunc::Path::IsRelative(assetpath.c_str()))
            {
                AZ_Error("AssetBuilder", false, "GetAssetPathById - assetInfo m_relativePath  %s isn't a relative path.", assetInfo.m_relativePath.c_str());
                return;
            }

            // Validate that we get the products for this asset
            bool result = false;
            AZStd::vector<AZ::Data::AssetInfo> productsInfo;

            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(result, 
                &AzToolsFramework::AssetSystemRequestBus::Events::GetAssetsProducedBySourceUUID, 
                assetId.m_guid, productsInfo);

            if (productsInfo.size() == 0)
            {
                AZ_Error("AssetBuilder", false, 
                    "GetAssetsProducedBySourceUUID - list of products can't be empty. Assetid ( %s ) ( %s )", 
                    assetId.ToString<AZStd::string>().c_str(), fileName.c_str());

                return;
            }

            AzFramework::StringFunc::Path::ReplaceExtension(fileName, "sourceprocessed");
        }
        else if (AzFramework::StringFunc::Equal(ext.c_str(), "dependent"))
        {
            AzFramework::StringFunc::Path::ReplaceExtension(fileName, "dependentprocessed");
            outputAssetType = AZ::AzTypeInfo<TestDependentAsset>::Uuid();
            outputSubId = DependentSubID;
        }
        else if (AzFramework::StringFunc::Equal(ext.c_str(), "foldertest"))
        {
            AzFramework::StringFunc::Path::ReplaceExtension(fileName, "foldertestprocessed");
        }
        else if (AzFramework::StringFunc::Equal(ext.c_str(), "slicetest"))
        {
            AzFramework::StringFunc::Path::ReplaceExtension(fileName, "slice");

            AZStd::string sourcePath{ request.m_fullPath };   // Sourcepath - full path to source slice
            AzFramework::StringFunc::Path::ReplaceExtension(sourcePath, "slice");
            AzFramework::StringFunc::Path::Normalize(sourcePath);

            // Verify copied slice in cache
            AZ::IO::HandleType dependentfileHandle;
            if (!fileIO->Open(fileName.c_str(), AZ::IO::OpenMode::ModeRead, dependentfileHandle))
            {
                AZ_Error("AssetBuilder", false, " Unable to open file in cache ( %s ) while processing source ( %s ) ", fileName.c_str(), request.m_sourceFile.c_str());
                return;
            }

            AZ::u64 dependentsizeBytes = 0;
            if (fileIO->Size(dependentfileHandle, dependentsizeBytes))
            {
                outputData.resize(outputData.size() + dependentsizeBytes);
                if (!fileIO->Read(dependentfileHandle, outputData.data() + sourceSizeBytes, dependentsizeBytes))
                {
                    AZ_Error("AssetBuilder", false, " Unable to read file data from cache ( %s ).", fileName.c_str());
                    fileIO->Close(dependentfileHandle);
                    return;
                }
            }

            fileIO->Close(dependentfileHandle);

            AZ::Data::AssetId depAssetId;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(depAssetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, fileName.c_str(), AZ::Data::s_invalidAssetType, false);
            if (!depAssetId.IsValid())
            {
                AZ_Error("AssetBuilder", false, "GetAssetIdByPath - Asset id should be valid for this asset ( %s ).", fileName.c_str());
                return;
            }

            AZ::Data::AssetInfo depAssetInfo;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(depAssetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, depAssetId);
            if (!depAssetInfo.m_assetId.IsValid())
            {
                AZ_Error("AssetBuilder", false, "GetAssetInfoById - Asset info should be valid for this asset ( %s ).", depAssetId.ToString<AZStd::string>().c_str());
                return;
            }
            if (depAssetInfo.m_assetId.m_subId != depAssetId.m_subId)
            {
                AZ_Error("AssetBuilder", false, "GetAssetInfoById - Asset Info m_subId for %s shoudl be %d.", depAssetId.ToString<AZStd::string>().c_str(), depAssetId.m_subId);
                return;
            }
            AZ::Data::AssetStreamInfo resultInfo = AZ::Data::AssetManager::Instance().GetLoadStreamInfoForAsset(depAssetId, depAssetInfo.m_assetType);
            if (!resultInfo.IsValid())
            {
                AZ_Error("AssetBuilder", false, "GetLoadStreamInfoForAsset - AssetStreamInfo should be valid for this asset ( %s ).", depAssetId.ToString<AZStd::string>().c_str());
                return;
            }
            if(AzFramework::StringFunc::Path::IsRelative(resultInfo.m_streamName.c_str()))
            {
                AZ_Error("AssetBuilder", false, "GetLoadStreamInfoForAsset - Source AssetStreamInfo streamName  %s is relative but should be absolute.", resultInfo.m_streamName.c_str());
                return;
            }
            if (resultInfo.m_streamName != sourcePath)
            {
                AZ_Error("AssetBuilder", false, "GetLoadStreamInfoForAsset - AssetStreamInfo streamName  %s isn't expected path %s.", resultInfo.m_streamName.c_str(), sourcePath.c_str());
                return;
            }

            AZStd::string relativePath;
            bool pathResult{ false };
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(pathResult, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetRelativeProductPathFromFullSourceOrProductPath, sourcePath, relativePath);
            if (!pathResult)
            {
                AZ_Error("AssetBuilder", false, "GetRelativeProductPathFromFullSourceOrProductPath - Couldn't get relative product path for ( %s ).", sourcePath.c_str());
                return;
            }
            if (relativePath != fileName)
            {
                AZ_Error("AssetBuilder", false, R"(GetRelativeProductPathFromFullSourceOrProductPath - relativePath "%s" and fileName "%s" didn't match for ( %s ).)",
                    relativePath.c_str(), fileName.c_str(), sourcePath.c_str());
                return;
            }

            AZ::Data::AssetId pathAssetId;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(pathAssetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, fileName.c_str(), AZ::Data::s_invalidAssetType, false);
            if (!pathAssetId.IsValid())
            {
                AZ_Error("AssetBuilder", false, "GetAssetIdByPath - Asset id should be valid for this asset ( %s ).", fileName.c_str());
                return;
            }

            const AZStd::string platformName = ""; // Empty for default

            bool gotAssetSystemInfoByIdFromProduct{ false };
            AZStd::string sourcePathFromProduct;
            AZ::Data::AssetInfo sliceSourceInfo;
            
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(gotAssetSystemInfoByIdFromProduct, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetAssetInfoById,
                pathAssetId, AZ::AzTypeInfo<AZ::SliceAsset>::Uuid(), platformName, sliceSourceInfo, sourcePathFromProduct);
            if (!gotAssetSystemInfoByIdFromProduct)
            {
                AZ_Error("AssetBuilder", false, "AssetSystemRequest::GetAssetInfoById - Failed to get asset info for ( %s ).", pathAssetId.ToString<AZStd::string>().c_str());
                return;
            }
            if (pathAssetId.m_subId != sliceSourceInfo.m_assetId.m_subId)
            {
                AZ_Error("AssetBuilder", false, "AssetSystemRequest::GetAssetInfoById - Response SubID should match for ( %s ) Received SubID %d.", pathAssetId.ToString<AZStd::string>().c_str(), sliceSourceInfo.m_assetId.m_subId);
                return;
            }
            if (sliceSourceInfo.m_assetType != AZ::AzTypeInfo<AZ::SliceAsset>::Uuid())
            {
                AZ_Error("AssetBuilder", false, "AssetSystemRequest::GetAssetInfoById - Lost asset type info for asset ( %s ).", pathAssetId.ToString<AZStd::string>().c_str());
                return;
            }

            // Now validate failure case
            AZ::Data::AssetId badAssetId;
            badAssetId.m_guid = AZ::Uuid::Create();
            badAssetId.m_subId = sliceSourceInfo.m_assetId.m_subId;
            gotAssetSystemInfoByIdFromProduct = false;

            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(gotAssetSystemInfoByIdFromProduct, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetAssetInfoById,
                badAssetId, AZ::AzTypeInfo<AZ::SliceAsset>::Uuid(), platformName, sliceSourceInfo, sourcePathFromProduct);
            if (gotAssetSystemInfoByIdFromProduct)
            {
                AZ_Error("AssetBuilder", false, "AssetSystemRequest::GetAssetInfoById - Got a valid result for invalid asset ( %s ).", badAssetId.ToString<AZStd::string>().c_str());
                return;
            }
            if (sliceSourceInfo.m_assetId.IsValid())
            {
                AZ_Error("AssetBuilder", false, R"(AssetSystemRequest::GetAssetInfoById - Response AssetID should not be valid for ( %s ). Received Asset ID "%s")",
                    badAssetId.ToString<AZStd::string>().c_str(), sliceSourceInfo.m_assetId.ToString<AZStd::string>().c_str());
                return;
            }
            if (badAssetId.m_subId == sliceSourceInfo.m_assetId.m_subId)
            {
                AZ_Error("AssetBuilder", false, "AssetSystemRequest::GetAssetInfoById - Response SubID should not match for ( %s ) Received SubID %d.",
                    badAssetId.ToString<AZStd::string>().c_str(), sliceSourceInfo.m_assetId.m_subId);
                return;
            }
            if (sliceSourceInfo.m_assetType != AZ::Data::s_invalidAssetType)
            {
                AZ_Error("AssetBuilder", false, R"(AssetSystemRequest::GetAssetInfoById - Response AssetType should not be valid for ( %s ). Received AssetType "%s")",
                    badAssetId.ToString<AZStd::string>().c_str(), sliceSourceInfo.m_assetType.ToString<AZStd::string>().c_str());
                return;
            }

            bool gotSourceInfo{ false };
            AZStd::string watchFolder;
            AZ::Data::AssetInfo sourcePathAssetInfo;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(gotSourceInfo, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath, sourcePath.c_str(), sourcePathAssetInfo, watchFolder);
            if (!gotSourceInfo)
            {
                AZ_Error("AssetBuilder", false, "GetSourceInfoBySourcePath - Failed to get source info for source ( %s ).", sourcePath.c_str());
                return;
            }
            if (!sourcePathAssetInfo.m_assetId.IsValid())
            {
                AZ_Error("AssetBuilder", false, "GetSourceInfoBySourcePath - Asset info should be valid for asset at source path ( %s ).", sourcePath.c_str());
                return;
            }
            if (watchFolder.empty())
            {
                AZ_Error("AssetBuilder", false, "GetSourceInfoBySourcePath - Got empty watch folder for asset at source path ( %s ).", sourcePath.c_str());
                return;
            }
            if (AzFramework::StringFunc::Path::IsRelative(watchFolder.c_str()))
            {
                AZ_Error("AssetBuilder", false, "GetSourceInfoBySourcePath - Got relative path %s for source asset ( %s ).", watchFolder.c_str(), sourcePath.c_str());
                return;
            }
            AZ::Data::AssetId sourceAssetId{ sourcePathAssetInfo.m_assetId };

            AZStd::string rootPath;
            bool gotResultAssetInfo{ false };
            AZ::Data::AssetInfo systemAssetInfo;

            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(gotResultAssetInfo, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetAssetInfoById, sourceAssetId, sourcePathAssetInfo.m_assetType, platformName, systemAssetInfo, rootPath);
            if (!gotResultAssetInfo)
            {
                AZ_Error("AssetBuilder", false, "GetAssetInfoById - Failed to get asset info for ( %s ).", sourceAssetId.ToString<AZStd::string>().c_str());
                return;
            }
            if (!systemAssetInfo.m_assetId.IsValid())
            {
                AZ_Error("AssetBuilder", false, "GetAssetInfoById - Asset info should be valid for this asset ( %s ).", sourceAssetId.ToString<AZStd::string>().c_str());
                return;
            }
            if (rootPath.empty())
            {
                AZ_Error("AssetBuilder", false, "GetAssetInfoById - Failed to get root path for ( %s ).", sourceAssetId.ToString<AZStd::string>().c_str());
                return;
            }
            if (watchFolder != rootPath)
            {
                AZ_Error("AssetBuilder", false, "GetAssetInfoById - Watch folder and root path don't match( %s vs %s  ).", watchFolder.c_str(), rootPath.c_str());
                return;
            }

            watchFolder.clear();
            AZ::Data::AssetInfo sourcePathAssetInfoByUUID;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(gotSourceInfo, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourceUUID, sourceAssetId.m_guid, sourcePathAssetInfoByUUID, watchFolder);
            if (!gotSourceInfo)
            {
                AZ_Error("AssetBuilder", false, "GetSourceInfoBySourceUUID - Asset info should be valid for asset with uuid ( %s ).", sourceAssetId.m_guid.ToString<AZ::OSString>().c_str());
                return;
            }
            if (!sourcePathAssetInfo.m_assetId.IsValid())
            {
                AZ_Error("AssetBuilder", false, "GetSourceInfoBySourceUUID - Asset info should be valid for asset with uuid ( %s ).", sourceAssetId.m_guid.ToString<AZ::OSString>().c_str());
                return;
            }
            if (watchFolder.empty())
            {
                AZ_Error("AssetBuilder", false, "GetSourceInfoBySourceUUID - Got empty watch folder for asset with uuid ( %s ).", sourceAssetId.m_guid.ToString<AZ::OSString>().c_str());
                return;
            }

            if (watchFolder != rootPath)
            {
                AZ_Error("AssetBuilder", false, "GetSourceInfoBySourceUUID - Watch folder and root path don't match( %s vs %s  ).", watchFolder.c_str(), rootPath.c_str());
                return;
            }

            AZ::Data::AssetStreamInfo sourceStreamInfo = AZ::Data::AssetManager::Instance().GetLoadStreamInfoForAsset(sourceAssetId, sourcePathAssetInfo.m_assetType);
            if (!resultInfo.IsValid())
            {
                AZ_Error("AssetBuilder", false, "GetLoadStreamInfoForAsset - Source AssetStreamInfo should be valid for this asset ( %s ).", sourceAssetId.ToString<AZStd::string>().c_str());
                return;
            }

            if (sourceStreamInfo.m_streamName != sourcePath)
            {
                AZ_Error("AssetBuilder", false, "GetLoadStreamInfoForAsset - Stream name doesn't match source path (%s vs %s ).", sourceStreamInfo.m_streamName.c_str(), sourcePath.c_str());
                return;
            };

            AzFramework::StringFunc::Path::ReplaceExtension(fileName, "slicetestout");
        }

        // write the file to the cache at (temppath)/filenameOnly
        AZStd::string destPath;
        AZStd::string fileNameOnly;
        AzFramework::StringFunc::Path::GetFullFileName(fileName.c_str(), fileNameOnly); // this removes the path from fileName.
        AzFramework::StringFunc::Path::ConstructFull(request.m_tempDirPath.c_str(), fileNameOnly.c_str(), destPath, true);

        // Check if we are cancelled or shutting down before doing intensive processing on this source file
        if (jobCancelListener.IsCancelled())
        {
            AZ_TracePrintf(AssetBuilderSDK::WarningWindow, "Cancel was requested for job %s.\n", request.m_fullPath.c_str());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            return;
        }
        if (m_isShuttingDown)
        {
            AZ_TracePrintf(AssetBuilderSDK::WarningWindow, "Cancelled job %s because shutdown was requested.\n", request.m_fullPath.c_str());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            return;
        }

        AZ::IO::HandleType assetfileHandle;
        if (!fileIO->Open(destPath.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary, assetfileHandle))
        {
            AZ_Error("AssetBuilder", false, " Unable to open file for writing ( %s ).", destPath.c_str());
            return;
        }
        if (!fileIO->Write(assetfileHandle, outputData.data(), outputData.size()))
        {
            AZ_Error("AssetBuilder", false, " Unable to write to file data ( %s ).", destPath.c_str());
            fileIO->Close(assetfileHandle);
            return;
        }
        fileIO->Close(assetfileHandle);

        AssetBuilderSDK::JobProduct jobProduct(fileNameOnly, outputAssetType, outputSubId);
        jobProduct.m_dependenciesHandled = true; // This builder has no product dependencies

        // once you've filled up the details of the product in jobProduct, add it to the result list:
        response.m_outputProducts.push_back(jobProduct);

        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;

    }

    void TestAssetBuilderComponent::ShutDown()
    {
        m_isShuttingDown = true;
    }

} // namespace TestAssetBuilder

