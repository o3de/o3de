/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gmock/gmock.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Module/Environment.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <Application/ToolsApplication.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <SceneAPI/SceneCore/Components/ExportingComponent.h>
#include <SceneAPI/SceneCore/Components/GenerationComponent.h>
#include <SceneAPI/SceneCore/Components/LoadingComponent.h>
#include <SceneAPI/SceneCore/Components/Utilities/EntityConstructor.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Events/ExportEventContext.h>
#include <SceneAPI/SceneCore/Events/GenerateEventContext.h>
#include <SceneAPI/SceneCore/Events/ImportEventContext.h>
#include <SceneAPI/SceneCore/Events/ProcessingResult.h>
#include <SceneAPI/SceneCore/Events/SceneSerializationBus.h>
#include <SceneAPI/SceneCore/Mocks/DataTypes/Groups/MockIGroup.h>
#include <SceneBuilder/SceneBuilderWorker.h>

// This component descriptor allows for a component to be created beforehand,
// and is then returned when an instance of that component is requested.
// This allows for a component to be pre-configured, via EXPECT calls.
template<class ComponentType>
class ComponentSingleton
    : public AZ::ComponentDescriptorDefault<ComponentType>
{
public:
    AZ_CLASS_ALLOCATOR(ComponentSingleton, AZ::SystemAllocator)
    AZ::Component* CreateComponent() override { return m_component; }
    void SetComponent(ComponentType* c) { m_component = c; }
private:
    ComponentType* m_component = nullptr;
};

class TestLoadingComponent
    : public AZ::SceneAPI::SceneCore::LoadingComponent
{
public:
    AZ_RTTI(TestLoadingComponent, "{19B714CA-6AEF-414D-A91C-54E73DF69625}", AZ::SceneAPI::SceneCore::LoadingComponent, AZ::Component)
    using DescriptorType = ComponentSingleton<TestLoadingComponent>;
    AZ_COMPONENT_BASE(TestLoadingComponent);
    AZ_CLASS_ALLOCATOR(TestLoadingComponent, AZ::ComponentAllocator);

    static void Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* sc = azrtti_cast<AZ::SerializeContext*>(context); sc)
        {
            sc->Class<TestLoadingComponent, AZ::SceneAPI::SceneCore::LoadingComponent>()->Version(1);
        }
    }
    TestLoadingComponent() { BindToCall(&TestLoadingComponent::Load); }
    MOCK_CONST_METHOD1(Load, AZ::SceneAPI::Events::ProcessingResult(AZ::SceneAPI::Events::ImportEventContext& context));
};

class TestGenerationComponent
    : public AZ::SceneAPI::SceneCore::GenerationComponent
{
public:
    AZ_RTTI(TestGenerationComponent, "{3350BD61-2EB1-4F77-B1BD-D108795015EE}", AZ::SceneAPI::SceneCore::GenerationComponent, AZ::Component)
    using DescriptorType = ComponentSingleton<TestGenerationComponent>;
    AZ_COMPONENT_BASE(TestGenerationComponent);
    AZ_CLASS_ALLOCATOR(TestGenerationComponent, AZ::ComponentAllocator);

    static void Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* sc = azrtti_cast<AZ::SerializeContext*>(context); sc)
        {
            sc->Class<TestGenerationComponent, AZ::SceneAPI::SceneCore::GenerationComponent>()->Version(1);
        }
    }
    TestGenerationComponent() { BindToCall(&TestGenerationComponent::Generate); }
    MOCK_CONST_METHOD1(Generate, AZ::SceneAPI::Events::ProcessingResult(AZ::SceneAPI::Events::GenerateEventContext& context));
};

class TestExportingComponent
    : public AZ::SceneAPI::SceneCore::ExportingComponent
{
public:
    AZ_RTTI(TestExportingComponent, "{EADA08AD-2068-4607-AA3D-8B17C59696D5}", AZ::SceneAPI::SceneCore::ExportingComponent, AZ::Component)
    using DescriptorType = ComponentSingleton<TestExportingComponent>;
    AZ_COMPONENT_BASE(TestExportingComponent);
    AZ_CLASS_ALLOCATOR(TestExportingComponent, AZ::ComponentAllocator);

    static void Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* sc = azrtti_cast<AZ::SerializeContext*>(context); sc)
        {
            sc->Class<TestExportingComponent, AZ::SceneAPI::SceneCore::ExportingComponent>()->Version(1);
        }
    }
    TestExportingComponent() { BindToCall(&TestExportingComponent::Export); }
    MOCK_CONST_METHOD1(Export, AZ::SceneAPI::Events::ProcessingResult(const AZ::SceneAPI::Events::ExportEventContext& context));
};

// This scene loader handler mocks the LoadScene method, so that the user can
// control the Scene that is generated in the test. But the default handler
// used in production is also responsible for generating the Import events.
// This test is designed to test the order of the import phases, so a method is
// provided to generate those events.
class TestSceneSerializationHandler
    : public AZ::SceneAPI::Events::SceneSerializationBus::Handler
{
public:
    TestSceneSerializationHandler() { BusConnect(); }
    ~TestSceneSerializationHandler() override { BusDisconnect(); }
    MOCK_METHOD3(LoadScene, AZStd::shared_ptr<AZ::SceneAPI::Containers::Scene>(const AZStd::string& sceneFilePath, AZ::Uuid sceneSourceGuid, const AZStd::string& watchFolder));

    void GenerateImportEvents(const AZStd::string& assetFilePath, [[maybe_unused]] const AZ::Uuid& sourceGuid, [[maybe_unused]] const AZStd::string& watchFolder)
    {
        auto loaders = AZ::SceneAPI::SceneCore::EntityConstructor::BuildEntity("Scene Loading", azrtti_typeid<AZ::SceneAPI::SceneCore::LoadingComponent>());
        auto scene = AZStd::make_shared<AZ::SceneAPI::Containers::Scene>("import scene");

        AZ::SceneAPI::Events::ProcessingResultCombiner contextResult;
        contextResult += AZ::SceneAPI::Events::Process<AZ::SceneAPI::Events::PreImportEventContext>(assetFilePath);
        contextResult += AZ::SceneAPI::Events::Process<AZ::SceneAPI::Events::ImportEventContext>(assetFilePath, *scene);
        contextResult += AZ::SceneAPI::Events::Process<AZ::SceneAPI::Events::PostImportEventContext>(*scene);
    }
};

// This fixture attaches the SceneCore and SceneData libraries, and attaches
// the AZ::Environment to them
class SceneBuilderPhasesFixture
    : public UnitTest::LeakDetectionFixture
{
public:
    void SetUp() override
    {
        AZ::SettingsRegistryInterface* registry = AZ::SettingsRegistry::Get();
        auto projectPathKey =
            AZ::SettingsRegistryInterface::FixedValueString(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey) + "/project_path";
        AZ::IO::FixedMaxPath enginePath;
        registry->Get(enginePath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        registry->Set(projectPathKey, (enginePath / "AutomatedTesting").Native());
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(*registry);

        AZ::ComponentApplication::StartupParameters startupParameters;
        startupParameters.m_loadSettingsRegistry = false;
        m_app.Start(AZ::ComponentApplication::Descriptor(), startupParameters);

        // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
        // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash 
        // in the unit tests.
        AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

        m_app.RegisterComponentDescriptor(TestLoadingComponent::CreateDescriptor());
        m_app.RegisterComponentDescriptor(TestGenerationComponent::CreateDescriptor());
        m_app.RegisterComponentDescriptor(TestExportingComponent::CreateDescriptor());

        m_sceneCoreModule = LoadSceneModule("SceneCore");
        m_sceneDataModule = LoadSceneModule("SceneData");
    }

    void TearDown() override
    {
        m_app.Stop();

        UnloadModule(m_sceneCoreModule);
        UnloadModule(m_sceneDataModule);
    }

private:
    static AZStd::unique_ptr<AZ::DynamicModuleHandle> LoadSceneModule(const char* name)
    {
        auto module = AZ::DynamicModuleHandle::Create(name);
        if (!module)
        {
            return {};
        }

        module->Load();
        if (auto init = module->GetFunction<AZ::InitializeDynamicModuleFunction>(AZ::InitializeDynamicModuleFunctionName); init)
        {
            AZStd::invoke(init);
        }
        return module;
    }

    static void UnloadModule(AZStd::unique_ptr<AZ::DynamicModuleHandle>& module)
    {
        if (!module)
        {
            return;
        }
        if (auto uninit = module->GetFunction<AZ::UninitializeDynamicModuleFunction>(AZ::UninitializeDynamicModuleFunctionName); uninit)
        {
            AZStd::invoke(uninit);
        }
        module = nullptr;
    }

    AzToolsFramework::ToolsApplication m_app;
    AZStd::unique_ptr<AZ::DynamicModuleHandle> m_sceneCoreModule;
    AZStd::unique_ptr<AZ::DynamicModuleHandle> m_sceneDataModule;
};

TEST_F(SceneBuilderPhasesFixture, TestProcessingPhases)
{
    auto scene = AZStd::make_shared<AZ::SceneAPI::Containers::Scene>("testScene");
    scene->GetManifest().AddEntry(AZStd::make_shared<AZ::SceneAPI::DataTypes::MockIGroup>());
    scene->SetManifestFilename("testScene.manifest");

    TestSceneSerializationHandler sceneLoadingHandler;
    EXPECT_CALL(sceneLoadingHandler, LoadScene(testing::_, testing::_, testing::_))
        .WillOnce(testing::DoAll(
            testing::Invoke(&sceneLoadingHandler, &TestSceneSerializationHandler::GenerateImportEvents),
            testing::Return(scene)
        ));

    auto* loadingComponent = aznew TestLoadingComponent();
    auto* generationComponent = aznew TestGenerationComponent();
    auto* exportingComponent = aznew TestExportingComponent();

    static_cast<ComponentSingleton<TestLoadingComponent>*>(TestLoadingComponent::CreateDescriptor())->SetComponent(loadingComponent);
    static_cast<ComponentSingleton<TestGenerationComponent>*>(TestGenerationComponent::CreateDescriptor())->SetComponent(generationComponent);
    static_cast<ComponentSingleton<TestExportingComponent>*>(TestExportingComponent::CreateDescriptor())->SetComponent(exportingComponent);

    {
        // Set up the order in which the event handlers should be called

        ::testing::InSequence sequence;
        using ::testing::_;
        EXPECT_CALL(*loadingComponent, Load(_))
            .WillOnce(testing::Return(AZ::SceneAPI::Events::ProcessingResult::Success));
        EXPECT_CALL(*generationComponent, Generate(_))
            .WillOnce(testing::Return(AZ::SceneAPI::Events::ProcessingResult::Success));
        EXPECT_CALL(*exportingComponent, Export(_))
            .WillOnce(testing::Return(AZ::SceneAPI::Events::ProcessingResult::Success));
    }

    SceneBuilder::SceneBuilderWorker worker;
    AssetBuilderSDK::ProcessJobResponse response;

    worker.ProcessJob({}, response);

    // The assertions set up before with the EXPECT_CALL calls are evaluated
    // when the mock objects go out of scope
}
