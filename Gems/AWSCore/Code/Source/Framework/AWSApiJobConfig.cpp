/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/AzFramework_Traits_Platform.h>

#include <Framework/AWSApiJobConfig.h>
#include <AWSCoreBus.h>
#include <Credential/AWSCredentialBus.h>

namespace AWSCore
{
    void AwsApiJobConfig::ApplySettings()
    {
        m_jobContext = nullptr;

        Visit<AwsApiJobConfig>(
            [this](const AwsApiJobConfig& config) {
                CheckAndSet(config.jobContext, m_jobContext);
            }
        );

        if (!m_jobContext)
        {
            AWSCoreRequestBus::BroadcastResult(m_jobContext, &AWSCoreRequests::GetDefaultJobContext);
        }

        if (!credentialsProvider)
        {
            AWSCredentialResult credentialResult;
            AWSCredentialRequestBus::BroadcastResult(credentialResult, &AWSCredentialRequests::GetCredentialsProvider);
            if (credentialResult.result)
            {
                credentialsProvider = credentialResult.result;
            }
        }

        m_settingsApplied = true;
    }

    Aws::Client::ClientConfiguration AwsApiJobConfig::GetClientConfiguration() const
    {
        Aws::Client::ClientConfiguration target;
        target.enableTcpKeepAlive = AZ_TRAIT_AZFRAMEWORK_AWS_ENABLE_TCP_KEEP_ALIVE_SUPPORTED;
        Visit<AwsApiJobConfig>(
            [&target](const AwsApiJobConfig& config) {
                CheckAndSet(config.userAgent, target.userAgent);
                CheckAndSet(config.scheme, target.scheme);
                CheckAndSet(config.region, target.region);
                CheckAndSet(config.maxConnections, target.maxConnections);
                CheckAndSet(config.requestTimeoutMs, target.requestTimeoutMs);
                CheckAndSet(config.connectTimeoutMs, target.connectTimeoutMs);
                CheckAndSet(config.retryStrategy, target.retryStrategy);
                CheckAndSet(config.endpointOverride, target.endpointOverride);
                CheckAndSet(config.proxyHost, target.proxyHost);
                CheckAndSet(config.proxyPort, target.proxyPort);
                CheckAndSet(config.proxyUserName, target.proxyUserName);
                CheckAndSet(config.proxyPassword, target.proxyPassword);
                CheckAndSet(config.executor, target.executor);
                CheckAndSet(config.verifySSL, target.verifySSL);
                CheckAndSet(config.writeRateLimiter, target.writeRateLimiter);
                CheckAndSet(config.readRateLimiter, target.readRateLimiter);
                CheckAndSet(config.httpLibOverride, target.httpLibOverride);
                CheckAndSet(config.followRedirects, target.followRedirects);
                CheckAndSet(config.caFile, target.caFile);
            }
        );
        return target;
    }

    AZ::JobContext* AwsApiJobConfig::GetJobContext()
    {
        EnsureSettingsApplied();
        return m_jobContext;
    }

    std::shared_ptr<Aws::Auth::AWSCredentialsProvider> AwsApiJobConfig::GetCredentialsProvider() const
    {
        std::shared_ptr<Aws::Auth::AWSCredentialsProvider> target;
        Visit<AwsApiJobConfig>([&target](const AwsApiJobConfig& config) { CheckAndSet(config.credentialsProvider, target); });
        return target;
    }

} // namespace AWSCore

