/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if defined(CARBONATED)

#include <AzCore/Script/ScriptContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Script/lua/lua.h>
#include <AzCore/Script/ScriptProperty.h>
#include <AzCore/Script/ScriptPropertyEntityRef.h>

namespace AZ
{
    ////////////////////////////
    // ScriptPropertyEntityRef
    ////////////////////////////
    void ScriptPropertyEntityRef::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);

        if (serializeContext)
        {
            serializeContext->Class<AZ::ScriptPropertyEntityRef, AZ::ScriptProperty>()->
                Version(1)->
                Field("value", &AZ::ScriptPropertyEntityRef::m_value);
        }
    }

    AZ::TypeId ScriptPropertyEntityRef::GetDataTypeUuid() const
    {
        return AZ::SerializeTypeInfo<AZ::EntityId>::GetUuid();
    }

    bool ScriptPropertyEntityRef::DoesTypeMatch(AZ::ScriptDataContext& context, int valueIndex) const
    {
        return context.IsRegisteredClass(valueIndex);
    }

    AZ::ScriptPropertyEntityRef* ScriptPropertyEntityRef::Clone(const char* name) const
    {
        AZ::ScriptPropertyEntityRef* clonedValue = aznew AZ::ScriptPropertyEntityRef(name ? name : m_name.c_str());
        clonedValue->m_value = m_value;
        return clonedValue;
    }

    bool ScriptPropertyEntityRef::Write(AZ::ScriptContext& context)
    {
        AZ::ScriptValue<AZ::EntityId>::StackPush(context.NativeContext(), m_value);
        return true;
    }

    void ScriptPropertyEntityRef::CloneDataFrom(const AZ::ScriptProperty* scriptProperty)
    {
        const AZ::ScriptPropertyEntityRef* entityProperty = azrtti_cast<const AZ::ScriptPropertyEntityRef*>(scriptProperty);

        AZ_Error("ScriptPropertyEntityRef", entityProperty, "Invalid call to CloneData. Types must match before clone attempt is made.\n");
        if (entityProperty)
        {
            m_value = entityProperty->m_value;
        }
    }
}

#endif
