/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Material/ShaderFunctionCollection.h>
#include <AzCore/Asset/AssetSerializer.h>

namespace AZ::RPI
{
    void ShaderFunction::Reflect(AZ::ReflectContext* context)
    {
        if (auto* sc = azrtti_cast<AZ::SerializeContext*>(context))
        {
            sc->Class<ShaderFunctionCollection>()
                ->Version(1)
                ->Field("SRGSource", &ShaderFunction::m_srgSource)
                ->Field("Source", &ShaderFunction::m_source);
        }
    }

    void ShaderFunctionCollection::Reflect(AZ::ReflectContext* context)
    {
        if (auto* sc = azrtti_cast<AZ::SerializeContext*>(context))
        {
            sc->Class<ShaderFunctionCollection>()
                ->Version(1)
                ->Field("ShaderFunctions", &ShaderFunctionCollection::m_shaderFunctions);
        }
    }

    ShaderFunction* ShaderFunctionCollection::LookupShaderFunction(AZStd::string name)
    {
        auto it = m_shaderFunctions.find(name);
        if (it == m_shaderFunctions.end())
        {
            return nullptr;
        }

        return &it->second;
    }
}

