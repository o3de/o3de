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

#include <TestImpactFramework/TestImpactChangeList.h>

#include <AzCore/std/string/string.h>

namespace TestImpact
{
    //! Serializes the specified change list to JSON format.
    AZStd::string SerializeChangeList(const ChangeList& changeList);

    //! Deserializes a change list from the specified test run data in JSON format.
    ChangeList DeserializeChangeList(const AZStd::string& changeListString);
} // namespace TestImpact
