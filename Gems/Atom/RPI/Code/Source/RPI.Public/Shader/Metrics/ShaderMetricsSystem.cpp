/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Shader/Metrics/ShaderMetricsSystem.h>
#include <Atom/RPI.Public/Shader/Metrics/ShaderMetrics.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>


#include <AzCore/Interface/Interface.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Utils//Utils.h>

#include <AzCore/Serialization/Json/JsonUtils.h>

#include <AzFramework/IO/LocalFileIO.h>

namespace AZ
{
    namespace RPI
    {
        AZ::IO::FixedMaxPath GetMetricsFilePath()
        {
            AZ::IO::FixedMaxPath resolvedPath;
            AZ::IO::LocalFileIO::GetInstance()->ResolvePath(resolvedPath, "@user@/ShaderMetrics.json");
            return resolvedPath;
        }

        ShaderMetricsSystemInterface* ShaderMetricsSystemInterface::Get()
        {
            return Interface<ShaderMetricsSystemInterface>::Get();
        }

        void ShaderMetricsSystem::Reflect(ReflectContext* context)
        {
            ShaderVariantRequest::Reflect(context);
            ShaderVariantMetrics::Reflect(context);
        }

        void ShaderMetricsSystem::Init()
        {
            // Register the system to the interface.
            Interface<ShaderMetricsSystemInterface>::Register(this);

            ReadLog();
        }

        void ShaderMetricsSystem::Shutdown()
        {
            WriteLog();

            // Unregister the system to the interface.
            Interface<ShaderMetricsSystemInterface>::Unregister(this);
        }

        void ShaderMetricsSystem::Reset()
        {
            m_metrics.m_requests.clear();
        }

        void ShaderMetricsSystem::ReadLog()
        {
            const AZ::IO::FixedMaxPath metricsFilePath = GetMetricsFilePath();

            if (AZ::IO::LocalFileIO::GetInstance()->Exists(metricsFilePath.c_str()))
            {
                auto loadResult = AZ::JsonSerializationUtils::LoadObjectFromFile<ShaderVariantMetrics>(m_metrics, metricsFilePath.c_str());

                if (!loadResult.IsSuccess())
                {
                    AZ_Error("ShaderMetrics", false, "Unable to read %s file", metricsFilePath.c_str());
                    return;
                }
            }
        }

        void ShaderMetricsSystem::WriteLog()
        {
            const AZ::IO::FixedMaxPath metricsFilePath = GetMetricsFilePath();

            auto saveResult = AZ::JsonSerializationUtils::SaveObjectToFile<ShaderVariantMetrics>(&m_metrics, metricsFilePath.c_str());

            if (!saveResult.IsSuccess())
            {
                AZ_Error("ShaderMetrics", false, "Unable to write %s file", metricsFilePath.c_str());
                return;
            }
        }

        bool ShaderMetricsSystem::IsEnabled() const
        {
            return m_isEnabled;
        }

        void ShaderMetricsSystem::SetEnabled(bool value)
        {
            m_isEnabled = value;
        }

        const ShaderVariantMetrics& ShaderMetricsSystem::GetMetrics() const
        {
            return m_metrics;
        }

        void ShaderMetricsSystem::RequestShaderVariant(const ShaderAsset* shader, const ShaderVariantId& shaderVariantId, const ShaderVariantSearchResult& result)
        {
            if (!m_isEnabled)
            {
                return;
            }

            AZ_PROFILE_SCOPE(RPI, "ShaderMetricsSystem: RequestShaderVariant");

            AZStd::lock_guard<AZStd::mutex> lock(m_metricsMutex);

            // Log a new request
            ShaderVariantRequest newRequest;
            newRequest.m_shaderId = shader->GetId();
            newRequest.m_shaderName = shader->GetName();
            newRequest.m_shaderVariantId = shaderVariantId;
            newRequest.m_shaderVariantStableId = result.GetStableId();
            newRequest.m_dynamicOptionCount = result.GetDynamicOptionCount();
            newRequest.m_requestCount = 1;

            // Check if the specific shader variant was already requested to increase its request count
            for (auto request = m_metrics.m_requests.begin(); request != m_metrics.m_requests.end(); ++request)
            {
                if (request->m_shaderId == newRequest.m_shaderId &&
                    request->m_shaderVariantId == newRequest.m_shaderVariantId)
                {
                    request->m_requestCount++;
                    return;
                }
            }

            // Otherwise, add a new request
            m_metrics.m_requests.push_back(newRequest);
        }

    }; // namespace RPI
}; // namespace AZ
