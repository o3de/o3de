/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/Component.h>
#include <AzCore/RTTI/ReflectContext.h>

#include <ScriptCanvas/AutoGen/ScriptCanvasAutoGenRegistry.h>
#include <Tests/Framework/ScriptCanvasUnitTestFixture.h>
#include <AzCore/Component/ComponentApplicationBus.h>

namespace AZ
{
    class ComponentDescriptor;
}

namespace ScriptCanvasUnitTest
{

    class MockRegistry
        : public ScriptCanvasModel
    {
    public:
        MockRegistry() = default;
        virtual ~MockRegistry() = default;

        MOCK_METHOD0(Init, void());
        MOCK_METHOD4(Register, void(const char* gemOrModuleName, const char* typeName, const char* typeHash, IScriptCanvasNodeFactory* factory));
        MOCK_METHOD1(Reflect, void(AZ::ReflectContext*));
        MOCK_METHOD2(RegisterReflection, void(const AZStd::string& name, ScriptCanvasModel::ReflectFunction reflect));
        MOCK_METHOD1(GetDescriptor, const AZ::ComponentDescriptor*(const char* typehash));
    };

    class MockFactory : public AZ::Component
    {
    public:
        AZ_COMPONENT(MockFactory, "{79A83E8A-0FFD-4CED-96E0-ADED256E6D8C}");
        SCRIPTCANVAS_NODE_FACTORY_DECL("ScriptCanvasUnitTest", MockFactory);

        static void Reflect(AZ::ReflectContext*) {}

        // AZ::Component...
        virtual void Activate() override { }
        virtual void Deactivate() override { }
    };
    MockFactory::ScriptCanvasNodeFactory_MockFactory* s_factory_MockFactory = MockFactory::ScriptCanvasNodeFactory_MockFactory::Impl();

    TEST_F(ScriptCanvasUnitTestFixture, GetDescriptors_ExpectItExists)
    {
        MockRegistry mockRegistry;

        AZStd::string input = AZStd::string::format("%s_%s", "ScriptCanvasUnitTest", "MockFactory").c_str();
        AZStd::string hashStr = ScriptCanvasNodeHash::Encode(input);
        auto* descriptor = ScriptCanvasModel::Instance().GetDescriptor(hashStr.c_str());

        EXPECT_TRUE(descriptor != nullptr);
        EXPECT_STREQ(descriptor->GetName(), "MockFactory");

        delete descriptor;
    }

} // namespace ScriptCanvasUnitTest

