/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/any.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Device/DeviceAttributesSystemComponent.h>

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

    class TestDeviceAttribute
        : public AzFramework::DeviceAttribute
    {
    public:
        using EvalFunc = AZStd::function<bool(AZStd::string_view)>;

        TestDeviceAttribute(AZStd::string_view name, AZStd::string_view description,
            AZStd::any value, EvalFunc eval)
            : m_name(name)
            , m_description(description)
            , m_eval(eval)
            , m_value(value)
        {

        }
        ~TestDeviceAttribute() = default;

        AZStd::string_view GetName() const override
        {
            return m_name;
        }

        AZStd::string_view GetDescription() const override
        {
            return m_description;
        }

        bool Evaluate(AZStd::string_view rule) const override
        {
            return m_eval(rule);
        }

        AZStd::any GetValue() const override
        {
            return m_value; 
        }

    private:
        AZStd::string m_name;
        AZStd::string m_description;
        EvalFunc m_eval;
        AZStd::any m_value;
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
        auto deviceAttribute = AZStd::make_unique<TestDeviceAttribute>(name, description, value, eval);

        // when the device attribute registrar exists
        auto registrar = AzFramework::DeviceAttributeRegistrar::Get();
        ASSERT_NE(registrar, nullptr);

        // expect a device attribute can be registered
        EXPECT_TRUE(registrar->RegisterDeviceAttribute(AZStd::move(deviceAttribute)));

        // expect the registered attribute can be found
        auto foundAttribute = registrar->FindDeviceAttribute(name);
        EXPECT_NE(foundAttribute, nullptr);
        auto foundValue = foundAttribute->GetValue();
        EXPECT_EQ(foundValue.type(), azrtti_typeid<AZStd::string>());
        EXPECT_EQ(AZStd::any_cast<AZStd::string>(foundValue), valueString);
        EXPECT_EQ(foundAttribute->GetDescription(), description);
        EXPECT_EQ(foundAttribute->GetName(), name);
        EXPECT_TRUE(foundAttribute->Evaluate("True"));
        EXPECT_FALSE(foundAttribute->Evaluate("Not true"));

        // when another device attribute with the same attribute name is registered
        auto deviceAttributeDuplicate = AZStd::make_unique<TestDeviceAttribute>(name, description, value, eval);

        // expect failure because attribute names must be unique
        EXPECT_FALSE(registrar->RegisterDeviceAttribute(AZStd::move(deviceAttributeDuplicate)));
    }

    TEST_F(DeviceAttributesSystemComponentTestFixture, DeviceAttributesSystem_Registers_Common_Device_Attributes)
    {
        // when the device attributes system component is activated

        // expect a device attribute registrar exists
        auto registrar = AzFramework::DeviceAttributeRegistrar::Get();
        ASSERT_NE(registrar, nullptr);

        // expect common attributes are registered
        auto deviceModel = registrar->FindDeviceAttribute("DeviceModel");
        EXPECT_NE(deviceModel, nullptr);
        auto deviceModelValue = deviceModel->GetValue();
        EXPECT_EQ(deviceModelValue.type(), azrtti_typeid<AZStd::string>());

        auto ram = registrar->FindDeviceAttribute("RAM");
        EXPECT_NE(ram, nullptr);
        auto ramValue = ram->GetValue();
        EXPECT_EQ(ramValue.type(), azrtti_typeid<float>());
        float ramValueFloat = AZStd::any_cast<float>(ramValue);
        EXPECT_GT(ramValueFloat, 0);
    }

} // namespace UnitTest

