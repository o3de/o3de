/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

        void Transform2DFunctor::Process(RuntimeContext& context)
        {
            using namespace RPI;

            auto center = context.GetMaterialPropertyValue<Vector2>(m_center);
            auto scale = context.GetMaterialPropertyValue<float>(m_scale);
            auto scaleX = context.GetMaterialPropertyValue<float>(m_scaleX);
            auto scaleY = context.GetMaterialPropertyValue<float>(m_scaleY);
            auto translateX = context.GetMaterialPropertyValue<float>(m_translateX);
            auto translateY = context.GetMaterialPropertyValue<float>(m_translateY);
            auto rotateDegrees = context.GetMaterialPropertyValue<float>(m_rotateDegrees);

            if (scaleX != 0.0f)
            {
                translateX *= (1.0f / scaleX);
            }

            if (scaleY != 0.0f)
            {
                translateY *= (1.0f / scaleY);
            }

            Matrix3x3 translateCenter2D = Matrix3x3::CreateIdentity();
            translateCenter2D.SetBasisZ(-center.GetX(), -center.GetY(), 1.0f);

            Matrix3x3 translateCenterInv2D = Matrix3x3::CreateIdentity();
            translateCenterInv2D.SetBasisZ(center.GetX(), center.GetY(), 1.0f);

            Matrix3x3 scale2D = Matrix3x3::CreateDiagonal(AZ::Vector3(scaleX * scale, scaleY * scale, 1.0f));

            Matrix3x3 translate2D = Matrix3x3::CreateIdentity();
            translate2D.SetBasisZ(translateX, translateY, 1.0f);

            Matrix3x3 rotate2D = Matrix3x3::CreateRotationZ(AZ::DegToRad(rotateDegrees));

            Matrix3x3 transform = translateCenter2D;
            for (auto transformType : m_transformOrder)
            {
                switch (transformType)
                {
                case TransformType::Scale:
                    transform = scale2D * transform;
                    break;
                case TransformType::Rotate:
                    transform = rotate2D * transform;
                    break;
                case TransformType::Translate:
                    transform = translate2D * transform;
                    break;
                }
            }
            transform = translateCenterInv2D * transform;

            context.GetShaderResourceGroup()->SetConstant(m_transformMatrix, transform);

            // There are some cases where the matrix is required but the inverse is not, so the SRG only has the regular matrix.
            // In that case, the.materialtype file will not provide the name of an inverse matrix because it doesn't have one.
            if (m_transformMatrixInverse.IsValid())
            {
                context.GetShaderResourceGroup()->SetConstant(m_transformMatrixInverse, transform.GetInverseFull());
            }
        }

    } // namespace Render
} // namespace AZ
