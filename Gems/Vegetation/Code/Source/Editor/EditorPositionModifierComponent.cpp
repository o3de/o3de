/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorPositionModifierComponent.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>

namespace Vegetation
{
    void EditorPositionModifierComponent::Reflect(AZ::ReflectContext* context)
    {
        ReflectSubClass<EditorPositionModifierComponent, BaseClassType>(context, 1, &EditorVegetationComponentBaseVersionConverter<typename BaseClassType::WrappedComponentType, typename BaseClassType::WrappedConfigType>);
    }

    void EditorPositionModifierComponent::Activate()
    {
        m_configuration.m_gradientSamplerX.m_ownerEntityId = GetEntityId();
        m_configuration.m_gradientSamplerY.m_ownerEntityId = GetEntityId();
        m_configuration.m_gradientSamplerZ.m_ownerEntityId = GetEntityId();

        BaseClassType::Activate();
    }
}
