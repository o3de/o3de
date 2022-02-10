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

    TEST_F(DomPrefixTreeTests, GetAndSetRoot)
    {
        DomPrefixTree<AZStd::string> tree;
        tree.SetValue(Path(), "root");
        EXPECT_EQ(*tree.ValueAtPath(Path(), PrefixTreeMatch::ExactPath), "root");
    }

    TEST_F(DomPrefixTreeTests, GetExactPath)
    {
        DomPrefixTree<int> tree;

        tree.SetValue(Path("/foo/0"), 0);
        tree.SetValue(Path("/foo/1"), 42);
        tree.SetValue(Path("/foo/foo"), 1);
        tree.SetValue(Path("/foo/bar"), 2);

        EXPECT_EQ(*tree.ValueAtPath(Path("/foo/0"), PrefixTreeMatch::ExactPath), 0);
        EXPECT_EQ(*tree.ValueAtPath(Path("/foo/1"), PrefixTreeMatch::ExactPath), 42);
        EXPECT_EQ(*tree.ValueAtPath(Path("/foo/foo"), PrefixTreeMatch::ExactPath), 1);
        EXPECT_EQ(*tree.ValueAtPath(Path("/foo/bar"), PrefixTreeMatch::ExactPath), 2);

        EXPECT_EQ(tree.ValueAtPath(Path(), PrefixTreeMatch::ExactPath), nullptr);
        EXPECT_EQ(tree.ValueAtPath(Path("/foo"), PrefixTreeMatch::ExactPath), nullptr);
        EXPECT_EQ(tree.ValueAtPath(Path("/foo/0/subpath"), PrefixTreeMatch::ExactPath), nullptr);
    }

    TEST_F(DomPrefixTreeTests, GetSubpath)
    {
        DomPrefixTree<int> tree;

        tree.SetValue(Path("/foo/0"), 0);
        tree.SetValue(Path("/foo/1"), 42);

        EXPECT_EQ(*tree.ValueAtPath(Path("/foo/0/bar"), PrefixTreeMatch::SubpathsOnly), 0);
        EXPECT_EQ(*tree.ValueAtPath(Path("/foo/0/bar/baz"), PrefixTreeMatch::SubpathsOnly), 0);
        EXPECT_EQ(*tree.ValueAtPath(Path("/foo/1/0"), PrefixTreeMatch::SubpathsOnly), 42);

        EXPECT_EQ(tree.ValueAtPath(Path("/foo/0"), PrefixTreeMatch::SubpathsOnly), nullptr);
        EXPECT_EQ(tree.ValueAtPath(Path("/foo/1"), PrefixTreeMatch::SubpathsOnly), nullptr);
    }

    TEST_F(DomPrefixTreeTests, GetPathOrSubpath)
    {
        DomPrefixTree<int> tree;

        tree.SetValue(Path("/foo/0"), 0);
        tree.SetValue(Path("/foo/1"), 42);

        EXPECT_EQ(*tree.ValueAtPath(Path("/foo/0"), PrefixTreeMatch::PathAndSubpaths), 0);
        EXPECT_EQ(*tree.ValueAtPath(Path("/foo/0/bar"), PrefixTreeMatch::PathAndSubpaths), 0);
        EXPECT_EQ(*tree.ValueAtPath(Path("/foo/0/bar/baz"), PrefixTreeMatch::PathAndSubpaths), 0);
        EXPECT_EQ(*tree.ValueAtPath(Path("/foo/1"), PrefixTreeMatch::PathAndSubpaths), 42);
        EXPECT_EQ(*tree.ValueAtPath(Path("/foo/1/0"), PrefixTreeMatch::PathAndSubpaths), 42);

        EXPECT_EQ(tree.ValueAtPath(Path(), PrefixTreeMatch::PathAndSubpaths), nullptr);
        EXPECT_EQ(tree.ValueAtPath(Path("/foo"), PrefixTreeMatch::PathAndSubpaths), nullptr);
        EXPECT_EQ(tree.ValueAtPath(Path("/path/0"), PrefixTreeMatch::PathAndSubpaths), nullptr);
    }

    TEST_F(DomPrefixTreeTests, RemovePath)
    {
        DomPrefixTree<int> tree;

        tree.SetValue(Path(), 20);
        tree.SetValue(Path("/foo"), 40);
        tree.SetValue(Path("/foo/0"), 80);

        tree.EraseValue(Path("/foo"));

        EXPECT_EQ(*tree.ValueAtPath(Path("/foo"), PrefixTreeMatch::PathAndSubpaths), 20);
        EXPECT_EQ(*tree.ValueAtPath(Path("/foo/0"), PrefixTreeMatch::PathAndSubpaths), 80);
    }

    TEST_F(DomPrefixTreeTests, RemovePathAndChildren)
    {
        DomPrefixTree<int> tree;

        tree.SetValue(Path(), 20);
        tree.SetValue(Path("/foo"), 40);
        tree.SetValue(Path("/foo/0"), 80);

        tree.EraseValue(Path("/foo"), true);

        EXPECT_EQ(*tree.ValueAtPath(Path("/foo"), PrefixTreeMatch::PathAndSubpaths), 20);
        EXPECT_EQ(*tree.ValueAtPath(Path("/foo/0"), PrefixTreeMatch::PathAndSubpaths), 20);
    }

    TEST_F(DomPrefixTreeTests, ClearTree)
    {
        DomPrefixTree<int> tree;

        tree.SetValue(Path(), 20);
        tree.SetValue(Path("/foo"), 40);

        tree.Clear();

        EXPECT_EQ(tree.ValueAtPathOrDefault(Path("/foo"), -10, PrefixTreeMatch::PathAndSubpaths), -10);
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
        EXPECT_EQ(results.size(), 0);
        results.clear();

        tree.VisitPath(Path("/foo/0"), PrefixTreeMatch::ExactPath, visitorFn);
        EXPECT_EQ(results.size(), 1);
        EXPECT_TRUE(validateResult(Path("/foo/0"), 0));
        results.clear();

        tree.VisitPath(Path("/foo/1"), PrefixTreeMatch::ExactPath, visitorFn);
        EXPECT_EQ(results.size(), 1);
        EXPECT_TRUE(validateResult(Path("/foo/1"), 42));
        results.clear();

        tree.VisitPath(Path("/foo"), PrefixTreeMatch::SubpathsOnly, visitorFn);
        EXPECT_EQ(results.size(), 2);
        EXPECT_TRUE(validateResult(Path("/foo/0"), 0));
        EXPECT_TRUE(validateResult(Path("/foo/1"), 42));
        results.clear();

        tree.VisitPath(Path("/foo"), PrefixTreeMatch::PathAndSubpaths, visitorFn);
        EXPECT_EQ(results.size(), 3);
        EXPECT_TRUE(validateResult(Path("/foo"), 99));
        EXPECT_TRUE(validateResult(Path("/foo/0"), 0));
        EXPECT_TRUE(validateResult(Path("/foo/1"), 42));
        results.clear();
    }
}
