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

#include <RC/ResourceCompilerScene/Tests/Cgf/CgfExportContextTestBase.h>
#include <RC/ResourceCompilerScene/Cgf/CgfExportContexts.h>
#include <RC/ResourceCompilerScene/Cgf/CgfGroupExporter.h>
#include <SceneAPI/SceneCore/Mocks/DataTypes/ManifestBase/MockISceneNodeSelectionList.h>

namespace AZ
{
    namespace RC
    {
        using ::testing::Return;
        using ::testing::ReturnRef;
        using ::testing::Const;

        class CgfGroupExporterContextTestBase
            : public CgfExporterContextTestBase
        {
        public:
            CgfGroupExporterContextTestBase()
                : m_testExporter(&m_mockAssetWriter)
            {
            }
            ~CgfGroupExporterContextTestBase() override = default;

        protected:
            StrictMock<MockIAssetWriter> m_mockAssetWriter;
            CgfGroupExporter m_testExporter;
        };

        class CgfGroupExporterNoOpTests
            : public CgfGroupExporterContextTestBase
        {
        public:
            ~CgfGroupExporterNoOpTests() override = default;
        };

        TEST_P(CgfGroupExporterNoOpTests, Process_UnsupportedContext_WriterNotUsed)
        {
            m_testExporter.Process(m_stubContext);
        }

        static const ContextPhaseTuple g_CgfMeshGroupExporterTestsUnsupportedContextPhaseTuples[] =
        {
            { TestContextMeshGroup, Phase::Construction },
            { TestContextMeshGroup, Phase::Finalizing },
            { TestContextContainer, Phase::Construction },
            { TestContextContainer, Phase::Filling },
            { TestContextContainer, Phase::Finalizing },
            { TestContextNode, Phase::Construction },
            { TestContextNode, Phase::Filling },
            { TestContextNode, Phase::Finalizing },
            { TestContextMeshNode, Phase::Construction },
            { TestContextMeshNode, Phase::Filling },
            { TestContextMeshNode, Phase::Finalizing }
        };

        INSTANTIATE_TEST_CASE_P(MeshGroupExporter,
            CgfGroupExporterNoOpTests,
            ::testing::ValuesIn(g_CgfMeshGroupExporterTestsUnsupportedContextPhaseTuples));

        class CgfGroupExporterSimpleTestFramework
            : public CgfGroupExporterContextTestBase
        {
        public:
            ~CgfGroupExporterSimpleTestFramework() override = default;

        protected:
            void SetUp() override
            {
            }

            void TearDown() override
            {
            }
            
            SceneAPI::DataTypes::MockISceneNodeSelectionList m_stubSceneNodeSelectionList;
        };

        TEST_P(CgfGroupExporterSimpleTestFramework, Process_SupportedContextNoNodesSelected_WriterNotUsed)
        {
            AZStd::string testGroupName = "testName";
            EXPECT_CALL(m_stubSceneNodeSelectionList, GetSelectedNodeCount())
                .WillRepeatedly(Return(0));
            EXPECT_CALL(m_stubSceneNodeSelectionList, GetUnselectedNodeCount())
                .WillRepeatedly(Return(0));
            EXPECT_CALL(Const(m_stubMeshGroup), GetSceneNodeSelectionList())
                .WillRepeatedly(ReturnRef(m_stubSceneNodeSelectionList));
            EXPECT_CALL(m_stubMeshGroup, GetName())
                .WillRepeatedly(ReturnRef(testGroupName));
            m_testExporter.Process(&m_stubMeshGroupExportContext);
        }

        static const ContextPhaseTuple g_CgfMeshGroupExporterTestsSupportedContextPhaseTuples[] =
        {
            { TestContextMeshGroup, Phase::Filling }
        };

        INSTANTIATE_TEST_CASE_P(MeshGroupExporter,
            CgfGroupExporterSimpleTestFramework,
            ::testing::ValuesIn(g_CgfMeshGroupExporterTestsSupportedContextPhaseTuples));
    } // namespace RC
} // namespace AZ
