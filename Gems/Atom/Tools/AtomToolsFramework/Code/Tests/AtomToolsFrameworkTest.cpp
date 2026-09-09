/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Utils/TestUtils/AssetSystemStub.h>
#include <AtomToolsFramework/Graph/AssetStatusReporter.h>
#include <AtomToolsFramework/Graph/GraphCompiler.h>
#include <AtomToolsFramework/Graph/GraphTemplateFileData.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzTest/AzTest.h>
#include <AzTest/Utils.h>

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

    class GraphCompilerLifecycleTestDouble : public AtomToolsFramework::GraphCompiler
    {
    public:
        bool Finish(State state)
        {
            return FinishCompile(state);
        }

        void SetState(State state) override
        {
            m_state = state;
        }
    };

    class AssetSystemJobRequestStub : public AzToolsFramework::AssetSystemJobRequestBus::Handler
    {
    public:
        AssetSystemJobRequestStub()
        {
            BusConnect();
        }

        ~AssetSystemJobRequestStub() override
        {
            BusDisconnect();
        }

        AZ::Outcome<AzToolsFramework::AssetSystem::JobInfoContainer> GetAssetJobsInfo(
            [[maybe_unused]] const AZStd::string& sourcePath, [[maybe_unused]] bool escalateJobs) override
        {
            ++m_requestCount;
            return AZ::Success(AzToolsFramework::AssetSystem::JobInfoContainer{});
        }

        AZ::Outcome<AzToolsFramework::AssetSystem::JobInfoContainer> GetAssetJobsInfoByAssetID(
            [[maybe_unused]] const AZ::Data::AssetId& assetId,
            [[maybe_unused]] bool escalateJobs,
            [[maybe_unused]] bool requireFencing) override
        {
            return AZ::Failure();
        }

        AZ::Outcome<AzToolsFramework::AssetSystem::JobInfoContainer> GetAssetJobsInfoByJobKey(
            [[maybe_unused]] const AZStd::string& jobKey, [[maybe_unused]] bool escalateJobs) override
        {
            return AZ::Failure();
        }

        AZ::Outcome<AzToolsFramework::AssetSystem::JobStatus> GetAssetJobsStatusByJobKey(
            [[maybe_unused]] const AZStd::string& jobKey, [[maybe_unused]] bool escalateJobs) override
        {
            return AZ::Failure();
        }

        AZ::Outcome<AZStd::string> GetJobLog([[maybe_unused]] AZ::u64 jobRunKey) override
        {
            return AZ::Failure();
        }

        size_t m_requestCount = 0;
    };

    TEST(AssetStatusReporterTest, UpdateDrainsAllSettledPaths)
    {
        AssetSystemJobRequestStub assetSystem;
        const AZStd::vector<AZStd::string> sourcePaths = { "first.azsl", "second.shader", "third.material" };
        AtomToolsFramework::AssetStatusReporter reporter(sourcePaths);

        EXPECT_EQ(reporter.Update(), AtomToolsFramework::AssetStatusReporterState::Succeeded);
        EXPECT_EQ(assetSystem.m_requestCount, sourcePaths.size());
    }

    TEST(GraphCompilerLifecycleTest, QueuedReplacementCancelsActiveCompile)
    {
        GraphCompilerLifecycleTestDouble compiler;

        EXPECT_TRUE(compiler.Reset());
        EXPECT_FALSE(compiler.CanCompileGraph());

        // A second reservation requests cancellation and leaves the replacement queued.
        EXPECT_FALSE(compiler.Reset());
        EXPECT_FALSE(compiler.Finish(AtomToolsFramework::GraphCompiler::State::Complete));
        EXPECT_EQ(compiler.GetState(), AtomToolsFramework::GraphCompiler::State::Canceled);

        EXPECT_TRUE(compiler.Reset());
        EXPECT_TRUE(compiler.Finish(AtomToolsFramework::GraphCompiler::State::Complete));
        EXPECT_EQ(compiler.GetState(), AtomToolsFramework::GraphCompiler::State::Complete);
    }

    TEST_F(AtomToolsFrameworkTest, GraphTemplateFileDataSaveSkipsIdenticalFile)
    {
        const AZ::Test::ScopedAutoTempDirectory tempDirectory;
        const AZ::IO::Path templatePath = tempDirectory.Resolve("template.txt");
        const AZ::IO::Path outputPath = tempDirectory.Resolve("output.txt");
        ASSERT_TRUE(AZ::Utils::WriteFile("generated content\n", templatePath.Native()).IsSuccess());

        AtomToolsFramework::GraphTemplateFileData templateData;
        ASSERT_TRUE(templateData.Load(templatePath.Native()));

        bool wroteFile = false;
        ASSERT_TRUE(templateData.Save(outputPath.Native(), &wroteFile));
        EXPECT_TRUE(wroteFile);

        wroteFile = true;
        ASSERT_TRUE(templateData.Save(outputPath.Native(), &wroteFile));
        EXPECT_FALSE(wroteFile);
    }

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
