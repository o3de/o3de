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
    class DomPrefixTreeTests : public DomTestFixture
    {
    public:
        void SetUp() override
        {
            DomTestFixture::SetUp();

            m_runData = AZStd::make_unique<RunData>();
            m_runData->m_visitorFn = [this](const Path& path, int value)
            {
                auto pathString = path.ToString();

                AZStd::string visitedPathString = path.ToString();
                for (const auto& entry : m_runData->m_visitorResults)
                {
                    AZStd::string entryPathString = entry.first.ToString();
                    if (m_runData->m_useDepthFirstTraversal)
                    {
                        EXPECT_FALSE(visitedPathString.starts_with(entryPathString));
                    }
                    else
                    {
                        EXPECT_FALSE(entryPathString.starts_with(visitedPathString));
                    }
                }

                m_runData->m_visitorResults.emplace_back(path, value);
                return true;
            };

            m_runData->m_visitorTree.SetValue(Path("/foo"), 99);
            m_runData->m_visitorTree.SetValue(Path("/foo/0"), 0);
            m_runData->m_visitorTree.SetValue(Path("/foo/1"), 42);
            m_runData->m_visitorTree.SetValue(Path("/bar/bat"), 1);
            m_runData->m_visitorTree.SetValue(Path("/bar/baz"), 2);
        }

        void TearDown() override
        {
            m_runData.reset();

            DomTestFixture::TearDown();
        }

    protected:
        struct RunData
        {
            DomPrefixTree<int> m_visitorTree;
            DomPrefixTree<int>::VisitorFunction m_visitorFn;
            AZStd::vector<AZStd::pair<Path, int>> m_visitorResults;
            bool m_useDepthFirstTraversal = false;
        };
        AZStd::unique_ptr<RunData> m_runData;

        size_t VisitorResultCount()
        {
            return m_runData->m_visitorResults.size();
        }

        bool ValidateResult(const Path& path, int n)
        {
            for (const auto& pair : m_runData->m_visitorResults)
            {
                if (pair.first == path)
                {
                    return pair.second == n;
                }
            }
            return false;
        }

        void SetVisitDepthFirst(bool visitDepthFirst)
        {
            m_runData->m_useDepthFirstTraversal = visitDepthFirst;
        }

        void TestVisitPath(const Path& path, PrefixTreeTraversalFlags flags)
        {
            if (m_runData->m_useDepthFirstTraversal)
            {
                flags = flags | PrefixTreeTraversalFlags::TraverseMostToLeastSpecific;
            }
            else
            {
                flags = flags | PrefixTreeTraversalFlags::TraverseLeastToMostSpecific;
            }
            m_runData->m_visitorTree.VisitPath(path, m_runData->m_visitorFn, flags);
        }
    };

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

    TEST_F(DomPrefixTreeTests, GetParent)
    {
        DomPrefixTree<int> tree;

        tree.SetValue(Path("/foo/0"), 0);
        tree.SetValue(Path("/foo/1"), 42);

        EXPECT_EQ(0, *tree.ValueAtPath(Path("/foo/0/bar"), PrefixTreeMatch::ParentsOnly));
        EXPECT_EQ(0, *tree.ValueAtPath(Path("/foo/0/bar/baz"), PrefixTreeMatch::ParentsOnly));
        EXPECT_EQ(42, *tree.ValueAtPath(Path("/foo/1/0"), PrefixTreeMatch::ParentsOnly));

        EXPECT_EQ(nullptr, tree.ValueAtPath(Path("/foo/0"), PrefixTreeMatch::ParentsOnly));
        EXPECT_EQ(nullptr, tree.ValueAtPath(Path("/foo/1"), PrefixTreeMatch::ParentsOnly));
    }

    TEST_F(DomPrefixTreeTests, GetPathOrParent)
    {
        DomPrefixTree<int> tree;

        tree.SetValue(Path("/foo/0"), 0);
        tree.SetValue(Path("/foo/1"), 42);

        EXPECT_EQ(0, *tree.ValueAtPath(Path("/foo/0"), PrefixTreeMatch::PathAndParents));
        EXPECT_EQ(0, *tree.ValueAtPath(Path("/foo/0/bar"), PrefixTreeMatch::PathAndParents));
        EXPECT_EQ(0, *tree.ValueAtPath(Path("/foo/0/bar/baz"), PrefixTreeMatch::PathAndParents));
        EXPECT_EQ(42, *tree.ValueAtPath(Path("/foo/1"), PrefixTreeMatch::PathAndParents));
        EXPECT_EQ(42, *tree.ValueAtPath(Path("/foo/1/0"), PrefixTreeMatch::PathAndParents));

        EXPECT_EQ(nullptr, tree.ValueAtPath(Path(), PrefixTreeMatch::PathAndParents));
        EXPECT_EQ(nullptr, tree.ValueAtPath(Path("/foo"), PrefixTreeMatch::PathAndParents));
        EXPECT_EQ(nullptr, tree.ValueAtPath(Path("/path/0"), PrefixTreeMatch::PathAndParents));
    }

    TEST_F(DomPrefixTreeTests, RemovePath)
    {
        DomPrefixTree<int> tree;

        tree.SetValue(Path(), 20);
        tree.SetValue(Path("/foo"), 40);
        tree.SetValue(Path("/foo/0"), 80);

        tree.EraseValue(Path("/foo"));

        EXPECT_EQ(20, *tree.ValueAtPath(Path("/foo"), PrefixTreeMatch::PathAndParents));
        EXPECT_EQ(80, *tree.ValueAtPath(Path("/foo/0"), PrefixTreeMatch::PathAndParents));
    }

    TEST_F(DomPrefixTreeTests, RemovePathAndChildren)
    {
        DomPrefixTree<int> tree;

        tree.SetValue(Path(), 20);
        tree.SetValue(Path("/foo"), 40);
        tree.SetValue(Path("/foo/0"), 80);

        tree.EraseValue(Path("/foo"), true);

        EXPECT_EQ(20, *tree.ValueAtPath(Path("/foo"), PrefixTreeMatch::PathAndParents));
        EXPECT_EQ(20, *tree.ValueAtPath(Path("/foo/0"), PrefixTreeMatch::PathAndParents));
    }

    TEST_F(DomPrefixTreeTests, DetachSubTreeAtExistingPath)
    {
        DomPrefixTree<int> tree;
        tree.SetValue(Path("/foo"), 40);
        tree.SetValue(Path("/foo/0"), 80);

        DomPrefixTree<int> detachedTree = tree.DetachSubTree(AZ::Dom::Path("/foo"));

        // Validate that the values aren't present in the old tree after detaching.
        EXPECT_EQ(nullptr, tree.ValueAtPath(Path("/foo"), PrefixTreeMatch::ExactPath));
        EXPECT_EQ(nullptr, tree.ValueAtPath(Path("/foo/0"), PrefixTreeMatch::ExactPath));

        // Validate that the values are present in the detached tree.
        EXPECT_EQ(40, *detachedTree.ValueAtPath(Path(), PrefixTreeMatch::ExactPath));
        EXPECT_EQ(80, *detachedTree.ValueAtPath(Path("/0"), PrefixTreeMatch::ExactPath));
    }

    TEST_F(DomPrefixTreeTests, DetachSubTreeAtNonExistingPath)
    {
        DomPrefixTree<int> tree;
        tree.SetValue(Path("/foo"), 40);
        tree.SetValue(Path("/foo/0"), 80);

        DomPrefixTree<int> detachedTree = tree.DetachSubTree(AZ::Dom::Path("/bar"));

        // Validate that the returned tree is empty since a non-existing path is provided.
        EXPECT_TRUE(detachedTree.IsEmpty());

        // Validate that the values in the original tree are still present.
        EXPECT_EQ(40, *tree.ValueAtPath(Path("/foo"), PrefixTreeMatch::ExactPath));
        EXPECT_EQ(80, *tree.ValueAtPath(Path("/foo/0"), PrefixTreeMatch::ExactPath));
    }

    TEST_F(DomPrefixTreeTests, AttachSubTreeAddsNewNode)
    {
        DomPrefixTree<int> tree;
        tree.SetValue(Path("/foo"), 40);

        DomPrefixTree<int> treeToAttach;
        treeToAttach.SetValue(Path(), 80);
        treeToAttach.SetValue(Path("/1"), 120);
        tree.AttachSubTree(Path("/foo/0"), AZStd::move(treeToAttach));

        // Validate that the new values is present at the paths in the original tree.
        EXPECT_EQ(80, *tree.ValueAtPath(Path("/foo/0"), PrefixTreeMatch::ExactPath));
        EXPECT_EQ(120, *tree.ValueAtPath(Path("/foo/0/1"), PrefixTreeMatch::ExactPath));
    }

    TEST_F(DomPrefixTreeTests, AttachSubTreeReplacesExistingNode)
    {
        DomPrefixTree<int> tree;

        tree.SetValue(Path("/foo"), 40);

        DomPrefixTree<int> treeToAttach;
        treeToAttach.SetValue(Path(), 80);
        treeToAttach.SetValue(Path("/1"), 120);
        tree.AttachSubTree(Path("/foo"), AZStd::move(treeToAttach));

        // Validate that existing values are replaced in the original tree.
        EXPECT_EQ(80, *tree.ValueAtPath(Path("/foo"), PrefixTreeMatch::ExactPath));
        EXPECT_EQ(120, *tree.ValueAtPath(Path("/foo/1"), PrefixTreeMatch::ExactPath));
    }

    TEST_F(DomPrefixTreeTests, ClearTree)
    {
        DomPrefixTree<int> tree;

        tree.SetValue(Path(), 20);
        tree.SetValue(Path("/foo"), 40);

        tree.Clear();

        EXPECT_EQ(-10, tree.ValueAtPathOrDefault(Path("/foo"), -10, PrefixTreeMatch::PathAndParents));
        EXPECT_TRUE(tree.IsEmpty());
    }

    TEST_F(DomPrefixTreeTests, VisitMissingExactPath)
    {
        TestVisitPath(Path("/bar"), PrefixTreeTraversalFlags::ExcludeChildPaths | PrefixTreeTraversalFlags::ExcludeParentPaths);
        EXPECT_EQ(0, VisitorResultCount());
    }

    TEST_F(DomPrefixTreeTests, VisitExactPath)
    {
        TestVisitPath(Path("/foo/0"), PrefixTreeTraversalFlags::ExcludeChildPaths | PrefixTreeTraversalFlags::ExcludeParentPaths);
        EXPECT_EQ(1, VisitorResultCount());
        EXPECT_TRUE(ValidateResult(Path("/foo/0"), 0));
    }

    TEST_F(DomPrefixTreeTests, VisitChildren)
    {
        TestVisitPath(Path("/foo"), PrefixTreeTraversalFlags::ExcludeExactPath | PrefixTreeTraversalFlags::ExcludeParentPaths);
        EXPECT_EQ(2, VisitorResultCount());
        EXPECT_TRUE(ValidateResult(Path("/foo/0"), 0));
        EXPECT_TRUE(ValidateResult(Path("/foo/1"), 42));
    }

    TEST_F(DomPrefixTreeTests, VisitPathAndChildren)
    {
        TestVisitPath(Path("/foo"), PrefixTreeTraversalFlags::ExcludeParentPaths);
        EXPECT_EQ(3, VisitorResultCount());
        EXPECT_TRUE(ValidateResult(Path("/foo"), 99));
        EXPECT_TRUE(ValidateResult(Path("/foo/0"), 0));
        EXPECT_TRUE(ValidateResult(Path("/foo/1"), 42));
    }

    TEST_F(DomPrefixTreeTests, VisitEntireTree)
    {
        TestVisitPath(Path(), PrefixTreeTraversalFlags::ExcludeExactPath);
        EXPECT_EQ(5, VisitorResultCount());
        EXPECT_TRUE(ValidateResult(Path("/foo"), 99));
        EXPECT_TRUE(ValidateResult(Path("/foo/0"), 0));
        EXPECT_TRUE(ValidateResult(Path("/foo/1"), 42));
        EXPECT_TRUE(ValidateResult(Path("/bar/bat"), 1));
        EXPECT_TRUE(ValidateResult(Path("/bar/baz"), 2));
    }

    TEST_F(DomPrefixTreeTests, VisitEntireTree_DepthFirst)
    {
        SetVisitDepthFirst(true);
        TestVisitPath(Path(), PrefixTreeTraversalFlags::ExcludeExactPath);
        EXPECT_EQ(5, VisitorResultCount());
        EXPECT_TRUE(ValidateResult(Path("/foo"), 99));
        EXPECT_TRUE(ValidateResult(Path("/foo/0"), 0));
        EXPECT_TRUE(ValidateResult(Path("/foo/1"), 42));
        EXPECT_TRUE(ValidateResult(Path("/bar/bat"), 1));
        EXPECT_TRUE(ValidateResult(Path("/bar/baz"), 2));
    }

    TEST_F(DomPrefixTreeTests, VisitParentsAndSelf)
    {
        TestVisitPath(Path("/foo/0"), PrefixTreeTraversalFlags::ExcludeChildPaths);
        EXPECT_EQ(2, VisitorResultCount());
        EXPECT_TRUE(ValidateResult(Path("/foo"), 99));
        EXPECT_TRUE(ValidateResult(Path("/foo/0"), 0));
    }

    TEST_F(DomPrefixTreeTests, VisitParentsOnly)
    {
        TestVisitPath(Path("/foo/0"), PrefixTreeTraversalFlags::ExcludeExactPath | PrefixTreeTraversalFlags::ExcludeChildPaths);
        EXPECT_EQ(1, VisitorResultCount());
        EXPECT_TRUE(ValidateResult(Path("/foo"), 99));
    }

    TEST_F(DomPrefixTreeTests, EarlyTerminateVisit)
    {
        size_t visitCalls = 0;
        auto visitorFn = [&visitCalls](const Path&, int)
        {
            ++visitCalls;
            return false;
        };
        m_runData->m_visitorTree.VisitPath(Path(), visitorFn, PrefixTreeTraversalFlags::TraverseLeastToMostSpecific);
        EXPECT_EQ(visitCalls, 1);
        visitCalls = 0;
        m_runData->m_visitorTree.VisitPath(Path(), visitorFn, PrefixTreeTraversalFlags::TraverseMostToLeastSpecific);
        EXPECT_EQ(visitCalls, 1);
    }

    TEST_F(DomPrefixTreeTests, RemoveChildrenDuringVisit)
    {
        // Remove children during a depth-first traversal
        auto visitorFn = [&](const Path& path, int)
        {
            m_runData->m_visitorTree.EraseValue(path, true);
            return true;
        };
        m_runData->m_visitorTree.VisitPath(Path(), visitorFn, PrefixTreeTraversalFlags::TraverseMostToLeastSpecific);

        // Ensure all entries were successfully deleted
        TestVisitPath(Path(), PrefixTreeTraversalFlags::None);
        EXPECT_EQ(0, VisitorResultCount());
    }
} // namespace AZ::Dom::Tests
