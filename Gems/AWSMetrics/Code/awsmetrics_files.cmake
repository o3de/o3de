#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

set(FILES
    Include/Public/AWSMetricsBus.h
    Include/Public/AWSMetricsConstant.h
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
