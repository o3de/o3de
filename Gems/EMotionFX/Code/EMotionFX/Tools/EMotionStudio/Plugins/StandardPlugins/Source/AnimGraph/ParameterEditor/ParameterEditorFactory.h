/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
