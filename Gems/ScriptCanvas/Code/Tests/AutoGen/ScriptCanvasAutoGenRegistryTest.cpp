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
    class MockNode
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(MockNode, "{79A83E8A-0FFD-4CED-96E0-ADED256E6D8C}");

        ~MockNode() override = default;

        static void Reflect(AZ::ReflectContext*) {}

        // AZ::Component...
        void Activate() override { }
        void Deactivate() override { }
    };

    TEST_F(ScriptCanvasUnitTestFixture, GetDescriptors_ExpectItExists)
    {
        auto* descriptor = MockNode::CreateDescriptor();
        ScriptCanvasModel::Instance().RegisterReflection("MockNode", [](AZ::ReflectContext* context) { MockNode::Reflect(context); }, descriptor);


        auto& descriptors = ScriptCanvasModel::Instance().GetDescriptors();
        EXPECT_TRUE(!descriptors.empty());

        ScriptCanvasModel::Instance().RemoveDescriptor(descriptor);

        descriptor->ReleaseDescriptor();
    }

} // namespace ScriptCanvasUnitTest

