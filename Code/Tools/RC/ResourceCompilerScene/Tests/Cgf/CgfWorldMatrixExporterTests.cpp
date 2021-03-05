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

#include <Cry_Geo.h> // Needed for Cry_Matrix34.h
#include <Cry_Matrix34.h>
#include <RC/ResourceCompilerScene/Tests/Cgf/CgfExportContextTestBase.h>
#include <RC/ResourceCompilerScene/Cgf/CgfExportContexts.h>
#include <RC/ResourceCompilerScene/Common/WorldMatrixExporter.h>
#include <SceneAPI/SceneCore/Mocks/DataTypes/GraphData/MockITransform.h>
#include <SceneAPI/SceneCore/Mocks/DataTypes/GraphData/MockIMeshData.h>
#include <SceneAPI/SceneCore/Mocks/DataTypes/ManifestBase/MockISceneNodeSelectionList.h>

namespace AZ
{
    namespace RC
    {
        using ::testing::Const;
        using ::testing::Return;
        using ::testing::ReturnRef;

        class WorldMatrixExporterContextTestBase
            : public CgfExporterContextTestBase
        {
        public:
            WorldMatrixExporterContextTestBase()
                : m_stubTransformData(new SceneAPI::DataTypes::MockITransform())
                , m_stubMeshData(new SceneAPI::DataTypes::MockIMeshData())
                , m_stubTransform(AZ::SceneAPI::DataTypes::MatrixType::CreateTranslation(AZ::Vector3(0.f, 0.f, 1.f)))
            {
            }
            ~WorldMatrixExporterContextTestBase() override = default;

        protected:
            void SetUp()
            {
                CgfExporterContextTestBase::SetUp();

                m_outNode.bIdentityMatrix = true;
                SceneAPI::Containers::SceneGraph& sceneGraph = m_stubScene.GetGraph();
                SceneAPI::Containers::SceneGraph::NodeIndex rootIndex = sceneGraph.GetRoot();
                SceneAPI::Containers::SceneGraph::NodeIndex transformIndex = sceneGraph.AddChild(rootIndex, "SampleTransformData", m_stubTransformData);
                SceneAPI::Containers::SceneGraph::NodeIndex meshIndex = sceneGraph.AddChild(transformIndex, "SampleMeshData", m_stubMeshData);

                UpdateNodeIndex(meshIndex);

                EXPECT_CALL(Const(*m_stubTransformData), GetMatrix())
                    .WillRepeatedly(ReturnRef(m_stubTransform));
                ON_CALL(m_stubMeshGroup, GetRuleContainer())
                    .WillByDefault(ReturnRef(m_ruleContainer));
                ON_CALL(Const(m_stubMeshGroup), GetRuleContainerConst())
                    .WillByDefault(ReturnRef(m_ruleContainer));
            }

            bool TestChangedData()
            {
                return !m_outNode.bIdentityMatrix;
            }

            AZStd::shared_ptr<SceneAPI::DataTypes::MockITransform> m_stubTransformData;
            AZStd::shared_ptr<SceneAPI::DataTypes::MockIMeshData> m_stubMeshData;
            AZ::SceneAPI::DataTypes::MatrixType m_stubTransform;
            WorldMatrixExporter m_testExporter;
            SceneAPI::Containers::RuleContainer m_ruleContainer;
        };

        class WorldMatrixExporterNoOpTestFramework
            : public WorldMatrixExporterContextTestBase
        {
        public:

            ~WorldMatrixExporterNoOpTestFramework() override = default;
        };

        TEST_P(WorldMatrixExporterNoOpTestFramework, Process_UnsupportedContext_OutNodeAtIndentity)
        {
            m_testExporter.Process(m_stubContext);
            EXPECT_FALSE(TestChangedData());
        }

        ContextPhaseTuple g_CgfWorldMatrixExporterTestsUnsupportedContextPhaseTuples[] =
        {
            { TestContextMeshGroup, Phase::Filling },
            { TestContextMeshGroup, Phase::Finalizing },
            { TestContextContainer, Phase::Construction },
            { TestContextContainer, Phase::Filling },
            { TestContextContainer, Phase::Finalizing },
            { TestContextNode, Phase::Construction },
            { TestContextNode, Phase::Finalizing },
            { TestContextMeshNode, Phase::Construction },
            { TestContextMeshNode, Phase::Filling },
            { TestContextMeshNode, Phase::Finalizing }
        };

        INSTANTIATE_TEST_CASE_P(WorldMatrixExporter,
            WorldMatrixExporterNoOpTestFramework,
            ::testing::ValuesIn(g_CgfWorldMatrixExporterTestsUnsupportedContextPhaseTuples));

        class WorldMatrixExporterSimpleTests
            : public WorldMatrixExporterContextTestBase
        {
        public:
            WorldMatrixExporterSimpleTests()
                : m_cacheGenerationContext(m_productList, m_stubScene, m_sampleOutputDirectory, m_stubMeshGroup, Phase::Construction)
            {
            }
            ~WorldMatrixExporterSimpleTests() override = default;

        protected:
            void SetUp() override
            {
                WorldMatrixExporterContextTestBase::SetUp();
                m_testExporter.Process(&m_cacheGenerationContext);
            }

            CgfGroupExportContext m_cacheGenerationContext;
        };

        // Disable the test due to an assert that checking consistency of cached mesh group which lacks a way to support currently
        //TEST_P(WorldMatrixExporterSimpleTests, Process_SupportedContext_OutNodeNotAtIndentity)
        //{
        //    m_testExporter.Process(m_stubContext);
        //    EXPECT_TRUE(TestChangedData());
        //}

        ContextPhaseTuple g_CgfWorldMatrixExporterTestsSupportedContextPhaseTuples[] =
        {
            { TestContextNode, Phase::Filling }
        };

        INSTANTIATE_TEST_CASE_P(WorldMatrixExporter,
            WorldMatrixExporterSimpleTests,
            ::testing::ValuesIn(g_CgfWorldMatrixExporterTestsSupportedContextPhaseTuples));
    } // namespace RC
} // namespace AZ
