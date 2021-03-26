/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <CommonFiles/GlobalBuildOptions.h>

#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/API/ApplicationAPI.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Debug/TraceContext.h>

#include <AtomCore/Serialization/Json/JsonUtils.h>

namespace AZ
{
    namespace ShaderBuilder
    {
        void GlobalBuildOptions::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<GlobalBuildOptions>()
                    ->Version(1)
                    ->Field("PreprocessorOptions", &GlobalBuildOptions::m_preprocessorSettings)
                    ->Field("ShaderCompilerArguments", &GlobalBuildOptions::m_compilerArguments);
            }
        }

        namespace
        {
            // this function wraps BroadcastResult on GetSourceInfoBySourcePath
            // return value: whether the search query succeeded or not
            bool MutateToAbsolutePathIfFound(AZStd::string& relativePathToSearch)
            {
                bool found = false;
                if (AzFramework::StringFunc::Path::IsRelative(relativePathToSearch.c_str()))
                {
                    AZ::Data::AssetInfo sourceInfo;
                    AZStd::string watchFolder;
                    AzToolsFramework::AssetSystemRequestBus::BroadcastResult(found,
                                                                             &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath,
                                                                             relativePathToSearch.c_str(),
                                                                             sourceInfo,
                                                                             watchFolder);
                    if (found)
                    {
                        AZStd::string absoluteOutput;
                        AzFramework::StringFunc::Path::Join(watchFolder.c_str(), relativePathToSearch.c_str(), absoluteOutput);
                        relativePathToSearch = std::move(absoluteOutput);
                    }
                }
                return found;
            }
        }

        GlobalBuildOptions ReadBuildOptions(const char* builderName)
        {
            GlobalBuildOptions output;
            // try to parse some config file for eventual additional options
            AZStd::string globalBuildOption = "Config/shader_global_build_options.json";
            bool found = MutateToAbsolutePathIfFound(globalBuildOption);
            if (found)
            {
                // load settings in a temporary object to be able to do some input treatment, before merging with the final options
                Outcome loadResult = AZ::JsonSerializationUtils::LoadObjectFromFile(output, globalBuildOption);
                AZ_Warning(builderName, loadResult.IsSuccess(), "Failed to load shader-build environment include paths settings from file [%s]", globalBuildOption.c_str());
                AZ_Warning(builderName, loadResult.IsSuccess(), "  Details: %s", loadResult.GetError().c_str());
            }
            else
            {
                AZ_TracePrintf(builderName, "config file [%s] not found.", globalBuildOption.c_str());
            }
            InitializePreprocessorOptions(output.m_preprocessorSettings, builderName);
            return output;
        }
    }
}
