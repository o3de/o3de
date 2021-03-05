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

#include <AzCore/Math/Uuid.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/regex.h>
#include <AzFramework/Dependency/Dependency.h>

#include "GemRegistry/Version.h"

namespace Gems
{
    using EngineSpecifier = AzFramework::Specifier<EngineVersion::parts_count>;
    using GemSpecifier = AzFramework::Specifier<GemVersion::parts_count>;
    using EngineDependency = AzFramework::Dependency<EngineVersion::parts_count>;
    using GemDependency = AzFramework::Dependency<GemVersion::parts_count>;
} // namespace Gems
