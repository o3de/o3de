/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AssetBuilderSDK/SerializationDependencies.h>
#include <Common/AnyAssetBuilder.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/algorithm.h>

#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

#include <AzCore/Serialization/Json/JsonUtils.h>

#include <Atom/RPI.Edit/Common/ConvertibleSource.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>

namespace AZ
{
    namespace RPI
    {
        namespace
        {
            [[maybe_unused]] const char* AnyAssetBuilderName = "AnyAssetBuilder";
            const char* AnyAssetBuilderJobKey = "Any Asset Builder";
            const char* AnyAssetBuilderDefaultExtension = "azasset";
            const char* AnyAssetSourceExtensions[] =
            {
                "azasset",
                "attimage",
                "azbuffer",
            };
            const uint32_t NumberOfSourceExtensions = AZ_ARRAY_SIZE(AnyAssetSourceExtensions);
        }

        void AnyAssetBuilder::RegisterBuilder()
        {
            // build source extension patterns
            AZStd::vector<AssetBuilderSDK::AssetBuilderPattern> patterns(NumberOfSourceExtensions);
            AZStd::for_each(patterns.begin(), patterns.end(), [&, index = 0](AssetBuilderSDK::AssetBuilderPattern& pattern) mutable
            {
                pattern = AssetBuilderSDK::AssetBuilderPattern(AZStd::string("*.") + AnyAssetSourceExtensions[index++], AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard);
            });

            // setup builder descriptor
            AssetBuilderSDK::AssetBuilderDesc builderDescriptor;
            builderDescriptor.m_name = AnyAssetBuilderJobKey;
            builderDescriptor.m_patterns.insert(builderDescriptor.m_patterns.end(), patterns.begin(), patterns.end());
            builderDescriptor.m_busId = azrtti_typeid<AnyAssetBuilder>();
            builderDescriptor.m_createJobFunction = AZStd::bind(&AnyAssetBuilder::CreateJobs, this,
                AZStd::placeholders::_1, AZStd::placeholders::_2);
            builderDescriptor.m_processJobFunction = AZStd::bind(&AnyAssetBuilder::ProcessJob, this,
                AZStd::placeholders::_1, AZStd::placeholders::_2);
            builderDescriptor.m_version = 9;

            BusConnect(builderDescriptor.m_busId);

            AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBusTraits::RegisterBuilderInformation, builderDescriptor);
        }

        AnyAssetBuilder::~AnyAssetBuilder()
        {
            BusDisconnect();
        }
        
        void AnyAssetBuilder::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
        {
            if (m_isShuttingDown)
            {
                response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
                return;
            }

            for (const AssetBuilderSDK::PlatformInfo& platformInfo : request.m_enabledPlatforms)
            {
                AssetBuilderSDK::JobDescriptor descriptor;
                descriptor.m_jobKey = AnyAssetBuilderJobKey;
                descriptor.SetPlatformIdentifier(platformInfo.m_identifier.c_str());
                // [GFX TODO][ATOM-2830] Set 'm_critical' back to 'false' once proper fix for Atom startup issues are in 
                descriptor.m_critical = true;

                response.m_createJobOutputs.push_back(descriptor);
            }

            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
        }
        
        void AnyAssetBuilder::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
        {
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

            Outcome<AZStd::any, AZStd::string> loadResult = AZ::JsonSerializationUtils::LoadAnyObjectFromFile(request.m_fullPath);
            
            if (!loadResult.IsSuccess())
            {
                AZ_Error(AnyAssetBuilderName, false, "Failed to load file [%s] as an any asset", request.m_fullPath.c_str());
                AZ_Error(AnyAssetBuilderName, false, "Loading issues: %s", loadResult.GetError().data());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                return ;
            }

            AZStd::any loadedClass = loadResult.GetValue();
            auto loadedClassId = loadedClass.get_type_info().m_id;
            void* loadedInstance = AZStd::any_cast<void>(&loadedClass);
            
            // Apply converter if the source data class is a ConvertibleSource
            bool isConvertible = false;
            context->EnumerateBase(
                [&isConvertible](const AZ::SerializeContext::ClassData* classData, AZ::Uuid)
                {
                    if (classData && classData->m_typeId == ConvertibleSource::TYPEINFO_Uuid())
                    {
                        isConvertible = true;
                    }
                    return true;
                },
                loadedClassId);

            AZStd::shared_ptr<void> convertedData;
            TypeId outputTypeId = loadedClassId;
            void* outputData = loadedInstance;
            
            if (isConvertible)
            {
                const ConvertibleSource* convertible = reinterpret_cast<const ConvertibleSource*>(loadedInstance);

                // convert and save the converted data to outputData and its type id to outputTypeId
                bool converted = convertible->Convert(outputTypeId, convertedData);

                if (!converted)
                {
                    AZ_Error(AnyAssetBuilderName, false, "Failed to convert asset [%s]", request.m_fullPath.c_str());
                    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                    return;
                }
                outputData = convertedData.get();
            }
                        
            // Get file name from source file path, then replace the extension to generate product file name
            // Note that we preserve the file extension since it is used during reflection
            AZStd::string destFileName;
            AzFramework::StringFunc::Path::GetFullFileName(request.m_fullPath.c_str(), destFileName);

            // Construct product full path
            AZStd::string destPath;
            AzFramework::StringFunc::Path::ConstructFull(request.m_tempDirPath.c_str(), destFileName.c_str(), destPath, true);
            
            // Save the asset to binary format for production
            bool result = Utils::SaveObjectToFile(destPath, DataStream::ST_BINARY, outputData, outputTypeId, context);

            if (result == false)
            {
                AZ_Error(AnyAssetBuilderName, false, "Failed to save asset to %s", destPath.c_str());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                return;
            }
            
            // Success. Save output product(s) to response.
            // Note that we use the classId we found in the source file, unless it's a standard .azasset            
            AZStd::string sourceFileExtension;
            AzFramework::StringFunc::Path::GetExtension(request.m_fullPath.c_str(), sourceFileExtension, false);
            auto destClassId = (sourceFileExtension == AnyAssetBuilderDefaultExtension) ? AnyAsset::RTTI_Type() : loadedClassId;
            AssetBuilderSDK::JobProduct jobProduct(destPath, destClassId, 0);


            if (AssetBuilderSDK::OutputObject(outputData, outputTypeId, destPath, destClassId, 0, jobProduct))
            {
                response.m_outputProducts.push_back(jobProduct);
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
            }
        }

        void AnyAssetBuilder::ShutDown()
        {
            m_isShuttingDown = true;
        }

    } // namespace RPI_Builder
} // namespace AZ
