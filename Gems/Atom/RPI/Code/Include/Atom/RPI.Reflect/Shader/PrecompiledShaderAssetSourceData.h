/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Configuration.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Name/Name.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        //! This asset is loaded from a Json file and contains information about
        //! precompiled shader variants and their associated API name.
        AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
        struct ATOM_RPI_REFLECT_API PrecompiledRootShaderVariantAssetSourceData final
            : public Data::AssetData
        {
            AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING

        public:
            AZ_RTTI(PrecompiledRootShaderVariantAssetSourceData, "{661EF8A7-7BAC-41B6-AD5C-C7249B2390AD}");
            AZ_CLASS_ALLOCATOR(PrecompiledRootShaderVariantAssetSourceData, SystemAllocator);

            static void Reflect(ReflectContext* context);

            AZ::Name m_apiName;
            AZStd::string m_rootShaderVariantAssetFileName;
        };

        //! This asset is loaded from a Json file and contains information about
        //! precompiled shader supervariants
        AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
        struct ATOM_RPI_REFLECT_API PrecompiledSupervariantSourceData final
            : public Data::AssetData
        {
            AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING

        public:
            AZ_RTTI(PrecompiledSupervariantSourceData, "{630BDF15-CE7C-4E2C-882E-4F7AF09C8BB6}");
            AZ_CLASS_ALLOCATOR(PrecompiledSupervariantSourceData, SystemAllocator);

            static void Reflect(ReflectContext* context);

            AZ::Name m_name;
            AZStd::vector<AZStd::unique_ptr<PrecompiledRootShaderVariantAssetSourceData>> m_rootShaderVariantAssets;
        };

        //! This asset is loaded from a Json file and contains information about
        //! precompiled shader assets.
        AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
        struct ATOM_RPI_REFLECT_API PrecompiledShaderAssetSourceData final
            : public Data::AssetData
        {
            AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING

        public:
            AZ_RTTI(PrecompiledShaderAssetSourceData, "{C6B606EF-B788-4979-BA0F-6A28B33A1372}");
            AZ_CLASS_ALLOCATOR(PrecompiledShaderAssetSourceData, SystemAllocator);

            static void Reflect(ReflectContext* context);

            AZStd::string m_shaderAssetFileName;
            AZStd::vector<AZStd::string> m_platformIdentifiers;
            AZStd::vector<AZStd::unique_ptr<PrecompiledSupervariantSourceData>> m_supervariants;
        };
    } // namespace RPI
} // namespace AZ
