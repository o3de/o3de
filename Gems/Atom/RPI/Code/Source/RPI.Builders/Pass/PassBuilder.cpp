/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Pass/PassBuilder.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>

#include <AzCore/Asset/AssetManagerBus.h>

#include <AtomCore/Serialization/Json/JsonUtils.h>

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
            static const char* PassBuilderName = "PassBuilder";
            static const char* PassBuilderJobKey = "Pass Asset Builder";
            static const char* PassAssetExtension = "pass";
        }

        void PassBuilder::RegisterBuilder()
        {
            AssetBuilderSDK::AssetBuilderDesc builder;
            builder.m_name = PassBuilderJobKey;
            builder.m_version = 11;
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

        void PassBuilder::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const
        {
            if (m_isShuttingDown)
            {
                response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
                return;
            }

            for (const AssetBuilderSDK::PlatformInfo& platformInfo : request.m_enabledPlatforms)
            {
                AssetBuilderSDK::JobDescriptor job;
                job.m_jobKey = PassBuilderJobKey;
                job.SetPlatformIdentifier(platformInfo.m_identifier.c_str());

                // Passes are a critical part of the rendering system
                job.m_critical = true;

                response.m_createJobOutputs.push_back(job);
            }

            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
        }

        // Helper function to find all assetId's and object references
        bool PassBuilder::FindPassReferencedAssets(void* objectPtr, Uuid passAssetUuid, SerializeContext* context, AZStd::unordered_set<Data::AssetId> &referencedAssetList) const
        {
            SerializeContext::ErrorHandler errorLogger;
            errorLogger.Reset();
            
            bool foundProblems = false;

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
                        AZStd::string path = assetReference->m_filePath;
                        uint32_t subId = 0;

                        // [GFX TODO][ATOM-2789] This is a temporary hack that assumes anything ending with ":foo" is indicating the name of an SRG which is used to generate the SRG AssetId
                        if (path.find(':') != AZStd::string::npos)
                        {
                            AZStd::vector<AZStd::string> tokens;
                            AzFramework::StringFunc::Tokenize(assetReference->m_filePath.c_str(), tokens, ":");
                            if (tokens.size() == 2)
                            {
                                path = tokens[0];
                                subId = AssetUtils::CalcSrgProductSubId(Name{tokens[1]});
                            }
                            else
                            {
                                AZ_Error(PassBuilderName, false, "Path not formatted correctly [%s]", assetReference->m_filePath.c_str());
                                foundProblems = true;
                            }
                        }

                        auto assetIdOutcome = AssetUtils::MakeAssetId(path, subId);

                        if (assetIdOutcome)
                        {
                            assetReference->m_assetId = assetIdOutcome.GetValue();
                        }
                        else
                        {
                            AZ_Error(PassBuilderName, false, "Could not get AssetId for [%s]", assetReference->m_filePath.c_str());
                            foundProblems = true;
                        }
                    }
                
                    // If the asset ID is valid, add it as a dependency
                    if (assetReference->m_assetId.IsValid())
                    {
                        referencedAssetList.insert(assetReference->m_assetId);
                    }
                }
                return true;
            };

            // Setup enumeration context
            SerializeContext::EnumerateInstanceCallContext callContext(
                AZStd::move(beginCallback),
                nullptr,
                context,
                SerializeContext::ENUM_ACCESS_FOR_READ,
                &errorLogger
            );

            // Recursively iterate over all elements in the object to find asset references with the above callback
            context->EnumerateInstance(
                &callContext
                , objectPtr
                , passAssetUuid
                , nullptr
                , nullptr
            );

            return !foundProblems;
        }

        void PassBuilder::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const
        {
            // Handle job cancellation and shutdown cases
            AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);
            if (jobCancelListener.IsCancelled() || m_isShuttingDown)
            {
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
                return;
            }

            // Get serialization context
            SerializeContext* context = nullptr;
            ComponentApplicationBus::BroadcastResult(context, &ComponentApplicationBus::Events::GetSerializeContext);
            if (!context)
            {
                AZ_Assert(false, "No serialize context");
                return;
            }

            // Load PassAsset
            PassAsset passAsset;
            AZ::Outcome<void, AZStd::string> loadResult = JsonSerializationUtils::LoadObjectFromFile(passAsset, request.m_fullPath);

            if (!loadResult.IsSuccess())
            {
                AZ_Error(PassBuilderName, false, "Failed to load pass asset [%s]", request.m_fullPath.c_str());
                AZ_Error(PassBuilderName, false, "Loading issues: %s", loadResult.GetError().data());
                return;
            }

            // Find all Asset IDs we depend on
            AZStd::unordered_set<Data::AssetId> dependentList;
            Uuid passAssetUuid = AzTypeInfo<PassAsset>::Uuid();
            if (!FindPassReferencedAssets(&passAsset, passAssetUuid, context, dependentList))
            {
                return;
            }

            // Get destination file name and path
            AZStd::string destFileName;
            AZStd::string destPath;
            AzFramework::StringFunc::Path::GetFullFileName(request.m_fullPath.c_str(), destFileName);
            AzFramework::StringFunc::Path::ConstructFull(request.m_tempDirPath.c_str(), destFileName.c_str(), destPath, true);

            // Save the asset to binary format for production
            bool result = Utils::SaveObjectToFile(destPath, DataStream::ST_BINARY, &passAsset, passAssetUuid, context);
            if (result == false)
            {
                AZ_Error(PassBuilderName, false, "Failed to save asset to %s", destPath.c_str());
                return;
            }

            // Success. Save output product(s) to response
            AssetBuilderSDK::JobProduct jobProduct(destPath, PassAsset::RTTI_Type(), 0);
            for (auto& assetId : dependentList)
            {
                jobProduct.m_dependencies.emplace_back(AssetBuilderSDK::ProductDependency(assetId, 0));
            }

            jobProduct.m_dependenciesHandled = true; // We've output the dependencies immediately above so it's OK to tell the AP we've handled dependencies
            response.m_outputProducts.push_back(jobProduct);
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        }

    } // namespace RPI
} // namespace AZ
