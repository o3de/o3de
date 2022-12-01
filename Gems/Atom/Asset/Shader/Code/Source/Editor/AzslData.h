/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI.Reflect/Handle.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

#include <CommonFiles/CommonTypes.h>

namespace AZ
{
    namespace ShaderBuilder
    {
        using ConstantBufferContainer = AZStd::vector<ConstantBufferData>;
        using StructContainer = AZStd::vector<StructData>;
        using SamplerContainer = AZStd::vector<SamplerSrgData>;
        using TextureContainer = AZStd::vector<TextureSrgData>;
        using BufferContainer = AZStd::vector<BufferSrgData>;
        using SrgConstantContainer = AZStd::vector<SrgConstantData>;

        struct SrgData
        {
            //friend bool operator == (const SrgData&, const SrgData&);

            AZStd::string m_name;
            AZStd::string m_containingFileName;

            // One SRG contains the ShaderVariantKey fallback structure
            // A size greater than 0 indicates that this SRG data is designated as fallback
            Name                    m_fallbackName;
            uint32_t                m_fallbackSize = 0;

            RHI::Handle<uint32_t>   m_bindingSlot;

            ConstantBufferContainer m_constantBuffers;
            SamplerContainer        m_samplers;
            StructContainer         m_structs;
            TextureContainer        m_textures;
            BufferContainer         m_buffers;
            SrgConstantContainer    m_srgConstantData;
            uint32_t                m_srgConstantDataRegisterId = RHI::UndefinedRegisterSlot;
            uint32_t                m_srgConstantDataSpaceId = RHI::UndefinedRegisterSlot;
        };

        struct FunctionData
        {
            AZStd::string m_returnType;
            AZStd::string m_name;
            AZStd::string m_parametersAndContents; // Everything else is lumped together

            // Indicates if the function declares any shader stage inputs or outputs by the use of semantics
            bool m_hasShaderStageVaryings = false;
            RHI::ShaderStageAttributeMap attributesList;
        };

        typedef AZStd::vector<SrgData> SrgDataContainer;
        typedef AZStd::vector<FunctionData> AzslFunctions;

        struct RootConstantData
        {
            RootConstantBinding     m_bindingInfo;
            SrgConstantContainer    m_constants;
        };

        struct ShaderFiles
        {
            AZStd::string    m_azslSourceFullPath;   //!< Full path to the source AZSL file (referred by the "Source" json element in .shader)
            AZStd::string    m_shaderFileName;       //!< Name for the .shader file
            AZStd::string    m_azslFileName;         //!< Name for the source .azsl file
        };


        //! DEPRECATED [ATOM-15472]
        //! This class is used to collect all the json files produced by the compilation
        //! of an AZSL file as objects.
        struct AzslData
        {
            AzslData(const AZStd::shared_ptr<ShaderFiles>& a_sources) : m_sources(a_sources) { }

            AZStd::shared_ptr<ShaderFiles> m_sources;
            AZStd::string    m_preprocessedFullPath; // Full path to a preprocessed version of the original AZSL file
            AZStd::string m_shaderCodePrefix; // AssetProcessor generated shader code which is added to the
                                              // AZSLc emitted code prior to invoking the native shader compiler

            SrgDataContainer m_srgData;
            AzslFunctions m_functions;
            StructContainer m_structs;
            RootConstantData m_rootConstantData;
        };

        //! This class is used to collect all the json files produced by the compilation
        //! of an AZSL file as objects.
        struct AzslData2
        {
            AzslData2(const AZStd::shared_ptr<ShaderFiles>& a_sources)
                : m_sources(a_sources)
            {
            }

            AZStd::shared_ptr<ShaderFiles> m_sources;
            AZStd::string m_preprocessedFullPath; // Full path to a preprocessed version of the original AZSL file

            SrgDataContainer m_srgData;
            AzslFunctions m_functions;
            StructContainer m_structs;
            RootConstantData m_rootConstantData;
        };
    } // ShaderBuilder
} // AZ
