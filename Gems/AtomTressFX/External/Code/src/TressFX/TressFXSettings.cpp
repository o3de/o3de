// ----------------------------------------------------------------------------
// Wrappers for setting values that end up in constant buffers.
// ----------------------------------------------------------------------------
//
// Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
#include <TressFX/TressFXSettings.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <Assets/HairAsset.h>

namespace AMD
{
    void TressFXSimulationSettings::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TressFXSimulationSettings>()
                ->Version(0)
                ->Field("vspCoeff", &TressFXSimulationSettings::m_vspCoeff)
                ->Field("vspAccelThreshold", &TressFXSimulationSettings::m_vspAccelThreshold)
                ->Field("localConstraintStiffness", &TressFXSimulationSettings::m_localConstraintStiffness)
                ->Field("localConstraintsIterations", &TressFXSimulationSettings::m_localConstraintsIterations)
                ->Field("globalConstraintStiffness", &TressFXSimulationSettings::m_globalConstraintStiffness)
                ->Field("globalConstraintsRange", &TressFXSimulationSettings::m_globalConstraintsRange)
                ->Field("lengthConstraintsIterations", &TressFXSimulationSettings::m_lengthConstraintsIterations)
                ->Field("damping", &TressFXSimulationSettings::m_damping)
                ->Field("gravityMagnitude", &TressFXSimulationSettings::m_gravityMagnitude)
                ->Field("tipSeparation", &TressFXSimulationSettings::m_tipSeparation)
                ->Field("windMagnitude", &TressFXSimulationSettings::m_windMagnitude)
                ->Field("windDirection", &TressFXSimulationSettings::m_windDirection)
                ->Field("windAngleRadians", &TressFXSimulationSettings::m_windAngleRadians)
                ->Field("clampPositionDelta", &TressFXSimulationSettings::m_clampPositionDelta);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<TressFXSimulationSettings>("TressFXSimulationSettings", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &TressFXSimulationSettings::m_vspCoeff, "Vsp Coeffs",
                        "VSP (Velocity Shock Propagation) value. VSP makes the root vertex velocity propagate through the rest "
                        "of vertices in the hair strand.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &TressFXSimulationSettings::m_vspAccelThreshold, "Vsp Accel Threshold",
                        "VSP acceleration threshold makes the VSP value increase when the pseudo-acceleration of the root "
                        "vertex is greater than the threshold value.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 10.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &TressFXSimulationSettings::m_localConstraintStiffness, "Local Constraint Stiffness",
                        "Controls the stiffness of a strand, meaning both global and local stiffness are used to keep the original "
                        "(imported) hair shape.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &TressFXSimulationSettings::m_localConstraintsIterations,
                        "Local Constraint Iterations", "Allocates more simulation time (iterations) toward keeping the local hair shape.")
                        ->Attribute(AZ::Edit::Attributes::Min, 1)
                        ->Attribute(AZ::Edit::Attributes::Max, 10)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &TressFXSimulationSettings::m_globalConstraintStiffness,
                        "Global Constraint Stiffness", "Controls the stiffness of a strand, meaning both global and local stiffness are used "
                        "to keep the original(imported) hair shape.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &TressFXSimulationSettings::m_globalConstraintsRange, "Global Constraint Range",
                        "Controls how much of the hair strand is affected by the global shape stiffness requirement.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &TressFXSimulationSettings::m_lengthConstraintsIterations,
                        "Length Constraint Iterations", "Allocates more simulation time (iterations) toward keeping the global hair shape.")
                        ->Attribute(AZ::Edit::Attributes::Min, 1)
                        ->Attribute(AZ::Edit::Attributes::Max, 10)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &TressFXSimulationSettings::m_damping, "Damping",
                        "Damping smooths out the motion of the hair. It also slows down the hair movement.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &TressFXSimulationSettings::m_gravityMagnitude, "Gravity Magnitude",
                        "Gravity pseudo value. A value of 10 closely approximates regular gravity in common game engine.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &TressFXSimulationSettings::m_tipSeparation, "Tip Separation",
                        "Forces the tips of the strands away from each other.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &TressFXSimulationSettings::m_windMagnitude, "Wind Magnitude",
                        "Wind multiplier value. It allows you to see the effect of wind on the hair.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TressFXSimulationSettings::m_windDirection, "Wind Direction",
                        "xyz-vector (world space) for the wind direction.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &TressFXSimulationSettings::m_windAngleRadians, "Wind Angle Radians",
                        "Wind angle in radians")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &TressFXSimulationSettings::m_clampPositionDelta, "Clamp Velocity",
                        "To increase stability at low or unstable framerates, this parameter limits the displacement of hair segments per frame.")
                        ->Attribute(AZ::Edit::Attributes::Min, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 24.0f)
                    ;
            }
        }
    }

    void TressFXRenderingSettings::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TressFXRenderingSettings>()
                ->Version(3)
                // Albedo assets
                ->Field("BaseAlbedoAsset", &TressFXRenderingSettings::m_baseAlbedoAsset)
                ->Field("StrandAlbedoAsset", &TressFXRenderingSettings::m_strandAlbedoAsset)

                // LOD Settings
                ->Field("LODStartDistance", &TressFXRenderingSettings::m_LODStartDistance)
                ->Field("LODEndDistance", &TressFXRenderingSettings::m_LODEndDistance)
                ->Field("LODPercent", &TressFXRenderingSettings::m_LODPercent)
                ->Field("LODWidthMultiplier", &TressFXRenderingSettings::m_LODWidthMultiplier)

                // General information
                ->Field("FiberRadius", &TressFXRenderingSettings::m_FiberRadius)
                ->Field("TipPercentage", &TressFXRenderingSettings::m_TipPercentage)
                ->Field("StrandUVTilingFactor", &TressFXRenderingSettings::m_StrandUVTilingFactor)
                ->Field("FiberRatio", &TressFXRenderingSettings::m_FiberRatio)

                // Lighting/shading - Original TressFX Kajiya lighting model
                ->Field("HairMatBaseColor", &TressFXRenderingSettings::m_HairMatBaseColor)
                ->Field("HairMatTipColor", &TressFXRenderingSettings::m_HairMatTipColor)
                ->Field("HairKDiffuse", &TressFXRenderingSettings::m_HairKDiffuse)
                ->Field("HairKSpec1", &TressFXRenderingSettings::m_HairKSpec1)
                ->Field("HairSpecExp1", &TressFXRenderingSettings::m_HairSpecExp1)
                ->Field("HairKSpec2", &TressFXRenderingSettings::m_HairKSpec2)
                ->Field("HairSpecExp2", &TressFXRenderingSettings::m_HairSpecExp2)
                // Lighting/shading - modified Marschner lighting model
                ->Field("HairRoughness", &TressFXRenderingSettings::m_HairRoughness)
                ->Field("HairCuticleTilt", &TressFXRenderingSettings::m_HairCuticleTilt)

                // shadow lookup
                ->Field("HairShadowAlpha", &TressFXRenderingSettings::m_HairShadowAlpha)
                ->Field("HairFiberSpacing", &TressFXRenderingSettings::m_HairFiberSpacing)
                ->Field("HairMaxShadowFibers", &TressFXRenderingSettings::m_HairMaxShadowFibers)
                ->Field("ShadowLODStartDistance", &TressFXRenderingSettings::m_ShadowLODStartDistance)
                ->Field("ShadowLODEndDistance", &TressFXRenderingSettings::m_ShadowLODEndDistance)
                ->Field("ShadowLODPercent", &TressFXRenderingSettings::m_ShadowLODPercent)
                ->Field("ShadowLODWidthMultiplier", &TressFXRenderingSettings::m_ShadowLODWidthMultiplier)

                // others
                ->Field("EnableStrandUV", &TressFXRenderingSettings::m_EnableStrandUV)
                ->Field("EnableStrandTangent", &TressFXRenderingSettings::m_EnableStrandTangent)
                ->Field("EnableThinTip", &TressFXRenderingSettings::m_EnableThinTip)
                ->Field("EnableHairLOD", &TressFXRenderingSettings::m_EnableHairLOD)
                ->Field("EnableShadowLOD", &TressFXRenderingSettings::m_EnableShadowLOD)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<TressFXRenderingSettings>("TressFXRenderingSettings", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TressFXRenderingSettings::m_baseAlbedoAsset, "Base Albedo Asset",
                        "This texture is the UV mapped color texture to use for the 'scalp'.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &TressFXRenderingSettings::OnImgChanged)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TressFXRenderingSettings::m_strandAlbedoAsset, "Strand Albedo Asset",
                        "Albedo texture to use along the strand.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &TressFXRenderingSettings::OnImgChanged)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TressFXRenderingSettings::m_LODStartDistance, "LOD Start Distance",
                        "Distance to begin LOD. Distance is in centimeters between the camera and hair.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TressFXRenderingSettings::m_LODEndDistance, "LOD End Distance",
                        "Distance where LOD should be at its maximum reduction/multiplier values, in centimeters.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &TressFXRenderingSettings::m_LODPercent, "Max LOD Reduction",
                        "Maximum amount of reduction as a percentage of the original.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &TressFXRenderingSettings::m_LODWidthMultiplier, "Max LOD Strand Width Multiplier",
                        "Maximum amount the strand width would be multiplied by.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 10.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &TressFXRenderingSettings::m_FiberRadius, "Fiber Radius", "Diameter of the fiber.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 0.01f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &TressFXRenderingSettings::m_TipPercentage, "Tip Percentage",
                        "Dictates the amount of lerp blend between Base Scalp Albedo and Mat Tip Color.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &TressFXRenderingSettings::m_StrandUVTilingFactor, "Strand UVTiling Factor",
                        "Amount of tiling to use (1D) along the strand.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 10.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &TressFXRenderingSettings::m_FiberRatio, "Fiber ratio",
                        "Used with thin tip. Sets the extent to which the hair strand will taper.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Color, &TressFXRenderingSettings::m_HairMatBaseColor, "Base Color",
                        "RGB color to be used for the base color of the hair.")
                        ->Attribute(AZ_CRC_CE("AlphaChannel"), true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Color, &TressFXRenderingSettings::m_HairMatTipColor, "Mat Tip Color",
                        "RGB color to use for a blend from root to tip.")
                        ->Attribute(AZ_CRC_CE("AlphaChannel"), true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TressFXRenderingSettings::m_HairKDiffuse, "Hair Kdiffuse",
                        "Diffuse coefficient, think of it as a gain value. The Kajiya-Kay model diffuse component is proportional to "
                        "the sine between the light and tangent vectors.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &TressFXRenderingSettings::m_HairKSpec1, "Hair Ks1",
                        "Primary specular reflection coefficient (shifted toward the root).")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &TressFXRenderingSettings::m_HairSpecExp1, "Hair Ex1",
                        "Specular power to use for the calculated specular root value (primary highlight that is shifted toward the root.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &TressFXRenderingSettings::m_HairKSpec2, "Hair Ks2",
                        "Secondary specular reflection coefficient (shifted toward the tip).")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &TressFXRenderingSettings::m_HairSpecExp2, "Hair Ex2",
                        "Specular power to use for the calculated specular tip value.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &TressFXRenderingSettings::m_HairRoughness, "Hair Roughness",
                        "Roughness of the hair - effective value is between 0.5 and 0.7")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.4f)
                    ->Attribute(AZ::Edit::Attributes::Max, 0.9f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &TressFXRenderingSettings::m_HairCuticleTilt, "Hair Cuticle Angle",
                        "Cutical angle in radians determeaning how the light refraction behaves. Suggested value is 5-8 degrees --> 0.07")
                    ->Attribute(AZ::Edit::Attributes::Min, -0.05f)
                    ->Attribute(AZ::Edit::Attributes::Max, 0.15f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &TressFXRenderingSettings::m_HairShadowAlpha, "Hair Shadow Alpha",
                        "Used to attenuate hair shadows based on distance (depth into the strands of hair).")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &TressFXRenderingSettings::m_HairFiberSpacing, "Fiber Spacing",
                        "How much spacing between the fibers (should include fiber radius when setting this value). ")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &TressFXRenderingSettings::m_HairMaxShadowFibers, "Max Shadow Fibers",
                        "Used as a cutoff value for the shadow attenuation calculation.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0)
                        ->Attribute(AZ::Edit::Attributes::Max, 100)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TressFXRenderingSettings::m_ShadowLODStartDistance, "Shadow LOD Start Distance",
                        "(Shadow)Distance to begin LOD.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TressFXRenderingSettings::m_ShadowLODEndDistance, "Shadow LOD End Distance",
                        "(Shadow)Distance where LOD should be at its maximum reduction/multiplier values.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &TressFXRenderingSettings::m_ShadowLODPercent, "Shadow Max LOD Reduction",
                        "Maximum amount of reduction as a percentage of the original. ")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &TressFXRenderingSettings::m_ShadowLODWidthMultiplier,
                        "Shadow Max LOD Strand Width Multiplier",
                        "Maximum amount the shadow width cast by the strand would be multiplied by.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 10.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::CheckBox, &TressFXRenderingSettings::m_EnableStrandUV, "Enable Strand UV",
                        "Turns on usage of Strand Albedo.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::CheckBox, &TressFXRenderingSettings::m_EnableStrandTangent, "Enable Strand Tangent",
                        "Turns on usage of Strand Tangent.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::CheckBox, &TressFXRenderingSettings::m_EnableThinTip, "Enable Thin Tip",
                        "If selected, the very end of the hair will narrow to a tip, otherwise it will stay squared.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::CheckBox, &TressFXRenderingSettings::m_EnableHairLOD, "Enable Hair LOD",
                        "Turn on Level of Detail usage for the hair.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::CheckBox, &TressFXRenderingSettings::m_EnableShadowLOD, "Enable Hair LOD(Shadow)",
                        "Turn on Level of Detail usage for the shadow of the hair.")
                    ;
            }
        }
    }

    void TressFXRenderingSettings::OnImgChanged()
    {
        m_imgDirty = true;
    }
} // namespace AMD
