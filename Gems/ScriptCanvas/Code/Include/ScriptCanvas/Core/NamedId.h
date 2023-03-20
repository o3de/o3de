/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>


namespace ScriptCanvas
{
    class ReflectContext;

    template<typename t_Id>
    class NamedId : public t_Id
    {
    public:
        using ThisType = NamedId<t_Id>;

        AZ_CLASS_ALLOCATOR(NamedId<t_Id>, AZ::SystemAllocator);
        AZ_RTTI((NamedId , "{7DFA6B31-B283-48BE-9D6F-260D8994C593}", t_Id), t_Id);

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ThisType, t_Id>()
                    ->Version(0)
                    ->Field("name", &ThisType::m_name)
                    ;
            }
        }

        AZStd::string m_name;
        
        NamedId() = default;
        virtual ~NamedId() = default;

        NamedId(const NamedId&) = default;
        
        explicit NamedId(const t_Id& id)
            : t_Id(id)
            , m_name("")
        {}

        NamedId(const t_Id& id, AZStd::string_view name)
            : t_Id(id)
            , m_name(name)
        {}
        
        AZStd::string ToString() const
        {
            return AZStd::string::format("%s [%s]", m_name.data(), t_Id::ToString().data());
        }

        bool operator==(const NamedId& rhs) const
        {
            return t_Id::operator==(rhs);
        }

        bool operator==(const t_Id& rhs) const
        {
            return t_Id::operator==(rhs);
        }

        bool operator!=(const NamedId& rhs) const
        {
            return t_Id::operator!=(rhs);
        }

        bool operator!=(const t_Id& rhs) const
        {
            return t_Id::operator!=(rhs);
        }

        bool operator<(const NamedId& rhs) const
        {
            return t_Id::operator<(rhs);
        }

        bool operator<(const t_Id& rhs) const
        {
            return t_Id::operator<(rhs);
        }

        bool operator>(const NamedId& rhs) const
        {
            return t_Id::operator>(rhs);
        }

        bool operator>(const t_Id& rhs) const
        {
            return t_Id::operator>(rhs);
        }
    };
}   // namespace AZ

namespace AZStd
{
    template<typename t_Id>
    struct hash<ScriptCanvas::NamedId<t_Id>>
    {
        typedef ScriptCanvas::NamedId<t_Id> argument_type;
        typedef AZStd::size_t result_type;

        AZ_FORCE_INLINE size_t operator()(const argument_type& namedId) const
        {
            return AZStd::hash<t_Id>()(namedId);
        }
    };

} // namespace AZStd
