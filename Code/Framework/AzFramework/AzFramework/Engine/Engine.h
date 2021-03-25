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

#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/string/string_view.h>

namespace AzFramework
{
    namespace Engine
    {
        // Helper to attempt to locate the engine root by searching up the directory tree.  If no search path is
        // provided the current executable path is used
        AZ::IO::FixedMaxPath FindEngineRoot(AZStd::string_view searchPath = {});
    } // Engine
} // AzFramework
