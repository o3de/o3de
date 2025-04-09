/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Prefab/PrefabTestComponent.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace UnitTest
{
    void PrefabTestComponent::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = AZ::RttiCast<AZ::SerializeContext*>(reflection);

        if (serializeContext)
        {
            serializeContext->Class<PrefabTestComponent, AzToolsFramework::Components::EditorComponentBase>()->
                Field("BoolProperty", &PrefabTestComponent::m_boolProperty)->
                Field("IntProperty", &PrefabTestComponent::m_intProperty)->
                Field("EntityReferenceProperty", &PrefabTestComponent::m_entityIdProperty);
        }
    }

    PrefabTestComponent::PrefabTestComponent(bool boolProperty)
        : m_boolProperty(boolProperty)
    {
    }

    void PrefabTestComponentWithUnReflectedTypeMember::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = AZ::RttiCast<AZ::SerializeContext*>(reflection);

        // We reflect our member but not its type this will result in missing reflection data
        // when we try to store or load this field
        if (serializeContext)
        {
            serializeContext->Class<PrefabTestComponentWithUnReflectedTypeMember, AzToolsFramework::Components::EditorComponentBase>()
                ->Field("UnReflectedType", &PrefabTestComponentWithUnReflectedTypeMember::m_unReflectedType)
                ->Field("ReflectedType", &PrefabTestComponentWithUnReflectedTypeMember::m_reflectedType);
        }
    }

    void PrefabNonEditorComponent::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = AZ::RttiCast<AZ::SerializeContext*>(reflection);

        if (serializeContext)
        {
            serializeContext->Class<PrefabNonEditorComponent, AZ::Component>()
                ->Field("IntProperty", &PrefabNonEditorComponent::m_intProperty);
        }
    }

    void PrefabNonEditorComponent::Activate()
    {
    }

    void PrefabNonEditorComponent::Deactivate()
    {
    }
}
