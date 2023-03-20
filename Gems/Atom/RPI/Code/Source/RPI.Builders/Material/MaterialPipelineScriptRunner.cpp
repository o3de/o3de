/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MaterialPipelineScriptRunner.h"
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Reflect/Material/LuaScriptUtilities.h>
#include <AzCore/Script/ScriptAsset.h>
#include <AzCore/Utils/Utils.h>

namespace AZ
{
    namespace RPI
    {
        /*static*/ void MaterialPipelineScriptRunner::ScriptExecutionContext::Reflect(ReflectContext* reflect)
        {
            if (auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflect))
            {
                behaviorContext->Class<ScriptExecutionContext>()
                    ->Method("GetLightingModelName", &ScriptExecutionContext::GetLightingModelName)
                    ->Method("IncludeAllShaders", &ScriptExecutionContext::IncludeAllShaders)
                    ->Method("ExcludeAllShaders", &ScriptExecutionContext::ExcludeAllShaders)
                    ->Method("IncludeShader", &ScriptExecutionContext::IncludeShader)
                    ->Method("ExcludeShader", &ScriptExecutionContext::ExcludeShader)
                    ;
            }
        }

        MaterialPipelineScriptRunner::ScriptExecutionContext::ScriptExecutionContext(const MaterialTypeSourceData& materialType, const ShaderTemplatesList& availableShaderTemplates)
            : m_materialType(materialType)
        {
            for (auto shaderTemplate : availableShaderTemplates)
            {
                AZ::IO::Path shaderName = shaderTemplate.m_shader;
                shaderName = shaderName.Filename(); // Removes the folder path
                shaderName = shaderName.ReplaceExtension(""); // This will remove the ".template" extension
                shaderName = shaderName.ReplaceExtension(""); // This will remove the ".shader" extension

                ShaderTemplateInfo shaderTemplateInfo;
                shaderTemplateInfo.m_template = shaderTemplate;
                shaderTemplateInfo.m_isIncluded = true;

                m_shaderTemplateStatusMap.emplace(AZStd::move(shaderName), AZStd::move(shaderTemplateInfo));
            }
        }

        MaterialPipelineScriptRunner::ScriptExecutionContext::ShaderTemplateStatusMap::iterator
            MaterialPipelineScriptRunner::ScriptExecutionContext::GetShaderStatusIterator(const AZStd::string& shaderTemplateName)
        {
            auto iter = m_shaderTemplateStatusMap.find(shaderTemplateName);
            if (iter == m_shaderTemplateStatusMap.end())
            {
                AZStd::vector<AZStd::string> availableShaderTemplateList;
                for (auto& [availableShaderTemplateName, enabled] : m_shaderTemplateStatusMap)
                {
                    AZ_UNUSED(enabled);
                    availableShaderTemplateList.push_back(availableShaderTemplateName);
                }

                AZStd::string availableShaderTemplateListString;
                AzFramework::StringFunc::Join(availableShaderTemplateListString, availableShaderTemplateList.begin(), availableShaderTemplateList.end(), ",");

                LuaScriptUtilities::Error(AZStd::string::format("Shader template named '%s' does not exist. The available shader templates are [%s]",
                    shaderTemplateName.c_str(), availableShaderTemplateListString.c_str()));
            }

            return iter;
        }

        void MaterialPipelineScriptRunner::ScriptExecutionContext::SetIncludeShader(const AZStd::string& shaderTemplateName, bool include)
        {
            auto iter = GetShaderStatusIterator(shaderTemplateName);
            if (iter != m_shaderTemplateStatusMap.end())
            {
                iter->second.m_isIncluded = include;
            }
        }

        void MaterialPipelineScriptRunner::ScriptExecutionContext::IncludeAllShaders()
        {
            for (auto& [shaderTemplateName, shaderTemplateInfo] : m_shaderTemplateStatusMap)
            {
                shaderTemplateInfo.m_isIncluded = true;
            }
        }

        void MaterialPipelineScriptRunner::ScriptExecutionContext::ExcludeAllShaders()
        {
            for (auto& [shaderTemplateName, shaderTemplateInfo] : m_shaderTemplateStatusMap)
            {
                shaderTemplateInfo.m_isIncluded = false;
            }
        }

        void MaterialPipelineScriptRunner::ScriptExecutionContext::IncludeShader(const char* shaderTemplateName)
        {
            SetIncludeShader(shaderTemplateName, true);
        }

        void MaterialPipelineScriptRunner::ScriptExecutionContext::ExcludeShader(const char* shaderTemplateName)
        {
            SetIncludeShader(shaderTemplateName, false);
        }

        MaterialPipelineScriptRunner::ShaderTemplatesList MaterialPipelineScriptRunner::ScriptExecutionContext::GetIncludedShaderTemplates() const
        {
            ShaderTemplatesList includedTemplates;
            for (auto& [shaderTemplateName, shaderTemplateInfo] : m_shaderTemplateStatusMap)
            {
                if (shaderTemplateInfo.m_isIncluded)
                {
                    includedTemplates.push_back(shaderTemplateInfo.m_template);
                }
            }
            return includedTemplates;
        }

        AZStd::string MaterialPipelineScriptRunner::ScriptExecutionContext::GetLightingModelName() const
        {
            return m_materialType.m_lightingModel;
        }

        MaterialPipelineScriptRunner::MaterialPipelineScriptRunner()
        {
            MaterialPipelineScriptRunner::ScriptExecutionContext::Reflect(&m_scriptBehaviorContext);
            LuaScriptUtilities::Reflect(&m_scriptBehaviorContext);
        }

        void MaterialPipelineScriptRunner::Reset()
        {
            m_relevantShaderTemplates.clear();
        }

        const MaterialPipelineScriptRunner::ShaderTemplatesList& MaterialPipelineScriptRunner::GetRelevantShaderTemplates() const
        {
            return m_relevantShaderTemplates;
        }

        bool MaterialPipelineScriptRunner::RunScript(
            const AZ::IO::Path& materialPipelineFile,
            const MaterialPipelineSourceData& materialPipeline,
            const MaterialTypeSourceData& materialType)
        {
            Reset();

            if (materialPipeline.m_pipelineScript.empty())
            {
                m_relevantShaderTemplates = materialPipeline.m_shaderTemplates;
                return true;
            }

            const AZStd::string scriptPath = RPI::AssetUtils::ResolvePathReference(materialPipelineFile.c_str(), materialPipeline.m_pipelineScript);

            auto reportError = [&]([[maybe_unused]] const AZStd::string& message)
            {
                AZ_Error(DebugName, false, "Script '%s' failed. %s", scriptPath.c_str(), message.c_str());
            };

            AZ::ScriptContext scriptContext;
            scriptContext.BindTo(&m_scriptBehaviorContext);

            // TODO(MaterialPipeline): At some point it would be nice if we didn't have to parse the lua script every time we need to run it, and
            // instead just use the corresponding ScriptAsset, similar to how LuaMaterialFunctorSourceData works. But AssetProcessor does not allow
            // an asset job for the "common" to load a product from the catalog of some specific platform, nor does it support loading any assets
            // from the "common" catalog. See https://github.com/o3de/o3de/issues/12863
            // (Remember this will require replacing the source dependency with a job dependency).
            const size_t MaxScriptFileSize = 1024 * 1024;
            auto luaScriptContent = AZ::Utils::ReadFile(scriptPath, MaxScriptFileSize);
            if (!luaScriptContent)
            {
                reportError(AZStd::string::format("Could not load script. %s", luaScriptContent.GetError().c_str()));
                return false;
            }

            if (!scriptContext.Execute(luaScriptContent.GetValue().data(), materialPipeline.m_pipelineScript.c_str(), luaScriptContent.GetValue().size()))
            {
                reportError("Error initializing script.");
                return false;
            }

            AZ::ScriptDataContext call;
            if (!scriptContext.Call(MainFunctionName, call))
            {
                reportError(AZStd::string::format("Function %s() is not defined.", MainFunctionName));
                return false;
            }

            ScriptExecutionContext luaContext{materialType, materialPipeline.m_shaderTemplates};
            call.PushArg(luaContext);

            if (!call.CallExecute())
            {
                reportError(AZStd::string::format("Failed calling %s().", MainFunctionName));
                return false;
            }

            if (1 != call.GetNumResults() || !call.IsBoolean(0))
            {
                reportError(AZStd::string::format("%s() must return a boolean.", MainFunctionName));
                return false;
            }

            bool result = false;
            if (!call.ReadResult(0, result))
            {
                reportError(AZStd::string::format("Failed reading the result of %s().", MainFunctionName));
                return false;
            }

            if (result)
            {
                m_relevantShaderTemplates = luaContext.GetIncludedShaderTemplates();
            }
            else
            {
                reportError(AZStd::string::format("%s() returned false.", MainFunctionName).c_str());
            }

            return result;
        }

    } // namespace RPI
} // namespace AZ
