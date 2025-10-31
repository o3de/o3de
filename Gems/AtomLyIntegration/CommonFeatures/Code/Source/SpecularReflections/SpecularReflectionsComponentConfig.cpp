/*7
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SpecularReflections/SpecularReflectionsComponentConfig.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace AZ
{
    namespace Render
    {
        void SpecularReflectionsComponentSSRConfig::Reflect(ReflectContext* context)
        {
            SSROptions::Reflect(context);

            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<SpecularReflectionsComponentSSRConfig>()
                    ->Version(1)
                    ->Field("Options", &SpecularReflectionsComponentSSRConfig::m_options)
                ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<SSROptions>("Screen Space Reflections (SSR)", "Screen Space Reflections (SSR) Configuration")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->DataElement(Edit::UIHandlers::CheckBox, &SSROptions::m_enable, "Enable SSR", "Enable Screen Space Reflections (SSR)")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::EntireTree)
                        ->DataElement(Edit::UIHandlers::Slider, &SSROptions::m_maxRayDistance, "Maximum Ray Distance", "The maximum length of the rays to consider for hit detection")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &SSROptions::IsEnabled)
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 1.0f)
                        ->DataElement(Edit::UIHandlers::Slider, &SSROptions::m_maxDepthThreshold, "Maximum Depth Threshold", "The maximum delta between the ray depth and depth buffer value which will be considered a hit.  Also known as thickness.")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &SSROptions::IsEnabled)
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 5.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                        ->DataElement(Edit::UIHandlers::Slider, &SSROptions::m_maxRoughness, "Maximum Roughness", "Surfaces at or below this roughness value will have SSR applied")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &SSROptions::IsEnabled)
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                        ->DataElement(Edit::UIHandlers::Slider, &SSROptions::m_roughnessBias, "Roughness Bias", "Bias applied to the surface roughness")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &SSROptions::IsEnabled)
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                        ->DataElement(Edit::UIHandlers::CheckBox, &SSROptions::m_halfResolution, "Half Resolution", "Use half resolution in the reflected image, improves performance but may increase artifacts during camera motion")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &SSROptions::IsEnabled)
                            ->DataElement(Edit::UIHandlers::ComboBox, &SSROptions::m_reflectionMethod, "Reflection Method",
                                      "Screen-space: Use screen-space reflections only\n\n"
                                      "Hybrid SSR-RT: Use ray tracing for hit detection and screen-space data for surface evaluation\n\n"
                                      "Hybrid SSR-RT + Ray Tracing fallback: Use screen-space reflection data when available and ray tracing when not\n\n"
                                      "Ray Tracing: Use hardware ray tracing only")
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::EntireTree)
                                ->EnumAttribute(SSROptions::ReflectionMethod::ScreenSpace, "Screen Space")
                                ->EnumAttribute(SSROptions::ReflectionMethod::Hybrid, "Hybrid SSR-RT")
                                ->EnumAttribute(SSROptions::ReflectionMethod::HybridWithFallback, "Hybrid SSR-RT + Ray Tracing fallback")
                                ->EnumAttribute(SSROptions::ReflectionMethod::RayTracing, "Ray Tracing")
                            ->DataElement(Edit::UIHandlers::CheckBox, &SSROptions::m_rayTraceFallbackSpecular, "Apply Fallback Specular Lighting", "Apply specular lighting in the fallback image, improves fallback image accuracy but may reduce performance and increase artifacts")
                                ->Attribute(AZ::Edit::Attributes::Visibility, &SSROptions::IsRayTracingFallbackEnabled)
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Temporal Filtering")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->DataElement(Edit::UIHandlers::CheckBox, &SSROptions::m_temporalFiltering, "Temporal Filtering", "Reduce artifacts with temporal filtering over multiple frames")
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::EntireTree)
                                ->Attribute(AZ::Edit::Attributes::Visibility, &SSROptions::IsEnabled)
                            ->DataElement(Edit::UIHandlers::Slider, &SSROptions::m_temporalFilteringStrength, "Temporal Filtering Strength", "Higher strength reduces motion artifacts but increases temporal lag")
                                ->Attribute(AZ::Edit::Attributes::Visibility, &SSROptions::IsTemporalFilteringEnabled)
                                ->Attribute(AZ::Edit::Attributes::Min, 0.1f)
                                ->Attribute(AZ::Edit::Attributes::Max, 2.0f)
                                ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                                ->Attribute(AZ::Edit::Attributes::Decimals, 1)
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Luminance")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->DataElement(Edit::UIHandlers::CheckBox, &SSROptions::m_luminanceClamp, "Luminance Clamp", "Reduce specular artifacts by clamping the luminance to a maximum value")
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::EntireTree)
                                ->Attribute(AZ::Edit::Attributes::Visibility, &SSROptions::IsEnabled)
                            ->DataElement(Edit::UIHandlers::Slider, &SSROptions::m_maxLuminance, "Maximum Luminance", "Maximum luminance value")
                                ->Attribute(AZ::Edit::Attributes::Visibility, &SSROptions::IsLuminanceClampEnabled)
                                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                                ->Attribute(AZ::Edit::Attributes::Max, 5.0f)
                                ->Attribute(AZ::Edit::Attributes::Decimals, 3)
                    ;
                }
            }
        }

        void SpecularReflectionsComponentConfig::Reflect(ReflectContext* context)
        {
            SpecularReflectionsComponentSSRConfig::Reflect(context);

            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<SpecularReflectionsComponentConfig, ComponentConfig>()
                    ->Version(1)
                    ->Field("SSR", &SpecularReflectionsComponentConfig::m_ssr)
                    ;
            }
        }
    } // namespace Render
} // namespace AZ
