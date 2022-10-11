/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/ReflectContext.h>
#include <AutoGen/ScriptCanvasAutoGenRegistry.h>
#include <Tests/Framework/ScriptCanvasUnitTestFixture.h>

namespace ScriptCanvasUnitTest
{
    using ScriptCanvasAutoGenRegistry = ScriptCanvasUnitTestFixture;

    class MockRegistry
        : public ScriptCanvas::ScriptCanvasRegistry
    {
    public:
        MockRegistry() = default;
        virtual ~MockRegistry() = default;

        MOCK_METHOD1(Init, void(ScriptCanvas::NodeRegistry*));
        MOCK_METHOD1(Reflect, void(AZ::ReflectContext*));
        MOCK_METHOD0(GetComponentDescriptors, AZStd::vector<AZ::ComponentDescriptor*>());
    };

    TEST_F(ScriptCanvasAutoGenRegistry, GetInstance_Call_ExpectToBeValid)
    {
        auto autogenRegistry = ScriptCanvas::AutoGenRegistryManager::GetInstance();
        EXPECT_TRUE(autogenRegistry);
    }

    TEST_F(ScriptCanvasAutoGenRegistry, GetInstance_Call_ExpectToBeConsistent)
    {
        auto autogenRegistry1 = ScriptCanvas::AutoGenRegistryManager::GetInstance();
        auto autogenRegistry2 = ScriptCanvas::AutoGenRegistryManager::GetInstance();
        EXPECT_EQ(autogenRegistry1, autogenRegistry2);
    }

    TEST_F(ScriptCanvasAutoGenRegistry, Init_CallWithCorrectName_ExpectToBeCalledOnce)
    {
        auto autogenRegistry = ScriptCanvas::AutoGenRegistryManager::GetInstance();
        MockRegistry mockRegistry;
        autogenRegistry->RegisterRegistry("MockFunctionRegistry", &mockRegistry);

        EXPECT_CALL(mockRegistry, Init(::testing::_)).Times(1);
        ScriptCanvas::AutoGenRegistryManager::Init("Mock");

        autogenRegistry->UnregisterRegistry("MockFunctionRegistry");
    }

    TEST_F(ScriptCanvasAutoGenRegistry, Init_CallWithIncorrectName_ExpectNotToBeCalled)
    {
        auto autogenRegistry = ScriptCanvas::AutoGenRegistryManager::GetInstance();
        MockRegistry mockRegistry;
        autogenRegistry->RegisterRegistry("MockFunctionRegistry", &mockRegistry);

        EXPECT_CALL(mockRegistry, Init(::testing::_)).Times(0);
        ScriptCanvas::AutoGenRegistryManager::Init("Test");

        autogenRegistry->UnregisterRegistry("MockFunctionRegistry");
    }

    TEST_F(ScriptCanvasAutoGenRegistry, Reflect_CallWithCorrectName_ExpectToBeCalledOnce)
    {
        auto autogenRegistry = ScriptCanvas::AutoGenRegistryManager::GetInstance();
        MockRegistry mockRegistry;
        autogenRegistry->RegisterRegistry("MockFunctionRegistry", &mockRegistry);
        AZ::ReflectContext reflectContext;

        EXPECT_CALL(mockRegistry, Reflect(::testing::_)).Times(1);
        ScriptCanvas::AutoGenRegistryManager::Reflect(&reflectContext, "Mock");

        autogenRegistry->UnregisterRegistry("MockFunctionRegistry");
    }

    TEST_F(ScriptCanvasAutoGenRegistry, Reflect_CallWithIncorrectName_ExpectNotToBeCalled)
    {
        auto autogenRegistry = ScriptCanvas::AutoGenRegistryManager::GetInstance();
        MockRegistry mockRegistry;
        autogenRegistry->RegisterRegistry("MockFunctionRegistry", &mockRegistry);
        AZ::ReflectContext reflectContext;

        EXPECT_CALL(mockRegistry, Reflect(::testing::_)).Times(0);
        ScriptCanvas::AutoGenRegistryManager::Reflect(&reflectContext, "Test");

        autogenRegistry->UnregisterRegistry("MockFunctionRegistry");
    }

    TEST_F(ScriptCanvasAutoGenRegistry, GetComponentDescriptors_CallWithCorrectName_ExpectToBeCalledOnce)
    {
        auto autogenRegistry = ScriptCanvas::AutoGenRegistryManager::GetInstance();
        MockRegistry mockRegistry;
        autogenRegistry->RegisterRegistry("MockFunctionRegistry", &mockRegistry);

        EXPECT_CALL(mockRegistry, GetComponentDescriptors()).Times(1).WillOnce(testing::Return(AZStd::vector<AZ::ComponentDescriptor*>{nullptr}));
        auto actualResult = ScriptCanvas::AutoGenRegistryManager::GetComponentDescriptors("Mock");

        EXPECT_TRUE(actualResult.size() == 1);
        autogenRegistry->UnregisterRegistry("MockFunctionRegistry");
    }

    TEST_F(ScriptCanvasAutoGenRegistry, GetComponentDescriptors_CallWithIncorrectName_ExpectNotToBeCalled)
    {
        auto autogenRegistry = ScriptCanvas::AutoGenRegistryManager::GetInstance();
        MockRegistry mockRegistry;
        autogenRegistry->RegisterRegistry("MockFunctionRegistry", &mockRegistry);

        EXPECT_CALL(mockRegistry, GetComponentDescriptors()).Times(0);
        auto actualResult = ScriptCanvas::AutoGenRegistryManager::GetComponentDescriptors("Test");

        EXPECT_TRUE(actualResult.size() == 0);
        autogenRegistry->UnregisterRegistry("MockFunctionRegistry");
    }
} // namespace ScriptCanvasUnitTest
