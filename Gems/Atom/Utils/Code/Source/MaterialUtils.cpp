/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Utils/MaterialUtils.h>

namespace AZ::Render
{
    
    Matrix3x3 CreateUvTransformMatrix(const UvTransformDescriptor& desc, AZStd::span<const TransformType> transformOrder)
    {
        float translateX = desc.m_translateX;
        float translateY = desc.m_translateY;

        if (desc.m_scaleX != 0.0f)
        {
            translateX *= (1.0f / desc.m_scaleX);
        }

        if (desc.m_scaleY != 0.0f)
        {
            translateY *= (1.0f / desc.m_scaleY);
        }

        Matrix3x3 translateCenter2D = Matrix3x3::CreateIdentity();
        translateCenter2D.SetBasisZ(-desc.m_center.GetX(), -desc.m_center.GetY(), 1.0f);

        Matrix3x3 translateCenterInv2D = Matrix3x3::CreateIdentity();
        translateCenterInv2D.SetBasisZ(desc.m_center.GetX(), desc.m_center.GetY(), 1.0f);

        Matrix3x3 scale2D = Matrix3x3::CreateDiagonal(AZ::Vector3(desc.m_scaleX * desc.m_scale, desc.m_scaleY * desc.m_scale, 1.0f));

        Matrix3x3 translate2D = Matrix3x3::CreateIdentity();
        translate2D.SetBasisZ(translateX, translateY, 1.0f);

        Matrix3x3 rotate2D = Matrix3x3::CreateRotationZ(AZ::DegToRad(desc.m_rotateDegrees));
            
        Matrix3x3 transform = translateCenter2D;
        for (auto transformType : transformOrder)
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
        return transform;
    }

}
