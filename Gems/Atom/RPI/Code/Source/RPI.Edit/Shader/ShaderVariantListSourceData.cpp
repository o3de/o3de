/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Shader/ShaderVariantListSourceData.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>


namespace AZ
{
    namespace RPI
    {
        void ShaderVariantListSourceData::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderVariantListSourceData::VariantInfo>()
                    ->Version(1)
                    ->Field("StableId", &VariantInfo::m_stableId)
                    ->Field("Options", &VariantInfo::m_options)
                    ;

                serializeContext->Class<ShaderVariantListSourceData>()
                    ->Version(1)
                    ->Field("Shader", &ShaderVariantListSourceData::m_shaderFilePath)
                    ->Field("Variants", &ShaderVariantListSourceData::m_shaderVariants)
                    ;
            }

            if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
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
