/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "DrawListFunctorSourceData.h"
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroupLayout.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/RPI.Edit/Material/MaterialUtils.h>

namespace AZ
{
    namespace Render
    {
        void DrawListFunctorSourceData::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<DrawListFunctorSourceData>()
                    ->Version(2)
                    ->Field("triggerProperty", &DrawListFunctorSourceData::m_triggerPropertyName)
                    ->Field("triggerValue", &DrawListFunctorSourceData::m_triggerPropertyValue)
                    ->Field("shaderIndex", &DrawListFunctorSourceData::m_shaderItemIndex)
                    ->Field("drawList", &DrawListFunctorSourceData::m_drawListName)
                    ;
            }
        }

        RPI::MaterialFunctorSourceData::FunctorResult DrawListFunctorSourceData::CreateFunctor(const RuntimeContext& context) const
        {
            using namespace RPI;

            Ptr<DrawListFunctor> functor = aznew DrawListFunctor;

            functor->m_triggerPropertyIndex = context.FindMaterialPropertyIndex(Name{ m_triggerPropertyName });
            if (functor->m_triggerPropertyIndex.IsNull())
            {
                return Failure();
            }
            AddMaterialPropertyDependency(functor, functor->m_triggerPropertyIndex);

            if (!context.CheckShaderIndexValid(m_shaderItemIndex))
            {
                return Failure();
            }

            if (!m_triggerPropertyValue.Resolve(*context.GetMaterialPropertiesLayout(), Name{m_triggerPropertyName}))
            {
                // Error is reported in Resolve().
                return Failure();
            }

            const MaterialPropertyDescriptor* propertyDescriptor = context.GetMaterialPropertiesLayout()->GetPropertyDescriptor(functor->m_triggerPropertyIndex);
            // Enum type should resolve further to a unit32_t from the string source.
            if (propertyDescriptor->GetDataType() == RPI::MaterialPropertyDataType::Enum)
            {
                if (!RPI::MaterialUtils::ResolveMaterialPropertyEnumValue(
                    propertyDescriptor,
                    Name(m_triggerPropertyValue.GetValue().GetValue<AZStd::string>()),
                    functor->m_triggerValue))
                {
                    return Failure();
                }
            }
            else
            {
                functor->m_triggerValue = m_triggerPropertyValue.GetValue();
            }

            functor->m_shaderItemIndex = m_shaderItemIndex;
            functor->m_drawListName = m_drawListName;

            return Success(Ptr<MaterialFunctor>(functor));
        }
    } // namespace Render
} // namespace AZ
