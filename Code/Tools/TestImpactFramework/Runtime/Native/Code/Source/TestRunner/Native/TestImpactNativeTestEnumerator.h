/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestRunner/Common/TestImpactTestEnumerator.h>
#include <TestRunner/Common/Job/TestImpactTestEnumerationJobData.h>


#include <TestImpactFramework/TestImpactUtils.h>
#include <TestRunner/Common/Enumeration/TestImpactTestEnumerationSerializer.h>
#include <Artifact/Factory/TestImpactTestEnumerationSuiteFactory.h>

namespace TestImpact
{
    struct NativeTestEnumerationJobData
        : public TestEnumerationJobData
    {
        using TestEnumerationJobData::TestEnumerationJobData;
    };

    class NativeTestEnumerator
        : public TestEnumerator<NativeTestEnumerationJobData>
    {
    public:
        using TestEnumerator<NativeTestEnumerationJobData>::TestEnumerator;
    };

    template<>
    inline PayloadOutcome<TestEnumeration> PayloadFactory(const JobInfo<NativeTestEnumerationJobData>& jobData, [[maybe_unused]]const JobMeta& jobMeta)
    {
        try
        {
            return AZ::Success(TestEnumeration(
                GTest::TestEnumerationSuitesFactory(ReadFileContents<TestRunnerException>(jobData.GetEnumerationArtifactPath()))));
        } catch (const Exception& e)
        {
            return AZ::Failure(AZStd::string::format("%s\n", e.what()));
        }
    };
} // namespace TestImpact
