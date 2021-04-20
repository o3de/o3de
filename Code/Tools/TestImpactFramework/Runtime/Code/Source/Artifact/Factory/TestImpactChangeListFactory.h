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

#include <Artifact/Dynamic/TestImpactChangeList.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace TestImpact
{
    namespace UnifiedDiff
    {
        //! Constructs a change list artifact from the specified unified diff data.
        //! @param unifiedDiffData The raw change list data in unified diff format.
        //! @return The constructed change list artifact.
        ChangeList ChangeListFactory(const AZStd::string& unifiedDiffData);
    } // namespace UnifiedDiff
} // namespace TestImpact
