/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Shader/PrecompiledShaderAssetSourceData.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace RPI
    {
        void RootShaderVariantAssetSourceData::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<RootShaderVariantAssetSourceData>()
                    ->Version(0)
                    ->Field("APIName", &RootShaderVariantAssetSourceData::m_apiName)
                    ->Field("RootShaderVariantAssetFileName", &RootShaderVariantAssetSourceData::m_rootShaderVariantAssetFileName)
                    ;
            }
        }

        void PrecompiledShaderAssetSourceData::Reflect(ReflectContext* context)
        {
            RootShaderVariantAssetSourceData::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<PrecompiledShaderAssetSourceData>()
                    ->Version(0)
                    ->Field("ShaderAssetFileName", &PrecompiledShaderAssetSourceData::m_shaderAssetFileName)
                    ->Field("PlatformIdentifiers", &PrecompiledShaderAssetSourceData::m_platformIdentifiers)
                    ->Field("ShaderResourceGroupAssets", &PrecompiledShaderAssetSourceData::m_srgAssetFileNames)
                    ->Field("RootShaderVariantAssets", &PrecompiledShaderAssetSourceData::m_rootShaderVariantAssets)
                    ;
            }
        }
    } // namespace RPI
} // namespace AZ
