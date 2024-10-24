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
#include <Atom/RPI.Reflect/Image/Image.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyDescriptor.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Name/Name.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    namespace RPI
    {

        struct MaterialShaderParameterDescriptor
        {
            AZ_TYPE_INFO(MaterialShaderParameterDescriptor, "{C2F4447C-1E01-479B-8E99-72AA8DFD9F97}");

            static void Reflect(ReflectContext* context);

            struct BufferBinding
            {
                AZ_TYPE_INFO(MaterialShaderParameterDescriptor::BufferBinding, "{9D088B39-B392-4603-8465-94E48D6083C9}");
                static void Reflect(ReflectContext* context);

                size_t m_elementSize;
                size_t m_elementCount;
                size_t m_offset;
            };

            AZStd::string m_name;
            AZStd::string m_typeName;
            BufferBinding m_structuredBufferBinding;
            AZStd::variant<AZStd::monostate, RHI::ShaderInputConstantIndex, RHI::ShaderInputImageIndex> m_srgInputIndex;
            bool m_isBindlessReadIndex = false;
            bool m_isPseudoParam = false;
        };

        class MaterialShaderParameterLayout
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

            AZStd::vector<MaterialShaderParameterDescriptor>& GetDescriptors()
            {
                return m_descriptors;
            }

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
            Index AddMaterialParameter(const Name& name, const bool isPseudoParam = false, const size_t count = 1);
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

    } // namespace RPI
} // namespace AZ
