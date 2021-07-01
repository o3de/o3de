/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GradientSignal_precompiled.h"
#include "EditorShapeAreaFalloffGradientComponent.h"

namespace GradientSignal
{
    void EditorShapeAreaFalloffGradientComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorGradientComponentBase::ReflectSubClass<EditorShapeAreaFalloffGradientComponent, BaseClassType>(context);
    }
}
