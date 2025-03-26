/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/SerializeContextFixture.h>
#include <Tests/Serialization/Json/JsonSerializationTests.h>

namespace JsonSerializationTests
{
    enum UnscopedEnumFlags : int32_t
    {
        UnscopedFlags0 = 0,
        UnscopedFlags1,
        UnscopedFlags2,
        UnscopedFlags3
    };

    enum UnscopedEnumBitFlags : uint8_t
    {
        U8UnscopedBitFlags0 = 0,
        U8UnscopedBitFlags1,
        U8UnscopedBitFlags2,
        U8UnscopedBitFlags3
    };

    enum class ScopedEnumFlagsU8 : uint8_t
    {
        ScopedFlag0,
        ScopedFlag1,
        ScopedFlag2,
        ScopedFlag3 = 129
    };

    enum class ScopedEnumFlagsS16 : int16_t
    {
        ScopedFlag0,
        ScopedFlag1,
        ScopedFlag2 = -8,
        ScopedFlag3 = 22
    };

    enum class ScopedEnumFlagsU16 : uint16_t
    {
        ScopedFlag0,
        ScopedFlag1,
        ScopedFlag2,
        ScopedFlag3 = 32768
    };

    enum class ScopedEnumFlagsS32 : int32_t
    {
        ScopedFlag0,
        ScopedFlag1,
        ScopedFlag2 = -8,
        ScopedFlag3 = 22
    };

    enum class ScopedEnumFlagsU32 : uint32_t
    {
        ScopedFlag0,
        ScopedFlag1,
        ScopedFlag2,
        ScopedFlag3 = 2147483648
    };

    enum class ScopedEnumFlagsS64 : int64_t
    {
        ScopedFlag0,
        ScopedFlag1,
        ScopedFlag2 = -8,
        ScopedFlag3 = 22
    };

    enum class ScopedEnumFlagsU64 : uint64_t
    {
        ScopedFlag0,
        ScopedFlag1,
        ScopedFlag2,
        ScopedFlag3 = (1 < 63) + 5
    };

    enum class ScopedEnumBitFlagsS64 : int64_t
    {
        BitFlagNegative = static_cast<int64_t>(0b1000'0000'0000'0000'0000'0000'0000'0100'0000'0001'0000'0000'0000'0000'0000'0000),
        BitFlag0 = 0,
        BitFlag1 = 0b1,
        BitFlag2 = 0b10,
        BitFlag3 = 0b100,
        BitFlag4 = 0b1000,

        BitFlag1And2 = BitFlag1 | BitFlag2,
        BitFlag2And4 = BitFlag2 | BitFlag4,
    };

    enum class ScopedEnumBitFlagsU64 : uint64_t
    {
        BitFlag0 = 0,
        BitFlag1 = 0b1,
        BitFlag2 = 0b10,
        BitFlag3 = 0b100,
        BitFlag4 = 0b1000,

        BitFlag1And2 = BitFlag1 | BitFlag2,
        BitFlag2And4 = BitFlag2 | BitFlag4,
    };

    enum class ScopedEnumBitFlagsNoZero : uint64_t
    {
        BitFlag1 = 0b1,
        BitFlag2 = 0b10,
        BitFlag3 = 0b100,
        BitFlag4 = 0b1000,

        BitFlag1And2 = BitFlag1 | BitFlag2,
        BitFlag2And4 = BitFlag2 | BitFlag4,
    };
} // namespace JsonSerializationTests

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(JsonSerializationTests::UnscopedEnumFlags, "{2F8F49D7-0AEC-4F73-9DC9-0B883B86ACDB}");
    AZ_TYPE_INFO_SPECIALIZE(JsonSerializationTests::UnscopedEnumBitFlags, "{A9B115BD-A3E5-48E6-B60C-0C7FA52DC532}");
    AZ_TYPE_INFO_SPECIALIZE(JsonSerializationTests::ScopedEnumFlagsU8, "{2972770B-5738-4373-B893-E35D3008FE8F}");
    AZ_TYPE_INFO_SPECIALIZE(JsonSerializationTests::ScopedEnumFlagsS16, "{867590AB-E5BC-4092-A8A4-4652B14953B3}");
    AZ_TYPE_INFO_SPECIALIZE(JsonSerializationTests::ScopedEnumFlagsU16, "{4D30ECAB-F211-4416-9AD7-B8F1DCF7BE5C}");
    AZ_TYPE_INFO_SPECIALIZE(JsonSerializationTests::ScopedEnumFlagsS32, "{2231A213-1AC1-46E2-80FD-0C45389F3ABC}");
    AZ_TYPE_INFO_SPECIALIZE(JsonSerializationTests::ScopedEnumFlagsU32, "{0A45EFB3-D2C6-4504-ABD0-494E31257722}");
    AZ_TYPE_INFO_SPECIALIZE(JsonSerializationTests::ScopedEnumFlagsS64, "{77897A4E-D1DC-440F-B209-3A961685D4DA}");
    AZ_TYPE_INFO_SPECIALIZE(JsonSerializationTests::ScopedEnumFlagsU64, "{97A03F02-A02E-4ED3-94DA-80A825FBCEE6}");
    AZ_TYPE_INFO_SPECIALIZE(JsonSerializationTests::ScopedEnumBitFlagsS64, "{F06D6834-59FA-4BE3-A80E-EAB4D5B42CA7}");
    AZ_TYPE_INFO_SPECIALIZE(JsonSerializationTests::ScopedEnumBitFlagsU64, "{10DCE70D-1362-4726-BB9B-678473076E6E}");
    AZ_TYPE_INFO_SPECIALIZE(JsonSerializationTests::ScopedEnumBitFlagsNoZero, "{8DE2AE7B-85F6-459A-AC6C-82C2F3225C8E}");
}

namespace JsonSerializationTests
{
    template<typename EnumType>
    class TypedJsonEnumSerializationTests
        : public BaseJsonSerializerFixture
    {
    };

    using EnumTypes = ::testing::Types<
        UnscopedEnumFlags,
        UnscopedEnumBitFlags,
        ScopedEnumFlagsU8,
        ScopedEnumFlagsS16,
        ScopedEnumFlagsU16,
        ScopedEnumFlagsS32,
        ScopedEnumFlagsU32,
        ScopedEnumFlagsS64,
        ScopedEnumFlagsU64,
        ScopedEnumBitFlagsS64,
        ScopedEnumBitFlagsU64,
        ScopedEnumBitFlagsNoZero
    >;
    TYPED_TEST_SUITE(TypedJsonEnumSerializationTests, EnumTypes);

    TYPED_TEST(TypedJsonEnumSerializationTests, Load_EmptyInstanceOfArrayType_ReturnsDefault)
    {
        using namespace AZ::JsonSerializationResult;

        this->m_serializeContext->template Enum<TypeParam>();
        rapidjson::Value testValue(rapidjson::kArrayType);
        TypeParam convertedValue{};

        ResultCode resultCode = AZ::JsonSerialization::Load(&convertedValue, azrtti_typeid<TypeParam>(),
            testValue, *this->m_deserializationSettings);

        EXPECT_EQ(Outcomes::DefaultsUsed, resultCode.GetOutcome());
        EXPECT_EQ(static_cast<TypeParam>(0), convertedValue);

        this->m_serializeContext->EnableRemoveReflection();
        this->m_serializeContext->template Enum<TypeParam>();
        this->m_serializeContext->DisableRemoveReflection();
    }

    TYPED_TEST(TypedJsonEnumSerializationTests, Load_EmptyString_ReturnsDefault)
    {
        using namespace AZ::JsonSerializationResult;

        this->m_serializeContext->template Enum<TypeParam>();
        rapidjson::Value testValue(rapidjson::kStringType);
        TypeParam convertedValue{};

        ResultCode resultCode = AZ::JsonSerialization::Load(&convertedValue, azrtti_typeid<TypeParam>(),
            testValue, *this->m_deserializationSettings);

        EXPECT_EQ(Outcomes::DefaultsUsed, resultCode.GetOutcome());
        EXPECT_EQ(static_cast<TypeParam>(0), convertedValue);

        this->m_serializeContext->EnableRemoveReflection();
        this->m_serializeContext->template Enum<TypeParam>();
        this->m_serializeContext->DisableRemoveReflection();
    }

    TYPED_TEST(TypedJsonEnumSerializationTests, Store_DefaultValue_ReturnsDefault)
    {
        using namespace AZ::JsonSerializationResult;

        this->m_serializeContext->template Enum<TypeParam>();
        rapidjson::Value testValue(rapidjson::kStringType);
        TypeParam convertedValue{};
        this->m_serializationSettings->m_keepDefaults = true;

        ResultCode resultCode = AZ::JsonSerialization::Store(testValue, this->m_jsonDocument->GetAllocator(),
            &convertedValue, nullptr, azrtti_typeid<TypeParam>(), *this->m_serializationSettings);

        EXPECT_EQ(Outcomes::Success, resultCode.GetOutcome());
        EXPECT_FALSE(testValue.IsObject());
        
        this->m_serializeContext->EnableRemoveReflection();
        this->m_serializeContext->template Enum<TypeParam>();
        this->m_serializeContext->DisableRemoveReflection();
    }


    // Additional enum types

    // Load

    TEST_F(JsonSerializationTests, Load_EnumUint_ReturnsSuccess)
    {
        using namespace AZ::JsonSerializationResult;
        auto scopedReflector = [](AZ::SerializeContext* serializeContext)
        {
            serializeContext->Enum<UnscopedEnumFlags>()
                ->Value("Flags0", UnscopedEnumFlags::UnscopedFlags0)
                ->Value("Flags1", UnscopedEnumFlags::UnscopedFlags1)
                ->Value("Flags2", UnscopedEnumFlags::UnscopedFlags2)
                ->Value("Flags3", UnscopedEnumFlags::UnscopedFlags3)
                ;
        };
        UnitTest::ScopedSerializeContextReflector scopedJsonReflector(*this->m_serializeContext, { AZStd::move(scopedReflector) });

        rapidjson::Value testValue;
        testValue.SetUint64(UnscopedEnumFlags::UnscopedFlags1);

        UnscopedEnumFlags convertedValue{};
        ResultCode resultCode = AZ::JsonSerialization::Load(&convertedValue, azrtti_typeid<decltype(convertedValue)>(),
            testValue, *m_deserializationSettings);

        EXPECT_EQ(Outcomes::Success, resultCode.GetOutcome());
        EXPECT_EQ(UnscopedEnumFlags::UnscopedFlags1, convertedValue);
    }

    TEST_F(JsonSerializationTests, Load_EnumInt_ReturnsSuccess)
    {
        using namespace AZ::JsonSerializationResult;
        auto scopedReflector = [](AZ::SerializeContext* serializeContext)
        {
            serializeContext->Enum<ScopedEnumFlagsS32>()
                ->Value("Flags0", ScopedEnumFlagsS32::ScopedFlag0)
                ->Value("Flags1", ScopedEnumFlagsS32::ScopedFlag1)
                ->Value("Flags2", ScopedEnumFlagsS32::ScopedFlag2)
                ->Value("Flags3", ScopedEnumFlagsS32::ScopedFlag3)
                ;
        };
        UnitTest::ScopedSerializeContextReflector scopedJsonReflector(*this->m_serializeContext, { AZStd::move(scopedReflector) });

        rapidjson::Value testValue;
        testValue.SetInt64(aznumeric_cast<int64_t>(ScopedEnumFlagsS32::ScopedFlag2));

        ScopedEnumFlagsS32 convertedValue{};
        ResultCode resultCode = AZ::JsonSerialization::Load(&convertedValue, azrtti_typeid<decltype(convertedValue)>(),
            testValue, *m_deserializationSettings);

        EXPECT_EQ(Outcomes::Success, resultCode.GetOutcome());
        EXPECT_EQ(ScopedEnumFlagsS32::ScopedFlag2, convertedValue);
    }

    TEST_F(JsonSerializationTests, Load_EnumIntOutOfRange_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = UnscopedEnumBitFlags;
        auto scopedReflector = [](AZ::SerializeContext* serializeContext)
        {
            serializeContext->Enum<EnumType>()
                ->Value("Flags0", EnumType::U8UnscopedBitFlags0)
                ->Value("Flags1", EnumType::U8UnscopedBitFlags1)
                ->Value("Flags2", EnumType::U8UnscopedBitFlags2)
                ->Value("Flags3", EnumType::U8UnscopedBitFlags3)
                ;
        };
        UnitTest::ScopedSerializeContextReflector scopedJsonReflector(*this->m_serializeContext, { AZStd::move(scopedReflector) });

        rapidjson::Value testValue;
        testValue.SetInt64(256);

        EnumType convertedValue{};
        ResultCode resultCode = AZ::JsonSerialization::Load(&convertedValue, azrtti_typeid<decltype(convertedValue)>(),
            testValue, *m_deserializationSettings);

        EXPECT_EQ(Outcomes::Unsupported, resultCode.GetOutcome());
        EXPECT_EQ(static_cast<EnumType>(0), convertedValue);
    }

    TEST_F(JsonSerializationTests, Load_EnumIntInRangeButNotEnumOption_ReturnsSuccess)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = UnscopedEnumBitFlags;
        auto scopedReflector = [](AZ::SerializeContext* serializeContext)
        {
            serializeContext->Enum<EnumType>()
                ->Value("Flags0", EnumType::U8UnscopedBitFlags0)
                ->Value("Flags1", EnumType::U8UnscopedBitFlags1)
                ->Value("Flags2", EnumType::U8UnscopedBitFlags2)
                ->Value("Flags3", EnumType::U8UnscopedBitFlags3)
                ;
        };
        UnitTest::ScopedSerializeContextReflector scopedJsonReflector(*this->m_serializeContext, { AZStd::move(scopedReflector) });

        rapidjson::Value testValue;
        testValue.SetInt64(255);

        EnumType convertedValue{};
        ResultCode resultCode = AZ::JsonSerialization::Load(&convertedValue, azrtti_typeid<decltype(convertedValue)>(),
            testValue, *m_deserializationSettings);

        EXPECT_EQ(Outcomes::Success, resultCode.GetOutcome());
        EXPECT_EQ(static_cast<EnumType>(255), convertedValue);
    }

    TEST_F(JsonSerializationTests, Load_EnumInvalidInt_ReportsAvailableOptions)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumFlagsU8;
        auto scopedReflector = [](AZ::SerializeContext* serializeContext)
        {
            serializeContext->Enum<EnumType>()
                ->Value("Flags0", EnumType::ScopedFlag0)
                ->Value("Flags1", EnumType::ScopedFlag1)
                ->Value("Flags2", EnumType::ScopedFlag2)
                ->Value("Flags3", EnumType::ScopedFlag3)
                ;
        };
        UnitTest::ScopedSerializeContextReflector scopedJsonReflector(*this->m_serializeContext, { AZStd::move(scopedReflector) });

        rapidjson::Value testValue;
        testValue.SetInt64(256);

        bool isFirstError = true; // The first error message is the cast reporting a failure.
        bool issueReported = false;
        auto reportingCallback = [&issueReported, &isFirstError](AZStd::string_view message,
            AZ::JsonSerializationResult::ResultCode result, AZStd::string_view) -> AZ::JsonSerializationResult::ResultCode
        {
            if (isFirstError)
            {
                isFirstError = false;
                return result;
            }

            if (result.GetOutcome() != AZ::JsonSerializationResult::Outcomes::Success)
            {
                EXPECT_NE(AZStd::string_view::npos, message.find("Flags0"));
                EXPECT_NE(AZStd::string_view::npos, message.find("Flags1"));
                EXPECT_NE(AZStd::string_view::npos, message.find("Flags2"));
                EXPECT_NE(AZStd::string_view::npos, message.find("Flags3"));

                EXPECT_NE(AZStd::string_view::npos, message.find(AZStd::to_string(static_cast<uint32_t>(EnumType::ScopedFlag0))));
                EXPECT_NE(AZStd::string_view::npos, message.find(AZStd::to_string(static_cast<uint32_t>(EnumType::ScopedFlag1))));
                EXPECT_NE(AZStd::string_view::npos, message.find(AZStd::to_string(static_cast<uint32_t>(EnumType::ScopedFlag2))));
                EXPECT_NE(AZStd::string_view::npos, message.find(AZStd::to_string(static_cast<uint32_t>(EnumType::ScopedFlag3))));

                issueReported = true;
            }
            return result;
        };
        m_deserializationSettings->m_reporting = reportingCallback;
        ResetJsonContexts();

        EnumType convertedValue{};
        AZ::JsonSerialization::Load(&convertedValue, azrtti_typeid<decltype(convertedValue)>(),
            testValue, *m_deserializationSettings);

        EXPECT_TRUE(issueReported);
    }

    TEST_F(JsonSerializationTests, Load_EnumInvalidString_ReturnsFailure)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumFlagsU8;
        auto scopedReflector = [](AZ::SerializeContext* serializeContext)
        {
            serializeContext->Enum<EnumType>()
                ->Value("Flags0", EnumType::ScopedFlag0)
                ->Value("Flags1", EnumType::ScopedFlag1)
                ->Value("Flags2", EnumType::ScopedFlag2)
                ->Value("Flags3", EnumType::ScopedFlag3)
                ;
        };
        UnitTest::ScopedSerializeContextReflector scopedJsonReflector(*this->m_serializeContext, { AZStd::move(scopedReflector) });

        rapidjson::Value testValue("Johannesburg");

        EnumType convertedValue{};
        ResultCode resultCode = AZ::JsonSerialization::Load(&convertedValue, azrtti_typeid<decltype(convertedValue)>(),
            testValue, *m_deserializationSettings);

        EXPECT_EQ(Outcomes::Unsupported, resultCode.GetOutcome());
        EXPECT_EQ(static_cast<EnumType>(0), convertedValue);
    }

    TEST_F(JsonSerializationTests, Load_EnumInvalidString_ReportsAvailableOptions)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumFlagsU8;
        auto scopedReflector = [](AZ::SerializeContext* serializeContext)
        {
            serializeContext->Enum<EnumType>()
                ->Value("Flags0", EnumType::ScopedFlag0)
                ->Value("Flags1", EnumType::ScopedFlag1)
                ->Value("Flags2", EnumType::ScopedFlag2)
                ->Value("Flags3", EnumType::ScopedFlag3)
                ;
        };
        UnitTest::ScopedSerializeContextReflector scopedJsonReflector(*this->m_serializeContext, { AZStd::move(scopedReflector) });

        rapidjson::Value testValue("Johannesburg");

        bool issueReported = false;
        auto reportingCallback = [&issueReported](AZStd::string_view message,
            AZ::JsonSerializationResult::ResultCode result, AZStd::string_view) -> AZ::JsonSerializationResult::ResultCode
        {
            if (result.GetOutcome() != AZ::JsonSerializationResult::Outcomes::Success)
            {
                EXPECT_NE(AZStd::string_view::npos, message.find("Flags0"));
                EXPECT_NE(AZStd::string_view::npos, message.find("Flags1"));
                EXPECT_NE(AZStd::string_view::npos, message.find("Flags2"));
                EXPECT_NE(AZStd::string_view::npos, message.find("Flags3"));

                EXPECT_NE(AZStd::string_view::npos, message.find(AZStd::to_string(static_cast<uint32_t>(EnumType::ScopedFlag0))));
                EXPECT_NE(AZStd::string_view::npos, message.find(AZStd::to_string(static_cast<uint32_t>(EnumType::ScopedFlag1))));
                EXPECT_NE(AZStd::string_view::npos, message.find(AZStd::to_string(static_cast<uint32_t>(EnumType::ScopedFlag2))));
                EXPECT_NE(AZStd::string_view::npos, message.find(AZStd::to_string(static_cast<uint32_t>(EnumType::ScopedFlag3))));

                issueReported = true;
            }
            return result;
        };
        m_deserializationSettings->m_reporting = reportingCallback;
        ResetJsonContexts();

        EnumType convertedValue{};
        AZ::JsonSerialization::Load(&convertedValue, azrtti_typeid<decltype(convertedValue)>(),
            testValue, *m_deserializationSettings);

        EXPECT_TRUE(issueReported);
    }

    TEST_F(JsonSerializationTests, Load_EnumStringWithEnumOption_ReturnsSuccess)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumFlagsS16;
        auto scopedReflector = [](AZ::SerializeContext* serializeContext)
        {
            serializeContext->Enum<EnumType>()
                ->Value("Flags0", EnumType::ScopedFlag0)
                ->Value("Flags1", EnumType::ScopedFlag1)
                ->Value("Flags2", EnumType::ScopedFlag2)
                ->Value("Flags3", EnumType::ScopedFlag3)
                ;
        };
        UnitTest::ScopedSerializeContextReflector scopedJsonReflector(*this->m_serializeContext, { AZStd::move(scopedReflector) });

        rapidjson::Value testValue("Flags3");

        EnumType convertedValue{};
        ResultCode resultCode = AZ::JsonSerialization::Load(&convertedValue, azrtti_typeid<decltype(convertedValue)>(),
            testValue, *m_deserializationSettings);

        EXPECT_EQ(Outcomes::Success, resultCode.GetOutcome());
        EXPECT_EQ(EnumType::ScopedFlag3, convertedValue);
    }

    TEST_F(JsonSerializationTests, Load_EnumStringWithInt_ReturnsSuccess)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumFlagsU16;
        auto scopedReflector = [](AZ::SerializeContext* serializeContext)
        {
            serializeContext->Enum<EnumType>()
                ->Value("Flags0", EnumType::ScopedFlag0)
                ->Value("Flags1", EnumType::ScopedFlag1)
                ->Value("Flags2", EnumType::ScopedFlag2)
                ->Value("Flags3", EnumType::ScopedFlag3)
                ;
        };
        UnitTest::ScopedSerializeContextReflector scopedJsonReflector(*this->m_serializeContext, { AZStd::move(scopedReflector) });

        rapidjson::Value testValue("2");

        EnumType convertedValue{};
        ResultCode resultCode = AZ::JsonSerialization::Load(&convertedValue, azrtti_typeid<decltype(convertedValue)>(),
            testValue, *m_deserializationSettings);

        EXPECT_EQ(Outcomes::Success, resultCode.GetOutcome());
        EXPECT_EQ(EnumType::ScopedFlag2, convertedValue);
    }

    TEST_F(JsonSerializationTests, Load_EnumStringWithIntOutOfRange_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumFlagsU16;
        auto scopedReflector = [](AZ::SerializeContext* serializeContext)
        {
            serializeContext->Enum<EnumType>()
                ->Value("Flags0", EnumType::ScopedFlag0)
                ->Value("Flags1", EnumType::ScopedFlag1)
                ->Value("Flags2", EnumType::ScopedFlag2)
                ->Value("Flags3", EnumType::ScopedFlag3)
                ;
        };
        UnitTest::ScopedSerializeContextReflector scopedJsonReflector(*this->m_serializeContext, { AZStd::move(scopedReflector) });

        rapidjson::Value testValue("-1");

        EnumType convertedValue{};
        ResultCode resultCode = AZ::JsonSerialization::Load(&convertedValue, azrtti_typeid<decltype(convertedValue)>(),
            testValue, *m_deserializationSettings);

        EXPECT_EQ(Outcomes::Unsupported, resultCode.GetOutcome());
        EXPECT_EQ(static_cast<EnumType>(0), convertedValue);
    }

    TEST_F(JsonSerializationTests, Load_EnumArrayWithSingleStringEnumOption_ReturnsSuccess)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumFlagsS32;
        auto scopedReflector = [](AZ::SerializeContext* serializeContext)
        {
            serializeContext->Enum<EnumType>()
                ->Value("Flags0", EnumType::ScopedFlag0)
                ->Value("Flags1", EnumType::ScopedFlag1)
                ->Value("Flags2", EnumType::ScopedFlag2)
                ->Value("Flags3", EnumType::ScopedFlag3)
                ;
        };
        UnitTest::ScopedSerializeContextReflector scopedJsonReflector(*this->m_serializeContext, { AZStd::move(scopedReflector) });

        rapidjson::Value testValue(rapidjson::kArrayType);
        testValue.PushBack("Flags1", m_jsonDocument->GetAllocator());

        EnumType convertedValue{};
        ResultCode resultCode = AZ::JsonSerialization::Load(&convertedValue, azrtti_typeid<decltype(convertedValue)>(),
            testValue, *m_deserializationSettings);

        EXPECT_EQ(Outcomes::Success, resultCode.GetOutcome());
        EXPECT_EQ(EnumType::ScopedFlag1, convertedValue);
    }

    TEST_F(JsonSerializationTests, Load_EnumArrayWithStringIntValue_ReturnsSuccess)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumFlagsS32;
        auto scopedReflector = [](AZ::SerializeContext* serializeContext)
        {
            serializeContext->Enum<EnumType>()
                ->Value("Flags0", EnumType::ScopedFlag0)
                ->Value("Flags1", EnumType::ScopedFlag1)
                ->Value("Flags2", EnumType::ScopedFlag2)
                ->Value("Flags3", EnumType::ScopedFlag3)
                ;
        };
        UnitTest::ScopedSerializeContextReflector scopedJsonReflector(*this->m_serializeContext, { AZStd::move(scopedReflector) });

        rapidjson::Value testValue(rapidjson::kArrayType);
        testValue.PushBack("452", m_jsonDocument->GetAllocator());

        EnumType convertedValue{};
        ResultCode resultCode = AZ::JsonSerialization::Load(&convertedValue, azrtti_typeid<decltype(convertedValue)>(),
            testValue, *m_deserializationSettings);

        EXPECT_EQ(Outcomes::Success, resultCode.GetOutcome());
        EXPECT_EQ(static_cast<EnumType>(452), convertedValue);
    }

    TEST_F(JsonSerializationTests, Load_EnumArrayWithStringIntOutOfRange_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumFlagsS16;
        auto scopedReflector = [](AZ::SerializeContext* serializeContext)
        {
            serializeContext->Enum<EnumType>()
                ->Value("Flags0", EnumType::ScopedFlag0)
                ->Value("Flags1", EnumType::ScopedFlag1)
                ->Value("Flags2", EnumType::ScopedFlag2)
                ->Value("Flags3", EnumType::ScopedFlag3)
                ;
        };
        UnitTest::ScopedSerializeContextReflector scopedJsonReflector(*this->m_serializeContext, { AZStd::move(scopedReflector) });

        rapidjson::Value testValue(rapidjson::kArrayType);
        testValue.PushBack("32769", m_jsonDocument->GetAllocator());

        EnumType convertedValue{};
        ResultCode resultCode = AZ::JsonSerialization::Load(&convertedValue, azrtti_typeid<decltype(convertedValue)>(),
            testValue, *m_deserializationSettings);

        EXPECT_EQ(Outcomes::Unsupported, resultCode.GetOutcome());
        EXPECT_EQ(static_cast<EnumType>(0), convertedValue);
    }

    TEST_F(JsonSerializationTests, Load_EnumArrayWithSingleIntOption_ReturnsSuccess)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumFlagsU32;
        auto scopedReflector = [](AZ::SerializeContext* serializeContext)
        {
            serializeContext->Enum<EnumType>()
                ->Value("Flags0", EnumType::ScopedFlag0)
                ->Value("Flags1", EnumType::ScopedFlag1)
                ->Value("Flags2", EnumType::ScopedFlag2)
                ->Value("Flags3", EnumType::ScopedFlag3)
                ;
        };
        UnitTest::ScopedSerializeContextReflector scopedJsonReflector(*this->m_serializeContext, { AZStd::move(scopedReflector) });

        rapidjson::Value testValue(rapidjson::kArrayType);
        testValue.PushBack(rapidjson::Value(2), m_jsonDocument->GetAllocator());

        EnumType convertedValue{};
        ResultCode resultCode = AZ::JsonSerialization::Load(&convertedValue, azrtti_typeid<decltype(convertedValue)>(),
            testValue, *m_deserializationSettings);

        EXPECT_EQ(Outcomes::Success, resultCode.GetOutcome());
        EXPECT_EQ(EnumType::ScopedFlag2, convertedValue);
    }

    TEST_F(JsonSerializationTests, Load_EnumArrayWithIntOptionOutOfRange_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumFlagsU8;
        auto scopedReflector = [](AZ::SerializeContext* serializeContext)
        {
            serializeContext->Enum<EnumType>()
                ->Value("Flags0", EnumType::ScopedFlag0)
                ->Value("Flags1", EnumType::ScopedFlag1)
                ->Value("Flags2", EnumType::ScopedFlag2)
                ->Value("Flags3", EnumType::ScopedFlag3)
                ;
        };
        UnitTest::ScopedSerializeContextReflector scopedJsonReflector(*this->m_serializeContext, { AZStd::move(scopedReflector) });

        rapidjson::Value testValue(rapidjson::kArrayType);
        testValue.PushBack(rapidjson::Value(256), m_jsonDocument->GetAllocator());

        EnumType convertedValue{};
        ResultCode resultCode = AZ::JsonSerialization::Load(&convertedValue, azrtti_typeid<decltype(convertedValue)>(),
            testValue, *m_deserializationSettings);

        EXPECT_EQ(Outcomes::Unsupported, resultCode.GetOutcome());
        EXPECT_EQ(static_cast<EnumType>(0), convertedValue);
    }

    TEST_F(JsonSerializationTests, Load_EnumArrayWithInvalidString_ReturnsFailure)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumFlagsS64;
        auto scopedReflector = [](AZ::SerializeContext* serializeContext)
        {
            serializeContext->Enum<EnumType>()
                ->Value("Flags0", EnumType::ScopedFlag0)
                ->Value("Flags1", EnumType::ScopedFlag1)
                ->Value("Flags2", EnumType::ScopedFlag2)
                ->Value("Flags3", EnumType::ScopedFlag3)
                ;
        };
        UnitTest::ScopedSerializeContextReflector scopedJsonReflector(*this->m_serializeContext, { AZStd::move(scopedReflector) });

        rapidjson::Value testValue(rapidjson::kArrayType);
        testValue.PushBack("Cape Town", m_jsonDocument->GetAllocator());

        EnumType convertedValue{};
        ResultCode resultCode = AZ::JsonSerialization::Load(&convertedValue, azrtti_typeid<decltype(convertedValue)>(),
            testValue, *m_deserializationSettings);

        EXPECT_EQ(Outcomes::Unsupported, resultCode.GetOutcome());
        EXPECT_EQ(static_cast<EnumType>(0), convertedValue);
    }

    TEST_F(JsonSerializationTests, Load_EnumArrayWithMultipleBitFlags_ReturnsSuccess)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumBitFlagsS64;
        auto scopedReflector = [](AZ::SerializeContext* serializeContext)
        {
            serializeContext->Enum<EnumType>()
                ->Value("BitFlag0", EnumType::BitFlag0)
                ->Value("BitFlag1", EnumType::BitFlag1)
                ->Value("BitFlag2", EnumType::BitFlag2)
                ->Value("BitFlag3", EnumType::BitFlag3)
                ->Value("BitFlag4", EnumType::BitFlag4)
                ->Value("BitFlag1And2", EnumType::BitFlag1And2)
                ->Value("BitFlag2And4", EnumType::BitFlag2And4)
                ;
        };
        UnitTest::ScopedSerializeContextReflector scopedJsonReflector(*this->m_serializeContext, { AZStd::move(scopedReflector) });

        rapidjson::Value testValue(rapidjson::kArrayType);
        testValue.PushBack("BitFlag1And2", m_jsonDocument->GetAllocator());
        testValue.PushBack("BitFlag2And4", m_jsonDocument->GetAllocator());
        testValue.PushBack(0b1010'0000, m_jsonDocument->GetAllocator());
        EnumType expectedResult(static_cast<EnumType>(static_cast<int64_t>(EnumType::BitFlag1And2) | static_cast<int64_t>(EnumType::BitFlag2And4) | 0b1010'0000));

        EnumType convertedValue{ EnumType::BitFlagNegative };
        ResultCode resultCode = AZ::JsonSerialization::Load(&convertedValue, azrtti_typeid<decltype(convertedValue)>(),
            testValue, *m_deserializationSettings);

        EXPECT_EQ(Outcomes::Success, resultCode.GetOutcome());
        EXPECT_EQ(expectedResult, convertedValue);
    }

    TEST_F(JsonSerializationTests, Load_UnsupportedNullType_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumBitFlagsS64;
        auto scopedReflector = [](AZ::SerializeContext* serializeContext)
        {
            serializeContext->Enum<EnumType>()
                ->Value("BitFlag0", EnumType::BitFlag0)
                ->Value("BitFlag1", EnumType::BitFlag1)
                ->Value("BitFlag2", EnumType::BitFlag2)
                ->Value("BitFlag3", EnumType::BitFlag3)
                ->Value("BitFlag4", EnumType::BitFlag4)
                ->Value("BitFlag1And2", EnumType::BitFlag1And2)
                ->Value("BitFlag2And4", EnumType::BitFlag2And4)
                ;
        };
        UnitTest::ScopedSerializeContextReflector scopedJsonReflector(*this->m_serializeContext, { AZStd::move(scopedReflector) });

        rapidjson::Value testValue(rapidjson::kNullType);
        EnumType convertedValue{};
        ResultCode resultCode = AZ::JsonSerialization::Load(&convertedValue, azrtti_typeid<decltype(convertedValue)>(),
            testValue, *m_deserializationSettings);

        EXPECT_EQ(Outcomes::Unsupported, resultCode.GetOutcome());
    }

    TEST_F(JsonSerializationTests, Load_UnsupportedTrueType_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumBitFlagsS64;
        auto scopedReflector = [](AZ::SerializeContext* serializeContext)
        {
            serializeContext->Enum<EnumType>()
                ->Value("BitFlag0", EnumType::BitFlag0)
                ->Value("BitFlag1", EnumType::BitFlag1)
                ->Value("BitFlag2", EnumType::BitFlag2)
                ->Value("BitFlag3", EnumType::BitFlag3)
                ->Value("BitFlag4", EnumType::BitFlag4)
                ->Value("BitFlag1And2", EnumType::BitFlag1And2)
                ->Value("BitFlag2And4", EnumType::BitFlag2And4)
                ;
        };
        UnitTest::ScopedSerializeContextReflector scopedJsonReflector(*this->m_serializeContext, { AZStd::move(scopedReflector) });

        rapidjson::Value testValue(rapidjson::kTrueType);
        EnumType convertedValue{};
        ResultCode resultCode = AZ::JsonSerialization::Load(&convertedValue, azrtti_typeid<decltype(convertedValue)>(),
            testValue, *m_deserializationSettings);

        EXPECT_EQ(Outcomes::Unsupported, resultCode.GetOutcome());
    }

    TEST_F(JsonSerializationTests, Load_UnsupportedFalseType_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumBitFlagsS64;
        auto scopedReflector = [](AZ::SerializeContext* serializeContext)
        {
            serializeContext->Enum<EnumType>()
                ->Value("BitFlag0", EnumType::BitFlag0)
                ->Value("BitFlag1", EnumType::BitFlag1)
                ->Value("BitFlag2", EnumType::BitFlag2)
                ->Value("BitFlag3", EnumType::BitFlag3)
                ->Value("BitFlag4", EnumType::BitFlag4)
                ->Value("BitFlag1And2", EnumType::BitFlag1And2)
                ->Value("BitFlag2And4", EnumType::BitFlag2And4)
                ;
        };
        UnitTest::ScopedSerializeContextReflector scopedJsonReflector(*this->m_serializeContext, { AZStd::move(scopedReflector) });

        rapidjson::Value testValue(rapidjson::kFalseType);
        EnumType convertedValue{};
        ResultCode resultCode = AZ::JsonSerialization::Load(&convertedValue, azrtti_typeid<decltype(convertedValue)>(),
            testValue, *m_deserializationSettings);

        EXPECT_EQ(Outcomes::Unsupported, resultCode.GetOutcome());
    }

    TEST_F(JsonSerializationTests, Load_UnsupportedObjectType_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumBitFlagsS64;
        auto scopedReflector = [](AZ::SerializeContext* serializeContext)
        {
            serializeContext->Enum<EnumType>()
                ->Value("BitFlag0", EnumType::BitFlag0)
                ->Value("BitFlag1", EnumType::BitFlag1)
                ->Value("BitFlag2", EnumType::BitFlag2)
                ->Value("BitFlag3", EnumType::BitFlag3)
                ->Value("BitFlag4", EnumType::BitFlag4)
                ->Value("BitFlag1And2", EnumType::BitFlag1And2)
                ->Value("BitFlag2And4", EnumType::BitFlag2And4)
                ;
        };
        UnitTest::ScopedSerializeContextReflector scopedJsonReflector(*this->m_serializeContext, { AZStd::move(scopedReflector) });

        rapidjson::Value testValue(rapidjson::kObjectType);
        // Add member otherwise the value will be used as a default.
        testValue.AddMember(rapidjson::StringRef("Test"), 42, m_jsonDocument->GetAllocator());
        EnumType convertedValue{};
        ResultCode resultCode = AZ::JsonSerialization::Load(&convertedValue, azrtti_typeid<decltype(convertedValue)>(),
            testValue, *m_deserializationSettings);

        EXPECT_EQ(Outcomes::Unsupported, resultCode.GetOutcome());
    }

    TEST_F(JsonSerializationTests, Load_UnsupportedObjectTypeInArray_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumBitFlagsS64;
        auto scopedReflector = [](AZ::SerializeContext* serializeContext)
        {
            serializeContext->Enum<EnumType>()
                ->Value("BitFlag0", EnumType::BitFlag0)
                ->Value("BitFlag1", EnumType::BitFlag1)
                ->Value("BitFlag2", EnumType::BitFlag2)
                ->Value("BitFlag3", EnumType::BitFlag3)
                ->Value("BitFlag4", EnumType::BitFlag4)
                ->Value("BitFlag1And2", EnumType::BitFlag1And2)
                ->Value("BitFlag2And4", EnumType::BitFlag2And4)
                ;
        };
        UnitTest::ScopedSerializeContextReflector scopedJsonReflector(*this->m_serializeContext, { AZStd::move(scopedReflector) });

        rapidjson::Value testValue(rapidjson::kArrayType);
        testValue.PushBack(rapidjson::StringRef("BitFlag0"), m_jsonDocument->GetAllocator());
        testValue.PushBack(false, m_jsonDocument->GetAllocator());
        testValue.PushBack(rapidjson::StringRef("BitFlag2"), m_jsonDocument->GetAllocator());
        EnumType convertedValue{};
        ResultCode resultCode = AZ::JsonSerialization::Load(&convertedValue, azrtti_typeid<decltype(convertedValue)>(),
            testValue, *m_deserializationSettings);

        EXPECT_EQ(Outcomes::Unsupported, resultCode.GetOutcome());
    }

    TEST_F(JsonSerializationTests, Load_EnumThatHasNotBeenRegistered_ReturnsUnknown)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumBitFlagsS64;
        
        rapidjson::Value testValue(rapidjson::kStringType);
        EnumType convertedValue{};
        ResultCode resultCode = AZ::JsonSerialization::Load(&convertedValue, azrtti_typeid<decltype(convertedValue)>(),
            testValue, *m_deserializationSettings);

        EXPECT_EQ(Outcomes::Unknown, resultCode.GetOutcome());
    }

    TEST_F(JsonSerializationTests, Load_EnumStringWithEnumOption_DoesNotSendReturnCodeUnsupportToReportingFunction)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumFlagsS16;
        auto scopedReflector = [](AZ::SerializeContext* serializeContext)
        {
            serializeContext->Enum<EnumType>()
                ->Value("Flags0", EnumType::ScopedFlag0)
                ->Value("Flags1", EnumType::ScopedFlag1)
                ->Value("Flags2", EnumType::ScopedFlag2)
                ->Value("Flags3", EnumType::ScopedFlag3)
                ;
        };
        UnitTest::ScopedSerializeContextReflector scopedJsonReflector(*this->m_serializeContext, { AZStd::move(scopedReflector) });

        rapidjson::Value testValue("Flags3");

        AZ::ScopedContextReporter reporter(*m_jsonDeserializationContext,
            [this]([[maybe_unused]] AZStd::string_view message, ResultCode resultCode, AZStd::string_view path)
        {
            if (resultCode.GetOutcome() == Outcomes::Unsupported &&  m_jsonDeserializationContext->GetPath() == path)
            {
                ADD_FAILURE() << "EnumSerializer LoadString() internal function should not report Outcomes::Unsupported for this test";
            }

            return resultCode;
        });
        EnumType convertedValue{};
        ResultCode resultCode = AZ::JsonSerialization::Load(&convertedValue, azrtti_typeid<decltype(convertedValue)>(),
            testValue, *m_deserializationSettings);

        EXPECT_EQ(Outcomes::Success, resultCode.GetOutcome());
        EXPECT_EQ(EnumType::ScopedFlag3, convertedValue);
    }


    // Store

    TEST_F(JsonSerializationTests, Store_EnumEnumWithSingleOption_ValueIsStored)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumBitFlagsU64;
        auto scopedReflector = [](AZ::SerializeContext* serializeContext)
        {
            serializeContext->Enum<EnumType>()
                ->Value("BitFlag0", EnumType::BitFlag0)
                ->Value("BitFlag1", EnumType::BitFlag1)
                ->Value("BitFlag2", EnumType::BitFlag2)
                ->Value("BitFlag3", EnumType::BitFlag3)
                ->Value("BitFlag4", EnumType::BitFlag4)
                ->Value("BitFlag1And2", EnumType::BitFlag1And2)
                ->Value("BitFlag2And4", EnumType::BitFlag2And4)
                ;
        };
        UnitTest::ScopedSerializeContextReflector scopedJsonReflector(*this->m_serializeContext, { AZStd::move(scopedReflector) });

        EnumType testValue(EnumType::BitFlag3);
        const EnumType* defaultValue = nullptr;
        rapidjson::Value outputValue;
        ResultCode resultCode = AZ::JsonSerialization::Store(outputValue, m_jsonDocument->GetAllocator(), &testValue, defaultValue,
            azrtti_typeid<decltype(testValue)>(), *m_serializationSettings);

        EXPECT_EQ(Outcomes::Success, resultCode.GetOutcome());
        ASSERT_EQ(rapidjson::kStringType, outputValue.GetType());
        EXPECT_STREQ("BitFlag3", outputValue.GetString());
    }

    TEST_F(JsonSerializationTests, Store_EnumEnumWithZeroValueAndReflectionOfZeroOption_ValueIsStoredAsString)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumBitFlagsS64;
        auto scopedReflector = [](AZ::SerializeContext* serializeContext)
        {
            serializeContext->Enum<EnumType>()
                ->Value("BitFlag0", EnumType::BitFlag0)
                ->Value("BitFlag1", EnumType::BitFlag1)
                ->Value("BitFlag2", EnumType::BitFlag2)
                ->Value("BitFlag3", EnumType::BitFlag3)
                ->Value("BitFlag4", EnumType::BitFlag4)
                ->Value("BitFlag1And2", EnumType::BitFlag1And2)
                ->Value("BitFlag2And4", EnumType::BitFlag2And4)
                ;
        };
        UnitTest::ScopedSerializeContextReflector scopedJsonReflector(*this->m_serializeContext, { AZStd::move(scopedReflector) });

        m_serializationSettings->m_keepDefaults = true;
        EnumType testValue{};
        const EnumType* defaultValue = nullptr;
        rapidjson::Value outputValue;
        ResultCode resultCode = AZ::JsonSerialization::Store(outputValue, m_jsonDocument->GetAllocator(), &testValue, defaultValue,
            azrtti_typeid<decltype(testValue)>(), *m_serializationSettings);

        EXPECT_EQ(Outcomes::Success, resultCode.GetOutcome());
        ASSERT_EQ(rapidjson::kStringType, outputValue.GetType());
        EXPECT_STREQ("BitFlag0", outputValue.GetString());
    }

    TEST_F(JsonSerializationTests, Store_EnumEnumWithZeroValueAndNoReflectionOfZeroOption_ValueIsStoredAsInt)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumBitFlagsNoZero;
        auto scopedReflector = [](AZ::SerializeContext* serializeContext)
        {
            serializeContext->Enum<EnumType>()
                ->Value("BitFlag1", EnumType::BitFlag1)
                ->Value("BitFlag2", EnumType::BitFlag2)
                ->Value("BitFlag3", EnumType::BitFlag3)
                ->Value("BitFlag4", EnumType::BitFlag4)
                ->Value("BitFlag1And2", EnumType::BitFlag1And2)
                ->Value("BitFlag2And4", EnumType::BitFlag2And4)
                ;
        };
        UnitTest::ScopedSerializeContextReflector scopedJsonReflector(*this->m_serializeContext, { AZStd::move(scopedReflector) });

        m_serializationSettings->m_keepDefaults = true;
        EnumType testValue{};
        const EnumType* defaultValue = nullptr;
        rapidjson::Value outputValue;
        ResultCode resultCode = AZ::JsonSerialization::Store(outputValue, m_jsonDocument->GetAllocator(), &testValue, defaultValue,
            azrtti_typeid<decltype(testValue)>(), *m_serializationSettings);

        EXPECT_EQ(Outcomes::Success, resultCode.GetOutcome());
        ASSERT_EQ(rapidjson::kNumberType, outputValue.GetType());
        EXPECT_EQ(0, outputValue.GetInt64());
    }

    TEST_F(JsonSerializationTests, Store_EnumWithSignedUnderlyingTypeAndNegativeBitFlag_RoundTripsValueSuccessfully)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumBitFlagsS64;
        auto scopedReflector = [](AZ::SerializeContext* serializeContext)
        {
            serializeContext->Enum<EnumType>()
                ->Value("BitFlag0", EnumType::BitFlag0)
                ->Value("BitFlag1", EnumType::BitFlag1)
                ->Value("BitFlag2", EnumType::BitFlag2)
                ->Value("BitFlag3", EnumType::BitFlag3)
                ->Value("BitFlag4", EnumType::BitFlag4)
                ->Value("BitFlag1And2", EnumType::BitFlag1And2)
                ->Value("BitFlag2And4", EnumType::BitFlag2And4)
                ;
        };
        UnitTest::ScopedSerializeContextReflector scopedJsonReflector(*this->m_serializeContext, { AZStd::move(scopedReflector) });

        EnumType testValue{ EnumType::BitFlagNegative };
        const EnumType* defaultValue = nullptr;
        rapidjson::Value outputValue;
        ResultCode resultCode = AZ::JsonSerialization::Store(outputValue, m_jsonDocument->GetAllocator(), &testValue, defaultValue,
            azrtti_typeid<decltype(testValue)>(), *m_serializationSettings);

        EXPECT_EQ(Outcomes::Success, resultCode.GetOutcome());
        ASSERT_EQ(rapidjson::kNumberType, outputValue.GetType());
        EXPECT_EQ(EnumType::BitFlagNegative, static_cast<EnumType>(outputValue.GetInt64()));
    }

    TEST_F(JsonSerializationTests, Store_EnumWithMultipleOptionsWhichMatchesAggregateOption_StoresValueInString)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumBitFlagsU64;
        auto scopedReflector = [](AZ::SerializeContext* serializeContext)
        {
            serializeContext->Enum<EnumType>()
                ->Value("BitFlag0", EnumType::BitFlag0)
                ->Value("BitFlag1", EnumType::BitFlag1)
                ->Value("BitFlag2", EnumType::BitFlag2)
                ->Value("BitFlag3", EnumType::BitFlag3)
                ->Value("BitFlag4", EnumType::BitFlag4)
                ->Value("BitFlag1And2", EnumType::BitFlag1And2)
                ->Value("BitFlag2And4", EnumType::BitFlag2And4)
                ;
        };
        UnitTest::ScopedSerializeContextReflector scopedJsonReflector(*this->m_serializeContext, { AZStd::move(scopedReflector) });

        EnumType testValue{ static_cast<EnumType>(static_cast<uint64_t>(EnumType::BitFlag2) | static_cast<uint64_t>(EnumType::BitFlag4)) };
        const EnumType* defaultValue = nullptr;
        rapidjson::Value outputValue;
        ResultCode resultCode = AZ::JsonSerialization::Store(outputValue, m_jsonDocument->GetAllocator(), &testValue, defaultValue,
            azrtti_typeid<decltype(testValue)>(), *m_serializationSettings);

        EXPECT_EQ(Outcomes::Success, resultCode.GetOutcome());
        ASSERT_EQ(rapidjson::kStringType, outputValue.GetType());
        EXPECT_STREQ("BitFlag2And4", outputValue.GetString());
    }

    TEST_F(JsonSerializationTests, Store_EnumWithMultipleOptions_StoresAllOptionsInArray)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumBitFlagsS64;
        auto scopedReflector = [](AZ::SerializeContext* serializeContext)
        {
            serializeContext->Enum<EnumType>()
                ->Value("BitFlag0", EnumType::BitFlag0)
                ->Value("BitFlag1", EnumType::BitFlag1)
                ->Value("BitFlag2", EnumType::BitFlag2)
                ->Value("BitFlag3", EnumType::BitFlag3)
                ->Value("BitFlag4", EnumType::BitFlag4)
                ->Value("BitFlag1And2", EnumType::BitFlag1And2)
                ->Value("BitFlag2And4", EnumType::BitFlag2And4)
                ;
        };
        UnitTest::ScopedSerializeContextReflector scopedJsonReflector(*this->m_serializeContext, { AZStd::move(scopedReflector) });

        EnumType testValue{ static_cast<EnumType>(static_cast<uint64_t>(EnumType::BitFlag1) | static_cast<uint64_t>(EnumType::BitFlag3)
            | static_cast<uint64_t>(EnumType::BitFlag4)) };
        const EnumType* defaultValue = nullptr;
        rapidjson::Value outputValue;
        ResultCode resultCode = AZ::JsonSerialization::Store(outputValue, m_jsonDocument->GetAllocator(), &testValue, defaultValue,
            azrtti_typeid<decltype(testValue)>(), *m_serializationSettings);

        EXPECT_EQ(Outcomes::Success, resultCode.GetOutcome());
        ASSERT_EQ(rapidjson::kArrayType, outputValue.GetType());

        ASSERT_EQ(3, outputValue.Size());
        // All three elements are string and the elements
        // are written from largest unsigned integral value to smallest
        ASSERT_EQ(rapidjson::kStringType, outputValue[0].GetType());
        ASSERT_EQ(rapidjson::kStringType, outputValue[1].GetType());
        ASSERT_EQ(rapidjson::kStringType, outputValue[2].GetType());

        EXPECT_STREQ("BitFlag4", outputValue[0].GetString());
        EXPECT_STREQ("BitFlag3", outputValue[1].GetString());
        EXPECT_STREQ("BitFlag1", outputValue[2].GetString());
    }

    TEST_F(JsonSerializationTests, Store_EnumWithMultipleOptionsAndNonOptionBits_StoresAllValuesInArray)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumBitFlagsS64;
        auto scopedReflector = [](AZ::SerializeContext* serializeContext)
        {
            serializeContext->Enum<EnumType>()
                ->Value("BitFlag0", EnumType::BitFlag0)
                ->Value("BitFlag1", EnumType::BitFlag1)
                ->Value("BitFlag2", EnumType::BitFlag2)
                ->Value("BitFlag3", EnumType::BitFlag3)
                ->Value("BitFlag4", EnumType::BitFlag4)
                ->Value("BitFlag1And2", EnumType::BitFlag1And2)
                ->Value("BitFlag2And4", EnumType::BitFlag2And4)
                ;
        };
        UnitTest::ScopedSerializeContextReflector scopedJsonReflector(*this->m_serializeContext, { AZStd::move(scopedReflector) });

        constexpr uint64_t nonOptionBits = 0b1011'0110'0000'0000;
        EnumType testValue{ static_cast<EnumType>(static_cast<uint64_t>(EnumType::BitFlag1) | static_cast<uint64_t>(EnumType::BitFlag3)
            | static_cast<uint64_t>(EnumType::BitFlag4) | nonOptionBits) };
        const EnumType* defaultValue = nullptr;
        rapidjson::Value outputValue;
        ResultCode resultCode = AZ::JsonSerialization::Store(outputValue, m_jsonDocument->GetAllocator(), &testValue, defaultValue,
            azrtti_typeid<decltype(testValue)>(), *m_serializationSettings);

        EXPECT_EQ(Outcomes::Success, resultCode.GetOutcome());
        ASSERT_EQ(rapidjson::kArrayType, outputValue.GetType());

        ASSERT_EQ(4, outputValue.Size());
        // The first three elements should be strings
        // The final element should be an integer of the left over bit values
        // that doesn't correspond to a value
        // are written from largest unsigned integral value to smallest
        ASSERT_EQ(rapidjson::kStringType, outputValue[0].GetType());
        ASSERT_EQ(rapidjson::kStringType, outputValue[1].GetType());
        ASSERT_EQ(rapidjson::kStringType, outputValue[2].GetType());
        ASSERT_EQ(rapidjson::kNumberType, outputValue[3].GetType());

        EXPECT_STREQ("BitFlag4", outputValue[0].GetString());
        EXPECT_STREQ("BitFlag3", outputValue[1].GetString());
        EXPECT_STREQ("BitFlag1", outputValue[2].GetString());
        EXPECT_EQ(nonOptionBits, outputValue[3].GetUint64());
    }

    TEST_F(JsonSerializationTests, Store_EnumWithSignedUnderlyingTypeAndMultipleOptions_StoresValuesInOrderOfHighestUnsignedBitValueFirst)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumBitFlagsS64;
        auto scopedReflector = [](AZ::SerializeContext* serializeContext)
        {
            serializeContext->Enum<EnumType>()
                ->Value("BitFlagNegative", EnumType::BitFlagNegative)
                ->Value("BitFlag0", EnumType::BitFlag0)
                ->Value("BitFlag1", EnumType::BitFlag1)
                ->Value("BitFlag2", EnumType::BitFlag2)
                ->Value("BitFlag3", EnumType::BitFlag3)
                ->Value("BitFlag4", EnumType::BitFlag4)
                ->Value("BitFlag1And2", EnumType::BitFlag1And2)
                ->Value("BitFlag2And4", EnumType::BitFlag2And4)
                ;
        };
        UnitTest::ScopedSerializeContextReflector scopedJsonReflector(*this->m_serializeContext, { AZStd::move(scopedReflector) });

        EnumType testValue{ static_cast<EnumType>(static_cast<uint64_t>(EnumType::BitFlag1) | static_cast<uint64_t>(EnumType::BitFlag3)
            | static_cast<uint64_t>(EnumType::BitFlag4) | static_cast<uint64_t>(EnumType::BitFlagNegative)) };
        const EnumType* defaultValue = nullptr;
        rapidjson::Value outputValue;
        ResultCode resultCode = AZ::JsonSerialization::Store(outputValue, m_jsonDocument->GetAllocator(), &testValue, defaultValue,
            azrtti_typeid<decltype(testValue)>(), *m_serializationSettings);

        EXPECT_EQ(Outcomes::Success, resultCode.GetOutcome());
        ASSERT_EQ(rapidjson::kArrayType, outputValue.GetType());

        ASSERT_EQ(4, outputValue.Size());
        // All four elements should be strings
        // The first correspond element should be the element
        // with the largest unsigned bit value.
        ASSERT_EQ(rapidjson::kStringType, outputValue[0].GetType());
        ASSERT_EQ(rapidjson::kStringType, outputValue[1].GetType());
        ASSERT_EQ(rapidjson::kStringType, outputValue[2].GetType());
        ASSERT_EQ(rapidjson::kStringType, outputValue[3].GetType());

        EXPECT_STREQ("BitFlagNegative", outputValue[0].GetString());
        EXPECT_STREQ("BitFlag4", outputValue[1].GetString());
        EXPECT_STREQ("BitFlag3", outputValue[2].GetString());
        EXPECT_STREQ("BitFlag1", outputValue[3].GetString());
    }

    TEST_F(JsonSerializationTests, Store_EnumWithDefaultValueMatchingSingleValue_ReturnsDefaulted)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumBitFlagsU64;
        auto scopedReflector = [](AZ::SerializeContext* serializeContext)
        {
            serializeContext->Enum<EnumType>()
                ->Value("BitFlag0", EnumType::BitFlag0)
                ->Value("BitFlag1", EnumType::BitFlag1)
                ->Value("BitFlag2", EnumType::BitFlag2)
                ->Value("BitFlag3", EnumType::BitFlag3)
                ->Value("BitFlag4", EnumType::BitFlag4)
                ->Value("BitFlag1And2", EnumType::BitFlag1And2)
                ->Value("BitFlag2And4", EnumType::BitFlag2And4)
                ;
        };
        UnitTest::ScopedSerializeContextReflector scopedJsonReflector(*this->m_serializeContext, { AZStd::move(scopedReflector) });

        EnumType testValue{ EnumType::BitFlag1And2 };
        const EnumType* defaultValue = &testValue;
        rapidjson::Value outputValue;
        ResultCode resultCode = AZ::JsonSerialization::Store(outputValue, m_jsonDocument->GetAllocator(), &testValue, defaultValue,
            azrtti_typeid<decltype(testValue)>(), *m_serializationSettings);

        EXPECT_EQ(Outcomes::DefaultsUsed, resultCode.GetOutcome());
        Expect_ExplicitDefault(outputValue);
    }

    TEST_F(JsonSerializationTests, Store_EnumWithDefaultValueMatchingEnumOptions_ReturnsDefaulted)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumBitFlagsU64;
        auto scopedReflector = [](AZ::SerializeContext* serializeContext)
        {
            serializeContext->Enum<EnumType>()
                ->Value("BitFlag0", EnumType::BitFlag0)
                ->Value("BitFlag1", EnumType::BitFlag1)
                ->Value("BitFlag2", EnumType::BitFlag2)
                ->Value("BitFlag3", EnumType::BitFlag3)
                ->Value("BitFlag4", EnumType::BitFlag4)
                ->Value("BitFlag1And2", EnumType::BitFlag1And2)
                ->Value("BitFlag2And4", EnumType::BitFlag2And4)
                ;
        };
        UnitTest::ScopedSerializeContextReflector scopedJsonReflector(*this->m_serializeContext, { AZStd::move(scopedReflector) });

        EnumType testValue{ static_cast<EnumType>(static_cast<uint64_t>(EnumType::BitFlag1And2) | static_cast<uint64_t>(EnumType::BitFlag2And4)) };
        const EnumType* defaultValue = &testValue;
        rapidjson::Value outputValue;
        ResultCode resultCode = AZ::JsonSerialization::Store(outputValue, m_jsonDocument->GetAllocator(), &testValue, defaultValue,
            azrtti_typeid<decltype(testValue)>(), *m_serializationSettings);

        EXPECT_EQ(Outcomes::DefaultsUsed, resultCode.GetOutcome());
    }

    TEST_F(JsonSerializationTests, Store_EnumWithDefaultValuesNotMatchingEnumOptions_ReturnsDefaulted)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumBitFlagsU64;
        auto scopedReflector = [](AZ::SerializeContext* serializeContext)
        {
            serializeContext->Enum<EnumType>()
                ->Value("BitFlag0", EnumType::BitFlag0)
                ->Value("BitFlag1", EnumType::BitFlag1)
                ->Value("BitFlag2", EnumType::BitFlag2)
                ->Value("BitFlag3", EnumType::BitFlag3)
                ->Value("BitFlag4", EnumType::BitFlag4)
                ;
        };
        UnitTest::ScopedSerializeContextReflector scopedJsonReflector(*this->m_serializeContext, { AZStd::move(scopedReflector) });

        EnumType testValue{ static_cast<EnumType>(static_cast<uint64_t>(0b0100'0000)) };
        const EnumType* defaultValue = &testValue;
        rapidjson::Value outputValue;
        ResultCode resultCode = AZ::JsonSerialization::Store(outputValue, m_jsonDocument->GetAllocator(), &testValue, defaultValue,
            azrtti_typeid<decltype(testValue)>(), *m_serializationSettings);

        EXPECT_EQ(Outcomes::DefaultsUsed, resultCode.GetOutcome());
    }

    TEST_F(JsonSerializationTests, Store_EnumThatIsNotReflected_ReturnsUnknown)
    {
        using namespace AZ::JsonSerializationResult;
        using EnumType = ScopedEnumBitFlagsU64;
        
        EnumType testValue{};
        rapidjson::Value outputValue;
        ResultCode resultCode = AZ::JsonSerialization::Store(outputValue, m_jsonDocument->GetAllocator(), &testValue, nullptr,
            azrtti_typeid<decltype(testValue)>(), *m_serializationSettings);

        EXPECT_EQ(Outcomes::Unknown, resultCode.GetOutcome());
    }
} // namespace JsonSerializationTests
