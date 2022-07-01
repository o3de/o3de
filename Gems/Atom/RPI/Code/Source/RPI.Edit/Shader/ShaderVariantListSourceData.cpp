/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <Atom/RPI.Edit/Shader/ShaderVariantListSourceData.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace RPI
    {
        void ShaderVariantListSourceData::Reflect(ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<VariantInfo>()
                    ->Version(1)
                    ->Field("StableId", &VariantInfo::m_stableId)
                    ->Field("Options", &VariantInfo::m_options)
                    ;

                serializeContext->Class<ShaderVariantListSourceData>()
                    ->Version(1)
                    ->Field("Shader", &ShaderVariantListSourceData::m_shaderFilePath)
                    ->Field("Variants", &ShaderVariantListSourceData::m_shaderVariants)
                    ;

                if (auto editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<VariantInfo>("VariantInfo", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &VariantInfo::m_stableId, "Stable Id", "Unique identifier for this shader variant within the list")
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &VariantInfo::m_options, "Options", "Table of shader options for configuring this variant")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                            ->Attribute(AZ::Edit::Attributes::ContainerReorderAllow, false)
                        ;

                    editContext->Class<ShaderVariantListSourceData>("ShaderVariantListSourceData", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ShaderVariantListSourceData::m_shaderFilePath, "Shader File Path", "Path to the shader source this variant list represents")
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ShaderVariantListSourceData::m_shaderVariants, "Shader Variants", "Container of all variants and options configured for the shader")
                            ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                            ->Attribute(AZ::Edit::Attributes::ContainerReorderAllow, false)
                        ;
                }
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<VariantInfo>("ShaderVariantInfo")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Shader")
                    ->Attribute(AZ::Script::Attributes::Module, "shader")
                    ->Property("stableId", BehaviorValueGetter(&VariantInfo::m_stableId), BehaviorValueSetter(&VariantInfo::m_stableId))
                    ->Property("options", BehaviorValueGetter(&VariantInfo::m_options), BehaviorValueSetter(&VariantInfo::m_options))
                    ;

                behaviorContext->Class<ShaderVariantListSourceData>("ShaderVariantListSourceData")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Shader")
                    ->Attribute(AZ::Script::Attributes::Module, "shader")
                    ->Property("shaderFilePath", BehaviorValueGetter(&ShaderVariantListSourceData::m_shaderFilePath), BehaviorValueSetter(&ShaderVariantListSourceData::m_shaderFilePath))
                    ->Property("shaderVariants", BehaviorValueGetter(&ShaderVariantListSourceData::m_shaderVariants), BehaviorValueSetter(&ShaderVariantListSourceData::m_shaderVariants))
                    ;

                // [GFX TODO][ATOM-14858] Expose JsonUtils to Behavior Context in JsonUtils.cpp and make it generic
                behaviorContext->Method("SaveShaderVariantListSourceData", &JsonUtils::SaveObjectToFile<ShaderVariantListSourceData>)
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Shader")
                    ->Attribute(AZ::Script::Attributes::Module, "shader")
                    ;
            }
        }
    } // namespace RPI
} // namespace AZ
