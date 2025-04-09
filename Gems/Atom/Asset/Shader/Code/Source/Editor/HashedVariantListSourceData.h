/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RPI.Edit/Shader/ShaderVariantListSourceData.h>

namespace AZ
{
    class ReflectContext;

    namespace ShaderBuilder
    {
        //! This structure represents the content of an intermediate asset
        //! with file extension "hashedvariantinfo".
        //! The ShaderVariantAssetBuilder will react to this file pattern and create
        //! a single product with extension "azshadervariant" (ShaderVariantAsset) per "hashedvariantinfo".
        //!
        //! This struct is also leveraged by HashedVariantListSourceData (see below)
        //! to create a single list of all variants, which will be used to create
        //! another intermediate asset called the "hashedvariantlist" which will be used
        //! by the ShaderVariantAssetBuilder to build the "azshadervarianttree" output asset
        //! that the runtime will load a ShaderVariantTreeAsset.
        //!
        //! REMARK1: Users are not expected to create .hashedvariantinfo files. These files are produced by the ShaderVariantListBuilder
        //! as intermediate assets.
        //!
        //! REMARK2: These files will be named <Shader Name>_<StableId>.hashedvariantinfo, Where the StableId is a 1-based index.
        //!
        //! REMARK3: As an Intermediate Asset the Product SubID will be the StableId, because SubId 0 is reserved for the
        //! ".hashedvariantlist".
        struct HashedVariantInfoSourceData
        {
            AZ_TYPE_INFO_WITH_NAME_DECL(HashedVariantInfoSourceData);

            static constexpr const char* Extension = "hashedvariantbatch";

            AZ::RPI::ShaderVariantListSourceData::VariantInfo m_variantInfo;
            size_t m_hash = 0; // Hash of all the data in @m_variantInfo

            //! Hash combines all the data in @optionValues.
            static size_t HashCombineShaderOptionValues(size_t startingHash, const AZ::RPI::ShaderOptionValuesSourceData& optionValues);

            //! Hash combines the result of a previous call to HashCombineShaderOptionValues, which is passed to this function
            //! in @optionValuesHash argument, with the rest of the data in @variantInfo.
            static size_t CalculateHash(size_t optionValuesHash, const AZ::RPI::ShaderVariantListSourceData::VariantInfo& variantInfo);

            //! Same as above, but uses this.m_variantInfo and stores the result in this.m_hash.
            void CalculateHash(size_t optionValuesHash);
        };

        //! This is a simple data structure that represents a .hashedvariantlist file.
        //! Users are not expected to create .hashedvariantlist files. These files are produced by the ShaderVariantListBuilder
        //! as intermediate assets.
        //! Provides configuration data about which shader variants should be used to create a ShaderVariantTreeAsset.
        //!
        //! REMARK:  These files will be named <Shader Name>.hashedvariantlist.
        //! Using the name and the subpath of this file We can figure out the name of the
        //! *.shader file.
        struct HashedVariantListSourceData
        {
            AZ_TYPE_INFO_WITH_NAME_DECL(HashedVariantListSourceData);
            AZ_CLASS_ALLOCATOR(HashedVariantListSourceData, AZ::SystemAllocator);

            static constexpr const char* Extension = "hashedvariantlist";
            static constexpr uint32_t SubId = 0;

            static void Reflect(ReflectContext* context);

            // Original, and absolute, path of the corresponsing *.shader file.
            // This needs to be stored to preserve the casing. Without this Linux
            // won't work.
            AZStd::string m_shaderPath;

            AZStd::vector<HashedVariantInfoSourceData> m_hashedVariants;
        };

    } // namespace ShaderBuilder
} // namespace AZ
