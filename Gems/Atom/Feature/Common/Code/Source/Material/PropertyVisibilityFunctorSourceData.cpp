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

#include "PropertyVisibilityFunctorSourceData.h"
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroupLayout.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/RPI.Edit/Material/MaterialUtils.h>

namespace AZ
{
    namespace Render
    {
        void PropertyVisibilityFunctorSourceData::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ActionSourceData>()
                    ->Version(1)
                    ->Field("triggerProperty", &ActionSourceData::m_triggerPropertyName)
                    ->Field("triggerValue", &ActionSourceData::m_triggerValue)
                    ->Field("visibility", &ActionSourceData::m_visibility)
                    ;
                serializeContext->Class<PropertyVisibilityFunctorSourceData>()
                    ->Version(2)
                    ->Field("actions", &PropertyVisibilityFunctorSourceData::m_actions)
                    ->Field("affectedProperties", &PropertyVisibilityFunctorSourceData::m_affectedPropertyNames)
                    ;
            }
        }

        RPI::MaterialFunctorSourceData::FunctorResult PropertyVisibilityFunctorSourceData::CreateFunctor(const EditorContext& context) const
        {
            using namespace RPI;

            RPI::Ptr<PropertyVisibilityFunctor> functor = aznew PropertyVisibilityFunctor;

            functor->m_actions.reserve(m_actions.size());

            for (const auto& actionSource : m_actions)
            {
                functor->m_actions.emplace_back();
                PropertyVisibilityFunctor::Action& action = functor->m_actions.back();
                action.m_triggerPropertyIndex = context.FindMaterialPropertyIndex(AZ::Name{ actionSource.m_triggerPropertyName });
                if (action.m_triggerPropertyIndex.IsNull())
                {
                    return Failure();
                }
                AddMaterialPropertyDependency(functor, action.m_triggerPropertyIndex);

                if (!actionSource.m_triggerValue.Resolve(*context.GetMaterialPropertiesLayout(), Name{ actionSource.m_triggerPropertyName }))
                {
                    // Error is reported in Resolve().
                    return Failure();
                }

                const MaterialPropertyDescriptor* propertyDescriptor = context.GetMaterialPropertiesLayout()->GetPropertyDescriptor(action.m_triggerPropertyIndex);
                // Enum type should resolve further to a unit32_t from the string source.
                if (propertyDescriptor->GetDataType() == RPI::MaterialPropertyDataType::Enum)
                {
                    if (!RPI::MaterialUtils::ResolveMaterialPropertyEnumValue(
                        propertyDescriptor,
                        Name(actionSource.m_triggerValue.GetValue().GetValue<AZStd::string>()),
                        action.m_triggerValue))
                    {
                        return Failure();
                    }
                }
                else
                {
                    action.m_triggerValue = actionSource.m_triggerValue.GetValue();
                }

                action.m_visibility = actionSource.m_visibility;
            }

            functor->m_affectedProperties.reserve(m_affectedPropertyNames.size());
            for (const AZStd::string& name : m_affectedPropertyNames)
            {
                RPI::MaterialPropertyIndex index = context.FindMaterialPropertyIndex(AZ::Name{ name });
                if (index.IsNull())
                {
                    return Failure();
                }
                functor->m_affectedProperties.push_back(index);
            }

            return Success(RPI::Ptr<MaterialFunctor>(functor));
        }
    } // namespace Render
} // namespace AZ
