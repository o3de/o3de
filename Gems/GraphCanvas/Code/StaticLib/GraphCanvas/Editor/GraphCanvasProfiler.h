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
#pragma once

#include <AzCore/Debug/Profiler.h>
 
#define GRAPH_CANVAS_PROFILE_FUNCTION() AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
#define GRAPH_CANVAS_PROFILE_SCOPE(message) AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzToolsFramework, message);

#if GRAPH_CANVAS_ENABLE_DETAILED_PROFILING
#define GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION() AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);
#define GRAPH_CANVAS_DETAILED_PROFILE_SCOPE(message) AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzToolsFramework, message);
#else
#define GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION()
#define GRAPH_CANVAS_DETAILED_PROFILE_SCOPE(message)
#endif
