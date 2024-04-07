/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <Atom/RPI.Public/FeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        struct SSROptions final
        {
            AZ_RTTI(SSROptions, "{A3DE7EDD-3680-458F-A69C-FE7550B75652}");
            AZ_CLASS_ALLOCATOR(SSROptions, SystemAllocator, 0);

            enum class ReflectionMethod
            {
                ScreenSpace,
                Hybrid,
                HybridWithFallback,
                RayTracing
            };

            static void Reflect(ReflectContext* context)
            {
                if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
                {
                    serializeContext->Class<SSROptions>()
                        ->Version(1)
                        ->Field("Enable", &SSROptions::m_enable)
                        ->Field("ConeTracing", &SSROptions::m_coneTracing)
                        ->Field("MaxRayDistance", &SSROptions::m_maxRayDistance)
                        ->Field("MaxDepthThreshold", &SSROptions::m_maxDepthThreshold)
                        ->Field("MaxRoughness", &SSROptions::m_maxRoughness)
                        ->Field("RoughnessBias", &SSROptions::m_roughnessBias)
                        ->Field("HalfResolution", &SSROptions::m_halfResolution)
                        ->Field("ReflectionMethod", &SSROptions::m_reflectionMethod)
                        ->Field("RayTraceFallbackSpecular", &SSROptions::m_rayTraceFallbackSpecular)
                        ->Field("TemporalFiltering", &SSROptions::m_temporalFiltering)
                        ->Field("TemporalFilteringStrength", &SSROptions::m_temporalFilteringStrength)
                        ->Field("LuminanceClamp", &SSROptions::m_luminanceClamp)
                        ->Field("MaxLuminance", &SSROptions::m_maxLuminance)
                    ;
                }
            }

            bool IsEnabled() const { return m_enable; }
            bool IsRayTracingEnabled() const { return m_enable && m_reflectionMethod != ReflectionMethod::ScreenSpace; }
            bool IsRayTracingFallbackEnabled() const { return IsRayTracingEnabled() && m_reflectionMethod != ReflectionMethod::Hybrid; }
            bool IsLuminanceClampEnabled() const { return m_enable && m_luminanceClamp; }
            bool IsTemporalFilteringEnabled() const { return m_enable && m_temporalFiltering; }
            float GetOutputScale() const { return m_halfResolution ? 0.5f : 1.0f; }

            bool  m_enable = false;
            bool  m_coneTracing = false;
            float m_maxRayDistance = 50.0f;
            float m_maxDepthThreshold = 0.1f;
            float m_maxRoughness = 0.31f;
            float m_roughnessBias = 0.0f;
            bool  m_halfResolution = true;
            ReflectionMethod m_reflectionMethod = ReflectionMethod::HybridWithFallback;
            bool  m_rayTraceFallbackSpecular = false;
            bool  m_temporalFiltering = true;
            float m_temporalFilteringStrength = 1.0f;
            bool  m_luminanceClamp = true;
            float m_maxLuminance = 1.5f;
        };

        //! The SpecularReflectionsFeatureProcessorInterface provides an interface to the feature processor for code outside of Atom.
        class SpecularReflectionsFeatureProcessorInterface
            : public RPI::FeatureProcessor
        {
        public:
            AZ_RTTI(AZ::Render::SpecularReflectionsFeatureProcessorInterface, "{DF5BFC4B-B29B-4A47-A2A2-D566617B4153}", AZ::RPI::FeatureProcessor);

            virtual void SetSSROptions(const SSROptions& ssrOptions) = 0;
            virtual const SSROptions& GetSSROptions() const = 0;
        };
    } // namespace Render
} // namespace AZ
