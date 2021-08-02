/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorAreaBlenderComponent.h"
#include <AzCore/Serialization/Utils.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>
#include <GradientSignal/Ebuses/SectorDataRequestBus.h>
#include <Vegetation/Ebuses/AreaSystemRequestBus.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>

namespace Vegetation
{
    void EditorAreaBlenderComponent::Reflect(AZ::ReflectContext* context)
    {
        ReflectSubClass<DerivedClassType, BaseClassType>(context, 1, &EditorAreaComponentBaseVersionConverter<typename BaseClassType::WrappedComponentType, typename BaseClassType::WrappedConfigType>);
    }

    void EditorAreaBlenderComponent::Init()
    {
        ForceOneEntry();
        BaseClassType::Init();
    }

    void EditorAreaBlenderComponent::Activate()
    {
        ForceOneEntry();
        BaseClassType::Activate();
    }

    AZ::u32 EditorAreaBlenderComponent::ConfigurationChanged()
    {
        ForceOneEntry();
        return BaseClassType::ConfigurationChanged();
    }

    void EditorAreaBlenderComponent::ForceOneEntry()
    {
        if (m_configuration.m_vegetationAreaIds.empty())
        {
            m_configuration.m_vegetationAreaIds.push_back();
            SetDirty();
        }
    }
}
