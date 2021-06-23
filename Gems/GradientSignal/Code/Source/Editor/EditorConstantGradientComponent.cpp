/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GradientSignal_precompiled.h"
#include "EditorConstantGradientComponent.h"

namespace GradientSignal
{
    void EditorConstantGradientComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorWrappedComponentBase::ReflectSubClass<EditorConstantGradientComponent, BaseClassType>(context);
    }
}
