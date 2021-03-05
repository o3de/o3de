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

#include "ResourceCompilerScene_precompiled.h"
#include <IRCLog.h>
#include <ISceneConfig.h>
#include <ISystem.h>
#include <SceneCompiler.h>

#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Memory/AllocatorManager.h>
#include <AzCore/Memory/MemoryComponent.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>

#include <AzFramework/Asset/AssetSystemComponent.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/TargetManagement/TargetManagementComponent.h>

#include <AzToolsFramework/Debug/TraceContext.h>
#include <AzToolsFramework/SourceControl/PerforceComponent.h>

#include <AssetBuilderSDK/AssetBuilderSDK.h>

#include <RC/ResourceCompilerScene/Cgf/CgfExporter.h>
#include <RC/ResourceCompilerScene/Cgf/CgfGroupExporter.h>
#include <RC/ResourceCompilerScene/Cgf/CgfLodExporter.h>

#include <RC/ResourceCompilerScene/Common/CommonExportContexts.h>
#include <RC/ResourceCompilerScene/Common/ContainerSettingsExporter.h>
#include <RC/ResourceCompilerScene/Common/MeshExporter.h>
#include <RC/ResourceCompilerScene/Common/MaterialExporter.h>
#include <RC/ResourceCompilerScene/Common/WorldMatrixExporter.h>
#include <RC/ResourceCompilerScene/Common/ColorStreamExporter.h>
#include <RC/ResourceCompilerScene/Common/UVStreamExporter.h>
#include <RC/ResourceCompilerScene/Common/SkeletonExporter.h>
#include <RC/ResourceCompilerScene/Common/SkinWeightExporter.h>
#include <RC/ResourceCompilerScene/Common/BlendShapeExporter.h>
#include <RC/ResourceCompilerScene/Common/TouchBendingExporter.h>
#include <RC/ResourceCompilerScene/SceneSerializationHandler.h>

#include <SceneAPI/SceneCore/Components/ExportingComponent.h>
#include <SceneAPI/SceneCore/Components/GenerationComponent.h>
#include <SceneAPI/SceneCore/Components/RCExportingComponent.h>
#include <SceneAPI/SceneCore/Components/SceneSystemComponent.h>
#include <SceneAPI/SceneCore/Components/Utilities/EntityConstructor.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>
#include <SceneAPI/SceneCore/Events/GenerateEventContext.h>
#include <SceneAPI/SceneCore/Events/ExportProductList.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>

namespace AZ
{
    namespace RC
    {
            //! This function returns the build system target name
            AZStd::string_view GetAssetBuilderTargetName()
            {
#if !defined (LY_CMAKE_TARGET)
#error "LY_CMAKE_TARGET must be defined to the build target of the AssetBuilder application which is currently \"AssetBuilder\" via the source file properties for the SceneCompiler.cpp"
#endif
                return AZStd::string_view{ LY_CMAKE_TARGET };
            }

        static const u32 s_maxLegacyCrcClashRetries = 255;

        namespace SceneEvents = AZ::SceneAPI::Events;
        namespace SceneContainers = AZ::SceneAPI::Containers;

        RCToolApplication::RCToolApplication()
        {
        }

        void RCToolApplication::RegisterDescriptors()
        {
            RegisterComponentDescriptor(SceneSerializationHandler::CreateDescriptor());
            RegisterComponentDescriptor(BlendShapeExporter::CreateDescriptor());
            RegisterComponentDescriptor(ColorStreamExporter::CreateDescriptor());
            RegisterComponentDescriptor(ContainerSettingsExporter::CreateDescriptor());
            RegisterComponentDescriptor(MaterialExporter::CreateDescriptor());
            RegisterComponentDescriptor(MeshExporter::CreateDescriptor());
            RegisterComponentDescriptor(SkeletonExporter::CreateDescriptor());
            RegisterComponentDescriptor(SkinWeightExporter::CreateDescriptor());
            RegisterComponentDescriptor(UVStreamExporter::CreateDescriptor());
            RegisterComponentDescriptor(WorldMatrixExporter::CreateDescriptor());
            RegisterComponentDescriptor(TouchBendingExporter::CreateDescriptor());
        }

        AZ::ComponentTypeList RCToolApplication::GetRequiredSystemComponents() const
        {
            AZ::ComponentTypeList components = AzToolsFramework::ToolsApplication::GetRequiredSystemComponents();
            
            auto removed = AZStd::remove_if(components.begin(), components.end(), 
                [](const Uuid& id) -> bool
                {
                    return id == azrtti_typeid<AzFramework::TargetManagementComponent>()
                        || id == azrtti_typeid<AzToolsFramework::PerforceComponent>()
                        || id == azrtti_typeid<AZ::UserSettingsComponent>();
                });
            components.erase(removed, components.end());

            return components;
        }

        void RCToolApplication::SetSettingsRegistrySpecializations(AZ::SettingsRegistryInterface::Specializations& specializations)
        {
            AzToolsFramework::ToolsApplication::SetSettingsRegistrySpecializations(specializations);
            specializations.Append("scenecompiler");
        }

        SceneCompiler::SceneCompiler(const AZStd::shared_ptr<ISceneConfig>& config, const char* appRoot)
            : m_config(config)
            , m_appRoot(appRoot)
        {
        }

        void SceneCompiler::Release()
        {
            delete this;
        }

        void SceneCompiler::BeginProcessing(const IConfig* /*config*/)
        {
        }

        bool SceneCompiler::Process()
        {
            AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "Starting scene processing.\n");
            AssetBuilderSDK::ProcessJobResponse response;

            RCToolApplication application;

            // Add the Build Target name as a specialization to the settings registry
            AZ::SettingsRegistryInterface& registry = *AZ::SettingsRegistry::Get();
            AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddBuildSystemTargetSpecialization(
                registry, AZ::RC::GetAssetBuilderTargetName());

            //the project name can be overridden, check it
            AZStd::string overrideProjectName;
            overrideProjectName = m_context.m_config->GetAsString("gamesubdirectory", "", "");
            if (!overrideProjectName.empty())
            {
                // Copy the gamesubdirectory argument into --regset command line parameter for the sys_game_folder
                auto gameNameOverride = AZStd::string::format("--regset=%s/sys_game_folder=%s", AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey,
                    overrideProjectName.c_str());
                AZ::CommandLine* commandLine = application.GetAzCommandLine();
                auto settingsRegistry = AZ::SettingsRegistry::Get();
                AZ::CommandLine::ParamContainer commandLineArgs;
                commandLine->Dump(commandLineArgs);
                commandLineArgs.emplace_back(gameNameOverride.c_str(), gameNameOverride.size());
                commandLine->Parse(commandLineArgs);

                AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_CommandLine(*settingsRegistry, *commandLine, false);
                AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(*settingsRegistry);
            }

            bool connectedToAP;
            if (!PrepareForExporting(application, m_appRoot, connectedToAP))
            {
                bool result = WriteResponse(m_context.GetOutputFolder().c_str(), response, connectedToAP ? AssetBuilderSDK::ProcessJobResult_Failed : AssetBuilderSDK::ProcessJobResult_NetworkIssue);
                AzFramework::AssetSystemRequestBus::Broadcast(&AzFramework::AssetSystemRequestBus::Events::StartDisconnectingAssetProcessor);
                return result;
            }

            // Do this  after PrepareForExporting is called so the types are registered for reading the request and writing a response.
            AZStd::unique_ptr<AssetBuilderSDK::ProcessJobRequest> request = ReadJobRequest(m_context.GetOutputFolder().c_str());
            if (!request)
            {
                bool result = WriteResponse(m_context.GetOutputFolder().c_str(), response, AssetBuilderSDK::ProcessJobResult_Failed);
                AzFramework::AssetSystemRequestBus::Broadcast(&AzFramework::AssetSystemRequestBus::Events::StartDisconnectingAssetProcessor);
                return result;
            }

            bool result = false;
            // Active components, load the scene then process and export it.
            {
                AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "Creating scene system modules.\n");
                AZ::Entity* systemEntity = AZ::SceneAPI::SceneCore::EntityConstructor::BuildSceneSystemEntity();
                if (systemEntity)
                {
                    constexpr const char PythonMarshalComponentTypeId[] = "{C733E1AD-9FDD-484E-A8D9-3EAB944B7841}";
                    constexpr const char PythonReflectionComponentTypeId[] = "{CBF32BE1-292C-4988-9E64-25127A8525A7}";
                    constexpr const char PythonSystemComponentTypeId[] = "{97F88B0F-CF68-4623-9541-549E59EE5F0C}";
                    systemEntity->CreateComponentIfReady({PythonSystemComponentTypeId});
                    systemEntity->CreateComponentIfReady({PythonMarshalComponentTypeId});
                    systemEntity->CreateComponentIfReady({PythonReflectionComponentTypeId});

                    systemEntity->Init();
                    systemEntity->Activate();

                    AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "Processing scene file.\n");
                    result = LoadAndExportScene(*request, response);

                    systemEntity->Deactivate();
                    delete systemEntity;
                }
                else
                {
                    AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Unable to create a system component for the SceneAPI.\n");
                    result = false;
                }
            }

            if (!result || m_config->GetErrorCount() > 0)
            {
                AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "During processing one or more problems were found.\n");
                result = false;
            }

            // Manually disconnect from the Asset Processor before the application goes out of scope to avoid
            // a potential serialization issue due to deficiencies in the order of teardown operations.
            AzFramework::AssetSystemRequestBus::Broadcast(&AzFramework::AssetSystemRequestBus::Events::StartDisconnectingAssetProcessor);
            
            AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "Finished scene processing.\n");
            return WriteResponse(m_context.GetOutputFolder().c_str(), response, result ? AssetBuilderSDK::ProcessJobResult_Success : AssetBuilderSDK::ProcessJobResult_Failed);
        }

        void SceneCompiler::EndProcessing()
        {
        }

        IConvertContext* SceneCompiler::GetConvertContext()
        {
            return &m_context;
        }

        bool SceneCompiler::PrepareForExporting(RCToolApplication& application, const AZStd::string& appRoot, bool& connectedToAssetProcessor)
        {
            // Not all Gems shutdown properly and leak memory, but this shouldn't
            //      prevent this builder from completing.
            AZ::AllocatorManager::Instance().SetAllocatorLeaking(true);

            AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "Initializing tools application environment.\n");
            AZ::ComponentApplication::Descriptor descriptor;
            descriptor.m_useExistingAllocator = true;
            descriptor.m_enableScriptReflection = false;

            AZ::ComponentApplication::StartupParameters startupParam;
            startupParam.m_appRootOverride = appRoot.c_str();
            startupParam.m_loadDynamicModules = false;

            application.Start(descriptor, startupParam);

            // Load Dynamic Modules after the Application::Start has been called to avoid creating
            // creating SystemComponents automatically
            application.LoadDynamicModules();

            application.RegisterDescriptors();

            // Register the AssetBuilderSDK structures needed later on.
            AssetBuilderSDK::InitializeSerializationContext();

            AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "Connecting to asset processor.\n");

            // Retrieve the asset processor connection params from the settings registry
            AzFramework::AssetSystem::ConnectionSettings connectionSettings;
            bool succeeded = AzFramework::AssetSystem::ReadConnectionSettingsFromSettingsRegistry(connectionSettings);
            if(!succeeded)
            {
                AZ_Error("RC Scene Compiler", false, "Getting bootstrap params failed");
                return false;
            }

            //override bootstrap params
            //the branch token can be overridden, check it
            AZStd::string overrideBranchToken;
            overrideBranchToken = m_context.m_config->GetAsString("branchtoken", "", "");
            if (!overrideBranchToken.empty())
            {
                connectionSettings.m_branchToken = overrideBranchToken;
            }

            //the port can be overridden, check it
            AZ::u16 overridePort = 0;
            overridePort = aznumeric_cast<AZ::u16>(m_context.m_config->GetAsInt("port", 0, 0));
            if (overridePort)
            {
                connectionSettings.m_assetProcessorPort = overridePort;
            }

            //the project name can be overridden, check it
            AZStd::string overrideProjectName;
            overrideProjectName = m_context.m_config->GetAsString("gamesubdirectory", "", "");
            if (!overrideProjectName.empty())
            {
                connectionSettings.m_projectName = overrideProjectName;
            }

            connectionSettings.m_connectionIdentifier = "RC Scene Compiler";
            connectionSettings.m_connectionDirection = AzFramework::AssetSystem::ConnectionSettings::ConnectionDirection::ConnectToAssetProcessor;
            connectionSettings.m_launchAssetProcessorOnFailedConnection = false; // builders shouldn't launch the AssetProcessor
            connectionSettings.m_waitUntilAssetProcessorIsReady = false; // builders are what make the AssetProcessor ready, so the cannot wait until the AssetProcessor is ready
            connectionSettings.m_waitForConnect = true; // application is a builder so it needs to wait for a connection

            // connect to Asset Processor.
            AzFramework::AssetSystemRequestBus::BroadcastResult(connectedToAssetProcessor,
                &AzFramework::AssetSystemRequestBus::Events::EstablishAssetProcessorConnection, connectionSettings);

            return connectedToAssetProcessor;
        }

        bool SceneCompiler::LoadAndExportScene(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
        {
            const string platformName = m_context.m_config->GetAsString("p", "<unknown>", "<invalid>");
            AZ_TraceContext("Platform", platformName.c_str());

            if (platformName == "<unknown>")
            {
                AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "No target platform provided - this compiler requires the /p=platformIdentifier option\n");
                return false;
            }

            if (platformName == "<invalid>")
            {
                AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Invalid target platform provided (Parse error reading command line)\n");
                return false;
            }

            AZStd::string sourcePath = m_context.GetSourcePath().c_str();
            AZ_TraceContext("Source", sourcePath);
            AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "Loading source files.\n");
            AZStd::shared_ptr<SceneContainers::Scene> scene;
            SceneEvents::SceneSerializationBus::BroadcastResult(scene, &SceneEvents::SceneSerializationBus::Events::LoadScene, sourcePath, request.m_sourceFileUUID);
            if (!scene)
            {
                AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Failed to load scene file.\n");
                return false;
            }

            AZ_TraceContext("Manifest", scene->GetManifestFilename());
            if (scene->GetManifest().IsEmpty())
            {
                AZ_TracePrintf(SceneAPI::Utilities::WarningWindow, "No manifest loaded and not enough information to create a default manifest.\n");
                return true;
            }

            AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "Generating data into scene.\n");
            if (!GenerateScene(*scene, platformName.c_str()))
            {
                AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Failed to run data generation for scene.\n");
                return false;
            }

            AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "Exporting loaded data to engine specific formats.\n");
            if (!ExportScene(request, response, *scene, platformName.c_str()))
            {
                AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Failed to convert and export scene\n");
                return false;
            }
            return true;
        }

        bool SceneCompiler::GenerateScene(AZ::SceneAPI::Containers::Scene& scene, const char* platformIdentifier)
        {
            AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "Creating generation entities.\n");
            AZ::SceneAPI::SceneCore::EntityConstructor::EntityPointer rcExporter = AZ::SceneAPI::SceneCore::EntityConstructor::BuildEntity(
                "Scene Generators", AZ::SceneAPI::SceneCore::GenerationComponent::TYPEINFO_Uuid());

            SceneEvents::ProcessingResultCombiner result;
            AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "Preparing for generation.\n");
            result += SceneEvents::Process<SceneEvents::PreGenerateEventContext>(scene, platformIdentifier);
            AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "Generating...\n");
            result += SceneEvents::Process<SceneEvents::GenerateEventContext>(scene, platformIdentifier);
            AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "Finalizing generation process.\n");
            result += SceneEvents::Process<SceneEvents::PostGenerateEventContext>(scene, platformIdentifier);

            switch (result.GetResult())
            {
            case SceneEvents::ProcessingResult::Success:
            case SceneEvents::ProcessingResult::Ignored:
                return true;
            case SceneEvents::ProcessingResult::Failure:
                AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Failure during conversion and exporting.\n");
                return false;
            }
            return false;
        }

        bool SceneCompiler::ExportScene(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response, const AZ::SceneAPI::Containers::Scene& scene, const char* platformIdentifier)
        {
            AZ_TraceContext("Output folder", m_context.GetOutputFolder().c_str());
            AZ_Assert(m_context.m_pRC->GetAssetWriter() != nullptr, "Invalid IAssetWriter initialization.");
            if (!m_context.m_pRC->GetAssetWriter())
            {
                return false;
            }

            AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "Creating export entities.\n");
            AZ::SceneAPI::SceneCore::EntityConstructor::EntityPointer rcExporter = AZ::SceneAPI::SceneCore::EntityConstructor::BuildEntity(
                "Scene RC Exporters", AZ::SceneAPI::SceneCore::RCExportingComponent::TYPEINFO_Uuid());

            // Register additional processors. Will be automatically unregistered when leaving scope.
            //      These have not yet been converted to components as they need special attention due
            //      to the arguments they currently need.
            AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "Registering export processors.\n");

            CgfExporter cgfProcessor(&m_context);
            CgfGroupExporter meshGroupExporter(m_context.m_pRC->GetAssetWriter());
            CgfLodExporter meshLodExporter(m_context.m_pRC->GetAssetWriter());

            SceneAPI::Events::ExportProductList productList;
            SceneEvents::ProcessingResultCombiner result;
            AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "Preparing for export.\n");
            result += SceneEvents::Process<SceneEvents::PreExportEventContext>(productList, m_context.GetOutputFolder().c_str(), scene, platformIdentifier);
            AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "Exporting...\n");
            result += SceneEvents::Process<SceneEvents::ExportEventContext>(productList, m_context.GetOutputFolder().c_str(), scene, platformIdentifier);
            AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "Finalizing export process.\n");
            result += SceneEvents::Process<SceneEvents::PostExportEventContext>(productList, m_context.GetOutputFolder().c_str(), platformIdentifier);

            AZStd::map<AZStd::string, size_t> preSubIdFiles;
            for (const auto& it : productList.GetProducts())
            {
                size_t index = response.m_outputProducts.size();
                AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "Listed product: %s+0x%08x - %s (type %s)\n", it.m_id.ToString<AZStd::string>().c_str(),
                    BuildSubId(it), it.m_filename.c_str(), it.m_assetType.ToString<AZStd::string>().c_str());
                response.m_outputProducts.emplace_back(AZStd::move(it.m_filename), it.m_assetType, BuildSubId(it));

                if (IsPreSubIdFile(it.m_filename))
                {
                    preSubIdFiles[it.m_filename] = index;
                }

                for (const AZStd::string& legacyIt : it.m_legacyFileNames)
                {
                    AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "  -> Legacy name: %s\n", legacyIt.c_str());
                    preSubIdFiles[legacyIt] = index;
                }

                // Add our relative path dependencies the exporters may have generated
                AssetBuilderSDK::JobProduct& currentProduct = response.m_outputProducts.back();
                for (const AZStd::string& pathDep : it.m_legacyPathDependencies)
                {
                    // For now, we assume the relative path dependencies are simply file names.
                    // Append the source product path to these dependencies to generate the proper path dependency
                    AZStd::string relativePath;
                    AzFramework::StringFunc::Path::GetFolderPath(request.m_sourceFile.c_str(), relativePath);
                    AzFramework::StringFunc::AssetDatabasePath::Join(relativePath.c_str(), pathDep.c_str(), relativePath);
                    currentProduct.m_pathDependencies.emplace(relativePath, AssetBuilderSDK::ProductPathDependencyType::SourceFile);
                }

                // If we have any output products that are a dependency of this product, add them here.
                // This will include adding LODs as dependencies of the base CGFs
                for (auto& exportProduct : it.m_productDependencies)
                {
                    AZ::Data::AssetId productAssetId(request.m_sourceFileUUID, BuildSubId(exportProduct));
                    currentProduct.m_dependencies.push_back(AssetBuilderSDK::ProductDependency(productAssetId, exportProduct.m_dependencyFlags));
                }

                currentProduct.m_dependenciesHandled = true; // We've populated the dependencies immediately above so it's OK to tell the AP we've handled dependencies
            }
            ResolvePreSubIds(response, preSubIdFiles);

            switch (result.GetResult())
            {
            case SceneEvents::ProcessingResult::Success:
                return true;
            case SceneEvents::ProcessingResult::Ignored:
                return true;
            case SceneEvents::ProcessingResult::Failure:
                AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Failure during conversion and exporting.\n");
                return false;
            default:
                AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow,
                    "Unexpected result from conversion and exporting (%i).\n", result.GetResult());
                return false;
            }
        }

        bool SceneCompiler::IsPreSubIdFile(const AZStd::string& file) const
        {
            AZStd::string extension;
            if (!AzFramework::StringFunc::Path::GetExtension(file.c_str(), extension))
            {
                return false;
            }

            return extension == ".caf" || extension == ".cgf" || extension == ".chr" || extension == ".mtl" || extension == ".skin";
        }

        // BuildSubId has an equivalent counterpart in SceneBuilder. Both need to remain the same to avoid problems with sub ids.
        u32 SceneCompiler::BuildSubId(const SceneAPI::Events::ExportProduct& product) const
        {
            // Instead of the just the lower 16-bits, use the full 32-bits that are available. There are production examples of
            //      uber-fbx files that contain hundreds of meshes that need to be split into individual mesh objects as an example.
            u32 id = static_cast<u32>(product.m_id.GetHash());

            if (product.m_lod.has_value())
            {
                AZ::u8 lod = product.m_lod.value();
                if (lod > 0xF)
                {
                    AZ_TracePrintf(SceneAPI::Utilities::WarningWindow, "%i is too large to fit in the allotted bits for LOD.\n", static_cast<u32>(lod));
                    lod = 0xF;
                }
                // The product uses lods so mask out the lod bits and set them appropriately.
                id &= ~AssetBuilderSDK::SUBID_MASK_LOD_LEVEL;
                id |= lod << AssetBuilderSDK::SUBID_LOD_LEVEL_SHIFT;
            }

            return id;
        }

        void SceneCompiler::ResolvePreSubIds(AssetBuilderSDK::ProcessJobResponse& response, const AZStd::map<AZStd::string, size_t>& preSubIdFiles) const
        {
            if (!preSubIdFiles.empty())
            {
                // Start by compiling a list of known sub ids. Include sub ids from non-legacy files as well because sub ids created
                //      here are not allowed to clash with any sub id not matter if it's legacy or not.
                AZStd::unordered_set<u32> assignedSubIds;
                for (const auto& it : response.m_outputProducts)
                {
                    if (assignedSubIds.find(it.m_productSubID) == assignedSubIds.end())
                    {
                        assignedSubIds.insert(it.m_productSubID);
                    }
                    else
                    {
                        AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Sub id collision found (0x%04x).\n", it.m_productSubID);
                    }
                }

                // First legacy product always had sub id 0. Also add the hashed version in the loop though as there might be a file in 
                //      front of it that the RCScene doesn't know about.
                response.m_outputProducts[preSubIdFiles.begin()->second].m_legacySubIDs.push_back(0);

                AZStd::string filename;
                for (const auto& it : preSubIdFiles)
                {
                    AZ_TraceContext("Legacy file name", it.first);
                    if (!AzFramework::StringFunc::Path::GetFullFileName(it.first.c_str(), filename))
                    {
                        AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Unable to extract filename for legacy sub id.\n");
                        continue;
                    }

                    // Modified version of the algorithm in RCBuilder.
                    for (u32 seedValue = 0; seedValue < s_maxLegacyCrcClashRetries; ++seedValue)
                    {
                        u32 fullCrc = AZ::Crc32(filename.c_str());
                        u32 maskedCrc = (fullCrc + seedValue) & AssetBuilderSDK::SUBID_MASK_ID;

                        if (assignedSubIds.find(maskedCrc) == assignedSubIds.end())
                        {
                            response.m_outputProducts[it.second].m_legacySubIDs.push_back(maskedCrc);
                            assignedSubIds.insert(maskedCrc);
                            AZ_TracePrintf(SceneAPI::Utilities::LogWindow, "Added legacy sub id 0x%04x - %s\n", maskedCrc, filename.c_str());
                            break;
                        }
                    }

                    filename.clear();
                }
            }
        }

        AZStd::unique_ptr<AssetBuilderSDK::ProcessJobRequest> SceneCompiler::ReadJobRequest(const char* cacheFolder) const
        {
            AZStd::string requestFilePath;
            AzFramework::StringFunc::Path::ConstructFull(cacheFolder, AssetBuilderSDK::s_processJobRequestFileName, requestFilePath);
            AssetBuilderSDK::ProcessJobRequest* result = AZ::Utils::LoadObjectFromFile<AssetBuilderSDK::ProcessJobRequest>(requestFilePath);

            if (!result)
            {
                AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Unable to load ProcessJobRequest. Not enough information to process this file %s.\n", requestFilePath.c_str());
            }

            return AZStd::unique_ptr<AssetBuilderSDK::ProcessJobRequest>(result);
        }

        bool SceneCompiler::WriteResponse(const char* cacheFolder, AssetBuilderSDK::ProcessJobResponse& response, AssetBuilderSDK::ProcessJobResultCode jobResult) const
        {
            AZStd::string responseFilePath;
            AzFramework::StringFunc::Path::ConstructFull(cacheFolder, AssetBuilderSDK::s_processJobResponseFileName, responseFilePath);

            response.m_requiresSubIdGeneration = false;
            response.m_resultCode = jobResult;
            
            bool result = AZ::Utils::SaveObjectToFile(responseFilePath, AZ::DataStream::StreamType::ST_XML, &response);
            return result && jobResult == AssetBuilderSDK::ProcessJobResult_Success;
        }
    } // namespace RC

} // namespace AZ
