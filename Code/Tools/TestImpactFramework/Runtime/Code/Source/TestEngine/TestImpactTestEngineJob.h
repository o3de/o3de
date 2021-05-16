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

#include <Process/JobRunner/TestImpactProcessJobMeta.h>

namespace TestImpact
{
    class TestTarget;

    class TestEngineJob
        : public JobMetaContainer
    {
    public:
        TestEngineJob(const TestTarget* testTarget, const AZStd::string& commandString, const JobMeta& jobMeta);
        const TestTarget* GetTestTarget() const;
        const AZStd::string& GetCommandString() const;
    private:
        const TestTarget* m_testTarget;
        AZStd::string m_commandString;
    };
} // namespace TestImpact
