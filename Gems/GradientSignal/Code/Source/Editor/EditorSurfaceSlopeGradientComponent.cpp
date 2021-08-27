/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorSurfaceSlopeGradientComponent.h"

namespace GradientSignal
{
    void EditorSurfaceSlopeGradientComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorGradientComponentBase::ReflectSubClass<EditorSurfaceSlopeGradientComponent, BaseClassType>(context);
    }
}
