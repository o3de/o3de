/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LevelBuilderWorker.h"
#include <AssetBuilderSDK/SerializationDependencies.h>
#include <AzCore/Asset/AssetDataStream.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Slice/SliceAsset.h>
#include <AzCore/Slice/SliceAssetHandler.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzToolsFramework/Archive/ArchiveAPI.h>
#include <AzCore/XML/rapidxml.h>
#include "AzFramework/Asset/SimpleAsset.h"
#include "AzCore/Component/Entity.h"
#include <AzCore/std/string/utf8/unchecked.h>

namespace LevelBuilder
{
    const char s_materialExtension[] = ".mtl";
    const char s_audioControlFilesLevelPath[] = "@projectroot@/libs/gameaudio/wwise/levels/%s";
    const char s_audioControlFilter[] = "*.xml";

    AZ::u64 readXmlDataLength(AZ::IO::GenericStream* stream, int& charSize)
    {
        // This code is replicated from readStringLength method found in .\dev\Code\Editor\Util\EditorUtils.h file
        // such that we do not have any Cry or Qt related dependencies.
        // The basic algorithm is that it reads in an 8 bit int, and if the length is less than 2^8,
        // then that's the length. Next it reads in a 16 bit int, and if the length is less than 2^16,
        // then that's the length. It does the same thing for 32 bit values and finally for 64 bit values.
        // The 16 bit length also indicates whether or not it's a UCS2 / wide-char Windows string, if it's
        // 0xfffe, but that comes after the first byte marker indicating there's a 16 bit length value.
        // So, if the first 3 bytes are: 0xFF, 0xFF, 0xFE, it's a 2 byte string being read in, and the real
        // length follows those 3 bytes (which may still be an 8, 16, or 32 bit length).

        // default to one byte strings
        charSize = 1;

        AZ::u8 len8;
        stream->Read(sizeof(AZ::u8), &len8);
        if (len8 < 0xff)
        {
            return len8;
        }

        AZ::u16 len16;
        stream->Read(sizeof(AZ::u16), &len16);
        if (len16 == 0xfffe)
        {
            charSize = 2;

            stream->Read(sizeof(AZ::u8), &len8);
            if (len8 < 0xff)
            {
                return len8;
            }

            stream->Read(sizeof(AZ::u16), &len16);
        }

        if (len16 < 0xffff)
        {
            return len16;
        }

        AZ::u32 len32;
        stream->Read(sizeof(AZ::u32), &len32);

        if (len32 < 0xffffffff)
        {
            return len32;
        }

        AZ::u64 len64;
        stream->Read(sizeof(AZ::u64), &len64);

        return len64;
    }



    void LevelBuilderWorker::ShutDown()
    {
        m_isShuttingDown = true;
    }

    void LevelBuilderWorker::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
    {
        if (m_isShuttingDown)
        {
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
            return;
        }

        for (const AssetBuilderSDK::PlatformInfo& info : request.m_enabledPlatforms)
        {
            AssetBuilderSDK::JobDescriptor descriptor;
            descriptor.m_jobKey = "Level Builder Job";
            descriptor.m_critical = true;
            descriptor.SetPlatformIdentifier(info.m_identifier.c_str());
            response.m_createJobOutputs.push_back(descriptor);
        }

        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
    }

    void LevelBuilderWorker::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
    {
        AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "LevelBuilderWorker Starting Job.\n");

        if (m_isShuttingDown)
        {
            AZ_TracePrintf(AssetBuilderSDK::WarningWindow, "Cancelled job %s because shutdown was requested.\n", request.m_fullPath.c_str());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            return;
        }

        AZStd::vector<AssetBuilderSDK::ProductDependency> productDependencies;
        AssetBuilderSDK::ProductPathDependencySet productPathDependencies;

        AZStd::string tempUnpackDirectory;
        AzFramework::StringFunc::Path::Join(request.m_tempDirPath.c_str(), "LevelUnpack", tempUnpackDirectory);
        AZ::IO::LocalFileIO fileIO;
        fileIO.DestroyPath(tempUnpackDirectory.c_str());
        fileIO.CreatePath(tempUnpackDirectory.c_str());
        PopulateProductDependencies(request.m_fullPath, request.m_sourceFile, tempUnpackDirectory, productDependencies, productPathDependencies);

        // level.pak needs to be copied into the cache, emitting the source as a product will have the
        // asset processor take care of that.
        AssetBuilderSDK::JobProduct jobProduct(request.m_fullPath);
        jobProduct.m_dependencies = AZStd::move(productDependencies);
        jobProduct.m_pathDependencies = AZStd::move(productPathDependencies);
        jobProduct.m_dependenciesHandled = true; // We've populated the dependencies immediately above so it's OK to tell the AP we've handled dependencies
        response.m_outputProducts.push_back(jobProduct);
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    }

    void LevelBuilderWorker::PopulateProductDependencies(
        const AZStd::string& levelPakFile,
        const AZStd::string& sourceRelativeFile,
        const AZStd::string& tempDirectory,
        AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
        AssetBuilderSDK::ProductPathDependencySet& productPathDependencies) const
    {
        PopulateOptionalLevelDependencies(sourceRelativeFile, productPathDependencies);

        std::future<bool> extractResult;
        AzToolsFramework::ArchiveCommandsBus::BroadcastResult(
            extractResult, &AzToolsFramework::ArchiveCommandsBus::Events::ExtractArchive, levelPakFile, tempDirectory);

        extractResult.wait();

        PopulateLevelSliceDependencies(tempDirectory, productDependencies, productPathDependencies);
        PopulateMissionDependencies(levelPakFile, tempDirectory, productPathDependencies);
        PopulateLevelAudioControlDependencies(levelPakFile, productPathDependencies);
    }

    AZStd::string GetLastFolderFromPath(const AZStd::string& path)
    {
        AZStd::string result(path);
        // AzFramework::StringFunc::Path::GetFolder gives different results in debug and profile, so get the last folder from the path another way.
        size_t lastSeparator(result.find_last_of(AZ_CORRECT_AND_WRONG_FILESYSTEM_SEPARATOR));
        // If it ends with a slash, strip it and try again.
        if (lastSeparator == result.length() - 1)
        {
            result = result.substr(0, result.length()-1);
            lastSeparator = result.find_last_of(AZ_CORRECT_AND_WRONG_FILESYSTEM_SEPARATOR);
        }
        return result.substr(lastSeparator + 1);
    }

    void LevelBuilderWorker::PopulateOptionalLevelDependencies(
        const AZStd::string& sourceRelativeFile,
        AssetBuilderSDK::ProductPathDependencySet& productPathDependencies) const
    {
        AZStd::string sourceLevelPakPath = sourceRelativeFile;
        AzFramework::StringFunc::Path::StripFullName(sourceLevelPakPath);

        AZStd::string levelFolderName(GetLastFolderFromPath(sourceLevelPakPath));

        // Hardcoded paths are used here instead of the defines because:
        //  The defines exist in CryEngine code that we can't link from here.
        //  We want to minimize engine changes to make it easier for game teams to incorporate these dependency improvements.

        // C3DEngine::LoadLevel attempts to load this file for the current level, if it exists.
        //      GetISystem()->LoadConfiguration(GetLevelFilePath(LEVEL_CONFIG_FILE));
        AddLevelRelativeSourcePathProductDependency("level.cfg", sourceLevelPakPath, productPathDependencies);

        // CResourceManager::PrepareLevel attemps to load this file for the current level, if it exists.
        //      string filename = PathUtil::Make(sLevelFolder, AUTO_LEVEL_RESOURCE_LIST);
        //      if (!pResList->Load(filename.c_str()))
        AddLevelRelativeSourcePathProductDependency("auto_resourcelist.txt", sourceLevelPakPath, productPathDependencies);

        // CLevelInfo::ReadMetaData() constructs a string based on levelName/LevelName.xml, and attempts to read that file.
        AZStd::string levelXml(AZStd::string::format("%s.xml", levelFolderName.c_str()));
        AddLevelRelativeSourcePathProductDependency(levelXml, sourceLevelPakPath, productPathDependencies);
    }

    void LevelBuilderWorker::AddLevelRelativeSourcePathProductDependency(
        const AZStd::string& optionalDependencyRelativeToLevel,
        const AZStd::string& sourceLevelPakPath,
        AssetBuilderSDK::ProductPathDependencySet& productPathDependencies) const
    {
        AZStd::string sourceDependency;
        AzFramework::StringFunc::Path::Join(
            sourceLevelPakPath.c_str(),
            optionalDependencyRelativeToLevel.c_str(),
            sourceDependency,
            false);
        productPathDependencies.emplace(sourceDependency, AssetBuilderSDK::ProductPathDependencyType::SourceFile);
    }

    void LevelBuilderWorker::PopulateLevelSliceDependencies(
        const AZStd::string& levelPath,
        AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
        AssetBuilderSDK::ProductPathDependencySet& productPathDependencies) const
    {
        const char levelDynamicSliceFileName[] = "mission0.entities_xml";

        AZStd::string entityFilename;
        AzFramework::StringFunc::Path::Join(levelPath.c_str(), levelDynamicSliceFileName, entityFilename);

        PopulateLevelSliceDependenciesHelper(entityFilename, productDependencies, productPathDependencies);
    }

    void LevelBuilderWorker::PopulateLevelSliceDependenciesHelper(
        const AZStd::string& levelSliceName,
        AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
        AssetBuilderSDK::ProductPathDependencySet& productPathDependencies) const
    {
        AZ::Data::Asset<AZ::SliceAsset> tempLevelSliceAsset;
        tempLevelSliceAsset.Create(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));

        AZ::u64 fileLength = 0;
        AZ::IO::FileIOBase::GetInstance()->Size(levelSliceName.c_str(), fileLength);
        AZStd::shared_ptr<AZ::Data::AssetDataStream> assetDataStream = AZStd::make_shared<AZ::Data::AssetDataStream>();
        assetDataStream->Open(levelSliceName, 0, fileLength);
        assetDataStream->BlockUntilLoadComplete();

        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        AZ::SliceAssetHandler assetHandler(context);
        assetHandler.LoadAssetData(tempLevelSliceAsset, assetDataStream, &AZ::Data::AssetFilterNoAssetLoading);

        AZ::Entity* entity = tempLevelSliceAsset.Get()->GetEntity();

        AssetBuilderSDK::GatherProductDependencies(*context, entity, productDependencies, productPathDependencies);

    }

    void LevelBuilderWorker::PopulateLevelSliceDependenciesHelper(
        AZ::Data::Asset<AZ::SliceAsset>& sliceAsset,
        AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
        AssetBuilderSDK::ProductPathDependencySet& productPathDependencies) const
    {
        AZ::Data::Asset<AZ::SliceAsset> tempLevelSliceAsset;
        tempLevelSliceAsset.Create(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));

        AZStd::shared_ptr<AZ::Data::AssetDataStream> assetDataStream = AZStd::make_shared<AZ::Data::AssetDataStream>();

        // Create a buffer containing the asset, and hand ownership over to the assetDataStream
        {
            AZ::SliceAssetHandler assetHandler;
            assetHandler.SetSerializeContext(nullptr);

            AZStd::vector<AZ::u8> charBuffer;
            AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> charStream(&charBuffer);
            assetHandler.SaveAssetData(sliceAsset, &charStream);

            assetDataStream->Open(AZStd::move(charBuffer));
        }

        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        AZ::SliceAssetHandler assetHandler(context);
        assetHandler.LoadAssetData(tempLevelSliceAsset, assetDataStream, &AZ::Data::AssetFilterNoAssetLoading);

        AZ::Entity* entity = tempLevelSliceAsset.Get()->GetEntity();

        AssetBuilderSDK::GatherProductDependencies(*context, entity, productDependencies, productPathDependencies);

    }

    void LevelBuilderWorker::PopulateMissionDependencies(
        [[maybe_unused]] const AZStd::string& levelPakFile,
        const AZStd::string& levelPath,
        AssetBuilderSDK::ProductPathDependencySet& productDependencies) const
    {
        const char* fileName = "mission_mission0.xml";

        AZStd::string fileFullPath;
        AzFramework::StringFunc::Path::Join(levelPath.c_str(), fileName, fileFullPath);

        AZ::IO::FileIOStream fileStream;

        if (fileStream.Open(fileFullPath.c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary))
        {
            PopulateMissionDependenciesHelper(&fileStream, productDependencies);
        }
    }

    void LevelBuilderWorker::PopulateLevelAudioControlDependenciesHelper(
        const AZStd::string& levelName, 
        AssetBuilderSDK::ProductPathDependencySet& productDependencies) const
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetDirectInstance();
        AZ::IO::FileIOBase::FindFilesCallbackType registerFoundFileAsProductPathDependencyCallback = [&productDependencies](const char* aliasedFilePath)->bool {
            // remove the alias at the front of path passed in to get the path relative to the cache.
            AZStd::string relativePath = aliasedFilePath;
            AzFramework::StringFunc::RKeep(relativePath, relativePath.find_first_of('/'));

            productDependencies.emplace(relativePath.c_str(), AssetBuilderSDK::ProductPathDependencyType::ProductFile);
            return true;
        };

        AZStd::string levelScopedControlsPath = AZStd::string::format(s_audioControlFilesLevelPath, levelName.c_str());
        if (fileIO->IsDirectory(levelScopedControlsPath.c_str()))
        {
            fileIO->FindFiles(levelScopedControlsPath.c_str(), s_audioControlFilter, registerFoundFileAsProductPathDependencyCallback);
        }
    }

    void LevelBuilderWorker::PopulateLevelAudioControlDependencies(
        const AZStd::string& levelPakFile, 
        AssetBuilderSDK::ProductPathDependencySet& productDependencies) const
    {
        AZStd::string normalizedPakPath = levelPakFile;
        AzFramework::StringFunc::Path::Normalize(normalizedPakPath);

        AZStd::string levelName;
        AzFramework::StringFunc::Path::GetFolder(normalizedPakPath.c_str(), levelName);

        // modify the level name to the scope name that the audio controls editor would use
        AZStd::to_lower(levelName.begin(), levelName.end());

        PopulateLevelAudioControlDependenciesHelper(levelName, productDependencies);
    }

    bool GetAttribute(const AZ::rapidxml::xml_node<char>* parentNode, const char* childNodeName, const char* attributeName, const char*& outValue)
    {
        const auto* childNode = parentNode->first_node(childNodeName);

        if (!childNode)
        {
            return false;
        }

        const auto* attribute = childNode->first_attribute(attributeName);

        if (!attribute)
        {
            return false;
        }

        outValue = attribute->value();

        return true;
    }

    bool AddAttribute(const AZ::rapidxml::xml_node<char>* parentNode, const char* childNodeName, const char* attributeName, bool required, const char* extensionToAppend, AssetBuilderSDK::ProductPathDependencySet& dependencySet)
    {
        const char* attributeValue = nullptr;

        if (!GetAttribute(parentNode, childNodeName, attributeName, attributeValue) && required)
        {
            return false;
        }

        if (attributeValue && strlen(attributeValue))
        {
            dependencySet.emplace(AZStd::string(attributeValue) + (extensionToAppend ? extensionToAppend : ""), AssetBuilderSDK::ProductPathDependencyType::ProductFile);
        }

        return true;
    }

    bool LevelBuilderWorker::PopulateMissionDependenciesHelper(AZ::IO::GenericStream* stream,
        AssetBuilderSDK::ProductPathDependencySet& productDependencies) const
    {
        if (!stream)
        {
            return false;
        }

        AZ::IO::SizeType length = stream->GetLength();

        if (length == 0)
        {
            return false;
        }

        AZStd::vector<char> charBuffer;

        charBuffer.resize_no_construct(length + 1);
        stream->Read(length, charBuffer.data());
        charBuffer.back() = 0;

        AZ::rapidxml::xml_document<char> xmlDoc;
        xmlDoc.parse<AZ::rapidxml::parse_no_data_nodes>(charBuffer.data());
        
        const auto* missionNode = xmlDoc.first_node("Mission");

        if (!missionNode)
        {
            return false;
        }

        const auto* environmentNode = missionNode->first_node("Environment");

        if (!environmentNode)
        {
            return false;
        }

        if(!AddAttribute(environmentNode, "SkyBox", "Material", true, s_materialExtension, productDependencies)
            || !AddAttribute(environmentNode, "SkyBox", "MaterialLowSpec", true, s_materialExtension, productDependencies)
            || !AddAttribute(environmentNode, "Ocean", "Material", true, s_materialExtension, productDependencies)
            || !AddAttribute(environmentNode, "Moon", "Texture", false, nullptr, productDependencies)
            || !AddAttribute(environmentNode, "CloudShadows", "CloudShadowTexture", false, nullptr, productDependencies))
        {
            return false;
        }

        return true;
    }
}
