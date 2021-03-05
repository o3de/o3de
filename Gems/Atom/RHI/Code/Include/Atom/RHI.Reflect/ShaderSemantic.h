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

#include <AzCore/Name/Name.h>
#include <AzCore/Utils/TypeHash.h>

namespace AZ
{
    namespace RHI
    {
         //! Describes a shader semantic (name + index). This should match the semantic declared in AZSL.
        class ShaderSemantic
        {
        public:
            AZ_TYPE_INFO(ShaderSemantic, "{C6FFF25F-FE52-4D08-8D96-D04C14048816}");
            static void Reflect(AZ::ReflectContext* context);

            //! The prefix keyword to extract UV shader inputs, so that we can stream different UV sets.
            static constexpr const char UvStreamSemantic[] = "UV";

            static ShaderSemantic Parse(AZStd::string_view semantic);

            ShaderSemantic() = default;
            explicit ShaderSemantic(const Name& name, size_t index = 0);
            explicit ShaderSemantic(AZStd::string_view name, size_t index = 0);

            bool operator==(const ShaderSemantic& rhs) const;

            HashValue64 GetHash(HashValue64 seed = HashValue64{ 0 }) const;

            AZStd::string ToString() const;

            //! Name of the binding.
            Name m_name;

            //! Index of the binding with this semantic.
            uint32_t m_index = 0;
        };
    } // namespace RHI
} // namespace AZ

namespace AZStd
{
    template <>
    struct hash<AZ::RHI::ShaderSemantic>
    {
        size_t operator()(const AZ::RHI::ShaderSemantic& shaderSemantic) const
        {
            return aznumeric_cast<size_t>(shaderSemantic.GetHash());
        }
    };
}
