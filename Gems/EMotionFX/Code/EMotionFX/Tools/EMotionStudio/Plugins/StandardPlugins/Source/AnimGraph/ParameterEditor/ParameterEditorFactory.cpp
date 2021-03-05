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

#include "ParameterEditorFactory.h"

#include <AzCore/std/containers/unordered_map.h>
#include <MCore/Source/StaticAllocator.h>
#include <EMotionFX/Source/Parameter/BoolParameter.h>
#include <EMotionFX/Source/Parameter/ColorParameter.h>
#include <EMotionFX/Source/Parameter/FloatSliderParameter.h>
#include <EMotionFX/Source/Parameter/FloatSpinnerParameter.h>
#include <EMotionFX/Source/Parameter/IntSliderParameter.h>
#include <EMotionFX/Source/Parameter/IntSpinnerParameter.h>
#include <EMotionFX/Source/Parameter/RotationParameter.h>
#include <EMotionFX/Source/Parameter/StringParameter.h>
#include <EMotionFX/Source/Parameter/TagParameter.h>
#include <EMotionFX/Source/Parameter/ValueParameter.h>
#include <EMotionFX/Source/Parameter/Vector2Parameter.h>
#include <EMotionFX/Source/Parameter/Vector3GizmoParameter.h>
#include <EMotionFX/Source/Parameter/Vector3Parameter.h>
#include <EMotionFX/Source/Parameter/Vector4Parameter.h>

#include "BoolParameterEditor.h"
#include "ColorParameterEditor.h"
#include "FloatSliderParameterEditor.h"
#include "FloatSpinnerParameterEditor.h"
#include "IntSliderParameterEditor.h"
#include "IntSpinnerParameterEditor.h"
#include "RotationParameterEditor.h"
#include "StringParameterEditor.h"
#include "TagParameterEditor.h"
#include "ValueParameterEditor.h"
#include "Vector2ParameterEditor.h"
#include "Vector3GizmoParameterEditor.h"
#include "Vector3ParameterEditor.h"
#include "Vector4ParameterEditor.h"

namespace EMStudio
{
    void ParameterEditorFactory::ReflectParameterEditorTypes(AZ::ReflectContext* context)
    {
        ValueParameterEditor::Reflect(context);
        BoolParameterEditor::Reflect(context);
        ColorParameterEditor::Reflect(context);
        FloatSliderParameterEditor::Reflect(context);
        FloatSpinnerParameterEditor::Reflect(context);
        IntSliderParameterEditor::Reflect(context);
        IntSpinnerParameterEditor::Reflect(context);
        RotationParameterEditor::Reflect(context);
        StringParameterEditor::Reflect(context);
        TagParameterEditor::Reflect(context);
        Vector2ParameterEditor::Reflect(context);
        Vector3GizmoParameterEditor::Reflect(context);
        Vector3ParameterEditor::Reflect(context);
        Vector4ParameterEditor::Reflect(context);
    }

    ValueParameterEditor* ParameterEditorFactory::Create(EMotionFX::AnimGraph* animGraph,
        const EMotionFX::ValueParameter* valueParameter,
        const AZStd::vector<MCore::Attribute*>& attributes)
    {
#define EMSTUDIO_PARAMETER_EDITOR_CREATE_ENTRY(TYPE)                                                                                             \
    { azrtti_typeid<EMotionFX::TYPE ## Parameter>(),                                                                                             \
      [](EMotionFX::AnimGraph* animGraph, const EMotionFX::ValueParameter* valueParameter, const AZStd::vector<MCore::Attribute*>& attributes) { \
          return aznew TYPE ## ParameterEditor(animGraph, valueParameter, attributes); }                                                         \
    }

        static AZStd::unordered_map<AZ::TypeId, AZStd::function<ValueParameterEditor*(EMotionFX::AnimGraph*, const EMotionFX::ValueParameter*, const AZStd::vector<MCore::Attribute*>&)>, AZStd::hash<AZ::TypeId>, AZStd::equal_to<AZ::TypeId>, MCore::StaticAllocator > creationFunctionByParameterType = {
            EMSTUDIO_PARAMETER_EDITOR_CREATE_ENTRY(Bool),
            EMSTUDIO_PARAMETER_EDITOR_CREATE_ENTRY(Color),
            EMSTUDIO_PARAMETER_EDITOR_CREATE_ENTRY(FloatSlider),
            EMSTUDIO_PARAMETER_EDITOR_CREATE_ENTRY(FloatSpinner),
            EMSTUDIO_PARAMETER_EDITOR_CREATE_ENTRY(IntSlider),
            EMSTUDIO_PARAMETER_EDITOR_CREATE_ENTRY(IntSpinner),
            EMSTUDIO_PARAMETER_EDITOR_CREATE_ENTRY(Rotation),
            EMSTUDIO_PARAMETER_EDITOR_CREATE_ENTRY(String),
            EMSTUDIO_PARAMETER_EDITOR_CREATE_ENTRY(Tag),
            EMSTUDIO_PARAMETER_EDITOR_CREATE_ENTRY(Vector2),
            EMSTUDIO_PARAMETER_EDITOR_CREATE_ENTRY(Vector3Gizmo),
            EMSTUDIO_PARAMETER_EDITOR_CREATE_ENTRY(Vector3),
            EMSTUDIO_PARAMETER_EDITOR_CREATE_ENTRY(Vector4)
        };
#undef EMSTUDIO_PARAMETER_EDITOR_CREATE_ENTRY

        const AZ::TypeId paramType = azrtti_typeid(valueParameter);
        auto foundIt = creationFunctionByParameterType.find(paramType);
        AZ_Assert(foundIt != creationFunctionByParameterType.end(), "Expected to find a parameter editor creation function for all parameter types");

        return foundIt->second(animGraph, valueParameter, attributes);
    }
}
