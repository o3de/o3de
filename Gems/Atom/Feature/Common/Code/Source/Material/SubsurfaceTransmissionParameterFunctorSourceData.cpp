/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "./SubsurfaceTransmissionParameterFunctorSourceData.h"
#include <Atom/RHI.Reflect/ShaderResourceGroupLayout.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace Render
    {
        void SubsurfaceTransmissionParameterFunctorSourceData::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<SubsurfaceTransmissionParameterFunctorSourceData, RPI::MaterialFunctorSourceData>()
                    ->Version(1)
                    ->Field("mode", &SubsurfaceTransmissionParameterFunctorSourceData::m_mode)
                    ->Field("scale", &SubsurfaceTransmissionParameterFunctorSourceData::m_scale)
                    ->Field("power", &SubsurfaceTransmissionParameterFunctorSourceData::m_power)
                    ->Field("distortion", &SubsurfaceTransmissionParameterFunctorSourceData::m_distortion)
                    ->Field("attenuation", &SubsurfaceTransmissionParameterFunctorSourceData::m_attenuation)
                    ->Field("shrinkFactor", &SubsurfaceTransmissionParameterFunctorSourceData::m_shrinkFactor)
                    ->Field("transmissionNdLBias", &SubsurfaceTransmissionParameterFunctorSourceData::m_transmissionNdLBias)
                    ->Field("distanceAttenuation", &SubsurfaceTransmissionParameterFunctorSourceData::m_distanceAttenuation)
                    ->Field("tintColor", &SubsurfaceTransmissionParameterFunctorSourceData::m_tintColor)
                    ->Field("thickness", &SubsurfaceTransmissionParameterFunctorSourceData::m_thickness)
                    ->Field("enabled", &SubsurfaceTransmissionParameterFunctorSourceData::m_enabled)
                    ->Field("scatterDistanceColor", &SubsurfaceTransmissionParameterFunctorSourceData::m_scatterDistanceColor)
                    ->Field("scatterDistanceIntensity", &SubsurfaceTransmissionParameterFunctorSourceData::m_scatterDistanceIntensity)
                    ->Field("scatterDistanceShaderInput", &SubsurfaceTransmissionParameterFunctorSourceData::m_scatterDistance)
                    ->Field("parametersShaderInput", &SubsurfaceTransmissionParameterFunctorSourceData::m_transmissionParams)
                    ->Field("tintThickenssShaderInput", &SubsurfaceTransmissionParameterFunctorSourceData::m_transmissionTintThickness);
                    ;
            }
        }

        RPI::MaterialFunctorSourceData::FunctorResult SubsurfaceTransmissionParameterFunctorSourceData::CreateFunctor(const RuntimeContext& context) const
        {
            using namespace RPI;

            RPI::Ptr<SubsurfaceTransmissionParameterFunctor> functor = aznew SubsurfaceTransmissionParameterFunctor;

            functor->m_mode                     = context.FindMaterialPropertyIndex(Name{ m_mode });
            functor->m_scale                    = context.FindMaterialPropertyIndex(Name{ m_scale });
            functor->m_power                    = context.FindMaterialPropertyIndex(Name{ m_power });
            functor->m_distortion               = context.FindMaterialPropertyIndex(Name{ m_distortion });
            functor->m_attenuation              = context.FindMaterialPropertyIndex(Name{ m_attenuation });
            functor->m_shrinkFactor             = context.FindMaterialPropertyIndex(Name{ m_shrinkFactor });
            functor->m_transmissionNdLBias      = context.FindMaterialPropertyIndex(Name{ m_transmissionNdLBias });
            functor->m_distanceAttenuation      = context.FindMaterialPropertyIndex(Name{ m_distanceAttenuation });
            functor->m_tintColor                = context.FindMaterialPropertyIndex(Name{ m_tintColor });
            functor->m_thickness                = context.FindMaterialPropertyIndex(Name{ m_thickness });
            functor->m_enabled                  = context.FindMaterialPropertyIndex(Name{ m_enabled });
            functor->m_scatterDistanceColor     = context.FindMaterialPropertyIndex(Name{ m_scatterDistanceColor });
            functor->m_scatterDistanceIntensity = context.FindMaterialPropertyIndex(Name{ m_scatterDistanceIntensity });

            if (functor->m_mode.IsNull() || functor->m_scale.IsNull() || functor->m_power.IsNull() || functor->m_distortion.IsNull() ||
                functor->m_tintColor.IsNull() || functor->m_thickness.IsNull() || functor->m_enabled.IsNull() || functor->m_attenuation.IsNull() || functor->m_scatterDistanceColor.IsNull() ||
                functor->m_scatterDistanceIntensity.IsNull() || functor->m_shrinkFactor.IsNull() || functor->m_transmissionNdLBias.IsNull() ||
                functor->m_distanceAttenuation.IsNull())
            {
                return Failure();
            }

            AddMaterialPropertyDependency(functor, functor->m_mode);
            AddMaterialPropertyDependency(functor, functor->m_scale);
            AddMaterialPropertyDependency(functor, functor->m_power);
            AddMaterialPropertyDependency(functor, functor->m_distortion);
            AddMaterialPropertyDependency(functor, functor->m_attenuation);
            AddMaterialPropertyDependency(functor, functor->m_shrinkFactor);
            AddMaterialPropertyDependency(functor, functor->m_transmissionNdLBias);
            AddMaterialPropertyDependency(functor, functor->m_distanceAttenuation);
            AddMaterialPropertyDependency(functor, functor->m_tintColor);
            AddMaterialPropertyDependency(functor, functor->m_thickness);
            AddMaterialPropertyDependency(functor, functor->m_enabled);
            AddMaterialPropertyDependency(functor, functor->m_scatterDistanceColor);
            AddMaterialPropertyDependency(functor, functor->m_scatterDistanceIntensity);

            functor->m_scatterDistance = context.GetShaderResourceGroupLayout()->FindShaderInputConstantIndex(Name{ m_scatterDistance });
            functor->m_transmissionParams = context.GetShaderResourceGroupLayout()->FindShaderInputConstantIndex(Name{ m_transmissionParams });
            functor->m_transmissionTintThickness = context.GetShaderResourceGroupLayout()->FindShaderInputConstantIndex(Name{ m_transmissionTintThickness });

            if (functor->m_scatterDistance.IsNull())
            {
                AZ_Error("ShaderCollectionFunctorSourceData", false, "Could not find shader input '%s'", m_scatterDistance.c_str());
                return Failure();
            }

            if (functor->m_transmissionParams.IsNull())
            {
                AZ_Error("ShaderCollectionFunctorSourceData", false, "Could not find shader input '%s'", m_transmissionParams.c_str());
                return Failure();
            }

            if (functor->m_transmissionTintThickness.IsNull())
            {
                AZ_Error("ShaderCollectionFunctorSourceData", false, "Could not find shader input '%s'", m_transmissionTintThickness.c_str());
                return Failure();
            }

            return Success(RPI::Ptr<MaterialFunctor>(functor));
        }
    }
}
