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
#include <IIndexedMesh.h>
#include <CGFContent.h>
#include <ConvertContext.h>
#include <RC/ResourceCompilerScene/Tests/Cgf/CgfExportContextTestBase.h>
#include <RC/ResourceCompilerScene/Common/MaterialExporter.h>
#include <SceneAPI/SceneCore/Mocks/DataTypes/ManifestBase/MockISceneNodeSelectionList.h>

namespace AZ
{
    namespace RC
    {
        class MaterialExporterContextTestBase
            : public CgfExporterContextTestBase
        {
        public:
            MaterialExporterContextTestBase()
                : m_cacheGenerationContext(m_productList, m_stubScene, m_sampleOutputDirectory, m_stubMeshGroup, Phase::Construction)
                , m_testExporter()
            {
            }
            ~MaterialExporterContextTestBase() override = default;

        protected:
            static const size_t s_testFaceCount = 3;
            static const unsigned char s_testDefaultSubset = 0;

            void SetUp() override
            {
                CgfExporterContextTestBase::SetUp();

                m_outContent.SetCommonMaterial(nullptr);
                m_outNode.pMaterial = nullptr;

                // We require a MeshGroupContext in the Construction phase for correct behavior of 
                // all other contexts
                m_testExporter.Process(&m_cacheGenerationContext);
            }

            virtual bool TestChangedData()
            {
                if (m_outContent.GetCommonMaterial() != nullptr)
                {
                    return true;
                }
                
                if (m_outNode.pMaterial != nullptr)
                {
                    return true;
                }

                return false;
            }

            ConvertContext m_convertContext;
            CgfGroupExportContext m_cacheGenerationContext;
            MaterialExporter m_testExporter;
        };

        class MaterialExporterNoOpTests
            : public MaterialExporterContextTestBase
        {
        public:
            ~MaterialExporterNoOpTests() override = default;

        protected:
            // To 
            void SetUp() override
            {
                MaterialExporterContextTestBase::SetUp();
            }
        };

        TEST_P(MaterialExporterNoOpTests, Process_UnsupportedContext_DataNotChanged)
        {
            m_testExporter.Process(m_stubContext);
            EXPECT_FALSE(TestChangedData());
        }

        static const ContextPhaseTuple g_CgfMaterialExporterTestsUnsupportedContextPhaseTuples[] =
        {
            { TestContextMeshGroup, Phase::Filling },
            { TestContextMeshGroup, Phase::Finalizing }, // Technically changes, but only internal state
            { TestContextContainer, Phase::Filling },
            { TestContextNode, Phase::Construction },
            { TestContextNode, Phase::Finalizing },
            { TestContextMeshNode, Phase::Construction },
            { TestContextMeshNode, Phase::Finalizing }
        };

        INSTANTIATE_TEST_CASE_P(MaterialExporter,
            MaterialExporterNoOpTests,
            ::testing::ValuesIn(g_CgfMaterialExporterTestsUnsupportedContextPhaseTuples));

        class MaterialExporterContainerContextTests
            : public MaterialExporterContextTestBase
        {
        public:
            ~MaterialExporterContainerContextTests() override = default;

        protected:
            // To 
            void SetUp() override
            {
                MaterialExporterContextTestBase::SetUp();
            }
        };

        // Tests still required
        // ContainerContext/Finalizing - Should be trivial
        // NodeContext/Filling - Will require complex setup of internal cache
        // MeshNodeContext/Filling - Will require complex setup of internal cache

        static const ContextPhaseTuple g_CgfMaterialExporterTestsSupportedContextPhaseTuples[] =
        {
            { TestContextContainer, Phase::Construction }
        };

        INSTANTIATE_TEST_CASE_P(MaterialExporter,
            MaterialExporterContainerContextTests,
            ::testing::ValuesIn(g_CgfMaterialExporterTestsSupportedContextPhaseTuples));
    } // namespace RC
} // namespace AZ
