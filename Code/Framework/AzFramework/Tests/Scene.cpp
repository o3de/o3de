/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>

AZ_PUSH_DISABLE_WARNING(, "-Wdelete-non-virtual-dtor")

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/Component.h>
#include <AzFramework/Scene/SceneSystemComponent.h>
#include <AzFramework/Scene/Scene.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/IO/Streamer/StreamerComponent.h>
#include <AzCore/Jobs/JobManagerComponent.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Slice/SliceAssetHandler.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzCore/Slice/SliceSystemComponent.h>

using namespace AzFramework;

// Test component that allows code to be injected into activate / deactivate for testing.

namespace SceneUnitTest
{

    class TestComponent;

    class TestComponentConfig : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(TestComponentConfig, AZ::SystemAllocator)
        AZ_RTTI(TestComponentConfig, "{DCD12D72-3BFE-43A9-9679-66B745814CAF}", ComponentConfig);

        typedef void(*ActivateFunction)(TestComponent* component);
        ActivateFunction m_activateFunction = nullptr;

        typedef void(*DeactivateFunction)(TestComponent* component);
        DeactivateFunction m_deactivateFunction = nullptr;
    };

    static constexpr AZ::TypeId TestComponentTypeId{ "{DC096267-4815-47D1-BA23-A1CDF0D72D9D}" };
    class TestComponent : public AZ::Component
    {
    public:
        AZ_COMPONENT(TestComponent, TestComponentTypeId);

        static void Reflect(AZ::ReflectContext*) {};

        void Activate() override
        {
            if (m_config.m_activateFunction)
            {
                m_config.m_activateFunction(this);
            }
        }

        void Deactivate() override
        {
            if (m_config.m_deactivateFunction)
            {
                m_config.m_deactivateFunction(this);
            }
        }

        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override
        {
            if (auto config = azrtti_cast<const TestComponentConfig*>(baseConfig))
            {
                m_config = *config;
                return true;
            }
            return false;
        }

        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override
        {
            if (auto outConfig = azrtti_cast<TestComponentConfig*>(outBaseConfig))
            {
                *outConfig = m_config;
                return true;
            }
            return false;
        }

        TestComponentConfig m_config;
    };

    // Fixture that creates a bare-bones app with only the system components necesary.

    class SceneTest
        : public UnitTest::LeakDetectionFixture
    {
    public:
        void SetUp() override
        {
            m_prevFileIO = AZ::IO::FileIOBase::GetInstance();
            AZ::IO::FileIOBase::SetInstance(&m_fileIO);

            m_app.RegisterComponentDescriptor(SceneSystemComponent::CreateDescriptor());
            m_app.RegisterComponentDescriptor(AZ::SliceSystemComponent::CreateDescriptor());
            m_app.RegisterComponentDescriptor(AZ::AssetManagerComponent::CreateDescriptor());
            m_app.RegisterComponentDescriptor(AZ::JobManagerComponent::CreateDescriptor());
            m_app.RegisterComponentDescriptor(AZ::StreamerComponent::CreateDescriptor());

            AZ::ComponentApplication::Descriptor desc;
            AZ::ComponentApplication::StartupParameters startupParameters;
            startupParameters.m_loadSettingsRegistry = false;
            m_systemEntity = m_app.Create(desc, startupParameters);
            m_systemEntity->Init();

            m_systemEntity->CreateComponent<SceneSystemComponent>();

            // Asset / slice system components needed by entity contexts
            m_systemEntity->CreateComponent<AZ::SliceSystemComponent>();
            m_systemEntity->CreateComponent<AZ::AssetManagerComponent>();
            m_systemEntity->CreateComponent<AZ::JobManagerComponent>();
            m_systemEntity->CreateComponent<AZ::StreamerComponent>();
            m_systemEntity->Activate();

            m_sceneSystem = AzFramework::SceneSystemInterface::Get();
        }

        void TearDown() override
        {
            m_app.Destroy();

            AZ::IO::FileIOBase::SetInstance(m_prevFileIO);
        }

        AZ::IO::LocalFileIO m_fileIO;
        AZ::IO::FileIOBase* m_prevFileIO;
        AZ::ComponentApplication m_app;
        AZ::Entity* m_systemEntity = nullptr;
        AzFramework::ISceneSystem* m_sceneSystem = nullptr;

    };

    TEST_F(SceneTest, CreateScene)
    {   
        // A scene should be able to be created with a given name.
        AZ::Outcome<AZStd::shared_ptr<Scene>, AZStd::string> createSceneOutcome = m_sceneSystem->CreateScene("TestScene");
        EXPECT_TRUE(createSceneOutcome.IsSuccess()) << "Unable to create a scene.";
    
        // The scene pointer returned should be valid
        AZStd::shared_ptr<Scene> scene = createSceneOutcome.TakeValue();
        EXPECT_NE(scene, nullptr) << "Scene creation reported success, but no scene actually was actually returned.";

        // Attempting to create another scene with the same name should fail.
        createSceneOutcome = m_sceneSystem->CreateScene("TestScene");
        EXPECT_TRUE(!createSceneOutcome.IsSuccess()) << "Should not be able to create two scenes with the same name.";

    }

    TEST_F(SceneTest, GetScene)
    {
        constexpr AZStd::string_view sceneName = "TestScene";

        AZ::Outcome<AZStd::shared_ptr<Scene>, AZStd::string> createSceneOutcome = m_sceneSystem->CreateScene(sceneName);
        AZStd::shared_ptr<Scene> createdScene = createSceneOutcome.TakeValue();

        // Should be able to get a scene by name, and it should match the scene that was created.
        AZStd::shared_ptr<Scene> retrievedScene = m_sceneSystem->GetScene(sceneName);
        EXPECT_NE(retrievedScene, nullptr) << "Attempting to get scene by name resulted in nullptr.";
        EXPECT_EQ(retrievedScene, createdScene) << "Retrieved scene does not match created scene.";

        // An invalid name should return a null scene.
        AZStd::shared_ptr<Scene> nullScene = m_sceneSystem->GetScene("non-existant scene");
        EXPECT_EQ(nullScene, nullptr) << "Should not be able to retrieve a scene that wasn't created.";
    }

    TEST_F(SceneTest, RemoveScene)
    {
        constexpr AZStd::string_view sceneName = "TestScene";

        AZ::Outcome<AZStd::shared_ptr<Scene>, AZStd::string> createSceneOutcome = m_sceneSystem->CreateScene(sceneName);
        bool success = m_sceneSystem->RemoveScene(sceneName);
        EXPECT_TRUE(success) << "Failed to remove the scene that was just created.";

        success = m_sceneSystem->RemoveScene("non-existant scene");
        EXPECT_FALSE(success) << "Remove scene returned success for a non-existant scene.";
    }

    TEST_F(SceneTest, IterateActiveScenes)
    {
        constexpr size_t NumScenes = 5;

        AZStd::shared_ptr<Scene> scenes[NumScenes] = {nullptr};

        for (size_t i = 0; i < NumScenes; ++i)
        {
            AZStd::string sceneName = AZStd::string::format("scene %zu", i);
            AZ::Outcome<AZStd::shared_ptr<Scene>, AZStd::string> createSceneOutcome = m_sceneSystem->CreateScene(sceneName);
            scenes[i] = createSceneOutcome.TakeValue();
        }

        size_t index = 0;
        m_sceneSystem->IterateActiveScenes([&index, &scenes](const AZStd::shared_ptr<Scene>& scene)
            {
                AZStd::shared_ptr<Scene> newscene = scenes[index++];
                EXPECT_EQ(newscene, scene);
                return true;
            });
    }

    TEST_F(SceneTest, IterateZombieScenes)
    {
        constexpr size_t NumScenes = 5;

        AZStd::shared_ptr<Scene> scenes[NumScenes] = {nullptr};

        // Create zombies.
        for (size_t i = 0; i < NumScenes; ++i)
        {
            AZStd::string sceneName = AZStd::string::format("scene %zu", i);
            AZ::Outcome<AZStd::shared_ptr<Scene>, AZStd::string> createSceneOutcome = m_sceneSystem->CreateScene(sceneName);
            scenes[i] = createSceneOutcome.TakeValue();
            m_sceneSystem->RemoveScene(sceneName);
        }

        // Check to make sure there are no more active scenes.
        size_t index = 0;
        m_sceneSystem->IterateActiveScenes([&index](const AZStd::shared_ptr<Scene>&)
            {
                index++;
                return true;
            });
        EXPECT_EQ(0, index);

        // Check that the scenes are still returned as zombies.
        index = 0;
        m_sceneSystem->IterateZombieScenes([&index, &scenes](Scene& scene)
            {
                AZStd::shared_ptr<Scene> zombieScene = scenes[index++];
                EXPECT_EQ(zombieScene.get(), &scene);
                return true;
            });

        // Check that all scenes are removed when there are no more handles.
        for (size_t i = 0; i < NumScenes; ++i)
        {
            scenes[i].reset();
        }
        index = 0;
        m_sceneSystem->IterateZombieScenes([&index](Scene&) {
            index++;
            return true;
        });
        EXPECT_EQ(0, index);
    }

    // Test classes for use in the SceneSystem test. These can't be defined in the test itself due to some functions created by AZ_RTTI not having a body which breaks VS2015.
    class Foo1
    {
    public:
        AZ_RTTI(Foo1, "{9A6AA770-E2EA-4C5E-952A-341802E2DE58}");
    };
    class Foo2
    {
    public:
        AZ_RTTI(Foo2, "{916A2DB4-9C30-4B90-837E-2BC9855B474B}");
    };

    TEST_F(SceneTest, SceneSystem)
    {
        // Create the scene
        AZ::Outcome<AZStd::shared_ptr<Scene>, AZStd::string> createSceneOutcome = m_sceneSystem->CreateScene("TestScene");
        EXPECT_TRUE(createSceneOutcome.IsSuccess());
        AZStd::shared_ptr<Scene> scene = createSceneOutcome.TakeValue();

        // Set a class on the Scene
        Foo1* foo1a = new Foo1();
        EXPECT_TRUE(scene->SetSubsystem(foo1a));

        // Get that class back from the Scene
        EXPECT_EQ(foo1a, *scene->FindSubsystem<Foo1*>());

        // Try to set the same class type twice, this should fail.
        Foo1* foo1b = new Foo1();
        EXPECT_FALSE(scene->SetSubsystem(foo1b));
        delete foo1b;

        // Add a child scene
        createSceneOutcome = m_sceneSystem->CreateSceneWithParent("ChildScene", scene);
        EXPECT_TRUE(createSceneOutcome.IsSuccess());
        AZStd::shared_ptr<Scene> childScene = createSceneOutcome.TakeValue();

        // Get class back from parent scene.
        EXPECT_EQ(foo1a, *childScene->FindSubsystem<Foo1*>());

        // Find overloaded version of class on child scene.
        Foo1* foo1c = new Foo1();
        EXPECT_TRUE(childScene->SetSubsystem(foo1c));
        EXPECT_EQ(foo1c, *childScene->FindSubsystem<Foo1*>());

        // Unset system on child scene, using alternative unset function.
        EXPECT_TRUE(childScene->UnsetSubsystem(foo1c));
        delete foo1c;

        // Try to un-set a class that was never set, this should fail.
        EXPECT_FALSE(scene->UnsetSubsystem<Foo2>());

        // Unset the class that was previously set
        EXPECT_TRUE(scene->UnsetSubsystem<Foo1>());
        delete foo1a;

        // Make sure that the previously set class was really removed.
        EXPECT_EQ(nullptr, scene->FindSubsystem<Foo1*>());
    }
} // UnitTest

AZ_POP_DISABLE_WARNING
