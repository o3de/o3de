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
#include "ShaderBuilderUtility.h"

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/Asset/AssetSystemBus.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <AzToolsFramework/AssetCatalog/PlatformAddressedAssetCatalog.h>

#include <AzCore/Asset/AssetManager.h>

#include <AzCore/IO/IOUtils.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>

#include <AtomCore/Serialization/Json/JsonUtils.h>

#include <Atom/RPI.Edit/Common/JsonReportingHelper.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroup.h>

#include <Atom/RHI.Edit/Utils.h>
#include <Atom/RHI.Edit/ShaderPlatformInterface.h>

#include <CommonFiles/Preprocessor.h>
#include <AzslCompiler.h>

#include "ShaderPlatformInterfaceRequest.h"
#include "AtomShaderConfig.h"

namespace AZ
{
    namespace ShaderBuilder
    {
        namespace ShaderBuilderUtility
        {
            static const char* ShaderBuilderUtilityName = "ShaderBuilderUtility";

            Outcome<RPI::ShaderSourceData, AZStd::string> LoadShaderDataJson(const AZStd::string& fullPathToJsonFile)
            {
                RPI::ShaderSourceData shaderSourceData;

                auto document = JsonSerializationUtils::ReadJsonFile(fullPathToJsonFile);

                if (!document.IsSuccess())
                {
                    return Failure(document.GetError());
                }

                JsonDeserializerSettings settings;
                RPI::JsonReportingHelper reportingHelper;
                reportingHelper.Attach(settings);

                JsonSerialization::Load(shaderSourceData, document.GetValue(), settings);

                return AZ::Success(shaderSourceData);
            }

            void GetAbsolutePathToAzslFile(const AZStd::string& shaderTemplatePathAndFile, AZStd::string specifiedShaderPathAndName, AZStd::string& absoluteAzslPath)
            {
                AZStd::string sourcePath;

                AzFramework::StringFunc::Path::GetFullPath(shaderTemplatePathAndFile.data(), sourcePath);
                AzFramework::StringFunc::Path::Normalize(specifiedShaderPathAndName);

                bool shaderNameHasPath = (specifiedShaderPathAndName.find(AZ_CORRECT_FILESYSTEM_SEPARATOR) != AZStd::string::npos);

                // Join will handle overlapping directory structures for us 
                AzFramework::StringFunc::Path::Join(sourcePath.data(), specifiedShaderPathAndName.data(), absoluteAzslPath, shaderNameHasPath /* handle directory overlap? */, false /* be case insensitive? */);

                AzFramework::StringFunc::Path::ReplaceExtension(absoluteAzslPath, "azsl");
            }

            uint32_t MakeDebugByproductSubId(RHI::APIType apiType, const AZStd::string& productFileName)
            {
                // bits: ----- 24 -----|-   4    -|-   4   -
                //          fn hash    | id + api |   0xF
                uint32_t subId = 0xF;    // to avoid collisions with subid of other source outputs using RPI::ShaderAssetSubId::GeneratedSource + api
                uint32_t id_api = static_cast<uint32_t>(RPI::ShaderAssetSubId::DebugByProduct);
                id_api += apiType;
                id_api <<= 4;
                subId |= id_api;
                size_t fnHash = AZStd::hash<AZStd::string>()(productFileName);
                subId |= static_cast<uint32_t>(fnHash) & 0xFFFFFF00;
                return subId;
            }

            static bool LoadShaderResourceGroupAssets(
                [[maybe_unused]] const char* BuilderName,
                const SrgDataContainer& resourceGroups,
                ShaderResourceGroupAssets& srgAssets)
            {
                bool readSRGsSuccessfuly = true;

                // Load all SRGs included in source file 
                for (const SrgData& srgData : resourceGroups)
                {
                    Data::AssetId assetId = {};
                    AZStd::string srgFilePath = "";

                    srgFilePath = srgData.m_containingFileName;
                    AzFramework::StringFunc::Path::Normalize(srgFilePath);

                    bool assetFound = false;
                    Data::AssetInfo sourceInfo;
                    AZStd::string watchFolder;
                    AzToolsFramework::AssetSystemRequestBus::BroadcastResult(assetFound, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath, srgFilePath.c_str(), sourceInfo, watchFolder);

                    if (!assetFound)
                    {
                        AZ_Error(BuilderName, false, "Could not find asset identified by path '%s'", srgFilePath.c_str());
                        readSRGsSuccessfuly = false;
                        continue;
                    }

                    assetId.m_guid = sourceInfo.m_assetId.m_guid;
                    assetId.m_subId = static_cast<uint32_t>(AZStd::hash<AZStd::string>()(srgData.m_name) & 0xFFFFFFFF);

                    Data::Asset<RPI::ShaderResourceGroupAsset> asset = Data::AssetManager::Instance().GetAsset<RPI::ShaderResourceGroupAsset>(
                        assetId, AZ::Data::AssetLoadBehavior::PreLoad);
                    asset.BlockUntilLoadComplete();
                    if (!asset.IsReady())
                    {
                        using Status = Data::AssetData::AssetStatus;
                        AZStd::string statusString = asset.GetStatus() == Status::Loading ? "loading"
                            : asset.GetStatus() == Status::ReadyPreNotify ? "ready-pre-notify"
                            : asset.GetStatus() == Status::Error ? "error" : "not-loaded/ready/unknown";

                        AZ_Error(BuilderName, false, "Searching SRG [%s]: Could not load SRG asset. (asset status [%s]) AssetId='%s' Path='%s'",
                            srgData.m_name.c_str(),
                            statusString.c_str(),
                            assetId.ToString<AZStd::string>().c_str(), srgFilePath.c_str());
                        readSRGsSuccessfuly = false;
                        continue;
                    }
                    else if (!asset->IsValid())
                    {
                        AZ_Error(BuilderName, false, "SRG asset has no layout information. AssetId='%s' Path='%s'",
                            assetId.ToString<AZStd::string>().c_str(), srgFilePath.c_str());
                        readSRGsSuccessfuly = false;
                        continue;
                    }

                    srgAssets.push_back(asset);
                }

                return readSRGsSuccessfuly;
            }

            AZStd::shared_ptr<ShaderFiles> PrepareSourceInput(
                [[maybe_unused]] const char* builderName,
                const AZStd::string& shaderAssetSourcePath,
                RPI::ShaderSourceData& sourceAsset)
            {
                auto shaderAssetSourceFileParseOutput = ShaderBuilderUtility::LoadShaderDataJson(shaderAssetSourcePath);
                if (!shaderAssetSourceFileParseOutput.IsSuccess())
                {
                    AZ_Error(builderName, false, "Failed to load/parse Shader Descriptor JSON: %s", shaderAssetSourceFileParseOutput.GetError().c_str());
                    return nullptr;
                }
                sourceAsset = AZStd::move(shaderAssetSourceFileParseOutput.GetValue());

                AZStd::shared_ptr<ShaderFiles> files(new ShaderFiles);
                const AZStd::string& specifiedAzslName = sourceAsset.m_source;
                ShaderBuilderUtility::GetAbsolutePathToAzslFile(shaderAssetSourcePath, specifiedAzslName, files->m_azslSourceFullPath);

                // specifiedAzslName may have a relative path on it so need to strip it
                AzFramework::StringFunc::Path::GetFileName(specifiedAzslName.c_str(), files->m_azslFileName);
                return files;
            }

            AssetBuilderSDK::ProcessJobResultCode PopulateAzslDataFromJsonFiles(
                const char* BuilderName,
                const AzslSubProducts::Paths& pathOfJsonFiles,
                AzslData& azslData,
                ShaderResourceGroupAssets& srgAssets,
                RPI::Ptr<RPI::ShaderOptionGroupLayout> shaderOptionGroupLayout,
                BindingDependencies& bindingDependencies,
                RootConstantData& rootConstantData)
            {
                AzslCompiler azslc(azslData.m_preprocessedFullPath);  // set the input file for eventual error messages, but the compiler won't be called on it.
                bool allReadSuccess = true;
                // read: input assembly reflection
                //       shader resource group reflection
                //       options reflection
                //       binding dependencies reflection
                int indicesOfInterest[] = { AzslSubProducts::ia, AzslSubProducts::srg, AzslSubProducts::options, AzslSubProducts::bindingdep };
                AZStd::unordered_map<int, Outcome<rapidjson::Document, AZStd::string>> outcomes;
                for (int i : indicesOfInterest)
                {
                    outcomes[i] = JsonSerializationUtils::ReadJsonFile(pathOfJsonFiles[i]);
                    if (!outcomes[i].IsSuccess())
                    {
                        AZ_Error(BuilderName, false, "%s", outcomes[i].GetError().c_str());
                        allReadSuccess = false;
                    }
                }
                if (!allReadSuccess)
                {
                    return AssetBuilderSDK::ProcessJobResult_Failed;
                }
                
                // Get full list of functions eligible for vertex shader entry points
                // along with metadata for constructing the InputAssembly for each of them
                if (!azslc.ParseIaPopulateFunctionData(outcomes[AzslSubProducts::ia].GetValue(), azslData.m_topData.m_functions))
                {
                    return AssetBuilderSDK::ProcessJobResult_Failed;
                }
                
                // Each SRG is built as a separate asset in the SrgLayoutBuilder, here we just
                // build the list and load the data from multiple dependency assets.
                if (!azslc.ParseSrgPopulateSrgData(outcomes[AzslSubProducts::srg].GetValue(), azslData.m_topData.m_srgData))
                {
                    return AssetBuilderSDK::ProcessJobResult_Failed;
                }

                // Add all Shader Resource Group Assets that were defined in the shader code to the shader asset
                if (!LoadShaderResourceGroupAssets(BuilderName, azslData.m_topData.m_srgData, srgAssets))
                {
                    AZ_Error(BuilderName, false, "Failed to obtain shader resource group assets");
                    return AssetBuilderSDK::ProcessJobResult_Failed;
                }

                // The shader options define what options are available, what are the allowed values/range
                // for each option and what is its default value.
                if (!azslc.ParseOptionsPopulateOptionGroupLayout(outcomes[AzslSubProducts::options].GetValue(), shaderOptionGroupLayout))
                {
                    AZ_Error(BuilderName, false, "Failed to find a valid list of shader options!");
                    return AssetBuilderSDK::ProcessJobResult_Failed;
                }

                // It analyzes the shader external bindings (all SRG contents)
                // and informs us on register indexes and shader stages using these resources
                if (!azslc.ParseBindingdepPopulateBindingDependencies(outcomes[AzslSubProducts::bindingdep].GetValue(), bindingDependencies))   // consuming data from binding-dep
                {
                    AZ_Error(BuilderName, false, "Failed to obtain shader resource binding reflection");
                    return AssetBuilderSDK::ProcessJobResult_Failed;
                }

                // access the root constants reflection
                if (!azslc.ParseSrgPopulateRootConstantData(outcomes[AzslSubProducts::srg].GetValue(), rootConstantData))    // consuming data from --srg ("InlineConstantBuffer" subjson section)
                {
                    AZ_Error(BuilderName, false, "Failed to obtain root constant data reflection");
                    return AssetBuilderSDK::ProcessJobResult_Failed;
                }

                return AssetBuilderSDK::ProcessJobResult_Success;
            }
   
            RHI::ShaderHardwareStage ToAssetBuilderShaderType(RPI::ShaderStageType stageType)
            {
                switch (stageType)
                {
                case RPI::ShaderStageType::Compute:
                    return RHI::ShaderHardwareStage::Compute;
                case RPI::ShaderStageType::Fragment:
                    return RHI::ShaderHardwareStage::Fragment;
                case RPI::ShaderStageType::Geometry:
                    return RHI::ShaderHardwareStage::Geometry;
                case RPI::ShaderStageType::TessellationControl:
                    return RHI::ShaderHardwareStage::TessellationControl;
                case RPI::ShaderStageType::TessellationEvaluation:
                    return RHI::ShaderHardwareStage::TessellationEvaluation;
                case RPI::ShaderStageType::Vertex:
                    return RHI::ShaderHardwareStage::Vertex;
                case RPI::ShaderStageType::RayTracing:
                    return RHI::ShaderHardwareStage::RayTracing;
                }
                AZ_Assert(false, "Unable to find Shader stage given RPI ShaderStage %d", stageType);
                return RHI::ShaderHardwareStage::Invalid;
            }

            //! the binding dependency structure may store lots of high level function names which are not entry points
            static void PruneNonEntryFunctions(BindingDependencies& bindingDependencies /*inout*/, const MapOfStringToStageType& shaderEntryPoints)
            {
                auto cleaner = [&shaderEntryPoints](BindingDependencies::FunctionsNameVector& functionVector)
                {
                    functionVector.erase(
                        AZStd::remove_if(
                            functionVector.begin(),
                            functionVector.end(),
                            [&shaderEntryPoints](const AZStd::string& functionName)
                            {
                                return shaderEntryPoints.find(functionName) == shaderEntryPoints.end();
                            }),
                        functionVector.end());
                };
                for (auto& srg : bindingDependencies.m_orderedSrgs)
                {
                    cleaner(srg.m_srgConstantsDependencies.m_binding.m_dependentFunctions);
                    AZStd::for_each(
                        srg.m_resources.begin(),
                        srg.m_resources.end(),
                        [&cleaner](decltype(*srg.m_resources.begin())& nameResourcePair)
                        {
                            cleaner(nameResourcePair.second.m_dependentFunctions);
                        });
                }
            }
    
            RHI::Ptr<RHI::PipelineLayoutDescriptor> BuildPipelineLayoutDescriptorForApi(
                [[maybe_unused]] const char* BuilderName,
                RHI::ShaderPlatformInterface* shaderPlatformInterface,
                BindingDependencies& bindingDependencies /*inout*/,
                const ShaderResourceGroupAssets& srgAssets,
                const MapOfStringToStageType& shaderEntryPoints,
                const RHI::ShaderCompilerArguments& shaderCompilerArguments,
                const RootConstantData* rootConstantData /*= nullptr*/)
            {        
                PruneNonEntryFunctions(bindingDependencies, shaderEntryPoints);
            
                // Translates from a list of function names that use a resource to a shader stage mask.
                auto getRHIShaderStageMask = [&shaderEntryPoints](const BindingDependencies::FunctionsNameVector& functions)
                {
                    RHI::ShaderStageMask mask = RHI::ShaderStageMask::None;
                    // Iterate through all the functions that are using the resource.
                    for (const auto& functionName : functions)
                    {
                        // Search the function name into the list of valid entry points into the shader.
                        auto findId = AZStd::find_if(shaderEntryPoints.begin(), shaderEntryPoints.end(), [&functionName, &mask](const auto& item)
                            {
                                return item.first == functionName;
                            });
        
                        if (findId != shaderEntryPoints.end())
                        {
                            // Use the entry point shader stage type to calculate the mask.
                            RHI::ShaderHardwareStage hardwareStage = ToAssetBuilderShaderType(findId->second);
                            mask |= static_cast<RHI::ShaderStageMask>(AZ_BIT(static_cast<uint32_t>(RHI::ToRHIShaderStage(hardwareStage))));
                        }
                    }
        
                    return mask;
                };
            
                // Build general PipelineLayoutDescriptor data that is provided for all platforms
                RHI::Ptr<RHI::PipelineLayoutDescriptor> pipelineLayoutDescriptor = shaderPlatformInterface->CreatePipelineLayoutDescriptor();
                RHI::ShaderPlatformInterface::ShaderResourceGroupInfoList srgInfos;
                for (const auto& srgAsset : srgAssets)
                {
                    // Search the binding info for a Shader Resource Group.
                    AZStd::string_view srgName = srgAsset->GetName().GetStringView();
                    const BindingDependencies::SrgResources* srgResources = bindingDependencies.GetSrg(srgName);
                    if (!srgResources)
                    {
                        AZ_Error(BuilderName, false, "SRG %s not found in the dependency dataset", srgName.data());
                        return nullptr;
                    }
        
                    RHI::ShaderResourceGroupBindingInfo srgBindingInfo;
                    srgBindingInfo.m_spaceId = srgResources->m_registerSpace;
                    const RHI::ShaderResourceGroupLayout* layout = srgAsset->GetLayout(shaderPlatformInterface->GetAPIType());
                    // Calculate the binding in for the constant data. All constant data share the same binding info.
                    srgBindingInfo.m_constantDataBindingInfo = {
                        getRHIShaderStageMask(srgResources->m_srgConstantsDependencies.m_binding.m_dependentFunctions),
                        srgResources->m_srgConstantsDependencies.m_binding.m_registerId };
                    // Calculate the binding info for each resource of the Shader Resource Group.
                    for (auto const& resource : srgResources->m_resources)
                    {
                        auto const& resourceInfo = resource.second;
                        srgBindingInfo.m_resourcesRegisterMap.insert(
                            { AZ::Name(resourceInfo.m_selfName),
                            RHI::ResourceBindingInfo(getRHIShaderStageMask(resourceInfo.m_dependentFunctions), resourceInfo.m_registerId) });
                    }
                    pipelineLayoutDescriptor->AddShaderResourceGroupLayoutInfo(*layout, srgBindingInfo);
                    srgInfos.push_back(RHI::ShaderPlatformInterface::ShaderResourceGroupInfo{ layout, srgBindingInfo });
                }
        
                RHI::Ptr<RHI::ConstantsLayout> rootConstantsLayout = RHI::ConstantsLayout::Create();
                if (rootConstantData)
                {
                    for (const auto& constantData : rootConstantData->m_constants)
                    {
                        RHI::ShaderInputConstantDescriptor rootConstantDesc(
                            constantData.m_nameId,
                            constantData.m_constantByteOffset,
                            constantData.m_constantByteSize,
                            rootConstantData->m_bindingInfo.m_registerId);
        
                        rootConstantsLayout->AddShaderInput(rootConstantDesc);
                    }
                }
        
                if (!rootConstantsLayout->Finalize())
                {
                    AZ_Error(BuilderName, false, "Failed to finalize root constants layout");
                    return nullptr;
                }

                pipelineLayoutDescriptor->SetRootConstantsLayout(*rootConstantsLayout);
        
                RHI::ShaderPlatformInterface::RootConstantsInfo rootConstantInfo;
                if (rootConstantData)
                {
                    rootConstantInfo.m_spaceId = rootConstantData->m_bindingInfo.m_space;
                    rootConstantInfo.m_registerId = rootConstantData->m_bindingInfo.m_registerId;
                }
                else
                {
                    RootConstantData dummyRootConstantData;
                    rootConstantInfo.m_spaceId = dummyRootConstantData.m_bindingInfo.m_space;
                    rootConstantInfo.m_registerId = dummyRootConstantData.m_bindingInfo.m_registerId;
                }
                rootConstantInfo.m_totalSizeInBytes = rootConstantsLayout->GetDataSize();
            
                // Build platform-specific PipelineLayoutDescriptor data, and finalize
                if (!shaderPlatformInterface->BuildPipelineLayoutDescriptor(pipelineLayoutDescriptor, srgInfos, rootConstantInfo, shaderCompilerArguments))
                {
                    AZ_Error(BuilderName, false, "Failed to build pipeline layout descriptor");
                    return nullptr;
                }
            
                return pipelineLayoutDescriptor;
            }

            static AZStd::string DumpCode(
                [[maybe_unused]] const char* builderName,
                const AZStd::string& codeInString,
                const AZStd::string& dumpDirectory,
                const AZStd::string& stemName,
                const AZStd::string& apiTypeString,
                const AZStd::string& extension)
            {
                AZStd::string finalFilePath;
                AZStd::string formatted;
                if (apiTypeString.empty())
                {
                    formatted = AZStd::string::format("%s.%s", stemName.c_str(), extension.c_str());
                }
                else
                {
                    formatted = AZStd::string::format("%s.%s.%s", stemName.c_str(), apiTypeString.c_str(), extension.c_str());
                }
                AzFramework::StringFunc::Path::Join(dumpDirectory.c_str(), formatted.c_str(), finalFilePath, true, true);
                AZ::IO::FileIOStream outFileStream(finalFilePath.data(), AZ::IO::OpenMode::ModeWrite);
                if (!outFileStream.IsOpen())
                {
                    AZ_Error(builderName, false, "Failed to open file to write (%s)\n", finalFilePath.data());
                    return "";
                }

                outFileStream.Write(codeInString.size(), codeInString.data());

                // Prevent warning: "warning: End of input with no newline"
                static constexpr char newLine[] = "\n";
                outFileStream.Write(sizeof(newLine) - 1, newLine);

                outFileStream.Close();

                return finalFilePath;
            }

            AZStd::string DumpPreprocessedCode(const char* builderName, const AZStd::string& preprocessedCode, const AZStd::string& tempDirPath, const AZStd::string& stemName, const AZStd::string& apiTypeString)
            {
                return DumpCode(builderName, preprocessedCode, tempDirPath, stemName, apiTypeString, "azslin");
            }

            AZStd::string DumpAzslPrependedCode(const char* builderName, const AZStd::string& nonPreprocessedYetAzslSource, const AZStd::string& tempDirPath, const AZStd::string& stemName, const AZStd::string& apiTypeString)
            {
                return DumpCode(builderName, nonPreprocessedYetAzslSource, tempDirPath, stemName, apiTypeString, "azsl.prepend");
            }

            AZStd::string ExtractStemName(const char* path)
            {
                AZStd::string result;
                AzFramework::StringFunc::Path::GetFileName(path, result);
                return result;
            }

            AZStd::vector<RHI::ShaderPlatformInterface*> DiscoverValidShaderPlatformInterfaces(const AssetBuilderSDK::PlatformInfo& info)
            {
                AZStd::vector<RHI::ShaderPlatformInterface*> platformInterfaces;
                ShaderPlatformInterfaceRequestBus::BroadcastResult(platformInterfaces, &ShaderPlatformInterfaceRequest::GetShaderPlatformInterface, info);
                // filter out nulls:
                platformInterfaces.erase(AZStd::remove_if(AZ_BEGIN_END(platformInterfaces), [](auto* element) { return element == nullptr; }), platformInterfaces.end());
                return platformInterfaces;
            }

            static void ReadShaderCompilerProfiling([[maybe_unused]] const char* builderName, RHI::ShaderCompilerProfiling& shaderCompilerProfiling, AZStd::string_view shaderPath)
            {
                AZStd::string folderPath;
                AzFramework::StringFunc::Path::GetFullPath(shaderPath.data(), folderPath);

                AZStd::vector<AZStd::string> fileNames;

                IO::FileIOBase::GetInstance()->FindFiles(folderPath.c_str(), "*.profiling", [&](const char* filePath) -> bool
                    {
                        fileNames.push_back(AZStd::string(filePath));
                        return true;
                    });

                for (const AZStd::string& fileName : fileNames)
                {
                    RHI::ShaderCompilerProfiling profiling;
                    auto loadResult = AZ::JsonSerializationUtils::LoadObjectFromFile<RHI::ShaderCompilerProfiling>(profiling, fileName);

                    if (!loadResult.IsSuccess())
                    {
                        AZ_Error(builderName, false, "Failed to load shader compiler profiling from file [%s]", fileName.data());
                        AZ_Error(builderName, false, "Loading issues: %s", loadResult.GetError().data());
                        continue;
                    }

                    shaderCompilerProfiling.m_entries.insert(
                        shaderCompilerProfiling.m_entries.begin(),
                        profiling.m_entries.begin(), profiling.m_entries.end());
                }
            }

            void LogProfilingData(const char* builderName, AZStd::string_view shaderPath)
            {
                RHI::ShaderCompilerProfiling shaderCompilerProfiling;
                ReadShaderCompilerProfiling(builderName, shaderCompilerProfiling, shaderPath);

                struct ProfilingPerCompiler
                {
                    size_t m_calls;
                    float m_totalElapsedTime;
                };

                // The key is the compiler executable path.
                AZStd::unordered_map<AZStd::string, ProfilingPerCompiler> profilingPerCompiler;

                for (const RHI::ShaderCompilerProfiling::Entry& shaderCompilerProfilingEntry : shaderCompilerProfiling.m_entries)
                {
                    auto it = profilingPerCompiler.find(shaderCompilerProfilingEntry.m_executablePath);
                    if (it == profilingPerCompiler.end())
                    {
                        profilingPerCompiler.emplace(
                            shaderCompilerProfilingEntry.m_executablePath,
                            ProfilingPerCompiler{ 1, shaderCompilerProfilingEntry.m_elapsedTimeSeconds });
                    }
                    else
                    {
                        (*it).second.m_calls++;
                        (*it).second.m_totalElapsedTime += shaderCompilerProfilingEntry.m_elapsedTimeSeconds;
                    }
                }

                for (const auto& profiling : profilingPerCompiler)
                {
                    AZ_TracePrintf(builderName, "Compiler: %s\n>\tCalls: %d\n>\tTime: %.2f seconds\n",
                        profiling.first.c_str(), profiling.second.m_calls, profiling.second.m_totalElapsedTime);
                }
            }

            uint32_t MakeAzslBuildProductSubId(RPI::ShaderAssetSubId subId, RHI::APIType apiType)
            {
                auto subIdMaxEnumerator = RPI::ShaderAssetSubId::GeneratedSource;
                // separate bit space between subid enum, and api-type:
                int shiftLeft = static_cast<uint32_t>(log2(static_cast<uint32_t>(subIdMaxEnumerator))) + 1;
                return static_cast<uint32_t>(subId) + (apiType << shiftLeft);
            }

            Outcome<AzslSubProducts::Paths> ObtainBuildArtifactsFromAzslBuilder([[maybe_unused]] const char* builderName, const AZStd::string& sourceFullPath, RHI::APIType apiType, const AZStd::string& platform)
            {
                AzslSubProducts::Paths products;

                // platform id from identifier
                AzFramework::PlatformId platformId = AzFramework::PlatformId::PC;
                if (platform == "pc")
                {
                    platformId = AzFramework::PlatformId::PC;
                }
                else if (platform == "osx_gl")
                {
                    platformId = AzFramework::PlatformId::OSX;
                }
                else if (platform == "es3")
                {
                    platformId = AzFramework::PlatformId::ES3;
                }
                else if (platform == "ios")
                {
                    platformId = AzFramework::PlatformId::IOS;
                }

                for (RPI::ShaderAssetSubId sub : AzslSubProducts::SubList)
                {
                    uint32_t assetSubId = MakeAzslBuildProductSubId(sub, apiType);
                    auto assetIdOutcome = RPI::AssetUtils::MakeAssetId(sourceFullPath, assetSubId);
                    AZ_Error(builderName, assetIdOutcome.IsSuccess(), "Missing AZSL product %s, for sub %d", sourceFullPath.c_str(), (uint32_t)sub);
                    if (!assetIdOutcome.IsSuccess())
                    {
                        return Failure();
                    }
                    Data::AssetId assetId = assetIdOutcome.TakeValue();
                    // get the relative path:
                    AZStd::string assetPath;
                    Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &Data::AssetCatalogRequests::GetAssetPathById, assetId);

                    // get the root:
                    AZStd::string assetRoot = AzToolsFramework::PlatformAddressedAssetCatalog::GetAssetRootForPlatform(platformId);
                    // join
                    AZStd::string assetFullPath;
                    AzFramework::StringFunc::Path::Join(assetRoot.c_str(), assetPath.c_str(), assetFullPath);
                    bool fileExists = IO::FileIOBase::GetInstance()->Exists(assetFullPath.c_str()) && !IO::FileIOBase::GetInstance()->IsDirectory(assetFullPath.c_str());
                    if (!fileExists)
                    {
                        return Failure();
                    }
                    products.push_back(assetFullPath);
                }
                return AZ::Success(products);
            }

            // See header for info.
            // REMARK: The approach to string searching and matching done in this function is kind of naive
            // because the strings can match text within a comment block, etc. So it is not 100% fool proof.
            // We would need proper grammar parsing to reach 100% confidence.
            // [GFX TODO][ATOM-5302][ATOM-5308] The following function will be removed once, both, [ATOM-5302] & [ATOM-5308] are addressed, and
            // azslc allows redundant SrgSemantics for "partial" qualified SRGs.
            SrgSkipFileResult ShouldSkipFileForSrgProcessing([[maybe_unused]] const char* builderName, const AZStd::string_view fullPath)
            {
                AZ::IO::FileIOStream stream(fullPath.data(), AZ::IO::OpenMode::ModeRead);
                if (!stream.IsOpen())
                {
                    AZ_Warning(builderName, false, "\"%s\" source file could not be opened.", fullPath.data());
                    return SrgSkipFileResult::Error;
                }

                if (!stream.CanRead())
                {
                    AZ_Warning(builderName, false, "\"%s\" source file could not be read.", fullPath.data());
                    return SrgSkipFileResult::Error;
                }

                // Do a quick check for "ShaderResourceGroup" to determine if this file might even have a ShaderResourceGroup to parse.
                AZStd::string fileContents;
                fileContents.resize(stream.GetLength());
                stream.Read(stream.GetLength(), fileContents.data());

                static const AZStd::regex partialSrgRegex("\n\\s*partial\\s+ShaderResourceGroup\\s+", AZStd::regex::ECMAScript);
                if (AZStd::regex_search(fileContents.data(), partialSrgRegex))
                {
                    // It is considered a programmer's error if a file declares both, non-partial and partial SRGs.
                    static const AZStd::regex srgRegex("\n\\s*ShaderResourceGroup\\s+", AZStd::regex::ECMAScript);
                    if (AZStd::regex_search(fileContents.data(), srgRegex))
                    {
                        AZ_Error(builderName, false, "\"%s\" defines both partial and non-partial SRGs.", fullPath.data());
                        return SrgSkipFileResult::Error;
                    }
                    // We should skip files that define partial Srgs because an srgi file will eventually
                    // include it.
                    return SrgSkipFileResult::SkipFile;
                }

                // This is an optimization to avoid unnecessary preprocessing a whole tree of azsli files; we can detect when a
                // ShaderResourceGroupAsset wouldn't be produced and return early. Note, we could remove this early-return check
                // if the preprocessing code below is updated to not follow include paths [ATOM-5302].
                // (Note this optimization is not valid for srgi files because those do require scanning all include paths)"
                if (fileContents.find("ShaderResourceGroup") == AZStd::string::npos)
                {
                    // No ShaderResourceGroup in this file, so there's nothing to do. Create no jobs and report success.
                    return SrgSkipFileResult::SkipFile;
                }

                return SrgSkipFileResult::ContinueProcess;
            }
        }  // namespace ShaderBuilderUtility
    } // namespace ShaderBuilder
} // AZ
