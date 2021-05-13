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

#include <Target/TestImpactTestTarget.h>
#include <TestEngine/Enumeration/TestImpactTestEnumerator.h>
#include <TestEngine/Run/TestImpactTestRunner.h>

#include <AzCore/std/containers/vector.h>

namespace TestImpact
{
    enum class TestRunResult;

    class TestEngine
    {
    public:
        ~TestEngine();

        void UpdateEnumerationCache(const AZStd::vector<const TestTarget*> testTargetsToEnumerate);

    private:
        TestEnumerator m_testEnumerator;
        TestRunner m_testRunner;
    };
}
