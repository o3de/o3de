/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorSurfaceAltitudeFilterComponent.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>

namespace Vegetation
{
    void EditorSurfaceAltitudeFilterComponent::Reflect(AZ::ReflectContext* context)
    {
        ReflectSubClass<DerivedClassType, BaseClassType>(context, 1, &EditorVegetationComponentBaseVersionConverter<typename BaseClassType::WrappedComponentType, typename BaseClassType::WrappedConfigType>);
    }

    AZ::u32 EditorSurfaceAltitudeFilterComponent::ConfigurationChanged()
    {
        BaseClassType::ConfigurationChanged();
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }
}
