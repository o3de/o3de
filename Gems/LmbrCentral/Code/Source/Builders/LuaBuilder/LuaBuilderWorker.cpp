/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Math/MathReflection.h>
#include <AzCore/Script/ScriptAsset.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Script/lua/lua.h> // for lua_tostring
#include <AzCore/std/string/conversions.h>
#include <AzFramework/FileFunc/FileFunc.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <Builders/LuaBuilder/LuaHelpers.h>

#include <Builders/LuaBuilder/LuaBuilderWorker.h>

 // Ensures condition is true, otherwise returns (and presumably fails the build job).
#define LB_VERIFY(condition, ...)\
        if (!(condition))\
        {\
            AZ_Error(AssetBuilderSDK::ErrorWindow, false, __VA_ARGS__);\
            return;\
        }

namespace LuaBuilder
{
    namespace
    {
        AZStd::vector<AZ::Data::Asset<AZ::ScriptAsset>> ConvertToAssets(AssetBuilderSDK::ProductPathDependencySet& dependencySet)
        {
            AZStd::vector<AZ::Data::Asset<AZ::ScriptAsset>> assets;

            if (AzToolsFramework::AssetSystemRequestBus::Events* assetSystem = AzToolsFramework::AssetSystemRequestBus::FindFirstHandler())
            {
                for (auto dependency : dependencySet)
                {
                    AZStd::string watchFolder;
                    AZ::Data::AssetInfo assetInfo;
                    AZ::IO::Path path(dependency.m_dependencyPath);
                    bool isLuaDependency = !path.HasExtension() || path.Extension() == ".lua" || path.Extension() == ".luac";
                    auto sourcePath = path.ReplaceExtension(".lua");

                    if (assetSystem->GetSourceInfoBySourcePath(sourcePath.c_str(), assetInfo, watchFolder)
                        && assetInfo.m_assetId.IsValid())
                    {
                        AZ::Data::Asset<AZ::ScriptAsset> asset(AZ::Data::AssetId
                            ( assetInfo.m_assetId.m_guid
                            , AZ::ScriptAsset::CompiledAssetSubId)
                            , azrtti_typeid<AZ::ScriptAsset>());
                        asset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::PreLoad);
                        assets.push_back(asset);
                    }
                    else if(isLuaDependency)
                    {
                        AZ_Error("LuaBuilder", false, "Did not find dependency %s referenced by script.", dependency.m_dependencyPath.c_str());
                    }
                    else
                    {
                        AZ_Error("LuaBuilder", false, "%s referenced by script does not appear to be a lua file.\n"
                            "References to assets should be handled using Property slots and AssetIds to ensure proper dependency tracking.\n"
                            "This file will not be tracked as a dependency.",
                            dependency.m_dependencyPath.c_str());
                    }
                }
            }
            else
            {
                AZ_Error("LuaBuilder", false, "AssetSystemBus not available");
            }

            return assets;
        }

        //////////////////////////////////////////////////////////////////////////
        // Helper for writing to a generic stream
        template<typename T>
        bool WriteToStream(AZ::IO::GenericStream& stream, const T* t)
        {
            return stream.Write(sizeof(T), t) == sizeof(T);
        }

        static const AZ::u32 s_BuildTypeKey = AZ_CRC_CE("BuildType");
        static const char* s_BuildTypeCompiled = "Compiled";
    }

    AZStd::string LuaBuilderWorker::GetAnalysisFingerprint()
    {
        // mutating the Analysis Fingerprint will cause the CreateJobs function to run even
        // on files which have not changed.
        return AZ::ScriptDataContext::GetInterpreterVersion();
    }
    //////////////////////////////////////////////////////////////////////////
    // CreateJobs
    void LuaBuilderWorker::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
    {
        using namespace AssetBuilderSDK;

        // Check for shutdown
        if (m_isShuttingDown)
        {
            response.m_result = CreateJobsResultCode::ShuttingDown;
            return;
        }

        AssetBuilderSDK::ProductPathDependencySet dependencySet;
        AZ::IO::Path path = request.m_watchFolder;
        path = path / request.m_sourceFile;

        ParseDependencies(path.c_str(), dependencySet);
        auto dependentAssets = ConvertToAssets(dependencySet);




        for (const AssetBuilderSDK::PlatformInfo& info : request.m_enabledPlatforms)
        {
            JobDescriptor descriptor;
            descriptor.m_jobKey = "Lua Compile";
            descriptor.SetPlatformIdentifier(info.m_identifier.c_str());
            descriptor.m_critical = true;
            // mutating the AdditionalFingerprintInfo will cause the job to run even if
            // nothing else has changed (i.e., files are the same, version of this builder didn't change)
            // by doing this, changing the version of the interpreter is enough to cause the files to rebuild
            // automatically.
            descriptor.m_additionalFingerprintInfo = GetAnalysisFingerprint();
            descriptor.m_jobParameters[s_BuildTypeKey] = s_BuildTypeCompiled;

            for (auto& dependentAsset : dependentAssets)
            {
                AssetBuilderSDK::JobDependency jobDependency;
                jobDependency.m_sourceFile.m_sourceFileDependencyUUID = dependentAsset.GetId().m_guid;
                jobDependency.m_jobKey = "Lua Compile";
                jobDependency.m_platformIdentifier = info.m_identifier;
                jobDependency.m_type = AssetBuilderSDK::JobDependencyType::Order;
                descriptor.m_jobDependencyList.emplace_back(AZStd::move(jobDependency));
            }

            response.m_createJobOutputs.push_back(descriptor);
        }

        response.m_result = CreateJobsResultCode::Success;
    }

    //////////////////////////////////////////////////////////////////////////
    // ProcessJob
    void LuaBuilderWorker::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
    {
        using namespace AZ::IO;

        AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "Starting Job.\n");
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;

        LB_VERIFY(!m_isShuttingDown, "Cancelled job %s because shutdown was requested.\n", request.m_sourceFile.c_str());
        LB_VERIFY(request.m_jobDescription.m_jobParameters.at(s_BuildTypeKey) == s_BuildTypeCompiled
            , "Cancelled job %s because job key was invalid.\n", request.m_sourceFile.c_str());

        AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "Starting script compile.\n");
        // setup lua state
        AZ::ScriptContext scriptContext(AZ::DefaultScriptContextId);
        // reset filename to .luac, reconstruct full path
        AZStd::string destFileName;
        AzFramework::StringFunc::Path::GetFullFileName(request.m_fullPath.c_str(), destFileName);
        AzFramework::StringFunc::Path::ReplaceExtension(destFileName, "luac");

        AZStd::string debugName = "@" + request.m_sourceFile;
        AZStd::to_lower(debugName.begin(), debugName.end());

        {
            // read script
            FileIOStream inputStream;
            LB_VERIFY(inputStream.Open(request.m_fullPath.c_str(), OpenMode::ModeRead | OpenMode::ModeText)
                , "Failed to open input file %s", request.m_sourceFile.c_str());

            // parse script
            LB_VERIFY(scriptContext.LoadFromStream(&inputStream, debugName.c_str(), "t")
                , "%s"
                , lua_tostring(scriptContext.NativeContext(), -1));

            inputStream.Seek(0, AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);
        }

        // initialize asset data
        AZ::LuaScriptData assetData;
        assetData.m_debugName = debugName;
        AssetBuilderSDK::ProductPathDependencySet dependencySet;
        ParseDependencies(request.m_fullPath, dependencySet);
        assetData.m_dependencies = ConvertToAssets(dependencySet);
        auto scriptStream = assetData.CreateScriptWriteStream();
        LB_VERIFY(LuaDumpToStream(scriptStream, scriptContext.NativeContext()), "Failed to write lua bytecode to stream.");

        {
            // write asset data to disk
            AZStd::string destPath;
            AzFramework::StringFunc::Path::ConstructFull(request.m_tempDirPath.c_str(), destFileName.data(), destPath, true);

            FileIOStream outputStream;
            LB_VERIFY(outputStream.Open(destPath.c_str(), OpenMode::ModeWrite | OpenMode::ModeBinary)
                , "Failed to open output file %s", destPath.data());

            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
            LB_VERIFY(serializeContext, "Unable to retrieve serialize context.");

            LB_VERIFY
                ( AZ::Utils::SaveObjectToStream<AZ::LuaScriptData>(outputStream, AZ::ObjectStream::ST_BINARY, &assetData, serializeContext)
                , "Failed to write asset data to disk");
        }

        AssetBuilderSDK::JobProduct compileProduct{ destFileName, azrtti_typeid<AZ::ScriptAsset>(), AZ::ScriptAsset::CompiledAssetSubId };

        for (auto& dependency : assetData.m_dependencies)
        {
            compileProduct.m_dependencies.push_back({ dependency.GetId(), AZ::Data::ProductDependencyInfo::CreateFlags(AZ::Data::AssetLoadBehavior::PreLoad) });
        }

        // report success
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
        response.m_outputProducts.emplace_back(compileProduct);
        response.m_outputProducts.back().m_dependenciesHandled = true;
        AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "Finished job.\n");
    }

    //////////////////////////////////////////////////////////////////////////
    // ShutDown
    void LuaBuilderWorker::ShutDown()
    {
        // it is important to note that this will be called on a different thread than your process job thread
        m_isShuttingDown = true;
    }

    void LuaBuilderWorker::ParseDependencies(const AZStd::string& file, AssetBuilderSDK::ProductPathDependencySet& outDependencies)
    {
        bool isInsideBlockComment = false;
        AzFramework::FileFunc::ReadTextFileByLine(file, [&outDependencies, &isInsideBlockComment](const char* line) -> bool
        {
            AZStd::string lineCopy(line);

            // Block comments can be negated by adding an extra '-' to the front of the comment marker
            // We should strip these out of every line, as a negated block comment should be parsed like regular code
            AzFramework::StringFunc::Replace(lineCopy, "---[[", "");

            // Splitting the line into tokens with "--" will give us the following behavior:
            //   case 1: "code to parse -- commented out line" -> {"code to parse "," commented out line"}
            //   case 2: "code to parse --[[ contents of block comment --]] more code to parse"
            //               -> {"code to parse ","[[ contents of block comment ","]] more code to parse"}
            AZStd::vector<AZStd::string> tokens;
            AzFramework::StringFunc::Tokenize(lineCopy, tokens, "--", true, true);

            if (isInsideBlockComment)
            {
                // If the block comment ends this line, we'll handle that later
                lineCopy.clear();
            }
            else if (!tokens.empty())
            {
                // Unless inside a block comment, all characters to the left of "--" should be parsed
                lineCopy = tokens[0];
            }

            for (int tokenIndex = 1; tokenIndex < tokens.size(); ++tokenIndex)
            {
                if (AzFramework::StringFunc::StartsWith(tokens[tokenIndex].c_str(), "[["))
                {
                    // "--[[" indicates the start of a block comment. Ignore contents of this token.
                    isInsideBlockComment = true;
                    continue;
                }
                else if (AzFramework::StringFunc::StartsWith(tokens[tokenIndex].c_str(), "]]"))
                {
                    // "--]]" indicates the end of a block comment. Parse contents of this token.
                    isInsideBlockComment = false;
                    AzFramework::StringFunc::LChop(tokens[tokenIndex], 2);
                    lineCopy.append(tokens[tokenIndex]);
                }
                else if (!tokens[tokenIndex].empty())
                {
                    // "--" (with no special characters after) indicates a whole line comment. Ignore all further tokens.
                    break;
                }
            }

            // Regex to match lines looking similar to require("a") or Script.ReloadScript("a") or require "a"
            // Group 1: require or empty
            // Group 2: quotation mark ("), apostrophe ('), or empty
            // Group 3: specified path or variable (variable will be indicated by empty group 2)
            // Group 4: Same as group 2
            AZStd::regex requireRegex(R"(\b(?:(require)|Script\.ReloadScript)\s*(?:\(|(?="|'))\s*("|'|)([^"')]*)(\2)\s*\)?)");
            // Regex to match lines looking like a path (containing a /)
            // Group 1: the string contents
            AZStd::regex pathRegex(R"~("((?=[^"]*\/)[^"\n<>:"|?*]{2,})")~");

            // Regex to match lines looking like ExecuteConsoleCommand("exec somefile.cfg")
            AZStd::regex consoleCommandRegex(R"~(ExecuteConsoleCommand\("exec (.*)"\))~");

            AZStd::smatch match;
            if (AZStd::regex_search(lineCopy, match, requireRegex))
            {
                if (!match[2].matched || !match[4].matched)
                {
                    // Result is not a string literal, we'll have to rely on the path regex to pick up the dependency
                }
                else
                {
                    AZStd::string filePath = match[3].str();

                    if (match[1].matched)
                    {
                        // This is a "require" include, which has a format that uses . instead of / and has no file extension included
                        static constexpr char s_luaExtension[] = ".luac";

                        // Replace '.' in module name with '/'
                        for (auto pos = filePath.find('.'); pos != AZStd::string::npos; pos = filePath.find('.'))
                        {
                            filePath.replace(pos, 1, "/", 1);
                        }

                        // Add file extension to path
                        if (filePath.find(s_luaExtension) == AZStd::string::npos)
                        {
                            filePath += s_luaExtension;
                        }
                    }

                    outDependencies.emplace(filePath, AssetBuilderSDK::ProductPathDependencyType::ProductFile);
                }
            }
            else if (AZStd::regex_search(lineCopy, match, consoleCommandRegex))
            {
                outDependencies.emplace(match[1].str().c_str(), AssetBuilderSDK::ProductPathDependencyType::ProductFile);
            }
            else if (AZStd::regex_search(lineCopy, match, pathRegex))
            {
                AZ_TracePrintf("LuaBuilder", "Found potential dependency on file: %s\n", match[1].str().c_str());

                outDependencies.emplace(match[1].str().c_str(), AssetBuilderSDK::ProductPathDependencyType::ProductFile);
            }

            return true;
        });
    }



#undef LB_VERIFY
}
