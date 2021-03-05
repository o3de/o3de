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

#include <Prefab/PrefabTestComponent.h>

namespace UnitTest
{
    void PrefabTestComponent::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = AZ::RttiCast<AZ::SerializeContext*>(reflection);

        if (serializeContext)
        {
            serializeContext->Class<PrefabTestComponent, AzToolsFramework::Components::EditorComponentBase>()->
                Field("BoolProperty", &PrefabTestComponent::m_boolProperty);
        }
    }

    PrefabTestComponent::PrefabTestComponent(bool boolProperty)
        : m_boolProperty(boolProperty)
    {
    }
}
