/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
#include <AzCore/std/string/regex.h>

#include <AzCore/Serialization/Json/JsonUtils.h>

#include <Atom/RPI.Edit/Common/JsonReportingHelper.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroup.h>

#include <Atom/RHI.Edit/Utils.h>
#include <Atom/RHI.Edit/ShaderPlatformInterface.h>

#include <CommonFiles/Preprocessor.h>
#include <AzslCompiler.h>

#include "ShaderPlatformInterfaceRequest.h"
#include "ShaderBuilder_Traits_Platform.h"
#include "AtomShaderConfig.h"

#include "SrgLayoutUtility.h"

namespace AZ
{
    namespace ShaderBuilder
    {
        namespace ShaderBuilderUtility
        {
            [[maybe_unused]] static constexpr char ShaderBuilderUtilityName[] = "ShaderBuilderUtility";

            Outcome<RPI::ShaderSourceData, AZStd::string> LoadShaderDataJson(const AZStd::string& fullPathToJsonFile)
            {
                RPI::ShaderSourceData shaderSourceData;

                auto document = JsonSerializationUtils::ReadJsonFile(fullPathToJsonFile, AZ::RPI::JsonUtils::DefaultMaxFileSize);

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

            void GetAbsolutePathToAzslFile(const AZStd::string& shaderSourceFileFullPath, AZStd::string specifiedShaderPathAndName, AZStd::string& absoluteAzslPath)
            {
                AZStd::string sourcePath;

                AzFramework::StringFunc::Path::GetFullPath(shaderSourceFileFullPath.c_str(), sourcePath);
                AzFramework::StringFunc::Path::Normalize(specifiedShaderPathAndName);

                bool shaderNameHasPath = (specifiedShaderPathAndName.find(AZ_CORRECT_FILESYSTEM_SEPARATOR) != AZStd::string::npos);

                // Join will handle overlapping directory structures for us 
                AzFramework::StringFunc::Path::Join(sourcePath.data(), specifiedShaderPathAndName.data(), absoluteAzslPath, shaderNameHasPath /* handle directory overlap? */, false /* be case insensitive? */);

                // The builders used to automatically set the ".azsl" extension, but no more, because that would make the .shader file confusing to read.
                // Here we just detect the issue and instruct the user what to change.
                // (There's no need to return a failure code, the builder will eventually fail anyway when it can't find the file).
                if (!IO::FileIOBase::GetInstance()->Exists(absoluteAzslPath.c_str()))
                {
                    AZStd::string absoluteAzslPathWithForcedExtension = absoluteAzslPath;
                    AzFramework::StringFunc::Path::ReplaceExtension(absoluteAzslPathWithForcedExtension, "azsl");

                    if (IO::FileIOBase::GetInstance()->Exists(absoluteAzslPathWithForcedExtension.c_str()))
                    {
                        AZ_Error(ShaderBuilderUtilityName, false, "When the .shader file references a .azsl file, it must include the \".azsl\" extension.");
                    }
                }
            }

            AZStd::shared_ptr<ShaderFiles> PrepareSourceInput(
                [[maybe_unused]] const char* builderName,
                const AZStd::string& shaderSourceFileFullPath,
                RPI::ShaderSourceData& sourceAsset)
            {
                auto shaderAssetSourceFileParseOutput = ShaderBuilderUtility::LoadShaderDataJson(shaderSourceFileFullPath);
                if (!shaderAssetSourceFileParseOutput.IsSuccess())
                {
                    AZ_Error(builderName, false, "Failed to load/parse Shader Descriptor JSON: %s", shaderAssetSourceFileParseOutput.GetError().c_str());
                    return nullptr;
                }
                sourceAsset = AZStd::move(shaderAssetSourceFileParseOutput.GetValue());

                AZStd::shared_ptr<ShaderFiles> files(new ShaderFiles);
                const AZStd::string& specifiedAzslName = sourceAsset.m_source;
                ShaderBuilderUtility::GetAbsolutePathToAzslFile(shaderSourceFileFullPath, specifiedAzslName, files->m_azslSourceFullPath);

                // specifiedAzslName may have a relative path on it so need to strip it
                AzFramework::StringFunc::Path::GetFileName(specifiedAzslName.c_str(), files->m_azslFileName);
                return files;
            }

            AssetBuilderSDK::ProcessJobResultCode PopulateAzslDataFromJsonFiles(
                const char* builderName,
                const AzslSubProducts::Paths& pathOfJsonFiles,
                const bool platformUsesRegisterSpaces,
                AzslData& azslData,
                RPI::ShaderResourceGroupLayoutList& srgLayoutList,
                RPI::Ptr<RPI::ShaderOptionGroupLayout> shaderOptionGroupLayout,
                BindingDependencies& bindingDependencies,
                RootConstantData& rootConstantData)
            {
                AzslCompiler azslc(
                    azslData
                        .m_preprocessedFullPath); // set the input file for eventual error messages, but the compiler won't be called on it.
                bool allReadSuccess = true;
                // read: input assembly reflection
                //       shader resource group reflection
                //       options reflection
                //       binding dependencies reflection
                int indicesOfInterest[] = {
                    AzslSubProducts::ia, AzslSubProducts::srg, AzslSubProducts::options, AzslSubProducts::bindingdep};
                AZStd::unordered_map<int, Outcome<rapidjson::Document, AZStd::string>> outcomes;
                for (int i : indicesOfInterest)
                {
                    outcomes[i] = JsonSerializationUtils::ReadJsonFile(pathOfJsonFiles[i], AZ::RPI::JsonUtils::DefaultMaxFileSize);
                    if (!outcomes[i].IsSuccess())
                    {
                        AZ_Error(builderName, false, "%s", outcomes[i].GetError().c_str());
                        allReadSuccess = false;
                    }
                }
                if (!allReadSuccess)
                {
                    return AssetBuilderSDK::ProcessJobResult_Failed;
                }

                // Get full list of functions eligible for vertex shader entry points
                // along with metadata for constructing the InputAssembly for each of them
                if (!azslc.ParseIaPopulateFunctionData(outcomes[AzslSubProducts::ia].GetValue(), azslData.m_functions))
                {
                    return AssetBuilderSDK::ProcessJobResult_Failed;
                }

                // Each SRG is built as a separate asset in the SrgLayoutBuilder, here we just
                // build the list and load the data from multiple dependency assets.
                if (!azslc.ParseSrgPopulateSrgData(outcomes[AzslSubProducts::srg].GetValue(), azslData.m_srgData))
                {
                    return AssetBuilderSDK::ProcessJobResult_Failed;
                }

                // Add all Shader Resource Group Assets that were defined in the shader code to the shader asset
                if (!SrgLayoutUtility::LoadShaderResourceGroupLayouts(builderName, azslData.m_srgData, platformUsesRegisterSpaces, srgLayoutList))
                {
                    AZ_Error(builderName, false, "Failed to obtain shader resource group assets");
                    return AssetBuilderSDK::ProcessJobResult_Failed;
                }

                // The shader options define what options are available, what are the allowed values/range
                // for each option and what is its default value.
                if (!azslc.ParseOptionsPopulateOptionGroupLayout(outcomes[AzslSubProducts::options].GetValue(), shaderOptionGroupLayout))
                {
                    AZ_Error(builderName, false, "Failed to find a valid list of shader options!");
                    return AssetBuilderSDK::ProcessJobResult_Failed;
                }

                // It analyzes the shader external bindings (all SRG contents)
                // and informs us on register indexes and shader stages using these resources
                if (!azslc.ParseBindingdepPopulateBindingDependencies(
                        outcomes[AzslSubProducts::bindingdep].GetValue(), bindingDependencies)) // consuming data from binding-dep
                {
                    AZ_Error(builderName, false, "Failed to obtain shader resource binding reflection");
                    return AssetBuilderSDK::ProcessJobResult_Failed;
                }

                // access the root constants reflection
                if (!azslc.ParseSrgPopulateRootConstantData(
                        outcomes[AzslSubProducts::srg].GetValue(),
                        rootConstantData)) // consuming data from --srg ("RootConstantBuffer" subjson section)
                {
                    AZ_Error(builderName, false, "Failed to obtain root constant data reflection");
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
            static void PruneNonEntryFunctions(
                BindingDependencies& bindingDependencies /*inout*/, const MapOfStringToStageType& shaderEntryPoints)
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
                    formatted = AZStd::string::format("%s_%s.%s", stemName.c_str(), apiTypeString.c_str(), extension.c_str());
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
                return DumpCode(builderName, nonPreprocessedYetAzslSource, tempDirPath, stemName, apiTypeString, "azslprepend");
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


            AZStd::vector<RHI::ShaderPlatformInterface*> DiscoverEnabledShaderPlatformInterfaces(const AssetBuilderSDK::PlatformInfo& info, const RPI::ShaderSourceData& shaderSourceData)
            {
                // Request the list of valid shader platform interfaces for the target platform.
                AZStd::vector<RHI::ShaderPlatformInterface*> platformInterfaces;
                ShaderPlatformInterfaceRequestBus::BroadcastResult(
                    platformInterfaces, &ShaderPlatformInterfaceRequest::GetShaderPlatformInterface, info);

                // Let's remove the unwanted RHI interfaces from the list.
                platformInterfaces.erase(
                    AZStd::remove_if(AZ_BEGIN_END(platformInterfaces),
                        [&](const RHI::ShaderPlatformInterface* shaderPlatformInterface) {
                            return !shaderPlatformInterface ||
                                   shaderSourceData.IsRhiBackendDisabled(shaderPlatformInterface->GetAPIName());
                        }),
                    platformInterfaces.end());
                return platformInterfaces;
            }

            static bool IsValidSupervariantName(const AZStd::string& supervariantName)
            {
                return AZStd::all_of(AZ_BEGIN_END(supervariantName),
                    [](AZStd::string::value_type ch)
                    {
                        return AZStd::is_alnum(ch); // allow alpha numeric only
                    }
                );
            }

            AZStd::vector<RPI::ShaderSourceData::SupervariantInfo> GetSupervariantListFromShaderSourceData(
                const RPI::ShaderSourceData& shaderSourceData)
            {
                AZStd::vector<RPI::ShaderSourceData::SupervariantInfo> supervariants;
                supervariants.reserve(shaderSourceData.m_supervariants.size() + 1);

                // Add the supervariants, always making sure that:
                //  1- The default, nameless, supervariant goes to the front.
                //  2- Each supervariant has a unique name
                AZStd::unordered_set<AZ::Name> uniqueSuperVariants; // This set helps duplicate detection.
                // Although it is not common, it is possible to declare a nameless supervariant.
                bool addedNamelessSupervariant = false;
                for (const auto& supervariantInfo : shaderSourceData.m_supervariants)
                {
                    if (!IsValidSupervariantName(supervariantInfo.m_name.GetStringView()))
                    {
                        AZ_Error(
                            ShaderBuilderUtilityName, false, "The supervariant name: [%s] contains invalid characters. Only [a-zA-Z0-9] are supported",
                            supervariantInfo.m_name.GetCStr());
                        return {}; // Return an empty vector.
                    }
                    if (uniqueSuperVariants.count(supervariantInfo.m_name))
                    {
                        AZ_Error(
                            ShaderBuilderUtilityName, false, "It is invalid to specify more than one supervariant with the same name: [%s]",
                            supervariantInfo.m_name.GetCStr());
                        return {}; // Return an empty vector.
                    }
                    uniqueSuperVariants.emplace(supervariantInfo.m_name);
                    supervariants.push_back(supervariantInfo);
                    if (supervariantInfo.m_name.IsEmpty())
                    {
                        addedNamelessSupervariant = true;
                        // Always move the default, nameless, variant to the begining of the list.
                        AZStd::swap(supervariants.front(), supervariants.back());
                    }
                }
                if (!addedNamelessSupervariant)
                {
                    supervariants.push_back({});
                    // Always move the default, nameless, variant to the begining of the list.
                    AZStd::swap(supervariants.front(), supervariants.back());
                }

                return supervariants;
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

#if defined(AZ_ENABLE_TRACING)
                for (const auto& profiling : profilingPerCompiler)
                {
                    AZ_TracePrintf(builderName, "Compiler: %s\n>\tCalls: %d\n>\tTime: %.2f seconds\n",
                        profiling.first.c_str(), profiling.second.m_calls, profiling.second.m_totalElapsedTime);
                }
#endif
            }

            Outcome<AZStd::string, AZStd::string> ObtainBuildArtifactPathFromShaderAssetBuilder(
                const uint32_t rhiUniqueIndex, const AZStd::string& platformIdentifier, const AZStd::string& shaderJsonPath,
                const uint32_t supervariantIndex, RPI::ShaderAssetSubId shaderAssetSubId)
            {
                // Define a fallback platform ID based on the current host platform
                AzFramework::PlatformId platformId = AZ_TRAIT_ATOM_FALLBACK_ASSET_HOST_PLATFORM;

                if (platformIdentifier == "pc")
                {
                    platformId = AzFramework::PlatformId::PC;
                }
                else if (platformIdentifier == "linux")
                {
                    platformId = AzFramework::PlatformId::LINUX_ID;
                }
                else if (platformIdentifier == "mac")
                {
                    platformId = AzFramework::PlatformId::MAC_ID;
                }
                else if (platformIdentifier == "android")
                {
                    platformId = AzFramework::PlatformId::ANDROID_ID;
                }
                else if (platformIdentifier == "ios")
                {
                    platformId = AzFramework::PlatformId::IOS;
                }
                else if (platformIdentifier == "server")
                {
                    platformId = AzFramework::PlatformId::SERVER;
                }

                uint32_t assetSubId = RPI::ShaderAsset::MakeProductAssetSubId(rhiUniqueIndex, supervariantIndex, aznumeric_cast<uint32_t>(shaderAssetSubId));
                auto assetIdOutcome = RPI::AssetUtils::MakeAssetId(shaderJsonPath, assetSubId);
                if (!assetIdOutcome.IsSuccess())
                {
                    return Failure(AZStd::string::format(
                        "Missing ShaderAssetBuilder product %s, for sub %d", shaderJsonPath.c_str(), (uint32_t)shaderAssetSubId));
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
                bool fileExists = IO::FileIOBase::GetInstance()->Exists(assetFullPath.c_str()) &&
                    !IO::FileIOBase::GetInstance()->IsDirectory(assetFullPath.c_str());
                if (!fileExists)
                {
                    return Failure(AZStd::string::format(
                        "asset [%s] from shader source %s and subId %d doesn't exist", assetFullPath.c_str(), shaderJsonPath.c_str(),
                        (uint32_t)shaderAssetSubId));
                }
                return AZ::Success(assetFullPath);
            }

            RHI::Ptr<RHI::PipelineLayoutDescriptor> BuildPipelineLayoutDescriptorForApi(
                [[maybe_unused]] const char* builderName, const RPI::ShaderResourceGroupLayoutList& srgLayoutList, const MapOfStringToStageType& shaderEntryPoints,
                const RHI::ShaderCompilerArguments& shaderCompilerArguments, const RootConstantData& rootConstantData,
                RHI::ShaderPlatformInterface* shaderPlatformInterface, BindingDependencies& bindingDependencies /*inout*/)
            {
                PruneNonEntryFunctions(bindingDependencies, shaderEntryPoints);

                // Translates from a list of function names that use a resource to a shader stage mask.
                auto getRHIShaderStageMask = [&shaderEntryPoints](const BindingDependencies::FunctionsNameVector& functions) {
                    RHI::ShaderStageMask mask = RHI::ShaderStageMask::None;
                    // Iterate through all the functions that are using the resource.
                    for (const auto& functionName : functions)
                    {
                        // Search the function name into the list of valid entry points into the shader.
                        auto findId =
                            AZStd::find_if(shaderEntryPoints.begin(), shaderEntryPoints.end(), [&functionName](const auto& item) {
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
                RHI::Ptr<RHI::PipelineLayoutDescriptor> pipelineLayoutDescriptor =
                    shaderPlatformInterface->CreatePipelineLayoutDescriptor();
                RHI::ShaderPlatformInterface::ShaderResourceGroupInfoList srgInfos;
                for (const auto& srgLayout : srgLayoutList)
                {
                    // Search the binding info for a Shader Resource Group.
                    AZStd::string_view srgName = srgLayout->GetName().GetStringView();
                    const BindingDependencies::SrgResources* srgResources = bindingDependencies.GetSrg(srgName);
                    if (!srgResources)
                    {
                        AZ_Error(builderName, false, "SRG %s not found in the dependency dataset", srgName.data());
                        return nullptr;
                    }

                    RHI::ShaderResourceGroupBindingInfo srgBindingInfo;
                    srgBindingInfo.m_spaceId = srgResources->m_registerSpace;
                    const RHI::ShaderResourceGroupLayout* layout = srgLayout.get();
                    // Calculate the binding in for the constant data. All constant data share the same binding info.
                    srgBindingInfo.m_constantDataBindingInfo = {
                        getRHIShaderStageMask(srgResources->m_srgConstantsDependencies.m_binding.m_dependentFunctions),
                        srgResources->m_srgConstantsDependencies.m_binding.m_registerId};
                    // Calculate the binding info for each resource of the Shader Resource Group.
                    for (auto const& resource : srgResources->m_resources)
                    {
                        auto const& resourceInfo = resource.second;
                        srgBindingInfo.m_resourcesRegisterMap.insert(
                            {AZ::Name(resourceInfo.m_selfName),
                             RHI::ResourceBindingInfo(
                                 getRHIShaderStageMask(resourceInfo.m_dependentFunctions), resourceInfo.m_registerId)});
                    }
                    pipelineLayoutDescriptor->AddShaderResourceGroupLayoutInfo(*layout, srgBindingInfo);
                    srgInfos.push_back(RHI::ShaderPlatformInterface::ShaderResourceGroupInfo{layout, srgBindingInfo});
                }

                RHI::Ptr<RHI::ConstantsLayout> rootConstantsLayout = RHI::ConstantsLayout::Create();
                for (const auto& constantData : rootConstantData.m_constants)
                {
                    RHI::ShaderInputConstantDescriptor rootConstantDesc(
                        constantData.m_nameId, constantData.m_constantByteOffset, constantData.m_constantByteSize,
                        rootConstantData.m_bindingInfo.m_registerId);

                    rootConstantsLayout->AddShaderInput(rootConstantDesc);
                }


                if (!rootConstantsLayout->Finalize())
                {
                    AZ_Error(builderName, false, "Failed to finalize root constants layout");
                    return nullptr;
                }

                pipelineLayoutDescriptor->SetRootConstantsLayout(*rootConstantsLayout);

                RHI::ShaderPlatformInterface::RootConstantsInfo rootConstantInfo;
                rootConstantInfo.m_spaceId = rootConstantData.m_bindingInfo.m_space;
                rootConstantInfo.m_registerId = rootConstantData.m_bindingInfo.m_registerId;
                rootConstantInfo.m_totalSizeInBytes = rootConstantsLayout->GetDataSize();

                // Build platform-specific PipelineLayoutDescriptor data, and finalize
                if (!shaderPlatformInterface->BuildPipelineLayoutDescriptor(
                        pipelineLayoutDescriptor, srgInfos, rootConstantInfo, shaderCompilerArguments))
                {
                    AZ_Error(builderName, false, "Failed to build pipeline layout descriptor");
                    return nullptr;
                }

                return pipelineLayoutDescriptor;
            }

            static bool IsSystemValueSemantic(const AZStd::string_view semantic)
            {
                // https://docs.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-semantics#system-value-semantics
                return AzFramework::StringFunc::StartsWith(semantic, "sv_", false);
            }

            static bool CreateShaderInputContract(
                const AzslData& azslData,
                const AZStd::string& vertexShaderName,
                const RPI::ShaderOptionGroupLayout& shaderOptionGroupLayout,
                const AZStd::string& pathToIaJson,
                RPI::ShaderInputContract& contract)
            {
                StructData inputStruct;
                inputStruct.m_id = "";

                auto jsonOutcome = JsonSerializationUtils::ReadJsonFile(pathToIaJson, AZ::RPI::JsonUtils::DefaultMaxFileSize);
                if (!jsonOutcome.IsSuccess())
                {
                    AZ_Error(ShaderBuilderUtilityName, false, "%s", jsonOutcome.GetError().c_str());
                    return AssetBuilderSDK::ProcessJobResult_Failed;
                }

                AzslCompiler azslc(azslData.m_preprocessedFullPath);
                if (!azslc.ParseIaPopulateStructData(jsonOutcome.GetValue(), vertexShaderName, inputStruct))
                {
                    AZ_Error(ShaderBuilderUtilityName, false, "Failed to parse input layout\n");
                    return false;
                }

                if (inputStruct.m_id.empty())
                {
                    AZ_Error(
                        ShaderBuilderUtilityName, false, "Failed to find the input struct for vertex shader %s.",
                        vertexShaderName.c_str());
                    return false;
                }

                for (const auto& member : inputStruct.m_members)
                {
                    RHI::ShaderSemantic streamChannelSemantic{Name{member.m_semanticText}, static_cast<uint32_t>(member.m_semanticIndex)};

                    // Semantics that represent a system-generated value do not map to an input stream
                    if (IsSystemValueSemantic(streamChannelSemantic.m_name.GetStringView()))
                    {
                        continue;
                    }

                    contract.m_streamChannels.push_back();
                    contract.m_streamChannels.back().m_semantic = streamChannelSemantic;

                    if (member.m_variable.m_typeModifier == MatrixMajor::ColumnMajor)
                    {
                        contract.m_streamChannels.back().m_componentCount = member.m_variable.m_cols;
                    }
                    else
                    {
                        contract.m_streamChannels.back().m_componentCount = member.m_variable.m_rows;
                    }

                    // [GFX_TODO][ATOM-14475]: Come up with a more elegant way to mark optional channels and their corresponding shader
                    // option
                    static const char OptionalInputStreamPrefix[] = "m_optional_";
                    if (AzFramework::StringFunc::StartsWith(member.m_variable.m_name, OptionalInputStreamPrefix, true))
                    {
                        AZStd::string expectedOptionName = AZStd::string::format(
                            "o_%s_isBound", member.m_variable.m_name.substr(strlen(OptionalInputStreamPrefix)).c_str());

                        RPI::ShaderOptionIndex shaderOptionIndex = shaderOptionGroupLayout.FindShaderOptionIndex(Name{expectedOptionName});
                        if (!shaderOptionIndex.IsValid())
                        {
                            AZ_Error(
                                ShaderBuilderUtilityName, false, "Shader option '%s' not found for optional input stream '%s'",
                                expectedOptionName.c_str(), member.m_variable.m_name.c_str());
                            return false;
                        }

                        const RPI::ShaderOptionDescriptor& option = shaderOptionGroupLayout.GetShaderOption(shaderOptionIndex);
                        if (option.GetType() != RPI::ShaderOptionType::Boolean)
                        {
                            AZ_Error(ShaderBuilderUtilityName, false, "Shader option '%s' must be a bool.", expectedOptionName.c_str());
                            return false;
                        }

                        if (option.GetDefaultValue().GetStringView() != "false")
                        {
                            AZ_Error(
                                ShaderBuilderUtilityName, false, "Shader option '%s' must default to false.",
                                expectedOptionName.c_str());
                            return false;
                        }

                        contract.m_streamChannels.back().m_isOptional = true;
                        contract.m_streamChannels.back().m_streamBoundIndicatorIndex = shaderOptionIndex;
                    }
                }

                return true;
            }

            static bool CreateShaderOutputContract(
                const AzslData& azslData,
                const AZStd::string& fragmentShaderName,
                const AZStd::string& pathToOmJson,
                RPI::ShaderOutputContract& contract)
            {
                StructData outputStruct;
                outputStruct.m_id = "";

                auto jsonOutcome = JsonSerializationUtils::ReadJsonFile(pathToOmJson, AZ::RPI::JsonUtils::DefaultMaxFileSize);
                if (!jsonOutcome.IsSuccess())
                {
                    AZ_Error(ShaderBuilderUtilityName, false, "%s", jsonOutcome.GetError().c_str());
                    return AssetBuilderSDK::ProcessJobResult_Failed;
                }

                AzslCompiler azslc(azslData.m_preprocessedFullPath);
                if (!azslc.ParseOmPopulateStructData(jsonOutcome.GetValue(), fragmentShaderName, outputStruct))
                {
                    AZ_Error(ShaderBuilderUtilityName, false, "Failed to parse output layout\n");
                    return false;
                }

                for (const auto& member : outputStruct.m_members)
                {
                    RHI::ShaderSemantic semantic = RHI::ShaderSemantic::Parse(member.m_semanticText);

                    bool depthFound = false;

                    if (semantic.m_name.GetStringView() == "SV_Target")
                    {
                        contract.m_requiredColorAttachments.push_back();
                        // Render targets only support 1-D vector types and those are always column-major (per DXC)
                        contract.m_requiredColorAttachments.back().m_componentCount = member.m_variable.m_cols;
                    }
                    else if (
                        semantic.m_name.GetStringView() == "SV_Depth" || semantic.m_name.GetStringView() == "SV_DepthGreaterEqual" ||
                        semantic.m_name.GetStringView() == "SV_DepthLessEqual")
                    {
                        if (depthFound)
                        {
                            AZ_Error(
                                ShaderBuilderUtilityName, false,
                                "SV_Depth specified more than once in the fragment shader output structure");
                            return false;
                        }
                        depthFound = true;
                    }
                    else
                    {
                        AZ_Error(
                            ShaderBuilderUtilityName, false, "Unsupported shader output semantic '%s'.", semantic.m_name.GetCStr());
                        return false;
                    }
                }

                return true;
            }

            bool CreateShaderInputAndOutputContracts(
                const AzslData& azslData,
                const MapOfStringToStageType& shaderEntryPoints,
                const RPI::ShaderOptionGroupLayout& shaderOptionGroupLayout,
                const AZStd::string& pathToOmJson,
                const AZStd::string& pathToIaJson,
                RPI::ShaderInputContract& shaderInputContract,
                RPI::ShaderOutputContract& shaderOutputContract,
                size_t& colorAttachmentCount)
            {
                bool success = true;
                for (const auto& shaderEntryPoint : shaderEntryPoints)
                {
                    auto shaderEntryName = shaderEntryPoint.first;
                    auto shaderStageType = shaderEntryPoint.second;

                    if (shaderStageType == RPI::ShaderStageType::Vertex)
                    {
                        const bool layoutCreated = CreateShaderInputContract(azslData, shaderEntryName, shaderOptionGroupLayout, pathToIaJson, shaderInputContract);
                        if (!layoutCreated)
                        {
                            success = false;
                            AZ_Error(
                                ShaderBuilderUtilityName, false, "Could not create the input contract for the vertex function %s",
                                shaderEntryName.c_str());
                            continue; // Using continue to report all the errors found
                        }
                    }

                    if (shaderStageType == RPI::ShaderStageType::Fragment)
                    {
                        const bool layoutCreated =
                            CreateShaderOutputContract(azslData, shaderEntryName, pathToOmJson, shaderOutputContract);
                        if (!layoutCreated)
                        {
                            success = false;
                            AZ_Error(
                                ShaderBuilderUtilityName, false, "Could not create the output contract for the fragment function %s",
                                shaderEntryName.c_str());
                            continue; // Using continue to report all the errors found
                        }

                        colorAttachmentCount = shaderOutputContract.m_requiredColorAttachments.size();
                    }
                }
                return success;
            }

            IncludedFilesParser::IncludedFilesParser()
            {
                AZStd::regex regex(R"(#\s*include\s+[<|"]([\w|/|\\|\.|-]+)[>|"])", AZStd::regex::ECMAScript);
                m_includeRegex.swap(regex);
            }

            AZStd::vector<AZStd::string> IncludedFilesParser::ParseStringAndGetIncludedFiles(AZStd::string_view haystack) const
            {
                AZStd::vector<AZStd::string> listOfFilePaths;
                AZStd::smatch match;
                AZStd::string::const_iterator searchStart(haystack.cbegin());
                while (AZStd::regex_search(searchStart, haystack.cend(), match, m_includeRegex))
                {
                    if (match.size() > 1)
                    {
                        AZStd::string relativeFilePath(match[1].str().c_str());
                        AzFramework::StringFunc::Path::Normalize(relativeFilePath);
                        listOfFilePaths.push_back(relativeFilePath);
                    }
                    searchStart = match.suffix().first;
                }
                return listOfFilePaths;
            }

            AZ::Outcome<AZStd::vector<AZStd::string>, AZStd::string> IncludedFilesParser::ParseFileAndGetIncludedFiles(AZStd::string_view sourceFilePath) const
            {
                AZ::IO::FileIOStream stream(sourceFilePath.data(), AZ::IO::OpenMode::ModeRead);
                if (!stream.IsOpen())
                {
                    return AZ::Failure(AZStd::string::format("\"%s\" source file could not be opened.", sourceFilePath.data()));
                }

                if (!stream.CanRead())
                {
                    return AZ::Failure(AZStd::string::format("\"%s\" source file could not be read.", sourceFilePath.data()));
                }

                AZStd::string hayStack;
                hayStack.resize_no_construct(stream.GetLength());
                stream.Read(stream.GetLength(), hayStack.data());

                auto listOfFilePaths = ParseStringAndGetIncludedFiles(hayStack);
                return AZ::Success(AZStd::move(listOfFilePaths));
            }

        }  // namespace ShaderBuilderUtility
    } // namespace ShaderBuilder
} // AZ
