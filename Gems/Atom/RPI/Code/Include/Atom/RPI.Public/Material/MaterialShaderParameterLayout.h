/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Handle.h>
#include <Atom/RHI.Reflect/NameIdReflectionMap.h>
#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Reflect/Image/Image.h>
#include <Atom/RPI.Reflect/Material/MaterialNameContext.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyDescriptor.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Name/Name.h>
#include <AzCore/std/containers/vector.h>

namespace AZ::RPI
{
    // descriptor for a single Material Property Value used by the shader. Can be used to find the offset in a StructuredBuffer, or the
    // input-Index in an SRG.
    struct ATOM_RPI_PUBLIC_API MaterialShaderParameterDescriptor
    {
        AZ_TYPE_INFO(MaterialShaderParameterDescriptor, "{C2F4447C-1E01-479B-8E99-72AA8DFD9F97}");

        static void Reflect(ReflectContext* context);

        struct BufferBinding
        {
            AZ_TYPE_INFO(MaterialShaderParameterDescriptor::BufferBinding, "{9D088B39-B392-4603-8465-94E48D6083C9}");
            static void Reflect(ReflectContext* context);

            uint32_t m_elementSize{ 0 };
            uint32_t m_elementCount{ 0 };
            uint32_t m_offset{ 0 };
        };

        // Parameter name
        AZStd::string m_name;
        // GPU type name
        AZStd::string m_typeName;
        // Offset in the structured buffer.
        BufferBinding m_structuredBufferBinding;
        AZStd::variant<AZStd::monostate, RHI::ShaderInputConstantIndex, RHI::ShaderInputImageIndex> m_srgInputIndex;
        bool m_isBindlessReadIndex = false;
        // Pseudo-Params are parameters that aren't created from Material-Properties and can't be changed meaningfully,
        // but influence the layout (e.g. the MaterialTypeId and MaterialInstanceId, but also paddings).
        // We don't count these to figure out if the material has no parameters.
        bool m_isPseudoParam = false;
    };

    // Layout to store the value of a Material-Property (with a ShaderInput-Connection) in a StructuredBuffer and/or an entry in an SRG.
    // We generate the layout based on the Material-Properties found in a .materialtype - file, and we assume the shader expects the same
    // layout in the MaterialParameters - struct, but we can't actually verify that. Instead we provide the function
    // WriteMaterialParameterStructureAzsli() to generate an azsli - file containing a `struct MaterialParameters` definition with a
    // compatible layout, but the material actually has to use that file. Materials generated with the MaterialPipeline - process generally
    // do use it, but others generally dont.
    class ATOM_RPI_PUBLIC_API MaterialShaderParameterLayout
    {
    public:
        AZ_TYPE_INFO(MaterialShaderParameterLayout, "{538D434D-86A0-40DB-84FD-E4D0B4CF50ED}");

        using Index = RHI::Handle<uint32_t, MaterialShaderParameterLayout>;

        static void Reflect(ReflectContext* context);

        const Index GetParameterIndex(const AZStd::string_view& name) const;
        MaterialShaderParameterDescriptor* GetDescriptor(const Index parameterIndex);
        const MaterialShaderParameterDescriptor* const GetDescriptor(Index const parameterIndex) const;
        int ConnectParametersToSrg(const RHI::ShaderResourceGroupLayout* srgLayout);
        bool ConnectParameterToSrg(MaterialShaderParameterDescriptor* desc, const RHI::ShaderResourceGroupLayout* srgLayout);

        Index AddParameterFromPropertyConnection(const AZ::Name& name, const MaterialPropertyDataType dataType);
        Index AddParameterFromFunctor(const AZStd::string& name, const AZStd::string& typeName, const size_t typeSize = 0);

        Index AddMaterialParameter(
            const AZStd::string_view& name,
            const MaterialPropertyDataType dataType,
            const bool isPseudoParam = false,
            const size_t count = 1);
        Index AddTypedMaterialParameter(
            const AZStd::string_view& name,
            const AZStd::string_view& typeName,
            const size_t gpuTypesize,
            const bool isPseudoParam = false,
            const size_t count = 1);

        bool WriteMaterialParameterStructureAzsli(const AZ::IO::Path& filename) const;

        AZStd::span<const MaterialShaderParameterDescriptor> GetDescriptors() const
        {
            return m_descriptors;
        }

        const RHI::NameIdReflectionMap<Index>& GetNames() const
        {
            return m_names;
        }

        const int GetNonPseudoParameterCount() const
        {
            int count = 0;
            for (const auto& desc : m_descriptors)
            {
                if (!desc.m_isPseudoParam)
                {
                    count++;
                }
            }
            return count;
        }

        template<typename T>
        Index AddMaterialParameter(const Name& name, const bool isPseudoParam = false, const size_t count = 1)
        {
            return AddMaterialParameter<T>(name.GetStringView(), isPseudoParam, count);
        }

        template<typename T>
        Index AddMaterialParameter(const AZStd::string_view& name, const bool isPseudoParam = false, const size_t count = 1);

        void FinalizeLayout();

        void Reset();

        bool IsPropertyTypeCompatibleWithShaderParameter(
            const MaterialShaderParameterDescriptor* desc, const MaterialPropertyDataType dataType);
        bool IsPropertyTypeCompatibleWithSrg(const MaterialShaderParameterDescriptor* desc, const MaterialPropertyDataType dataType);

    private:
        size_t getStructuredBufferOffset() const;

        RHI::NameIdReflectionMap<Index> m_names;
        AZStd::vector<MaterialShaderParameterDescriptor> m_descriptors = {};
        int m_matrixPaddingIndex = 0;
    };

    class ATOM_RPI_PUBLIC_API MaterialShaderParameterNameIndex
    {
    public:
        AZ_TYPE_INFO(MaterialShaderParameterNameIndex, "{BD335A0E-A3F6-44EE-B477-D5B241FFC9B1}");

        static void Reflect(ReflectContext* context);

        MaterialShaderParameterNameIndex() = default;

        MaterialShaderParameterNameIndex(const Name& name, const MaterialNameContext* context = nullptr)
            : m_name(name)
        {
            ContextualizeName(context);
        };
        MaterialShaderParameterNameIndex(const AZStd::string& name, const MaterialNameContext* context = nullptr)
            : m_name(name)
        {
            ContextualizeName(context);
        };

        const AZ::Name& GetName()
        {
            return m_name;
        }
        const MaterialShaderParameterLayout::Index GetIndex() const
        {
            return m_index;
        }

        bool ValidateOrFindIndex(const MaterialShaderParameterLayout* layout);

    private:
        void ContextualizeName(const MaterialNameContext* context);

        AZ::Name m_name;
        MaterialShaderParameterLayout::Index m_index;
    };

} // namespace AZ::RPI