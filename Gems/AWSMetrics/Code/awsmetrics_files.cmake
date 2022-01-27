#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    Include/AWSMetricsBus.h
    Include/MetricsAttribute.h

    Source/AWSMetricsConstant.h
    Source/AWSMetricsServiceApi.cpp
    Source/AWSMetricsServiceApi.h
    Source/AWSMetricsSystemComponent.cpp
    Source/AWSMetricsSystemComponent.h
    Source/ClientConfiguration.cpp
    Source/ClientConfiguration.h
    Source/DefaultClientIdProvider.h
    Source/DefaultClientIdProvider.cpp
    Source/GlobalStatistics.h
    Source/IdentityProvider.cpp
    Source/IdentityProvider.h
    Source/MetricsAttribute.cpp
    Source/MetricsEvent.cpp
    Source/MetricsEvent.h
    Source/MetricsEventBuilder.cpp
    Source/MetricsEventBuilder.h
    Source/MetricsManager.cpp
    Source/MetricsManager.h
    Source/MetricsQueue.cpp
    Source/MetricsQueue.h
)
