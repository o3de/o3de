/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Edit/Material/MaterialPipelineSourceData.h>
#include <AzCore/IO/Path/Path.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        class MaterialTypeSourceData;

        class MaterialPipelineScriptRunner
        {
        public:
            using ShaderTemplatesList = AZStd::vector<MaterialPipelineSourceData::ShaderTemplate>;

            MaterialPipelineScriptRunner();

            static void Reflect(ReflectContext* context);

            bool RunScript(const AZ::IO::Path& materialPipelineFile, const MaterialPipelineSourceData& materialPipeline, const MaterialTypeSourceData& materialType);
            void Reset();

            const ShaderTemplatesList& GetRelevantShaderTemplates() const;

        private:

            class ScriptExecutionContext
            {
            public:
                AZ_TYPE_INFO(ScriptExecutionContext, "{DB3E5775-40FB-4F68-BCF4-4E21649F2316}");

                static void Reflect(ReflectContext* context);

                ScriptExecutionContext(const MaterialTypeSourceData& materialType, const ShaderTemplatesList& availableShaderTemplates);

                AZStd::string GetLightingModelName() const;
                void IncludeAllShaders();
                void ExcludeAllShaders();
                void IncludeShader(const char* shaderTemplateName);
                void ExcludeShader(const char* shaderTemplateName);

                ShaderTemplatesList GetIncludedShaderTemplates() const;

            private:

                struct ShaderTemplateInfo
                {
                    MaterialPipelineSourceData::ShaderTemplate m_template;
                    bool m_isIncluded;
                };

                using ShaderTemplateStatusMap = AZStd::unordered_map<AZStd::string/*shaderTemplateName*/, ShaderTemplateInfo>;
                ShaderTemplateStatusMap::iterator GetShaderStatusIterator(const AZStd::string& shaderTemplateName);
                void SetIncludeShader(const AZStd::string& shaderTemplateName, bool include);

                const MaterialTypeSourceData& m_materialType;

                ShaderTemplateStatusMap m_shaderTemplateStatusMap;
            };

            static constexpr char const DebugName[] = "MaterialPipelineScriptRunner";

            // TODO(MaterialPipeline): I think I want to rename this to something better, maybe "MaterialTypeBuilderSetup" since
            // what it's really configuring is the behavior of the MaterialTypeBuilder.
            static constexpr char const MainFunctionName[] = "MaterialTypeSetup";

            ShaderTemplatesList m_relevantShaderTemplates;
        };

    } // namespace RPI
} // namespace AZ
