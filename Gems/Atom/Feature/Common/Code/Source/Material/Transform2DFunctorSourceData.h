/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "./Transform2DFunctor.h"
#include <Atom/RPI.Edit/Material/MaterialFunctorSourceData.h>

namespace AZ
{
    namespace Render
    {
        //! Builds a Transform2DFunctor.
        //! Materials can use this functor to map 2D scale, translate, and rotate properties into a float3x3 transform matrix.
        class Transform2DFunctorSourceData final
            : public AZ::RPI::MaterialFunctorSourceData
        {
        public:
            AZ_RTTI(Transform2DFunctorSourceData, "{82E9FE9B-A0C2-42D4-BCE7-A0C10CC0E445}", RPI::MaterialFunctorSourceData);

            static void Reflect(AZ::ReflectContext* context);

            using AZ::RPI::MaterialFunctorSourceData::CreateFunctor;
            FunctorResult CreateFunctor(const RuntimeContext& context) const override;

        private:

            AZStd::vector<Transform2DFunctor::TransformType> m_transformOrder; //!< Controls the order in which Scale, Translate, Rotate are performed

            // Material property inputs...
            AZStd::string m_center;        //!< material property for center of scaling and rotation
            AZStd::string m_scale;         //!< material property for scaling in both directions
            AZStd::string m_scaleX;        //!< material property for X scale
            AZStd::string m_scaleY;        //!< material property for Y scale
            AZStd::string m_translateX;    //!< material property for X translation
            AZStd::string m_translateY;    //!< material property for Y translation
            AZStd::string m_rotateDegrees; //!< material property for rotating

            // Shader setting outputs...
            AZStd::string m_transformMatrix;          //!< name of a float3x3 shader input
            AZStd::string m_transformMatrixInverse;   //!< name of the inverse float3x3 shader input
        };

    } // namespace Render
} // namespace AZ
