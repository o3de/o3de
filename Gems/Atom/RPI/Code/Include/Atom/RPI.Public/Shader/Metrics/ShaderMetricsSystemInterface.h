/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/RTTI.h>

namespace AZ
{
    namespace RPI
    {
        class ShaderAsset;
        struct ShaderVariantId;
        struct ShaderVariantSearchResult;
        struct ShaderVariantMetrics;

        //! The ShaderMetricsSystem is the RPI interface that is responsible logging the requests to shader variants.
        //! The system keeps track of the shader variants that are requested and found in the baked assets.
        class ShaderMetricsSystemInterface
        {
        public:
            AZ_RTTI(ShaderMetricsSystemInterface, "{45F6E72A-8815-4F42-A727-865BE29F8D38}");

            ShaderMetricsSystemInterface() = default;
            virtual ~ShaderMetricsSystemInterface() = default;

            static ShaderMetricsSystemInterface* Get();

            AZ_DISABLE_COPY_MOVE(ShaderMetricsSystemInterface);

            //! Resets the metrics.
            virtual void Reset() = 0;

            //! Reads the metrics from a log file.
            virtual void ReadLog() = 0;

            //! Write the metrics to a log file.
            virtual void WriteLog() = 0;

            //! Returns a value indicating wheter the metrics are enabled.
            virtual bool IsEnabled() const = 0;

            //! Sets a value indicating wheter the metrics are enabled.
            //! @param[in]  value true to enable the metrics; otherwise, false.
            virtual void SetEnabled(bool value) = 0;

            //! Gets the shader metrics.
            virtual const ShaderVariantMetrics& GetMetrics() const = 0;

            //! Requests a shader variant.
            //! @param[in]  shader The shader for which to request a specific variant.
            //! @param[in] shaderVariantId The requested shader variant.
            //! @param[in]  result The result of the search.
            virtual void RequestShaderVariant(const ShaderAsset* shader, const ShaderVariantId& shaderVariantId, const ShaderVariantSearchResult& result) = 0;
        };
    }; // namespace RPI
}; // namespace AZ
