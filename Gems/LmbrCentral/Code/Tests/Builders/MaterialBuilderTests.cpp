/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <Builders/MaterialBuilder/MaterialBuilderComponent.h>
#include <AzTest/Utils.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/IO/FileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Utils/Utils.h>

namespace UnitTest
{
    using namespace MaterialBuilder;
    using namespace AZ;

    class MaterialBuilderTests
        : public UnitTest::AllocatorsTestFixture
        , public UnitTest::TraceBusRedirector
    {
    protected:
        void SetUp() override
        {
            UnitTest::AllocatorsTestFixture::SetUp();

            m_app.reset(aznew AzToolsFramework::ToolsApplication);
            m_app->Start(AZ::ComponentApplication::Descriptor());
            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);
            AZ::Debug::TraceMessageBus::Handler::BusConnect();

            const AZStd::string engineRoot = AZ::Test::GetEngineRootPath();
            AZ::IO::FileIOBase::GetInstance()->SetAlias("@engroot@", engineRoot.c_str());

            AZ::IO::Path assetRoot(AZ::Utils::GetProjectPath());
            assetRoot /= "Cache";
            AZ::IO::FileIOBase::GetInstance()->SetAlias("@products@", assetRoot.c_str());
        }

        void TearDown() override
        {
            AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
            m_app->Stop();
            m_app.reset();

            UnitTest::AllocatorsTestFixture::TearDown();
        }

        AZStd::string GetTestFileAliasedPath(AZStd::string_view fileName)
        {
            constexpr char testFileFolder[] = "@engroot@/Gems/LmbrCentral/Code/Tests/Materials/";
            return AZStd::string::format("%s%.*s", testFileFolder, aznumeric_cast<int>(fileName.size()), fileName.data());
        }

        AZStd::string GetTestFileFullPath(AZStd::string_view fileName)
        {
            AZStd::string aliasedPath = GetTestFileAliasedPath(fileName);
            char resolvedPath[AZ_MAX_PATH_LEN];
            AZ::IO::FileIOBase::GetInstance()->ResolvePath(aliasedPath.c_str(), resolvedPath, AZ_MAX_PATH_LEN);
            return AZStd::string(resolvedPath);
        }

        void TestFailureCase(AZStd::string_view fileName, [[maybe_unused]] int expectedErrorCount)
        {
            MaterialBuilderWorker worker;
            AZStd::vector<AZStd::string> resolvedPaths;

            AZStd::string absoluteMatPath = GetTestFileFullPath(fileName);

            AZ_TEST_START_ASSERTTEST;
            ASSERT_FALSE(worker.GetResolvedTexturePathsFromMaterial(absoluteMatPath, resolvedPaths));
            AZ_TEST_STOP_ASSERTTEST(expectedErrorCount * 2); // The assert tests double count AZ errors, so just multiply expected count by 2
            ASSERT_EQ(resolvedPaths.size(), 0);
        }

        void TestSuccessCase(AZStd::string_view fileName, AZStd::vector<const char*>& expectedTextures)
        {
            MaterialBuilderWorker worker;
            AZStd::vector<AZStd::string> resolvedPaths;
            size_t texturesInMaterialFile = expectedTextures.size();

            AZStd::string absoluteMatPath = GetTestFileFullPath(fileName);
            ASSERT_TRUE(worker.GetResolvedTexturePathsFromMaterial(absoluteMatPath, resolvedPaths));
            ASSERT_EQ(resolvedPaths.size(), texturesInMaterialFile);
            if (texturesInMaterialFile > 0)
            {
                ASSERT_THAT(resolvedPaths, testing::ElementsAreArray(expectedTextures));

                AssetBuilderSDK::ProductPathDependencySet dependencies;
                ASSERT_TRUE(worker.PopulateProductDependencyList(resolvedPaths, dependencies));
                ASSERT_EQ(dependencies.size(), texturesInMaterialFile);
            }
        }

        void TestSuccessCase(AZStd::string_view fileName, const char* expectedTexture)
        {
            AZStd::vector<const char*> expectedTextures;
            expectedTextures.push_back(expectedTexture);
            TestSuccessCase(fileName, expectedTextures);
        }

        void TestSuccessCaseNoDependencies(AZStd::string_view fileName)
        {
            AZStd::vector<const char*> expectedTextures;
            TestSuccessCase(fileName, expectedTextures);
        }

        AZStd::unique_ptr<AzToolsFramework::ToolsApplication> m_app;
    };

    TEST_F(MaterialBuilderTests, MaterialBuilder_EmptyFile_ExpectFailure)
    {
        // Should fail in MaterialBuilderWorker::GetResolvedTexturePathsFromMaterial, when checking for the size of the file.
        TestFailureCase("test_mat1.mtl", 1);
    }

    TEST_F(MaterialBuilderTests, MaterialBuilder_MalformedMaterial_NoChildren_ExpectFailure)
    {
        // Should fail in MaterialBuilderWorker::GetResolvedTexturePathsFromMaterial after calling
        //  Internal::GetTexturePathsFromMaterial, which should return an AZ::Failure when both a Textures node and a
        //  SubMaterials node are not found. No other AZ_Errors should be generated.
        TestFailureCase("test_mat2.mtl", 1);
    }

    TEST_F(MaterialBuilderTests, MaterialBuilder_MalformedMaterial_EmptyTexturesNode_NoDependencies)
    {
        TestSuccessCaseNoDependencies("test_mat3.mtl");
    }

    TEST_F(MaterialBuilderTests, MaterialBuilder_MalformedMaterial_EmptySubMaterialNode_ExpectFailure)
    {
        // Should fail in MaterialBuilderWorker::GetResolvedTexturePathsFromMaterial after calling
        //  Internal::GetTexturePathsFromMaterial, which should return an AZ::Failure when a SubMaterials node is present,
        //  but has no children Material node. No other AZ_Errors should be generated.
        TestFailureCase("test_mat4.mtl", 1);
    }

    TEST_F(MaterialBuilderTests, MaterialBuilder_MalformedMaterial_EmptyTextureNode_NoDependencies)
    {
        TestSuccessCaseNoDependencies("test_mat5.mtl");
    }

    TEST_F(MaterialBuilderTests, MaterialBuilder_MalformedMaterial_EmptyMaterialInSubMaterial_ExpectFailure)
    {
        // Should fail in MaterialBuilderWorker::GetResolvedTexturePathsFromMaterial after calling
        //  Internal::GetTexturePathsFromMaterial, which should return an AZ::Failure when a SubMaterials node is present,
        //  but a child Material node has no child Textures node and no child SubMaterials node. No other AZ_Errors should
        //  be generated.
        TestFailureCase("test_mat6.mtl", 1);
    }

    TEST_F(MaterialBuilderTests, MaterialBuilder_MalformedMaterial_EmptyTextureNodeInSubMaterial_NoDependencies)
    {
        TestSuccessCaseNoDependencies("test_mat7.mtl");
    }

#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS // The following test file 'test_mat8.mtl' has a windows-specific absolute path, so this test is only valid on windows
    TEST_F(MaterialBuilderTests, MaterialBuilder_TextureAbsolutePath_NoDependencies)
    {
        TestSuccessCaseNoDependencies("test_mat8.mtl");
    }
#endif

    TEST_F(MaterialBuilderTests, MaterialBuilder_TextureRuntimeAlias_NoDependencies)
    {
        TestSuccessCaseNoDependencies("test_mat9.mtl");
    }

    TEST_F(MaterialBuilderTests, MaterialBuilder_TextureRuntimeTexture_NoDependencies)
    {
        TestSuccessCaseNoDependencies("test_mat10.mtl");
    }

    TEST_F(MaterialBuilderTests, MaterialBuilder_SingleMaterialSingleTexture_ValidSourceFormat)
    {
        // texture referenced is textures/natural/terrain/am_floor_tile_ddn.png
        const char* expectedPath = "textures/natural/terrain/am_floor_tile_ddn.dds";
        TestSuccessCase("test_mat11.mtl", expectedPath);
    }

    TEST_F(MaterialBuilderTests, MaterialBuilder_SingleMaterialSingleTexture_ValidProductFormat)
    {
        // texture referenced is textures/natural/terrain/am_floor_tile_ddn.dds
        const char* expectedPath = "textures/natural/terrain/am_floor_tile_ddn.dds";
        TestSuccessCase("test_mat12.mtl", expectedPath);
    }

    TEST_F(MaterialBuilderTests, MaterialBuilder_SingleMaterialSingleTexture_InvalidSourceFormat_NoDependenices)
    {
        // texture referenced is textures/natural/terrain/am_floor_tile_ddn.txt
        TestSuccessCaseNoDependencies("test_mat13.mtl");
    }

    TEST_F(MaterialBuilderTests, MaterialBuilder_TextureAnimSequence)
    {
        AZStd::vector<const char*> expectedPaths = {
            "path/to/my/textures/test_anim_sequence_01_texture000.dds",
            "path/to/my/textures/test_anim_sequence_01_texture001.dds",
            "path/to/my/textures/test_anim_sequence_01_texture002.dds",
            "path/to/my/textures/test_anim_sequence_01_texture003.dds",
            "path/to/my/textures/test_anim_sequence_01_texture004.dds",
            "path/to/my/textures/test_anim_sequence_01_texture005.dds"
        };
        TestSuccessCase("test_mat14.mtl", expectedPaths);
    }

    TEST_F(MaterialBuilderTests, MaterialBuilder_SingleMaterialMultipleTexture)
    {
        AZStd::vector<const char*> expectedPaths = {
            "engineassets/textures/hex.dds",
            "engineassets/textures/hex_ddn.dds"
        };
        TestSuccessCase("test_mat15.mtl", expectedPaths);
    }

    TEST_F(MaterialBuilderTests, MaterialBuilder_MalformedMaterial_MultipleTextures_OneEmptyTexture)
    {
        TestSuccessCase("test_mat16.mtl", "engineassets/textures/hex_ddn.dds");
    }

    TEST_F(MaterialBuilderTests, MaterialBuilder_SingleMaterialMultipleTexture_ResolveLeadingSeparatorsAndAliases)
    {
        AZStd::vector<const char*> expectedPaths = {
            "engineassets/textures/hex.dds",        // resolved from "/engineassets/textures/hex.dds"
            "engineassets/textures/hex_ddn.dds",    // resolved from "./engineassets/textures/hex_ddn.dds"
            "engineassets/textures/hex_spec.dds"    // resolved from "@products@/engineassets/textures/hex_spec.dds"
        };
        TestSuccessCase("test_mat17.mtl", expectedPaths);
    }

    TEST_F(MaterialBuilderTests, MaterialBuilder_SubMaterialSingleTexture)
    {
        AZStd::vector<const char*> expectedPaths = {
            "engineassets/textures/scratch.dds",
            "engineassets/textures/perlinnoise2d.dds"
        };
        TestSuccessCase("test_mat18.mtl", expectedPaths);
    }

    TEST_F(MaterialBuilderTests, MaterialBuilder_SubMaterialMultipleTexture)
    {
        AZStd::vector<const char*> expectedPaths = {
            "engineassets/textures/scratch.dds",
            "engineassets/textures/scratch_ddn.dds",
            "engineassets/textures/perlinnoise2d.dds",
            "engineassets/textures/perlinnoisenormal_ddn.dds"
        };
        TestSuccessCase("test_mat19.mtl", expectedPaths);
    }
}
