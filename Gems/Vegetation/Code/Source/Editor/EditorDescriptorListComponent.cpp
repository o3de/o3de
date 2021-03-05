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

#include "Vegetation_precompiled.h"
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