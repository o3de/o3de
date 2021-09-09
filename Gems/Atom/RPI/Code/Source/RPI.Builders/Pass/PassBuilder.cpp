/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Pass/PassBuilder.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>

#include <AzCore/Asset/AssetManagerBus.h>

#include <AzCore/Serialization/Json/JsonUtils.h>

#include <Atom/RPI.Reflect/Asset/AssetReference.h>
#include <Atom/RPI.Reflect/Pass/PassAsset.h>

#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

#include <AssetBuilderSDK/AssetBuilderSDK.h>

namespace AZ
{
    namespace RPI
    {
        namespace
        {
            [[maybe_unused]] static const char* PassBuilderName = "PassBuilder";
            static const char* PassBuilderJobKey = "Pass Asset Builder";
            static const char* PassAssetExtension = "pass";
        }

        void PassBuilder::RegisterBuilder()
        {
            AssetBuilderSDK::AssetBuilderDesc builder;
            builder.m_name = PassBuilderJobKey;
            builder.m_version = 13; // antonmic: making .pass files declare dependency on shaders they reference
            builder.m_busId = azrtti_typeid<PassBuilder>();
            builder.m_createJobFunction = AZStd::bind(&PassBuilder::CreateJobs, this, AZStd::placeholders::_1, AZStd::placeholders::_2);
            builder.m_processJobFunction = AZStd::bind(&PassBuilder::ProcessJob, this, AZStd::placeholders::_1, AZStd::placeholders::_2);

            // Match *.pass extension
            builder.m_patterns.emplace_back(
                AssetBuilderSDK::AssetBuilderPattern(
                    AZStd::string("*.") + PassAssetExtension,
                    AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard
                )
            );

            BusConnect(builder.m_busId);

            AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBus::Handler::RegisterBuilderInformation, builder);
        }

        PassBuilder::~PassBuilder()
        {
            BusDisconnect();
        }

        void PassBuilder::ShutDown()
        {
            m_isShuttingDown = true;
        }

        // --- Code related to dependency shader asset handling ---

        // Helper class to pass parameters to the AddDependency and FindReferencedAssets functions below
        struct FindPassReferenceAssetParams
        {
            void* passAssetObject = nullptr;
            Uuid passAssetUuid;
            SerializeContext* serializeContext = nullptr;
            AZStd::string_view passAssetSourceFile;         // File path of the pass asset
            AZStd::string_view dependencySourceFile;        // File pass of the asset the pass asset depends on
            const char* jobKey = nullptr;                   // Job key for adding job dependency
        };

        // Helper function to get a file reference and create a corresponding job dependency
        bool AddDependency(FindPassReferenceAssetParams& params, AssetBuilderSDK::JobDescriptor* job)
        {
            AZStd::string_view& file = params.dependencySourceFile;
            AZ::Data::AssetInfo sourceInfo;
            AZStd::string watchFolder;
            bool fileFound = false;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(fileFound, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath, file.data(), sourceInfo, watchFolder);

            if (fileFound)
            {
                AssetBuilderSDK::JobDependency jobDependency;
                jobDependency.m_jobKey = params.jobKey;
                jobDependency.m_type = AssetBuilderSDK::JobDependencyType::Order;
                jobDependency.m_sourceFile.m_sourceFileDependencyPath = file;
                job->m_jobDependencyList.push_back(jobDependency);
                AZ_TracePrintf(PassBuilderName, "Creating job dependency on file [%s] \n", file.data());
                return true;
            }
            else
            {
                AZ_Error(PassBuilderName, false, "Could not find referenced file [%s]", file.data());
                return false;
            }
        }

        // Helper function to find all assetId's and object references
        bool FindReferencedAssets(FindPassReferenceAssetParams& params, AssetBuilderSDK::JobDescriptor* job)
        {
            SerializeContext::ErrorHandler errorLogger;
            errorLogger.Reset();

            bool success = true;

            // This callback will check whether the given element is an asset reference. If so, it will add it to the list of asset references
            auto beginCallback = [&](void* ptr, const SerializeContext::ClassData* classData, [[maybe_unused]] const SerializeContext::ClassElement* classElement)
            {
                // If the enumerated element is an asset reference
                if (classData->m_typeId == azrtti_typeid<AssetReference>())
                {
                    AssetReference* assetReference = reinterpret_cast<AssetReference*>(ptr);

                    // If the asset id isn't already provided, get it using the source file path
                    if (!assetReference->m_assetId.IsValid() && !assetReference->m_filePath.empty())
                    {
                        const AZStd::string& path = assetReference->m_filePath;
                        uint32_t subId = 0;

                        if (job != nullptr) // Create Job Phase
                        {
                            params.dependencySourceFile = path;
                            bool dependencyAddedSuccessfully = AddDependency(params, job);
                            success = dependencyAddedSuccessfully && success;
                        }
                        else // Process Job Phase
                        {
                            auto assetIdOutcome = AssetUtils::MakeAssetId(path, subId);

                            if (assetIdOutcome)
                            {
                                assetReference->m_assetId = assetIdOutcome.GetValue();
                            }
                            else
                            {
                                AZ_Error(PassBuilderName, false, "Could not get AssetId for [%s]", assetReference->m_filePath.c_str());
                                success = false;
                            }
                        }
                    }
                }
                return true;
            };

            // Setup enumeration context
            SerializeContext::EnumerateInstanceCallContext callContext(
                AZStd::move(beginCallback),
                nullptr,
                params.serializeContext,
                SerializeContext::ENUM_ACCESS_FOR_READ,
                &errorLogger
            );

            // Recursively iterate over all elements in the object to find asset references with the above callback
            params.serializeContext->EnumerateInstance(
                &callContext
                , params.passAssetObject
                , params.passAssetUuid
                , nullptr
                , nullptr
            );

            return success;
        }

        // --- Code related to dependency shader asset handling ---

        void PassBuilder::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const
        {
            // --- Handle shutdown case ---

            if (m_isShuttingDown)
            {
                response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
                return;
            }

            // --- Get serialization context ---

            SerializeContext* serializeContext = nullptr;
            ComponentApplicationBus::BroadcastResult(serializeContext, &ComponentApplicationBus::Events::GetSerializeContext);
            if (!serializeContext)
            {
                AZ_Assert(false, "No serialize context");
                return;
            }

            // --- Load PassAsset ---

            AZStd::string fullPath;
            AzFramework::StringFunc::Path::ConstructFull(request.m_watchFolder.c_str(), request.m_sourceFile.c_str(), fullPath, true);

            PassAsset passAsset;
            AZ::Outcome<void, AZStd::string> loadResult = JsonSerializationUtils::LoadObjectFromFile(passAsset, fullPath);

            if (!loadResult.IsSuccess())
            {
                AZ_Error(PassBuilderName, false, "Failed to load pass asset [%s]", request.m_sourceFile.c_str());
                AZ_Error(PassBuilderName, false, "Loading issues: %s", loadResult.GetError().data());
                return;
            }

            AssetBuilderSDK::JobDescriptor job;
            job.m_jobKey = PassBuilderJobKey;
            job.m_critical = true;                  // Passes are a critical part of the rendering system

            // --- Find all dependencies ---

            AZStd::unordered_set<Data::AssetId> dependentList;
            Uuid passAssetUuid = AzTypeInfo<PassAsset>::Uuid();

            FindPassReferenceAssetParams params;
            params.passAssetObject = &passAsset;
            params.passAssetSourceFile = request.m_sourceFile;
            params.passAssetUuid = passAssetUuid;
            params.serializeContext = serializeContext;
            params.jobKey = "Shader Asset";

            if (!FindReferencedAssets(params, &job))
            {
                return;
            }

            // --- Create a job per platform ---

            for (const AssetBuilderSDK::PlatformInfo& platformInfo : request.m_enabledPlatforms)
            {
                for (auto& jobDependency : job.m_jobDependencyList)
                {
                    jobDependency.m_platformIdentifier = platformInfo.m_identifier.c_str();
                }
                job.SetPlatformIdentifier(platformInfo.m_identifier.c_str());
                response.m_createJobOutputs.push_back(job);
            }

            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
        }

        void PassBuilder::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const
        {
            // --- Handle job cancellation and shutdown cases ---

            AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);
            if (jobCancelListener.IsCancelled() || m_isShuttingDown)
            {
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
                return;
            }

            // --- Get serialization context ---

            SerializeContext* serializeContext = nullptr;
            ComponentApplicationBus::BroadcastResult(serializeContext, &ComponentApplicationBus::Events::GetSerializeContext);
            if (!serializeContext)
            {
                AZ_Assert(false, "No serialize context");
                return;
            }

            // --- Load PassAsset ---

            PassAsset passAsset;
            AZ::Outcome<void, AZStd::string> loadResult = JsonSerializationUtils::LoadObjectFromFile(passAsset, request.m_fullPath);

            if (!loadResult.IsSuccess())
            {
                AZ_Error(PassBuilderName, false, "Failed to load pass asset [%s]", request.m_fullPath.c_str());
                AZ_Error(PassBuilderName, false, "Loading issues: %s", loadResult.GetError().data());
                return;
            }

            // --- Find all dependencies ---

            Uuid passAssetUuid = AzTypeInfo<PassAsset>::Uuid();

            FindPassReferenceAssetParams params;
            params.passAssetObject = &passAsset;
            params.passAssetSourceFile = request.m_sourceFile;
            params.passAssetUuid = passAssetUuid;
            params.serializeContext = serializeContext;
            params.jobKey = "Shader Asset";

            if (!FindReferencedAssets(params, nullptr))
            {
                return;
            }

            // --- Get destination file name and path ---

            AZStd::string destFileName;
            AZStd::string destPath;
            AzFramework::StringFunc::Path::GetFullFileName(request.m_fullPath.c_str(), destFileName);
            AzFramework::StringFunc::Path::ConstructFull(request.m_tempDirPath.c_str(), destFileName.c_str(), destPath, true);

            // --- Save the asset to binary format for production ---

            bool result = Utils::SaveObjectToFile(destPath, DataStream::ST_BINARY, &passAsset, passAssetUuid, serializeContext);
            if (result == false)
            {
                AZ_Error(PassBuilderName, false, "Failed to save asset to %s", destPath.c_str());
                return;
            }

            // --- Save output product(s) to response ---

            AssetBuilderSDK::JobProduct jobProduct(destPath, PassAsset::RTTI_Type(), 0);
            jobProduct.m_dependenciesHandled = true;
            response.m_outputProducts.push_back(jobProduct);
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        }

    } // namespace RPI
} // namespace AZ
