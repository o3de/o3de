/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyDescriptor.h>
#include <Atom/RHI.Reflect/Limits.h>

namespace AZ
{
    namespace Render
    {
        //! Materials can use this functor to map 2D scale, translate, and rotate properties into a float3x3 transform matrix.
        class Transform2DFunctor final
            : public RPI::MaterialFunctor
        {
            friend class Transform2DFunctorSourceData;
        public:
            AZ_RTTI(Transform2DFunctor, "{3E9C4357-6B2D-4A22-89DB-462441C9D8CD}", RPI::MaterialFunctor);

            enum class TransformType
            {
                Invalid,
                Scale,
                Rotate,
                Translate
            };

            static void Reflect(ReflectContext* context);

            using RPI::MaterialFunctor::Process;
            void Process(RuntimeContext& context) override;

        private:

            AZStd::vector<TransformType> m_transformOrder; //!< Controls the order in which Scale, Translate, Rotate are performed

            // Material property inputs...
            RPI::MaterialPropertyIndex m_center;        //!< index of material property for the center of scaling and rotation
            RPI::MaterialPropertyIndex m_scale;         //!< index of material property for scaling in both directions
            RPI::MaterialPropertyIndex m_scaleX;        //!< index of material property for X scale
            RPI::MaterialPropertyIndex m_scaleY;        //!< index of material property for Y scale
            RPI::MaterialPropertyIndex m_translateX;    //!< index of material property for X translation
            RPI::MaterialPropertyIndex m_translateY;    //!< index of material property for Y translation
            RPI::MaterialPropertyIndex m_rotateDegrees; //!< index of material property for rotating

            // Shader setting output...
            RHI::ShaderInputConstantIndex m_transformMatrix;        //!< the index of a float3x3 shader input
            RHI::ShaderInputConstantIndex m_transformMatrixInverse; //!< the index of the inverse float3x3 shader input
        };

    } // namespace Render

    AZ_TYPE_INFO_SPECIALIZE(Render::Transform2DFunctor::TransformType, "{D8C15D33-CE3D-4297-A646-030B0625BF84}");

} // namespace AZ
