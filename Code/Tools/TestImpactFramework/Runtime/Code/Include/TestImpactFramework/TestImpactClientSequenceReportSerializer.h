/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactClientSequenceReport.h>

#include <AzCore/std/string/string.h>

namespace TestImpact
{
    //! Serializes a regular sequence report to JSON format.
    AZStd::string SerializeSequenceReport(const Client::RegularSequenceReport& sequenceReport);

    //! Serializes a seed sequence report to JSON format.
    AZStd::string SerializeSequenceReport(const Client::SeedSequenceReport& sequenceReport);

    //! Serializes an impact analysis sequence report to JSON format.
    AZStd::string SerializeSequenceReport(const Client::ImpactAnalysisSequenceReport& sequenceReport);

    //! Serializes a safe impact analysis sequence report to JSON format.
    AZStd::string SerializeSequenceReport(const Client::SafeImpactAnalysisSequenceReport& sequenceReport);

    //! Deserialize a regular sequence report from JSON format.
    Client::RegularSequenceReport DeserializeRegularSequenceReport(const AZStd::string& sequenceReportJSON);

    //! Deserialize a seed sequence report from JSON format.
    Client::SeedSequenceReport DeserializeSeedSequenceReport(const AZStd::string& sequenceReportJSON);

    //! Deserialize an impact analysis sequence report from JSON format.
    Client::ImpactAnalysisSequenceReport DeserializeImpactAnalysisSequenceReport(const AZStd::string& sequenceReportJSON);

    //! Deserialize a safe impact analysis sequence report from JSON format.
    Client::SafeImpactAnalysisSequenceReport DeserializeSafeImpactAnalysisSequenceReport(const AZStd::string& sequenceReportJSON);
} // namespace TestImpact
