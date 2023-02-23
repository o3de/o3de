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
        //! This is a simple data structure that represents a .hashedvariantlist file.
        //! Users are not expected to create .hashedvariantlist files. These files are produced by the ShaderVariantListBuilder
        //! as intermediate assets.
        //! Provides configuration data about which shader variants should be generated for a given shader.
        struct HashedVariantListSourceData
        {
            AZ_TYPE_INFO_WITH_NAME_DECL(HashedVariantListSourceData);
            AZ_CLASS_ALLOCATOR(HashedVariantListSourceData, AZ::SystemAllocator);

            static constexpr const char* Extension = "hashedvariantlist";
            static constexpr uint32_t SubId = 0;

            static void Reflect(ReflectContext* context);

            //! A struct that describes each hashed shader variant data.
            struct HashedVariantInfo
            {
                AZ_TYPE_INFO_WITH_NAME_DECL(HashedVariantInfo);

                AZ::RPI::ShaderVariantListSourceData::VariantInfo m_variantInfo;
                size_t m_hash = 0; // Hash of all the data in @m_variantInfo
                bool m_isNew = false; // If true, the corresponding ShaderVariantAsset should be built/rebuilt.

                //! Hash combines all the data in @optionValues.
                static size_t HashCombineShaderOptionValues(size_t startingHash, const AZ::RPI::ShaderOptionValuesSourceData& optionValues);

                //! Hash combines the result of a previous call to HashCombineShaderOptionValues, which is passed to this function
                //! in @optionValuesHash argument, with the rest of the data in @variantInfo.
                static size_t CalculateHash(size_t optionValuesHash, const AZ::RPI::ShaderVariantListSourceData::VariantInfo& variantInfo);

                //! Same as above, but uses this.m_variantInfo and stores the result in this.m_hash.
                void CalculateHash(size_t optionValuesHash);
            };

            // A time stamp is necessary, because building shader variants takes time.
            // We calculate hashes to figure out the variants that changed and based on the match
            // we set HashedVariantInfo::m_isNew to false or true.
            // Imagine a user makes changes to a .shadervariantlist, and some HashedVariantInfos are marked as new.
            // then within a few seconds later they make another change to the .shadervariantlist file. Because
            // it happened so quickly those HashedVariantInfos that were set as new would become "old" and those
            // shader variants won't be compiled.
            // This timestamp comes to the rescue and we can measure if the change happened too quick
            // and it such case we preserve the state of each HashedVariantInfo::m_isNew.
            long long m_timeStamp;

            AZStd::string m_shaderFilePath; // .shader file.
            AZStd::vector<HashedVariantInfo> m_hashedVariants;
        };

    } // namespace ShaderBuilder
} // namespace AZ
