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

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/hash.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/array.h>

#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/Dependency/Version.h>

#include <initializer_list>
#include <sstream>

namespace Gems
{
    using GemVersion = AzFramework::SemanticVersion;

    // /**
    //  * Represents a version of the Lumberyard Game Engine
    //  */
    using EngineVersion = AzFramework::Version<4>;
} // namespace Gems

namespace AZStd
{
    template <size_t N>
    struct hash<AzFramework::Version<N>>
    {
        size_t operator()(const AzFramework::Version<N>& ver) const
        {
            return AZStd::hash_range(ver.m_parts.begin(), ver.m_parts.end());
        }
    };

    template <>
    struct hash<AzFramework::SemanticVersion>
        : public hash<AzFramework::Version<3>>
    { };
} // namespace AZStd
