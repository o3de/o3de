/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Dependency/TestImpactSourceCoveringTestsList.h>

#include <AzCore/std/string/string.h>

namespace TestImpact
{
    //! Serializes the specified source covering tests list to plain text format.
    AZStd::string SerializeSourceCoveringTestsList(const SourceCoveringTestsList& sourceCoveringTestsList);

    //! Deserializes a source covering tests list from the specified source covering tests data in plain text format.
    SourceCoveringTestsList DeserializeSourceCoveringTestsList(const AZStd::string& sourceCoveringTestsListString);
} // namespace TestImpact
