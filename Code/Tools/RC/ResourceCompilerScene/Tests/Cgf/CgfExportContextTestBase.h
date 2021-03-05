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

#include <Cry_Geo.h>
#include <Mocks/MockCGFContent.h>
#include <RC/ResourceCompilerScene/Common/CommonExportContexts.h>
#include <RC/ResourceCompilerScene/Cgf/CgfExportContexts.h>
#include <SceneAPI/SceneCore/Events/CallProcessorBus.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Events/ExportProductList.h>
#include <SceneAPI/SceneCore/Mocks/DataTypes/Groups/MockIMeshGroup.h>
#include <gmock/gmock.h>

#pragma once

namespace AZ
{
    namespace RC
    {
        using ::testing::StrictMock;
        namespace SceneEvents = SceneAPI::Events;

        enum TestContextType
        {
            TestContextMeshGroup,
            TestContextContainer,
            TestContextNode,
            TestContextMeshNode,
            TestContextCount
        };

        typedef AZStd::pair<TestContextType, Phase> ContextPhaseTuple;

        class CgfExporterContextTestBase
            : public ::testing::TestWithParam<ContextPhaseTuple>
        {
        public:
            CgfExporterContextTestBase()
                : m_stubScene(m_sampleSceneName)
                , m_outContent(m_sampleOutputDirectory.c_str())
                , m_sampleNodeIndex(m_stubScene.GetGraph().Find("InvalidNodeName"))
                , m_stubContext(nullptr)
                , m_stubMeshGroupExportContext(m_productList, m_stubScene, m_sampleOutputDirectory, m_stubMeshGroup, GetParam().second)
                , m_stubContainerExportContext(m_stubScene, m_sampleOutputDirectory, m_stubMeshGroup, m_outContent, GetParam().second)
                , m_stubNodeExportContext(m_stubContainerExportContext, m_outNode, m_sampleNodeName, m_sampleNodeIndex, m_samplePhysGeomType, m_sampleRootBoneName, GetParam().second)
                , m_stubMeshNodeExportContext(m_stubNodeExportContext, m_outMesh, GetParam().second)
            {
                m_outContent.GetExportInfo()->bWantF32Vertices = false;
            }
            ~CgfExporterContextTestBase() override = default;

        protected:
            void SetUp() override
            {
                switch(GetParam().first)
                {
                case TestContextMeshGroup:
                    m_stubContext = &m_stubMeshGroupExportContext;
                    break;
                case TestContextContainer:
                    m_stubContext = &m_stubContainerExportContext;
                    break;
                case TestContextNode:
                    m_stubContext = &m_stubNodeExportContext;
                    break;
                case TestContextMeshNode:
                    m_stubContext = &m_stubMeshNodeExportContext;
                    break;
                default:
                    m_stubContext = nullptr;
                    break;
                }
            }

            void TearDown() override
            {
            }

            void UpdateNodeIndex(SceneAPI::Containers::SceneGraph::NodeIndex nodeIndex)
            {
                m_sampleNodeIndex = nodeIndex;
                m_stubNodeExportContext.m_nodeIndex = nodeIndex;
                m_stubMeshNodeExportContext.m_nodeIndex = nodeIndex;
            }

            // Sample Context Data Payloads
            AZ::SceneAPI::Events::ExportProductList m_productList;
            AZStd::string m_sampleSceneName = "SampleScene";
            SceneAPI::Containers::Scene m_stubScene;
            AZStd::string m_sampleOutputDirectory = "TEST:\\Sample\\Output";
            AZStd::string m_sampleGroupName = "SampleGroupName";
            SceneAPI::DataTypes::MockIMeshGroup m_stubMeshGroup;
            CContentCGF m_outContent;
            CNodeCGF m_outNode;
            AZStd::string m_sampleNodeName = "SampleNodeName";
            // Note that m_sampleNodeIndex will always be invalid and fetched using a non existent node
            // from the graph. This is not important for the tests, just needs to be present as a parameter
            SceneAPI::Containers::SceneGraph::NodeIndex m_sampleNodeIndex;
            EPhysicsGeomType m_samplePhysGeomType = PHYS_GEOM_TYPE_NONE;
            AZStd::string m_sampleRootBoneName;
            CMesh m_outMesh;

            // Sample Context Types
            SceneEvents::ICallContext* m_stubContext;
            CgfGroupExportContext m_stubMeshGroupExportContext;
            ContainerExportContext m_stubContainerExportContext;
            NodeExportContext m_stubNodeExportContext;
            MeshNodeExportContext m_stubMeshNodeExportContext;
        };
    }
}