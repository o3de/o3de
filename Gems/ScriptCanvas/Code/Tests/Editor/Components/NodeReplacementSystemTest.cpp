/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/UnitTest/MockComponentApplication.h>

#include <Editor/Include/ScriptCanvas/Components/NodeReplacementSystem.h>
#include <Include/ScriptCanvas/Core/Node.h>
#include <Include/ScriptCanvas/Libraries/Core/Method.h>
#include <Include/ScriptCanvas/Utils/VersioningUtils.h>
#include <Tests/Framework/ScriptCanvasUnitTestFixture.h>

namespace ScriptCanvasEditorUnitTest
{
    using namespace AZ;
    using namespace ScriptCanvas;
    using namespace ScriptCanvasEditor;

    static constexpr const int ValidGraphId = 1234567890;
    static constexpr const char ValidClassMethodNodeKey1[] = "{E42861BD-1956-45AE-8DD7-CCFC1E3E5ACF}_Old Test Class1_Old Test Method1";
    static constexpr const char ValidFreeMethodNodeKey1[] = "{E42861BD-1956-45AE-8DD7-CCFC1E3E5ACF}_Old Test Free Method1";
    static constexpr const char ValidFreeMethodNodeKey2[] = "{E42861BD-1956-45AE-8DD7-CCFC1E3E5ACF}_Old Test Free Method2";
    static constexpr const char ValidOldCustomNodeKey[] = "{F1030112-BA70-4786-BBEB-43ACADA5B846}";
    static constexpr const char ValidNewMethodNodeKey[] = "{E42861BD-1956-45AE-8DD7-CCFC1E3E5ACF}";
    static constexpr const char ValidNodeReplacement1[] =
R"({
    "O3DE": {
        "NodeReplacement": {
            "ScriptCanvas1": [
                {
                    "OldNode" : {
                        "Uuid": "E42861BD-1956-45AE-8DD7-CCFC1E3E5ACF",
                        "Class": "Old Test Class1",
                        "Method": "Old Test Method1"
                    },
                    "NewNode" : {
                        "Uuid": "E42861BD-1956-45AE-8DD7-CCFC1E3E5ACF",
                        "Class": "New Test Class1",
                        "Method": "New Test Method1"
                    }
                },
                {
                    "OldNode" : {
                        "Uuid": "E42861BD-1956-45AE-8DD7-CCFC1E3E5ACF",
                        "Method": "Old Test Free Method1"
                    },
                    "NewNode" : {
                        "Uuid": "E42861BD-1956-45AE-8DD7-CCFC1E3E5ACF",
                        "Class": "New Test Class1",
                        "Method": "New Test Method1"
                    }
                }
            ]
        }
    }
})";

    static constexpr const char ValidNodeReplacement2[] =
R"({
    "O3DE": {
        "NodeReplacement": {
            "ScriptCanvas2": [
                {
                    "OldNode" : {
                        "Uuid": "E42861BD-1956-45AE-8DD7-CCFC1E3E5ACF",
                        "Method": "Old Test Free Method2"
                    },
                    "NewNode" : {
                        "Uuid": "E42861BD-1956-45AE-8DD7-CCFC1E3E5ACF",
                        "Class": "New Test Class2",
                        "Method": "New Test Method2"
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

    class OldTestCustomNode
        : public Node
    {
    public:
        AZ_COMPONENT(OldTestCustomNode, ValidOldCustomNodeKey, Node);
    };

    class ScriptCanvasEditorUnitTest
        : public ScriptCanvasUnitTest::ScriptCanvasUnitTestFixture
    {
    public:
        void SetUp() override
        {
            ScriptCanvasUnitTest::ScriptCanvasUnitTestFixture::SetUp();

            m_settingsRegistry = AZStd::make_unique<AZ::SettingsRegistryImpl>();
            AZ::SettingsRegistry::Register(m_settingsRegistry.get());
            m_componentApplicationMock = AZStd::make_unique<testing::NiceMock<UnitTest::MockComponentApplication>>();
        }

        void TearDown() override
        {
            m_componentApplicationMock.reset();
            AZ::SettingsRegistry::Unregister(m_settingsRegistry.get());
            m_settingsRegistry.reset();

            ScriptCanvasUnitTest::ScriptCanvasUnitTestFixture::TearDown();
        }

    protected:
        AZStd::unique_ptr<AZ::SettingsRegistryImpl> m_settingsRegistry;
        AZStd::unique_ptr<testing::NiceMock<UnitTest::MockComponentApplication>> m_componentApplicationMock;
    };

    TEST_F(ScriptCanvasEditorUnitTest, GenerateReplacementId_GetExpectedKey_WhileGivingClassMethodMetadata)
    {
        auto replacementId = NodeReplacementSystem::GenerateReplacementId(
            AZ::Uuid::CreateString("E42861BD-1956-45AE-8DD7-CCFC1E3E5ACF"), "Old Test Class1", "Old Test Method1");
        EXPECT_EQ(replacementId, ValidClassMethodNodeKey1);
    }

    TEST_F(ScriptCanvasEditorUnitTest, GenerateReplacementId_GetExpectedKey_WhileGivingClassMethodMetadataFromJson)
    {
        auto replacementId = NodeReplacementSystem::GenerateReplacementId(
            AZ::Uuid::CreateString("E42861BD-1956-45AE-8DD7-CCFC1E3E5ACF"), "Old Test Class1", "Old Test Method1");
        EXPECT_EQ(replacementId, ValidClassMethodNodeKey1);
    }

    TEST_F(ScriptCanvasEditorUnitTest, GenerateReplacementId_GetExpectedKey_WhileGivingFreeMethodMetadata)
    {
        auto replacementId = NodeReplacementSystem::GenerateReplacementId(
            AZ::Uuid::CreateString("E42861BD-1956-45AE-8DD7-CCFC1E3E5ACF"), "", "Old Test Free Method1");
        EXPECT_EQ(replacementId, ValidFreeMethodNodeKey1);
    }

    TEST_F(ScriptCanvasEditorUnitTest, GenerateReplacementId_GetExpectedKey_WhileGivingCustomNodeMetadata)
    {
        AZ::Uuid testUuid = AZ::Uuid::CreateRandom();
        auto replacementId = NodeReplacementSystem::GenerateReplacementId(testUuid);
        EXPECT_EQ(replacementId, testUuid.ToFixedString().c_str());
    }

    TEST_F(ScriptCanvasEditorUnitTest, GenerateReplacementId_GetExpectedKey_WhileGivingClassMethodNode)
    {
        ON_CALL(*m_componentApplicationMock, AddEntity(::testing::_))
            .WillByDefault(
                []([[maybe_unused]] AZ::Entity*)
                {
                    return true;
                });
        AZ::BehaviorContext testBehaviorContext;
        testBehaviorContext.Class<OldTestClass>("Old Test Class1").Method("Old Test Method1", &OldTestClass::OldTestMethod);
        ON_CALL(*m_componentApplicationMock, GetBehaviorContext())
            .WillByDefault(
                [&testBehaviorContext]()
                {
                    return &testBehaviorContext;
                });

        Nodes::Core::Method* testMethodNode = aznew Nodes::Core::Method();
        AZ::Entity* testMethodEntity = aznew AZ::Entity();
        testMethodEntity->Init();
        testMethodEntity->AddComponent(testMethodNode);
        testMethodNode->InitializeBehaviorMethod({}, "Old Test Class1", "Old Test Method1", PropertyStatus::None);
        auto replacementId = NodeReplacementSystem::GenerateReplacementId(testMethodNode);
        EXPECT_EQ(replacementId, ValidClassMethodNodeKey1);

        testMethodEntity->Reset();
        delete testMethodEntity;
        testMethodEntity = nullptr;
        testMethodNode = nullptr;
    }

    TEST_F(ScriptCanvasEditorUnitTest, GenerateReplacementId_GetExpectedKey_WhileGivingFreeMethodNode)
    {
        ON_CALL(*m_componentApplicationMock, AddEntity(::testing::_))
            .WillByDefault(
                []([[maybe_unused]] AZ::Entity*)
                {
                    return true;
                });
        AZ::BehaviorContext testBehaviorContext;
        testBehaviorContext.Method("Old Test Free Method1", [](){});
        ON_CALL(*m_componentApplicationMock, GetBehaviorContext())
            .WillByDefault(
                [&testBehaviorContext]()
                {
                    return &testBehaviorContext;
                });
        
        Nodes::Core::Method* testMethodNode = aznew Nodes::Core::Method();
        AZ::Entity* testMethodEntity = aznew AZ::Entity();
        testMethodEntity->Init();
        testMethodEntity->AddComponent(testMethodNode);
        testMethodNode->InitializeBehaviorMethod({}, "", "Old Test Free Method1", PropertyStatus::None);
        auto replacementId = NodeReplacementSystem::GenerateReplacementId(testMethodNode);
        EXPECT_EQ(replacementId, ValidFreeMethodNodeKey1);

        testMethodEntity->Reset();
        delete testMethodEntity;
        testMethodEntity = nullptr;
        testMethodNode = nullptr;
    }

    TEST_F(ScriptCanvasEditorUnitTest, GenerateReplacementId_GetExpectedKey_WhileGivingCustomNode)
    {
        OldTestCustomNode* testCustomNode = aznew OldTestCustomNode();
        auto replacementId = NodeReplacementSystem::GenerateReplacementId(testCustomNode);
        EXPECT_EQ(replacementId, ValidOldCustomNodeKey);

        delete testCustomNode;
        testCustomNode = nullptr;
    }

    TEST_F(ScriptCanvasEditorUnitTest, GenerateReplacementId_GetEmptyKey_WhileGivingNullPointer)
    {
        auto replacementId = NodeReplacementSystem::GenerateReplacementId(nullptr);
        EXPECT_EQ(replacementId, "");
    }

    TEST_F(ScriptCanvasEditorUnitTest, GetNodeReplacementConfiguration_GetValidConfig_WhileLookingForExistingMethodKey)
    {
        m_settingsRegistry->MergeSettings(ValidNodeReplacement1, AZ::SettingsRegistryInterface::Format::JsonMergePatch);
        m_settingsRegistry->MergeSettings(ValidNodeReplacement2, AZ::SettingsRegistryInterface::Format::JsonMergePatch);
        NodeReplacementSystem testSystem;
        testSystem.LoadReplacementMetadata();

        auto config = testSystem.GetNodeReplacementConfiguration(ValidClassMethodNodeKey1);
        EXPECT_TRUE(config.IsValid());
        EXPECT_EQ(config.m_className, "New Test Class1");
        EXPECT_EQ(config.m_methodName, "New Test Method1");
    }

    TEST_F(ScriptCanvasEditorUnitTest, GetNodeReplacementConfiguration_GetValidConfig_WhileLoadingMultipleNodeReplacement)
    {
        m_settingsRegistry->MergeSettings(ValidNodeReplacement1, AZ::SettingsRegistryInterface::Format::JsonMergePatch);
        m_settingsRegistry->MergeSettings(ValidNodeReplacement2, AZ::SettingsRegistryInterface::Format::JsonMergePatch);
        NodeReplacementSystem testSystem;
        testSystem.LoadReplacementMetadata();

        auto config = testSystem.GetNodeReplacementConfiguration(ValidFreeMethodNodeKey2);
        EXPECT_TRUE(config.IsValid());
        EXPECT_EQ(config.m_className, "New Test Class2");
        EXPECT_EQ(config.m_methodName, "New Test Method2");
    }

    TEST_F(ScriptCanvasEditorUnitTest, GetNodeReplacementConfiguration_GetInvalidConfig_WhileLookingForNonExistingKey)
    {
        m_settingsRegistry->MergeSettings(ValidNodeReplacement1, AZ::SettingsRegistryInterface::Format::JsonMergePatch);
        NodeReplacementSystem testSystem;
        testSystem.LoadReplacementMetadata();

        auto config = testSystem.GetNodeReplacementConfiguration("");
        EXPECT_FALSE(config.IsValid());
    }

    TEST_F(ScriptCanvasEditorUnitTest, LoadReplacementMetadata_GetValidConfig_WhileBroadcastResultAfterLoading)
    {
        m_settingsRegistry->MergeSettings(ValidNodeReplacement1, AZ::SettingsRegistryInterface::Format::JsonMergePatch);
        NodeReplacementSystem testSystem;
        testSystem.LoadReplacementMetadata();

        NodeReplacementConfiguration config;
        ScriptCanvasEditor::NodeReplacementRequestBus::BroadcastResult(
            config, &ScriptCanvasEditor::NodeReplacementRequestBus::Events::GetNodeReplacementConfiguration, ValidClassMethodNodeKey1);
        EXPECT_TRUE(config.IsValid());
        EXPECT_EQ(config.m_className, "New Test Class1");
        EXPECT_EQ(config.m_methodName, "New Test Method1");
    }

    TEST_F(ScriptCanvasEditorUnitTest, ReplaceNodeByReplacementConfiguration_GetEmptyReport_WhileGraphIdInvalid)
    {
        ON_CALL(*m_componentApplicationMock, AddEntity(::testing::_))
            .WillByDefault(
                []([[maybe_unused]] AZ::Entity*)
                {
                    return true;
                });
        NodeReplacementConfiguration config;
        OldTestCustomNode* testCustomNode = aznew OldTestCustomNode();
        AZ::Entity* testCustomEntity =  aznew AZ::Entity();
        testCustomEntity->Init();
        testCustomEntity->AddComponent(testCustomNode);
        NodeReplacementSystem testSystem;
        auto report = testSystem.ReplaceNodeByReplacementConfiguration(AZ::EntityId(), testCustomNode, config);
        EXPECT_TRUE(report.IsEmpty());

        delete testCustomNode;
        testCustomNode = nullptr;
        delete testCustomEntity;
        testCustomEntity = nullptr;
    }

    TEST_F(ScriptCanvasEditorUnitTest, ReplaceNodeByReplacementConfiguration_GetEmptyReport_WhileNodeIsNotAttachedToEntity)
    {
        NodeReplacementConfiguration config;
        OldTestCustomNode* testCustomNode = aznew OldTestCustomNode();
        NodeReplacementSystem testSystem;
        auto report = testSystem.ReplaceNodeByReplacementConfiguration(AZ::EntityId(ValidGraphId), testCustomNode, config);
        EXPECT_TRUE(report.IsEmpty());

        delete testCustomNode;
        testCustomNode = nullptr;
    }

    TEST_F(ScriptCanvasEditorUnitTest, ReplaceNodeByReplacementConfiguration_GetEmptyReport_WhileSerializeContextIsNull)
    {
        ON_CALL(*m_componentApplicationMock, AddEntity(::testing::_))
            .WillByDefault(
                []([[maybe_unused]] AZ::Entity*)
                {
                    return true;
                });
        ON_CALL(*m_componentApplicationMock, GetSerializeContext())
            .WillByDefault(
                []()
                {
                    return nullptr;
                });
        NodeReplacementConfiguration config;
        OldTestCustomNode* testCustomNode = aznew OldTestCustomNode();
        AZ::Entity* testCustomEntity = aznew AZ::Entity();
        testCustomEntity->Init();
        testCustomEntity->AddComponent(testCustomNode);

        NodeReplacementSystem testSystem;
        auto report = testSystem.ReplaceNodeByReplacementConfiguration(AZ::EntityId(ValidGraphId), testCustomNode, config);
        EXPECT_TRUE(report.IsEmpty());

        delete testCustomNode;
        testCustomNode = nullptr;
        delete testCustomEntity;
        testCustomEntity = nullptr;
    }

    TEST_F(ScriptCanvasEditorUnitTest, ReplaceNodeByReplacementConfiguration_GetEmptyReport_WhileSerializeContextHasNoReplacementNode)
    {
        ON_CALL(*m_componentApplicationMock, AddEntity(::testing::_))
            .WillByDefault(
                []([[maybe_unused]] AZ::Entity*)
                {
                    return true;
                });
        AZ::SerializeContext testSerializeContext;
        ON_CALL(*m_componentApplicationMock, GetSerializeContext())
            .WillByDefault(
                [&testSerializeContext]()
                {
                    return &testSerializeContext;
                });
        NodeReplacementConfiguration config;
        config.m_type = AZ::Uuid::CreateRandom();
        OldTestCustomNode* testCustomNode = aznew OldTestCustomNode();
        AZ::Entity* testCustomEntity = aznew AZ::Entity();
        testCustomEntity->Init();
        testCustomEntity->AddComponent(testCustomNode);

        NodeReplacementSystem testSystem;
        auto report = testSystem.ReplaceNodeByReplacementConfiguration(AZ::EntityId(ValidGraphId), testCustomNode, config);
        EXPECT_TRUE(report.IsEmpty());

        delete testCustomNode;
        testCustomNode = nullptr;
        delete testCustomEntity;
        testCustomEntity = nullptr;
    }

    TEST_F(ScriptCanvasEditorUnitTest, ReplaceNodeByReplacementConfiguration_GetValidReport_WhileNoDataSlotMethodTopologyMatch)
    {
        AZ::BehaviorContext testBehaviorContext;
        testBehaviorContext.Method("Old Test Free Method", [](){});
        testBehaviorContext.Method("New Test Free Method", [](){});
        ON_CALL(*m_componentApplicationMock, GetBehaviorContext())
            .WillByDefault(
                [&testBehaviorContext]()
                {
                    return &testBehaviorContext;
                });
        ON_CALL(*m_componentApplicationMock, AddEntity(::testing::_))
            .WillByDefault(
                []([[maybe_unused]] AZ::Entity*)
                {
                    return true;
                });
        AZ::SerializeContext testSerializeContext;
        Nodes::Core::Method::Reflect(&testSerializeContext);
        ON_CALL(*m_componentApplicationMock, GetSerializeContext())
            .WillByDefault(
                [&testSerializeContext]()
                {
                    return &testSerializeContext;
                });
        NodeReplacementConfiguration config;
        config.m_type = AZ::Uuid(ValidNewMethodNodeKey);
        config.m_methodName = "New Test Free Method";
        Nodes::Core::Method* testMethodNode = aznew ScriptCanvas::Nodes::Core::Method();
        AZ::Entity* testMethodEntity = aznew AZ::Entity();
        testMethodEntity->Init();
        testMethodEntity->AddComponent(testMethodNode);
        testMethodNode->InitializeBehaviorMethod({}, "", "Old Test Free Method", PropertyStatus::None);

        NodeReplacementSystem testSystem;
        AZ_TEST_START_TRACE_SUPPRESSION;
        auto report = testSystem.ReplaceNodeByReplacementConfiguration(AZ::EntityId(ValidGraphId), testMethodNode, config);
        AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;
        EXPECT_FALSE(report.IsEmpty());
        EXPECT_EQ(report.m_oldSlotsToNewSlots.size(), 2); // two execution slots

        testMethodNode = nullptr;
        testMethodEntity->Reset();
        delete testMethodEntity;
        testMethodEntity = nullptr;
        report.m_newNode = nullptr;
    }

    TEST_F(ScriptCanvasEditorUnitTest, ReplaceNodeByReplacementConfiguration_GetValidReport_WhileDataSlotMethodTopologyMatch)
    {
        AZ::BehaviorContext testBehaviorContext;
        testBehaviorContext.Method("Old Test Free Method", [](float input){ return input;});
        testBehaviorContext.Method("New Test Free Method", [](float input){ return input;});
        ON_CALL(*m_componentApplicationMock, GetBehaviorContext())
            .WillByDefault(
                [&testBehaviorContext]()
                {
                    return &testBehaviorContext;
                });
        ON_CALL(*m_componentApplicationMock, AddEntity(::testing::_))
            .WillByDefault(
                []([[maybe_unused]] AZ::Entity*)
                {
                    return true;
                });
        AZ::SerializeContext testSerializeContext;
        Nodes::Core::Method::Reflect(&testSerializeContext);
        ON_CALL(*m_componentApplicationMock, GetSerializeContext())
            .WillByDefault(
                [&testSerializeContext]()
                {
                    return &testSerializeContext;
                });
        NodeReplacementConfiguration config;
        config.m_type = AZ::Uuid(ValidNewMethodNodeKey);
        config.m_methodName = "New Test Free Method";
        Nodes::Core::Method* testMethodNode = aznew ScriptCanvas::Nodes::Core::Method();
        AZ::Entity* testMethodEntity = aznew AZ::Entity();
        testMethodEntity->Init();
        testMethodEntity->AddComponent(testMethodNode);
        testMethodNode->InitializeBehaviorMethod({}, "", "Old Test Free Method", PropertyStatus::None);

        NodeReplacementSystem testSystem;
        AZ_TEST_START_TRACE_SUPPRESSION;
        auto report = testSystem.ReplaceNodeByReplacementConfiguration(AZ::EntityId(ValidGraphId), testMethodNode, config);
        AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;
        EXPECT_FALSE(report.IsEmpty());
        EXPECT_EQ(report.m_oldSlotsToNewSlots.size(), 4); // two execution slots and two data slots

        testMethodNode = nullptr;
        testMethodEntity->Reset();
        delete testMethodEntity;
        testMethodEntity = nullptr;
        report.m_newNode = nullptr;
    }

    TEST_F(ScriptCanvasEditorUnitTest, ReplaceNodeByReplacementConfiguration_GetEmptyReport_WhileMethodTopologyNotMatch)
    {
        AZ::BehaviorContext testBehaviorContext;
        testBehaviorContext.Method("Old Test Free Method", [](AZStd::string input){ return input;});
        testBehaviorContext.Method("New Test Free Method", [](float input){ return input;});
        ON_CALL(*m_componentApplicationMock, GetBehaviorContext())
            .WillByDefault(
                [&testBehaviorContext]()
                {
                    return &testBehaviorContext;
                });
        ON_CALL(*m_componentApplicationMock, AddEntity(::testing::_))
            .WillByDefault(
                []([[maybe_unused]] AZ::Entity*)
                {
                    return true;
                });
        AZ::SerializeContext testSerializeContext;
        Nodes::Core::Method::Reflect(&testSerializeContext);
        ON_CALL(*m_componentApplicationMock, GetSerializeContext())
            .WillByDefault(
                [&testSerializeContext]()
                {
                    return &testSerializeContext;
                });
        NodeReplacementConfiguration config;
        config.m_type = AZ::Uuid(ValidNewMethodNodeKey);
        config.m_methodName = "New Test Free Method";
        Nodes::Core::Method* testMethodNode = aznew ScriptCanvas::Nodes::Core::Method();
        AZ::Entity* testMethodEntity = aznew AZ::Entity();
        testMethodEntity->Init();
        testMethodEntity->AddComponent(testMethodNode);
        testMethodNode->InitializeBehaviorMethod({}, "", "Old Test Free Method", PropertyStatus::None);

        NodeReplacementSystem testSystem;
        AZ_TEST_START_TRACE_SUPPRESSION;
        auto report = testSystem.ReplaceNodeByReplacementConfiguration(AZ::EntityId(ValidGraphId), testMethodNode, config);
        AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;
        EXPECT_TRUE(report.IsEmpty());

        testMethodEntity->Reset();
        delete testMethodEntity;
        testMethodEntity = nullptr;
        testMethodNode = nullptr;
    }

    TEST_F(ScriptCanvasEditorUnitTest, UnloadReplacementMetadata_GetInvalidConfig_WhileBroadcastResultAfterUnloading)
    {
        m_settingsRegistry->MergeSettings(ValidNodeReplacement1, AZ::SettingsRegistryInterface::Format::JsonMergePatch);
        NodeReplacementSystem testSystem;
        testSystem.LoadReplacementMetadata();
        testSystem.UnloadReplacementMetadata();

        NodeReplacementConfiguration config;
        ScriptCanvasEditor::NodeReplacementRequestBus::BroadcastResult(
            config, &ScriptCanvasEditor::NodeReplacementRequestBus::Events::GetNodeReplacementConfiguration, ValidClassMethodNodeKey1);
        EXPECT_FALSE(config.IsValid());
    }
}
