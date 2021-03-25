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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>

#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Mocks/DataTypes/GraphData/MockIBlendShapeData.h>
#include <SceneAPI/SceneCore/Mocks/DataTypes/GraphData/MockIMeshData.h>

#include <Generation/Components/MeshOptimizer/MeshOptimizerComponent.h>
#include <InitSceneAPIFixture.h>

namespace AZ::SceneGenerationComponents
{
    class HasBlendShapesFixture
        : public SceneProcessing::InitSceneAPIFixture
    {
    public:
        HasBlendShapesFixture()
            // NodeIndex's constructor is private. We can't initialize this value until SetUp when the allocators and
            // the graph are made. So just get some value in place to be filled in later.
            : m_meshIndex(*reinterpret_cast<SceneAPI::Containers::SceneGraph::NodeIndex*>(&m_meshIndex))
        {
        }

        void SetUp() override
        {
            SceneProcessing::InitSceneAPIFixture::SetUp();
            m_graph = AZStd::make_unique<SceneAPI::Containers::SceneGraph>();
            const SceneAPI::Containers::SceneGraph::NodeIndex root = GetGraph().GetRoot();
            m_meshIndex = GetGraph().AddChild(root, "testMesh", AZStd::make_shared<SceneAPI::DataTypes::MockIMeshData>());
        }

        SceneAPI::Containers::SceneGraph& GetGraph() { return *m_graph; }
        SceneAPI::Containers::SceneGraph::NodeIndex GetMeshNodeIndex() { return m_meshIndex; }

    private:
        AZStd::unique_ptr<SceneAPI::Containers::SceneGraph> m_graph;
        SceneAPI::Containers::SceneGraph::NodeIndex m_meshIndex;
    };

    TEST_F(HasBlendShapesFixture, DoesNotHaveBlendShapes)
    {
        EXPECT_FALSE(SceneGenerationComponents::MeshOptimizerComponent::HasAnyBlendShapeChild(GetGraph(), GetMeshNodeIndex()));
    }

    TEST_F(HasBlendShapesFixture, HasBlendShapes)
    {
        GetGraph().AddChild(GetMeshNodeIndex(), "blendShape", AZStd::make_shared<SceneAPI::DataTypes::MockIBlendShapeData>());

        EXPECT_TRUE(SceneGenerationComponents::MeshOptimizerComponent::HasAnyBlendShapeChild(GetGraph(), GetMeshNodeIndex()));
    }
} // namespace AZ::SceneGenerationComponents
