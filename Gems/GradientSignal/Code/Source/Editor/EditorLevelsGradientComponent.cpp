/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorLevelsGradientComponent.h"

namespace GradientSignal
{
    void EditorLevelsGradientComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorGradientComponentBase::ReflectSubClass<EditorLevelsGradientComponent, BaseClassType>(context);
    }
}
