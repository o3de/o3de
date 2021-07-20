/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Prefab/PrefabTestComponent.h>

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
}
