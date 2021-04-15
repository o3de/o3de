/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
