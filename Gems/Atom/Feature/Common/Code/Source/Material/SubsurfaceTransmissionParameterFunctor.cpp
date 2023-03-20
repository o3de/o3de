/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "./SubsurfaceTransmissionParameterFunctor.h"
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>

namespace AZ
{
    namespace Render
    {
        void SubsurfaceTransmissionParameterFunctor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<SubsurfaceTransmissionParameterFunctor, RPI::MaterialFunctor>()
                    ->Version(2)
                    ->Field("m_mode", &SubsurfaceTransmissionParameterFunctor::m_mode)
                    ->Field("m_scale", &SubsurfaceTransmissionParameterFunctor::m_scale)
                    ->Field("m_power", &SubsurfaceTransmissionParameterFunctor::m_power)
                    ->Field("m_distortion", &SubsurfaceTransmissionParameterFunctor::m_distortion)
                    ->Field("m_attenuation", &SubsurfaceTransmissionParameterFunctor::m_attenuation)
                    ->Field("m_shrinkFactor", &SubsurfaceTransmissionParameterFunctor::m_shrinkFactor)
                    ->Field("m_transmissionNdLBias", &SubsurfaceTransmissionParameterFunctor::m_transmissionNdLBias)
                    ->Field("m_distanceAttenuation", &SubsurfaceTransmissionParameterFunctor::m_distanceAttenuation)
                    ->Field("m_tintColor", &SubsurfaceTransmissionParameterFunctor::m_tintColor)
                    ->Field("m_thickness", &SubsurfaceTransmissionParameterFunctor::m_thickness)
                    ->Field("m_enabled", &SubsurfaceTransmissionParameterFunctor::m_enabled)
                    ->Field("m_scatterDistanceColor", &SubsurfaceTransmissionParameterFunctor::m_scatterDistanceColor)
                    ->Field("m_scatterDistanceIntensity", &SubsurfaceTransmissionParameterFunctor::m_scatterDistanceIntensity)
                    ->Field("m_scatterDistance", &SubsurfaceTransmissionParameterFunctor::m_scatterDistance)
                    ->Field("m_transmissionParams", &SubsurfaceTransmissionParameterFunctor::m_transmissionParams)
                    ->Field("m_transmissionTintThickness", &SubsurfaceTransmissionParameterFunctor::m_transmissionTintThickness)
                    ;
            }
        }

        void SubsurfaceTransmissionParameterFunctor::Process(RPI::MaterialFunctorAPI::RuntimeContext& context)
        {
            // Build & preprocess all parameters used by subsurface scattering feature

            using namespace RPI;
            enum class TransmissionMode { None, ThickObject, ThinObject };

            auto mode = context.GetMaterialPropertyValue<uint32_t>(m_mode);
            auto scale = context.GetMaterialPropertyValue<float>(m_scale);
            auto power = context.GetMaterialPropertyValue<float>(m_power);
            auto distortion = context.GetMaterialPropertyValue<float>(m_distortion);
            auto attenuation = context.GetMaterialPropertyValue<float>(m_attenuation);
            auto shrinkFactor = context.GetMaterialPropertyValue<float>(m_shrinkFactor);
            auto transmissionNdLBias = context.GetMaterialPropertyValue<float>(m_transmissionNdLBias);
            auto distanceAttenuation = context.GetMaterialPropertyValue<float>(m_distanceAttenuation);
            auto tintColor = context.GetMaterialPropertyValue<Color>(m_tintColor);
            auto thickness = context.GetMaterialPropertyValue<float>(m_thickness);
            auto scatterDistanceColor = context.GetMaterialPropertyValue<Color>(m_scatterDistanceColor);
            auto scatterDistanceIntensity = context.GetMaterialPropertyValue<float>(m_scatterDistanceIntensity);

            Vector3 scatterDistance = Vector3(scatterDistanceColor.GetAsVector3()) * Vector3(scatterDistanceIntensity, scatterDistanceIntensity, scatterDistanceIntensity);

            Vector4 transmissionParams;
            if (mode == static_cast<uint32_t>(TransmissionMode::ThickObject))
            {
                transmissionParams.SetX(attenuation);
                transmissionParams.SetY(power);
                transmissionParams.SetZ(distortion);
                transmissionParams.SetW(scale);
            }
            else
            {
                transmissionParams.SetX(shrinkFactor);
                transmissionParams.SetY(transmissionNdLBias);
                transmissionParams.SetZ(distanceAttenuation);
                transmissionParams.SetW(scale);
            }

            Vector4 transmissionTintThickness;
            transmissionTintThickness.Set(tintColor.GetAsVector3());
            transmissionTintThickness.SetW(thickness);

            context.GetShaderResourceGroup()->SetConstant(m_scatterDistance, scatterDistance);
            context.GetShaderResourceGroup()->SetConstant(m_transmissionParams, transmissionParams);
            context.GetShaderResourceGroup()->SetConstant(m_transmissionTintThickness, transmissionTintThickness);
        }

    } // namespace Render
} // namespace AZ
