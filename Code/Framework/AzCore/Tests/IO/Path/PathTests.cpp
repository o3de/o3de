/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>

#include <AzCore/IO/Path/Path.h>
#include <AzCore/StringFunc/StringFunc.h>

namespace UnitTest
{
#if !AZ_UNIT_TEST_SKIP_PATH_TESTS

    class PathFixture
        : public ScopedAllocatorSetupFixture
    {};

    TEST_F(PathFixture, AppendWithFixedPath_IsConstexpr_Compiles)
    {
        using path = AZ::IO::FixedMaxPath;
        constexpr const char* input1 = "C:\\";
        constexpr AZStd::string_view input2 = "test";
        constexpr AZStd::fixed_string<32> input3 = "dev";

        constexpr auto resultPath = path(input1) / input2 / input3;

        constexpr const char* expected = "C:" AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING "test" AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING "dev";
        constexpr auto normalPath = resultPath.LexicallyNormal();
        static_assert(normalPath.Native() == expected);
        EXPECT_STREQ(expected, normalPath.c_str());
    }

    TEST_F(PathFixture, AppendInplaceWithFixedPath_IsConstexpr_Compiles)
    {
        auto JoinLambda = []() constexpr->AZ::IO::FixedMaxPath
        {
            using path = AZ::IO::FixedMaxPath;
            constexpr const char* input1 = "Cache";
            constexpr AZStd::string_view input2 = "StarterGame";
            constexpr AZStd::fixed_string<32> input3 = "pc/startergame";
            path resultPath = "F:\\EngineRoot/";
            resultPath /= path(input1) / path(input2) / path(input3);
            return resultPath.MakePreferred();
        };

        constexpr const char* expected = "F:" AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING "EngineRoot" AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING "Cache"
            AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING "StarterGame" AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING "pc" AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING "startergame";

        constexpr auto resultPath = JoinLambda();
        static_assert(resultPath.Native() == expected);
        EXPECT_STREQ(expected, resultPath.LexicallyNormal().c_str());
    }


    // PathView::IsAbsolute test
    TEST_F(PathFixture, IsAbsolute_ReturnsTrue)
    {
        using fixed_max_path = AZ::IO::FixedMaxPath;

        auto IsAbsolute = []() constexpr -> bool
        {
#if AZ_TRAIT_USE_WINDOWS_FILE_API
            {
                static_assert(!fixed_max_path(AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING).IsAbsolute());
                static_assert(!fixed_max_path(AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING "content").IsAbsolute());
                static_assert(fixed_max_path("C:" AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING).IsAbsolute());
                static_assert(fixed_max_path("C:" AZ_WRONG_FILESYSTEM_SEPARATOR_STRING).IsAbsolute());
                static_assert(fixed_max_path("C:" AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING "content").IsAbsolute());
                static_assert(fixed_max_path("C:" AZ_WRONG_FILESYSTEM_SEPARATOR_STRING "content").IsAbsolute());
                static_assert(fixed_max_path(AZ_NETWORK_PATH_START "server").IsAbsolute());
                static_assert(fixed_max_path(AZ_NETWORK_PATH_START "?\\device").IsAbsolute());
                static_assert(fixed_max_path(AZ_NETWORK_PATH_START ".\\device").IsAbsolute());
                static_assert(fixed_max_path("\\??\\device").IsAbsolute());
            }
#else
            {
                static_assert(fixed_max_path(AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING).IsAbsolute());
                static_assert(fixed_max_path(AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING "content").IsAbsolute());
            }
#endif
            return true;
        };

        static_assert(IsAbsolute());
    }

    // PathView::isRelative test
    TEST_F(PathFixture, IsRelative_ReturnsTrue)
    {
        using fixed_max_path = AZ::IO::FixedMaxPath;
        auto IsRelative = []() constexpr -> bool
        {
            static_assert(fixed_max_path("").IsRelative());
            static_assert(fixed_max_path("." AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING).IsRelative());
            static_assert(fixed_max_path("." AZ_WRONG_FILESYSTEM_SEPARATOR_STRING).IsRelative());
            static_assert(fixed_max_path(".." AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING).IsRelative());
            static_assert(fixed_max_path(".." AZ_WRONG_FILESYSTEM_SEPARATOR_STRING).IsRelative());
            static_assert(fixed_max_path("foo").IsRelative());
            static_assert(fixed_max_path("foo" AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING).IsRelative());
            static_assert(fixed_max_path("foo" AZ_WRONG_FILESYSTEM_SEPARATOR_STRING).IsRelative());
            static_assert(fixed_max_path("test" AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING "dir" AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING).IsRelative());
            static_assert(fixed_max_path("foo" AZ_WRONG_FILESYSTEM_SEPARATOR_STRING "dir" AZ_WRONG_FILESYSTEM_SEPARATOR_STRING).IsRelative());
            static_assert(fixed_max_path("C:").IsRelative());
            return true;
        };

        EXPECT_TRUE(fixed_max_path("").IsRelative());
        static_assert(IsRelative());
    }

    class PathParamFixture
        : public ScopedAllocatorSetupFixture
        , public ::testing::WithParamInterface<AZStd::tuple<AZStd::string_view, AZStd::string_view>>
    {};

    TEST_P(PathParamFixture, Compare_ResultInEqualPaths)
    {
        AZ::IO::Path path1{ AZStd::get<0>(GetParam()) };
        AZ::IO::Path path2{ AZStd::get<1>(GetParam()) };
        EXPECT_EQ(0, path1.Compare(path2));
    }

    TEST_P(PathParamFixture, OperatorEquals_ResultInEqualPaths)
    {
        AZ::IO::Path path1{ AZStd::get<0>(GetParam()) };
        AZ::IO::Path path2{ AZStd::get<1>(GetParam()) };
        EXPECT_EQ(path1, path2);

        AZ::IO::PathView pathView1{ AZStd::get<0>(GetParam()) };
        AZ::IO::PathView pathView2{ AZStd::get<1>(GetParam()) };
        EXPECT_EQ(pathView1, path2);
        EXPECT_EQ(pathView1, path1);
        EXPECT_EQ(pathView1, pathView2);
        EXPECT_EQ(path1, pathView2);
        EXPECT_EQ(path2, pathView2);
    }

    INSTANTIATE_TEST_CASE_P(
        ComparePaths,
        PathParamFixture,
        ::testing::Values(
            AZStd::tuple<AZStd::string_view, AZStd::string_view>("test/foo", "test/foo"),
            AZStd::tuple<AZStd::string_view, AZStd::string_view>("test/foo", "test\\foo"),
            AZStd::tuple<AZStd::string_view, AZStd::string_view>("test////foo", "test///foo"),
            AZStd::tuple<AZStd::string_view, AZStd::string_view>("test/bar/baz//foo", "test/bar/baz\\\\\\foo")
        ));

    TEST_F(PathFixture, ComparisonOperators_Succeed)
    {
        constexpr AZ::IO::FixedMaxPath path1{ "foo/bar" };
        constexpr AZ::IO::FixedMaxPath path2{ "foo/bap" };
        constexpr AZ::IO::PathView pathView{ "foo/bar" };
        EXPECT_EQ(path1, pathView);
        EXPECT_NE(path1, path2);
        EXPECT_LT(path2, path1);
        EXPECT_LE(pathView, path1);
        EXPECT_GT(path1, path2);
        EXPECT_GE(pathView, path2);

        EXPECT_LE(pathView, pathView);
        EXPECT_GE(pathView, pathView);
    }

    using WindowsPathCompareParamFixture = PathParamFixture;

    TEST_P(WindowsPathCompareParamFixture, OperatorEqual_ComparesPathCaseInsensitively)
    {
        AZ::IO::Path path1{ AZStd::get<0>(GetParam()), AZ::IO::WindowsPathSeparator };
        AZ::IO::Path path2{ AZStd::get<1>(GetParam()), AZ::IO::WindowsPathSeparator };
        EXPECT_EQ(path1, path2);
    }

    INSTANTIATE_TEST_CASE_P(
        CompareWindowsPaths,
        WindowsPathCompareParamFixture,
        ::testing::Values(
            AZStd::tuple<AZStd::string_view, AZStd::string_view>("C:/test/foo", R"(c:\test/foo)"),
            AZStd::tuple<AZStd::string_view, AZStd::string_view>(R"(D:\test/bar/baz//foo)", "d:/test/bar/baz\\\\\\foo"),
            AZStd::tuple<AZStd::string_view, AZStd::string_view>(R"(foO/Bar)", "foo/bar")
        ));

    using PathHashParamFixture = PathParamFixture;
    TEST_P(PathHashParamFixture, HashOperator_HashesCaseInsensitiveForWindowsPaths)
    {
        AZ::IO::Path path1{ AZStd::get<0>(GetParam()), AZ::IO::WindowsPathSeparator };
        AZ::IO::Path path2{ AZStd::get<1>(GetParam()), AZ::IO::WindowsPathSeparator };
        size_t path1Hash = AZStd::hash<AZ::IO::PathView>{}(path1);
        size_t path2Hash = AZStd::hash<AZ::IO::PathView>{}(path2);
        EXPECT_EQ(path1Hash, path2Hash) << AZStd::string::format(R"(path1 "%s" should hash to path2 "%s"\n)",
            path1.c_str(), path2.c_str()).c_str();
    }

    TEST_P(PathHashParamFixture, HashOperator_HashesCaseSensitiveForPosixPaths)
    {
        AZ::IO::Path path1{ AZStd::get<0>(GetParam()), AZ::IO::PosixPathSeparator };
        AZ::IO::Path path2{ AZStd::get<1>(GetParam()), AZ::IO::PosixPathSeparator };
        size_t path1Hash = AZStd::hash<AZ::IO::PathView>{}(path1);
        size_t path2Hash = AZStd::hash<AZ::IO::PathView>{}(path2);
        EXPECT_NE(path1Hash, path2Hash) << AZStd::string::format(R"(path1 "%s" should NOT hash to path2 "%s"\n)",
            path1.c_str(), path2.c_str()).c_str();
    }

    INSTANTIATE_TEST_CASE_P(
        HashPaths,
        PathHashParamFixture,
        ::testing::Values(
            AZStd::tuple<AZStd::string_view, AZStd::string_view>("C:/test/foo", R"(c:\test/foo)"),
            AZStd::tuple<AZStd::string_view, AZStd::string_view>(R"(D:\test/bar/baz//foo)", "d:/test/bar/baz\\\\\\foo"),
            AZStd::tuple<AZStd::string_view, AZStd::string_view>(R"(foO/Bar)", "foo/bar")
        ));


    struct PathHashCompareParams
    {
        AZ::IO::PathView m_testPath{};
        ::testing::Matcher<AZ::IO::PathView> m_compareMatcher;
        ::testing::Matcher<size_t> m_hashMatcher;
    };

    class PathHashCompareFixture
        : public ScopedAllocatorSetupFixture
        , public ::testing::WithParamInterface<PathHashCompareParams>
    {};

    // Verifies that two paths that compare equal has their hash value compare equal
    TEST_P(PathHashCompareFixture, PathsWhichCompareEqual_HashesToSameValue_Succeeds)
    {
        auto&& [testPath1, compareMatcher, hashMatcher] = GetParam();

        // Compare path using parameterized Matcher
        EXPECT_THAT(testPath1, compareMatcher);
        // Compare hash using parameterized Matcher
        const size_t testPath1Hash = AZStd::hash<AZ::IO::PathView>{}(testPath1);
AZ_PUSH_DISABLE_WARNING(4296, "-Wunknown-warning-option")
        EXPECT_THAT(testPath1Hash, hashMatcher);
AZ_POP_DISABLE_WARNING
    }

    INSTANTIATE_TEST_CASE_P(
        HashPathCompareValidation,
        PathHashCompareFixture,
        ::testing::Values(
            PathHashCompareParams{ AZ::IO::PathView("C:/test/foo", AZ::IO::WindowsPathSeparator),
                testing::Eq(AZ::IO::PathView(R"(c:\test/foo)", AZ::IO::WindowsPathSeparator)),
                testing::Eq(AZStd::hash<AZ::IO::PathView>{}(AZ::IO::PathView(R"(c:\test/foo)", AZ::IO::WindowsPathSeparator))) },
            PathHashCompareParams{ AZ::IO::PathView("/test/foo", AZ::IO::WindowsPathSeparator),
                testing::Eq(AZ::IO::PathView(R"(/test/FOO)", AZ::IO::WindowsPathSeparator)),
                testing::Eq(AZStd::hash<AZ::IO::PathView>{}(AZ::IO::PathView(R"(/test/FOO)", AZ::IO::WindowsPathSeparator))) },
            PathHashCompareParams{ AZ::IO::PathView("C:/test/foo", AZ::IO::WindowsPathSeparator),
                testing::Eq(AZ::IO::PathView(R"(c:\test/foo)", AZ::IO::WindowsPathSeparator)),
                testing::Eq(AZStd::hash<AZ::IO::PathView>{}(AZ::IO::PathView(R"(c:\test/foo)", AZ::IO::WindowsPathSeparator))) },
            PathHashCompareParams{ AZ::IO::PathView("C:/test/foo", AZ::IO::PosixPathSeparator),
                testing::Ne(AZ::IO::PathView(R"(c:\test/foo)", AZ::IO::WindowsPathSeparator)),
                testing::Ne(AZStd::hash<AZ::IO::PathView>{}(AZ::IO::PathView(R"(c:\test/foo)", AZ::IO::WindowsPathSeparator))) },
            PathHashCompareParams{ AZ::IO::PathView(R"(C:\test\foo)", AZ::IO::WindowsPathSeparator),
                testing::Ne(AZ::IO::PathView(R"(c:/test/foo)", AZ::IO::PosixPathSeparator)),
                testing::Ne(AZStd::hash<AZ::IO::PathView>{}(AZ::IO::PathView(R"(c:/test/foo)", AZ::IO::PosixPathSeparator))) },
            PathHashCompareParams{ AZ::IO::PathView("/test/aoo", AZ::IO::WindowsPathSeparator),
                testing::Eq(AZ::IO::PathView(R"(/test/AOO)", AZ::IO::WindowsPathSeparator)),
                testing::Eq(AZStd::hash<AZ::IO::PathView>{}(AZ::IO::PathView(R"(/test/AOO)", AZ::IO::WindowsPathSeparator))) },
            PathHashCompareParams{ AZ::IO::PathView("/test/aoo", AZ::IO::PosixPathSeparator),
                testing::Gt(AZ::IO::PathView(R"(/test/AOO)", AZ::IO::WindowsPathSeparator)),
                testing::Eq(AZStd::hash<AZ::IO::PathView>{}(AZ::IO::PathView(R"(/test/AOO)", AZ::IO::WindowsPathSeparator))) },
            PathHashCompareParams{ AZ::IO::PathView("/test/aoo", AZ::IO::WindowsPathSeparator),
                testing::Gt(AZ::IO::PathView(R"(/test/AOO)", AZ::IO::PosixPathSeparator)),
                testing::Ne(AZStd::hash<AZ::IO::PathView>{}(AZ::IO::PathView(R"(/test/AOO)", AZ::IO::PosixPathSeparator))) },
            PathHashCompareParams{ AZ::IO::PathView("/test/AOO", AZ::IO::PosixPathSeparator),
                testing::Lt(AZ::IO::PathView(R"(/test/aoo)", AZ::IO::WindowsPathSeparator)),
                testing::Ne(AZStd::hash<AZ::IO::PathView>{}(AZ::IO::PathView(R"(/test/aoo)", AZ::IO::WindowsPathSeparator))) },
            PathHashCompareParams{ AZ::IO::PathView("/test/AOO", AZ::IO::WindowsPathSeparator),
                testing::Lt(AZ::IO::PathView(R"(/test/aoo)", AZ::IO::PosixPathSeparator)),
                testing::Eq(AZStd::hash<AZ::IO::PathView>{}(AZ::IO::PathView(R"(/test/aoo)", AZ::IO::PosixPathSeparator))) },
            // Paths with different character values, comparison based on path separator
            PathHashCompareParams{ AZ::IO::PathView("/test/BOO", AZ::IO::PosixPathSeparator),
                testing::Le(AZ::IO::PathView(R"(/test/aoo)", AZ::IO::WindowsPathSeparator)),
                testing::Ne(AZStd::hash<AZ::IO::PathView>{}(AZ::IO::PathView(R"(/test/aoo)", AZ::IO::WindowsPathSeparator))) },
            PathHashCompareParams{ AZ::IO::PathView("/test/BOO", AZ::IO::WindowsPathSeparator),
                testing::Ge(AZ::IO::PathView(R"(/test/aoo)", AZ::IO::WindowsPathSeparator)),
                testing::Ne(AZStd::hash<AZ::IO::PathView>{}(AZ::IO::PathView(R"(/test/aoo)", AZ::IO::WindowsPathSeparator))) },
            PathHashCompareParams{ AZ::IO::PathView("/test/aoo", AZ::IO::WindowsPathSeparator),
                testing::Le(AZ::IO::PathView(R"(/test/Boo)", AZ::IO::WindowsPathSeparator)),
                testing::Ne(AZStd::hash<AZ::IO::PathView>{}(AZ::IO::PathView(R"(/test/Boo)", AZ::IO::WindowsPathSeparator))) },
            PathHashCompareParams{ AZ::IO::PathView("/test/aoo", AZ::IO::PosixPathSeparator),
                testing::Ge(AZ::IO::PathView(R"(/test/Boo)", AZ::IO::WindowsPathSeparator)),
                testing::Ne(AZStd::hash<AZ::IO::PathView>{}(AZ::IO::PathView(R"(/test/Boo)", AZ::IO::WindowsPathSeparator))) }
        ));

    class PathSingleParamFixture
        : public ScopedAllocatorSetupFixture
        , public ::testing::WithParamInterface<AZStd::tuple<AZStd::string_view>>
    {};

    using WindowsPathRelativeParamFixture = PathSingleParamFixture;

    TEST_P(WindowsPathRelativeParamFixture, IsRelativeRuntime_Succeeds)
    {
        AZ::IO::FixedMaxPath testPath{ AZStd::get<0>(GetParam()), '\\' };
        EXPECT_TRUE(testPath.IsRelative());
    }

    INSTANTIATE_TEST_CASE_P(
        RelativePaths,
        WindowsPathRelativeParamFixture,
        ::testing::Values(
            AZStd::tuple<AZStd::string_view>("test/foo"),
            AZStd::tuple<AZStd::string_view>("test\\foo"),
            AZStd::tuple<AZStd::string_view>(""),
            AZStd::tuple<AZStd::string_view>("."),
            AZStd::tuple<AZStd::string_view>(".."),
            AZStd::tuple<AZStd::string_view>(".."),
            AZStd::tuple<AZStd::string_view>("C:"),
            AZStd::tuple<AZStd::string_view>("D:foo"),
            AZStd::tuple<AZStd::string_view>("D:foo/"),
            AZStd::tuple<AZStd::string_view>("D:foo\\")
        ));

    using WindowsPathAbsoluteParamFixture = PathSingleParamFixture;

    TEST_P(WindowsPathAbsoluteParamFixture, IsAbsolutePath_Succeeds)
    {
        AZ::IO::FixedMaxPath testPath{ AZStd::get<0>(GetParam()), '\\' };
        EXPECT_TRUE(testPath.IsAbsolute());
    }
    TEST_P(WindowsPathAbsoluteParamFixture, IsAbsolutePathView_Succeeds)
    {
        AZ::IO::PathView testPath{ AZStd::get<0>(GetParam()), '\\' };
        EXPECT_TRUE(testPath.IsAbsolute());
    }

    INSTANTIATE_TEST_CASE_P(
        AbsolutePaths,
        WindowsPathAbsoluteParamFixture,
        ::testing::Values(
            AZStd::tuple<AZStd::string_view>("C:/"),
            AZStd::tuple<AZStd::string_view>("C:\\"),
            AZStd::tuple<AZStd::string_view>("C:/content"),
            AZStd::tuple<AZStd::string_view>("C:\\content"),
            AZStd::tuple<AZStd::string_view>(R"(\\server)"),
            AZStd::tuple<AZStd::string_view>(R"(\\server\share)"),
            AZStd::tuple<AZStd::string_view>(R"(\\?\device)"),
            AZStd::tuple<AZStd::string_view>(R"(\\.\device)"),
            AZStd::tuple<AZStd::string_view>(R"(\??\device)"),
            AZStd::tuple<AZStd::string_view>(R"(D://..)")
        ));

    using PosixPathRelativeParamFixture = PathSingleParamFixture;

    TEST_P(PosixPathRelativeParamFixture, IsRelativePath_Succeeds)
    {
        AZ::IO::FixedMaxPath testPath{ AZStd::get<0>(GetParam()), '/' };
        EXPECT_TRUE(testPath.IsRelative());
    }
    TEST_P(PosixPathRelativeParamFixture, IsRelativePathView_Succeeds)
    {
        AZ::IO::PathView testPath{ AZStd::get<0>(GetParam()), '/' };
        EXPECT_TRUE(testPath.IsRelative());
    }

    INSTANTIATE_TEST_CASE_P(
        RelativePaths,
        PosixPathRelativeParamFixture,
        ::testing::Values(
            AZStd::tuple<AZStd::string_view>("test/foo"),
            AZStd::tuple<AZStd::string_view>(""),
            AZStd::tuple<AZStd::string_view>("."),
            AZStd::tuple<AZStd::string_view>(".."),
            AZStd::tuple<AZStd::string_view>(".."),
            AZStd::tuple<AZStd::string_view>("C:"),
            AZStd::tuple<AZStd::string_view>("D:foo"),
            AZStd::tuple<AZStd::string_view>("D:foo/"),
            AZStd::tuple<AZStd::string_view>("D:foo\\"),
            AZStd::tuple<AZStd::string_view>("D:/foo"),
            AZStd::tuple<AZStd::string_view>("F:/foo/bat")
        ));

    using PosixPathAbsoluteParamFixture = PathSingleParamFixture;

    TEST_P(PosixPathAbsoluteParamFixture, IsAbsolutePath_Succeeds)
    {
        AZ::IO::FixedMaxPath testPath{ AZStd::get<0>(GetParam()), '/' };
        EXPECT_TRUE(testPath.IsAbsolute());
    }
    TEST_P(PosixPathAbsoluteParamFixture, IsAbsolutePathView_Succeeds)
    {
        AZ::IO::PathView testPath{ AZStd::get<0>(GetParam()), '/' };
        EXPECT_TRUE(testPath.IsAbsolute());
    }

    INSTANTIATE_TEST_CASE_P(
        AbsolutePaths,
        PosixPathAbsoluteParamFixture,
        ::testing::Values(
            AZStd::tuple<AZStd::string_view>("/"),
            AZStd::tuple<AZStd::string_view>("//"),
            AZStd::tuple<AZStd::string_view>("/content"),
            AZStd::tuple<AZStd::string_view>(R"(/////server///share)")
        ));

    struct TestParams
    {
        AZStd::string_view path1;
        AZStd::string_view path2;
        AZStd::string_view path3;
        AZStd::string_view resultPath;
        bool expectedResult;
    };
    class PathCustomParamFixture
        : public ScopedAllocatorSetupFixture
        , public ::testing::WithParamInterface<TestParams>
    {};

    using PathAppendTest = PathCustomParamFixture;
    TEST_P(PathAppendTest, Append_AppendsPathWithSeparator)
    {
        using path = AZ::IO::FixedMaxPath;
        TestParams testParam = GetParam();
        {
            path testPath = testParam.path1;
            testPath /= testParam.path2;
            testPath /= testParam.path3;
            EXPECT_STREQ(testParam.resultPath.data(), testPath.c_str());
        }
        {
            path testPath = testParam.path1;
            testPath.Append(testParam.path2);
            testPath.Append(testParam.path3);
            EXPECT_STREQ(testParam.resultPath.data(), testPath.c_str());
        }
    }

    INSTANTIATE_TEST_CASE_P(
        AppendPaths,
        PathAppendTest,
        ::testing::Values(
            TestParams{ "", "var", "lib", "var" AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING "lib" },
            TestParams{ "C:/test/foo", "", "7", "C:/test/foo" AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING "7" },
            TestParams{ "test/foo", "test\\foo", "", "test/foo" AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING "test\\foo" },
            TestParams{ "a", "b", "c", "a" AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING "b" AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING "c" },
            TestParams{ "", "//host", "foo", "//host" AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING "foo" },
            TestParams{ "", "//host/", "foo", "//host/foo" }, // Uses the existing separator from "host" instead of appended preferred_separator
            TestParams{ "", "//host" AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING, "foo", "//host" AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING "foo" }
    ));

    using WindowsPathAppendTest = PathCustomParamFixture;
    TEST_P(WindowsPathAppendTest, Append_AppendsPathWithSeparator)
    {
        TestParams testParam = GetParam();
        {
            AZ::IO::FixedMaxPath testPath(testParam.path1, '\\');
            testPath /= testParam.path2;
            testPath /= testParam.path3;
            EXPECT_STREQ(testParam.resultPath.data(), testPath.c_str());
        }
        {
            AZ::IO::FixedMaxPath testPath(testParam.path1, '\\');
            testPath.Append(testParam.path2);
            testPath.Append(testParam.path3);
            EXPECT_STREQ(testParam.resultPath.data(), testPath.c_str());
        }
    }

    INSTANTIATE_TEST_CASE_P(
        AppendPaths,
        WindowsPathAppendTest,
        ::testing::Values(
            TestParams{ "", "foo", "C:/bar", "C:/bar" }, // (replaces)
            TestParams{ "", "foo", "C:", "C:" }, // (replaces)
            TestParams{ "", "C:", "", "C:" }, // (appends, without separator)
            TestParams{ "", "C:foo", "/bar", R"(C:/bar)" }, // (removes relative path, then appends[keeps path separator])
            TestParams{ "", "C:foo", "\\bar", R"(C:\bar)" }, // (removes relative path, then appends[keeps path separator])
            TestParams{ "", "C:foo", "C:bar", R"(C:foo\bar)" } // (removes relative path, then appends)
    ));

    using PosixPathAppendTest = PathCustomParamFixture;
    TEST_P(PosixPathAppendTest, Append_AppendsPathWithSeparator)
    {
        TestParams testParam = GetParam();
        {
            AZ::IO::FixedMaxPath testPath(testParam.path1, '/');
            testPath /= testParam.path2;
            testPath /= testParam.path3;
            EXPECT_STREQ(testParam.resultPath.data(), testPath.c_str());
        }
        {
            AZ::IO::FixedMaxPath testPath(testParam.path1, '/');
            testPath.Append(testParam.path2);
            testPath.Append(testParam.path3);
            EXPECT_STREQ(testParam.resultPath.data(), testPath.c_str());
        }

        // Check each operator/= and append overload for success
        {
            AZ::IO::FixedMaxPathString pathString{ "foo" };
            AZ::IO::FixedMaxPath testPath('/');
            testPath /= AZ::IO::PathView(pathString);
            testPath /= pathString;
            testPath /= AZStd::string_view(pathString);
            testPath /= pathString.c_str();
            testPath /= pathString.front();
            EXPECT_STREQ("foo/foo/foo/foo/f", testPath.c_str());
        }
        {
            AZ::IO::FixedMaxPathString pathString{ "foo" };
            AZ::IO::FixedMaxPath testPath('/');
            testPath.Append(AZ::IO::PathView(pathString));
            testPath.Append(pathString);
            testPath.Append(AZStd::string_view(pathString));
            testPath.Append(pathString.c_str());
            testPath.Append(pathString.front());
            EXPECT_STREQ("foo/foo/foo/foo/f", testPath.c_str());
        }
    }

    INSTANTIATE_TEST_CASE_P(
        AppendPaths,
        PosixPathAppendTest,
        ::testing::Values(
            TestParams{ "", "foo", "", "foo" }, // (appends)
            TestParams{ "", "foo","/bar", "/bar" } // (replaces)
    ));

    struct PathIteratorParams
    {
        const char m_preferredSeparator;
        AZStd::string_view m_testString;
        AZStd::fixed_vector<AZStd::string_view, 20> m_expectedValues;
    };
    class PathIteratorFixture
        : public ScopedAllocatorSetupFixture
        , public ::testing::WithParamInterface<PathIteratorParams>
    {};

    TEST_P(PathIteratorFixture, PathIterationWorks_ForwardAndBackwards)
    {
        const PathIteratorParams& testParams = GetParam();
        size_t expectedIndex = 0;
        AZ::IO::PathView pathView(testParams.m_testString, testParams.m_preferredSeparator);
        auto first = pathView.begin();
        auto last = pathView.end();
        EXPECT_EQ(testParams.m_expectedValues.size(), AZStd::distance(first, last));

        // Iterate forwards
        for (; expectedIndex < testParams.m_expectedValues.size() && first != last; ++expectedIndex, ++first)
        {
            EXPECT_EQ(testParams.m_expectedValues[expectedIndex], *first);
        }
        EXPECT_EQ(testParams.m_expectedValues.size(), expectedIndex);
        EXPECT_EQ(last, first);

        // Iterate forward, backwards and forwards using the same iteraator
        {
            // Forwards
            expectedIndex = 0;
            first = pathView.begin();
            last = pathView.end();
            auto currentIter = first;
            for (; expectedIndex < testParams.m_expectedValues.size() && currentIter != last; ++expectedIndex, ++currentIter)
            {
                EXPECT_EQ(testParams.m_expectedValues[expectedIndex], *currentIter);
            }
            EXPECT_EQ(testParams.m_expectedValues.size(), expectedIndex);
            EXPECT_EQ(last, currentIter);

            // Backwards
            // Iterate in reverse DO NOT USE reverse_iterator, it causes undefined behavior due to the PathIterator
            // storing a copy of the PathView element with itself.
            for (; expectedIndex > 0 && currentIter != first; --expectedIndex, --currentIter)
            {
                EXPECT_EQ(testParams.m_expectedValues[expectedIndex - 1], *AZStd::prev(currentIter));
            }
            EXPECT_EQ(0, expectedIndex);
            EXPECT_EQ(first, currentIter);
            // Forwards again
            for (; expectedIndex < testParams.m_expectedValues.size() && currentIter != last; ++expectedIndex, ++currentIter)
            {
                EXPECT_EQ(testParams.m_expectedValues[expectedIndex], *currentIter);
            }
            EXPECT_EQ(testParams.m_expectedValues.size(), expectedIndex);
            EXPECT_EQ(last, currentIter);
        }

    }

    INSTANTIATE_TEST_CASE_P(
        PathIterator,
        PathIteratorFixture,
        ::testing::Values(
            PathIteratorParams{ '\\', R"(C:\users\abcdef\AppData\Local\Temp)", {"C:", R"(\)", "users", "abcdef", "AppData", "Local", "Temp"} },
            PathIteratorParams{ '\\', R"(C:\users\abcdef\AppData\Local\Temp\)", {"C:", R"(\)", "users", "abcdef", "AppData", "Local", "Temp"} },
            PathIteratorParams{ '\\', R"(\\server\share\users\abcdef\AppData\Local\Temp)", {R"(\\server)", R"(\)", "share", "users", "abcdef", "AppData", "Local", "Temp"} },
            PathIteratorParams{ '\\', R"(\\?\share\users\abcdef\AppData\Local\Temp)", {R"(\\?)", R"(\)", "share", "users", "abcdef", "AppData", "Local", "Temp"} },
            PathIteratorParams{ '\\', R"(\??\share\users\abcdef\AppData\Local\Temp)", {R"(\??)", R"(\)", "share", "users", "abcdef", "AppData", "Local", "Temp"} },
            PathIteratorParams{ '\\', R"(\\.\share\users\abcdef\AppData\Local\Temp)", {R"(\\.)", R"(\)", "share", "users", "abcdef", "AppData", "Local", "Temp"} },
            PathIteratorParams{ '\\', R"(C:/share\users/abcdef\AppData/Local\Temp)", {R"(C:)", R"(/)", "share", "users", "abcdef", "AppData", "Local", "Temp"} },
            PathIteratorParams{ '\\', R"(share\users/abcdef\AppData/Local\Temp)", {"share", "users", "abcdef", "AppData", "Local", "Temp"} },
            PathIteratorParams{ '\\', R"(share\users/..\abcdef\AppData/./Local\Temp)", {"share", "users", "..", "abcdef", "AppData", ".", "Local", "Temp"} },
            PathIteratorParams{ '/', R"(C:/share\users/abcdef\AppData/Local\Temp)", {R"(C:)", "share", "users", "abcdef", "AppData", "Local", "Temp"} },
            PathIteratorParams{ '/', R"(C:/share\users/abcdef\AppData/Local\Temp)", {R"(C:)", "share", "users", "abcdef", "AppData", "Local", "Temp"} },
            PathIteratorParams{ '/', R"(/home/user/dir)", {R"(/)", "home", "user", "dir"} },
            PathIteratorParams{ '/', R"(/home/user/dir/)", {R"(/)", "home", "user", "dir"} },
            PathIteratorParams{ '/', R"(usr/bin\X/)", {R"(usr)", "bin", "X"} },
            PathIteratorParams{ '/', R"(usr\bin\Xorg)", {R"(usr)", "bin", "Xorg"} },
            PathIteratorParams{ '/', R"(\usr\bin\Xorg)", {R"(\)", "usr", "bin", "Xorg"} }
    ));

    struct PathLexicallyNormalParams
    {
        const char m_preferredSeparator{};
        const char* m_testPathString{};
        const char* m_expectedResult{};
    };
    struct PathLexicallyRelativeParams
        : PathLexicallyNormalParams
    {
        const char* m_basePath{};
    };
    template <typename ParamType>
    class PathLexicallyFixture
        : public ScopedAllocatorSetupFixture
        , public ::testing::WithParamInterface<ParamType>
    {};

    using PathLexicallyNormalFixture = PathLexicallyFixture<PathLexicallyNormalParams>;

    TEST_P(PathLexicallyNormalFixture, LexicallyNormal_NormalizesPath)
    {
        const PathLexicallyNormalParams& testParams = GetParam();
        AZ::IO::FixedMaxPath testPath(testParams.m_testPathString, testParams.m_preferredSeparator);
        testPath = testPath.LexicallyNormal();
        EXPECT_STREQ(testParams.m_expectedResult, testPath.c_str());
    }

    TEST_P(PathLexicallyNormalFixture, PathViewLexicallyNormal_NormalizesPath)
    {
        const PathLexicallyNormalParams& testParams = GetParam();
        AZ::IO::PathView testPath(testParams.m_testPathString, testParams.m_preferredSeparator);
        AZ::IO::FixedMaxPath resultPath = testPath.LexicallyNormal();
        EXPECT_STREQ(testParams.m_expectedResult, resultPath.c_str());
    }

    INSTANTIATE_TEST_CASE_P(
        PathLexicallyNormal,
        PathLexicallyNormalFixture,
        ::testing::Values(
            PathLexicallyNormalParams{ '/', "foo/./bar/..", "foo" },
            PathLexicallyNormalParams{ '/', "foo/.///bar/../", "foo" },
            PathLexicallyNormalParams{ '/', R"(/foo\./bar\..\)", "/foo" },
            PathLexicallyNormalParams{ '/', R"(/..)", "/" },
            PathLexicallyNormalParams{ '\\', R"(C:/O3DE/dev/Cache\game/../pc)", R"(C:\O3DE\dev\Cache\pc)" },
            PathLexicallyNormalParams{ '\\', R"(C:/foo/C:bar)", R"(C:\foo\bar)" },
            PathLexicallyNormalParams{ '\\', R"(C:foo/C:bar)", R"(C:foo\bar)" },
            PathLexicallyNormalParams{ '\\', R"(C:/foo/C:/bar)", R"(C:\bar)" },
            PathLexicallyNormalParams{ '\\', R"(C:/foo/C:)", R"(C:\foo)" },
            PathLexicallyNormalParams{ '\\', R"(C:/foo/C:/)", R"(C:\)" },
            PathLexicallyNormalParams{ '\\', R"(C:/foo/D:bar)", R"(D:bar)" },
            PathLexicallyNormalParams{ '\\', R"(..)", R"(..)" },
            PathLexicallyNormalParams{ '\\', R"(foo/../../bar)", R"(..\bar)" }
        )
    );

    using PathLexicallyRelativeFixture = PathLexicallyFixture<PathLexicallyRelativeParams>;

    TEST_P(PathLexicallyRelativeFixture, LexicallyRelative_ReturnsRelativePath)
    {
        const PathLexicallyRelativeParams& testParams = GetParam();
        AZ::IO::FixedMaxPath testPath(testParams.m_testPathString, testParams.m_preferredSeparator);
        testPath = testPath.LexicallyRelative(AZ::IO::PathView{ testParams.m_basePath, testParams.m_preferredSeparator });
        EXPECT_STREQ(testParams.m_expectedResult, testPath.c_str());
    }

    INSTANTIATE_TEST_CASE_P(
        PathLexicallyRelative,
        PathLexicallyRelativeFixture,
        ::testing::Values(
            PathLexicallyRelativeParams{ {'/', "/a/d", "../../d"}, "/a/b/c" },
            PathLexicallyRelativeParams{ {'/', "/a/b/c", "../b/c"}, "/a/d" },
            PathLexicallyRelativeParams{ {'/', "a/b/c", "b/c"}, "a" },
            PathLexicallyRelativeParams{ {'/', "a/b/c", "../.."}, "a/b/c/x/y" },
            PathLexicallyRelativeParams{ {'/', "a/b/c", "."}, "a/b/c" },
            PathLexicallyRelativeParams{ {'/', "a/b/c", "../c"}, "a/b/./c" },
            PathLexicallyRelativeParams{ {'/', "a/b/c", "../c"}, "a/b/d/../c" },
            PathLexicallyRelativeParams{ {'/', "a/b", "../../a/b"}, "c/d" }
        )
    );

    struct PathViewLexicallyProximateParams
    {
        const char m_preferredSeparator{};
        const char* m_testPathString{};
        const char* m_testBasePath{};
        const char* m_expectedRelativePath{};
        bool m_expectedIsRelativeTo{};
    };

    using PathViewLexicallyProximateFixture = PathLexicallyFixture<PathViewLexicallyProximateParams>;

    TEST_P(PathViewLexicallyProximateFixture, LexicallyProximate_ReturnsRelativePathIfNotEmptyOrTestPath)
    {
        const auto& testParams = GetParam();
        AZ::IO::PathView testPath(testParams.m_testPathString, testParams.m_preferredSeparator);
        AZ::IO::FixedMaxPath resultPath = testPath.LexicallyProximate(AZ::IO::PathView{ testParams.m_testBasePath, testParams.m_preferredSeparator });
        EXPECT_STREQ(testParams.m_expectedRelativePath, resultPath.c_str());
    }
    TEST_P(PathViewLexicallyProximateFixture, IsRelativeTo_ReturnsTrueIfPathIsSubPathOfBase)
    {
        const auto& testParams = GetParam();
        AZ::IO::PathView testPath(testParams.m_testPathString, testParams.m_preferredSeparator);
        EXPECT_EQ(testParams.m_expectedIsRelativeTo, testPath.IsRelativeTo(AZ::IO::PathView{ testParams.m_testBasePath, testParams.m_preferredSeparator }));
    }

    INSTANTIATE_TEST_CASE_P(
        PathLexicallyProximate,
        PathViewLexicallyProximateFixture,
        ::testing::Values(
            PathViewLexicallyProximateParams{ '/', "/a/d", "/a/b/c", "../../d", false },
            PathViewLexicallyProximateParams{ '/', "/a/b/c", "/a/d", "../b/c", false },
            PathViewLexicallyProximateParams{ '/', "a/b/c", "a/b", "c", true },
            PathViewLexicallyProximateParams{ '/', "a/b/c", "a/b/c/x/y", "../..", false },
            PathViewLexicallyProximateParams{ '/', "/a/b/c", "a/b/c", "/a/b/c", false},
            PathViewLexicallyProximateParams{ '/', "a/b/c", "/a/b/c", "a/b/c", false},
            PathViewLexicallyProximateParams{ '\\', "C:\\a\\b", "C:\\a\\b\\c", "..", false },
            PathViewLexicallyProximateParams{ '\\', "C:\\a\\b", "C:\\a\\d\\c", "..\\..\\b", false },
            PathViewLexicallyProximateParams{ '\\', "C:a\\b", "C:\\a\\b", "C:a\\b", false },
            PathViewLexicallyProximateParams{ '\\', "C:\\a\\b", "C:a\\b", "C:\\a\\b", false },
            PathViewLexicallyProximateParams{ '\\', "E:\\a\\b", "F:\\a\\b", "E:\\a\\b", false },
            PathViewLexicallyProximateParams{ '\\', "D:/o3de/proJECT/cache/asset.txt", "d:\\o3de\\Project\\Cache", "asset.txt", true },
            PathViewLexicallyProximateParams{ '\\', "D:/o3de/proJECT/cache/pc/..", "d:\\o3de\\Project\\Cache", "pc\\..", true },
            PathViewLexicallyProximateParams{ '\\', "D:/o3de/proJECT/cache/pc/asset.txt/..", "d:\\o3de\\Project\\Cache\\", "pc\\asset.txt\\..", true },
            PathViewLexicallyProximateParams{ '\\', "D:/o3de/proJECT/cache\\", "D:\\o3de\\Project\\Cache/", ".", true },
            PathViewLexicallyProximateParams{ '\\', "D:/o3de/proJECT/cache/../foo", "D:\\o3de\\Project\\Cache", "..\\foo", false }
        )
    );

    struct PathViewMatchParams
    {
        const char m_preferredSeperator{};
        const char* m_testPath{};
        const char* m_testPattern{};
        bool m_expectedMatch{};
    };
    using PathViewMatchFixture = PathLexicallyFixture<PathViewMatchParams>;

    TEST_P(PathViewMatchFixture, Match_PerformsGlobStyleMatching_ReturnsExpected)
    {
        const auto& testParams = GetParam();
        AZ::IO::PathView testPath(testParams.m_testPath, testParams.m_preferredSeperator);
        EXPECT_EQ(testParams.m_expectedMatch, testPath.Match(testParams.m_testPattern));
    }

    INSTANTIATE_TEST_CASE_P(
        PathMatch,
        PathViewMatchFixture,
        ::testing::Values(
            PathViewMatchParams{ '/', "a/b.ext", "*.ext", true },
            PathViewMatchParams{ '/', "a/b/c.ext", "b/*.ext", true },
            PathViewMatchParams{ '/', "a/b/c.ext", "a/*.ext", false },
            PathViewMatchParams{ '/', "/a.ext", "/*.ext", true },
            PathViewMatchParams{ '/', "a/b.ext", "/*.ext", false },
            PathViewMatchParams{ '/', "a/b/c.ext", "a/*/c.ext", true },
            PathViewMatchParams{ '/', "a/d/e.ext", "a/*/*.ext", true },
            PathViewMatchParams{ '/', "a/g/h.ext", "a/*/f.ext", false },
            PathViewMatchParams{ '\\', "C:\\a\\b\\c", "C:\\a\\*", false },
            PathViewMatchParams{ '\\', "C:\\a\\b\\c", "C:\\a\\b\\*", true },
            PathViewMatchParams{ '\\', "C:\\a\\b\\c", "C:\\a\\b\\c\\*", false }
        )
    );

    using PathMakePreferredFixture = PathLexicallyFixture<PathLexicallyNormalParams>;

    TEST_P(PathMakePreferredFixture, MakePreferred_ConvertsPathToOnlyHavePreferredSeparators)
    {
        const auto& testParams = GetParam();
        AZ::IO::FixedMaxPath testPath(testParams.m_testPathString, testParams.m_preferredSeparator);
        testPath.MakePreferred();
        EXPECT_STREQ(testParams.m_expectedResult, testPath.c_str());
    }

    INSTANTIATE_TEST_CASE_P(
        PathMakePreferred,
        PathMakePreferredFixture,
        ::testing::Values(
            PathLexicallyNormalParams{ '/', R"(C:\foo/bar\base)", "C:/foo/bar/base" },
            PathLexicallyNormalParams{ '/', "\\\\foo/bar/base/baz/", "//foo/bar/base/baz/" },
            PathLexicallyNormalParams{ '\\', R"(C:\foo/bar/base\boo)", R"(C:\foo\bar\base\boo)" },
            PathLexicallyNormalParams{ '\\', R"(//foo//bar\\base)", R"(\\foo\\bar\\base)" },
            PathLexicallyNormalParams{ '\\', R"(C:\\/\\bar\baz/)", R"(C:\\\\\bar\baz\)" }
        )
    );

    struct PathReplaceFilenameParams
    {
        const char* m_testPath{};
        const char* m_replacementFilename{};
        const char* m_expectedNativeResult{};
    };
    using PathReplaceFilenameTest = PathLexicallyFixture<PathReplaceFilenameParams>;
    TEST_P(PathReplaceFilenameTest, ReplacesFilename_ComparesEqualToExpectedStringAndPath)
    {
        PathReplaceFilenameParams testParam = GetParam();
        {
            AZ::IO::FixedMaxPath testPath(testParam.m_testPath, AZ::IO::PosixPathSeparator);
            testPath.ReplaceFilename(testParam.m_replacementFilename);
            EXPECT_STREQ(testParam.m_expectedNativeResult, testPath.c_str());
            EXPECT_EQ(AZ::IO::PathView(testParam.m_expectedNativeResult), testPath);
        }
    }

    INSTANTIATE_TEST_CASE_P(
        ReplaceFilenames,
        PathReplaceFilenameTest,
        ::testing::Values(
            PathReplaceFilenameParams{ "test/foo", "foo", "test/foo" },
            PathReplaceFilenameParams{ "test/bar", "gaz", "test/gaz" },
            PathReplaceFilenameParams{ "test/bar", "", "test" },
            PathReplaceFilenameParams{ "test/bar/", "", "test" },
            PathReplaceFilenameParams{ "test/bar/", "ice",  "test/ice" }
    ));

    struct PathRemoveFilenameParams
    {
        const char* m_testPath{};
        const char* m_expectedNativeResult{};
    };
    using PathRemoveFilenameTest = PathLexicallyFixture<PathRemoveFilenameParams>;
    TEST_P(PathRemoveFilenameTest, RemoveFilename_RemovesFinalPathSegmentAndPreviousSeparator)
    {
        PathRemoveFilenameParams testParam = GetParam();
        {
            AZ::IO::FixedMaxPath testPath = testParam.m_testPath;
            testPath.RemoveFilename();
            EXPECT_STREQ(testParam.m_expectedNativeResult, testPath.c_str());
        }
    }

    INSTANTIATE_TEST_CASE_P(
        RemoveFilenames,
        PathRemoveFilenameTest,
        ::testing::Values(
            PathRemoveFilenameParams{ "test/foo", "test" },
            PathRemoveFilenameParams{ "test/foo/", "test" }
    ));

    struct PathPrefixParams
    {
        const char* m_testPrefixPath{};
        const char* m_testPostfixPath{};
        bool m_expectedResult{};
    };
    using PathPrefixFixture = PathLexicallyFixture<PathPrefixParams>;

    TEST_P(PathPrefixFixture, InvokingStdMismatch_OnPath_IsAbleToReturnPathPrefix)
    {
        const auto& testParams = GetParam();
        AZ::IO::PathView testPath{ testParams.m_testPrefixPath };
        AZ::IO::PathView subPath{ testParams.m_testPostfixPath };
        auto [prefixIter, postfixIter] = AZStd::mismatch(testPath.begin(), testPath.end(), subPath.begin(), subPath.end());
        EXPECT_EQ(testParams.m_expectedResult, prefixIter == testPath.end());
    }

    INSTANTIATE_TEST_CASE_P(
        PathPrefix,
        PathPrefixFixture,
        ::testing::Values(
            PathPrefixParams{ "C:\\", "C:\\", true },
            PathPrefixParams{ "C:\\", "C:\\foo", true },
            PathPrefixParams{ "C:\\\\", "C:\\foo", true },
            PathPrefixParams{ "C:/", "C:\\foo", true },
            PathPrefixParams{ "C:\\foo\\", "C:\\foo", true },
            PathPrefixParams{ "C:", "C:\\foo", true },
            PathPrefixParams{ "D:\\", "C:\\foo", false },
            PathPrefixParams{ "/O3DE/dev/", "/O3DE/dev", true },
            PathPrefixParams{ "/O3DE/dev", "/O3DE/dev/", true },
            PathPrefixParams{ "/O3DE/dev/", "/O3DE/dev/Cache", true },
            PathPrefixParams{ "/O3DE/dev", "/O3DE/dev/Cache", true },
            PathPrefixParams{ "/O3DE/dev", "/O3DE/dev/Cache/", true },
            PathPrefixParams{ "O3DE/dev/", "O3DE/dev/Cache/", true },
            PathPrefixParams{ "O3DE\\dev/Assets", "O3DE/dev/Cache/", false }
    ));

    struct PathDecompositionParams
    {
        const char m_preferredSeparator{};
        const char* m_testPathString{};
        const char* m_expectedRootName{};
        const char* m_expectedRootDirectory{};
        const char* m_expectedRootPath{};
        const char* m_expectedRelativePath{};
        const char* m_expectedParentPath{};
        const char* m_expectedFilename{};
        const char* m_expectedStem{};
        const char* m_expectedExtension{};
    };

    using PathDecompositionFixture = PathLexicallyFixture<PathDecompositionParams>;

    TEST_P(PathDecompositionFixture, PathDecomposition_SplitsPathCorrectly)
    {
        const auto& testParams = GetParam();
        AZ::IO::PathView testPath{ testParams.m_testPathString, testParams.m_preferredSeparator };
        EXPECT_EQ(testParams.m_expectedRootName, testPath.RootName());
        EXPECT_EQ(testParams.m_expectedRootDirectory, testPath.RootDirectory());
        EXPECT_EQ(testParams.m_expectedRootPath, testPath.RootPath());
        EXPECT_EQ(testParams.m_expectedRelativePath, testPath.RelativePath());
        EXPECT_EQ(testParams.m_expectedParentPath, testPath.ParentPath());
        EXPECT_EQ(testParams.m_expectedFilename, testPath.Filename());
        EXPECT_EQ(testParams.m_expectedStem, testPath.Stem());
        EXPECT_EQ(testParams.m_expectedExtension, testPath.Extension());
    }

    INSTANTIATE_TEST_CASE_P(
        PathExtractComponents,
        PathDecompositionFixture,
        ::testing::Values(
            PathDecompositionParams{ '\\', R"(C:\path\to\some\resource.exe)", "C:", R"(\)", R"(C:\)", R"(path\to\some\resource.exe)",
            R"(C:\path\to\some)", "resource.exe", "resource", ".exe" },
            PathDecompositionParams{ '/', R"(C:\path\to\some\resource.exe)", "", "", "", R"(C:\path\to\some\resource.exe)",
            R"(C:\path\to\some)", "resource.exe", "resource", ".exe" },
            PathDecompositionParams{ '/', R"(/path/to/some/resource.exe)", "", "/", "/", "path/to/some/resource.exe",
            "/path/to/some", "resource.exe", "resource", ".exe" },
            PathDecompositionParams{ '/', R"(relpath/to/some/resource.exe)", "", "", "", "relpath/to/some/resource.exe",
            "relpath/to/some", "resource.exe", "resource", ".exe" },
            PathDecompositionParams{ '/', R"(relpath/to/some/resource)", "", "", "", "relpath/to/some/resource",
            "relpath/to/some", "resource", "resource", "" },
            PathDecompositionParams{ '/', R"(relpath/to/some/.hidden)", "", "", "", "relpath/to/some/.hidden",
            "relpath/to/some", ".hidden", ".hidden", "" },
            PathDecompositionParams{ '/', R"(relpath/to/some/..bar)", "", "", "", "relpath/to/some/..bar",
            "relpath/to/some", "..bar", ".", ".bar" },
            PathDecompositionParams{ '/', R"(relpath/to/some/bar.)", "", "", "", "relpath/to/some/bar.",
            "relpath/to/some", "bar.", "bar", "." },
            PathDecompositionParams{ '/', R"(relpath/to/some/bar/.)", "", "", "", "relpath/to/some/bar/.",
            "relpath/to/some/bar", ".", ".", "" },
            PathDecompositionParams{ '/', R"(relpath/to/some/bar/..)", "", "", "", "relpath/to/some/bar/..",
            "relpath/to/some/bar", "..", "..", "" },
            PathDecompositionParams{ '\\', R"(C:relpath\to\some\bar)", "C:", "", "C:", R"(relpath\to\some\bar)",
            R"(C:relpath\to\some)", "bar", "bar", "" },
            PathDecompositionParams{ '\\', R"(\\server\relpath\to\some\bar)", R"(\\server)", R"(\)", R"(\\server\)", R"(relpath\to\some\bar)",
            R"(\\server\relpath\to\some)", "bar", "bar", "" },
            PathDecompositionParams{ '\\', R"(\\?\relpath\to\some\bar.foo.run)", R"(\\?)", R"(\)", R"(\\?\)", R"(relpath\to\some\bar.foo.run)",
            R"(\\?\relpath\to\some)", "bar.foo.run", "bar.foo", ".run" },
            PathDecompositionParams{ '\\', R"(\??\relpath\to\some\double.bar\bar)", R"(\??)", R"(\)", R"(\??\)", R"(relpath\to\some\double.bar\bar)",
            R"(\??\relpath\to\some\double.bar)", "bar", "bar", "" },
            PathDecompositionParams{ '\\', R"(\\.\relpath\to\some\double.bar\bar)", R"(\\.)", R"(\)", R"(\\.\)", R"(relpath\to\some\double.bar\bar)",
            R"(\\.\relpath\to\some\double.bar)", "bar", "bar", "" },
            PathDecompositionParams{ '\\', R"(C:\path\with\trailing\separator\)", R"(C:)", R"(\)", R"(C:\)", R"(path\with\trailing\separator)",
            R"(C:\path\with\trailing)", "separator", "separator", "" }
        )
    );
#endif // AZ_UNIT_TEST_SKIP_PATH_TESTS
}

#if defined(HAVE_BENCHMARK)
namespace Benchmark
{
    class PathBenchmarkFixture
        : public ::UnitTest::AllocatorsBenchmarkFixture
    {
    protected:
        AZStd::fixed_vector<const char*, 20> m_appendPaths{ "foo", "bar", "baz", "bazzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz",
            "boo/bar/base", "C:\\path\\to\\O3DE", "C", "\\\\", "/", R"(test\\path/with\mixed\separators)" };
    };

    BENCHMARK_F(PathBenchmarkFixture, BM_PathAppendFixedPath)(benchmark::State& state)
    {
        AZ::IO::FixedMaxPath m_testPath{ "." };
        for (auto _ : state)
        {
            for (const auto& appendPath : m_appendPaths)
            {
                m_testPath /= appendPath;
            }
        }
    }
    BENCHMARK_F(PathBenchmarkFixture, BM_PathAppendAllocatingPath)(benchmark::State& state)
    {
        AZ::IO::Path m_testPath{ "." };
        for (auto _ : state)
        {
            for (const auto& appendPath : m_appendPaths)
            {
                m_testPath /= appendPath;
            }
        }
    }

    BENCHMARK_F(PathBenchmarkFixture, BM_StringFuncPathJoinFixedString)(benchmark::State& state)
    {
        AZStd::string m_testPath{ "." };
        for (auto _ : state)
        {
            for (const auto& appendPath : m_appendPaths)
            {
                AZ::StringFunc::Path::Join(m_testPath.c_str(), appendPath, m_testPath);
            }
        }
    }
    BENCHMARK_F(PathBenchmarkFixture, BM_StringFuncPathJoinAZStdString)(benchmark::State& state)
    {
        AZStd::string m_testPath{ "." };
        for (auto _ : state)
        {
            for (const auto& appendPath : m_appendPaths)
            {
                AZ::StringFunc::Path::Join(m_testPath.c_str(), appendPath, m_testPath);
            }
        }
    }
}
#endif
