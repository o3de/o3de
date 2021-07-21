/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDescriptorListComponent.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>

namespace Vegetation
{
    void EditorDescriptorListComponent::Reflect(AZ::ReflectContext* context)
    {
        ReflectSubClass<EditorDescriptorListComponent, BaseClassType>(context, 1, &EditorVegetationComponentBaseVersionConverter<typename BaseClassType::WrappedComponentType, typename BaseClassType::WrappedConfigType>);
    }

    void EditorDescriptorListComponent::Init()
    {
        ForceOneEntry();
        BaseClassType::Init();
    }

    void EditorDescriptorListComponent::Activate()
    {
        ForceOneEntry();
        BaseClassType::Activate();
    }

    AZ::u32 EditorDescriptorListComponent::ConfigurationChanged()
    {
        ForceOneEntry();
        return BaseClassType::ConfigurationChanged();
    }

    void EditorDescriptorListComponent::ForceOneEntry()
    {
        if (m_configuration.m_descriptors.empty())
        {
            m_configuration.m_descriptors.push_back();
            SetDirty();
        }
    }
}
