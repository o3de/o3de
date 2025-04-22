/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AssetBuilderSDK/SerializationDependencies.h>
#include <AzCore/Asset/AssetDataStream.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/IOUtils.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <MiniAudio/SoundAsset.h>
#include <Tools/SoundAssetBuilder.h>

namespace MiniAudio
{
    void SoundAssetBuilder::CreateJobs(
        const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const
    {
        for (const AssetBuilderSDK::PlatformInfo& platformInfo : request.m_enabledPlatforms)
        {
            AssetBuilderSDK::JobDescriptor jobDescriptor;
            jobDescriptor.m_critical = true;
            jobDescriptor.m_jobKey = "MiniSound Asset";
            jobDescriptor.SetPlatformIdentifier(platformInfo.m_identifier.c_str());

            response.m_createJobOutputs.push_back(jobDescriptor);
        }

        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
    }

    void SoundAssetBuilder::ProcessJob(
        [[maybe_unused]] const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const
    {
        const AZStd::string& fromFile = request.m_fullPath;

        AZ::Data::Asset<SoundAsset> soundAsset;
        soundAsset.Create(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));

        auto assetDataStream = AZStd::make_shared<AZ::Data::AssetDataStream>();
        // Read in the data from a file to a buffer, then hand ownership of the buffer over to the assetDataStream
        {
            AZ::IO::FileIOStream stream(fromFile.c_str(), AZ::IO::OpenMode::ModeRead);
            if (!AZ::IO::RetryOpenStream(stream))
            {
                AZ_Error("SoundAssetBuilder", false, "Source file '%s' could not be opened.", fromFile.c_str());
                return;
            }

            AZStd::vector<AZ::u8> fileBuffer(stream.GetLength());
            const size_t bytesRead = stream.Read(fileBuffer.size(), fileBuffer.data());
            if (bytesRead != stream.GetLength())
            {
                AZ_Error("SoundAssetBuilder", false, "Source file '%s' could not be read.", fromFile.c_str());
                return;
            }

            soundAsset->m_data.swap(fileBuffer);
        }

        AZStd::string filename;
        AzFramework::StringFunc::Path::GetFileName(request.m_sourceFile.c_str(), filename);

        AZStd::string currentExtension;
        AzFramework::StringFunc::Path::GetExtension(request.m_sourceFile.c_str(), currentExtension);
        AZStd::string outputExtension = currentExtension + "." + SoundAsset::FileExtension;

        AzFramework::StringFunc::Path::ReplaceExtension(filename, outputExtension.c_str());

        AZStd::string outputPath;
        AzFramework::StringFunc::Path::ConstructFull(request.m_tempDirPath.c_str(), filename.c_str(), outputPath, true);

        if (!AZ::Utils::SaveObjectToFile(outputPath, AZ::DataStream::ST_BINARY, soundAsset.Get()))
        {
            AZ_Error(__FUNCTION__, false, "Failed to save material type to file '%s'!", outputPath.c_str());
            return;
        }

        AssetBuilderSDK::JobProduct soundJobProduct;
        if (!AssetBuilderSDK::OutputObject(
                soundAsset.Get(), outputPath, azrtti_typeid<SoundAsset>(), SoundAsset::AssetSubId, soundJobProduct))
        {
            AZ_Error("SoundAssetBuilder", false, "Failed to output product dependencies.");
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
        }
        else
        {
            response.m_outputProducts.push_back(AZStd::move(soundJobProduct));
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        }
    }
} // namespace MiniAudio
