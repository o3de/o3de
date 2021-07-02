/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorSurfaceSlopeFilterComponent.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>

namespace Vegetation
{
    void EditorSurfaceSlopeFilterComponent::Reflect(AZ::ReflectContext* context)
    {
        ReflectSubClass<EditorSurfaceSlopeFilterComponent, BaseClassType>(context, 1, &EditorVegetationComponentBaseVersionConverter<typename BaseClassType::WrappedComponentType, typename BaseClassType::WrappedConfigType>);
    }
}
