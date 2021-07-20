/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorBlockerComponent.h"
#include <AzCore/Serialization/Utils.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>

namespace Vegetation
{
    void EditorBlockerComponent::Reflect(AZ::ReflectContext* context)
    {
        ReflectSubClass<DerivedClassType, BaseClassType>(context, 1, &EditorAreaComponentBaseVersionConverter<typename BaseClassType::WrappedComponentType, typename BaseClassType::WrappedConfigType>);
    }
}
