/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            AZ_CLASS_ALLOCATOR(SubsurfaceTransmissionParameterFunctorSourceData, AZ::SystemAllocator)
            AZ_RTTI(SubsurfaceTransmissionParameterFunctorSourceData, "{FEDECF94-0351-4775-8AE4-2005171B5634}", RPI::MaterialFunctorSourceData);

            static void Reflect(AZ::ReflectContext* context);

            using AZ::RPI::MaterialFunctorSourceData::CreateFunctor;
            FunctorResult CreateFunctor(const RuntimeContext& context) const override;

        private:

            // Material property inputs...
            AZStd::string m_mode;                       //!< material property for transmission mode
            AZStd::string m_scale;                      //!< material property for global scaling factor
            AZStd::string m_power;                      //!< material property for thick transmission power
            AZStd::string m_distortion;                 //!< material property for thick transmission distortion towards surface normal
            AZStd::string m_attenuation;                //!< material property for thick transmission volume absorption
            AZStd::string m_shrinkFactor;               //!< material property for thin transmission position shrink factor towards surface normal
            AZStd::string m_transmissionNdLBias;        //!< material property for thin transmission bias of the NdL value
            AZStd::string m_distanceAttenuation;        //!< material property for thin transmission attenuation with distance 
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
