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

#include <AzCore/std/containers/vector.h>

namespace TestImpact
{
    //! Encapsulation of test suites into a class with meta-data about each the suites.
    //! @tparam TestSuite The test suite data structure to encapsulate.
    template<typename TestSuite>
    class TestSuiteContainer
    {
    public:
        TestSuiteContainer(AZStd::vector<TestSuite>&& testSuites);

        //! Returns the test suites in this container.
        const AZStd::vector<TestSuite>& GetTestSuites() const;

        //! Returns the number of test suites in this container.
        size_t GetNumTestSuites() const;

        //! Returns the total number of tests across all test suites.
        size_t GetNumTests() const;

        //! Returns the total number of enabled tests across all test suites.
        size_t GetNumEnabledTests() const;

        //! Returns the total number of disabled tests across all test suites.
        size_t GetNumDisabledTests() const;

    protected:
        AZStd::vector<TestSuite> m_testSuites;
        size_t m_numDisabledTests = 0;
        size_t m_numEnabledTests = 0;
    };

    template<typename TestSuite>
    TestSuiteContainer<TestSuite>::TestSuiteContainer(AZStd::vector<TestSuite>&& testSuites)
        : m_testSuites(std::move(testSuites))
    {
        for (const auto& suite : m_testSuites)
        {
            if (suite.m_enabled)
            {
                const auto enabled = std::count_if(suite.m_tests.begin(), suite.m_tests.end(), [](const auto& test)
                {
                    return test.m_enabled;
                });

                m_numEnabledTests += enabled;
                m_numDisabledTests += suite.m_tests.size() - enabled;
            }
            else
            {
                // Disabled status of suites propagates down to all tests regardless of whether or not each individual test is disabled
                m_numDisabledTests += suite.m_tests.size();
            }
        }
    }

    template<typename TestSuite>
    const AZStd::vector<TestSuite>& TestSuiteContainer<TestSuite>::GetTestSuites() const
    {
        return m_testSuites;
    }

    template<typename TestSuite>
    size_t TestSuiteContainer<TestSuite>::GetNumTests() const
    {
        return m_numEnabledTests + m_numDisabledTests;
    }

    template<typename TestSuite>
    size_t TestSuiteContainer<TestSuite>::GetNumEnabledTests() const
    {
        return m_numEnabledTests;
    }

    template<typename TestSuite>
    size_t TestSuiteContainer<TestSuite>::GetNumDisabledTests() const
    {
        return m_numDisabledTests;
    }

    template<typename TestSuite>
    size_t TestSuiteContainer<TestSuite>::GetNumTestSuites() const
    {
        return m_testSuites.size();
    }
} // namespace TestImpact
