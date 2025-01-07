/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "./Transform2DFunctor.h"
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ
{
    namespace Render
    {
        void Transform2DFunctor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Enum<TransformType>()
                    ->Value("Scale", TransformType::Scale)
                    ->Value("Rotate", TransformType::Rotate)
                    ->Value("Translate", TransformType::Translate)
                    ;

                serializeContext->Class<Transform2DFunctor, RPI::MaterialFunctor>()
                    ->Version(2)
                    ->Field("transformOrder", &Transform2DFunctor::m_transformOrder)
                    ->Field("center", &Transform2DFunctor::m_center)
                    ->Field("scale", &Transform2DFunctor::m_scale)
                    ->Field("scaleX", &Transform2DFunctor::m_scaleX)
                    ->Field("scaleY", &Transform2DFunctor::m_scaleY)
                    ->Field("translateX", &Transform2DFunctor::m_translateX)
                    ->Field("translateY", &Transform2DFunctor::m_translateY)
                    ->Field("rotateDegrees", &Transform2DFunctor::m_rotateDegrees)
                    ->Field("transformMatrix", &Transform2DFunctor::m_transformMatrix)
                    ->Field("transformMatrixInverse", &Transform2DFunctor::m_transformMatrixInverse)
                    ;
            }
        }
        
        void Transform2DFunctor::Process(RPI::MaterialFunctorAPI::RuntimeContext& context)
        {
            using namespace RPI;

            UvTransformDescriptor desc;
            desc.m_center = context.GetMaterialPropertyValue<Vector2>(m_center);
            desc.m_scale = context.GetMaterialPropertyValue<float>(m_scale);
            desc.m_scaleX = context.GetMaterialPropertyValue<float>(m_scaleX);
            desc.m_scaleY = context.GetMaterialPropertyValue<float>(m_scaleY);
            desc.m_translateX = context.GetMaterialPropertyValue<float>(m_translateX);
            desc.m_translateY = context.GetMaterialPropertyValue<float>(m_translateY);
            desc.m_rotateDegrees = context.GetMaterialPropertyValue<float>(m_rotateDegrees);

            Matrix3x3 transform = CreateUvTransformMatrix(desc, m_transformOrder);

            context.GetMaterialShaderParameter()->SetParameter(m_transformMatrix, transform);
            if (m_transformMatrixInverse.GetIndex().IsValid())
            {
                context.GetMaterialShaderParameter()->SetParameter(m_transformMatrixInverse, transform.GetInverseFull());
            }
        }

        bool Transform2DFunctor::UpdateShaderParameterConnections(const RPI::MaterialShaderParameterLayout* layout)
        {
            bool valid = true;
            if (m_transformMatrix.ValidateOrFindIndex(layout) == false)
            {
                AZ_Error(
                    "Transform2DFunctorSourceData", false, "Could not find shader parameter '%s'", m_transformMatrix.GetName().GetCStr());
                valid &= false;
            }

            // There are some cases where the matrix is required but the inverse is not, and the shader parameters only have the regular
            // matrix.
            if (!m_transformMatrixInverse.GetName().IsEmpty())
            {
                if (m_transformMatrixInverse.ValidateOrFindIndex(layout) == false)
                {
                    AZ_Error(
                        "Transform2DFunctorSourceData",
                        false,
                        "Could not find shader parameter '%s'",
                        m_transformMatrixInverse.GetName().GetCStr());
                    valid &= false;
                }
            }
            return valid;
        }
    } // namespace Render
} // namespace AZ
