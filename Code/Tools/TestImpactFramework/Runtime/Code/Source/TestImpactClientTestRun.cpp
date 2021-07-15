/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactClientTestRun.h>
namespace TestImpact
{
    namespace Client
    {
        TestRun::TestRun(const AZStd::string& name, TestRunResult result, AZStd::chrono::milliseconds duration)
            : m_targetName(name)
            , m_result(result)
            , m_duration(duration)
        {
        }

        const AZStd::string& TestRun::GetTargetName() const
        {
            return m_targetName;
        }

        AZStd::chrono::milliseconds TestRun::GetDuration() const
        {
            return m_duration;
        }

        TestRunResult TestRun::GetResult() const
        {
            return m_result;
        }
    } // namespace Client
} // namespace TestImpact
