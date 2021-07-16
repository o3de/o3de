/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <EMotionFX/Source/Parameter/ParameterFactory.h>
#include <EMotionFX/Source/Parameter/BoolParameter.h>
#include <EMotionFX/Source/Parameter/ColorParameter.h>
#include <EMotionFX/Source/Parameter/FloatParameter.h>
#include <EMotionFX/Source/Parameter/FloatSliderParameter.h>
#include <EMotionFX/Source/Parameter/FloatSpinnerParameter.h>
#include <EMotionFX/Source/Parameter/GroupParameter.h>
#include <EMotionFX/Source/Parameter/IntParameter.h>
#include <EMotionFX/Source/Parameter/IntSliderParameter.h>
#include <EMotionFX/Source/Parameter/IntSpinnerParameter.h>
#include <EMotionFX/Source/Parameter/Parameter.h>
#include <EMotionFX/Source/Parameter/RotationParameter.h>
#include <EMotionFX/Source/Parameter/StringParameter.h>
#include <EMotionFX/Source/Parameter/TagParameter.h>
#include <EMotionFX/Source/Parameter/ValueParameter.h>
#include <EMotionFX/Source/Parameter/Vector2Parameter.h>
#include <EMotionFX/Source/Parameter/Vector3GizmoParameter.h>
#include <EMotionFX/Source/Parameter/Vector3Parameter.h>
#include <EMotionFX/Source/Parameter/Vector4Parameter.h>


namespace EMotionFX
{
    void ParameterFactory::ReflectParameterTypes(AZ::ReflectContext* context)
    {
        Parameter::Reflect(context);
        GroupParameter::Reflect(context);
        ValueParameter::Reflect(context);
        BoolParameter::Reflect(context);
        ColorParameter::Reflect(context);
        FloatParameter::Reflect(context);
        FloatSliderParameter::Reflect(context);
        FloatSpinnerParameter::Reflect(context);
        IntParameter::Reflect(context);
        IntSliderParameter::Reflect(context);
        IntSpinnerParameter::Reflect(context);
        RotationParameter::Reflect(context);
        StringParameter::Reflect(context);
        TagParameter::Reflect(context);
        Vector2Parameter::Reflect(context);
        Vector3Parameter::Reflect(context);
        Vector3GizmoParameter::Reflect(context);
        Vector4Parameter::Reflect(context);
    }

    AZStd::vector<AZ::TypeId> ParameterFactory::GetValueParameterTypes()
    {
        return
        {
            azrtti_typeid<FloatSliderParameter>(),
            azrtti_typeid<FloatSpinnerParameter>(),
            azrtti_typeid<BoolParameter>(),
            azrtti_typeid<TagParameter>(),
            azrtti_typeid<IntSliderParameter>(),
            azrtti_typeid<IntSpinnerParameter>(),
            azrtti_typeid<Vector2Parameter>(),
            azrtti_typeid<Vector3Parameter>(),
            azrtti_typeid<Vector3GizmoParameter>(),
            azrtti_typeid<Vector4Parameter>(),
            azrtti_typeid<StringParameter>(),
            azrtti_typeid<ColorParameter>(),
            azrtti_typeid<RotationParameter>()
        };
    }

    AZStd::vector<AZ::TypeId> ParameterFactory::GetParameterTypes()
    {
        AZStd::vector<AZ::TypeId> result = GetValueParameterTypes();
        result.push_back(azrtti_typeid<GroupParameter>());
        return result;
    }

    Parameter* ParameterFactory::Create(const AZ::TypeId& type)
    {
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!context)
        {
            AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
            return nullptr;
        }

        const AZ::SerializeContext::ClassData* classData = context->FindClassData(type);
        void* parameterInstance = classData->m_factory->Create(classData->m_name);
        return reinterpret_cast<Parameter*>(parameterInstance);
    }
}
