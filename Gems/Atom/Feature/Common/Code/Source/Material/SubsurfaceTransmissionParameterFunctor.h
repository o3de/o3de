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
        //! This functor is used to pack and precalculate parameters required by subsurface scattering and transmission calculation
        class SubsurfaceTransmissionParameterFunctor final
            : public RPI::MaterialFunctor
        {
            friend class SubsurfaceTransmissionParameterFunctorSourceData;
        public:
            AZ_CLASS_ALLOCATOR(SubsurfaceTransmissionParameterFunctor, SystemAllocator)
            AZ_RTTI(SubsurfaceTransmissionParameterFunctor, "{1F95BF80-354E-4A65-9A9E-4C7276F8558F}", RPI::MaterialFunctor);

            static void Reflect(ReflectContext* context);

            using RPI::MaterialFunctor::Process;
            void Process(RPI::MaterialFunctorAPI::RuntimeContext& context) override;

        private:

            // Material property inputs...
            RPI::MaterialPropertyIndex m_mode;        
            RPI::MaterialPropertyIndex m_scale;
            RPI::MaterialPropertyIndex m_power;
            RPI::MaterialPropertyIndex m_distortion;
            RPI::MaterialPropertyIndex m_attenuation;
            RPI::MaterialPropertyIndex m_shrinkFactor;
            RPI::MaterialPropertyIndex m_transmissionNdLBias;
            RPI::MaterialPropertyIndex m_distanceAttenuation;
            RPI::MaterialPropertyIndex m_tintColor;
            RPI::MaterialPropertyIndex m_thickness;
            RPI::MaterialPropertyIndex m_enabled;
            RPI::MaterialPropertyIndex m_scatterDistanceColor;
            RPI::MaterialPropertyIndex m_scatterDistanceIntensity;

            // Shader setting output...
            RHI::ShaderInputConstantIndex m_scatterDistance;
            RHI::ShaderInputConstantIndex m_transmissionParams;
            RHI::ShaderInputConstantIndex m_transmissionTintThickness;
        };
    } // namespace Render
} // namespace AZ
