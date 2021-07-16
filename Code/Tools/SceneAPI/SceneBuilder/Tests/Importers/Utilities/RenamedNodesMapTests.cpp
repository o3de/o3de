/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzTest/AzTest.h>
#include <SceneAPI/SceneBuilder/Importers/Utilities/RenamedNodesMap.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/DataTypes/IGraphObject.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneBuilder
        {
            TEST(RenamedNodesMapTests, SanitizeNodeName_ValidNameProvided_ReturnsFalseAndNameUnchanged)
            {
                Containers::SceneGraph graph;
                AZStd::string name = "ValidName";

                bool result = RenamedNodesMap::SanitizeNodeName(name, graph, graph.GetRoot());

                EXPECT_FALSE(result);
                EXPECT_STREQ("ValidName", name.c_str());
            }

            TEST(RenamedNodesMapTests, SanitizeNodeName_NameWithInvalidCharacter_ReturnsTrueAndNameChanged)
            {
                Containers::SceneGraph graph;
                AZStd::string check = "Valid";
                check += Containers::SceneGraph::GetNodeSeperationCharacter();
                check += "Name";
                AZStd::string name = check;

                bool result = RenamedNodesMap::SanitizeNodeName(name, graph, graph.GetRoot());

                EXPECT_TRUE(result);
                EXPECT_STRNE(check.c_str(), name.c_str());
            }

            TEST(RenamedNodesMapTests, SanitizeNodeName_BlankName_ReturnsTrueAndNameSetToDefault)
            {
                Containers::SceneGraph graph;
                AZStd::string name;

                bool result = RenamedNodesMap::SanitizeNodeName(name, graph, graph.GetRoot(), "Default");

                EXPECT_TRUE(result);
                EXPECT_STREQ("Default", name.c_str());
            }

            TEST(RenamedNodesMapTests, SanitizeNodeName_SingleCollision_ReturnsTrueAndNameHasAppendixOf1)
            {
                Containers::SceneGraph graph;
                graph.AddChild(graph.GetRoot(), "Child");
                AZStd::string name = "Child";

                bool result = RenamedNodesMap::SanitizeNodeName(name, graph, graph.GetRoot());

                EXPECT_TRUE(result);
                EXPECT_STREQ("Child_1", name.c_str());
            }

            TEST(RenamedNodesMapTests, SanitizeNodeName_MultipleCollisions_ReturnsTrueAndNameHasAppendixOf2)
            {
                Containers::SceneGraph graph;
                auto child = graph.AddChild(graph.GetRoot(), "Child");
                graph.AddSibling(child, "Child_1");
                AZStd::string name = "Child";

                bool result = RenamedNodesMap::SanitizeNodeName(name, graph, graph.GetRoot());

                EXPECT_TRUE(result);
                EXPECT_STREQ("Child_2", name.c_str());
            }
        } // namespace SceneBuilder
    } // namespace SceneAPI
} // namespace AZ
