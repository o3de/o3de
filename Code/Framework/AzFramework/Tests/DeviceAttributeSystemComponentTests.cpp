/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/any.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzFramework/Device/DeviceAttributesSystemComponent.h>
#include "DeviceAttribute/TestDeviceAttribute.h" 

namespace UnitTest
{
    class DeviceAttributesSystemComponentTestFixture
        : public LeakDetectionFixture
    {
    public:
        DeviceAttributesSystemComponentTestFixture()
            : LeakDetectionFixture()
        {
        }

    protected:
        void SetUp() override
        {
            m_settingsRegistry = AZStd::make_unique<AZ::SettingsRegistryImpl>();
            AZ::SettingsRegistry::Register(m_settingsRegistry.get());

            m_deviceAttributesSystemComponent = AZStd::make_unique<AzFramework::DeviceAttributesSystemComponent>();
            m_deviceAttributesSystemComponent->Activate();
        }

        void TearDown() override
        {
            m_deviceAttributesSystemComponent->Deactivate();
            m_deviceAttributesSystemComponent.reset();


            AZ::SettingsRegistry::Unregister(m_settingsRegistry.get());
            m_settingsRegistry.reset();
        }

        AZStd::unique_ptr<AzFramework::DeviceAttributesSystemComponent> m_deviceAttributesSystemComponent;
        AZStd::unique_ptr<AZ::SettingsRegistryInterface> m_settingsRegistry;
    };


    TEST_F(DeviceAttributesSystemComponentTestFixture, DeviceAttributesSystem_Can_Register_Device_Attributes)
    {
        AZStd::string name { "TestAttribute" };
        AZStd::string description { "Description" };
        AZStd::string valueString{ "ABC123" };
        AZStd::any value{ valueString };
        TestDeviceAttribute::EvalFunc eval = [](AZStd::string_view rule)
        {
            // simple string comparison evaluator
            return rule == "True";
        };
        auto deviceAttribute = AZStd::make_shared<TestDeviceAttribute>(name, description, value, eval);

        // when the device attribute registrar exists
        auto registrar = AzFramework::DeviceAttributeRegistrar::Get();
        ASSERT_NE(nullptr, registrar);

        // expect a device attribute can be registered
        EXPECT_TRUE(registrar->RegisterDeviceAttribute(deviceAttribute));

        // expect the registered attribute can be found
        auto foundAttribute = registrar->FindDeviceAttribute(name);
        EXPECT_NE(nullptr, foundAttribute);
        auto foundValue = foundAttribute->GetValue();
        EXPECT_EQ(azrtti_typeid<AZStd::string>(), foundValue.type());
        EXPECT_EQ(valueString, AZStd::any_cast<AZStd::string>(foundValue));
        EXPECT_EQ(description, foundAttribute->GetDescription());
        EXPECT_EQ(name, foundAttribute->GetName());
        EXPECT_TRUE(foundAttribute->Evaluate("True"));
        EXPECT_FALSE(foundAttribute->Evaluate("Not true"));

        // when another device attribute with the same attribute name is registered
        auto deviceAttributeDuplicate = AZStd::make_shared<TestDeviceAttribute>(name, description, value, eval);

        // expect failure because attribute names must be unique
        EXPECT_FALSE(registrar->RegisterDeviceAttribute(deviceAttributeDuplicate));

        // expect device attribute can be removed
        EXPECT_TRUE(registrar->UnregisterDeviceAttribute(deviceAttribute->GetName()));

        // expect the alternate device attribute can be registered now
        EXPECT_TRUE(registrar->RegisterDeviceAttribute(deviceAttributeDuplicate));

        // expect alternate device attribute can be removed
        EXPECT_TRUE(registrar->UnregisterDeviceAttribute(deviceAttributeDuplicate->GetName()));

        // expect alternate attribute is not found after removal
        EXPECT_EQ(nullptr, registrar->FindDeviceAttribute(deviceAttributeDuplicate->GetName()));
    }

    TEST_F(DeviceAttributesSystemComponentTestFixture, DeviceAttributesSystem_Registers_Common_Device_Attributes)
    {
        // when the device attributes system component is activated

        // expect a device attribute registrar exists
        auto registrar = AzFramework::DeviceAttributeRegistrar::Get();
        ASSERT_NE(registrar, nullptr);

        // expect common attributes are registered
        auto deviceModel = registrar->FindDeviceAttribute("DeviceModel");
        EXPECT_NE(nullptr, deviceModel);
        auto deviceModelValue = deviceModel->GetValue();
        ASSERT_TRUE(deviceModelValue.is<AZStd::string>());
        EXPECT_FALSE(AZStd::any_cast<AZStd::string>(deviceModelValue).empty());

        auto ram = registrar->FindDeviceAttribute("RAM");
        EXPECT_NE(nullptr, ram);
        auto ramValue = ram->GetValue();
        ASSERT_TRUE(ramValue.is<float>());
        EXPECT_GT(AZStd::any_cast<float>(ramValue), 0);
    }

} // namespace UnitTest

