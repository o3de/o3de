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
#include <RC/ResourceCompilerScene/Common/ContainerSettingsExporter.h>
#include <SceneAPI/SceneCore/Mocks/DataTypes/ManifestBase/MockISceneNodeSelectionList.h>
#include <SceneAPI/SceneCore/Mocks/DataTypes/Rules/MockIMeshAdvancedRule.h>

namespace AZ
{
    namespace RC
    {
        using ::testing::Const;
        using ::testing::Return;
        using ::testing::ReturnRef;
        using ::testing::_;

        class ContainerSettingsExporterContextTestBase
            : public CgfExporterContextTestBase
        {
        public:
            ContainerSettingsExporterContextTestBase()
                : m_stubMeshAdvancedRule(new SceneAPI::DataTypes::MockIMeshAdvancedRule())
            {
                m_ruleContainer.AddRule(m_stubMeshAdvancedRule);
            }

            ~ContainerSettingsExporterContextTestBase() override = default;

        protected:
            // Minimal subset for check
            // - Group has advanced rule
            // - Advanced rule defines 32 bit vertex precision to be true
            void SetUp() override
            {
                CgfExporterContextTestBase::SetUp();

                EXPECT_CALL(*m_stubMeshAdvancedRule, Use32bitVertices())
                    .WillRepeatedly(Return(true));
                EXPECT_CALL(*m_stubMeshAdvancedRule, MergeMeshes())
                    .WillRepeatedly(Return(false));
                ON_CALL(m_stubMeshGroup, GetRuleContainer())
                    .WillByDefault(ReturnRef(m_ruleContainer));
                ON_CALL(Const(m_stubMeshGroup), GetRuleContainerConst())
                    .WillByDefault(ReturnRef(m_ruleContainer));
            }

            bool TestDataChanged()
            {
                return m_outContent.GetExportInfo()->bWantF32Vertices;
            }

            AZStd::shared_ptr<SceneAPI::DataTypes::MockIMeshAdvancedRule> m_stubMeshAdvancedRule;
            SceneAPI::Containers::RuleContainer m_ruleContainer;
            ContainerSettingsExporter m_testExporter;
        };

        class ContainerSettingsExporterNoOpTests
            : public ContainerSettingsExporterContextTestBase
        {
        public:
            ~ContainerSettingsExporterNoOpTests() override = default;
        };

        TEST_P(ContainerSettingsExporterNoOpTests, Process_UnsupportedContext_ExportInfoNotChanged)
        {
            m_testExporter.Process(m_stubContext);
            EXPECT_FALSE(TestDataChanged());
        }

        static const ContextPhaseTuple g_CgfContainerSettingsExporterTestsUnsupportedContextPhaseTuples[] =
        {
            { TestContextMeshGroup, Phase::Construction },
            { TestContextMeshGroup, Phase::Filling },
            { TestContextMeshGroup, Phase::Finalizing },
            { TestContextContainer, Phase::Filling },
            { TestContextContainer, Phase::Finalizing },
            { TestContextNode, Phase::Construction },
            { TestContextNode, Phase::Filling },
            { TestContextNode, Phase::Finalizing },
            { TestContextMeshNode, Phase::Construction },
            { TestContextMeshNode, Phase::Filling },
            { TestContextMeshNode, Phase::Finalizing }
        };

        INSTANTIATE_TEST_CASE_P(ContainerSettingsExporter,
            ContainerSettingsExporterNoOpTests,
            ::testing::ValuesIn(g_CgfContainerSettingsExporterTestsUnsupportedContextPhaseTuples));

        class ContainerSettingsExporterSimpleTests
            : public ContainerSettingsExporterContextTestBase
        {
        public:
            ~ContainerSettingsExporterSimpleTests() override = default;
        };

        TEST_P(ContainerSettingsExporterSimpleTests, Process_SupportedContext_ExportInfoChanged)
        {
            m_testExporter.Process(m_stubContext);
            EXPECT_TRUE(TestDataChanged());
        }

        static const ContextPhaseTuple g_CgfContainerSettingsExporterTestsSupportedContextPhaseTuples[] =
        {
            {TestContextContainer, Phase::Construction}
        };

        INSTANTIATE_TEST_CASE_P(ContainerSettingsExporter,
            ContainerSettingsExporterSimpleTests,
            ::testing::ValuesIn(g_CgfContainerSettingsExporterTestsSupportedContextPhaseTuples));
    } // namespace RC
} // namespace AZ
