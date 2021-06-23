/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GradientSignal_precompiled.h"
#include "EditorSmoothStepGradientComponent.h"

namespace GradientSignal
{
    void EditorSmoothStepGradientComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorGradientComponentBase::ReflectSubClass<EditorSmoothStepGradientComponent, BaseClassType>(context);
    }
}
