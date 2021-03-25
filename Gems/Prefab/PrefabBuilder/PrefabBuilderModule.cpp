/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/Component/Component.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Module/Module.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponent.h>
#include <AzToolsFramework/Prefab/Spawnable/PrefabConversionPipeline.h>

namespace AZ::Prefab
{
    class PrefabBuilderComponent
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
    {
    public:
        PrefabBuilderComponent()
            : m_builderId("{A2E0791C-4607-4363-A7FD-73D01ED49660}")
        {
            AssetBuilderSDK::AssetBuilderCommandBus::Handler::BusConnect(m_builderId);
        }

        ~PrefabBuilderComponent()
        {
            if (m_prefabSystem)
            {
                m_prefabSystem->Deactivate();
            }

            AssetBuilderSDK::AssetBuilderCommandBus::Handler::BusDisconnect(m_builderId);
        }

        void Register()
        {
            AssetBuilderSDK::AssetBuilderDesc builderDesc;
            builderDesc.m_name = "Prefab Builder";
            builderDesc.m_patterns.emplace_back("*.prefab", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard);
            builderDesc.m_builderType = AssetBuilderSDK::AssetBuilderDesc::AssetBuilderType::Internal;
            builderDesc.m_busId = m_builderId;
            builderDesc.m_createJobFunction =
                [this](const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
                {
                    CreateJobs(request, response);
                };
            builderDesc.m_processJobFunction =
                [this](const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
                {
                    ProcessJob(request, response);
                };

            AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBusTraits::RegisterBuilderInformation, builderDesc);
        }

        void ShutDown() override
        {
            m_isShuttingDown = true;
        }

    private:
        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
        {
            if (m_isShuttingDown)
            {
                response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
                return;
            }

            for (const AssetBuilderSDK::PlatformInfo& info : request.m_enabledPlatforms)
            {
                AssetBuilderSDK::JobDescriptor job;
                job.m_jobKey = "Prefabs";
                job.SetPlatformIdentifier(info.m_identifier.c_str());
                response.m_createJobOutputs.push_back(AZStd::move(job));
            }

            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
        }

        bool StoreProducts(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response,
            const AzToolsFramework::Prefab::PrefabConversionUtils::PrefabProcessorContext::ProcessedObjectStoreContainer& store)
        {
            response.m_outputProducts.reserve(store.size());

            AZStd::vector<uint8_t> data;
            for (auto& object : store)
            {
                AZ_TracePrintf("Prefab Builder", "    Serializing Prefab product '%s'.\n", object.GetId().c_str());
                if (!object.Serialize(data))
                {
                    AZ_Error("Prefab Builder", false, "Failed to serialize object '%s'.", object.GetId().c_str());
                    return false;
                }

                AZ::IO::Path productPath = request.m_tempDirPath;
                productPath /= object.GetId();
                
                AZ_TracePrintf("Prefab Builder", "    Storing Prefab product '%s'.\n", object.GetId().c_str());

                AZ::IO::SystemFile productFile;
                if (!productFile.Open(productPath.c_str(),
                    AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY | AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH))
                {
                    AZ_Error("Prefab Builder", false, "Unable to open product file at '%s'.", productPath.c_str());
                    return false;
                }

                if (productFile.Write(data.data(), data.size()) != data.size())
                {
                    AZ_Error("Prefab Builder", false, "Unable to write product file at '%s'.", productPath.c_str());
                    return false;
                }

                AssetBuilderSDK::JobProduct product;
                product.m_productFileName = productPath.String();
                product.m_productAssetType = object.GetAssetType();
                product.m_productSubID = object.BuildSubId();

                // TODO: Handle dependencies.
                product.m_dependenciesHandled = true;

                response.m_outputProducts.push_back(AZStd::move(product));

                data.clear();
            }
            return true;
        }

        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
        {
            using namespace AzToolsFramework::Prefab;
            using namespace AzToolsFramework::Prefab::PrefabConversionUtils;

            if (m_isShuttingDown)
            {
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
                return;
            }

            if (!m_prefabSystem)
            {
                m_prefabSystem = AZStd::make_unique<AzToolsFramework::Prefab::PrefabSystemComponent>();
                m_prefabSystem->Init();
                m_prefabSystem->Activate();

                m_pipeline.LoadStackProfile("GameObjectCreation");
            }

            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;

            auto* system = AZ::Interface<PrefabSystemComponentInterface>::Get();
            if (!system)
            {
                AZ_Error("Prefab Builder", false, "Prefab system is not available.");
                return;
            }

            auto* loader = AZ::Interface<PrefabLoaderInterface>::Get();
            if (!loader)
            {
                AZ_Error("Prefab Builder", false, "Prefab loader is not available.");
                return;
            }

            AZ_TracePrintf("Prefab Builder", "Loading Prefab in '%s'.\n", request.m_fullPath.c_str());
            TemplateId templateId = loader->LoadTemplate(request.m_fullPath);
            if (templateId == InvalidTemplateId)
            {
                AZ_Error("Prefab Builder", false, "Failed to load Prefab template.");
                return;
            }

            PrefabProcessorContext context;
            PrefabDom mutableRootDom;
            // TODO: LoadTemplate should be updated so there's no need to get the extra copy here.
            mutableRootDom.CopyFrom(system->FindTemplateDom(templateId), mutableRootDom.GetAllocator());
            AZStd::string rootPrefabName;
            if (!StringFunc::Path::GetFileName(request.m_fullPath.c_str(), rootPrefabName))
            {
                AZ_Warning("Prefab Builder", false, "Unable to extract filename from '%s', using 'Root' instead.",
                    request.m_fullPath.c_str());
                rootPrefabName = "Root";
            }
            context.AddPrefab(AZStd::move(rootPrefabName), AZStd::move(mutableRootDom));
            
            AZ_TracePrintf("Prefab Builder", "Sending Prefab to the processor stack.\n");
            m_pipeline.ProcessPrefab(context);

            AZ_TracePrintf("Prefab Builder", "Finalizing products.\n");
            if (!context.HasPrefabs())
            {
                if (StoreProducts(request, response, context.GetProcessedObjects()))
                {
                    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
                }
                else
                {
                    AZ_Error("Prefab Builder", false, "One or more objects couldn't be committed to disk.");
                }
            }
            else
            {
                AZ_Error("Prefab Builder", false, "After processing there were still Prefabs left.");
            }

            AZ_TracePrintf("Prefab Builder", "Cleaning up.\n");
            system->RemoveTemplate(templateId);
            AZ_TracePrintf("Prefab Builder", "Prefab processing completed.\n");
        }

        AZStd::unique_ptr<AzToolsFramework::Prefab::PrefabSystemComponent> m_prefabSystem;
        AzToolsFramework::Prefab::PrefabConversionUtils::PrefabConversionPipeline m_pipeline;
        AZ::Uuid m_builderId;
        AZStd::atomic_bool m_isShuttingDown{ false };
    };

    class PrefabBuilderModule
        : public Module
    {
    public:
        AZ_RTTI(AZ::Prefab::PrefabBuilderModule, "{088B2BA8-9F19-469C-A0B5-1DD523879C70}", Module);
        AZ_CLASS_ALLOCATOR(PrefabBuilderModule, AZ::SystemAllocator, 0);

        PrefabBuilderModule()
            : Module()
        {
            m_builder.Register();
        }

        void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("PrefabBuilder"));
        }

        void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("PrefabBuilder"));
        }

        PrefabBuilderComponent m_builder;
    };
} // namespace AZ::Prefab

AZ_DECLARE_MODULE_CLASS(Gem_Prefab_PrefabBuilder, AZ::Prefab::PrefabBuilderModule)
