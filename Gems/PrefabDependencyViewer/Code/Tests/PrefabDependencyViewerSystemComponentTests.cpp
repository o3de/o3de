/* Copyright (c) Contributors to the Open 3D Engine Project
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <PrefabDependencyViewerEditorSystemComponent.h>
#include <PrefabDependencyTreeGenerator.h>

#include <Utils.h>
#include <PrefabDependencyViewerFixture.h>

#include <AzCore/std/algorithm.h>

namespace PrefabDependencyViewer
{
    using DirectedGraph = Utils::DirectedGraph;
    using TestComponent = PrefabDependencyViewerEditorSystemComponent;
    using Outcome       = AZ::Outcome<PrefabDependencyTree, const char*>;
    using NodeList      = AZStd::vector<Utils::Node*>;

    bool str_equal(const char* expected, const char* result)
    {
        return (AZStd::string)expected == (AZStd::string)result;
    }

    void EXPECT_STR_EQ(const char* expected, const char* result)
    {
        EXPECT_EQ(true, str_equal(expected, result));
    }

    TEST_F(PrefabDependencyViewerFixture, INVALID_TEMPLATE_ID)
    {
        Outcome outcome = PrefabDependencyTree::GenerateTreeAndSetRoot(
                            InvalidTemplateId, m_prefabSystemComponent);

        EXPECT_EQ(false, outcome.IsSuccess());
    }

    TEST_F(PrefabDependencyViewerFixture, EMPTY_PREFAB_NO_SOURCE_TEST)
    {
        TemplateId tid = 10;
        EXPECT_CALL(*m_prefabSystemComponent, FindTemplateDom(tid))
            .Times(1)
            .WillRepeatedly(::testing::ReturnRef(m_prefabDomsCases["emptyJSON"]));

        Outcome outcome = PrefabDependencyTree::GenerateTreeAndSetRoot(10, m_prefabSystemComponent);
        EXPECT_EQ(false, outcome.IsSuccess());
    }

    TEST_F(PrefabDependencyViewerFixture, EMPTY_PREFAB_WITH_SOURCE_TEST)
    {
        TemplateId tid = 2000;
        EXPECT_CALL(*m_prefabSystemComponent, FindTemplateDom(tid))
            .Times(1)
            .WillRepeatedly(::testing::ReturnRef(m_prefabDomsCases["emptyJSONWithSource"]));

        Outcome outcome = PrefabDependencyTree::GenerateTreeAndSetRoot(tid, m_prefabSystemComponent);
        EXPECT_EQ(true, outcome.IsSuccess());

        PrefabDependencyTree tree = outcome.GetValue();
        EXPECT_EQ(tid, tree.GetRoot()->GetMetaData().GetTemplateId());
        EXPECT_STR_EQ("Prefabs/emptySavedJSON.prefab", tree.GetRoot()->GetMetaData().GetSource());

        EXPECT_EQ(0, tree.GetChildren(tree.GetRoot()).size());
    }

    TEST_F(PrefabDependencyViewerFixture, NESTED_PREFAB_WITH_ATLEAST_ONE_INVALID_SOURCE_FILE)
    {
        TemplateId tid = 52893;
        EXPECT_CALL(*m_prefabSystemComponent, FindTemplateDom(tid))
            .Times(1)
            .WillRepeatedly(::testing::ReturnRef(m_prefabDomsCases["NestedPrefabWithAtleastOneInvalidNestedInstance"]));

        EXPECT_CALL(*m_prefabSystemComponent, GetTemplateIdFromFilePath(AZ::IO::PathView("Prefabs/goodPrefab.prefab")))
            .Times(1)
            .WillRepeatedly(::testing::Return(5));

        EXPECT_CALL(*m_prefabSystemComponent, GetTemplateIdFromFilePath(AZ::IO::PathView("")))
            .Times(1)
            .WillRepeatedly(::testing::Return(InvalidTemplateId));

        // Depending on which TemplateId stack gets to first
        // you can or can't call FindTemplateDom for GoodNestedPrefab's
        // TemplateId
        EXPECT_CALL(*m_prefabSystemComponent, FindTemplateDom(5))
            .Times(0);

        Outcome outcome = PrefabDependencyTree::GenerateTreeAndSetRoot(tid, m_prefabSystemComponent);
        EXPECT_EQ(false, outcome.IsSuccess());
    }

    NodeList FindNodes(Utils::NodeSet& nodeSet, TemplateId tid, const char* source)
    {
        NodeList nodes;

        for (auto it = nodeSet.begin(); it != nodeSet.end(); ++it)
        {
            Utils::Node* node = *it;
            if (node->GetMetaData().GetTemplateId() == tid
                && str_equal(node->GetMetaData().GetSource(), source))
            {
                nodes.push_back(node);
            }
        }
        return nodes;
    }


    TEST_F(PrefabDependencyViewerFixture, VALID_NESTED_PREFAB)
    {
        TemplateId tid = 2022412;
        EXPECT_CALL(*m_prefabSystemComponent, FindTemplateDom(tid))
            .Times(1)
            .WillRepeatedly(::testing::ReturnRef(m_prefabDomsCases["ValidPrefab"]));

        EXPECT_CALL(*m_prefabSystemComponent, GetTemplateIdFromFilePath(AZ::IO::PathView("Prefabs/level11.prefab")))
            .Times(1)
            .WillRepeatedly(::testing::Return(10000));

        EXPECT_CALL(*m_prefabSystemComponent, GetTemplateIdFromFilePath(AZ::IO::PathView("Prefabs/level12.prefab")))
            .Times(2)
            .WillRepeatedly(::testing::Return(121));

        EXPECT_CALL(*m_prefabSystemComponent, GetTemplateIdFromFilePath(AZ::IO::PathView("Prefabs/level13.prefab")))
            .Times(1)
            .WillRepeatedly(::testing::Return(12141));

        EXPECT_CALL(*m_prefabSystemComponent, FindTemplateDom(10000))
            .Times(1)
            .WillRepeatedly(::testing::ReturnRef(m_prefabDomsCases["level11Prefab"]));

        EXPECT_CALL(*m_prefabSystemComponent, FindTemplateDom(121))
            .Times(2)
            .WillRepeatedly(::testing::ReturnRef(m_prefabDomsCases["level12Prefab"]));

        EXPECT_CALL(*m_prefabSystemComponent, FindTemplateDom(12141))
            .Times(1)
            .WillRepeatedly(::testing::ReturnRef(m_prefabDomsCases["level13Prefab"]));

        EXPECT_CALL(*m_prefabSystemComponent, GetTemplateIdFromFilePath(AZ::IO::PathView("Prefabs/level22.prefab")))
            .Times(1)
            .WillRepeatedly(::testing::Return(240121));

        EXPECT_CALL(*m_prefabSystemComponent, GetTemplateIdFromFilePath(AZ::IO::PathView("Prefabs/level23.prefab")))
            .Times(1)
            .WillRepeatedly(::testing::Return(123));

        EXPECT_CALL(*m_prefabSystemComponent, FindTemplateDom(240121))
            .Times(1)
            .WillRepeatedly(::testing::ReturnRef(m_prefabDomsCases["level22Prefab"]));

        EXPECT_CALL(*m_prefabSystemComponent, FindTemplateDom(123))
            .Times(1)
            .WillRepeatedly(::testing::ReturnRef(m_prefabDomsCases["level23Prefab"]));

        EXPECT_CALL(*m_prefabSystemComponent, GetTemplateIdFromFilePath(AZ::IO::PathView("Prefabs/level31.prefab")))
            .Times(1)
            .WillRepeatedly(::testing::Return(221));

        EXPECT_CALL(*m_prefabSystemComponent, FindTemplateDom(221))
            .Times(1)
            .WillRepeatedly(::testing::ReturnRef(m_prefabDomsCases["level31Prefab"]));

        Outcome outcome = PrefabDependencyTree::GenerateTreeAndSetRoot(tid, m_prefabSystemComponent);
        EXPECT_EQ(true, outcome.IsSuccess());

        PrefabDependencyTree tree = outcome.GetValue();
        EXPECT_EQ(tid, tree.GetRoot()->GetMetaData().GetTemplateId());

        EXPECT_EQ(nullptr, tree.GetRoot()->GetParent());
        EXPECT_EQ(3, tree.GetChildren(tree.GetRoot()).size());

        // Check Level 1 Nodes
        Utils::NodeSet level1Nodes = tree.GetChildren(tree.GetRoot());
        NodeList level11Nodes = FindNodes(level1Nodes, 10000, "Prefabs/level11.prefab");
        EXPECT_EQ(1, level11Nodes.size());

        NodeList level12Nodes = FindNodes(level1Nodes, 121, "Prefabs/level12.prefab");
        EXPECT_EQ(1, level12Nodes.size());

        NodeList level13Nodes = FindNodes(level1Nodes, 12141, "Prefabs/level13.prefab");
        EXPECT_EQ(1, level13Nodes.size());

        EXPECT_EQ(0, FindNodes(level1Nodes, 10000, "asa.prefab").size());

        Utils::Node* level11Node = level11Nodes[0];
        Utils::Node* level12Node = level12Nodes[0];
        Utils::Node* level13Node = level13Nodes[0];

        EXPECT_EQ(tree.GetRoot(), level11Node->GetParent());
        EXPECT_EQ(tree.GetRoot(), level12Node->GetParent());
        EXPECT_EQ(tree.GetRoot(), level13Node->GetParent());

        EXPECT_EQ(1, tree.GetChildren(level11Node).size());
        EXPECT_EQ(0, tree.GetChildren(level12Node).size());
        EXPECT_EQ(2, tree.GetChildren(level13Node).size());

        // Check Level 2 Nodes

        Utils::NodeSet level13Children = tree.GetChildren(level13Node);
        auto it = level13Children.begin();

        Utils::Node* level21Node = *tree.GetChildren(level11Node).begin();
        Utils::Node* level22Node = *it; ++it;
        Utils::Node* level23Node = *it;

        EXPECT_EQ(level11Node, level21Node->GetParent());
        EXPECT_EQ(121, level21Node->GetMetaData().GetTemplateId());
        EXPECT_STR_EQ("Prefabs/level12.prefab", level21Node->GetMetaData().GetSource());

        EXPECT_EQ(level13Node, level22Node->GetParent());
        EXPECT_EQ(240121, level22Node->GetMetaData().GetTemplateId());
        EXPECT_STR_EQ("Prefabs/level22.prefab", level22Node->GetMetaData().GetSource());

        EXPECT_EQ(level13Node, level23Node->GetParent());
        EXPECT_EQ(123, level23Node->GetMetaData().GetTemplateId());
        EXPECT_STR_EQ("Prefabs/level23.prefab", level23Node->GetMetaData().GetSource());

        EXPECT_EQ(0, tree.GetChildren(level21Node).size());
        EXPECT_EQ(0, tree.GetChildren(level22Node).size());
        EXPECT_EQ(1, tree.GetChildren(level23Node).size());

        Utils::Node* level31Node = *tree.GetChildren(level23Node).begin();
        EXPECT_EQ(level23Node, level31Node->GetParent());
        EXPECT_EQ(221, level31Node->GetMetaData().GetTemplateId());
        EXPECT_STR_EQ("Prefabs/level31.prefab", level31Node->GetMetaData().GetSource());

        EXPECT_EQ(0, tree.GetChildren(level31Node).size());
    }

} // namespace PrefabDependencyViewer
