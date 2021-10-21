/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzTest/AzTest.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Serialization/Json/JsonSerializationSettings.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UnitTest/Mocks/MockFileIOBase.h>
#include <AzCore/UnitTest/Mocks/MockSettingsRegistry.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>
#include <SceneBuilder/SceneBuilderWorker.h>
#include <SceneAPI/SceneCore/Events/ExportProductList.h>
#include <Tests/FileIOBaseTestTypes.h>

using namespace AZ;
using namespace SceneBuilder;

class SceneBuilderTests
    : public UnitTest::AllocatorsFixture
    , public UnitTest::TraceBusRedirector
{
protected:
    void SetUp() override
    {
        AZ::SettingsRegistryInterface* registry = AZ::SettingsRegistry::Get();
        auto projectPathKey =
            AZ::SettingsRegistryInterface::FixedValueString(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey) + "/project_path";
        registry->Set(projectPathKey, "AutomatedTesting");
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(*registry);

        m_app.Start(AZ::ComponentApplication::Descriptor());
        AZ::Debug::TraceMessageBus::Handler::BusConnect();
        // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
        // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash
        // in the unit tests.
        AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

        m_workingDirectory = m_app.GetExecutableFolder();
        AZ::IO::FileIOBase::GetInstance()->SetAlias("@products@", m_workingDirectory);
    }

    void TearDown() override
    {
        AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
        m_app.Stop();
    }

    void TestSuccessCase(const SceneAPI::Events::ExportProduct& exportProduct,
        const AssetBuilderSDK::ProductPathDependencySet& expectedPathDependencies,
        const AZStd::vector<AZ::Uuid>& expectedProductDependencies)
    {
        SceneBuilderWorker worker;
        AssetBuilderSDK::JobProduct jobProduct(exportProduct.m_filename, exportProduct.m_assetType, 0);
        worker.PopulateProductDependencies(exportProduct, m_workingDirectory, jobProduct);

        ASSERT_EQ(expectedPathDependencies.size(), jobProduct.m_pathDependencies.size());
        if (expectedPathDependencies.size() > 0)
        {
            for (const AssetBuilderSDK::ProductPathDependency& dependency : expectedPathDependencies)
            {
                ASSERT_TRUE(jobProduct.m_pathDependencies.find(dependency) != jobProduct.m_pathDependencies.end());
            }
        }
        ASSERT_EQ(expectedProductDependencies.size(), jobProduct.m_dependencies.size());
        if (expectedProductDependencies.size() > 0)
        {
            for (const AssetBuilderSDK::ProductDependency& dependency : jobProduct.m_dependencies)
            {
                ASSERT_TRUE(AZStd::find(expectedProductDependencies.begin(), expectedProductDependencies.end(), dependency.m_dependencyId.m_guid) != expectedProductDependencies.end());
            }
        }
    }

    void TestSuccessCase(const SceneAPI::Events::ExportProduct& exportProduct,
        const AssetBuilderSDK::ProductPathDependency* expectedPathDependency = nullptr,
        const AZ::Uuid* expectedProductDependency = nullptr)
    {
        AssetBuilderSDK::ProductPathDependencySet expectedPathDependencies;
        if (expectedPathDependency)
        {
            expectedPathDependencies.emplace(*expectedPathDependency);
        }

        AZStd::vector<AZ::Uuid> expectedProductDependencies;
        if (expectedProductDependency)
        {
            expectedProductDependencies.push_back(*expectedProductDependency);
        }

        TestSuccessCase(exportProduct, expectedPathDependencies, expectedProductDependencies);
    }

    void TestSuccessCaseNoDependencies(const SceneAPI::Events::ExportProduct& exportProduct)
    {
        AssetBuilderSDK::ProductPathDependencySet expectedPathDependencies;
        AZStd::vector<AZ::Uuid> expectedProductDependencies;
        TestSuccessCase(exportProduct, expectedPathDependencies, expectedProductDependencies);
    }

    AzToolsFramework::ToolsApplication m_app;
    const char* m_workingDirectory = nullptr;
};

TEST_F(SceneBuilderTests, SceneBuilderWorker_ExportProductDependencies_NoDependencies)
{
    SceneAPI::Events::ExportProduct exportProduct("testExportFile", AZ::Uuid::CreateRandom(), AZ::Data::AssetType::CreateNull(), u8(0), AZStd::nullopt);
    TestSuccessCaseNoDependencies(exportProduct);
}

TEST_F(SceneBuilderTests, SceneBuilderWorker_ExportProductDependencies_PathDependencyOnSourceAsset)
{
#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
    const char* absolutePathToFile = "C:/some/test/file.mtl";
#else
    const char* absolutePathToFile = "/some/test/file.mtl";
#endif // AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
    AssetBuilderSDK::ProductPathDependency expectedPathDependency(absolutePathToFile, AssetBuilderSDK::ProductPathDependencyType::SourceFile);

    SceneAPI::Events::ExportProduct product("testExportFile", AZ::Uuid::CreateRandom(), AZ::Data::AssetType::CreateNull(), u8(0), AZStd::nullopt);
    product.m_legacyPathDependencies.push_back(absolutePathToFile);
    TestSuccessCase(product, &expectedPathDependency);

}

TEST_F(SceneBuilderTests, SceneBuilderWorker_ExportProductDependencies_PathDependencyOnProductAsset)
{
    const char* relativeDependencyPathToFile = "some/test/file.mtl";

    AssetBuilderSDK::ProductPathDependency expectedPathDependency(relativeDependencyPathToFile, AssetBuilderSDK::ProductPathDependencyType::ProductFile);

    SceneAPI::Events::ExportProduct product("testExportFile", AZ::Uuid::CreateRandom(), AZ::Data::AssetType::CreateNull(), u8(0), AZStd::nullopt);
    product.m_legacyPathDependencies.push_back(relativeDependencyPathToFile);

    TestSuccessCase(product, &expectedPathDependency);
}

TEST_F(SceneBuilderTests, SceneBuilderWorker_ExportProductDependencies_PathDependencyOnSourceAndProductAsset)
{
    const char* relativeDependencyPathToFile = "some/test/file.mtl";

#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
    const char* absolutePathToFile = "C:/some/test/file.mtl";
#else
    const char* absolutePathToFile = "/some/test/file.mtl";
#endif // AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS

    SceneAPI::Events::ExportProduct exportProduct("testExportFile", AZ::Uuid::CreateRandom(), AZ::Data::AssetType::CreateNull(), u8(0), AZStd::nullopt);
    exportProduct.m_legacyPathDependencies.push_back(absolutePathToFile);
    exportProduct.m_legacyPathDependencies.push_back(relativeDependencyPathToFile);

    AssetBuilderSDK::ProductPathDependencySet expectedPathDependencies = {
        {absolutePathToFile, AssetBuilderSDK::ProductPathDependencyType::SourceFile},
        {relativeDependencyPathToFile, AssetBuilderSDK::ProductPathDependencyType::ProductFile}
    };
    TestSuccessCase(exportProduct, expectedPathDependencies, {});
}

TEST_F(SceneBuilderTests, SceneBuilderWorker_ExportProductDependencies_ProductDependency)
{
    AZ::Uuid dependencyId = AZ::Uuid::CreateRandom();
    SceneAPI::Events::ExportProduct exportProduct("testExportFile", AZ::Uuid::CreateRandom(), AZ::Data::AssetType::CreateNull(), AZ::u8(0), AZStd::nullopt);
    exportProduct.m_productDependencies.push_back(SceneAPI::Events::ExportProduct("testDependencyFile", dependencyId, AZ::Data::AssetType::CreateNull(), AZ::u8(0), AZStd::nullopt));

    TestSuccessCase(exportProduct, nullptr, &dependencyId);
}

TEST_F(SceneBuilderTests, SceneBuilderWorker_ExportProductDependencies_ProductAndPathDependencies)
{
    AZ::Uuid dependencyId = AZ::Uuid::CreateRandom();
    SceneAPI::Events::ExportProduct exportProduct("testExportFile", AZ::Uuid::CreateRandom(), AZ::Data::AssetType::CreateNull(), AZ::u8(0), AZStd::nullopt);
    exportProduct.m_productDependencies.push_back(SceneAPI::Events::ExportProduct("testDependencyFile", dependencyId, AZ::Data::AssetType::CreateNull(), AZ::u8(0), AZStd::nullopt));

    const char* relativeDependencyPathToFile = "some/test/file.mtl";

#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
    const char* absolutePathToFile = "C:/some/test/file.mtl";
#else
    const char* absolutePathToFile = "/some/test/file.mtl";
#endif // AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS

    exportProduct.m_legacyPathDependencies.push_back(absolutePathToFile);
    exportProduct.m_legacyPathDependencies.push_back(relativeDependencyPathToFile);

    AssetBuilderSDK::ProductPathDependencySet expectedPathDependencies = {
        {absolutePathToFile, AssetBuilderSDK::ProductPathDependencyType::SourceFile},
        {relativeDependencyPathToFile, AssetBuilderSDK::ProductPathDependencyType::ProductFile}
    };

    TestSuccessCase(exportProduct, expectedPathDependencies, { dependencyId });
}

struct ImportHandler
    : SceneAPI::Events::AssetImportRequestBus::Handler
{
    ImportHandler()
    {
        BusConnect();
    }

    ~ImportHandler() override
    {
        BusDisconnect();
    }

    void GetManifestDependencyPaths(AZStd::vector<AZStd::string>& paths) override
    {
        paths.emplace_back("/scriptFilename");
        paths.emplace_back("/layer1/layer2/0/target");
    }

    void GetManifestExtension(AZStd::string& result) override
    {
        result = ".test";
    }

    void GetGeneratedManifestExtension(AZStd::string& result) override
    {
        result = ".test.gen";
    }
};

using SourceDependencyTests = UnitTest::ScopedAllocatorSetupFixture;

namespace SourceDependencyJson
{
    constexpr const char* TestJson = R"JSON(
{
    "values": [
        {
            "$type": "Test1",
            "scriptFilename": "a/test/path.png"
        },
        {
            "$type": "Test2",
            "layer1" : {
                "layer2" : [
                    {
                        "target": "value.png",
                        "otherData": "value2.png"
                    },
                    {
                        "target" : "wrong.png"
                    }
                ]
            }
        }
    ]
}
    )JSON";
}

TEST_F(SourceDependencyTests, SourceDependencyTest)
{
    ImportHandler handler;
    AZStd::vector<AssetBuilderSDK::SourceFileDependency> dependencies;

    SceneBuilderWorker::PopulateSourceDependencies(SourceDependencyJson::TestJson, dependencies);

    ASSERT_EQ(dependencies.size(), 2);
    ASSERT_STREQ(dependencies[0].m_sourceFileDependencyPath.c_str(), "a/test/path.png");
    ASSERT_STREQ(dependencies[1].m_sourceFileDependencyPath.c_str(), "value.png");
}

struct SourceDependencyMockedIOTests : UnitTest::ScopedAllocatorSetupFixture
    , UnitTest::SetRestoreFileIOBaseRAII
{
    SourceDependencyMockedIOTests()
        : UnitTest::SetRestoreFileIOBaseRAII(m_ioMock)
    {
        
    }

    void SetUp() override
    {
        using namespace ::testing;

        ON_CALL(m_ioMock, Open(_, _, _))
            .WillByDefault(Invoke(
                [](auto, auto, IO::HandleType& handle)
                {
                    handle = 1234;
                    return AZ::IO::Result(AZ::IO::ResultCode::Success);
                }));

        ON_CALL(m_ioMock, Size(An<AZ::IO::HandleType>(), _)).WillByDefault(Invoke([](auto, AZ::u64& size)
        {
            size = strlen(SourceDependencyJson::TestJson);
            return AZ::IO::ResultCode::Success;
        }));

        EXPECT_CALL(m_ioMock, Read(_, _, _, _, _))
            .WillRepeatedly(Invoke(
                [](auto, void* buffer, auto, auto, AZ::u64* bytesRead)
                {
                    memcpy(buffer, SourceDependencyJson::TestJson, strlen(SourceDependencyJson::TestJson));
                    *bytesRead = strlen(SourceDependencyJson::TestJson);
                    return AZ::IO::ResultCode::Success;
                }));

        EXPECT_CALL(m_ioMock, Close(_)).WillRepeatedly(Return(AZ::IO::ResultCode::Success));
    }

    IO::NiceFileIOBaseMock m_ioMock;
};

TEST_F(SourceDependencyMockedIOTests, RegularManifestHasPriority)
{
    ImportHandler handler;
    MockSettingsRegistry settingsRegistry;
    SettingsRegistry::Register(&settingsRegistry);
    using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
    auto MockGetFixedStringCall = [](FixedValueString& result, AZStd::string_view) -> bool
    {
        result = "cache";
        return true;
    };
    ON_CALL(settingsRegistry, Get(::testing::An<FixedValueString&>(), ::testing::_)).WillByDefault(MockGetFixedStringCall);

    AssetBuilderSDK::CreateJobsRequest request;
    AssetBuilderSDK::CreateJobsResponse response;

    request.m_sourceFile = "file.fbx";

    using namespace ::testing;

    AZStd::string genPath = AZStd::string("cache").append(1, AZ_TRAIT_OS_PATH_SEPARATOR).append("file.fbx.test.gen");

    EXPECT_CALL(m_ioMock, Exists(StrEq("file.fbx.test"))).WillRepeatedly(Return(true));
    EXPECT_CALL(m_ioMock, Exists(StrEq(genPath.c_str()))).Times(Exactly(0));
    
    ASSERT_TRUE(SceneBuilderWorker::ManifestDependencyCheck(request, response));
    ASSERT_EQ(response.m_sourceFileDependencyList.size(), 2);
    SettingsRegistry::Unregister(&settingsRegistry);
}

TEST_F(SourceDependencyMockedIOTests, GeneratedManifestTest)
{
    ImportHandler handler;
    MockSettingsRegistry settingsRegistry;
    SettingsRegistry::Register(&settingsRegistry);
    using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
    auto MockGetFixedStringCall = [](FixedValueString& result, AZStd::string_view) -> bool
    {
        result = "cache";
        return true;
    };
    ON_CALL(settingsRegistry, Get(::testing::An<FixedValueString&>(), ::testing::_)).WillByDefault(MockGetFixedStringCall);

    AssetBuilderSDK::CreateJobsRequest request;
    AssetBuilderSDK::CreateJobsResponse response;

    request.m_sourceFile = "file.fbx";

    using namespace ::testing;

    AZStd::string genPath = AZStd::string("cache").append(1, AZ_TRAIT_OS_PATH_SEPARATOR).append("file.fbx.test.gen");

    EXPECT_CALL(m_ioMock, Exists(StrEq("file.fbx.test"))).WillRepeatedly(Return(false));
    EXPECT_CALL(m_ioMock, Exists(StrEq(genPath.c_str()))).WillRepeatedly(Return(true));

    ASSERT_TRUE(SceneBuilderWorker::ManifestDependencyCheck(request, response));
    ASSERT_EQ(response.m_sourceFileDependencyList.size(), 2);
    SettingsRegistry::Unregister(&settingsRegistry);
}
