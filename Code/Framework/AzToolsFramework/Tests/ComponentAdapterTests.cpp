/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AzCore/Component/Entity.h>
#include <AzFramework/Components/ComponentAdapter.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <CustomSerializeContextTestFixture.h>

namespace UnitTest
{
    static bool s_activateCalled = false;
    static bool s_deactivateCalled = false;

    struct TestConfig
        : public AZ::ComponentConfig
    {
        AZ_RTTI(TestConfig, "{835CF711-77DB-4DF2-A364-936227A7AF5F}", AZ::ComponentConfig);
        AZ_CLASS_ALLOCATOR(TestConfig, AZ::SystemAllocator)

        uint32_t m_testValue = 0;
    };

    class TestController
    {
    public:
        AZ_TYPE_INFO(TestController, "{89C1FED9-C306-4B00-9EA4-577862D9277D}");

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ_UNUSED(context);
        }

        TestController() = default;
        TestController(const TestConfig& config):
            m_config(config)
        {
        }
        void Activate(AZ::EntityId entityId)
        {
            AZ_UNUSED(entityId);
            s_activateCalled = true;
        }
        void Deactivate()
        {
            s_deactivateCalled = true;
        }
        void SetConfiguration(const TestConfig& config)
        {
            m_config = config;
        }
        const TestConfig& GetConfiguration() const
        {
            return m_config;
        }
        TestConfig m_config;
    };

    class TestRuntimeComponent
        : public AzFramework::Components::ComponentAdapter<TestController, TestConfig>
    {
    public:
        using BaseClass = AzFramework::Components::ComponentAdapter<TestController, TestConfig>;
        AZ_COMPONENT(TestRuntimeComponent, "{136104E4-36A6-4778-AE65-065D33F87E76}", BaseClass);
        TestRuntimeComponent() = default;
        TestRuntimeComponent(const TestConfig& config)
            : BaseClass(config)
        {
        }
    };

    class TestEditorComponent
        : public AzToolsFramework::Components::EditorComponentAdapter<TestController, TestRuntimeComponent, TestConfig>
    {
    public:
        using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<TestController, TestRuntimeComponent, TestConfig>;
        AZ_EDITOR_COMPONENT(TestEditorComponent, "{5FA2B1D6-E2DA-47FB-8419-B6425C37AC80}", BaseClass);
        TestEditorComponent() = default;
        TestEditorComponent(const TestConfig& config)
            : BaseClass(config)
        {
        }
    };

    class WrappedComponentTest
        : public CustomSerializeContextTestFixture
    {

        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_testRuntimeComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_testEditorComponentDescriptor;

    public:
        void SetUp() override
        {
            CustomSerializeContextTestFixture::SetUp();

            s_activateCalled = false;
            s_deactivateCalled = false;

            m_testRuntimeComponentDescriptor.reset(TestRuntimeComponent::CreateDescriptor());
            m_testRuntimeComponentDescriptor->Reflect(&(*m_serializeContext));

            m_testEditorComponentDescriptor.reset(TestEditorComponent::CreateDescriptor());
            m_testEditorComponentDescriptor->Reflect(&(*m_serializeContext));
        }

        void TearDown() override
        {
            m_testEditorComponentDescriptor.reset();
            m_testRuntimeComponentDescriptor.reset();

            CustomSerializeContextTestFixture::TearDown();
        }
    };

    TEST_F(WrappedComponentTest, RuntimeWrappersWrapCommon)
    {
        AZ::Entity entity;
        TestRuntimeComponent* runtimeComponent = entity.CreateComponent<TestRuntimeComponent>();

        entity.Init();
        entity.Activate();
        EXPECT_TRUE(s_activateCalled);
        entity.Deactivate();
        EXPECT_TRUE(s_deactivateCalled);

        TestConfig config;
        config.m_testValue = 100;

        EXPECT_TRUE(runtimeComponent->SetConfiguration(config));

        TestConfig outConfig;
        EXPECT_TRUE(runtimeComponent->GetConfiguration(outConfig));

        EXPECT_EQ(config.m_testValue, outConfig.m_testValue);
    }

    TEST_F(WrappedComponentTest, EditorWrappersWrapCommon)
    {
        AZ::Entity entity;
        TestEditorComponent* editorComponent = entity.CreateComponent<TestEditorComponent>();

        entity.Init();
        entity.Activate();
        EXPECT_TRUE(s_activateCalled);
        entity.Deactivate();
        EXPECT_TRUE(s_deactivateCalled);

        TestConfig config;
        config.m_testValue = 100;

        EXPECT_TRUE(editorComponent->SetConfiguration(config));

        TestConfig outConfig;
        EXPECT_TRUE(editorComponent->GetConfiguration(outConfig));

        EXPECT_EQ(config.m_testValue, outConfig.m_testValue);

        AZ::Entity gameEntity;
        editorComponent->BuildGameEntity(&gameEntity);
        TestRuntimeComponent* testRuntimeComponent = gameEntity.FindComponent<TestRuntimeComponent>();

        EXPECT_NE(testRuntimeComponent, nullptr);

    }
}
