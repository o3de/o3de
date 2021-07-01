/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RPI.Public/Shader/Metrics/ShaderMetrics.h>
#include <AtomCore/Serialization/Json/JsonUtils.h>

namespace AZ
{
    namespace RPI
    {
        static ShaderVariantMetrics s_metrics;

        void ShaderVariantRequest::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderVariantRequest>()
                    ->Version(1)
                    ->Field("ShaderId", &ShaderVariantRequest::m_shaderId)
                    ->Field("ShaderName", &ShaderVariantRequest::m_shaderName)
                    ->Field("ShaderVariantId", &ShaderVariantRequest::m_shaderVariantId)
                    ->Field("ShaderVariantStableId", &ShaderVariantRequest::m_shaderVariantStableId)
                    ->Field("DynamicOptionCount", &ShaderVariantRequest::m_dynamicOptionCount)
                    ->Field("RequestCount", &ShaderVariantRequest::m_requestCount)
                    ;
            }
        }

        void ShaderVariantMetrics::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderVariantMetrics>()
                    ->Version(1)
                    ->Field("m_requests", &ShaderVariantMetrics::m_requests)
                    ;
            }
        }

    } // namespace RPI
} // namespace AZ
