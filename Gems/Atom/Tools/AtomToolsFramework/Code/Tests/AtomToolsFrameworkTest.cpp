/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Utils/TestUtils/AssetSystemStub.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzTest/AzTest.h>

namespace UnitTest
{
    class AtomToolsFrameworkTestEnvironment : public AZ::Test::ITestEnvironment
    {
    protected:
        void SetupEnvironment() override
        {
        }

        void TeardownEnvironment() override
        {
        }
    };

    class AtomToolsFrameworkTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            m_priorFileIO = AZ::IO::FileIOBase::GetInstance();
            m_localFileIO.reset(aznew AZ::IO::LocalFileIO());
            AZ::IO::FileIOBase::SetInstance(m_localFileIO.get());

            char rootPath[AZ_MAX_PATH_LEN];
            AZ::Utils::GetExecutableDirectory(rootPath, AZ_MAX_PATH_LEN);
            AZ::IO::FileIOBase::GetInstance()->SetAlias("@exefolder@", rootPath);

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

            m_assetSystemStub.RegisterScanFolder(AtomToolsFramework::GetPathWithoutAlias("@exefolder@/root1/projects/project1/assets/"));
            m_assetSystemStub.RegisterScanFolder(AtomToolsFramework::GetPathWithoutAlias("@exefolder@/root1/projects/project2/assets/"));
            m_assetSystemStub.RegisterScanFolder(AtomToolsFramework::GetPathWithoutAlias("@exefolder@/root1/o3de/gems/atom/assets/"));
            m_assetSystemStub.RegisterScanFolder(AtomToolsFramework::GetPathWithoutAlias("@exefolder@/root1/o3de/gems/atom/testdata/"));
            m_assetSystemStub.RegisterScanFolder(AtomToolsFramework::GetPathWithoutAlias("@exefolder@/root1/o3de/gems/atom/tools/materialeditor/assets/"));
        }

        void TearDown() override
        {
            m_assetSystemStub.Deactivate();

            AZ::IO::FileIOBase::SetInstance(m_priorFileIO);
            m_localFileIO.reset();
        }

        void RegisterSourceAsset(const AZStd::string& path)
        {
            const AZStd::string assetRoot = "@exefolder@/root1/project/assets/";
            AZ::IO::FixedMaxPath assetRootPath(AtomToolsFramework::GetPathWithoutAlias(assetRoot));
            AZ::IO::FixedMaxPath normalizedPath(AtomToolsFramework::GetPathWithoutAlias(assetRoot + path));

            AZ::Data::AssetInfo assetInfo = {};
            assetInfo.m_assetId = AZ::Uuid::CreateRandom();
            assetInfo.m_relativePath = normalizedPath.LexicallyRelative(assetRootPath).StringAsPosix();
            m_assetSystemStub.RegisterSourceInfo(normalizedPath.StringAsPosix().c_str(), assetInfo, assetRootPath.StringAsPosix().c_str());
        }

        AssetSystemStub m_assetSystemStub;
        AZ::IO::FileIOBase* m_priorFileIO = nullptr;
        AZStd::unique_ptr<AZ::IO::FileIOBase> m_localFileIO;
    };

    TEST_F(AtomToolsFrameworkTest, GetPathToExteralReference_Succeeds)
    {
        ASSERT_EQ(AtomToolsFramework::GetPathToExteralReference("", ""), "");
        ASSERT_EQ(AtomToolsFramework::GetPathToExteralReference("@exefolder@/root1/project/assets/materials/condor.material", ""), "");
        ASSERT_EQ(AtomToolsFramework::GetPathToExteralReference("@exefolder@/root1/project/assets/materials/talisman.material", ""), "");
        ASSERT_EQ(AtomToolsFramework::GetPathToExteralReference("@exefolder@/root1/project/assets/materials/talisman.material", "@exefolder@/root1/project/assets/textures/gold.png"), "../textures/gold.png");
        ASSERT_EQ(AtomToolsFramework::GetPathToExteralReference("@exefolder@/root1/project/assets/objects/upgrades/materials/supercondor.material", "@exefolder@/root1/project/assets/materials/condor.material"), "../../../materials/condor.material");
    }

    TEST_F(AtomToolsFrameworkTest, IsDocumentPathInSupportedFolder_Succeeds)
    {
        ASSERT_FALSE(AtomToolsFramework::IsDocumentPathInSupportedFolder("@exefolder@/root1/somerandomasset.json"));
        ASSERT_FALSE(AtomToolsFramework::IsDocumentPathInSupportedFolder("@exefolder@/root1/project/somerandomasset.json"));
        ASSERT_FALSE(AtomToolsFramework::IsDocumentPathInSupportedFolder("@exefolder@/root1/projects/somerandomasset.json"));
        ASSERT_FALSE(AtomToolsFramework::IsDocumentPathInSupportedFolder("@exefolder@/root1/projects/project1/somerandomasset.json"));
        ASSERT_FALSE(AtomToolsFramework::IsDocumentPathInSupportedFolder("@exefolder@/root2/projects/project1/assets/somerandomasset.json"));
        ASSERT_FALSE(AtomToolsFramework::IsDocumentPathInSupportedFolder("@exefolder@/root2/projects/project1/assets/subfolder/somerandomasset.json"));
        ASSERT_FALSE(AtomToolsFramework::IsDocumentPathInSupportedFolder("@exefolder@/root2/projects/project2/assets/somerandomasset.json"));
        ASSERT_FALSE(AtomToolsFramework::IsDocumentPathInSupportedFolder("@exefolder@/root2/o3de/gems/atom/tools/materialeditor/assets/somerandomasset.json"));
        ASSERT_TRUE(AtomToolsFramework::IsDocumentPathInSupportedFolder("@exefolder@/root1/projects/project1/assets/somerandomasset.json"));
        ASSERT_TRUE(AtomToolsFramework::IsDocumentPathInSupportedFolder("@exefolder@/root1/projects/project1/assets/subfolder/somerandomasset.json"));
        ASSERT_TRUE(AtomToolsFramework::IsDocumentPathInSupportedFolder("@exefolder@/root1/projects/project2/assets/somerandomasset.json"));
        ASSERT_TRUE(AtomToolsFramework::IsDocumentPathInSupportedFolder("@exefolder@/root1/o3de/gems/atom/tools/materialeditor/assets/somerandomasset.json"));
    }

    TEST_F(AtomToolsFrameworkTest, ValidateDocumentPath_Succeeds)
    {
        AZStd::string testPath;
        ASSERT_FALSE(AtomToolsFramework::ValidateDocumentPath(testPath));
        testPath = "somerandomasset.json";
        ASSERT_FALSE(AtomToolsFramework::ValidateDocumentPath(testPath));
        testPath = "../somerandomasset.json";
        ASSERT_FALSE(AtomToolsFramework::ValidateDocumentPath(testPath));
        testPath = "@exefolder@/root1/somerandomasset.json";
        ASSERT_FALSE(AtomToolsFramework::ValidateDocumentPath(testPath));
        testPath = "@exefolder@/root1/project/somerandomasset.json";
        ASSERT_FALSE(AtomToolsFramework::ValidateDocumentPath(testPath));
        testPath = "@exefolder@/root1/projects/somerandomasset.json";
        ASSERT_FALSE(AtomToolsFramework::ValidateDocumentPath(testPath));
        testPath = "@exefolder@/root1/projects/project1/somerandomasset.json";
        ASSERT_FALSE(AtomToolsFramework::ValidateDocumentPath(testPath));
        testPath = "@exefolder@/root2/projects/project1/assets/somerandomasset.json";
        ASSERT_FALSE(AtomToolsFramework::ValidateDocumentPath(testPath));
        testPath = "@exefolder@/root2/projects/project1/assets/subfolder/somerandomasset.json";
        ASSERT_FALSE(AtomToolsFramework::ValidateDocumentPath(testPath));
        testPath = "@exefolder@/root2/projects/project2/assets/somerandomasset.json";
        ASSERT_FALSE(AtomToolsFramework::ValidateDocumentPath(testPath));
        testPath = "@exefolder@/root2/o3de/gems/atom/tools/materialeditor/assets/somerandomasset.json";
        ASSERT_FALSE(AtomToolsFramework::ValidateDocumentPath(testPath));
        testPath = "@exefolder@/root1/projects/project1/assets/somerandomasset.json";
        ASSERT_TRUE(AtomToolsFramework::ValidateDocumentPath(testPath));
        testPath = "@exefolder@/root1/projects/project1/assets/subfolder/somerandomasset.json";
        ASSERT_TRUE(AtomToolsFramework::ValidateDocumentPath(testPath));
        testPath = "@exefolder@/root1/projects/project2/assets/somerandomasset.json";
        ASSERT_TRUE(AtomToolsFramework::ValidateDocumentPath(testPath));
        testPath = "@exefolder@/root1/o3de/gems/atom/tools/materialeditor/assets/somerandomasset.json";
        ASSERT_TRUE(AtomToolsFramework::ValidateDocumentPath(testPath));
    }

    AZ_UNIT_TEST_HOOK(new AtomToolsFrameworkTestEnvironment);
} // namespace UnitTest
