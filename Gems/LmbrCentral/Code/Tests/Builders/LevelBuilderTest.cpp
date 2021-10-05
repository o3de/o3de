/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <Builders/LevelBuilder/LevelBuilderWorker.h>
#include <AzTest/Utils.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Slice/SliceAssetHandler.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <Tests/AZTestShared/Utils/Utils.h>

namespace UnitTest
{
    using namespace LevelBuilder;
    using namespace AZ;
    using namespace AssetBuilderSDK;

    class MockSimpleAsset
    {
    public:
        AZ_TYPE_INFO(MockSimpleAsset, "{A8A04FF5-1D58-450D-8FD4-2641F290B918}");

        static const char* GetFileFilter()
        {
            return "*.txt;";
        }
    };

    class SecondMockSimpleAsset
    {
    public:
        AZ_TYPE_INFO(SecondMockSimpleAsset, "{A443123A-FD95-45F6-9767-35B17DA2072F}");

        static const char* GetFileFilter()
        {
            return "*.txt;*.txt1;*.txt2";
        }
    };

    class ThirdMockSimpleAsset
    {
    public:
        AZ_TYPE_INFO(ThirdMockSimpleAsset, "{0298F78B-76EF-47CE-8812-B0BC80060016}");

        static const char* GetFileFilter()
        {
            return "txt";
        }
    };

    struct MockSimpleAssetRefComponent
        : public AZ::Component
    {
        AZ_COMPONENT(MockSimpleAssetRefComponent, "{7A37EE69-707B-435F-8B8C-B347C454DC6B}");

        static void Reflect(ReflectContext* reflection)
        {
            SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(reflection);

            AzFramework::SimpleAssetReference<MockSimpleAsset>::Register(*serializeContext);
            AzFramework::SimpleAssetReference<SecondMockSimpleAsset>::Register(*serializeContext);
            AzFramework::SimpleAssetReference<ThirdMockSimpleAsset>::Register(*serializeContext);

            if (serializeContext)
            {
                serializeContext->Class<MockSimpleAssetRefComponent, AZ::Component>()
                    ->Field("asset", &MockSimpleAssetRefComponent::m_asset)
                    ->Field("secondAsset", &MockSimpleAssetRefComponent::m_secondAsset)
                    ->Field("thirdAsset", &MockSimpleAssetRefComponent::m_thirdAsset);
            }
        }

        void Activate() override {}
        void Deactivate() override {}

        AzFramework::SimpleAssetReference<MockSimpleAsset> m_asset;
        AzFramework::SimpleAssetReference<SecondMockSimpleAsset> m_secondAsset;
        AzFramework::SimpleAssetReference<ThirdMockSimpleAsset> m_thirdAsset;
    };

    class LevelBuilderTest
        : public ::testing::Test
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

            m_app.Start(m_descriptor);
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

            auto* serializeContext = m_app.GetSerializeContext();

            m_simpleAssetRefDescriptor = MockSimpleAssetRefComponent::CreateDescriptor();
            m_simpleAssetRefDescriptor->Reflect(serializeContext);
        }

        void TearDown() override
        {
            delete m_simpleAssetRefDescriptor;

            m_app.Stop();
            AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
        }

        void TestFailureCase(AZStd::string_view fileName)
        {
            IO::FileIOStream fileStream;
            LevelBuilderWorker worker;
            ProductPathDependencySet productDependencies;

            ASSERT_TRUE(OpenTestFile(fileName, fileStream));
            ASSERT_EQ(productDependencies.size(), 0);
        }

        AZStd::string GetTestFileAliasedPath(AZStd::string_view fileName)
        {
            constexpr char testFileFolder[] = "@engroot@/Gems/LmbrCentral/Code/Tests/Levels/";
            return AZStd::string::format("%s%.*s", testFileFolder, aznumeric_cast<int>(fileName.size()), fileName.data());
        }


        AZStd::string GetTestFileFullPath(AZStd::string_view fileName)
        {
            AZStd::string aliasedPath = GetTestFileAliasedPath(fileName);
            char resolvedPath[AZ_MAX_PATH_LEN];
            AZ::IO::FileIOBase::GetInstance()->ResolvePath(aliasedPath.c_str(), resolvedPath, AZ_MAX_PATH_LEN);
            return AZStd::string(resolvedPath);
        }

        bool OpenTestFile(AZStd::string_view fileName, IO::FileIOStream& fileStream)
        {
            AZStd::string aliasedPath = GetTestFileFullPath(fileName);
            return fileStream.Open(aliasedPath.c_str(), IO::OpenMode::ModeRead | IO::OpenMode::ModeBinary);
        }

        AzToolsFramework::ToolsApplication m_app;
        AZ::ComponentApplication::Descriptor m_descriptor;
        AZ::ComponentDescriptor* m_simpleAssetRefDescriptor;
    };

    TEST_F(LevelBuilderTest, TestLevelData_EmptyFile)
    {
        // Tests processing a leveldata.xml file that is empty
        // Should output 0 dependencies and return false

        TestFailureCase("leveldata_test3.xml");
    }

    TEST_F(LevelBuilderTest, TestLevelData_NoSurfaceTypes)
    {
        // Tests processing a leveldata.xml file that contains no surface types
        // Should output 0 dependencies and return false

        TestFailureCase("leveldata_test4.xml");
    }

    TEST_F(LevelBuilderTest, TestLevelData_NoLevelData)
    {
        // Tests processing a leveldata.xml file that contains no level data
        // Should output 0 dependencies and return false

        TestFailureCase("leveldata_test5.xml");
    }

    TEST_F(LevelBuilderTest, TestLevelData_NonXMLData)
    {
        // Tests processing a leveldata.xml file that is not an xml file
        // Should output 0 dependencies and return false

        TestFailureCase("leveldata_test6.xml");
    }

    TEST_F(LevelBuilderTest, TestLevelData_MalformedXMLData)
    {
        // Tests processing a leveldata.xml file that contains malformed XML
        // Should output 0 dependencies and return false

        TestFailureCase("leveldata_test7.xml");
    }

    //////////////////////////////////////////////////////////////////////////

    TEST_F(LevelBuilderTest, TestMission_MultipleDependencies)
    {
        // Tests processing a mission_*.xml file containing multiple dependencies and no Cloud texture
        // Should output 3 dependencies

        IO::FileIOStream fileStream;

        ASSERT_TRUE(OpenTestFile("mission_mission0_test1.xml", fileStream));

        LevelBuilderWorker worker;
        ProductPathDependencySet productDependencies;

        ASSERT_TRUE(worker.PopulateMissionDependenciesHelper(&fileStream, productDependencies));
        ASSERT_THAT(productDependencies, testing::UnorderedElementsAre(
            ProductPathDependency{ "EngineAssets/Materials/Sky/Sky.mtl", AssetBuilderSDK::ProductPathDependencyType::ProductFile },
            ProductPathDependency{ "EngineAssets/Materials/Water/Ocean_default.mtl", AssetBuilderSDK::ProductPathDependencyType::ProductFile },
            ProductPathDependency{ "Textures/Skys/Night/half_moon.dds", AssetBuilderSDK::ProductPathDependencyType::ProductFile }));
    }

    TEST_F(LevelBuilderTest, TestMission_NoSkyBox)
    {
        // Tests processing a mission_*.xml file with no skybox settings
        // Should output 0 dependencies and return false

        TestFailureCase("mission_mission0_test2.xml");
    }

    TEST_F(LevelBuilderTest, TestMission_NoOcean)
    {
        // Tests processing a mission_*.xml file with no ocean settings
        // Should output 0 dependencies and return false

        TestFailureCase("mission_mission0_test3.xml");
    }

    TEST_F(LevelBuilderTest, TestMission_NoMoon)
    {
        // Tests processing a mission_*.xml file with no moon settings
        // Should output 2 dependencies

        IO::FileIOStream fileStream;

        ASSERT_TRUE(OpenTestFile("mission_mission0_test4.xml", fileStream));

        LevelBuilderWorker worker;
        ProductPathDependencySet productDependencies;

        ASSERT_TRUE(worker.PopulateMissionDependenciesHelper(&fileStream, productDependencies));
        ASSERT_THAT(productDependencies, testing::UnorderedElementsAre(
            ProductPathDependency{ "EngineAssets/Materials/Sky/Sky.mtl", AssetBuilderSDK::ProductPathDependencyType::ProductFile },
            ProductPathDependency{ "EngineAssets/Materials/Water/Ocean_default.mtl", AssetBuilderSDK::ProductPathDependencyType::ProductFile }));
    }

    TEST_F(LevelBuilderTest, TestMission_NoEnvironment)
    {
        // Tests processing a mission_*.xml file with no environment settings
        // Should output 0 dependencies and return false

        TestFailureCase("mission_mission0_test5.xml");
    }

    TEST_F(LevelBuilderTest, TestMission_EmptyFile)
    {
        // Tests processing an empty mission_*.xml
        // Should output 0 dependencies and return false

        TestFailureCase("mission_mission0_test6.xml");
    }

    TEST_F(LevelBuilderTest, TestMission_CloudShadow)
    {
        // Tests processing a mission_*.xml file with cloud shadow texture set
        // Should output 4 dependencies and return true

        using namespace AssetBuilderSDK;

        IO::FileIOStream fileStream;

        ASSERT_TRUE(OpenTestFile("mission_mission0_test7.xml", fileStream));

        LevelBuilderWorker worker;
        ProductPathDependencySet productDependencies;

        ASSERT_TRUE(worker.PopulateMissionDependenciesHelper(&fileStream, productDependencies));
        ASSERT_THAT(productDependencies, testing::UnorderedElementsAre(
            ProductPathDependency{ "EngineAssets/Materials/Sky/Sky.mtl", AssetBuilderSDK::ProductPathDependencyType::ProductFile },
            ProductPathDependency{ "EngineAssets/Materials/Water/Ocean_default.mtl", AssetBuilderSDK::ProductPathDependencyType::ProductFile },
            ProductPathDependency{ "Textures/Skys/Night/half_moon.dds", AssetBuilderSDK::ProductPathDependencyType::ProductFile },
            ProductPathDependency{ "textures/terrain/ftue_megatexture_02.dds", AssetBuilderSDK::ProductPathDependencyType::ProductFile }));
    }

    TEST_F(LevelBuilderTest, DynamicSlice_NoAssetReferences_HasNoProductDependencies)
    {
        LevelBuilderWorker worker;
        AZStd::vector<AssetBuilderSDK::ProductDependency> productDependencies;
        ProductPathDependencySet productPathDependencies;

        AZStd::string filePath(GetTestFileAliasedPath("levelSlice_noAssetReferences.entities_xml"));
        ASSERT_TRUE(AZ::IO::FileIOBase::GetInstance()->Exists(filePath.c_str()));

        worker.PopulateLevelSliceDependenciesHelper(filePath, productDependencies, productPathDependencies);
        ASSERT_EQ(productDependencies.size(), 0);
        ASSERT_EQ(productPathDependencies.size(), 0);
    }

    TEST_F(LevelBuilderTest, DynamicSlice_HasAssetReference_HasCorrectProductDependency)
    {
        LevelBuilderWorker worker;
        AZStd::vector<AssetBuilderSDK::ProductDependency> productDependencies;
        ProductPathDependencySet productPathDependencies;

        AZStd::string filePath(GetTestFileAliasedPath("levelSlice_oneAssetRef.entities_xml"));
        ASSERT_TRUE(AZ::IO::FileIOBase::GetInstance()->Exists(filePath.c_str()));

        worker.PopulateLevelSliceDependenciesHelper(filePath, productDependencies, productPathDependencies);
        ASSERT_EQ(productPathDependencies.size(), 0);
        ASSERT_EQ(productDependencies.size(), 1);
        ASSERT_EQ(productDependencies[0].m_dependencyId.m_guid, AZ::Uuid("A8970A25-5043-5519-A927-F180E7D6E8C1"));
        ASSERT_EQ(productDependencies[0].m_dependencyId.m_subId, 1);
    }

    void BuildSliceWithSimpleAssetReference(const AZStd::vector<AZStd::string>& filePaths, AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies, ProductPathDependencySet& productPathDependencies)
    {
        auto* assetComponent = aznew MockSimpleAssetRefComponent;

        assetComponent->m_asset.SetAssetPath(filePaths[0].c_str());
        assetComponent->m_secondAsset.SetAssetPath(filePaths[1].c_str());
        assetComponent->m_thirdAsset.SetAssetPath(filePaths[2].c_str());

        auto sliceAsset = AZ::Test::CreateSliceFromComponent(assetComponent, AZ::Data::AssetId(AZ::Uuid::CreateRandom(), 0));

        LevelBuilderWorker worker;

        worker.PopulateLevelSliceDependenciesHelper(sliceAsset, productDependencies, productPathDependencies);
    }

    TEST_F(LevelBuilderTest, DynamicSlice_HasPopulatedSimpleAssetReference_HasCorrectProductDependency)
    {
        AZStd::vector<AssetBuilderSDK::ProductDependency> productDependencies;
        ProductPathDependencySet productPathDependencies;
        AZStd::vector<AZStd::string> filePaths = { "some/test/path.txt", "", "" };
        BuildSliceWithSimpleAssetReference(filePaths, productDependencies, productPathDependencies);
        ASSERT_EQ(productDependencies.size(), 0);
        ASSERT_EQ(productPathDependencies.size(), 1);
        ASSERT_EQ(productPathDependencies.begin()->m_dependencyPath, filePaths[0]);
    }

    TEST_F(LevelBuilderTest, DynamicSlice_HasPopulatedSimpleAssetReferencesNoExtension_HasCorrectProductDependency)
    {
        AZStd::vector<AssetBuilderSDK::ProductDependency> productDependencies;
        ProductPathDependencySet productPathDependencies;
        AZStd::vector<AZStd::string> filePaths = { "some/test/path0", "some/test/path1", "some/test/path2" };
        BuildSliceWithSimpleAssetReference(filePaths, productDependencies, productPathDependencies);
        ASSERT_EQ(productDependencies.size(), 0);
        ASSERT_EQ(productPathDependencies.size(), 3);

        ASSERT_THAT(productPathDependencies, testing::UnorderedElementsAre(
            ProductPathDependency{ "some/test/path0.txt", AssetBuilderSDK::ProductPathDependencyType::ProductFile },
            ProductPathDependency{ "some/test/path1.txt", AssetBuilderSDK::ProductPathDependencyType::ProductFile },
            ProductPathDependency{ "some/test/path2.txt", AssetBuilderSDK::ProductPathDependencyType::ProductFile }));
    }

    TEST_F(LevelBuilderTest, DynamicSlice_HasEmptySimpleAssetReference_HasNoProductDependency)
    {
        AZStd::vector<AssetBuilderSDK::ProductDependency> productDependencies;
        ProductPathDependencySet productPathDependencies;
        AZStd::vector<AZStd::string> filePaths = { "", "", "" };
        BuildSliceWithSimpleAssetReference(filePaths, productDependencies, productPathDependencies);
        ASSERT_EQ(productDependencies.size(), 0);
        ASSERT_EQ(productPathDependencies.size(), 0);
    }
}
