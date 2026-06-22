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
#include <Atom/Utils/MaterialUtils.h>

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
            AZ_CLASS_ALLOCATOR(Transform2DFunctor, SystemAllocator)
            AZ_RTTI(Transform2DFunctor, "{3E9C4357-6B2D-4A22-89DB-462441C9D8CD}", RPI::MaterialFunctor);

            static void Reflect(ReflectContext* context);

            using RPI::MaterialFunctor::Process;
            void Process(RPI::MaterialFunctorAPI::RuntimeContext& context) override;
            bool UpdateShaderParameterConnections(const RPI::MaterialShaderParameterLayout* layout) override;

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
            // Each matrix is stored as 3 separate row_major float4 rows to work around a Vulkan structured buffer bug with float3x3.
            RPI::MaterialShaderParameterNameIndex m_transformMatrixRow0; //!< Row 0 of the UV transform matrix (float4, w unused)
            RPI::MaterialShaderParameterNameIndex m_transformMatrixRow1; //!< Row 1 of the UV transform matrix (float4, w unused)
            RPI::MaterialShaderParameterNameIndex m_transformMatrixRow2; //!< Row 2 of the UV transform matrix (float4, w unused)
            RPI::MaterialShaderParameterNameIndex m_transformMatrixInverseRow0; //!< Row 0 of the inverse UV transform matrix (float4, w unused)
            RPI::MaterialShaderParameterNameIndex m_transformMatrixInverseRow1; //!< Row 1 of the inverse UV transform matrix (float4, w unused)
            RPI::MaterialShaderParameterNameIndex m_transformMatrixInverseRow2; //!< Row 2 of the inverse UV transform matrix (float4, w unused)
        };

    } // namespace Render

} // namespace AZ
