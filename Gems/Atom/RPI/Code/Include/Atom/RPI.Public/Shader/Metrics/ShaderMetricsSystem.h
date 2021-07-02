/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Shader/Metrics/ShaderMetricsSystemInterface.h>
#include <Atom/RPI.Public/Shader/Metrics/ShaderMetrics.h>

#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    namespace RPI
    {
        class CommandList;

        class ShaderMetricsSystem final
            : public ShaderMetricsSystemInterface
        {
        public:
            AZ_TYPE_INFO(ShaderMetricsSystem, "{863D06A7-43DF-449E-A972-A2B7E09558FE}");
            AZ_CLASS_ALLOCATOR(ShaderMetricsSystem, AZ::SystemAllocator, 0);

            ShaderMetricsSystem() = default;

            static void Reflect(AZ::ReflectContext* context);

            void Init();
            void Shutdown();

        private:
            void Reset() final;

            void ReadLog() final;
            void WriteLog() final;

            virtual void SetEnabled(bool value) final;
            virtual bool IsEnabled() const final;

            virtual const ShaderVariantMetrics& GetMetrics() const final;

            void RequestShaderVariant(const ShaderAsset* shader, const ShaderVariantId& shaderVariantId, const ShaderVariantSearchResult& result) final;

            bool m_isEnabled = false;

            //! Lock for m_metrics
            AZStd::mutex m_metricsMutex;

            //! List of RPI shader requests.
            ShaderVariantMetrics m_metrics;
        };

    }; // namespace RPI
}; // namespace AZ
