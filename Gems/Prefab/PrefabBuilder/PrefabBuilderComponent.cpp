/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "PrefabBuilderComponent.h"
#include <AzToolsFramework/Debug/TraceContext.h>

namespace AZ::Prefab
{
    // This allows the component's serialization version, and the builder version, to remain in sync.
    // Not strictly required, but it's useful to modify one when the other is modified, to keep data synchronized.
    const int PrefabBuilderComponentVersion = 1;

    void PrefabBuilderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<PrefabBuilderComponent, AZ::Component>()
                ->Version(PrefabBuilderComponentVersion)
                ->Attribute(
                AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({AssetBuilderSDK::ComponentTags::AssetBuilder}));
        }
    }

    AzToolsFramework::Fingerprinting::TypeFingerprint PrefabBuilderComponent::CalculateBuilderFingerprint() const
    {
        AzToolsFramework::Fingerprinting::TypeCollection typeCollection = m_typeFingerprinter->GatherAllTypesForComponents();
        AzToolsFramework::Fingerprinting::TypeFingerprint fingerprint = m_typeFingerprinter->GenerateFingerprintForAllTypes(typeCollection);
        AZStd::hash_combine(fingerprint, m_pipeline.GetFingerprint());

        return fingerprint;
    }

    void PrefabBuilderComponent::Activate()
    {
        AssetBuilderSDK::AssetBuilderCommandBus::Handler::BusConnect(m_builderId);

        m_pipeline.LoadStackProfile(ConfigKey);

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        AZ_Assert(serializeContext, "SerializeContext not found");
        m_typeFingerprinter = AZStd::make_unique<AzToolsFramework::Fingerprinting::TypeFingerprinter>(*serializeContext);

        AzToolsFramework::Fingerprinting::TypeFingerprint fingerprint = CalculateBuilderFingerprint();

        AssetBuilderSDK::AssetBuilderDesc builderDesc;
        builderDesc.m_name = "Prefab Builder";
        builderDesc.m_version = PrefabBuilderComponentVersion;
        builderDesc.m_patterns.emplace_back("*.prefab", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard);
        builderDesc.m_builderType = AssetBuilderSDK::AssetBuilderDesc::AssetBuilderType::External;
        builderDesc.m_busId = m_builderId;
        builderDesc.m_analysisFingerprint = AZStd::to_string(fingerprint);

        builderDesc.m_createJobFunction =
            [this](const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) {
                CreateJobs(request, response);
            };
        builderDesc.m_processJobFunction =
            [this](const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) {
                ProcessJob(request, response);
            };

        AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBusTraits::RegisterBuilderInformation, builderDesc);
    }

    void PrefabBuilderComponent::Deactivate()
    {
        AssetBuilderSDK::AssetBuilderCommandBus::Handler::BusDisconnect(m_builderId);
    }

    void PrefabBuilderComponent::ShutDown()
    {
        m_isShuttingDown = true;
    }

    AzToolsFramework::Fingerprinting::TypeFingerprint PrefabBuilderComponent::CalculatePrefabFingerprint(
        const AzToolsFramework::Prefab::PrefabDom& genericDocument) const
    {
        AzToolsFramework::Fingerprinting::TypeFingerprint fingerprint = m_pipeline.GetFingerprint();

        // Deserialize all of the entities and their components (for this prefab only)
        auto newInstance = AZStd::make_unique<AzToolsFramework::Prefab::Instance>();
        AzToolsFramework::EntityList entities;
        if (AzToolsFramework::Prefab::PrefabDomUtils::LoadInstanceFromPrefabDom(*newInstance, entities, genericDocument))
        {
            // Add the fingerprint of all the components and their types
            AZStd::hash_combine(fingerprint, m_typeFingerprinter->GenerateFingerprintForAllTypesInObject(&entities));
        }

        return fingerprint;
    }

    AZStd::vector<AssetBuilderSDK::SourceFileDependency> PrefabBuilderComponent::GetSourceDependencies(const AzToolsFramework::Prefab::PrefabDom& genericDocument)
    {
        AZStd::vector<AssetBuilderSDK::SourceFileDependency> sourceFileDependencies;
        auto instancesIterator = genericDocument.FindMember(AzToolsFramework::Prefab::PrefabDomUtils::InstancesName);
    
        if (instancesIterator != genericDocument.MemberEnd())
        {
            auto&& instances = instancesIterator->value;
    
            if (instances.IsObject())
            {
                for (auto&& entry : instances.GetObject())
                {
                    auto sourceIterator = entry.value.FindMember(AzToolsFramework::Prefab::PrefabDomUtils::SourceName);
    
                    if (sourceIterator != entry.value.MemberEnd())
                    {
                        auto&& source = sourceIterator->value;
    
                        if (source.IsString())
                        {
                            sourceFileDependencies.emplace_back(source.GetString(), Uuid::CreateNull());
                        }
                    }
                }
            }
        }

        return sourceFileDependencies;
    }

    void PrefabBuilderComponent::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const
    {
        using namespace AzToolsFramework::Prefab;
        using namespace AzToolsFramework::Prefab::PrefabConversionUtils;

        if (m_isShuttingDown)
        {
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
            return;
        }

        AZStd::string fullPath;
        AZ::StringFunc::Path::ConstructFull(request.m_watchFolder.c_str(), request.m_sourceFile.c_str(), fullPath);
        
        // Load the JSON Dom
        AZ::Outcome<PrefabDom, AZStd::string> readPrefabFileResult = AZ::JsonSerializationUtils::ReadJsonFile(fullPath);
        if (!readPrefabFileResult.IsSuccess())
        {
            AZ_Error(
                "Prefab", false,
                "PrefabLoader::LoadPrefabFile - Failed to load Prefab file from '%s'."
                "Error message: '%s'",
                fullPath.c_str(), readPrefabFileResult.GetError().c_str());

            return;
        }

        auto genericDocument = readPrefabFileResult.TakeValue();

        size_t fingerprint = CalculatePrefabFingerprint(genericDocument);
        AZStd::vector<AssetBuilderSDK::SourceFileDependency> sourceFileDependencies = GetSourceDependencies(genericDocument);

        for (const AssetBuilderSDK::PlatformInfo& info : request.m_enabledPlatforms)
        {
            AssetBuilderSDK::JobDescriptor job;
            job.m_jobKey = PrefabJobKey;
            job.SetPlatformIdentifier(info.m_identifier.c_str());
            job.m_additionalFingerprintInfo = AZStd::to_string(fingerprint);

            // Add a fingerprint job dependency on any referenced prefab so this prefab will rebuild if the dependent fingerprint changes
            for (const AssetBuilderSDK::SourceFileDependency& sourceFileDependency : sourceFileDependencies)
            {
                job.m_jobDependencyList.emplace_back(
                    PrefabJobKey, info.m_identifier, AssetBuilderSDK::JobDependencyType::Fingerprint, sourceFileDependency);
            }

            response.m_createJobOutputs.push_back(AZStd::move(job));
        }
        
        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
    }

    bool PrefabBuilderComponent::StoreProducts(
        AZ::IO::PathView tempDirPath,
        const AzToolsFramework::Prefab::PrefabConversionUtils::PrefabProcessorContext::ProcessedObjectStoreContainer& store,
        const AzToolsFramework::Prefab::PrefabConversionUtils::PrefabProcessorContext::ProductAssetDependencyContainer& registeredDependencies,
        AZStd::vector<AssetBuilderSDK::JobProduct>& outputProducts) const
    {
        using namespace AzToolsFramework::Prefab::PrefabConversionUtils;

        outputProducts.reserve(store.size());

        AZStd::vector<uint8_t> data;
        for (auto& object : store)
        {
            AZ_TracePrintf("Prefab Builder", "    Serializing Prefab product '%s'.\n", object.GetId().c_str());
            if (!object.Serialize(data))
            {
                AZ_Error("Prefab Builder", false, "Failed to serialize object '%s'.", object.GetId().c_str());
                return false;
            }

            AZ::IO::Path productPath = tempDirPath;
            productPath /= object.GetId();

            AZ_TracePrintf("Prefab Builder", "    Storing Prefab product '%s'.\n", object.GetId().c_str());

            AZStd::unique_ptr<AZ::IO::GenericStream> productFile = GetOutputStream(productPath);

            if (!productFile->IsOpen())
            {
                AZ_Error("Prefab Builder", false, "Unable to open product file at '%s'.", productPath.c_str());
                return false;
            }

            if (productFile->Write(data.size(), data.data()) != data.size())
            {
                AZ_Error("Prefab Builder", false, "Unable to write product file at '%s'.", productPath.c_str());
                return false;
            }

            AssetBuilderSDK::JobProduct product;

            if (AssetBuilderSDK::OutputObject(&object.GetAsset(), object.GetAssetType(), productPath.String(), object.GetAssetType(),
                object.GetAsset().GetId().m_subId, product))
            {
                auto range = registeredDependencies.equal_range(object.GetAsset().GetId());
                AZStd::transform(range.first, range.second,
                    AZStd::back_inserter(product.m_dependencies),
                    [](const auto& dependency) -> AssetBuilderSDK::ProductDependency
                    {
                        return AssetBuilderSDK::ProductDependency(
                            dependency.second.m_assetId, AZ::Data::ProductDependencyInfo::CreateFlags(dependency.second.m_loadBehavior));
                    });

                outputProducts.push_back(AZStd::move(product));
            }

            data.clear();
        }
        return true;
    }

    AZStd::unique_ptr<AZ::IO::GenericStream> PrefabBuilderComponent::GetOutputStream(const AZ::IO::Path& path) const
    {
        return AZStd::make_unique<AZ::IO::FileIOStream>(path.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeCreatePath);
    }

    bool PrefabBuilderComponent::ProcessPrefab(
        const AZ::PlatformTagSet& platformTags, const char* filePath, AZ::IO::PathView tempDirPath, const AZ::Uuid& sourceFileUuid,
        AzToolsFramework::Prefab::PrefabDom&& rootDom, AZStd::vector<AssetBuilderSDK::JobProduct>& jobProducts)
    {
        AZ_TraceContext("Stack config", ConfigKey);
        AzToolsFramework::Prefab::PrefabConversionUtils::PrefabProcessorContext context(sourceFileUuid);
        AZStd::string rootPrefabName;
        if (!StringFunc::Path::GetFileName(filePath, rootPrefabName))
        {
            AZ_Error("Prefab Builder", false, "Unable to extract filename from '%s'.",
                       filePath);
            return false;
        }
        AzToolsFramework::Prefab::PrefabConversionUtils::PrefabDocument rootDocument(AZStd::move(rootPrefabName));
        rootDocument.SetPrefabDom(AZStd::move(rootDom));
        context.AddPrefab(AZStd::move(rootDocument));
        
        context.SetPlatformTags(AZStd::move(platformTags));

        AZ_TracePrintf("Prefab Builder", "Sending Prefab to the processor stack.\n");
        m_pipeline.ProcessPrefab(context);
        if (context.HasCompletedSuccessfully())
        {
            AZ_TracePrintf("Prefab Builder", "Finalizing products.\n");

            if (StoreProducts(tempDirPath, context.GetProcessedObjects(),
                context.GetRegisteredProductAssetDependencies(), jobProducts))
            {
                return true;
            }
            else
            {
                AZ_Error("Prefab Builder", false, "One or more objects couldn't be committed to disk.");
            }
        }
        else
        {
            AZ_Error("Prefab Builder", false, "Failed to fully process the target prefab.");
        }
        return false;
    }

    void PrefabBuilderComponent::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
    {
        using namespace AzToolsFramework::Prefab;
        using namespace AzToolsFramework::Prefab::PrefabConversionUtils;

        if (m_isShuttingDown)
        {
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            return;
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
        AzToolsFramework::Prefab::TemplateId templateId = loader->LoadTemplateFromFile(AZStd::string_view(request.m_fullPath));
        if (templateId == InvalidTemplateId)
        {
            AZ_Error("Prefab Builder", false, "Failed to load Prefab template.");
            return;
        }

        PrefabDom mutableRootDom;
        mutableRootDom.CopyFrom(system->FindTemplateDom(templateId), mutableRootDom.GetAllocator());

        AZ::PlatformTagSet platformTags;
        const auto& tags = request.m_platformInfo.m_tags;
        AZStd::for_each(tags.begin(), tags.end(), [&platformTags](const auto& tag) {
            platformTags.emplace(AZ::Crc32(tag.c_str(), tag.size(), true));
        });

        if (ProcessPrefab(
                platformTags, request.m_fullPath.c_str(), request.m_tempDirPath.c_str(), request.m_sourceFileUUID,
                AZStd::move(mutableRootDom), response.m_outputProducts))
        {
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        }

        AZ_TracePrintf("Prefab Builder", "Cleaning up.\n");
        system->RemoveAllTemplates();
        AZ_TracePrintf("Prefab Builder", "Prefab processing completed.\n");
    }
} // namespace AZ::Prefab
