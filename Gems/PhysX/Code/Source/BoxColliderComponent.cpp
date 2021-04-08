
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

#include <PhysX_precompiled.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <Source/BoxColliderComponent.h>
#include <Source/Utils.h>

namespace PhysX
{
    void BoxColliderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<BoxColliderComponent, BaseColliderComponent>()
                ->Version(1)
                ;
        }
    }

    // BaseColliderComponent
    void BoxColliderComponent::UpdateScaleForShapeConfigs()
    {
        if (m_shapeConfigList.size() != 1)
        {
            AZ_Error("PhysX Box Collider Component", false, 
                "Expected exactly one collider/shape configuration for entity \"%s\".", GetEntity()->GetName().c_str());
            return;
        }

        m_shapeConfigList[0].second->m_scale = Utils::GetTransformScale(GetEntityId());
    }
}
