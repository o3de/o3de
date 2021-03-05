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
