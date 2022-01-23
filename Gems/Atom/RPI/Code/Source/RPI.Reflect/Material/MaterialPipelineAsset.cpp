/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Material/MaterialPipelineAsset.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace AZ::RPI
{
    void MaterialPipelineAsset::Reflect(ReflectContext* context)
    {
        if (auto* sc = azrtti_cast<SerializeContext*>(context))
        {
            sc->Class<MaterialPipelineAsset, AZ::Data::AssetData>()
                ->Version(1)
                ->Field("Version", &MaterialPipelineAsset::m_version)
                ->Field("Name", &MaterialPipelineAsset::m_name)
                ->Field("ShaderCollection", &MaterialPipelineAsset::m_shaderCollection);
        }
    }

    const ShaderCollection& MaterialPipelineAsset::GetShaderCollection() const
    {
        return m_shaderCollection;
    }
} // namespace AZ::RPI
