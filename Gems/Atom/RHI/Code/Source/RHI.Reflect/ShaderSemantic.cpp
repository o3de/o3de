/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/ShaderSemantic.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/Utils/TypeHash.h>

namespace AZ::RHI
{
    void ShaderSemantic::Reflect(AZ::ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<ShaderSemantic>()
                ->Version(1)
                ->Field("m_name", &ShaderSemantic::m_name)
                ->Field("m_index", &ShaderSemantic::m_index);
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<ShaderSemantic>("ShaderSemantic")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "render")
                ->Attribute(AZ::Script::Attributes::Module, "render")
                ->Constructor()
                ->Constructor<const ShaderSemantic&>()
                ->Constructor<const Name&, size_t>()
                ->Constructor<AZStd::string_view, size_t>()
                ->Method("ToString", &ShaderSemantic::ToString)
                ->Property("name", BehaviorValueProperty(&ShaderSemantic::m_name))
                ->Property("index", BehaviorValueProperty(&ShaderSemantic::m_index))
                ;
        }
    }

    ShaderSemantic ShaderSemantic::Parse(AZStd::string_view semantic)
    {
        AZStd::string semanticName = semantic;
        const size_t semanticIndexPos = semanticName.find_last_not_of("0123456789") + 1;
        const size_t semanticIndex = AZStd::stoi(semanticName.substr(semanticIndexPos));
        semanticName = semanticName.substr(0, semanticIndexPos);

        return ShaderSemantic(Name{ semanticName.data() }, static_cast<uint32_t>(semanticIndex));
    }

    ShaderSemantic::ShaderSemantic(const Name& name, size_t index)
        : m_name{ name }
        , m_index{ static_cast<uint32_t>(index) }
    {
#ifdef AZ_ENABLE_TRACING
        if (!m_name.IsEmpty())
        {
            const char last = m_name.GetStringView()[m_name.GetStringView().size()-1];
            AZ_Assert(last < '0' || last > '9', "Name should not end with numeric characters. Use ShaderSemantic::Parse().");
        }
#endif
    }

    ShaderSemantic::ShaderSemantic(AZStd::string_view name, size_t index)
        : ShaderSemantic{ Name{name}, index }
    {
    }

    bool ShaderSemantic::operator==(const ShaderSemantic& rhs) const
    {
        return this->m_index == rhs.m_index && this->m_name == rhs.m_name;
    }

    HashValue64 ShaderSemantic::GetHash(HashValue64 seed) const
    {
        seed = TypeHash64(m_name.GetHash(), seed);
        seed = TypeHash64(m_index, seed);
        return seed;
    }

    AZStd::string ShaderSemantic::ToString() const
    {
        return m_name.GetCStr() + AZStd::to_string(m_index);
    }
}
