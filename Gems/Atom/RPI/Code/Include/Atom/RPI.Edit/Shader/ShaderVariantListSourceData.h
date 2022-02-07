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

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        //! This is a simple data structure that represents a .shadervariantlist file.
        //! Provides configuration data about which shader variants should be generated for a given shader.
        //! Although Gems/Features can define their .shadervariantlists, game projects can override what variants to generate
        //! by declaring their own .shadervariantlists.
        struct ShaderVariantListSourceData
        {
            AZ_TYPE_INFO(AZ::RPI::ShaderVariantListSourceData, "{F8679938-6D3F-47CC-A078-3D6EC0011366}");
            AZ_CLASS_ALLOCATOR(ShaderVariantListSourceData, AZ::SystemAllocator, 0);

            static constexpr const char* Extension = "shadervariantlist";

            static void Reflect(ReflectContext* context);

            //! A struct that describes shader variant data that is used to populate a ShaderVariantListSourceData at asset build time.
            struct VariantInfo
            {
                AZ_TYPE_INFO(AZ::RPI::ShaderVariantListSourceData::VariantInfo, "{C0E1DF8C-D1BE-4AF4-8100-5D71788399BA}");

                // See ShaderVariantStableId.
                AZ::u32 m_stableId = 0;

                AZStd::unordered_map<AZStd::string/*optionName*/, AZStd::string/*valueName*/> m_options;
            };

            AZStd::string m_shaderFilePath; // .shader file.
            AZStd::vector<VariantInfo> m_shaderVariants;

        };
        
    } // namespace RPI
} // namespace AZ
