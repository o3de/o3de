/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AzslBuilder.h"
#include "ShaderBuilderUtility.h"
#include "ShaderPlatformInterfaceRequest.h"
#include <CommonFiles/GlobalBuildOptions.h>

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <AzCore/IO/SystemFile.h>

#include <math.h>

namespace AZ
{
    namespace ShaderBuilder
    {
        enum class JobParameterIndices : uint32_t
        {
            ApiName,
            PreprocessedCode,
            PreprocessorError,
            SkipJob
        };

        AZ::Uuid AzslBuilder::GetUUID()
        {
            return AZ::Uuid::CreateString("{72DCFC95-1B9E-4A8D-8633-D497CACD98AB}");
        }

        RHI::ShaderPlatformInterface* GetShaderPlatformInterfaceForApi(const AZStd::string& apiNameFilter, const AssetBuilderSDK::PlatformInfo& currentPlatform)
        {
            AZStd::vector<RHI::ShaderPlatformInterface*> platformInterfaces;
            ShaderPlatformInterfaceRequestBus::BroadcastResult(platformInterfaces, &ShaderPlatformInterfaceRequest::GetShaderPlatformInterface, currentPlatform);
            for (RHI::ShaderPlatformInterface* oneInterface : platformInterfaces)
            {
                if (oneInterface && oneInterface->GetAPIName().GetStringView() == apiNameFilter)
                {
                    return oneInterface;
                }
            }
            return nullptr;
        }

        PreprocessorData PreprocessSource(const AZStd::string& inputFile, const AZStd::string& originalPath, const PreprocessorOptions& options)
        {
            // run mcpp
            PreprocessorData output;
            PreprocessFile(inputFile, output, options, true, true);
            // do not let the filename.api.azsl.prepend be the filename that will be regarded as the source.
            // because SRG assets are located using the 'containingFile' so we need to preserve the true origin:
            MutateLineDirectivesFileOrigin(output.code, originalPath);
            RHI::ReportErrorMessages(AzslBuilder::BuilderName, output.diagnostics);
            return output;
        }

        void AddAzslBuilderJobDependency(AssetBuilderSDK::JobDescriptor& jobDescriptor, const AZStd::string& platformInfoIdentifier, AZStd::string_view apiName, AZStd::string_view fullFilePath)
        {
            AssetBuilderSDK::SourceFileDependency fileDependency;
            fileDependency.m_sourceFileDependencyPath = fullFilePath;

            AssetBuilderSDK::JobDependency dependency;
            dependency.m_jobKey = AzslBuilder::JobKey;
            dependency.m_jobKey += " ";
            dependency.m_jobKey += apiName;
            dependency.m_platformIdentifier = platformInfoIdentifier;
            dependency.m_sourceFile = fileDependency;
            dependency.m_type = AssetBuilderSDK::JobDependencyType::Order;
            jobDescriptor.m_jobDependencyList.emplace_back(dependency);
        }

        void AzslBuilder::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const
        {
            AZStd::string fullPath;
            AzFramework::StringFunc::Path::ConstructFull(request.m_watchFolder.data(), request.m_sourceFile.data(), fullPath, true);

            // this builder may take as input:
            // .shader .azsl .azsli .srgi
            // it will not behave strictly exactly the same for each type

            // Only *.srgi files are supposed to include files that define "partial" qualified SRGs. 
            const bool isSrgi = AzFramework::StringFunc::Path::IsExtension(fullPath.c_str(), SrgIncludeExtension);

            // .azsli needs "skip check"
            const bool isAzsli = AzFramework::StringFunc::Path::IsExtension(fullPath.c_str(), "azsli");

            // .shader files must be opened to get their build options and the referenced azsl file
            const bool isShader = AzFramework::StringFunc::Path::IsExtension(fullPath.c_str(), RPI::ShaderSourceData::Extension);

            // .azsl must not be skipped, otherwise we're creating a risk of two .shader referring to the same .azsl racing for its output product

            // To avoid the warning:
            //    "No job was found to match the job dependency criteria declared by file "..."
            // We will schedule the job, but will do nothing 
            //bool shouldSkipFile = false;

            // We treat some issues as warnings and return "Success" from CreateJobs allows us to report the dependency.
            // If/when a valid dependency file appears, that will trigger the ShaderVariantAssetBuilder to run again.
            // Since CreateJobs will pass, we forward this message to ProcessJob which will report it as an error.
            //bool gotPreprocessingError = false;


            // The following if-block will be removed once [GFX TODO][ATOM-5302] is addressed, and
            // azslc allows redundant SrgSemantics for "partial" qualified SRGs.
            if (isAzsli)
            {
                auto skipCheck = ShaderBuilderUtility::ShouldSkipFileForSrgProcessing(BuilderName, fullPath);
                if (skipCheck != ShaderBuilderUtility::SrgSkipFileResult::ContinueProcess)
                {
                    response.m_result = skipCheck == ShaderBuilderUtility::SrgSkipFileResult::Error ?
                        AssetBuilderSDK::CreateJobsResultCode::Failed : AssetBuilderSDK::CreateJobsResultCode::Success;
                    return;
                }
            }

            if (isShader)
            {
                // Need to get the path to the shader file from the template, so that we can preprocess the shader data and setup
                // source file dependencies.
                auto descriptorParseOutput = ShaderBuilderUtility::LoadShaderDataJson(fullPath);
                if (!descriptorParseOutput.IsSuccess())
                {
                    AZ_Error(BuilderName, false, "Failed to parse Shader Descriptor JSON: %s", descriptorParseOutput.GetError().c_str());
                    return;
                }
                // update the value of fullPath to mean directly, the azsl file:
                ShaderBuilderUtility::GetAbsolutePathToAzslFile(fullPath, descriptorParseOutput.GetValue().m_source, fullPath);
            }

            GlobalBuildOptions buildOptions = ReadBuildOptions(BuilderName);

            for (const AssetBuilderSDK::PlatformInfo& info : request.m_enabledPlatforms)
            {
                AZ_TraceContext("For platform", info.m_identifier.data());

                // Get the platform interfaces to be able to access the prepend file
                AZStd::vector<RHI::ShaderPlatformInterface*> platformInterfaces = ShaderBuilderUtility::DiscoverValidShaderPlatformInterfaces(info);

                // Preprocess the shader file, per activated platform.
                for (RHI::ShaderPlatformInterface* shaderPlatformInterface : platformInterfaces)
                {
                    auto apiNameAsStringView = shaderPlatformInterface->GetAPIName().GetStringView();

                    AssetBuilderSDK::JobDescriptor jobDescriptor;
                    jobDescriptor.m_priority = 2;
                    // [GFX TODO][ATOM-2830] Set 'm_critical' back to 'false' once proper fix for Atom startup issues are in 
                    jobDescriptor.m_critical = true;
                    jobDescriptor.m_jobKey = JobKey;
                    jobDescriptor.m_jobKey += " ";
                    jobDescriptor.m_jobKey += apiNameAsStringView;
                    jobDescriptor.SetPlatformIdentifier(info.m_identifier.data());
                    jobDescriptor.m_jobParameters[(u32)JobParameterIndices::ApiName] = apiNameAsStringView;
                    if (isShader)
                    {
                        // add a job dependency on the azsl run (of that same job: AzslBuilder - because it also runs on .azsl)
                        AddAzslBuilderJobDependency(jobDescriptor, info.m_identifier, apiNameAsStringView, fullPath);
                    }

                    // execute azsl prepending here, before preprocess, in order to support macros in AzslcHeader.azsli
                    AZStd::string prependedAzslSourceCode;
                    RHI::PrependArguments args;
                    args.m_sourceFile = fullPath.c_str();
                    args.m_prependFile = shaderPlatformInterface->GetAzslHeader(info);
                    args.m_addSuffixToFileName = shaderPlatformInterface->GetAPIName().GetCStr();
                    args.m_destinationStringOpt = &prependedAzslSourceCode;

                    if (RHI::PrependFile(args) == fullPath)  // error case. it returns the combined-file's name on success, or original path on failure, but here we use the "direct to string" mode so we don't need to store the returned name.
                    {
                        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Failed;
                        return;
                    }
                    AZStd::string originalLocation;
                    // extract the (full) directory chain from the path: (eg from "d:/p/f.e" extract "d:/p/")
                    AzFramework::StringFunc::Path::GetFullPath(fullPath.c_str(), originalLocation);
                    // have to go through filesystem because we have no way to pipe data through mcpp (because of single threaded static link call and buffer limits)
                    // we can't use a temporary folder because CreateJobs API does not warrant side effects, and does not prepare a temp folder.
                    // we can't use the OS temp folder anyway, because many includes (eg #include "../RPI/Shadow.h") are relative and will only work from the original location
                    AZStd::string prependedPath = ShaderBuilderUtility::DumpAzslPrependedCode(
                        BuilderName, prependedAzslSourceCode, originalLocation, ShaderBuilderUtility::ExtractStemName(fullPath.c_str()),
                        shaderPlatformInterface->GetAPIName().GetStringView());
                    // run mcpp
                    PreprocessorData preprocessorData = PreprocessSource(prependedPath, fullPath, buildOptions.m_preprocessorSettings);
                    jobDescriptor.m_jobParameters[(u32)JobParameterIndices::PreprocessorError] = preprocessorData.diagnostics;  // save for ProcessJob
                    jobDescriptor.m_jobParameters[(u32)JobParameterIndices::PreprocessedCode] = preprocessorData.code;  // save for ProcessJob
                    AZ::IO::SystemFile::Delete(prependedPath.c_str());  // don't let that intermediate file dirty a folder under source version control.

                    for (AZStd::string includePath : preprocessorData.includedPaths)
                    {
                        // m_sourceFileDependencyList does not support paths with "." or ".." for relative lookup, but the preprocessor
                        // may produce path strings like "C:/a/b/c/../../d/file.azsli" so we have to normalize
                        AzFramework::StringFunc::Path::Normalize(includePath);

                        AssetBuilderSDK::SourceFileDependency includeFileDependency;
                        includeFileDependency.m_sourceFileDependencyPath = includePath;
                        response.m_sourceFileDependencyList.emplace_back(includeFileDependency);
                    }
                    response.m_createJobOutputs.push_back(jobDescriptor);
                }  // all RHI platforms
            }  // for all request.m_enabledPlatforms
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
        }

        enum class ErrorStrategy { HardFailIfAbsent, ReturnEmptyIfAbsent };
        static AZStd::string GetJobParameterValue(const AssetBuilderSDK::ProcessJobRequest& request, JobParameterIndices index, [[maybe_unused]] ErrorStrategy failBehavior)
        {
            auto iterator = request.m_jobDescription.m_jobParameters.find(static_cast<u32>(index));
            if (iterator == request.m_jobDescription.m_jobParameters.end())
            {
                AZ_Error(AzslBuilder::BuilderName, failBehavior != ErrorStrategy::HardFailIfAbsent, "Saved data is missing in job parameters. for index [%d]", index);
                return {};
            }
            return iterator->second;
        }

        // eg: ("D:/p/x.a", "D:/p/x.b") -> yes
        static bool HasSameFileName(const AZStd::string& lhsPath, const AZStd::string& rhsPath)
        {
            using namespace StringFunc::Path;
            AZStd::string stem1;
            GetFileName(lhsPath.c_str(), stem1);
            AZStd::string stem2;
            GetFileName(rhsPath.c_str(), stem2);
            return stem1 == stem2;
        }

        void AzslBuilder::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const
        {
            if (request.m_jobDescription.m_jobParameters.find((u32)JobParameterIndices::SkipJob) != request.m_jobDescription.m_jobParameters.end())
            {
                AZ_TracePrintf(BuilderName, "Early out because this file was determined to not need an independent build\n");
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
                return;
            }
            // report the deffered diagnostics:
            const AZStd::string& preprocessorErrors = GetJobParameterValue(request, JobParameterIndices::PreprocessorError, ErrorStrategy::ReturnEmptyIfAbsent);
            if (!preprocessorErrors.empty())
            {
                bool foundErrors = RHI::ReportErrorMessages(BuilderName, preprocessorErrors.c_str());
                if (foundErrors)
                {
                    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                    return;
                }
            }

            const AZStd::sys_time_t startTime = AZStd::GetTimeNowTicks();
            AZStd::string fullSourcePath;
            AzFramework::StringFunc::Path::ConstructFull(request.m_watchFolder.c_str(), request.m_sourceFile.c_str(), fullSourcePath, true);
            // extract "name" from "P:/F/name.x"
            AZStd::string sourceStemName;
            AzFramework::StringFunc::Path::GetFileName(fullSourcePath.c_str(), sourceStemName);

            GlobalBuildOptions buildOptions = ReadBuildOptions(BuilderName);

            // get the shader platform interface that matches this job's API
            const AZStd::string& apiName = GetJobParameterValue(request, JobParameterIndices::ApiName, ErrorStrategy::HardFailIfAbsent);
            const AZStd::string& preprocessedCode = GetJobParameterValue(request, JobParameterIndices::PreprocessedCode, ErrorStrategy::ReturnEmptyIfAbsent);
            RHI::ShaderPlatformInterface* platformInterface = GetShaderPlatformInterfaceForApi(apiName, request.m_platformInfo);
            if (!platformInterface)
            {
                AZ_Error(BuilderName, false, "Could not retreive Shader Platform Interface");
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                return;
            }

            const bool isSrgi   = AzFramework::StringFunc::Path::IsExtension(fullSourcePath.c_str(), SrgIncludeExtension);
            const bool isAzsli  = AzFramework::StringFunc::Path::IsExtension(fullSourcePath.c_str(), "azsli");
            const bool isShader = AzFramework::StringFunc::Path::IsExtension(fullSourcePath.c_str(), RPI::ShaderSourceData::Extension);
            if (isShader)
            {
                // read .shader -> access azsl path -> make absolute
                RPI::ShaderSourceData shaderAssetSource;
                AZStd::shared_ptr<ShaderFiles> inputFiles = ShaderBuilderUtility::PrepareSourceInput(BuilderName, fullSourcePath, shaderAssetSource);
                if (!inputFiles)
                {
                    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                    return;
                }

                if (shaderAssetSource.IsRhiBackendDisabled(platformInterface->GetAPIName()))
                {
                    // Gracefully do nothing and return success.
                    AZ_TracePrintf(
                        BuilderName, "Skipping shader compilation [%s] for API [%s]\n", fullSourcePath.c_str(),
                        platformInterface->GetAPIName().GetCStr());
                    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
                    return;
                }


                // Save .shader file name
                AzFramework::StringFunc::Path::GetFileName(request.m_sourceFile.data(), inputFiles->m_shaderFileName);

                // Verify the presence of potential differences between the global options, and local options:
                bool mustRebuild = buildOptions.m_compilerArguments.HasDifferentAzslcArguments(shaderAssetSource.m_compiler);
                
                // Merge compiler options coming from 2 source: global options (from project Config/), and .shader options.
                // We define a merge behavior that is: ".shader wins if set" (local overrides global)
                buildOptions.m_compilerArguments.Merge(shaderAssetSource.m_compiler);

                // Earlier, we declared a job dependency on the .azsl's job, let's access the produced assets:
                uint32_t subId = ShaderBuilderUtility::MakeAzslBuildProductSubId(
                    RPI::ShaderAssetSubId::GeneratedHlslSource, platformInterface->GetAPIType());
                auto assetIdOutcome = RPI::AssetUtils::MakeAssetId(inputFiles->m_azslSourceFullPath, subId);
                AZ_Warning(BuilderName, assetIdOutcome.IsSuccess(), "Product of dependency %s not found: this is an oddity but build can continue.", inputFiles->m_azslSourceFullPath.c_str());
                if (assetIdOutcome.IsSuccess())
                {
                    // The .azsl build job didn't know about the build options listed in the .shader
                    // so it produced "generic" artifacts xxx.ia.json, xxx.hlsl, etc.
                    if (!mustRebuild)
                    {
                        // They are in fact sufficient. nothing more to do
                        AZ_TracePrintf(BuilderName, "Product output already built by %s. exiting.", inputFiles->m_azslSourceFullPath.c_str());
                        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
                        return;
                    }
                    // Otherwise, let's go again, but we need to modify the output's name to avoid product conflicts.*
                    AZ_TracePrintf(BuilderName, "Product output already built by %s is not reusable because of incompatible azslc CompilerHints: launching independent build", inputFiles->m_azslSourceFullPath.c_str());
                }

                if (HasSameFileName(fullSourcePath, inputFiles->m_azslSourceFullPath))
                {
                    // let's add a "distinguisher" to the names of the outproduct artifacts of this build round.*
                    // Because otherwise the asset processor is not going to accept an overwrite of the ones output by the .azsl job
                    static constexpr char RebuildSuffix[] = ".shader-w-diff-azslc-opts";
                    sourceStemName += RebuildSuffix;
                }
            }

            AZStd::string preprocessedPath = ShaderBuilderUtility::DumpPreprocessedCode(
                BuilderName,
                preprocessedCode,
                request.m_tempDirPath,
                sourceStemName,
                apiName);

            AZ_TraceContext("Platform API", apiName);

            AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);
            if (jobCancelListener.IsCancelled())
            {
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
                return;
            }

            // compiler setup
            ShaderBuilder::AzslCompiler azslc(preprocessedPath);
            AZStd::string compilerParameters = platformInterface->GetAzslCompilerParameters(buildOptions.m_compilerArguments);
            compilerParameters += " ";
            compilerParameters += platformInterface->GetAzslCompilerWarningParameters(buildOptions.m_compilerArguments);
            AtomShaderConfig::AddParametersFromConfigFile(compilerParameters, request.m_platformInfo);
            if (isSrgi || isAzsli)
            {
                // When compiling srgi or azsli files, the SRGs may appear as unused. It is necessary
                // to remove the flag --strip-unused-srgs in case it is present in the compiler parameters.
                AzFramework::StringFunc::Replace(compilerParameters, " --strip-unused-srgs", "");
            }

            AZStd::string outputName = AZStd::string::format("%s.%s.hlsl", sourceStemName.c_str(), apiName.c_str());
            AzFramework::StringFunc::Path::Join(request.m_tempDirPath.c_str(), outputName.c_str(), outputName, true);

            auto emitFullOutcome = azslc.EmitFullData(compilerParameters, outputName);
            if (!emitFullOutcome.IsSuccess())
            {
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                return;
            }

            for (int i = 0; i < emitFullOutcome.GetValue().size(); ++i)
            {
                AssetBuilderSDK::JobProduct jobProduct;
                jobProduct.m_productFileName = emitFullOutcome.GetValue()[i];
                static const AZ::Uuid AzslOutcomeType = "{6977AEB1-17AD-4992-957B-23BB2E85B18B}";
                jobProduct.m_productAssetType = AzslOutcomeType;
                jobProduct.m_productSubID = ShaderBuilderUtility::MakeAzslBuildProductSubId(ShaderBuilderUtility::AzslSubProducts::SubList[i], platformInterface->GetAPIType());
                jobProduct.m_dependenciesHandled = true;
                // Note that the output products are not traditional product assets that will be used by the game project.
                // They are artifacts that are produced once, cached, and used later by other AssetBuilders as a way to centralize build organization.
                response.m_outputProducts.push_back(AZStd::move(jobProduct));
            }

            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;

            const AZStd::sys_time_t endTime = AZStd::GetTimeNowTicks();
            const AZStd::sys_time_t deltaTime = endTime - startTime;
            const float elapsedTimeSeconds = (float)(deltaTime) / (float)AZStd::GetTimeTicksPerSecond();

            AZ_TracePrintf(BuilderName, "Finished compiling %s in %.2f seconds\n", request.m_sourceFile.c_str(), elapsedTimeSeconds);

            ShaderBuilderUtility::LogProfilingData(BuilderName, sourceStemName);
        }  // end ProcessJob
    } // ShaderBuilder
} // AZ
