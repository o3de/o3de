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

#include <AzCore/Component/EntityId.h>
#include <ScriptCanvas/Core/Core.h>

namespace ScriptCanvas
{
    enum ExposeOption : AZ::s32
    {
        None = 0,
        ComponentInput = 1 << 0,
        ComponentOutput = 1 << 1
    };

    struct VariableId
    {
        AZ_TYPE_INFO(VariableId, "{CA57A57B-E510-4C09-B952-1F43742166AE}");
        AZ_CLASS_ALLOCATOR(VariableId, AZ::SystemAllocator, 0);

        VariableId() = default;

        VariableId(const VariableId&) = default;

        explicit VariableId(const AZ::Uuid& uniqueId)
            : m_id(uniqueId)
        {}

        //! AZ::Uuid has a constructor not marked as explicit that accepts a const char*
        //! Adding a constructor which accepts a const char* and deleting it prevents
        //! AZ::Uuid from being initialized with c-strings
        explicit VariableId(const char* str) = delete;

        static void Reflect(AZ::ReflectContext* context);
        static VariableId MakeVariableId();

        const AZ::Uuid& GetDatumId() const { return m_id; }

        bool IsValid() const
        {
            return !m_id.IsNull();
        }

        AZStd::string ToString() const
        {
            return m_id.ToString<AZStd::string>();
        }

        bool operator==(const VariableId& rhs) const
        {
            return m_id == rhs.m_id;
        }

        bool operator!=(const VariableId& rhs) const
        {
            return !operator==(rhs);
        }
        
        AZ::Uuid m_id{ AZ::Uuid::CreateNull() };
    };

    using NamedVariabledId = NamedId<VariableId>;
}

namespace AZStd
{
    template<>
    struct hash<ScriptCanvas::VariableId>
    {
        AZ_FORCE_INLINE size_t operator()(const ScriptCanvas::VariableId& ref) const
        {
            return AZStd::hash<AZ::Uuid>()(ref.GetDatumId());
        }
    };
}