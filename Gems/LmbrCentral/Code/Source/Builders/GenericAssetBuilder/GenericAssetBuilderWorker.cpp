/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Builders/GenericAssetBuilder/GenericAssetBuilderWorker.h>
#include <AzCore/std/string/wildcard.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Component/ComponentApplicationBus.h>

namespace LmbrCentral::GenericAssetBuilder
{
    // Note - Shutdown will be called on a different thread than your process job thread
    void GenericAssetBuilderWorker::ShutDown()
    {
        m_isShuttingDown = true;
    }

    void GenericAssetBuilderWorker::CreateJobs(
        const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
    {
        if (m_isShuttingDown)
        {
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
            return;
        }

        AZStd::string ext;
        constexpr bool includeDot = false;
        AZ::StringFunc::Path::GetExtension(request.m_sourceFile.c_str(), ext, includeDot);

        // There is no need to actually load the file here, if we got here it means
        // its one of the extensions we registered for so we should just go ahead and emit a job
        // to process it.

        for (const AssetBuilderSDK::PlatformInfo& info : request.m_enabledPlatforms)
        {
            AssetBuilderSDK::JobDescriptor descriptor;
            descriptor.m_jobKey = "Generic Asset Processing";
            descriptor.SetPlatformIdentifier(info.m_identifier.data());
            descriptor.m_critical = false;
            response.m_createJobOutputs.push_back(descriptor);
        }

        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
        return;
    }

    void GenericAssetBuilderWorker::ProcessJob(
        const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
    {
        // Before we begin, let's make sure we are not meant to abort.
        {
            AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);
            if (jobCancelListener.IsCancelled())
            {
                AZ_Warning(
                    AssetBuilderSDK::WarningWindow,
                    false,
                    "Cancel Request: Cancelled generic asset processing job for %s.\n",
                    request.m_fullPath.data());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
                return;
            }

            if (m_isShuttingDown)
            {
                AZ_Warning(
                    AssetBuilderSDK::WarningWindow,
                    false,
                    "Shutdown Request: Cancelled generic asset processing job for %s.\n",
                    request.m_fullPath.data());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
                return;
            }
        }

        
        // Strategy:  Figure out which registration this asset matches, then just copy the file to the cache and register it as a product.
        // Extract dependencies as part of this processing.
        GenericAssetBuilderRegistration* whichReg = nullptr;
        for (GenericAssetBuilderRegistration& reg : m_registrations)
        {
            if (AZStd::wildcard_match(reg.m_pattern, request.m_fullPath.c_str()))
            {
                whichReg = &reg;
                break;
            }
        }

        if (!whichReg)
        {
            AZ_Error(
                AssetBuilderSDK::ErrorWindow,
                false,
                "Source File %s did not match any registered generic asset patterns.",
                request.m_fullPath.c_str());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            return;
        }

        AssetBuilderSDK::JobProduct jobProduct;
        jobProduct.m_dependenciesHandled = true;

        // Note that emitting the entire full path to source file as a product file is an actual
        // supported operation, and causes the AP to just copy the source file to the cache directly, without us having to
        // compile anything.
        jobProduct.m_productFileName = request.m_fullPath;
        jobProduct.m_productSubID = 0;
        jobProduct.m_productAssetType = whichReg->m_outputAssetTypeId;

        // populate dependencies:
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        
        if (!serializeContext)
        {
            AZ_Error("GenericAssetBuilderWorker", false, "Failed to get serialize context.");
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            return;
        }

        const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(whichReg->m_outputAssetTypeId);

        if (!classData)
        {
            AZ_Warning(
                "GenericAssetBuilderWorker",
                false,
                "Asset file [%s] (type %s) requested automatic Generic Asset Building but is not registered with the serializer. "
                "If you want this file to automatically emit dependencies, make sure you have a reflection function for it. "
                "If you have a reflection function, make sure that the reflection function is actually being called, ie, it "
                "is in a module that is loaded during asset processing.",
                    request.m_fullPath.c_str(),
                    whichReg->m_outputAssetTypeId.ToString<AZStd::string>().c_str());
        }
        else
        {
            // A filter function gets called by the serializer whenever it encounters an asset reference.
            // it contains the details about the asset, and also allows you to return true or false,
            // depending on whether you want it to go ahead and load the asset, or just leave it at 0 refcount unloaded.
            auto filterFn = [&jobProduct](const AZ::Data::AssetFilterInfo& assetInfo) -> bool
            {
                jobProduct.m_dependencies.emplace_back(AssetBuilderSDK::ProductDependency(assetInfo.m_assetId, assetInfo.m_loadBehavior));
                return false; // return false since we dont actually want to load this asset, just know about it.
            };

            AZ::ObjectStream::FilterDescriptor filterDesc(filterFn, AZ::ObjectStream::FILTERFLAG_IGNORE_UNKNOWN_CLASSES);

            void* data = AZ::Utils::LoadObjectFromFile(request.m_fullPath, whichReg->m_outputAssetTypeId, serializeContext, filterDesc);
            if (data)
            {
                // we need to delete the data using the serializer, too, since we can't cast to it here.
                // it may seem strange that we create it and destroy it but the purpose is so that the serializer,
                // during reading, will call the filter function whenever it encounters an asset reference.
                classData->m_factory->Destroy(data);
            }
        }

        response.m_outputProducts.push_back(jobProduct);
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    }

    void GenericAssetBuilderWorker::SetRegistrations(AZStd::vector<GenericAssetBuilderRegistration>&& registrations)
    {
        m_registrations = AZStd::move(registrations);
    }
} // namespace LmbrCentral::GenericAssetBuilder
