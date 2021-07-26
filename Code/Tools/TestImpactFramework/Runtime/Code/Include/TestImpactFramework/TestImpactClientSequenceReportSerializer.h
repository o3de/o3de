/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of
 * this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactClientSequenceReport.h>

#include <AzCore/std/string/string.h>

namespace TestImpact
{
    //!
    AZStd::string SerializeSequenceReport(const Client::RegularSequenceReport& sequenceReport);

    //!
    AZStd::string SerializeSequenceReport(const Client::SeedSequenceReport& sequenceReport);

    //!
    AZStd::string SerializeSequenceReport(const Client::ImpactAnalysisSequenceReport& sequenceReport);

    //!
    AZStd::string SerializeSequenceReport(const Client::SafeImpactAnalysisSequenceReport& sequenceReport);

} // namespace TestImpact
