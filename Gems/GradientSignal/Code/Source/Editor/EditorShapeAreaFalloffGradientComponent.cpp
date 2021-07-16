/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorShapeAreaFalloffGradientComponent.h"

namespace GradientSignal
{
    void EditorShapeAreaFalloffGradientComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorGradientComponentBase::ReflectSubClass<EditorShapeAreaFalloffGradientComponent, BaseClassType>(context);
    }
}
