/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <Atom/Utils/TestUtils/AssetSystemStub.h>
#include <AtomToolsFramework/Util/Util.h>

namespace UnitTest
{
    class AtomToolsFrameworkTestEnvironment : public AZ::Test::ITestEnvironment
    {
    protected:
        void SetupEnvironment() override
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
        }

        void TeardownEnvironment() override
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
        }
    };

    class AtomToolsFrameworkTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            m_assetSystemStub.Activate();

            RegisterSourceAsset("objects/upgrades/materials/supercondor.material");
            RegisterSourceAsset("materials/condor.material");
            RegisterSourceAsset("materials/talisman.material");
            RegisterSourceAsset("materials/city.material");
            RegisterSourceAsset("materials/totem.material");
            RegisterSourceAsset("textures/orange.png");
            RegisterSourceAsset("textures/red.png");
            RegisterSourceAsset("textures/gold.png");
            RegisterSourceAsset("textures/fuzz.png");

            m_assetSystemStub.RegisterScanFolder("d:/projects/project1/assets/");
            m_assetSystemStub.RegisterScanFolder("d:/projects/project2/assets/");
            m_assetSystemStub.RegisterScanFolder("d:/o3de/gems/atom/assets/");
            m_assetSystemStub.RegisterScanFolder("d:/o3de/gems/atom/testdata/");
            m_assetSystemStub.RegisterScanFolder("d:/o3de/gems/atom/tools/materialeditor/assets/");
        }

        void TearDown() override
        {
            m_assetSystemStub.Deactivate();
        }

        void RegisterSourceAsset(const AZStd::string& path)
        {
            const AZ::IO::BasicPath assetRootPath = AZ::IO::PathView(m_assetRoot).LexicallyNormal();
            const AZ::IO::BasicPath normalizedPath = AZ::IO::BasicPath(assetRootPath).Append(path).LexicallyNormal();

            AZ::Data::AssetInfo assetInfo = {};
            assetInfo.m_assetId = AZ::Uuid::CreateRandom();
            assetInfo.m_relativePath = normalizedPath.LexicallyRelative(assetRootPath).StringAsPosix();
            m_assetSystemStub.RegisterSourceInfo(normalizedPath.StringAsPosix().c_str(), assetInfo, assetRootPath.StringAsPosix().c_str());
        }

        static constexpr const char* m_assetRoot = "d:/project/assets/";
        AssetSystemStub m_assetSystemStub;
    };

    TEST_F(AtomToolsFrameworkTest, GetExteralReferencePath_Succeeds)
    {
        ASSERT_EQ(AtomToolsFramework::GetExteralReferencePath("", "", true), "");
        ASSERT_EQ(AtomToolsFramework::GetExteralReferencePath("d:/project/assets/materials/condor.material", "", true), "");
        ASSERT_EQ(AtomToolsFramework::GetExteralReferencePath("d:/project/assets/materials/talisman.material", "", false), "");
        ASSERT_EQ(AtomToolsFramework::GetExteralReferencePath("d:/project/assets/materials/talisman.material", "d:/project/assets/textures/gold.png", true), "../textures/gold.png");
        ASSERT_EQ(AtomToolsFramework::GetExteralReferencePath("d:/project/assets/materials/talisman.material", "d:/project/assets/textures/gold.png", false), "textures/gold.png");
        ASSERT_EQ(AtomToolsFramework::GetExteralReferencePath("d:/project/assets/objects/upgrades/materials/supercondor.material", "d:/project/assets/materials/condor.material", true), "../../../materials/condor.material");
        ASSERT_EQ(AtomToolsFramework::GetExteralReferencePath("d:/project/assets/objects/upgrades/materials/supercondor.material", "d:/project/assets/materials/condor.material", false), "materials/condor.material");
        ASSERT_EQ(AtomToolsFramework::GetExteralReferencePath("d:/project/assets/objects/upgrades/materials/supercondor.material", "d:/project/assets/materials/condor.material", false), "materials/condor.material");
        ASSERT_EQ(AtomToolsFramework::GetExteralReferencePath("d:/project/assets/objects/upgrades/materials/supercondor.material", "d:/project/assets/materials/condor.material", false), "materials/condor.material");
    }

    TEST_F(AtomToolsFrameworkTest, IsValidSourceDocumentPath_Succeeds)
    {
        ASSERT_FALSE(AtomToolsFramework::IsValidSourceDocumentPath("c:/somerandomasset.json"));
        ASSERT_FALSE(AtomToolsFramework::IsValidSourceDocumentPath("d:/project/somerandomasset.json"));
        ASSERT_FALSE(AtomToolsFramework::IsValidSourceDocumentPath("d:/projects/somerandomasset.json"));
        ASSERT_FALSE(AtomToolsFramework::IsValidSourceDocumentPath("d:/projects/project1/somerandomasset.json"));
        ASSERT_FALSE(AtomToolsFramework::IsValidSourceDocumentPath("e:/projects/project1/assets/somerandomasset.json"));
        ASSERT_FALSE(AtomToolsFramework::IsValidSourceDocumentPath("e:/projects/project1/assets/subfolder/somerandomasset.json"));
        ASSERT_FALSE(AtomToolsFramework::IsValidSourceDocumentPath("e:/projects/project2/assets/somerandomasset.json"));
        ASSERT_FALSE(AtomToolsFramework::IsValidSourceDocumentPath("e:/o3de/gems/atom/tools/materialeditor/assets/somerandomasset.json"));
        ASSERT_TRUE(AtomToolsFramework::IsValidSourceDocumentPath("d:/projects/project1/assets/somerandomasset.json"));
        ASSERT_TRUE(AtomToolsFramework::IsValidSourceDocumentPath("d:/projects/project1/assets/subfolder/somerandomasset.json"));
        ASSERT_TRUE(AtomToolsFramework::IsValidSourceDocumentPath("d:/projects/project2/assets/somerandomasset.json"));
        ASSERT_TRUE(AtomToolsFramework::IsValidSourceDocumentPath("d:/o3de/gems/atom/tools/materialeditor/assets/somerandomasset.json"));
    }

    TEST_F(AtomToolsFrameworkTest, ValidateDocumentPath_Succeeds)
    {
        AZStd::string testPath;
        ASSERT_FALSE(AtomToolsFramework::ValidateDocumentPath(testPath));
        testPath = "somerandomasset.json";
        ASSERT_FALSE(AtomToolsFramework::ValidateDocumentPath(testPath));
        testPath = "../somerandomasset.json";
        ASSERT_FALSE(AtomToolsFramework::ValidateDocumentPath(testPath));
        testPath = "c:/somerandomasset.json";
        ASSERT_FALSE(AtomToolsFramework::ValidateDocumentPath(testPath));
        testPath = "d:/project/somerandomasset.json";
        ASSERT_FALSE(AtomToolsFramework::ValidateDocumentPath(testPath));
        testPath = "d:/projects/somerandomasset.json";
        ASSERT_FALSE(AtomToolsFramework::ValidateDocumentPath(testPath));
        testPath = "d:/projects/project1/somerandomasset.json";
        ASSERT_FALSE(AtomToolsFramework::ValidateDocumentPath(testPath));
        testPath = "e:/projects/project1/assets/somerandomasset.json";
        ASSERT_FALSE(AtomToolsFramework::ValidateDocumentPath(testPath));
        testPath = "e:/projects/project1/assets/subfolder/somerandomasset.json";
        ASSERT_FALSE(AtomToolsFramework::ValidateDocumentPath(testPath));
        testPath = "e:/projects/project2/assets/somerandomasset.json";
        testPath="e:/o3de/gems/atom/tools/materialeditor/assets/somerandomasset.json";
        ASSERT_FALSE(AtomToolsFramework::ValidateDocumentPath(testPath));
        testPath = "d:/projects/project1/assets/somerandomasset.json";
        ASSERT_TRUE(AtomToolsFramework::ValidateDocumentPath(testPath));
        testPath = "d:/projects/project1/assets/subfolder/somerandomasset.json";
        ASSERT_TRUE(AtomToolsFramework::ValidateDocumentPath(testPath));
        testPath = "d:/projects/project2/assets/somerandomasset.json";
        ASSERT_TRUE(AtomToolsFramework::ValidateDocumentPath(testPath));
        testPath = "d:/o3de/gems/atom/tools/materialeditor/assets/somerandomasset.json";
        ASSERT_TRUE(AtomToolsFramework::ValidateDocumentPath(testPath));
    }

    AZ_UNIT_TEST_HOOK(new AtomToolsFrameworkTestEnvironment);
} // namespace UnitTest
