/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Name/Name.h>
#include <AzCore/Utils/TypeHash.h>
#include <AzCore/Casting/numeric_cast.h>

namespace AZ::RHI
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
}

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
