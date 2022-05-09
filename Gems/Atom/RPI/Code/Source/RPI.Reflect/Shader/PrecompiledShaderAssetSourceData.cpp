/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        void PrecompiledRootShaderVariantAssetSourceData::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<PrecompiledRootShaderVariantAssetSourceData>()
                    ->Version(0)
                    ->Field("APIName", &PrecompiledRootShaderVariantAssetSourceData::m_apiName)
                    ->Field("RootShaderVariantAssetFileName", &PrecompiledRootShaderVariantAssetSourceData::m_rootShaderVariantAssetFileName)
                    ;
            }
        }

        void PrecompiledSupervariantSourceData::Reflect(ReflectContext* context)
        {
            PrecompiledRootShaderVariantAssetSourceData::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<PrecompiledSupervariantSourceData>()
                    ->Version(0)
                    ->Field("Name", &PrecompiledSupervariantSourceData::m_name)
                    ->Field("RootShaderVariantAssets", &PrecompiledSupervariantSourceData::m_rootShaderVariantAssets)
                    ;
            }
        }

        void PrecompiledShaderAssetSourceData::Reflect(ReflectContext* context)
        {
            PrecompiledSupervariantSourceData::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<PrecompiledShaderAssetSourceData>()
                    ->Version(2) // ATOM-15740
                    ->Field("ShaderAssetFileName", &PrecompiledShaderAssetSourceData::m_shaderAssetFileName)
                    ->Field("PlatformIdentifiers", &PrecompiledShaderAssetSourceData::m_platformIdentifiers)
                    ->Field("Supervariants", &PrecompiledShaderAssetSourceData::m_supervariants)
                    ;
            }
        }
    } // namespace RPI
} // namespace AZ
