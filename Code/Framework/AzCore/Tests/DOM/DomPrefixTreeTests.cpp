/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/DomPrefixTree.h>
#include <Tests/DOM/DomFixtures.h>

namespace AZ::Dom::Tests
{
    using DomPrefixTreeTests = DomTestFixture;

    static_assert(!RangeConvertibleToPrefixTree<AZStd::vector<int>, int>, "Non-pair range should not convert to tree");
    static_assert(
        !RangeConvertibleToPrefixTree<AZStd::vector<AZStd::pair<Path, AZStd::string>>, int>,
        "Mismatched value type should not convert to tree");
    static_assert(
        !RangeConvertibleToPrefixTree<AZStd::vector<AZStd::pair<AZStd::string, int>>, int>,
        "Mismatched value type should not convert to tree");
    static_assert(
        RangeConvertibleToPrefixTree<AZStd::vector<AZStd::pair<Path, int>>, int>,
        "Vector with path / key type pairs should convert to tree");

    TEST_F(DomPrefixTreeTests, InitializeFromInitializerList)
    {
        DomPrefixTree<int> tree({
            { Path(), 0 },
            { Path("/foo/bar"), 1 },
        });

        EXPECT_EQ(0, *tree.ValueAtPath(Path(), PrefixTreeMatch::ExactPath));
        EXPECT_EQ(1, *tree.ValueAtPath(Path("/foo/bar"), PrefixTreeMatch::ExactPath));
    }

    TEST_F(DomPrefixTreeTests, InitializeFromRange)
    {
        AZStd::vector<AZStd::pair<Path, int>> container({
            { Path(), 21 },
            { Path("/foo/bar"), 42 },
        });
        DomPrefixTree<int> tree(container);

        EXPECT_EQ(21, *tree.ValueAtPath(Path(), PrefixTreeMatch::ExactPath));
        EXPECT_EQ(42, *tree.ValueAtPath(Path("/foo/bar"), PrefixTreeMatch::ExactPath));
    }

    TEST_F(DomPrefixTreeTests, GetAndSetRoot)
    {
        DomPrefixTree<AZStd::string> tree;
        tree.SetValue(Path(), "root");
        EXPECT_EQ("root", *tree.ValueAtPath(Path(), PrefixTreeMatch::ExactPath));
    }

    TEST_F(DomPrefixTreeTests, GetExactPath)
    {
        DomPrefixTree<int> tree;

        tree.SetValue(Path("/foo/0"), 0);
        tree.SetValue(Path("/foo/1"), 42);
        tree.SetValue(Path("/foo/foo"), 1);
        tree.SetValue(Path("/foo/bar"), 2);

        EXPECT_EQ(0, *tree.ValueAtPath(Path("/foo/0"), PrefixTreeMatch::ExactPath));
        EXPECT_EQ(42, *tree.ValueAtPath(Path("/foo/1"), PrefixTreeMatch::ExactPath));
        EXPECT_EQ(1, *tree.ValueAtPath(Path("/foo/foo"), PrefixTreeMatch::ExactPath));
        EXPECT_EQ(2, *tree.ValueAtPath(Path("/foo/bar"), PrefixTreeMatch::ExactPath));

        EXPECT_EQ(nullptr, tree.ValueAtPath(Path(), PrefixTreeMatch::ExactPath));
        EXPECT_EQ(nullptr, tree.ValueAtPath(Path("/foo"), PrefixTreeMatch::ExactPath));
        EXPECT_EQ(nullptr, tree.ValueAtPath(Path("/foo/0/subpath"), PrefixTreeMatch::ExactPath));
    }

    TEST_F(DomPrefixTreeTests, GetSubpath)
    {
        DomPrefixTree<int> tree;

        tree.SetValue(Path("/foo/0"), 0);
        tree.SetValue(Path("/foo/1"), 42);

        EXPECT_EQ(0, *tree.ValueAtPath(Path("/foo/0/bar"), PrefixTreeMatch::SubpathsOnly));
        EXPECT_EQ(0, *tree.ValueAtPath(Path("/foo/0/bar/baz"), PrefixTreeMatch::SubpathsOnly));
        EXPECT_EQ(42, *tree.ValueAtPath(Path("/foo/1/0"), PrefixTreeMatch::SubpathsOnly));

        EXPECT_EQ(nullptr, tree.ValueAtPath(Path("/foo/0"), PrefixTreeMatch::SubpathsOnly));
        EXPECT_EQ(nullptr, tree.ValueAtPath(Path("/foo/1"), PrefixTreeMatch::SubpathsOnly));
    }

    TEST_F(DomPrefixTreeTests, GetPathOrSubpath)
    {
        DomPrefixTree<int> tree;

        tree.SetValue(Path("/foo/0"), 0);
        tree.SetValue(Path("/foo/1"), 42);

        EXPECT_EQ(0, *tree.ValueAtPath(Path("/foo/0"), PrefixTreeMatch::PathAndSubpaths));
        EXPECT_EQ(0, *tree.ValueAtPath(Path("/foo/0/bar"), PrefixTreeMatch::PathAndSubpaths));
        EXPECT_EQ(0, *tree.ValueAtPath(Path("/foo/0/bar/baz"), PrefixTreeMatch::PathAndSubpaths));
        EXPECT_EQ(42, *tree.ValueAtPath(Path("/foo/1"), PrefixTreeMatch::PathAndSubpaths));
        EXPECT_EQ(42, *tree.ValueAtPath(Path("/foo/1/0"), PrefixTreeMatch::PathAndSubpaths));

        EXPECT_EQ(nullptr, tree.ValueAtPath(Path(), PrefixTreeMatch::PathAndSubpaths));
        EXPECT_EQ(nullptr, tree.ValueAtPath(Path("/foo"), PrefixTreeMatch::PathAndSubpaths));
        EXPECT_EQ(nullptr, tree.ValueAtPath(Path("/path/0"), PrefixTreeMatch::PathAndSubpaths));
    }

    TEST_F(DomPrefixTreeTests, RemovePath)
    {
        DomPrefixTree<int> tree;

        tree.SetValue(Path(), 20);
        tree.SetValue(Path("/foo"), 40);
        tree.SetValue(Path("/foo/0"), 80);

        tree.EraseValue(Path("/foo"));

        EXPECT_EQ(20, *tree.ValueAtPath(Path("/foo"), PrefixTreeMatch::PathAndSubpaths));
        EXPECT_EQ(80, *tree.ValueAtPath(Path("/foo/0"), PrefixTreeMatch::PathAndSubpaths));
    }

    TEST_F(DomPrefixTreeTests, RemovePathAndChildren)
    {
        DomPrefixTree<int> tree;

        tree.SetValue(Path(), 20);
        tree.SetValue(Path("/foo"), 40);
        tree.SetValue(Path("/foo/0"), 80);

        tree.EraseValue(Path("/foo"), true);

        EXPECT_EQ(20, *tree.ValueAtPath(Path("/foo"), PrefixTreeMatch::PathAndSubpaths));
        EXPECT_EQ(20, *tree.ValueAtPath(Path("/foo/0"), PrefixTreeMatch::PathAndSubpaths));
    }

    TEST_F(DomPrefixTreeTests, ClearTree)
    {
        DomPrefixTree<int> tree;

        tree.SetValue(Path(), 20);
        tree.SetValue(Path("/foo"), 40);

        tree.Clear();

        EXPECT_EQ(-10, tree.ValueAtPathOrDefault(Path("/foo"), -10, PrefixTreeMatch::PathAndSubpaths));
    }

    TEST_F(DomPrefixTreeTests, Visit)
    {
        DomPrefixTree<int> tree;

        AZStd::vector<AZStd::pair<Path, int>> results;
        auto visitorFn = [&results](const Path& path, int n)
        {
            results.emplace_back(path, n);
        };

        auto validateResult = [&results](const Path& path, int n)
        {
            for (const auto& pair : results)
            {
                if (pair.first == path)
                {
                    return pair.second == n;
                }
            }
            return false;
        };

        tree.SetValue(Path("/foo"), 99);
        tree.SetValue(Path("/foo/0"), 0);
        tree.SetValue(Path("/foo/1"), 42);
        tree.SetValue(Path("/bar/bat"), 1);
        tree.SetValue(Path("/bar/baz"), 2);

        tree.VisitPath(Path("/bar"), PrefixTreeMatch::ExactPath, visitorFn);
        EXPECT_EQ(0, results.size());
        results.clear();

        tree.VisitPath(Path("/foo/0"), PrefixTreeMatch::ExactPath, visitorFn);
        EXPECT_EQ(1, results.size());
        EXPECT_TRUE(validateResult(Path("/foo/0"), 0));
        results.clear();

        tree.VisitPath(Path("/foo/1"), PrefixTreeMatch::ExactPath, visitorFn);
        EXPECT_EQ(1, results.size());
        EXPECT_TRUE(validateResult(Path("/foo/1"), 42));
        results.clear();

        tree.VisitPath(Path("/foo"), PrefixTreeMatch::SubpathsOnly, visitorFn);
        EXPECT_EQ(2, results.size());
        EXPECT_TRUE(validateResult(Path("/foo/0"), 0));
        EXPECT_TRUE(validateResult(Path("/foo/1"), 42));
        results.clear();

        tree.VisitPath(Path("/foo"), PrefixTreeMatch::PathAndSubpaths, visitorFn);
        EXPECT_EQ(3, results.size());
        EXPECT_TRUE(validateResult(Path("/foo"), 99));
        EXPECT_TRUE(validateResult(Path("/foo/0"), 0));
        EXPECT_TRUE(validateResult(Path("/foo/1"), 42));
        results.clear();
    }
} // namespace AZ::Dom::Tests
