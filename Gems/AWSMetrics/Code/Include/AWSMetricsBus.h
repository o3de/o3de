/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <MetricsAttribute.h>

#include <AzCore/EBus/EBus.h>

namespace AWSMetrics
{
    //! AWSMetrics request interface
    class AWSMetricsRequests
        : public AZ::EBusTraits
    {
    public:
        // Allow multiple threads to concurrently make requests
        using MutexType = AZStd::recursive_mutex;

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        //! Submit metrics to the backend or a local file.
        //! @param eventName Name of the metrics event.
        //! @param metricsAttributes Attributes of the metrics.
        //! @param eventPriority Priority of the event. Default to 0 which is considered as the highest priority.
        //! @param eventSourceOverride Event source used to override the default value.
        //! @param bufferMetrics Whether to buffer metrics and send them in batch.
        //! @return Whether the request is sent successfully.
        virtual bool SubmitMetrics(const AZStd::vector<MetricsAttribute>& metricsAttributes, int eventPriority = 0,
            const AZStd::string& eventSourceOverride = "", bool bufferMetrics = true)
        {
            AZ_UNUSED(metricsAttributes);
            AZ_UNUSED(eventPriority);
            AZ_UNUSED(eventSourceOverride);
            AZ_UNUSED(bufferMetrics);

            return true;
        };

        //! Flush all metrics buffered in memory.
        virtual void FlushMetrics()
        {
        };
    };

    using AWSMetricsRequestBus = AZ::EBus<AWSMetricsRequests>;

    //! Bus used to send notifications about the result of AWSMetrics requests
    class AWSMetricsNotifications
        : public AZ::EBusTraits
    {
    public:
        // Allow multiple threads to concurrently send notification
        using MutexType = AZStd::recursive_mutex;

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        //! Notification for sending metrics successfully.
        //! @param requestId Id of the request.
        virtual void OnSendMetricsSuccess(int requestId)
        {
            AZ_UNUSED(requestId);
        }

        //! Notification for failing to send metrics.
        //! @param requestId Id of the request.
        virtual void OnSendMetricsFailure(int requestId, const AZStd::string& errorMessage)
        {
            AZ_UNUSED(requestId);
            AZ_UNUSED(errorMessage);
        }
    };

    using AWSMetricsNotificationBus = AZ::EBus<AWSMetricsNotifications>;

} // namespace AWSMetrics
