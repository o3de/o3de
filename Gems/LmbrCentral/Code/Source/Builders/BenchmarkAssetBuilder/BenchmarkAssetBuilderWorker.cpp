/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Builders/BenchmarkAssetBuilder/BenchmarkAssetBuilderWorker.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Math/Random.h>
#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Debug/Trace.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AssetBuilderSDK/SerializationDependencies.h>

#include <CryCommon/platform.h>

namespace BenchmarkAssetBuilder
{
    // Note - Shutdown will be called on a different thread than your process job thread
    void BenchmarkAssetBuilderWorker::ShutDown()
    {
        m_isShuttingDown = true;
    }

    // Using the BenchmarkSettingsAsset asset, create the appropriate asset generation jobs to produce
    // the requested set of BenchmarkAsset assets.
    void BenchmarkAssetBuilderWorker::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request,
                                                 AssetBuilderSDK::CreateJobsResponse& response)
    {
        if (m_isShuttingDown)
        {
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
            return;
        }

        AZStd::string ext;
        constexpr bool includeDot = false;
        AZ::StringFunc::Path::GetExtension(request.m_sourceFile.c_str(), ext, includeDot);

        // If we're reprocessing a benchmark asset, just ignore it.  There's no reason these should change
        // outside of the generation process below.
        if (AZ::StringFunc::Equal(ext.c_str(), AzFramework::s_benchmarkAssetExtension))
        {
            AZ_TracePrintf(AssetBuilderSDK::InfoWindow,
                           "Request to process benchmark asset ignored: %s\n", request.m_sourceFile.c_str()); 
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
            return;
        }

        // Load the benchmark asset settings file to determine how to generate the benchmark assets.
        // The FilterDescriptor is here to ensure that we don't try to load any dependent BenchmarkAsset
        // assets when loading the BenchmarkSettings.
        AZStd::string fullPath;
        AZ::StringFunc::Path::Join(request.m_watchFolder.c_str(), request.m_sourceFile.c_str(), fullPath, true, true);
        AZStd::unique_ptr<AzFramework::BenchmarkSettingsAsset> settingsPtr(
            AZ::Utils::LoadObjectFromFile<AzFramework::BenchmarkSettingsAsset>(
                fullPath, nullptr, AZ::Utils::FilterDescriptor(&AZ::Data::AssetFilterNoAssetLoading))
        );

        // Validate that the settings load successfully, and that the combination of settings
        // won't blow up and create an excessive amount of data.
        if (!ValidateSettings(settingsPtr))
        {
            AZ_Error(AssetBuilderSDK::ErrorWindow, false,
                     "Error during settings validation: %s.\n",  request.m_sourceFile.c_str());
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Failed;
            return;
        }

        // Generate the benchmark assets for all platforms
        for (const AssetBuilderSDK::PlatformInfo& info : request.m_enabledPlatforms)
        {
            AssetBuilderSDK::JobDescriptor descriptor;
            descriptor.m_jobKey = "Benchmark Asset Generation";
            descriptor.SetPlatformIdentifier(info.m_identifier.data());
            descriptor.m_critical = false;

            // Save off the generation parameters so that we can access them during job processing.
            ConvertSettingsToJobParameters(*settingsPtr, descriptor.m_jobParameters);

            response.m_createJobOutputs.push_back(descriptor);
        }

        // We don't need to save off any SourceFileDependency info here, since the BenchmarkSettings
        // should be the only source file.

        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
        return;
    }

    // Process the BenchmarkSettingsAsset and generate a series of BenchmarkAssets from it.
    void BenchmarkAssetBuilderWorker::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request,
                                                 AssetBuilderSDK::ProcessJobResponse& response)
    {
        // Before we begin, let's make sure we are not meant to abort.
        {
            AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);
            if (jobCancelListener.IsCancelled())
            {
                AZ_Warning(AssetBuilderSDK::WarningWindow, false,
                           "Cancel Request: Cancelled benchmark asset generation job for %s.\n", request.m_fullPath.data());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
                return;
            }

            if (m_isShuttingDown)
            {
                AZ_Warning(AssetBuilderSDK::WarningWindow, false,
                           "Shutdown Request: Cancelled benchmark asset generation job for %s.\n", request.m_fullPath.data());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
                return;
            }
        }

        AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "Performing benchmark asset generation job for %s\n", request.m_fullPath.data());

        // Fetch the settings parameters for our asset generation.
        AzFramework::BenchmarkSettingsAsset settings;
        ConvertJobParametersToSettings(request.m_jobDescription.m_jobParameters, settings);

        // Construct the output path name for the BenchmarkSettings file.  This will get saved into a temp directory.
        // The Asset Processor will handle copying the files from the temp directory to the cache on successful completion.
        AZStd::string fileName;
        AZStd::string destPath;
        AZ::StringFunc::Path::GetFullFileName(request.m_fullPath.c_str(), fileName);
        AZ::StringFunc::Path::ConstructFull(request.m_tempDirPath.c_str(), fileName.c_str(), destPath, true);

        // Copy the original BenchmarkSettings file directly into the output.
        // This is necessary to open the file in the Asset Editor inside the LY Editor.
        // Otherwise, we can *create* the BenchmarkSettings file with the Asset Editor,
        // but if we try to re-open it, the Asset Editor will try to open a BenchmarkAsset file instead.
        {
            AZ::IO::LocalFileIO fileIO;
            if (fileIO.Copy(request.m_fullPath.c_str(), destPath.c_str()) != AZ::IO::ResultCode::Success)
            {
                AZ_Error(AssetBuilderSDK::ErrorWindow, false,
                         "Error copying original benchmarksettings file: %s\n", request.m_fullPath.c_str());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                return;
            }

            // Save a reference to it in the output products.
            // We intentionally mark "dependencies handled" with no dependencies added.  Even though
            // we generate BenchmarkAsset files, we don't need to directly create dependencies bewteen the
            // BenchmarkSettings file and the generated BenchmarkAsset assets.
            AssetBuilderSDK::JobProduct jobProduct(destPath);
            jobProduct.m_productAssetType = azrtti_typeid<AzFramework::BenchmarkSettingsAsset>();
            jobProduct.m_productSubID = 0;
            jobProduct.m_dependencies.clear();
            jobProduct.m_dependenciesHandled = true;
            response.m_outputProducts.push_back(jobProduct);
        }

        // Now, generate all of the BenchmarkAsset files from the provided BenchmarkSettings.
        // This will recursively generate assets from the final leaf assets backwards so that we
        // only create assets when its dependencies already exist and can be linked to.
        {
            constexpr uint32_t curDepth = 0;
            uint32_t uniqueSubId = 1;

            // Our primary generated asset will always have the same base name as the settings file,
            // just with a different extension.
            AZStd::string generatedAssetPath(destPath);
            AZ::StringFunc::Path::ReplaceExtension(generatedAssetPath, AzFramework::s_benchmarkAssetExtension);

            response.m_resultCode = GenerateDependentAssetSubTree(settings, request.m_sourceFileUUID, request.m_sourceFile,
                                                                  generatedAssetPath, curDepth, uniqueSubId, response);
        }
    }

    // Job parameters are passed around as key/value strings, so convert our generation parameters to strings to
    // pass them over to ProcessJobs.
    void BenchmarkAssetBuilderWorker::ConvertSettingsToJobParameters(const AzFramework::BenchmarkSettingsAsset& settings,
        AssetBuilderSDK::JobParameterMap& jobParameters)
    {
        jobParameters[AZ_CRC_CE("PrimaryAssetByteSize")] = AZStd::to_string(settings.m_primaryAssetByteSize);
        jobParameters[AZ_CRC_CE("DependentAssetByteSize")] = AZStd::to_string(settings.m_dependentAssetByteSize);
        jobParameters[AZ_CRC_CE("NumAssetsPerDependency")] = AZStd::to_string(settings.m_numAssetsPerDependency);
        jobParameters[AZ_CRC_CE("DependencyDepth")] = AZStd::to_string(settings.m_dependencyDepth);
        jobParameters[AZ_CRC_CE("AssetStorageType")] =
            AZStd::to_string(aznumeric_cast<uint32_t>(settings.m_assetStorageType));
    }

    // Job parameters are passed around as key/value strings, so convert them back to concrete numeric values that
    // we can more easily use for asset generation.
    void BenchmarkAssetBuilderWorker::ConvertJobParametersToSettings(const AssetBuilderSDK::JobParameterMap& jobParameters,
        AzFramework::BenchmarkSettingsAsset& settings)
    {
        settings.m_primaryAssetByteSize = AZStd::stoul(jobParameters.at(AZ_CRC_CE("PrimaryAssetByteSize")));
        settings.m_dependentAssetByteSize = AZStd::stoul(jobParameters.at(AZ_CRC_CE("DependentAssetByteSize")));
        settings.m_numAssetsPerDependency = static_cast<uint32_t>(AZStd::stoul(jobParameters.at(AZ_CRC_CE("NumAssetsPerDependency"))));
        settings.m_dependencyDepth = static_cast<uint32_t>(AZStd::stoul(jobParameters.at(AZ_CRC_CE("DependencyDepth"))));
        settings.m_assetStorageType = static_cast<AZ::DataStream::StreamType>(
            AZStd::stoul(jobParameters.at(AZ_CRC_CE("AssetStorageType"))));
    }

    // Perform some safety checks on our settings to make sure we've got reasonable values to generate results with.
    bool BenchmarkAssetBuilderWorker::ValidateSettings(const AZStd::unique_ptr<AzFramework::BenchmarkSettingsAsset>& settingsPtr)
    {
        // If null, then something went awry when trying to deserialize the asset.
        // Maybe somebody saved the wrong type of data with the BenchmarkSettings extension?
        if (!settingsPtr)
        {
            AZ_Error(AssetBuilderSDK::ErrorWindow, false,
                     "Benchmark settings asset failed to load / deserialize.\n");
            return false;
        }

        // Set some arbitrary maximums to make sure nobody accidentally sets some terribly bad parameters.
        // We'll cap the generation at 100K unique asset files, and a total of 100GB buffer size across the
        // full generated set of files.  Note that when using text-based formats (XML, JSON), the total size
        // could physically use ~2x the cap values listed here on the storage device.
        // These maximums can be adjusted if they ever become too limiting, they're just intended to provide
        // a safety net.
        constexpr uint64_t maxNumGeneratedAssets = UINT64_C(100000);
        constexpr uint64_t maxTotalGeneratedBytes = UINT64_C(100) * UINT64_C(1024 * 1024 * 1024);

        // We always generate 1 primary asset, and optionally generate X^Y dependent assets.
        uint64_t totalNumAssets = 1 + (
            (settingsPtr->m_dependencyDepth > 0)
                ? aznumeric_cast<uint64_t>(pow(settingsPtr->m_numAssetsPerDependency, settingsPtr->m_dependencyDepth))
                : 0
            );

        if (totalNumAssets > maxNumGeneratedAssets)
        {
            AZ_Error(AssetBuilderSDK::ErrorWindow, false, "Benchmark asset generation will generate %" PRIu64 " assets, "
                "but only a max of %" PRIu64 " generated assets is allowed.",
                totalNumAssets, maxNumGeneratedAssets);
            return false;
        }

        // TotalNumAssets includes both primary and dependent assets, but we have different generated byte sizes
        // for the two types.  So the first asset gets primary byte size, and (total - 1) assets get the dependent
        // byte size.
        uint64_t totalGeneratedBytes = settingsPtr->m_primaryAssetByteSize +
            ((totalNumAssets - 1) * settingsPtr->m_dependentAssetByteSize);

        if (totalGeneratedBytes > maxTotalGeneratedBytes)
        {
            AZ_Error(AssetBuilderSDK::ErrorWindow, false,
                "Benchmark asset generation will generate %" PRIu64 " bytes, "
                "but only a max of %" PRIu64 " generated bytes is allowed.",
                totalGeneratedBytes, maxTotalGeneratedBytes);
            return false;
        }

        // Every byte in the generated buffer will cost 1 byte of storage for binary formats,
        // and 2 bytes of storage for text-based formats.
        // This is just an approximate total size because there's a bit of additional overhead
        // for asset headers and the other fields in the generated asset.
#if defined(AZ_ENABLE_TRACING)
        uint64_t approximateTotalStorageBytes =
            (settingsPtr->m_assetStorageType == AZ::DataStream::StreamType::ST_BINARY)
            ? UINT64_C(1) * totalGeneratedBytes
            : UINT64_C(2) * totalGeneratedBytes;
#endif

        AZ_TracePrintf(AssetBuilderSDK::InfoWindow,
            "Benchmark asset generation will generate %" PRIu64 " assets "
            "containing %" PRIu64 " generated bytes total in the buffer.\n"
            "This will use approximiately %" PRIu64 " total bytes of storage.\n",
            totalNumAssets, totalGeneratedBytes, approximateTotalStorageBytes);
        return true;
    }

    // Fill the buffer with deterministically random numbers.
    // We fill with random numbers instead of a constant to ensure that there aren't any compression benefits
    // happening at the OS level or anywhere else when performing our benchmark loads.
    void BenchmarkAssetBuilderWorker::FillBuffer(AZStd::vector<uint8_t>& buffer, uint64_t bufferSize, uint64_t seed)
    {
        AZ::SimpleLcgRandom random(seed);
        uint64_t randomNum = 0;

        for (uint64_t offset = 0; offset < bufferSize; offset++)
        {
            // For efficiency, we only get a new random uint64 when we've used up all the random bytes
            // from the last one.  The SimpleLcgRandom generator only produces a 48-bit random number,
            // so we only get 6 usable bytes from each u64 random number.
            constexpr uint64_t usableRandomBytes = UINT64_C(6);
            constexpr uint32_t bitsPerByte = 8;
            randomNum = ((offset % usableRandomBytes) == 0) ? random.Getu64Random() : randomNum >> bitsPerByte;
            buffer[offset] = azlossy_cast<uint8_t>(randomNum);
        }
    }

    // Recursively generate an asset and all assets that it depends on in the generated tree.
    // Ex: if we have a "test" settings file that generates 2 dependencies at a time
    // with a total depth of 3, we'll end up with the following:
    // test
    //   |- test_00000001
    //   |    |-test_00000002
    //   |    |   |-test_00000003
    //   |    |   |-test_00000004
    //   |    |-test_00000005
    //   |    |   |-test_00000006
    //   |    |   |-test_00000007
    //   |- test_00000008
    //   |    |-test_00000009
    //   ...
    // The assets themselves are all saved as subIDs of the primary BenchmarkSettings asset.
    AssetBuilderSDK::ProcessJobResultCode BenchmarkAssetBuilderWorker::GenerateDependentAssetSubTree(
                const AzFramework::BenchmarkSettingsAsset& settings,
                AZ::Uuid sourceUuid,
                const AZStd::string& sourceAssetPath,
                const AZStd::string& generatedAssetPath,
                uint32_t curDepth,
                uint32_t& uniqueSubId,
                AssetBuilderSDK::ProcessJobResponse& response)
    {
        uint32_t thisAssetSubId = uniqueSubId++;

        // Create a unique asset name by appending the asset's subID to the name.
        // This gives us a name that's both unique and predictable / uniform in size.
        AZStd::string baseName;
        AZ::StringFunc::Path::GetFileName(generatedAssetPath.c_str(), baseName);
        AZStd::string newBaseName = AZStd::string::format("%s_%08X", baseName.c_str(), thisAssetSubId);
        AZStd::string destPath(generatedAssetPath);
        AZ::StringFunc::Path::ReplaceFullName(destPath, newBaseName.c_str(), AzFramework::s_benchmarkAssetExtension);

        // Create our benchmark asset.
        AzFramework::BenchmarkAsset asset;
        asset.m_bufferSize = (curDepth == 0) ? settings.m_primaryAssetByteSize : settings.m_dependentAssetByteSize;
        asset.m_assetReferences.clear();
        asset.m_buffer.resize(asset.m_bufferSize);

        // Fill the buffer with deterministically random numbers.
        // For our random seed, we'll use the hash of the base file name.  The path isn't used in the hash
        // to ensure that we're producing deterministic results across PCs using different drives or base paths.
        FillBuffer(asset.m_buffer, asset.m_bufferSize, AZStd::hash<AZStd::string>()(newBaseName));

        // Recursively create the nested dependency tree.
        if (curDepth < settings.m_dependencyDepth)
        {
            for (uint32_t dependentAssetNum = 0; dependentAssetNum < settings.m_numAssetsPerDependency; dependentAssetNum++)
            {
                // Add the topmost asset of the tree we're about to generate to our list of direct asset references
                // inside our generated BenchmarkAsset.
                // Because we're using sub IDs, the "hint" path uses the original source path, not
                // the output name of the sub-asset.
                AZ::Data::AssetId dependentAssetId(sourceUuid, uniqueSubId);
                AZ::Data::Asset<AzFramework::BenchmarkAsset> dependentAsset(dependentAssetId,
                    azrtti_typeid<AzFramework::BenchmarkAsset>(), sourceAssetPath);
                // Force each dependent asset to use a PreLoad behavior to ensure that it needs to load before the topmost
                // benchmark asset load is considered complete.
                dependentAsset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::PreLoad);
                asset.m_assetReferences.emplace_back(dependentAsset);

                // Recursively generate the dependent asset and everything it depends on.
                AssetBuilderSDK::ProcessJobResultCode result = GenerateDependentAssetSubTree(
                    settings, sourceUuid, sourceAssetPath,
                    generatedAssetPath, curDepth + 1, uniqueSubId, response);

                if (result != AssetBuilderSDK::ProcessJobResult_Success)
                {
                    return result;
                }
            }
        }

        // Now that we've finished creating all the dependent assets, serialize out our created asset.
        bool success = AZ::Utils::SaveObjectToFile<AzFramework::BenchmarkAsset>(
            destPath, settings.m_assetStorageType, &asset);

        if (!success)
        {
            AZ_Error(AssetBuilderSDK::ErrorWindow, false,
                     "Error while saving generated file: %s\n", destPath.c_str());
            return AssetBuilderSDK::ProcessJobResult_Failed;
        }

        // Create our output JobProduct record with the appropriate calculated dependencies.
        // Note that we use a simple always-incrementing subID scheme for our generated assets,
        // so that they appear as subIDs 1-N.  The original BenchmarkSettings asset will always have subID 0.
        // This isn't strictly correct, since the subIDs aren't stable for the outputs if the input settings
        // change, but since we're always generating all the outputs it shouldn't cause any problems.
        AssetBuilderSDK::JobProduct jobProduct;
        if (!AssetBuilderSDK::OutputObject(&asset, destPath, azrtti_typeid<AzFramework::BenchmarkAsset>(),
                                           thisAssetSubId, jobProduct))
        {
            AZ_Error(AssetBuilderSDK::ErrorWindow, false, "Failed to output product dependencies.");
            return AssetBuilderSDK::ProcessJobResult_Failed;
        }
        response.m_outputProducts.push_back(jobProduct);

        return AssetBuilderSDK::ProcessJobResult_Success;
    }
}
