/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Input/Devices/InputDeviceId.h>

#include <ScriptCanvas/Data/DataRegistry.h>
#include <ScriptCanvas/Data/DataTypeUtils.h>
#include <ScriptCanvas/SystemComponent.h>
#include <Tests/Framework/ScriptCanvasUnitTestFixture.h>

namespace ScriptCanvasUnitTest
{
    namespace
    {
        class TestSystemComponent
            : public ScriptCanvas::SystemComponent
        {
        public:
            using ScriptCanvas::SystemComponent::GetCreatibility;
        };
    } // namespace

    TEST_F(ScriptCanvasUnitTestFixture, InputDeviceId_IsUsableInSlotsButNotAsAVariable)
    {
        AZ::BehaviorContext behaviorContext;
        AzFramework::InputDeviceId::Reflect(&behaviorContext);

        const AZ::TypeId inputDeviceIdTypeId = azrtti_typeid<AzFramework::InputDeviceId>();
        const auto behaviorClassIterator = behaviorContext.m_typeToClassMap.find(inputDeviceIdTypeId);
        ASSERT_NE(behaviorClassIterator, behaviorContext.m_typeToClassMap.end());
        ASSERT_NE(behaviorClassIterator->second, nullptr);

        AZ::SerializeContext serializeContext;
        serializeContext.Class<AzFramework::InputDeviceId>();

        TestSystemComponent systemComponent;
        const auto [createability, typeProperties] =
            systemComponent.GetCreatibility(&serializeContext, behaviorClassIterator->second);

        EXPECT_EQ(createability, ScriptCanvas::DataRegistry::Createability::SlotOnly);
        EXPECT_TRUE(typeProperties.m_isTransient);

        ScriptCanvas::DataRegistry dataRegistry;
        dataRegistry.RegisterType(inputDeviceIdTypeId, typeProperties, createability);

        const ScriptCanvas::Data::Type inputDeviceIdType = ScriptCanvas::Data::FromAZType(inputDeviceIdTypeId);
        EXPECT_TRUE(dataRegistry.IsUseableInSlot(inputDeviceIdType));
        EXPECT_TRUE(dataRegistry.m_slottableTypes.contains(inputDeviceIdType));
        EXPECT_FALSE(dataRegistry.m_creatableTypes.contains(inputDeviceIdType));
    }
} // namespace ScriptCanvasUnitTest
