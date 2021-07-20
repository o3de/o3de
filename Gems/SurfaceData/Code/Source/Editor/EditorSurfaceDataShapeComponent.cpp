/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SurfaceData_precompiled.h"
#include "EditorSurfaceDataShapeComponent.h"
#include <AzCore/Serialization/Utils.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

namespace SurfaceData
{
    void EditorSurfaceDataShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClassType::ReflectSubClass<EditorSurfaceDataShapeComponent, BaseClassType>(context, 2, &LmbrCentral::EditorWrappedComponentBaseVersionConverter<typename BaseClassType::WrappedComponentType, typename BaseClassType::WrappedConfigType,2>);
    }
}
