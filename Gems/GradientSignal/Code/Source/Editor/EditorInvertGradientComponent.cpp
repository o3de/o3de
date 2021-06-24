/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GradientSignal_precompiled.h"
#include "EditorInvertGradientComponent.h"

namespace GradientSignal
{
    void EditorInvertGradientComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorGradientComponentBase::ReflectSubClass<EditorInvertGradientComponent, BaseClassType>(context);
    }
}
