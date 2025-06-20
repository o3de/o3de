/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "./Transform2DFunctorSourceData.h"
#include <Atom/RHI.Reflect/ShaderResourceGroupLayout.h>
#include <Atom/RPI.Reflect/Material/MaterialNameContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace Render
    {
        void Transform2DFunctorSourceData::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<Transform2DFunctorSourceData, RPI::MaterialFunctorSourceData>()
                    ->Version(4) // added base class
                    ->Field("transformOrder", &Transform2DFunctorSourceData::m_transformOrder)
                    ->Field("centerProperty", &Transform2DFunctorSourceData::m_center)
                    ->Field("scaleProperty", &Transform2DFunctorSourceData::m_scale)
                    ->Field("scaleXProperty", &Transform2DFunctorSourceData::m_scaleX)
                    ->Field("scaleYProperty", &Transform2DFunctorSourceData::m_scaleY)
                    ->Field("translateXProperty", &Transform2DFunctorSourceData::m_translateX)
                    ->Field("translateYProperty", &Transform2DFunctorSourceData::m_translateY)
                    ->Field("rotateDegreesProperty", &Transform2DFunctorSourceData::m_rotateDegrees)
                    ->Field("float3x3ShaderInput", &Transform2DFunctorSourceData::m_transformMatrix)
                    ->Field("float3x3InverseShaderInput", &Transform2DFunctorSourceData::m_transformMatrixInverse);
            }
        }

        RPI::MaterialFunctorSourceData::FunctorResult Transform2DFunctorSourceData::CreateFunctor(const RuntimeContext& context) const
        {
            using namespace RPI;

            RPI::Ptr<Transform2DFunctor> functor = aznew Transform2DFunctor;

            functor->m_center        = context.FindMaterialPropertyIndex(Name{ m_center });
            functor->m_scale         = context.FindMaterialPropertyIndex(Name{ m_scale });
            functor->m_scaleX        = context.FindMaterialPropertyIndex(Name{ m_scaleX });
            functor->m_scaleY        = context.FindMaterialPropertyIndex(Name{ m_scaleY });
            functor->m_translateX    = context.FindMaterialPropertyIndex(Name{ m_translateX });
            functor->m_translateY    = context.FindMaterialPropertyIndex(Name{ m_translateY });
            functor->m_rotateDegrees = context.FindMaterialPropertyIndex(Name{ m_rotateDegrees });

            if (functor->m_center.IsNull() || functor->m_scale.IsNull() || functor->m_scaleX.IsNull() || functor->m_scaleY.IsNull() ||
                functor->m_translateX.IsNull() || functor->m_translateY.IsNull() || functor->m_rotateDegrees.IsNull())
            {
                return Failure();
            }

            AddMaterialPropertyDependency(functor, functor->m_center);
            AddMaterialPropertyDependency(functor, functor->m_scale);
            AddMaterialPropertyDependency(functor, functor->m_scaleX);
            AddMaterialPropertyDependency(functor, functor->m_scaleY);
            AddMaterialPropertyDependency(functor, functor->m_translateX);
            AddMaterialPropertyDependency(functor, functor->m_translateY);
            AddMaterialPropertyDependency(functor, functor->m_rotateDegrees);

            functor->m_transformMatrix = MaterialShaderParameterNameIndex{ m_transformMatrix, context.GetNameContext() };
            if (m_transformMatrixInverse.empty() == false)
            {
                functor->m_transformMatrixInverse = MaterialShaderParameterNameIndex{ m_transformMatrixInverse, context.GetNameContext() };
            }
            functor->m_transformOrder = m_transformOrder;

            AZStd::set<TransformType> transformSet{m_transformOrder.begin(), m_transformOrder.end()};
            if (m_transformOrder.size() != transformSet.size())
            {
                AZ_Warning("Transform2DFunctor", false, "transformOrder field contains duplicate entries");
            }

            if (transformSet.find(TransformType::Invalid) != transformSet.end())
            {
                AZ_Warning("Transform2DFunctor", false, "transformOrder contains invalid entries");
            }

            SetFunctorShaderParameter(functor, GetMaterialShaderParameters(context.GetNameContext()));

            return Success(RPI::Ptr<MaterialFunctor>(functor));
        }
    }
}
