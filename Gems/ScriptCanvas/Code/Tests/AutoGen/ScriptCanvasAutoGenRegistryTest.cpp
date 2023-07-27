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

        MOCK_METHOD1(Reflect, void(AZ::ReflectContext*));
        MOCK_METHOD3(RegisterReflection, void(const AZStd::string& name, ScriptCanvasModel::ReflectFunction reflect, AZ::ComponentDescriptor*));
        MOCK_METHOD0(GetDescriptors, const AZStd::vector<AZ::ComponentDescriptor*>&());
    };

    class MockNode : public AZ::Component
    {
    public:
        AZ_COMPONENT(MockNode, "{79A83E8A-0FFD-4CED-96E0-ADED256E6D8C}");

        static void Reflect(AZ::ReflectContext*) {}

        // AZ::Component...
        virtual void Activate() override { }
        virtual void Deactivate() override { }
    };

    TEST_F(ScriptCanvasUnitTestFixture, GetDescriptors_ExpectItExists)
    {
        MockRegistry mockRegistry;

        auto descriptor = MockNode::CreateDescriptor();
        ScriptCanvasModel::Instance().RegisterReflection("MockNode", [](AZ::ReflectContext* context) {MockNode::Reflect(context); }, descriptor);

        auto descriptors = ScriptCanvasModel::Instance().GetDescriptors();
        EXPECT_TRUE(!descriptors.empty());

        descriptor->ReleaseDescriptor();
    }

} // namespace ScriptCanvasUnitTest

