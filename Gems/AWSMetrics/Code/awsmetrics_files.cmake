#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    Include/Public/AWSMetricsBus.h
    Include/Private/AWSMetricsConstant.h
    Include/Private/AWSMetricsServiceApi.h
    Include/Private/AWSMetricsSystemComponent.h
    Include/Private/ClientConfiguration.h
    Include/Private/DefaultClientIdProvider.h
    Include/Private/GlobalStatistics.h
    Include/Private/IdentityProvider.h
    Include/Private/MetricsAttribute.h
    Include/Private/MetricsEvent.h
    Include/Private/MetricsEventBuilder.h    
    Include/Private/MetricsManager.h
    Include/Private/MetricsQueue.h
    Source/ClientConfiguration.cpp
    Source/DefaultClientIdProvider.cpp
    Source/AWSMetricsServiceApi.cpp
    Source/AWSMetricsSystemComponent.cpp
    Source/IdentityProvider.cpp
    Source/MetricsEvent.cpp
    Source/MetricsEventBuilder.cpp
    Source/MetricsAttribute.cpp
    Source/MetricsManager.cpp
    Source/MetricsQueue.cpp
)
