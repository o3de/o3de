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

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzTest/AzTest.h>
#include <SceneAPI/FbxSceneBuilder/Importers/ImporterUtilities.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/DataTypes/IGraphObject.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>
#include <SceneAPI/SceneData/GraphData/BoneData.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace FbxSceneBuilder
        {
            TEST(FbxImporterUtilityTests, AreSceneGraphsEqual_EmptySceneGraphs_ReturnsTrue)
            {
                Containers::SceneGraph lhsGraph;
                Containers::SceneGraph rhsGraph;

                bool sceneGraphsEqual =
                    (AreSceneGraphsEqual(lhsGraph, rhsGraph) && AreSceneGraphsEqual(rhsGraph, lhsGraph));

                EXPECT_TRUE(sceneGraphsEqual);
            }

            TEST(FbxImporterUtilityTests, AreSceneGraphsEqual_SameNameSingleNodeBothNull_ReturnsTrue)
            {
                Containers::SceneGraph lhsGraph;
                lhsGraph.AddChild(lhsGraph.GetRoot(), "testChild");
                Containers::SceneGraph rhsGraph;
                rhsGraph.AddChild(rhsGraph.GetRoot(), "testChild");

                bool sceneGraphsEqual =
                    (AreSceneGraphsEqual(lhsGraph, rhsGraph) && AreSceneGraphsEqual(rhsGraph, lhsGraph));

                EXPECT_TRUE(sceneGraphsEqual);
            }

            TEST(FbxImporterUtilityTests, AreSceneGraphsEqual_SameNameSingleNodeSameType_ReturnsTrue)
            {
                Containers::SceneGraph lhsGraph;
                AZStd::shared_ptr<DataTypes::IGraphObject> lhsData = AZStd::make_shared<SceneData::GraphData::MeshData>();
                lhsGraph.AddChild(lhsGraph.GetRoot(), "testChild", AZStd::move(lhsData));
                Containers::SceneGraph rhsGraph;
                AZStd::shared_ptr<DataTypes::IGraphObject> rhsData = AZStd::make_shared<SceneData::GraphData::MeshData>();
                rhsGraph.AddChild(rhsGraph.GetRoot(), "testChild", AZStd::move(rhsData));

                bool sceneGraphsEqual =
                    (AreSceneGraphsEqual(lhsGraph, rhsGraph) && AreSceneGraphsEqual(rhsGraph, lhsGraph));

                EXPECT_TRUE(sceneGraphsEqual);
            }

            TEST(FbxImporterUtilityTests, AreSceneGraphsEqual_SameNameSingleNodeOneNull_ReturnsFalse)
            {
                Containers::SceneGraph lhsGraph;
                AZStd::shared_ptr<DataTypes::IGraphObject> lhsData = AZStd::make_shared<SceneData::GraphData::MeshData>();
                lhsGraph.AddChild(lhsGraph.GetRoot(), "testChild", AZStd::move(lhsData));
                Containers::SceneGraph rhsGraph;
                rhsGraph.AddChild(rhsGraph.GetRoot(), "testChild");

                bool sceneGraphsEqual =
                    (AreSceneGraphsEqual(lhsGraph, rhsGraph) && AreSceneGraphsEqual(rhsGraph, lhsGraph));

                EXPECT_FALSE(sceneGraphsEqual);
            }

            TEST(FbxImporterUtilityTests, AreSceneGraphsEqual_SameNameSingleNodeDifferentTypes_ReturnsFalse)
            {
                Containers::SceneGraph lhsGraph;
                AZStd::shared_ptr<DataTypes::IGraphObject> lhsData = AZStd::make_shared<SceneData::GraphData::MeshData>();
                lhsGraph.AddChild(lhsGraph.GetRoot(), "testChild", AZStd::move(lhsData));
                Containers::SceneGraph rhsGraph;
                AZStd::shared_ptr<DataTypes::IGraphObject> rhsData = AZStd::make_shared<SceneData::GraphData::BoneData>();
                rhsGraph.AddChild(rhsGraph.GetRoot(), "testChild", AZStd::move(rhsData));

                bool sceneGraphsEqual =
                    (AreSceneGraphsEqual(lhsGraph, rhsGraph) && AreSceneGraphsEqual(rhsGraph, lhsGraph));

                EXPECT_FALSE(sceneGraphsEqual);
            }

            TEST(FbxImporterUtilityTests, AreSceneGraphsEqual_SameNameOneEmptyOneSingleNode_ReturnsFalse)
            {
                Containers::SceneGraph lhsGraph;
                AZStd::shared_ptr<DataTypes::IGraphObject> lhsData = AZStd::make_shared<SceneData::GraphData::MeshData>();
                lhsGraph.AddChild(lhsGraph.GetRoot(), "testChild", AZStd::move(lhsData));
                Containers::SceneGraph rhsGraph;

                bool sceneGraphsEqual =
                    (AreSceneGraphsEqual(lhsGraph, rhsGraph) && AreSceneGraphsEqual(rhsGraph, lhsGraph));

                EXPECT_FALSE(sceneGraphsEqual);
            }

            TEST(FbxImpoterUtilityTests, AreSceneGraphsEqual_DifferentNamesSingleNodeBothNull_ReturnsFalse)
            {
                Containers::SceneGraph lhsGraph;
                lhsGraph.AddChild(lhsGraph.GetRoot(), "testChild");
                Containers::SceneGraph rhsGraph;
                rhsGraph.AddChild(rhsGraph.GetRoot(), "differentName");

                bool sceneGraphsEqual =
                    (AreSceneGraphsEqual(lhsGraph, rhsGraph) && AreSceneGraphsEqual(rhsGraph, lhsGraph));

                EXPECT_FALSE(sceneGraphsEqual);
            }

            TEST(FbxImporterUtilityTests, AreSceneGraphsEqual_SecondGraphExtraChild_ReturnsFalse)
            {
                Containers::SceneGraph lhsGraph;
                lhsGraph.AddChild(lhsGraph.GetRoot(), "testChild");
                lhsGraph.AddChild(lhsGraph.GetRoot(), "extraTestChild");
                Containers::SceneGraph rhsGraph;
                rhsGraph.AddChild(rhsGraph.GetRoot(), "testChild");

                bool sceneGraphsEqual =
                    (AreSceneGraphsEqual(lhsGraph, rhsGraph) && AreSceneGraphsEqual(rhsGraph, lhsGraph));

                EXPECT_FALSE(sceneGraphsEqual);
            }
        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ
