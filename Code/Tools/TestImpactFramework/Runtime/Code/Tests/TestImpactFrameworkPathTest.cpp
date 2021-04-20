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

#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>
#include <TestImpactFramework/TestImpactFrameworkPath.h>

namespace UnitTest
{
    class FrameworkPathTestFixture
        : public AllocatorsTestFixture
    {
    public:
        void SetUp() override
        {
            UnitTest::AllocatorsTestFixture::SetUp();

            m_parentPathAbs = AZStd::string(AZStd::string("parent") + AZ_TRAIT_OS_PATH_SEPARATOR + "path");
            m_childPathRel = AZStd::string(AZStd::string("child") + AZ_TRAIT_OS_PATH_SEPARATOR + "path");
            m_childPathAbs = AZStd::string(m_parentPathAbs + AZ_TRAIT_OS_PATH_SEPARATOR + m_childPathRel);

            m_pathComponentA = AZStd::string("DirA");
            m_pathComponentB = AZStd::string("DirB");
            m_pathComponentC = AZStd::string("DirC");

            m_posixPath = m_pathComponentA + AZ::IO::PosixPathSeparator + m_pathComponentB + AZ::IO::PosixPathSeparator + m_pathComponentC;

            m_windowsPath =
                m_pathComponentA + AZ::IO::WindowsPathSeparator + m_pathComponentB + AZ::IO::WindowsPathSeparator + m_pathComponentC;

            m_mixedPath =
                m_pathComponentA + AZ::IO::WindowsPathSeparator + m_pathComponentB + AZ::IO::PosixPathSeparator + m_pathComponentC;

            m_referredPath =
                m_pathComponentA + AZ_TRAIT_OS_PATH_SEPARATOR + m_pathComponentB + AZ_TRAIT_OS_PATH_SEPARATOR + m_pathComponentC;
        }

        void TearDown() override
        {
            UnitTest::AllocatorsTestFixture::TearDown();
        }

        AZStd::string m_parentPathAbs;
        AZStd::string m_childPathRel;
        AZStd::string m_childPathAbs;

        AZStd::string m_pathComponentA;
        AZStd::string m_pathComponentB;
        AZStd::string m_pathComponentC;

        AZ::IO::Path m_posixPath;
        AZ::IO::Path m_windowsPath;
        AZ::IO::Path m_mixedPath;
        AZ::IO::Path m_referredPath;
    };

    TEST_F(FrameworkPathTestFixture, DefaultConstructor_HasEmptyAbsAndRelPaths)
    {
        // Given an empty framework path
        TestImpact::FrameworkPath path;

        // Expect the absolute path to be empty
        EXPECT_TRUE(path.Absolute().empty());

        // Expect the relative path to be empty
        EXPECT_TRUE(path.Relative().empty());
    }

    TEST_F(FrameworkPathTestFixture, OrphanConstructor_HasAbsAndEmptyRelPaths)
    {
        // Given an orhpan framework path
        TestImpact::FrameworkPath path(m_parentPathAbs);

        // Expect the absolute path to be equal to the specified path
        EXPECT_EQ(path.Absolute().String(), m_parentPathAbs);

        // Expect the relative path to be current directory symbol
        EXPECT_STREQ(path.Relative().c_str(), ".");
    }

    TEST_F(FrameworkPathTestFixture, ParentConstructor_HasAbsAndRelPaths)
    {
        // Given a child framework path
        TestImpact::FrameworkPath path(m_childPathAbs, TestImpact::FrameworkPath(m_parentPathAbs));

        // Expect the absolute path to be equal to the concatenation of the parent and child path
        EXPECT_EQ(path.Absolute().String(), m_childPathAbs);

        // Expect the relative path to equal to the specified path
        EXPECT_EQ(path.Relative().String(), m_childPathRel);
    }

    TEST_F(FrameworkPathTestFixture, PosixSeperators_HasUniformPreferredSeperators)
    {
        // Given an orhpan framework path with Posix separators
        TestImpact::FrameworkPath path(m_posixPath);

        // Expect the absolute path to be equal to the specified path with preferred separators
        EXPECT_EQ(path.Absolute(), m_referredPath);

        // Expect the relative path to be current directory symbol
        EXPECT_STREQ(path.Relative().c_str(), ".");
    }

    TEST_F(FrameworkPathTestFixture, WindowsSeperators_HasUniformPreferredSeperators)
    {
        // Given an orhpan framework path with Windows separators
        TestImpact::FrameworkPath path(m_windowsPath);

        // Expect the absolute path to be equal to the specified path with preferred separators
        EXPECT_EQ(path.Absolute(), m_referredPath);

        // Expect the relative path to be current directory symbol
        EXPECT_STREQ(path.Relative().c_str(), ".");
    }

    TEST_F(FrameworkPathTestFixture, MixedSeperators_HasUniformPreferredSeperators)
    {
        // Given an orhpan framework path with mixed separators
        TestImpact::FrameworkPath path(m_windowsPath);

        // Expect the absolute path to be equal to the specified path with preferred separators
        EXPECT_EQ(path.Absolute(), m_referredPath);

        // Expect the relative path to be current directory symbol
        EXPECT_STREQ(path.Relative().c_str(), ".");
    }
} // namespace UnitTest
