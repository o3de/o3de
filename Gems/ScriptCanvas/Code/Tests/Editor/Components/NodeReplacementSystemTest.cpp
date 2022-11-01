/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>

#include <Editor/Include/ScriptCanvas/Components/NodeReplacementSystem.h>
#include <Include/ScriptCanvas/Libraries/Core/Method.h>
#include <Tests/Framework/ScriptCanvasUnitTestFixture.h>

namespace ScriptCanvasEditorUnitTest
{
    using namespace AZ;
    using namespace ScriptCanvas;
    using namespace ScriptCanvasEditor;

    static constexpr const char ValidClassMethodNodeKey[] = "{E42861BD-1956-45AE-8DD7-CCFC1E3E5ACF}_Old Test Class_Old Test Method";
    static constexpr const char ValidFreeMethodNodeKey[] = "{E42861BD-1956-45AE-8DD7-CCFC1E3E5ACF}_Old Test Free Method";
    static constexpr const char ValidNodeReplacement[] =
R"({
    "O3DE": {
        "NodeReplacement": {
            "ScriptCanvas": [
                {
                    "OldNode" : {
                        "Uuid": "E42861BD-1956-45AE-8DD7-CCFC1E3E5ACF",
                        "Class": "Old Test Class",
                        "Method": "Old Test Method"
                    },
                    "NewNode" : {
                        "Uuid": "E42861BD-1956-45AE-8DD7-CCFC1E3E5ACF",
                        "Class": "New Test Class",
                        "Method": "New Test Method"
                    }
                },
                {
                    "OldNode" : {
                        "Uuid": "E42861BD-1956-45AE-8DD7-CCFC1E3E5ACF",
                        "Method": "Old Test Free Method"
                    },
                    "NewNode" : {
                        "Uuid": "E42861BD-1956-45AE-8DD7-CCFC1E3E5ACF",
                        "Class": "New Test Class",
                        "Method": "New Test Method"
                    }
                }
            ]
        }
    }
})";

    class OldTestClass
    {
    public:
        AZ_RTTI(OldTestClass, "{A34DB600-4479-4FAC-A049-93FC6AB7C5D0}");
        AZ_CLASS_ALLOCATOR(OldTestClass, AZ::SystemAllocator, 0);

        OldTestClass() = default;
        virtual ~OldTestClass() = default;

        void OldTestMethod() {}
    };

    class ScriptCanvasEditorUnitTest
        : public ScriptCanvasUnitTest::ScriptCanvasUnitTestFixture
        , public ComponentApplicationBus::Handler
    {
    public:
        void SetUp() override
        {
            ScriptCanvasUnitTest::ScriptCanvasUnitTestFixture::SetUp();

            m_settingsRegistry = AZStd::make_unique<AZ::SettingsRegistryImpl>();
            AZ::SettingsRegistry::Register(m_settingsRegistry.get());

            m_behaviorContext = aznew BehaviorContext();
            m_behaviorContext->Method("Old Test Free Method", [](){});
            m_behaviorContext->Class<OldTestClass>("Old Test Class")->Method("Old Test Method", &OldTestClass::OldTestMethod);

            AZ::ComponentApplicationBus::Handler::BusConnect();
        }

        void TearDown() override
        {
            AZ::ComponentApplicationBus::Handler::BusDisconnect();

            delete m_behaviorContext;
            m_behaviorContext = nullptr;

            AZ::SettingsRegistry::Unregister(m_settingsRegistry.get());
            m_settingsRegistry.reset();

            ScriptCanvasUnitTest::ScriptCanvasUnitTestFixture::TearDown();
        }

        //////////////////////////////////////////////////////////////////////////
        // ComponentApplicationMessages
        ComponentApplication* GetApplication() override { return nullptr; }
        void RegisterComponentDescriptor(const ComponentDescriptor*) override { }
        void UnregisterComponentDescriptor(const ComponentDescriptor*) override { }
        void RegisterEntityAddedEventHandler(EntityAddedEvent::Handler&) override { }
        void RegisterEntityRemovedEventHandler(EntityRemovedEvent::Handler&) override { }
        void RegisterEntityActivatedEventHandler(EntityActivatedEvent::Handler&) override { }
        void RegisterEntityDeactivatedEventHandler(EntityDeactivatedEvent::Handler&) override { }
        void SignalEntityActivated(Entity*) override { }
        void SignalEntityDeactivated(Entity*) override { }
        bool AddEntity(Entity*) override { return true; }
        bool RemoveEntity(Entity*) override { return true; }
        bool DeleteEntity(const AZ::EntityId&) override { return true; }
        Entity* FindEntity(const AZ::EntityId&) override { return nullptr; }
        SerializeContext* GetSerializeContext() override { return nullptr; }
        BehaviorContext* GetBehaviorContext() override { return m_behaviorContext; }
        JsonRegistrationContext* GetJsonRegistrationContext() override { return nullptr; }
        const char* GetEngineRoot() const override { return nullptr; }
        const char* GetExecutableFolder() const override { return nullptr; }
        void EnumerateEntities(const EntityCallback& /*callback*/) override {}
        void QueryApplicationType(AZ::ApplicationTypeQuery& /*appType*/) const override {}
        //////////////////////////////////////////////////////////////////////////

        AZStd::unique_ptr<AZ::SettingsRegistryImpl> m_settingsRegistry;
        BehaviorContext* m_behaviorContext;
    };

    TEST_F(ScriptCanvasEditorUnitTest, GetNodeReplacementConfiguration_GetValidConfig_WhileLookingForExistingMethodKey)
    {
        m_settingsRegistry->MergeSettings(ValidNodeReplacement, AZ::SettingsRegistryInterface::Format::JsonMergePatch);
        NodeReplacementSystem testSystem;
        testSystem.LoadReplacementMetadata();

        auto config = testSystem.GetNodeReplacementConfiguration(ValidClassMethodNodeKey);
        EXPECT_TRUE(config.IsValid());
        EXPECT_EQ(config.m_className, "New Test Class");
        EXPECT_EQ(config.m_methodName, "New Test Method");
    }

    TEST_F(ScriptCanvasEditorUnitTest, GetNodeReplacementConfiguration_GetInvalidConfig_WhileLookingForNonExistingKey)
    {
        m_settingsRegistry->MergeSettings(ValidNodeReplacement, AZ::SettingsRegistryInterface::Format::JsonMergePatch);
        NodeReplacementSystem testSystem;
        testSystem.LoadReplacementMetadata();

        auto config = testSystem.GetNodeReplacementConfiguration("");
        EXPECT_FALSE(config.IsValid());
    }

    TEST_F(ScriptCanvasEditorUnitTest, GenerateReplacementId_GetExpectedKey_WhileGivingClassMethodMetadata)
    {
        auto replacementId = NodeReplacementSystem::GenerateReplacementId(
            AZ::Uuid::CreateString("E42861BD-1956-45AE-8DD7-CCFC1E3E5ACF"), "Old Test Class", "Old Test Method");
        EXPECT_EQ(replacementId, ValidClassMethodNodeKey);
    }

    TEST_F(ScriptCanvasEditorUnitTest, GenerateReplacementId_GetExpectedKey_WhileGivingClassMethodMetadataFromJson)
    {
        auto replacementId = NodeReplacementSystem::GenerateReplacementId(
            AZ::Uuid::CreateString("E42861BD-1956-45AE-8DD7-CCFC1E3E5ACF"), "Old Test Class", "Old Test Method");
        EXPECT_EQ(replacementId, ValidClassMethodNodeKey);
    }

    TEST_F(ScriptCanvasEditorUnitTest, GenerateReplacementId_GetExpectedKey_WhileGivingFreeMethodMetadata)
    {
        auto replacementId = NodeReplacementSystem::GenerateReplacementId(
            AZ::Uuid::CreateString("E42861BD-1956-45AE-8DD7-CCFC1E3E5ACF"), "", "Old Test Free Method");
        EXPECT_EQ(replacementId, ValidFreeMethodNodeKey);
    }

    TEST_F(ScriptCanvasEditorUnitTest, GenerateReplacementId_GetExpectedKey_WhileGivingClassMethodNode)
    {
        Nodes::Core::Method* testMethodNode = aznew Nodes::Core::Method();
        testMethodNode->InitializeBehaviorMethod({}, "Old Test Class", "Old Test Method", PropertyStatus::None);
        auto replacementId = NodeReplacementSystem::GenerateReplacementId(testMethodNode);
        EXPECT_EQ(replacementId, ValidClassMethodNodeKey);

        delete testMethodNode;
        testMethodNode = nullptr;
    }

    TEST_F(ScriptCanvasEditorUnitTest, GenerateReplacementId_GetExpectedKey_WhileGivingFreeMethodNode)
    {
        Nodes::Core::Method* testMethodNode = aznew Nodes::Core::Method();
        testMethodNode->InitializeBehaviorMethod({}, "", "Old Test Free Method", PropertyStatus::None);
        auto replacementId = NodeReplacementSystem::GenerateReplacementId(testMethodNode);
        EXPECT_EQ(replacementId, ValidFreeMethodNodeKey);

        delete testMethodNode;
        testMethodNode = nullptr;
    }
}
