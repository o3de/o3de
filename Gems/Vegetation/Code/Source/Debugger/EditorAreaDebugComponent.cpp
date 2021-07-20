/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorAreaDebugComponent.h"

#include <AzCore/Serialization/Utils.h>
#include <AzCore/base.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

#include <Vegetation/Ebuses/AreaSystemRequestBus.h>
#include <Vegetation/Ebuses/InstanceSystemRequestBus.h>
#include <VegetationSystemComponent.h>
#include <Debugger/DebugComponent.h>

namespace Vegetation
{
    void EditorAreaDebugComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorVegetationComponentBase::ReflectSubClass<EditorAreaDebugComponent, BaseClassType>(context);
    }
}
