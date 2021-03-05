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
            AZStd::vector<AZStd::string> m_srgAssetFileNames;
            AZStd::vector<AZStd::unique_ptr<RootShaderVariantAssetSourceData>> m_rootShaderVariantAssets;
        };
    } // namespace RPI
} // namespace AZ
