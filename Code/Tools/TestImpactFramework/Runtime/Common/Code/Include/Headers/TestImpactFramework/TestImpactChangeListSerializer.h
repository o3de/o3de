/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
