/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Name/Name.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        //! This asset contains is loaded from a Json file and contains information about
        //! precompiled shader variants and their associated API name.
        struct RootShaderVariantAssetSourceData final
            : public Data::AssetData
        {
        public:
            AZ_RTTI(RootShaderVariantAssetSourceData, "{661EF8A7-7BAC-41B6-AD5C-C7249B2390AD}");
            AZ_CLASS_ALLOCATOR(RootShaderVariantAssetSourceData, SystemAllocator, 0);

            static void Reflect(ReflectContext* context);

            AZ::Name m_apiName;
            AZStd::string m_rootShaderVariantAssetFileName;
        };

        //! This asset contains is loaded from a Json file and contains information about
        //! precompiled shader assets.
        struct PrecompiledShaderAssetSourceData final
            : public Data::AssetData
        {
        public:
            AZ_RTTI(PrecompiledShaderAssetSourceData, "{C6B606EF-B788-4979-BA0F-6A28B33A1372}");
            AZ_CLASS_ALLOCATOR(PrecompiledShaderAssetSourceData, SystemAllocator, 0);

            static void Reflect(ReflectContext* context);

            AZStd::string m_shaderAssetFileName;
            AZStd::vector<AZStd::string> m_platformIdentifiers;
            AZStd::vector<AZStd::string> m_srgAssetFileNames;
            AZStd::vector<AZStd::unique_ptr<RootShaderVariantAssetSourceData>> m_rootShaderVariantAssets;
        };
    } // namespace RPI
} // namespace AZ
