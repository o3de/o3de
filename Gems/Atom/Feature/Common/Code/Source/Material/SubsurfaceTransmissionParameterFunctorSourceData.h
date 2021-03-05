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

#include "./SubsurfaceTransmissionParameterFunctor.h"
#include <Atom/RPI.Edit/Material/MaterialFunctorSourceData.h>

namespace AZ
{
    namespace Render
    {
        //! Builds a SubsurfaceTransmissionParameterFunctor
        //! This functor is used to pack and precalculate parameters required by subsurface scattering and transmission calculation
        class SubsurfaceTransmissionParameterFunctorSourceData final
            : public AZ::RPI::MaterialFunctorSourceData
        {
        public:
            AZ_RTTI(SubsurfaceTransmissionParameterFunctorSourceData, "{FEDECF94-0351-4775-8AE4-2005171B5634}", RPI::MaterialFunctorSourceData);

            static void Reflect(AZ::ReflectContext* context);

            FunctorResult CreateFunctor(const RuntimeContext& context) const override;

        private:

            // Material property inputs...
            AZStd::string m_mode;                       //!< material property for transmission mode
            AZStd::string m_scale;                      //!< material property for global scaling factor
            AZStd::string m_power;                      //!< material property for thick transmission power
            AZStd::string m_distortion;                 //!< material property for thick transmission distortion towards surface normal
            AZStd::string m_attenuation;                //!< material property for thick transmission volume absorption
            AZStd::string m_tintColor;                  //!< material property for transmission tint
            AZStd::string m_thickness;                  //!< material property for normalized object thickness
            AZStd::string m_enabled;                    //!< material property for subsurface scattering feature switch (enabled or disabled)
            AZStd::string m_scatterDistanceColor;       //!< material property for scatter color
            AZStd::string m_scatterDistanceIntensity;   //!< material property for scatter distance

            // Shader setting outputs...
            AZStd::string m_scatterDistance;            //!< variable names in material srg
            AZStd::string m_transmissionParams;
            AZStd::string m_transmissionTintThickness;
        };

    } // namespace Render
} // namespace AZ
