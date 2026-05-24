/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ParticleBuilder.h"

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AssetBuilderSDK/SerializationDependencies.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>

#include <AzCore/Serialization/Json/JsonUtils.h>
#include <Serializer/ParticleSourceData.h>
#include <AzCore/Asset/AssetJsonSerializer.h>

namespace OpenParticle
{
    namespace
    {
        static constexpr char const PARTICLE_BUILDER_NAME[] = "ParticleBuilder";
    }

    const char* ParticleBuilder::jobKey = "Particle Builder";

    template<typename ObjectType>
    bool LoadFromFile(const AZStd::string& path, ObjectType& objectData)
    {
        objectData = ObjectType();

        auto loadOutcome = AZ::JsonSerializationUtils::ReadJsonFile(path);
        if (!loadOutcome.IsSuccess())
        {
            AZ_Error("AZ::JsonSerializationUtils", false, "%s", loadOutcome.GetError().c_str());
            return false;
        }

        rapidjson::Document& document = loadOutcome.GetValue();

        AZ::JsonDeserializerSettings jsonSettings;

        AZ::RPI::JsonReportingHelper reportingHelper;
        reportingHelper.Attach(jsonSettings);

        AZ::JsonSerialization::Load(objectData, document, jsonSettings);

        if (reportingHelper.ErrorsReported())
        {
            return false;
        }
        else if (reportingHelper.WarningsReported())
        {
            AZ_Warning(PARTICLE_BUILDER_NAME, false, "Warnings reported while loading '%s'", path.c_str());
            return true;
        }
        else
        {
            return true;
        }
    }

    void ParticleBuilder::RegisterBuilder()
    {
        AssetBuilderSDK::AssetBuilderDesc particleBuilderDescriptor;
        particleBuilderDescriptor.m_name = jobKey;
        particleBuilderDescriptor.m_version = 1; // bump this to reprocess all particles.
        particleBuilderDescriptor.m_busId = azrtti_typeid<ParticleBuilder>();
        particleBuilderDescriptor.m_patterns.push_back(
            AssetBuilderSDK::AssetBuilderPattern("*.particle", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        particleBuilderDescriptor.m_createJobFunction =
            AZStd::bind(&ParticleBuilder::CreateJobs, this, AZStd::placeholders::_1, AZStd::placeholders::_2);
        particleBuilderDescriptor.m_processJobFunction =
            AZStd::bind(&ParticleBuilder::ProcessJob, this, AZStd::placeholders::_1, AZStd::placeholders::_2);

        BusConnect(particleBuilderDescriptor.m_busId);
        AssetBuilderSDK::AssetBuilderBus::Broadcast(
            &AssetBuilderSDK::AssetBuilderBus::Handler::RegisterBuilderInformation, particleBuilderDescriptor);
    }

    ParticleBuilder::~ParticleBuilder()
    {
        BusDisconnect();
    }

    void ParticleBuilder::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const
    {
        if (m_isShuttingDown)
        {
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
            return;
        }

        AZStd::string fullSourcePath;
        AZStd::set<AZStd::string> dependencyPaths;
        AzFramework::StringFunc::Path::ConstructFull(request.m_watchFolder.data(), request.m_sourceFile.data(), fullSourcePath, true);
        OpenParticle::ParticleSourceData sourceData;
        if (!OpenParticle::LoadFromFile(fullSourcePath, sourceData))
        {
            AZ_Error(PARTICLE_BUILDER_NAME, false, "Failed to load particle from file '%s'!", fullSourcePath.c_str());
            return;
        }

        for (const AssetBuilderSDK::PlatformInfo& platformInfo : request.m_enabledPlatforms)
        {
            AssetBuilderSDK::JobDescriptor descriptor;
            descriptor.m_jobKey = jobKey;
            descriptor.SetPlatformIdentifier(platformInfo.m_identifier.c_str());
            descriptor.m_critical = false;
            descriptor.m_priority = 1; // since this is an entry point asset, we should prioritize it above the background

            // make it depend on the materials it references.
            // We didn't actually load them since we only need their assetid, not their data, at this stage.
            // We need to load it during job processing to check if its the right material type, but not now.
            for (const auto& emitter : sourceData.m_emitters)
            {
                if (emitter->m_material.GetId().IsValid())
                {
                    AssetBuilderSDK::SourceFileDependency dep;
                    dep.m_sourceFileDependencyUUID = emitter->m_material.GetId().m_guid;
                    descriptor.m_jobDependencyList.push_back(AssetBuilderSDK::JobDependency(
                        "Material Builder", platformInfo.m_identifier, AssetBuilderSDK::JobDependencyType::Order, dep));
                }
            }
            response.m_createJobOutputs.push_back(descriptor);
        }

        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
    }

    void ParticleBuilder::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const
    {
        AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);

        if (jobCancelListener.IsCancelled())
        {
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            return;
        }
        if (m_isShuttingDown)
        {
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            return;
        }

        AZStd::string fullSourcePath;
        AzFramework::StringFunc::Path::ConstructFull(request.m_watchFolder.data(), request.m_sourceFile.data(), fullSourcePath, true);
        OpenParticle::ParticleSourceData sourceData;
        if (!OpenParticle::LoadFromFile(fullSourcePath, sourceData))
        {
            AZ_Error(PARTICLE_BUILDER_NAME, false, "Failed to load particle from file '%s'!", fullSourcePath.c_str());
            return;
        }

        sourceData.Normalize();
        sourceData.ToEditor();
        sourceData.ToRuntime();
        if (!sourceData.CheckDistributionIndex())
        {
            AZ_Error("ParticleSourceData", false, "Distribution index doesn't match distribution.");
            return;
        }

        if (!sourceData.CheckEmitterNames())
        {
            AZ_Error("ParticleSourceData", false, "Duplicate emitters name in particle file.");
            return;
        }

        auto particleAssetOutcome = sourceData.CreateParticleAsset(AZ::Uuid::CreateName(fullSourcePath.c_str()), fullSourcePath, true);
        if (!particleAssetOutcome.IsSuccess())
        {
            AZ_Error(PARTICLE_BUILDER_NAME, false, "Failed to create particle asset '%s'!", fullSourcePath.c_str());
            return;
        }

        AZ::Data::Asset<ParticleAsset> particleAsset = particleAssetOutcome.GetValue();
        AZStd::string particleProductPath;
        AZStd::string fileNameNoExt;
        AzFramework::StringFunc::Path::GetFileName(request.m_sourceFile.c_str(), fileNameNoExt);
        if (fileNameNoExt.find(AZ_FILESYSTEM_EXTENSION_SEPARATOR) != AZStd::string::npos)
        {
            AZ_Error(PARTICLE_BUILDER_NAME, false, "Invalid particle filename '%s'!", fileNameNoExt.c_str());
            return;
        }

        AzFramework::StringFunc::Path::ConstructFull(request.m_tempDirPath.c_str(), fileNameNoExt.c_str(), particleProductPath, true);
        AzFramework::StringFunc::Path::ReplaceExtension(particleProductPath, ParticleAsset::Extension);

        if (!AZ::Utils::SaveObjectToFile(particleProductPath, AZ::DataStream::ST_XML, particleAsset.Get()))
        {
            AZ_Error(PARTICLE_BUILDER_NAME, false, "Failed to save particle to file '%s'!", particleProductPath.c_str());
            return;
        }

        AssetBuilderSDK::JobProduct jobProduct;
        if (!AssetBuilderSDK::OutputObject(particleAsset.Get(), particleProductPath, azrtti_typeid<ParticleAsset>(), 0, jobProduct))
        {
            AZ_Error(PARTICLE_BUILDER_NAME, false, "Failed to output product dependencies.");
            return;
        }
        response.m_outputProducts.push_back(AZStd::move(jobProduct));
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    }

    void ParticleBuilder::ShutDown()
    {
        m_isShuttingDown = true;
    }
} // namespace OpenParticle
