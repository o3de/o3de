/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Serialization/Json/PointerJsonSerializer.h>
#include <AzCore/Serialization/PointerObject.h>
#include <Tests/Serialization/Json/BaseJsonSerializerFixture.h>
#include <Tests/Serialization/Json/JsonSerializerConformityTests.h>

namespace JsonSerializationTests
{
    class PointerTestDescription
        : public JsonSerializerConformityTestDescriptor<AZ::PointerObject>
    {
        static inline constexpr uintptr_t PointerAddress = 0x00facade;
        static inline constexpr AZ::TypeId ContrivedTestTypeId = AZ::TypeId::CreateName("facade");
    public:
        PointerTestDescription()
        {
            m_fullySetJsonString = AZStd::string::format(R"(
                {
                    "$address": %)" PRIuPTR R"(,
                    "$type": "%s"
                })", PointerAddress, ContrivedTestTypeId.ToFixedString().c_str());
        }
        AZStd::shared_ptr<AZ::BaseJsonSerializer> CreateSerializer() override
        {
            return AZStd::make_shared<AZ::PointerJsonSerializer>();
        }

        AZStd::shared_ptr<AZ::PointerObject> CreateDefaultInstance() override
        {
            return AZStd::make_shared<AZ::PointerObject>();
        }

        AZStd::shared_ptr<AZ::PointerObject> CreateFullySetInstance() override
        {
            return AZStd::make_shared<AZ::PointerObject>(AZ::PointerObject{ reinterpret_cast<void*>(PointerAddress), ContrivedTestTypeId });
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            // As JSON doesn't support hexidecimal integer values
            // The value will get output as decimal integer
            return m_fullySetJsonString;
        }

        void ConfigureFeatures(JsonSerializerConformityTestDescriptorFeatures& features) override
        {
            features.EnableJsonType(rapidjson::kObjectType);
            features.m_supportsPartialInitialization = false;
            features.m_supportsInjection = false;
            features.m_defaultIsEqualToEmpty = false;
            features.m_mandatoryFields = { "$address", "$type" };
        }

        bool AreEqual(const AZ::PointerObject& lhs, const AZ::PointerObject& rhs) override
        {
            return lhs == rhs;
        }

    private:
        AZStd::string m_fullySetJsonString;
    };

    using PointerConformityTestTypes = ::testing::Types<PointerTestDescription>;
    IF_JSON_CONFORMITY_ENABLED(INSTANTIATE_TYPED_TEST_CASE_P(Pointer, JsonSerializerConformityTests, PointerConformityTestTypes));
} // namespace JsonSerializationTests
