
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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

        m_shapeConfigList[0].second->m_scale = AZ::Vector3(Utils::GetTransformScale(GetEntityId()));
    }
}
