/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GradientSignal_precompiled.h"
#include "EditorDitherGradientComponent.h"

namespace GradientSignal
{
    void EditorDitherGradientComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorGradientComponentBase::ReflectSubClass<EditorDitherGradientComponent, BaseClassType>(context);
    }

    AZ::u32 EditorDitherGradientComponent::ConfigurationChanged()
    {
        BaseClassType::ConfigurationChanged();
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }
}
