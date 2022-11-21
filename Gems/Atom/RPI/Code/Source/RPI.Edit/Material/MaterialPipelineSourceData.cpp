/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Material/MaterialPipelineSourceData.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace RPI
    {
        void MaterialPipelineSourceData::Reflect(ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderTemplate>()
                    ->Version(1)
                    ->Field("shader", &ShaderTemplate::m_shader)
                    ->Field("azsli", &ShaderTemplate::m_azsli)
                    ;

                serializeContext->Class<MaterialPipelineSourceData>()
                    ->Version(1)
                    ->Field("shaderTemplates", &MaterialPipelineSourceData::m_shaderTemplates)
                    ->Field("pipelineScript", &MaterialPipelineSourceData::m_pipelineScript)
                    ;

                if (auto editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<ShaderTemplate>("ShaderTemplate", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ShaderTemplate::m_shader, "Shader", "The template used to create the .shader file.")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ShaderTemplate::m_azsli, "AZSLi", "The azsli file that should stitched together with material shader code.")
                        ;

                    editContext->Class<MaterialPipelineSourceData>("MaterialPipelineSourceData", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialPipelineSourceData::m_shaderTemplates, "Shader Templates", "List of templates used to generate material-specific shaders.")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialPipelineSourceData::m_pipelineScript, "Pipeline Script Path", "The pipeline can use a lua script to configure shader compilation.")
                        ;
                }
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<ShaderTemplate>("ShaderTemplate")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "RPI")
                    ->Attribute(AZ::Script::Attributes::Module, "rpi")
                    ->Constructor()
                    ->Constructor<const ShaderTemplate&>()
                    ->Property("shader", BehaviorValueProperty(&ShaderTemplate::m_shader))
                    ->Property("azsli", BehaviorValueProperty(&ShaderTemplate::m_azsli))
                    ;

                behaviorContext->Class<MaterialPipelineSourceData>("MaterialPipelineSourceData")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "RPI")
                    ->Attribute(AZ::Script::Attributes::Module, "rpi")
                    ->Constructor()
                    ->Constructor<const MaterialPipelineSourceData&>()
                    ->Property("shaderTemplates", BehaviorValueProperty(&MaterialPipelineSourceData::m_shaderTemplates))
                    ->Property("pipelineScript", BehaviorValueProperty(&MaterialPipelineSourceData::m_pipelineScript))
                    ;
            }
        }

    } // namespace RPI
} // namespace AZ
