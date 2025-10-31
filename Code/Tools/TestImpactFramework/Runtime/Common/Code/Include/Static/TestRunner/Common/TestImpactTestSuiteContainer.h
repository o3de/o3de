/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        TestSuiteContainer(const TestSuiteContainer&);
        TestSuiteContainer(TestSuiteContainer&&) noexcept;
        TestSuiteContainer(const AZStd::vector<TestSuite>& testSuites);
        TestSuiteContainer(AZStd::vector<TestSuite>&& testSuites) noexcept;

        TestSuiteContainer& operator=(const TestSuiteContainer&);
        TestSuiteContainer& operator=(TestSuiteContainer&&) noexcept;

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

        //! Returns the total number of enabled test suites with one more enabled tests in each test suite.
        size_t GetNumEnabledTestSuites() const;

        //! Returns the total number of disabled test suites and test enabled suites with no enabled tests.
        size_t GetNumDisabledTestSuites() const;

    private:
        void CalculateTestMetrics();

    protected:
        AZStd::vector<TestSuite> m_testSuites;
        size_t m_numDisabledTests = 0;
        size_t m_numEnabledTests = 0;
        size_t m_numEnabledTestSuites = 0;
    };

    template<typename TestSuite>
    TestSuiteContainer<TestSuite>::TestSuiteContainer(TestSuiteContainer&& other) noexcept
        : m_testSuites(AZStd::move(other.m_testSuites))
        , m_numDisabledTests(other.m_numDisabledTests)
        , m_numEnabledTests(other.m_numEnabledTests)
    {
    }

    template<typename TestSuite>
    TestSuiteContainer<TestSuite>::TestSuiteContainer(const TestSuiteContainer& other)
        : m_testSuites(other.m_testSuites.begin(), other.m_testSuites.end())
        , m_numDisabledTests(other.m_numDisabledTests)
        , m_numEnabledTests(other.m_numEnabledTests)
    {
    }

    template<typename TestSuite>
    TestSuiteContainer<TestSuite>::TestSuiteContainer(AZStd::vector<TestSuite>&& testSuites) noexcept
        : m_testSuites(std::move(testSuites))
    {
        CalculateTestMetrics();
    }

    template<typename TestSuite>
    TestSuiteContainer<TestSuite>::TestSuiteContainer(const AZStd::vector<TestSuite>& testSuites)
        : m_testSuites(testSuites)
    {
        CalculateTestMetrics();
    }

    template<typename TestSuite>
    TestSuiteContainer<TestSuite>& TestSuiteContainer<TestSuite>::operator=(TestSuiteContainer&& other) noexcept
    {
        if (this != &other)
        {
            m_testSuites = AZStd::move(other.m_testSuites);
            m_numDisabledTests = other.m_numDisabledTests;
            m_numEnabledTests = other.m_numEnabledTests;
        }

        return *this;
    }

    template<typename TestSuite>
    TestSuiteContainer<TestSuite>& TestSuiteContainer<TestSuite>::operator=(const TestSuiteContainer& other)
    {
        if (this != &other)
        {
            m_testSuites = other.m_testSuites;
            m_numDisabledTests = other.m_numDisabledTests;
            m_numEnabledTests = other.m_numEnabledTests;
        }

        return *this;
    }

    template<typename TestSuite>
    void TestSuiteContainer<TestSuite>::CalculateTestMetrics()
    {
        m_numDisabledTests = 0;
        m_numEnabledTests = 0;
        m_numEnabledTestSuites = 0;

        for (const auto& suite : m_testSuites)
        {
            if (suite.m_enabled)
            {
                const auto enabledTests = std::count_if(suite.m_tests.begin(), suite.m_tests.end(), [](const auto& test)
                {
                    return test.m_enabled;
                });

                m_numEnabledTests += enabledTests;
                m_numDisabledTests += suite.m_tests.size() - enabledTests;

                if (enabledTests > 0)
                {
                    m_numEnabledTestSuites++;
                }
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

    template<typename TestSuite>
    size_t TestSuiteContainer<TestSuite>::GetNumEnabledTestSuites() const
    {
        return m_numEnabledTestSuites;
    }

    template<typename TestSuite>
    size_t TestSuiteContainer<TestSuite>::GetNumDisabledTestSuites() const
    {
        return GetNumTestSuites() - m_numEnabledTestSuites;
    }
} // namespace TestImpact
