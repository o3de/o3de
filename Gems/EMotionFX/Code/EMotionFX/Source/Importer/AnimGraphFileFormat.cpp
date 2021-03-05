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

#include "AnimGraphFileFormat.h"
#include <MCore/Source/Attribute.h>
#include <EMotionFX/Source/Parameter/BoolParameter.h>
#include <EMotionFX/Source/Parameter/ColorParameter.h>
#include <EMotionFX/Source/Parameter/FloatSliderParameter.h>
#include <EMotionFX/Source/Parameter/FloatSpinnerParameter.h>
#include <EMotionFX/Source/Parameter/IntSliderParameter.h>
#include <EMotionFX/Source/Parameter/IntSpinnerParameter.h>
#include <EMotionFX/Source/Parameter/TagParameter.h>
#include <EMotionFX/Source/Parameter/RotationParameter.h>
#include <EMotionFX/Source/Parameter/StringParameter.h>
#include <EMotionFX/Source/Parameter/Vector2Parameter.h>
#include <EMotionFX/Source/Parameter/Vector3GizmoParameter.h>
#include <EMotionFX/Source/Parameter/Vector3Parameter.h>
#include <EMotionFX/Source/Parameter/Vector4Parameter.h>

namespace EMotionFX
{
    namespace FileFormat
    {
        AZ::TypeId GetParameterTypeIdForInterfaceType(uint32 interfaceType)
        {
            switch (interfaceType)
            {
            case MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER:
                return azrtti_typeid<EMotionFX::FloatSpinnerParameter>();
            case MCore::ATTRIBUTE_INTERFACETYPE_FLOATSLIDER:
                return azrtti_typeid<EMotionFX::FloatSliderParameter>();
            case MCore::ATTRIBUTE_INTERFACETYPE_INTSPINNER:
                return azrtti_typeid<EMotionFX::IntSpinnerParameter>();
            case MCore::ATTRIBUTE_INTERFACETYPE_INTSLIDER:
                return azrtti_typeid<EMotionFX::IntSliderParameter>();
            case MCore::ATTRIBUTE_INTERFACETYPE_CHECKBOX:
                return azrtti_typeid<EMotionFX::BoolParameter>();
            case MCore::ATTRIBUTE_INTERFACETYPE_VECTOR2:
                return azrtti_typeid<EMotionFX::Vector2Parameter>();
            case MCore::ATTRIBUTE_INTERFACETYPE_VECTOR3GIZMO:
                return azrtti_typeid<EMotionFX::Vector3GizmoParameter>();
            case MCore::ATTRIBUTE_INTERFACETYPE_VECTOR4:
                return azrtti_typeid<EMotionFX::Vector4Parameter>();
            case MCore::ATTRIBUTE_INTERFACETYPE_COLOR:
                return azrtti_typeid<EMotionFX::ColorParameter>();
            case MCore::ATTRIBUTE_INTERFACETYPE_STRING:
                return azrtti_typeid<EMotionFX::StringParameter>();
            case MCore::ATTRIBUTE_INTERFACETYPE_VECTOR3:
                return azrtti_typeid<EMotionFX::Vector3Parameter>();
            case MCore::ATTRIBUTE_INTERFACETYPE_TAG:
                return azrtti_typeid<EMotionFX::TagParameter>();

            case MCore::ATTRIBUTE_INTERFACETYPE_COMBOBOX:
            case MCore::ATTRIBUTE_INTERFACETYPE_PROPERTYSET:
            case MCore::ATTRIBUTE_INTERFACETYPE_DEFAULT:
            default:
                break;
            }
            return AZ::TypeId();
        }

        uint32 GetInterfaceTypeForParameterTypeId(const AZ::TypeId& parameterTypeId)
        {
            if (parameterTypeId == azrtti_typeid<EMotionFX::FloatSpinnerParameter>())
            {
                return MCore::ATTRIBUTE_INTERFACETYPE_FLOATSPINNER;
            }
            else if (parameterTypeId == azrtti_typeid<EMotionFX::FloatSliderParameter>())
            {
                return MCore::ATTRIBUTE_INTERFACETYPE_FLOATSLIDER;
            }
            else if (parameterTypeId == azrtti_typeid<EMotionFX::IntSpinnerParameter>())
            {
                return MCore::ATTRIBUTE_INTERFACETYPE_INTSPINNER;
            }
            else if (parameterTypeId == azrtti_typeid<EMotionFX::IntSliderParameter>())
            {
                return MCore::ATTRIBUTE_INTERFACETYPE_INTSLIDER;
            }
            else if (parameterTypeId == azrtti_typeid<EMotionFX::BoolParameter>())
            {
                return MCore::ATTRIBUTE_INTERFACETYPE_CHECKBOX;
            }
            else if (parameterTypeId == azrtti_typeid<EMotionFX::Vector2Parameter>())
            {
                return MCore::ATTRIBUTE_INTERFACETYPE_VECTOR2;
            }
            else if (parameterTypeId == azrtti_typeid<EMotionFX::Vector3GizmoParameter>())
            {
                return MCore::ATTRIBUTE_INTERFACETYPE_VECTOR3GIZMO;
            }
            else if (parameterTypeId == azrtti_typeid<EMotionFX::Vector4Parameter>())
            {
                return MCore::ATTRIBUTE_INTERFACETYPE_VECTOR4;
            }
            else if (parameterTypeId == azrtti_typeid<EMotionFX::ColorParameter>())
            {
                return MCore::ATTRIBUTE_INTERFACETYPE_COLOR;
            }
            else if (parameterTypeId == azrtti_typeid<EMotionFX::StringParameter>())
            {
                return MCore::ATTRIBUTE_INTERFACETYPE_STRING;
            }
            else if (parameterTypeId == azrtti_typeid<EMotionFX::Vector3Parameter>())
            {
                return MCore::ATTRIBUTE_INTERFACETYPE_VECTOR3;
            }
            else if (parameterTypeId == azrtti_typeid<EMotionFX::TagParameter>())
            {
                return MCore::ATTRIBUTE_INTERFACETYPE_TAG;
            }
            return MCORE_INVALIDINDEX32;
        }
    }
} // namespace EMotionFX
