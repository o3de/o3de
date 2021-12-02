/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Builder/WwiseBuilderWorker.h>

#include <AzCore/Debug/Trace.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/JSON/document.h>
#include <AzCore/JSON/rapidjson.h>
#include <AzCore/StringFunc/StringFunc.h>

namespace WwiseBuilder
{
    const char WwiseBuilderWindowName[] = "WwiseBuilder";

    namespace Internal
    {
        const char SoundbankDependencyFileExtension[] = ".bankdeps";
        const char JsonDependencyKey[] = "dependencies";

        AZ::Outcome<AZStd::string, AZStd::string> GetDependenciesFromMetadata(const rapidjson::Value& rootObject, AZStd::vector<AZStd::string>& fileNames)
        {
            if (!rootObject.IsObject())
            {
                return AZ::Failure(AZStd::string("The root of the metadata file is not an object. Please regenerate the metadata for this soundbank."));
            }

            // If the file doesn't define a dependency field, then there are no dependencies.
            if (!rootObject.HasMember(JsonDependencyKey))
            {
                AZStd::string addingDefaultDependencyWarning = AZStd::string::format(
                    "Dependencies array does not exist. The file was likely manually edited. Registering a default "
                    "dependency on %s. Please regenerate the metadata for this bank.",
                    Audio::Wwise::InitBank);
                return AZ::Success(addingDefaultDependencyWarning);
            }

            const rapidjson::Value& dependenciesArray = rootObject[JsonDependencyKey];
            if (!dependenciesArray.IsArray())
            {
                return AZ::Failure(AZStd::string("Dependency field is not an array. Please regenerate the metadata for this soundbank."));
            }

            for (rapidjson::SizeType dependencyIndex = 0; dependencyIndex < dependenciesArray.Size(); ++dependencyIndex)
            {
                fileNames.push_back(dependenciesArray[dependencyIndex].GetString());
            }

            // The dependency array is empty, which likely means it was modified by hand. However, every bank is dependent
            //  on init.bnk (other than itself), so just force add it as a dependency here. and emit a warning.
            if (fileNames.size() == 0)
            {
                AZStd::string addingDefaultDependencyWarning = AZStd::string::format(
                    "Dependencies array is empty. The file was likely manually edited. Registering a default "
                    "dependency on %s. Please regenerate the metadata for this bank.",
                    Audio::Wwise::InitBank);
                return AZ::Success(addingDefaultDependencyWarning);
            }
            // Make sure init.bnk is in the dependency list. Force add it if it's not
            else if (AZStd::find(fileNames.begin(), fileNames.end(), Audio::Wwise::InitBank) == fileNames.end())
            {
                AZStd::string addingDefaultDependencyWarning = AZStd::string::format(
                    "Dependencies does not contain the initialization bank. The file was likely manually edited to remove "
                    "it, however it is necessary for all banks to have the initialization bank loaded. Registering a "
                    "default dependency on %s. Please regenerate the metadata for this bank.",
                    Audio::Wwise::InitBank);
                fileNames.push_back(Audio::Wwise::InitBank);
                return AZ::Success(addingDefaultDependencyWarning);
            }

            return AZ::Success(AZStd::string());
        }
    }


    WwiseBuilderWorker::WwiseBuilderWorker()
        : m_isShuttingDown(false)
    {
    }

    void WwiseBuilderWorker::ShutDown()
    {
        // This will be called on a different thread than the process job thread
        m_isShuttingDown = true;
    }

    void WwiseBuilderWorker::Initialize()
    {
        AZ::IO::Path configFile("@projectroot@");
        configFile /= Audio::Wwise::DefaultBanksPath;
        configFile /= Audio::Wwise::ConfigFile;

        if (AZ::IO::FileIOBase::GetInstance()->Exists(configFile.c_str()))
        {
            m_wwiseConfig.Load(configFile.Native());
        }

        m_initialized = true;
    }

    // This happens early on in the file scanning pass.
    // This function should always create the same jobs and not do any checking whether the job is up to date.
    void WwiseBuilderWorker::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
    {
        if (m_isShuttingDown)
        {
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
            return;
        }

        if (!m_initialized)
        {
            Initialize();
        }

        AZStd::string jobKey = "Wwise";
        if (AZ::StringFunc::EndsWith(request.m_sourceFile, Audio::Wwise::MediaExtension))
        {
            jobKey.append(" Media");
        }
        else if (AZ::StringFunc::EndsWith(request.m_sourceFile, Audio::Wwise::BankExtension))
        {
            jobKey.append(" Bank");
        }

        for (const AssetBuilderSDK::PlatformInfo& info : request.m_enabledPlatforms)
        {
            // If there are no platform mappings (i.e. there was no config file), we want to
            // process the job anyways.
            bool createJob = m_wwiseConfig.m_platformMappings.empty();

            // If the config file was parsed, need to filter out jobs that don't apply.
            for (const auto& platformConfig : m_wwiseConfig.m_platformMappings)
            {
                // Check if the job request should go through.
                if (info.m_identifier == platformConfig.m_assetPlatform
                    || info.m_identifier == platformConfig.m_altAssetPlatform)
                {
                    AZStd::string sourceFile(request.m_sourceFile);
                    AZStd::string_view banksPath(Audio::Wwise::DefaultBanksPath);
                    if (AZ::StringFunc::StartsWith(sourceFile, banksPath))
                    {
                        // Remove the leading banks path from the source file...
                        AZ::StringFunc::RKeep(sourceFile, banksPath.length(), true);
                    }

                    // If the source file now begins with the right Wwise platform folder, create the job...
                    if (AZ::StringFunc::StartsWith(sourceFile, platformConfig.m_wwisePlatform, true))
                    {
                        createJob = true;
                        break;
                    }
                }
            }

            if (createJob)
            {
                AssetBuilderSDK::JobDescriptor descriptor;
                descriptor.m_jobKey = jobKey;
                descriptor.m_critical = true;
                descriptor.SetPlatformIdentifier(info.m_identifier.c_str());
                descriptor.m_priority = 0;
                response.m_createJobOutputs.push_back(descriptor);
            }
        }

        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
    }

    // The request will contain the CreateJobResponse you constructed earlier, including any keys and
    // values you placed into the hash table
    void WwiseBuilderWorker::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
    {
        AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "Starting Job.\n");
        AZ::IO::PathView fullPath(request.m_fullPath);

        if (m_isShuttingDown)
        {
            AZ_TracePrintf(AssetBuilderSDK::ErrorWindow, "Cancelled job %s because shutdown was requested.\n", request.m_fullPath.c_str());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            return;
        }
        else
        {
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
            AssetBuilderSDK::JobProduct jobProduct(request.m_fullPath);

            // if the file is a bnk
            AZ::IO::PathView requestExtension = fullPath.Extension();
            if (requestExtension.Native() == Audio::Wwise::BankExtension)
            {
                AssetBuilderSDK::ProductPathDependencySet dependencyPaths;

                // Push assets back into the response's product list
                // Assets you created in your temp path can be specified using paths relative to the temp path
                // since that is assumed where you're writing stuff.
                AZ::Outcome<AZStd::string, AZStd::string> gatherProductDependenciesResponse = GatherProductDependencies(request.m_fullPath, request.m_sourceFile, dependencyPaths);
                if (!gatherProductDependenciesResponse.IsSuccess())
                {
                    AZ_Error(WwiseBuilderWindowName, false, "Dependency gathering for %s failed. %s",
                        request.m_fullPath.c_str(), gatherProductDependenciesResponse.GetError().c_str());
                }
                else
                {
                    if (gatherProductDependenciesResponse.GetValue().empty())
                    {
                        AZ_Warning(WwiseBuilderWindowName, false, gatherProductDependenciesResponse.GetValue().c_str());
                    }
                    jobProduct.m_pathDependencies = AZStd::move(dependencyPaths);
                }
            }

            response.m_outputProducts.push_back(jobProduct);
        }
    }

    AZ::Outcome<AZStd::string, AZStd::string> WwiseBuilderWorker::GatherProductDependencies(const AZStd::string& fullPath, const AZStd::string& relativePath, AssetBuilderSDK::ProductPathDependencySet& dependencies)
    {
        AZ::IO::Path bankMetadataPath(fullPath);
        bankMetadataPath.ReplaceExtension(Internal::SoundbankDependencyFileExtension);
        AZ::IO::Path relativeSoundsPath(relativePath, AZ::IO::PosixPathSeparator);
        relativeSoundsPath.RemoveFilename();
        AZStd::string success_message;

        // Look for the corresponding .bankdeps file next to the bank itself.
        if (!AZ::IO::SystemFile::Exists(bankMetadataPath.c_str()))
        {
            // If this is the init bank, skip it. Otherwise, register the init bank as a dependency, and warn that a full
            //  dependency graph can't be created without a .bankdeps file for the bank.
            AZ::IO::PathView requestFileName = AZ::IO::PathView(fullPath).Filename();
            if (requestFileName != Audio::Wwise::InitBank)
            {
                success_message = AZStd::string::format("Failed to find the metadata file %s for soundbank %s. Full dependency information cannot be determined without the metadata file. Please regenerate the metadata for this soundbank.", bankMetadataPath.c_str(), fullPath.c_str());
            }
            return AZ::Success(success_message);
        }

        AZ::u64 fileSize = AZ::IO::SystemFile::Length(bankMetadataPath.c_str());
        if (fileSize == 0)
        {
            return AZ::Failure(AZStd::string::format("Soundbank metadata file at path %s is an empty file. Please regenerate the metadata for this soundbank.", bankMetadataPath.c_str()));
        }

        AZStd::vector<char> buffer(fileSize + 1);
        buffer[fileSize] = 0;
        if (!AZ::IO::SystemFile::Read(bankMetadataPath.c_str(), buffer.data()))
        {
            return AZ::Failure(AZStd::string::format("Failed to read the soundbank metadata file at path %s. Please make sure the file is not open or being edited by another program.", bankMetadataPath.c_str()));
        }

        // load the file
        rapidjson::Document bankMetadataDoc;
        bankMetadataDoc.Parse(buffer.data());
        if (bankMetadataDoc.GetParseError() != rapidjson::ParseErrorCode::kParseErrorNone)
        {
            return AZ::Failure(AZStd::string::format("Failed to parse soundbank metadata at path %s into JSON. Please regenerate the metadata for this soundbank.", bankMetadataPath.c_str()));
        }

        AZStd::vector<AZStd::string> wwiseFiles;
        AZ::Outcome<AZStd::string, AZStd::string> gatherDependenciesResult = Internal::GetDependenciesFromMetadata(bankMetadataDoc, wwiseFiles);
        if (!gatherDependenciesResult.IsSuccess())
        {
            return AZ::Failure(AZStd::string::format("Failed to gather dependencies for %s from metadata file %s. %s", fullPath.c_str(), bankMetadataPath.c_str(), gatherDependenciesResult.GetError().c_str()));
        }
        else if (!gatherDependenciesResult.GetValue().empty())
        {
            success_message = AZStd::string::format("Dependency information for %s was unavailable in the metadata file %s. %s", fullPath.c_str(), bankMetadataPath.c_str(), gatherDependenciesResult.GetValue().c_str());
        }

        // Register dependencies stored in the file to the job response. (they'll be relative to the bank itself.)
        for (const AZStd::string& wwiseFile : wwiseFiles)
        {
            dependencies.emplace((relativeSoundsPath / wwiseFile).Native(), AssetBuilderSDK::ProductPathDependencyType::ProductFile);
        }

        return AZ::Success(success_message);
    }

} // namespace WwiseBuilder
