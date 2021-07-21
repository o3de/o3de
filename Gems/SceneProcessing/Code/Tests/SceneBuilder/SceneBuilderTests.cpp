/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzTest/AzTest.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <SceneBuilder/SceneBuilderWorker.h>
#include <SceneAPI/SceneCore/Events/ExportProductList.h>

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
        AZ::IO::FileIOBase::GetInstance()->SetAlias("@root@", m_workingDirectory);
        AZ::IO::FileIOBase::GetInstance()->SetAlias("@assets@", m_workingDirectory);
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
    SceneAPI::Events::ExportProduct exportProduct("testExportFile", AZ::Uuid::CreateRandom(), AZ::Data::AssetType::CreateNull(), 0, AZStd::nullopt);
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
    
    SceneAPI::Events::ExportProduct product("testExportFile", AZ::Uuid::CreateRandom(), AZ::Data::AssetType::CreateNull(), 0, AZStd::nullopt);
    product.m_legacyPathDependencies.push_back(absolutePathToFile);
    TestSuccessCase(product, &expectedPathDependency);

}

TEST_F(SceneBuilderTests, SceneBuilderWorker_ExportProductDependencies_PathDependencyOnProductAsset)
{
    const char* relativeDependencyPathToFile = "some/test/file.mtl";

    AssetBuilderSDK::ProductPathDependency expectedPathDependency(relativeDependencyPathToFile, AssetBuilderSDK::ProductPathDependencyType::ProductFile);
    
    SceneAPI::Events::ExportProduct product("testExportFile", AZ::Uuid::CreateRandom(), AZ::Data::AssetType::CreateNull(), 0, AZStd::nullopt);
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

    SceneAPI::Events::ExportProduct exportProduct("testExportFile", AZ::Uuid::CreateRandom(), AZ::Data::AssetType::CreateNull(), 0, AZStd::nullopt);
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
    SceneAPI::Events::ExportProduct exportProduct("testExportFile", AZ::Uuid::CreateRandom(), AZ::Data::AssetType::CreateNull(), 0, AZStd::nullopt);
    exportProduct.m_productDependencies.push_back(SceneAPI::Events::ExportProduct("testDependencyFile", dependencyId, AZ::Data::AssetType::CreateNull(), 0, AZStd::nullopt));

    TestSuccessCase(exportProduct, nullptr, &dependencyId);
}

TEST_F(SceneBuilderTests, SceneBuilderWorker_ExportProductDependencies_ProductAndPathDependencies)
{
    AZ::Uuid dependencyId = AZ::Uuid::CreateRandom();
    SceneAPI::Events::ExportProduct exportProduct("testExportFile", AZ::Uuid::CreateRandom(), AZ::Data::AssetType::CreateNull(), 0, AZStd::nullopt);
    exportProduct.m_productDependencies.push_back(SceneAPI::Events::ExportProduct("testDependencyFile", dependencyId, AZ::Data::AssetType::CreateNull(), 0, AZStd::nullopt));

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
