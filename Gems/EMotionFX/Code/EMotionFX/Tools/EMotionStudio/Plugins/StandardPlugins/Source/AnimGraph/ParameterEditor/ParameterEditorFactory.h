/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>

namespace AZ
{
    class ReflectContext;
}
namespace EMotionFX
{
    class AnimGraph;
    class ValueParameter;
}
namespace MCore
{
    class Attribute;
}

namespace EMStudio
{
    class ValueParameterEditor;

    class ParameterEditorFactory
    {
    public:
        static void ReflectParameterEditorTypes(AZ::ReflectContext* context);

        static ValueParameterEditor* Create(EMotionFX::AnimGraph* animGraph,
            const EMotionFX::ValueParameter* valueParameter,
            const AZStd::vector<MCore::Attribute*>& attributes);
    };
}
