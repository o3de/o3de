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
            AZ_RTTI(SubsurfaceTransmissionParameterFunctor, "{1F95BF80-354E-4A65-9A9E-4C7276F8558F}", RPI::MaterialFunctor);

            static void Reflect(ReflectContext* context);

            void Process(RuntimeContext& context) override;

        private:

            // Material property inputs...
            RPI::MaterialPropertyIndex m_mode;        
            RPI::MaterialPropertyIndex m_scale;
            RPI::MaterialPropertyIndex m_power;
            RPI::MaterialPropertyIndex m_distortion;
            RPI::MaterialPropertyIndex m_attenuation;
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
