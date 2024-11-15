/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Reflect/Pass/RenderPassData.h>
#include <Pass/PassBuilder.h>

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/StringFunc/StringFunc.h>

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

        namespace PassBuilderNamespace
        {
            enum PassDependencies
            {
                Shader,
                AttachmentImage,
                Count
            };

            static const AZStd::tuple<const char*, const char*> DependencyExtensionJobKeyTable[PassDependencies::Count] =
            {
                {".shader", "Shader Asset"},
                {".attimage", "Any Asset Builder"}
            };
        }

        void PassBuilder::RegisterBuilder()
        {
            AssetBuilderSDK::AssetBuilderDesc builder;
            builder.m_name = PassBuilderJobKey;
            builder.m_version = 18; // Add Allocator to ShaderStageFunction
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
            // We use an OrderOnce job dependency to ensure that the Asset Processor knows about the
            // referenced asset, so we can make an AssetId for it later in ProcessJob. OrderOnce is
            // enough because we don't need to read any data from the asset, we just needs its ID.
            AssetBuilderSDK::JobDependency jobDependency;
            jobDependency.m_jobKey = params.jobKey;
            jobDependency.m_type = AssetBuilderSDK::JobDependencyType::OrderOnce;
            jobDependency.m_sourceFile.m_sourceFileDependencyPath = params.dependencySourceFile;
            job->m_jobDependencyList.push_back(jobDependency);
            AZ_TracePrintf(PassBuilderName, "Creating job dependency on file [%.*s] \n", AZ_STRING_ARG(params.dependencySourceFile));
            return true;
        }

        bool SetJobKeyForExtension(const AZStd::string& filePath, FindPassReferenceAssetParams& params)
        {            
            AZStd::string extension;
            StringFunc::Path::GetExtension(filePath.c_str(), extension);
            for (const auto& [dependencyExtension, jobKey] : PassBuilderNamespace::DependencyExtensionJobKeyTable)
            {
                if (extension == dependencyExtension)
                {
                    params.jobKey = jobKey;
                    return true;
                }
            }

            AZ_Error(PassBuilderName, false, "PassBuilder found a dependency with extension '%s', but does not know the corresponding job key. Add the job key for that extension to SetJobKeyForExtension in PassBuilder.cpp", extension.c_str());
            params.jobKey = "Unknown";
            return false;
        }

        // Helper function to find all assetId's and object references
        bool FindReferencedAssets(
            FindPassReferenceAssetParams& params, AssetBuilderSDK::JobDescriptor* job, AZStd::vector<AssetBuilderSDK::ProductDependency>* productDependencies)
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
                            success &= SetJobKeyForExtension(path, params);
                            success &= AddDependency(params, job);
                        }
                        else // Process Job Phase
                        {
                            auto assetIdOutcome = AssetUtils::MakeAssetId(path, subId);

                            if (assetIdOutcome)
                            {
                                assetReference->m_assetId = assetIdOutcome.GetValue();
                                productDependencies->push_back(
                                    AssetBuilderSDK::ProductDependency{assetReference->m_assetId, AZ::Data::ProductDependencyInfo::CreateFlags(Data::AssetLoadBehavior::NoLoad)}
                                );
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
            params.jobKey = "Unknown";

            if (!FindReferencedAssets(params, &job, nullptr))
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
            params.jobKey = "Unknown";

            AZStd::vector<AssetBuilderSDK::ProductDependency> productDependencies;
            if (!FindReferencedAssets(params, nullptr, &productDependencies))
            {
                return;
            }

            // --- Get destination file name and path ---

            AZStd::string destFileName;
            AZStd::string destPath;
            AzFramework::StringFunc::Path::GetFullFileName(request.m_fullPath.c_str(), destFileName);
            AzFramework::StringFunc::Path::ConstructFull(request.m_tempDirPath.c_str(), destFileName.c_str(), destPath, true);

            // --- Ensure the BindPassSrg - flag is set if the pass-data has a PipelineViewTag set
            auto& passTemplate{ passAsset.GetPassTemplate() };
            if (passTemplate && passTemplate->m_passData)
            {
                auto* passData = azrtti_cast<RenderPassData*>(passTemplate->m_passData.get());
                if (passData && !passData->m_pipelineViewTag.IsEmpty())
                {
                    // "PipelineViewTag": "MainCamera" is deprecated, discard the view-tag and set BindViewSrg to true
                    if (passData->m_pipelineViewTag == AZ::Name("MainCamera"))
                    {
                        AZ_Warning(
                            PassBuilderName,
                            false,
                            "Asset %s: '\"PipelineViewTag\": \"MainCamera\"' is deprecated, use '\"BindViewSrg\": true' instead",
                            request.m_fullPath.c_str());
                        passData->m_pipelineViewTag = AZ::Name();
                        passData->m_bindViewSrg = true;
                    }
                    else if (!passData->m_bindViewSrg)
                    {
                        // Explicitly set "PipelineViewTag": implicitly set BindViewSrg to true as well, if it isn't yet.
                        AZ_Info(
                            PassBuilderName,
                            "Asset %s: Pass has explicit PipelineViewTag '%s' -> setting \"BindViewSrg\" to true as well.",
                            request.m_fullPath.c_str(),
                            passData->m_pipelineViewTag.GetCStr());
                        passData->m_bindViewSrg = true;
                    }
                }
            }

            // --- Save the asset to binary format for production ---

            bool result = Utils::SaveObjectToFile(destPath, DataStream::ST_BINARY, &passAsset, passAssetUuid, serializeContext);
            if (result == false)
            {
                AZ_Error(PassBuilderName, false, "Failed to save asset to %s", destPath.c_str());
                return;
            }

            // --- Save output product(s) to response ---

            AssetBuilderSDK::JobProduct jobProduct(destPath, PassAsset::RTTI_Type(), 0);
            jobProduct.m_dependencies = productDependencies;
            jobProduct.m_dependenciesHandled = true;
            response.m_outputProducts.push_back(jobProduct);
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        }

    } // namespace RPI
} // namespace AZ
