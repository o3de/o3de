/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzCore/Serialization/Json/JsonSerializationMetadata.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/typetraits/is_const.h>
#include <AzCore/std/typetraits/remove_pointer.h>
#include <Tests/Serialization/Json/BaseJsonSerializerFixture.h>

namespace JsonSerializationTests
{
    class JsonSerializationMetadataTests
        : public BaseJsonSerializerFixture
    {
    public:
        void SetUp() override
        {
            BaseJsonSerializerFixture::SetUp();
            m_metadata = AZStd::make_unique<AZ::JsonSerializationMetadata>();
        }

        void TearDown() override
        {
            m_metadata.reset();
            BaseJsonSerializerFixture::TearDown();
        }

    protected:
        AZStd::unique_ptr<AZ::JsonSerializationMetadata> m_metadata;
    };

    struct TestSettingsA
    {
        AZ_TYPE_INFO(TestSettingsA, "{7A4A4D3E-6ACC-4F9F-8AB3-BAE44CE9E1EA}");

        int32_t m_number{ 0 };
        bool m_moved{ false };

        TestSettingsA() = default;
        explicit TestSettingsA(int32_t number)
            : m_number(number)
        {}
        TestSettingsA(const TestSettingsA&) = default;
        TestSettingsA(TestSettingsA&& rhs)
            : m_number(rhs.m_number)
        {
            rhs.m_moved = true;
        }

        TestSettingsA& operator=(const TestSettingsA&) = default;
        TestSettingsA& operator=(TestSettingsA&& rhs)
        {
            m_number = rhs.m_number;
            rhs.m_moved = true;
            return *this;
        }
    };

    struct TestSettingsB
    {
        AZ_TYPE_INFO(TestSettingsB, "{C65E748D-CFAA-4E5A-A764-D2030FEBE823}");
        AZStd::string m_message;

        explicit TestSettingsB(const AZStd::string& message)
            : m_message(message)
        {}
    };

    TEST_F(JsonSerializationMetadataTests, Add_MoveNewValue_ReturnsTrue)
    {
        TestSettingsA value(42);
        EXPECT_TRUE(m_metadata->Add(AZStd::move(value)));
        EXPECT_TRUE(value.m_moved);
    }

    TEST_F(JsonSerializationMetadataTests, Add_MoveMultipleNewValue_ReturnsTrue)
    {
        m_metadata->Add(TestSettingsB{ "hello" });

        TestSettingsA value(42);
        EXPECT_TRUE(m_metadata->Add(AZStd::move(value)));
        EXPECT_TRUE(value.m_moved);
    }

    TEST_F(JsonSerializationMetadataTests, Add_MoveDuplicateValue_ReturnsFalse)
    {
        m_metadata->Add(TestSettingsA{ 42 });
        EXPECT_FALSE(m_metadata->Add(TestSettingsA{ 88 }));
    }

    TEST_F(JsonSerializationMetadataTests, Add_CopyNewValue_ReturnsTrue)
    {
        TestSettingsA value(42);
        EXPECT_TRUE(m_metadata->Add(value));
    }

    TEST_F(JsonSerializationMetadataTests, Find_PreviouslyAddedValue_ReturnsValue)
    {
        m_metadata->Add(TestSettingsA{ 42 });
        TestSettingsA* value = m_metadata->Find<TestSettingsA>();
        ASSERT_NE(nullptr, value);
        EXPECT_EQ(42, value->m_number);
    }

    TEST_F(JsonSerializationMetadataTests, Find_ValueThatHasNotBeenAdded_ReturnsNullptr)
    {
        TestSettingsA* value = m_metadata->Find<TestSettingsA>();
        EXPECT_EQ(nullptr, value);
    }

    TEST_F(JsonSerializationMetadataTests, Find_MultipleValues_ReturnsFirstValue)
    {
        m_metadata->Add(TestSettingsA{ 42 });
        m_metadata->Add(TestSettingsA{ 88 });
        TestSettingsA* value = m_metadata->Find<TestSettingsA>();
        ASSERT_NE(nullptr, value);
        EXPECT_EQ(42, value->m_number);
    }

    TEST_F(JsonSerializationMetadataTests, FindConst_PreviouslyAddedValue_ReturnsConstPointer)
    {
        m_metadata->Add(TestSettingsA{ 42 });

        const AZ::JsonSerializationMetadata* constMetadata = m_metadata.get();

        auto value = constMetadata->Find<TestSettingsA>();
        static_assert(AZStd::is_const_v<AZStd::remove_pointer_t<decltype(value)>>,
            "The const overload of AZ::JsonSerializationMetadataTests::Find is expected to return a pointer to const value.");
        ASSERT_NE(nullptr, value);
        EXPECT_EQ(42, value->m_number);
    }
} // namespace JsonSerializationTests
