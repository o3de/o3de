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
#include <Atom/RPI.Edit/Configuration.h>
#include <Atom/RPI.Edit/Shader/ShaderOptionValuesSourceData.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        //! This is a simple data structure that represents a .shadervariantlist file.
        //! Provides configuration data about which shader variants should be generated for a given shader.
        //! Although Gems/Features can define their .shadervariantlists, game projects can override what variants to generate
        //! by declaring their own .shadervariantlists.
        struct ATOM_RPI_EDIT_API ShaderVariantListSourceData
        {
            AZ_TYPE_INFO(AZ::RPI::ShaderVariantListSourceData, "{F8679938-6D3F-47CC-A078-3D6EC0011366}");
            AZ_CLASS_ALLOCATOR(ShaderVariantListSourceData, AZ::SystemAllocator);

            static constexpr const char* Extension = "shadervariantlist";
            static constexpr const char* DefaultTarget = "gfx1035";

            static void Reflect(ReflectContext* context);

            //! A struct that describes shader variant data that is used to populate a ShaderVariantListSourceData at asset build time.
            struct VariantInfo
            {
                AZ_TYPE_INFO(AZ::RPI::ShaderVariantListSourceData::VariantInfo, "{C0E1DF8C-D1BE-4AF4-8100-5D71788399BA}");

                VariantInfo() = default;

                VariantInfo(AZ::u32 id, ShaderOptionValuesSourceData options)
                    : m_stableId(id)
                    , m_options(options)
                    , m_enableRegisterAnalysis(false)
                    , m_asic(DefaultTarget)
                {}

                VariantInfo(AZ::u32 id, ShaderOptionValuesSourceData options, bool enableAnalysis, AZStd::string asic)
                    : m_stableId(id)
                    , m_options(options)
                    , m_enableRegisterAnalysis(enableAnalysis)
                    , m_asic(asic)
                {}

                // See ShaderVariantStableId.
                AZ::u32 m_stableId = 0;

                //! Map of option-names to value-names (string formatted values)
                ShaderOptionValuesSourceData m_options;

                
                // Output register analysis data
                // RGA did support DX12, but some how it couldn't build our hlsl
                // So for now we only use vulkan offline mode
                // To activate it:
                //     1. Set O3DE_RADEON_GPU_ANALYZER_ENABLED in {Build Folder Path (e.g. build/windows)}/CMakeCache.txt to TRUE, rerun cmake so RGA can be downloaded
                //     2. Set EnableAnalysis to true in your shader variant in .shadervariantlist
                bool m_enableRegisterAnalysis = false;

                // The GPU target to use on register analysis
                // The value depends on the version of RGA we use
                // Current RGA is 2.6.2
                // Supported values: gfx900 gfx902 gfx906 gfx90c gfx1010 gfx1011 gfx1012 gfx1030 gfx1031 gfx1032 gfx1034 gfx1035 
                AZStd::string m_asic = AZStd::string(DefaultTarget);
            };

            AZStd::string m_shaderFilePath; // .shader file.
            AZStd::vector<VariantInfo> m_shaderVariants;

            //! non-mandatory list of option-names that have been gatherd from material properties.
            //! This allows for UX hints in differenciating system options from material options.
            AZStd::unordered_set<Name> m_materialOptionsHint;
        };

    } // namespace RPI
} // namespace AZ
