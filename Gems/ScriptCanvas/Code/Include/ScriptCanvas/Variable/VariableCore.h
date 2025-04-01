/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        AZ_CLASS_ALLOCATOR(VariableId, AZ::SystemAllocator);

        VariableId() = default;

        VariableId(const VariableId&) = default;

        AZ_INLINE explicit VariableId(const AZ::Uuid& uniqueId)
            : m_id(uniqueId)
        {}

        //! AZ::Uuid has a constructor not marked as explicit that accepts a const char*
        //! Adding a constructor which accepts a const char* and deleting it prevents
        //! AZ::Uuid from being initialized with c-strings
        explicit VariableId(const char* str) = delete;

        static void Reflect(AZ::ReflectContext* context);
        static VariableId MakeVariableId();

        AZ_INLINE const AZ::Uuid& GetDatumId() const { return m_id; }

        AZ_INLINE bool IsValid() const
        {
            return !m_id.IsNull();
        }

        AZ_INLINE AZStd::string ToString() const
        {
            return m_id.ToString<AZStd::string>();
        }

        AZ_INLINE bool operator==(const VariableId& rhs) const
        {
            return m_id == rhs.m_id;
        }

        AZ_INLINE bool operator!=(const VariableId& rhs) const
        {
            return !operator==(rhs);
        }

        AZ_INLINE bool operator<(const VariableId& rhs) const
        {
            return m_id < rhs.m_id;
        }
        
        AZ_INLINE bool operator>(const VariableId& rhs) const
        {
            return m_id > rhs.m_id;
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
