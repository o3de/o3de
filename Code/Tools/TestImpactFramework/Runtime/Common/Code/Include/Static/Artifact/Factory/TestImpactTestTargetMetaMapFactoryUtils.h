/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactTestSequence.h>

#include <Artifact/Static/TestImpactTestSuiteMeta.h>

#include <AzCore/JSON/document.h>
#include <AzCore/std/optional.h>

namespace TestImpact
{
    //! Extracts the suite labels and places them in a suite label set.
    //! @returns If the label set contains a label in the suite label exclude set, `AZStd::nullopt`. Otherwise, the suite label set.
    std::optional<SuiteLabelSet> ExtractTestSuiteLabelSet(
        const rapidjson_ly::GenericArray<true, rapidjson_ly::Value>& suite, const SuiteLabelExcludeSet& suiteLabelExcludeSet);
} // namespace TestImpact
