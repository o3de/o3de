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
#include "EditorAreaBlenderComponent.h"
#include <AzCore/Serialization/Utils.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>
#include <MathConversion.h>
#include <IRenderAuxGeom.h>
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
