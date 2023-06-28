/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if defined(CARBONATED)

#include <AzCore/Component/EntityId.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/BehaviorObjectSignals.h>
#include <AzCore/Script/ScriptAsset.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Script/ScriptPropertyWatcherBus.h>
#include <AzCore/Serialization/DynamicSerializableField.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/set.h>

// Ideally, the properties would be able to marshal themselves
// to avoid needing this. But until then, in order to maintain
// data integrity, need to friend class in this guy.
namespace AzFramework
{
    class ScriptPropertyMarshaler;
}

namespace AzToolsFramework
{
    namespace Components
    {
        class ScriptEditorComponent;
    }
}

namespace AZ
{
    class ScriptPropertyEntityRef
        : public ScriptProperty
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptPropertyEntityRef, AZ::SystemAllocator, 0);
        AZ_RTTI(AZ::ScriptPropertyEntityRef, "{68EDE6C3-0A89-4C50-A86E-06C058C9F862}", ScriptProperty);
        
        static void Reflect(AZ::ReflectContext* reflection);

        ScriptPropertyEntityRef() {}
        ScriptPropertyEntityRef(const char* name)
            : ScriptProperty(name) {}
        virtual ~ScriptPropertyEntityRef() = default;
        const void* GetDataAddress() const override { return &m_value; }
        AZ::TypeId GetDataTypeUuid() const override;

        bool DoesTypeMatch(AZ::ScriptDataContext& context, int valueIndex) const override;

        ScriptPropertyEntityRef* Clone(const char* name = nullptr) const override;

        bool Write(AZ::ScriptContext& context) override;

        AZ::EntityId m_value;

    protected:
        void CloneDataFrom(const AZ::ScriptProperty* scriptProperty) override;
    };
}

#endif
