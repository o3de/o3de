/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SliceBuilderWorker.h"

#include <AssetBuilderSDK/SerializationDependencies.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/std/sort.h>
#include <AzCore/IO/IOUtils.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/JSON/stringbuffer.h>
#include <AzCore/JSON/writer.h>
#include <AzCore/Slice/SliceAsset.h>
#include <AzCore/Slice/SliceAssetHandler.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Component/ComponentExport.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/Slice/SliceCompilation.h>
#include <AzToolsFramework/Slice/SliceUtilities.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include "AzFramework/Asset/SimpleAsset.h"
#include <AzCore/PlatformIncl.h>

namespace SliceBuilder
{
    namespace
    {
        AzToolsFramework::Fingerprinting::TypeFingerprint CalculateFingerprintForSlice(AZ::SliceComponent& slice, const AzToolsFramework::Fingerprinting::TypeFingerprinter& typeFingerprinter, [[maybe_unused]] AZ::SerializeContext& serializeContext)
        {
            return typeFingerprinter.GenerateFingerprintForAllTypesInObject(&slice);
        }
    } // namespace anonymous

    [[maybe_unused]] static const char* const s_sliceBuilder = "SliceBuilder";
    static const char* const s_sliceBuilderSettingsFilename = "SliceBuilderSettings.json";

    SliceBuilderWorker::SliceBuilderWorker()
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        AZ_Assert(serializeContext, "SerializeContext not found");
        m_typeFingerprinter = AZStd::make_unique<AzToolsFramework::Fingerprinting::TypeFingerprinter>(*serializeContext);

        AssetBuilderSDK::AssetBuilderCommandBus::Handler::BusConnect(GetUUID());

        bool fileFound = false;

        AZ::Data::AssetInfo settingsAssetInfo;
        AZStd::string relativePath;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(fileFound, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, s_sliceBuilderSettingsFilename, settingsAssetInfo, relativePath);

        if (fileFound)
        {
            AZStd::string sliceBuilderSettingsPath;
            AzFramework::StringFunc::Path::Join(relativePath.c_str(), settingsAssetInfo.m_relativePath.c_str(), sliceBuilderSettingsPath, true, true);

            // Attempt to load the Slice Builder Settings file
            auto result = AZ::JsonSerializationUtils::ReadJsonFile(sliceBuilderSettingsPath);
            if (result.IsSuccess())
            {
                AZ::JsonSerializationResult::ResultCode serializaionResult = AZ::JsonSerialization::Load(m_settings, result.GetValue());
                if (serializaionResult.GetProcessing() == AZ::JsonSerializationResult::Processing::Halted)
                {
                    m_settingsWarning = "Error in Slice Builder Settings File.\nUsing Default Slice Builder Settings.";
                }
            }
            else
            {
                m_settingsWarning = "Failed to load Slice Builder Settings File.\nUsing Default Slice Builder Settings.";
            }
        }
        else
        {
            m_settingsWarning = "Slice Builder Settings File Missing.\nUsing Default Slice Builder Settings.";
        }
    }

    SliceBuilderWorker::~SliceBuilderWorker() = default;

    void SliceBuilderWorker::ShutDown()
    {
        m_isShuttingDown = true;
    }

    void SliceBuilderWorker::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
    {
        // Check for shutdown
        if (m_isShuttingDown)
        {
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
            return;
        }

        AZStd::string fullPath;
        AzFramework::StringFunc::Path::ConstructFull(request.m_watchFolder.c_str(), request.m_sourceFile.c_str(), fullPath, false);
        AzFramework::StringFunc::Path::Normalize(fullPath);

        AssetBuilderSDK::AssertAndErrorAbsorber assertAndErrorAbsorber(true);

        AZ_TracePrintf(s_sliceBuilder, "CreateJobs for slice \"%s\"\n", fullPath.c_str());

        // Serialize in the source slice to determine if we need to generate a .dynamicslice.
        AZStd::shared_ptr<AZ::Data::AssetDataStream> assetDataStream = AZStd::make_shared<AZ::Data::AssetDataStream>();

        // Read in the data from a file to a buffer, then hand ownership of the buffer over to the assetDataStream
        {
            AZ::IO::FileIOStream stream(fullPath.c_str(), AZ::IO::OpenMode::ModeRead);
            if (!AZ::IO::RetryOpenStream(stream))
            {
                AZ_Warning(s_sliceBuilder, false, "CreateJobs for \"%s\" failed because the source file could not be opened.", fullPath.c_str());
                return;
            }
            AZStd::vector<AZ::u8> fileBuffer(stream.GetLength());
            size_t bytesRead = stream.Read(fileBuffer.size(), fileBuffer.data());
            if (bytesRead != stream.GetLength())
            {
                AZ_Warning(s_sliceBuilder, false, "CreateJobs for \"%s\" failed because the source file could not be read.", fullPath.c_str());
                return;
            }
            assetDataStream->Open(AZStd::move(fileBuffer));
        }

        // Serialize in the source slice to determine if we need to generate a .dynamicslice.

        AZStd::vector<AssetBuilderSDK::SourceFileDependency> sourceFileDependencies;

        // Asset filter always returns false to prevent parsing dependencies, but makes note of the slice dependencies
        auto assetFilter = [&sourceFileDependencies](const AZ::Data::AssetFilterInfo& filterInfo)
        {
            if (filterInfo.m_assetType == AZ::AzTypeInfo<AZ::SliceAsset>::Uuid())
            {
                bool isSliceDependency = (filterInfo.m_loadBehavior != AZ::Data::AssetLoadBehavior::NoLoad);

                if (isSliceDependency)
                {
                    AssetBuilderSDK::SourceFileDependency dependency;
                    dependency.m_sourceFileDependencyUUID = filterInfo.m_assetId.m_guid;

                    sourceFileDependencies.push_back(dependency);
                }
            }

            return false;
        };

        AZ::Data::Asset<AZ::SliceAsset> asset;
        asset.Create(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
        AZ::SliceAssetHandler assetHandler;
        assetHandler.SetFilterFlags(AZ::ObjectStream::FilterFlags::FILTERFLAG_IGNORE_UNKNOWN_CLASSES);

        m_sliceDataPatchError = false;
        m_sliceHasLegacyDataPatches = false;

        // Listen for data patch events indicating a legacy slice file
        AZ::DataPatchNotificationBus::Handler::BusConnect();
        assetHandler.LoadAssetData(asset, assetDataStream, assetFilter);
        AZ::DataPatchNotificationBus::Handler::BusDisconnect();

        // Flush asset database events to ensure no asset references are held by closures queued on Ebuses.
        AZ::Data::AssetManager::Instance().DispatchEvents();

        // Fail gracefully if any errors occurred while serializing in the editor slice.
        // i.e. missing assets or serialization errors.
        if (assertAndErrorAbsorber.GetErrorCount() > 0)
        {
            AZ_Error("", false, "Exporting of createjobs response for \"%s\" failed due to errors loading editor slice.", fullPath.c_str());
            return;
        }

        AZ::SliceComponent* sourcePrefab = (asset.Get()) ? asset.Get()->GetComponent() : nullptr;

        if (!sourcePrefab)
        {
            AZ_Error(s_sliceBuilder, false, "Failed to find the slice component in the slice asset!");
            return; // this should fail!
        }

        bool requiresUpgrade = m_sliceHasLegacyDataPatches;
        bool sliceWritable = AZ::IO::SystemFile::IsWritable(fullPath.c_str());
        bool createDynamicSlice = sourcePrefab->IsDynamic();

        AZ::SerializeContext* context;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        AzToolsFramework::Fingerprinting::TypeFingerprint sourceSliceTypeFingerprint = CalculateFingerprintForSlice(*sourcePrefab, *m_typeFingerprinter, *context);

        const char* compilerVersion = "9";
        for (const AssetBuilderSDK::PlatformInfo& info : request.m_enabledPlatforms)
        {
            AssetBuilderSDK::JobDescriptor jobDescriptor;
            jobDescriptor.m_priority = 0;
            jobDescriptor.m_critical = true;
            jobDescriptor.m_jobKey = "Process Slice";

            jobDescriptor.SetPlatformIdentifier(info.m_identifier.c_str());
            jobDescriptor.m_additionalFingerprintInfo = AZStd::string(compilerVersion)
                .append(AZStd::string::format("|%zu", sourceSliceTypeFingerprint));

            for (const auto& sourceDependency : sourceFileDependencies)
            {
                jobDescriptor.m_jobDependencyList.emplace_back("Process Slice", info.m_identifier.c_str(), AssetBuilderSDK::JobDependencyType::Fingerprint, sourceDependency);
            }

            // Include the upgrade status of the slice in the fingerprint.
            // There are 3 possible states:
            // 1. The slice doesn't require an upgrade
            // 2. The slice requires an upgrade but it can't be upgraded.
            // 3. The slice requires and will receive an upgrade
            if (!requiresUpgrade)
            {
                jobDescriptor.m_additionalFingerprintInfo.append("|NoUpgrade");
            }
            else
            {
                if (!sliceWritable || !m_settings.m_enableSliceConversion)
                {
                    jobDescriptor.m_additionalFingerprintInfo.append("|NeedsUpgrade");
                }
                else
                {
                    jobDescriptor.m_additionalFingerprintInfo.append("|WillUpgrade");
                }
            }

            if (!m_settingsWarning.empty())
            {
                jobDescriptor.m_jobParameters.insert(AZStd::make_pair(AZ::u32(AZ_CRC("JobParam_SettingsFileWarning", 0xae8d98ac)), AZStd::string("Requires Re-save")));
            }

            if (requiresUpgrade)
            {
                jobDescriptor.m_jobParameters.insert(AZStd::make_pair(AZ::u32(AZ_CRC("JobParam_UpgradeSlice", 0x4be52dd5)), AZStd::string("Requires Re-save")));

                // Source file changes are Platform Agnostic. Avoid extra work by only scheduling it once.
                requiresUpgrade = false;
            }

            if (createDynamicSlice)
            {
                jobDescriptor.m_jobParameters.insert(AZStd::make_pair(AZ::u32(AZ_CRC("JobParam_MakeDynamicSlice", 0xa89310ab)), AZStd::string("Create Dynamic Slice")));
            }

            response.m_createJobOutputs.push_back(jobDescriptor);

            AssetBuilderSDK::JobDescriptor copyJobDescriptor("", "Editor Slice Copy", info.m_identifier.c_str());

            copyJobDescriptor.m_critical = true;
            copyJobDescriptor.m_priority = 2;
            copyJobDescriptor.m_jobParameters.insert(AZStd::make_pair(AZ::u32(AZ_CRC("JobParam_CopyJob", 0x428e33c6)), AZStd::string("Copy Slice")));

            response.m_createJobOutputs.push_back(copyJobDescriptor);
        }

        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
    }

    void SliceBuilderWorker::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const
    {
        // Check for shutdown
        if (m_isShuttingDown)
        {
            AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "Cancelled job %s because shutdown was requested.\n", request.m_sourceFile.c_str());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            return;
        }

        // Emit a settings file warning if required. We wait until now so the warnings will be clearly visible in the AP GUI.
        if (request.m_jobDescription.m_jobParameters.find(AZ_CRC("JobParam_SettingsFileWarning", 0xae8d98ac)) != request.m_jobDescription.m_jobParameters.end())
        {
            // .../dev/SliceBuilderSettings.json must exist and must be readable.
            AZ_Warning(s_sliceBuilder, false, m_settingsWarning.c_str());
        }

        AZStd::string fullPath;
        AZStd::string fileNameOnly;
        AzFramework::StringFunc::Path::GetFullFileName(request.m_sourceFile.c_str(), fileNameOnly);
        fullPath = request.m_fullPath.c_str();
        AzFramework::StringFunc::Path::Normalize(fullPath);

        AZ_TracePrintf(s_sliceBuilder, "Processing slice \"%s\".\n", fullPath.c_str());
        
        // Serialize in the source slice for processing.
        AZStd::shared_ptr<AZ::Data::AssetDataStream> assetDataStream = AZStd::make_shared<AZ::Data::AssetDataStream>();

        // Read in the data from a file to a buffer, then hand ownership of the buffer over to the assetDataStream
        {
            AZ::IO::FileIOStream stream(fullPath.c_str(), AZ::IO::OpenMode::ModeRead);
            if (!AZ::IO::RetryOpenStream(stream))
            {
                AZ_Warning(s_sliceBuilder, false, "Slice Processing for \"%s\" failed because the source file could not be opened.", fullPath.c_str());
                return;
            }
            AZStd::vector<AZ::u8> fileBuffer(stream.GetLength());
            size_t bytesRead = stream.Read(fileBuffer.size(), fileBuffer.data());
            if (bytesRead != stream.GetLength())
            {
                AZ_Warning(s_sliceBuilder, false, "Slice Processing for \"%s\" failed because the source file could not be read.", fullPath.c_str());
                return;
            }

            assetDataStream->Open(AZStd::move(fileBuffer));
        }

        AZStd::vector<AssetBuilderSDK::ProductDependency> productDependencies;
        AssetBuilderSDK::ProductPathDependencySet productPathDependencySet;
        AZ::PlatformTagSet platformTags;


        const AZStd::unordered_set<AZStd::string>& platformTagStrings = request.m_platformInfo.m_tags;
        for (const AZStd::string& platformTagString : platformTagStrings)
        {
            platformTags.insert(AZ::Crc32(platformTagString.c_str(), platformTagString.size(), true));
        }

        if (request.m_jobDescription.m_jobParameters.find(AZ_CRC("JobParam_CopyJob", 0x428e33c6)) != request.m_jobDescription.m_jobParameters.end())
        {
            AZ::Data::Asset<AZ::SliceAsset> exportSliceAsset;

            if (GetSourceSliceAsset(assetDataStream, fullPath.c_str(), exportSliceAsset))
            {
                AssetBuilderSDK::JobProduct jobProduct;

                if (AssetBuilderSDK::OutputObject(exportSliceAsset.Get()->GetEntity(), request.m_fullPath, azrtti_typeid<AZ::SliceAsset>(), AZ::SliceAsset::GetAssetSubId(), jobProduct))
                {
                    response.m_outputProducts.push_back(AZStd::move(jobProduct));
                }
            }
        }

        // Dynamic Slice Creation
        if (request.m_jobDescription.m_jobParameters.find(AZ_CRC("JobParam_MakeDynamicSlice", 0xa89310ab)) != request.m_jobDescription.m_jobParameters.end())
        {
            AZ::Data::Asset<AZ::SliceAsset> exportSliceAsset;

            if (GetCompiledSliceAsset(assetDataStream, fullPath.c_str(), platformTags, exportSliceAsset))
            {
                AZStd::string dynamicSliceOutputPath;
                AzFramework::StringFunc::Path::Join(request.m_tempDirPath.c_str(), fileNameOnly.c_str(), dynamicSliceOutputPath, true, true);
                AzFramework::StringFunc::Path::ReplaceExtension(dynamicSliceOutputPath, "dynamicslice");

                // Save runtime slice to disk.
                // Use SaveObjectToFile because it writes to a byte stream first and then to disk
                // which is much faster than SaveObjectToStream(outputStream...) when writing large slices
                if (AZ::Utils::SaveObjectToFile<AZ::Entity>(dynamicSliceOutputPath.c_str(), AzToolsFramework::SliceUtilities::GetSliceStreamFormat(), exportSliceAsset.Get()->GetEntity()))
                {
                    AZ_TracePrintf(s_sliceBuilder, "Output file %s", dynamicSliceOutputPath.c_str());
                }
                else
                {
                    AZ_Error(s_sliceBuilder, false, "Failed to open output file %s", dynamicSliceOutputPath.c_str());
                    return;
                }

                AssetBuilderSDK::JobProduct jobProduct;

                if(OutputSliceJob(exportSliceAsset, dynamicSliceOutputPath, jobProduct))
                {
                    response.m_outputProducts.push_back(AZStd::move(jobProduct));
                }
            }
        }

        // Slice Upgrades
        if (request.m_jobDescription.m_jobParameters.find(AZ_CRC("JobParam_UpgradeSlice", 0x4be52dd5)) != request.m_jobDescription.m_jobParameters.end())
        {
            AZ_TracePrintf(s_sliceBuilder, "Slice Upgrade: Starting Upgrade Process");
            // Check to see if the conditions for the builder to operate are met
            // The work is done here rather than in create jobs so that all warnings are clearly visible in the AP
            bool sliceWritable = AZ::IO::SystemFile::IsWritable(fullPath.c_str());
            if (!m_settings.m_enableSliceConversion || !sliceWritable)
            {
                [[maybe_unused]] static const char* const s_OutOfDate = "This slice file is out of date: ";
                [[maybe_unused]] static const char* const s_ToEnable = "To enable automatic upgrades:";
                [[maybe_unused]] static const char* const s_FixSettings1 = "In the settings file ";
                [[maybe_unused]] static const char* const s_FixSettings2 = ", Set 'EnableSliceConversion' to true and restart the Asset Processor";
                [[maybe_unused]] static const char* const s_FixReadOnly = "Make sure the slice file isn't marked read-only. If using perforce, check out the slice file.";

                // The Slice isn't marked as read only but Slice Upgrades aren't Enabled in the builder settings file
                if(!m_settings.m_enableSliceConversion && sliceWritable)
                {
                    AZ_Warning(s_sliceBuilder, false, "%s%s\n%s\n%s%s%s", s_OutOfDate, fullPath.c_str(), s_ToEnable, s_FixSettings1, s_sliceBuilderSettingsFilename, s_FixSettings2);
                }

                // Slice Upgrades are enabled in the builder settings file but the slice file is marked as read-only
                if (!sliceWritable && m_settings.m_enableSliceConversion)
                {
                    AZ_Warning(s_sliceBuilder, false, "%s%s\n%s\n%s", s_OutOfDate, fullPath.c_str(), s_ToEnable, s_FixReadOnly);
                }

                // Slice Upgrades are disabled in the builder settings file and the slice is marked as read-only.
                if (!m_settings.m_enableSliceConversion && !sliceWritable)
                {
                    AZ_Warning(s_sliceBuilder, false, "%s%s\n%s\n1. %s%s%s\n2. %s", s_OutOfDate, fullPath.c_str(), s_ToEnable, s_FixSettings1, s_sliceBuilderSettingsFilename, s_FixSettings2, s_FixReadOnly);
                }
            }
            else
            {
                AZ_TracePrintf(s_sliceBuilder, "Slice Upgrade: Instantiating Slice");

                AZ::SerializeContext* context;
                AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

                AZ::Data::Asset<AZ::SliceAsset> sourceAsset;
                sourceAsset.Create(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));

                AZ::SliceAssetHandler assetHandler(context);
                assetDataStream->Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
                assetHandler.LoadAssetData(sourceAsset, assetDataStream, &AZ::Data::AssetFilterSourceSlicesOnly);
                sourceAsset.SetHint(fullPath);

                // Make sure the original file stream is closed so it can be replaced
                assetDataStream.reset();

                // If the slice is designated as dynamic, generate the dynamic slice (.dynamicslice).
                AZ::SliceComponent* sourceSlice = (sourceAsset.Get()) ? sourceAsset.Get()->GetComponent() : nullptr;

                if(!sourceSlice)
                {
                    AZ_Error(s_sliceBuilder, false, "Failed to load the source file as a slice");
                    return;
                }

                if (sourceSlice->Instantiate() != AZ::SliceComponent::InstantiateResult::Success)
                {
                    AZ_Error(s_sliceBuilder, false, "Failed to Upgrade Slice - Slice Instantiation Failed.");
                    return;
                }

                AZ_TracePrintf(s_sliceBuilder, "Slice Upgrade: Recomputing Data Patches");

                // Recompute all the data patches associated with our object
                // This step is required to upgrade the data patch format stored in slices
                for (auto& slice : sourceSlice->GetSlices())
                {
                    slice.ComputeDataPatch();
                }

                // Save the slice as a new source file next to the old source file
                // Generate the new source file name
                AZStd::string tempPath(fullPath);
                AZStd::string tempFilename;
                AzFramework::StringFunc::Path::GetFileName(tempPath.c_str(), tempFilename);

                // Prepend the filename with a temporary value
                // Using the @tmp#_ prefix guarantees the new file will be ignored by the asset processor
                tempFilename.insert(0, "$tmp0_");
                AzFramework::StringFunc::Path::ReplaceFullName(tempPath, tempFilename.c_str());

                AZ_TracePrintf(s_sliceBuilder, "Slice Upgrade: Writing new slice to temporary file");

                // Save the upgraded slice to disk.
                if (!AZ::Utils::SaveObjectToFile<AZ::Entity>(tempPath.c_str(), AzToolsFramework::SliceUtilities::GetSliceStreamFormat(), sourceAsset.Get()->GetEntity()))
                {
                    AZ_Error(s_sliceBuilder, false, "Failed to Upgrade Slice - Could not open replacement slice file for writing.");
                    return;
                }

                if (!AZ::IO::SystemFile::Exists(tempPath.c_str()))
                {
                    AZ_Error(s_sliceBuilder, false, "Failed to Upgrade Slice - Could not write replacement slice file.");
                    return;
                }

                AZStd::string oldPath(fullPath);
                AZStd::string oldFilename;
                AzFramework::StringFunc::Path::GetFileName(oldPath.c_str(), oldFilename);

                // Prepend the filename with a temporary value
                // Using the @tmp#_ prefix guarantees the new file will be ignored by the asset processor
                oldFilename = "$tmp1_" + oldFilename;
                AzFramework::StringFunc::Path::ReplaceFullName(oldPath, oldFilename.c_str());

                AZ_TracePrintf(s_sliceBuilder, "Slice Upgrade: Swaping temp file with original");

                // Rename the source slice file
                if (!AZ::IO::SystemFile::Rename(fullPath.c_str(), oldPath.c_str(), true))
                {
                    AZ_Error(s_sliceBuilder, false, "Failed to Upgrade Slice - Could not rename existing file.");
                    return;
                }

                if (!AZ::IO::SystemFile::Rename(tempPath.c_str(), fullPath.c_str(), false))
                {
                    // Attempt to undo the previous rename operation to return to the original state
                    AZ::IO::SystemFile::Rename(oldPath.c_str(), fullPath.c_str(), true);

                    AZ_Error(s_sliceBuilder, false, "Failed to Upgrade Slice Could not rename new slice temp file.");
                    return;
                }

                AZ_TracePrintf(s_sliceBuilder, "Slice Upgraded: %s", fullPath.c_str());

                // To avoid potential data loss, only delete the old file if there is a data patching error detected
                if (m_sliceDataPatchError)
                {
                    [[maybe_unused]] static const char* const s_overrideWarning = "At least one Data Patch Upgrade wasn't completed:";
                    [[maybe_unused]] static const char* const s_checkLogs = "Please check the slice processing log for more information.";
                    [[maybe_unused]] static const char* const s_originalSliceAvailable = "The original slice file has been preserved at: ";
                    [[maybe_unused]] static const char* const s_recomendReload = "It's recomended that this slice be loaded into the editor and repaired before upgrading.";

                    AZ_Warning(s_sliceBuilder, false, "%s\n%s\n%s%s\n%s", s_overrideWarning, s_checkLogs, s_originalSliceAvailable, oldPath.c_str(), s_recomendReload);
                }
                else
                {
                    AZ_TracePrintf(s_sliceBuilder, "Removing original slice file.");
                    AZ::IO::SystemFile::Delete(oldPath.c_str());
                }
            }
        }

        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;

        AZ_TracePrintf(s_sliceBuilder, "Finished processing slice %s\n", fullPath.c_str());
    }

    bool SliceBuilderWorker::GetSourceSliceAsset(AZStd::shared_ptr<AZ::Data::AssetDataStream> stream, const char* fullPath, AZ::Data::Asset<AZ::SliceAsset>& sourceAsset)
    {
        AssetBuilderSDK::AssertAndErrorAbsorber assertAndErrorAbsorber(true);

        AZ::SerializeContext* context;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        sourceAsset.Create(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));

        AZ::SliceAssetHandler assetHandler(context);
        assetHandler.LoadAssetData(sourceAsset, stream, &AZ::Data::AssetFilterSourceSlicesOnly);
        sourceAsset.SetHint(fullPath);

        // Flush asset manager events to ensure no asset references are held by closures queued on Ebuses.
        AZ::Data::AssetManager::Instance().DispatchEvents();

        // Fail gracefully if any errors occurred while serializing in the editor slice.
        // i.e. missing assets or serialization errors.
        if (assertAndErrorAbsorber.GetErrorCount() > 0)
        {
            AZ_Error(s_sliceBuilder, false, "Exporting of .dynamicslice for \"%s\" failed due to errors loading editor slice.", fullPath);
            return false;
        }

        return true;
    }

    bool SliceBuilderWorker::GetCompiledSliceAsset(AZStd::shared_ptr<AZ::Data::AssetDataStream> stream, const char* fullPath,
        const AZ::PlatformTagSet& platformTags, AZ::Data::Asset<AZ::SliceAsset>& outSliceAsset)
    {
        AZ::SerializeContext* context;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        AssetBuilderSDK::AssertAndErrorAbsorber assertAndErrorAbsorber(true);
        AZ::Data::Asset<AZ::SliceAsset> sourceAsset;

        if(!GetSourceSliceAsset(stream, fullPath, sourceAsset))
        {
            return false;
        }

        // If the slice is designated as dynamic, generate the dynamic slice (.dynamicslice).
        AZ::SliceComponent* sourceSlice = (sourceAsset.Get()) ? sourceAsset.Get()->GetComponent() : nullptr;

        if (!sourceSlice)
        {
            AZ_Error(s_sliceBuilder, false, "Failed to find the slice component in the slice asset!");
            return false; // this should fail!
        }

        if (assertAndErrorAbsorber.GetErrorCount() > 0)
        {
            AZ_Error(s_sliceBuilder, false, "Loading of .dynamicslice for \"%s\" failed due to errors instantiating entities.", fullPath);
            return false;
        }

        AZ::SliceComponent::EntityList sourceEntities;
        sourceSlice->GetEntities(sourceEntities);

        // Compile the source slice into the runtime slice (with runtime components). Note that
        // we may be handling either world or UI entities, so we need handlers for both.
        AzToolsFramework::WorldEditorOnlyEntityHandler worldEditorOnlyEntityHandler;
        AzToolsFramework::UiEditorOnlyEntityHandler uiEditorOnlyEntityHandler;
        AzToolsFramework::EditorOnlyEntityHandlers handlers =
        {
            &worldEditorOnlyEntityHandler,
            &uiEditorOnlyEntityHandler,
        };
        AzToolsFramework::SliceCompilationResult sliceCompilationResult =
            AzToolsFramework::CompileEditorSlice(sourceAsset, platformTags, *context, handlers);

        if (!sliceCompilationResult)
        {
            AZ_Error("Slice compilation", false, "Slice compilation failed: %s", sliceCompilationResult.GetError().c_str());
            return false;
        }

        outSliceAsset = sliceCompilationResult.GetValue();

        return true;
    }

    bool SliceBuilderWorker::OutputSliceJob(const AZ::Data::Asset<AZ::SliceAsset>& sliceAsset, AZStd::string_view outputPath, AssetBuilderSDK::JobProduct& jobProduct)
    {
        return AssetBuilderSDK::OutputObject(sliceAsset.Get()->GetEntity(), outputPath, azrtti_typeid<AZ::DynamicSliceAsset>(), AZ::DynamicSliceAsset::GetAssetSubId(), jobProduct);
    }

    AZ::Uuid SliceBuilderWorker::GetUUID()
    {
        return AZ::Uuid::CreateString("{b92ad60c-d301-4484-8647-bb889ed717a2}");
    }

    void SliceBuilderWorker::OnLegacyDataPatchLoadFailed()
    {
        // Even if a legacy patch fails to load, the slice file is out of date and requires a re-save
        m_sliceHasLegacyDataPatches = true;

        // Note that ther is an error in the data patch so the job should be flagged with a warning to prevent
        // the old file from being removed, mitigating the risk of data loss.
        m_sliceDataPatchError = true;
    }

    void SliceBuilderWorker::OnLegacyDataPatchLoaded()
    {
        m_sliceHasLegacyDataPatches = true;
    }

    bool SliceBuilderWorker::SliceUpgradesAllowed() const
    {
        return m_settings.m_enableSliceConversion;
    }
}
