/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include<Data/ShaderVariantStatisticData.h>

namespace ShaderManagementConsole
{
    void ShaderVariantStatisticData::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ShaderVariantStatisticData>()
                ->Version(1)
                ->Field("OptionUsage", &ShaderVariantStatisticData::m_shaderOptionUsage)
                ->Field("VariantUsage", &ShaderVariantStatisticData::m_shaderVariantUsage)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<ShaderVariantStatisticData>("ShaderVariantStatisticData")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Shader")
                ->Attribute(AZ::Script::Attributes::Module, "shader")
                ->Property("shaderOptionUsage", BehaviorValueGetter(&ShaderVariantStatisticData::m_shaderOptionUsage), BehaviorValueSetter(&ShaderVariantStatisticData::m_shaderOptionUsage))
                ->Property("shaderVariantUsage", BehaviorValueGetter(&ShaderVariantStatisticData::m_shaderVariantUsage), BehaviorValueSetter(&ShaderVariantStatisticData::m_shaderVariantUsage))
                ;
        }
    }
}
