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
#include <OpenParticleSystem/Serializer/ParticleSourceData.h>

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

    void AddPossibleJobDependencies(
        AZStd::string_view currentFilePath,
        AZStd::set<AZStd::string>& dependencyPaths,
        AZStd::vector<AssetBuilderSDK::SourceFileDependency>& sourceFileDependencies,
        AZStd::vector<AssetBuilderSDK::JobDependency>& jobDependencies)
    {
        for (auto& filePath : dependencyPaths)
        {
            bool dependencyFileFound = false;

            AZStd::vector<AZStd::string> possibleDependencies;
            // Convert incoming paths containing aliases into absolute paths
            AZ::IO::FixedMaxPath referencedPath;
            AZ::IO::FileIOBase::GetInstance()->ReplaceAlias(referencedPath, AZ::IO::PathView{ filePath });

            if (referencedPath.IsRelative())
            {
                AZ::IO::FixedMaxPath originatingPath;
                AZ::IO::FileIOBase::GetInstance()->ReplaceAlias(originatingPath, AZ::IO::PathView{ currentFilePath });
                AZ::IO::FixedMaxPath combinedPath = originatingPath.ParentPath();
                combinedPath /= referencedPath;

                possibleDependencies.push_back(combinedPath.LexicallyNormal().String());
            }
            // Use the referencedSourceFilePath as a standard asset path
            possibleDependencies.push_back(referencedPath.LexicallyNormal().String());

            for (auto& file : possibleDependencies)
            {
                AssetBuilderSDK::SourceFileDependency sourceFileDependency;
                sourceFileDependency.m_sourceFileDependencyPath = file;
                sourceFileDependencies.push_back(sourceFileDependency);

                if (!dependencyFileFound)
                {
                    AZ::Data::AssetInfo sourceInfo;
                    AZStd::string watchFolder;
                    AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                        dependencyFileFound, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath, file.c_str(),
                        sourceInfo, watchFolder);

                    if (dependencyFileFound)
                    {
                        AssetBuilderSDK::JobDependency jobDependency;
                        if (AzFramework::StringFunc::Path::IsExtension(filePath.c_str(), "material"))
                        {
                            jobDependency.m_jobKey = "Material Builder";
                        }
                        else if (AzFramework::StringFunc::Path::IsExtension(filePath.c_str(), "fbx"))
                        {
                            jobDependency.m_jobKey = "Scene compilation";
                        }
                        else
                        {
                            continue;
                        }
                        jobDependency.m_type = AssetBuilderSDK::JobDependencyType::Order;
                        jobDependency.m_sourceFile.m_sourceFileDependencyPath = file;
                        jobDependencies.push_back(jobDependency);
                    }
                }
            }
        }
    }

    void ParticleBuilder::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const
    {
        if (m_isShuttingDown)
        {
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
            return;
        }

        AssetBuilderSDK::JobDescriptor descriptor;
        descriptor.m_jobKey = jobKey;
        AZStd::string fullSourcePath;
        AzFramework::StringFunc::Path::ConstructFull(request.m_watchFolder.data(), request.m_sourceFile.data(), fullSourcePath, true);

        auto loadOutcome = AZ::JsonSerializationUtils::ReadJsonFile(fullSourcePath);
        if (!loadOutcome.IsSuccess())
        {
            AZ_Error("ParticleBuilder", false, "%s", loadOutcome.GetError().c_str());
            return;
        }
        rapidjson::Document& document = loadOutcome.GetValue();
        AZStd::set<AZStd::string> dependencyPaths;

        if (document.IsObject() && document.HasMember("emitters") && document["emitters"].IsArray())
        {
            for (auto& emitter : document["emitters"].GetArray())
            {
                if (emitter.HasMember("material") && emitter["material"].IsString())
                {
                    dependencyPaths.insert(emitter["material"].GetString());
                }
                if (emitter.HasMember("model") && emitter["model"].IsString())
                {
                    if (AzFramework::StringFunc::Path::IsExtension(emitter["model"].GetString(), "azmodel"))
                    {
                        AZStd::string modelPath = emitter["model"].GetString();
                        AzFramework::StringFunc::Path::ReplaceExtension(modelPath, "fbx");
                        dependencyPaths.insert(modelPath.c_str());
                    }
                }
            }
            AddPossibleJobDependencies(
                request.m_sourceFile, dependencyPaths, response.m_sourceFileDependencyList, descriptor.m_jobDependencyList);
        }

        for (const AssetBuilderSDK::PlatformInfo& platformInfo : request.m_enabledPlatforms)
        {
            descriptor.SetPlatformIdentifier(platformInfo.m_identifier.c_str());
            descriptor.m_critical = false;
            for (auto& jobDep : descriptor.m_jobDependencyList)
            {
                jobDep.m_platformIdentifier = platformInfo.m_identifier;
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
