/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
