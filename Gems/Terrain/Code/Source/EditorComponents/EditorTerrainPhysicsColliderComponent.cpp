/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorComponents/EditorTerrainPhysicsColliderComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace Terrain
{
    void EditorTerrainPhysicsColliderComponent::Reflect(AZ::ReflectContext* context)
    {
        // Call ReflectSubClass in EditorWrappedComponentBase to handle all the boilerplate reflection.
        BaseClassType::ReflectSubClass<EditorTerrainPhysicsColliderComponent, BaseClassType>(
            context, 1,
            &LmbrCentral::EditorWrappedComponentBaseVersionConverter<typename BaseClassType::WrappedComponentType,
            typename BaseClassType::WrappedConfigType, 1>
        );
    }
}
