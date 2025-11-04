/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Artifact/Factory/TestImpactTestTargetMetaMapFactoryUtils.h>

namespace TestImpact
{
    std::optional<SuiteLabelSet> ExtractTestSuiteLabelSet(
        const rapidjson_ly::GenericArray<true, rapidjson_ly::Value>& suiteLabels, const SuiteLabelExcludeSet& suiteLabelExcludeSet)
    {
        SuiteLabelSet labelSet;
        for (const auto& label : suiteLabels)
        {
            const auto labelString = label.GetString();
            if (suiteLabelExcludeSet.contains(labelString))
            {
                return AZStd::nullopt;
            }

            labelSet.insert(labelString);
        }

        // Only test suite labels that contain the TIAF requirement label
        return labelSet.contains(RequiresTiafLabel) ? AZStd::optional<SuiteLabelSet>{ labelSet } : AZStd::nullopt;
    }
} // namespace TestImpact
