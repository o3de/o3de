/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/containers/unordered_set.h>
#include <random>

namespace UnitTest
{
    class UuidTests
        : public UnitTest::LeakDetectionFixture
    {
        static const int numUuid = 2000;
        AZ::Uuid* m_array;
    public:
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();

            m_array = (AZ::Uuid*)azmalloc(sizeof(AZ::Uuid) * numUuid, alignof(AZ::Uuid));
        }
        void TearDown() override
        {
            azfree(m_array);

            LeakDetectionFixture::TearDown();
        }
        void run()
        {
            AZ::Uuid defId("{B5700F2E-661B-4AC0-9335-817CB4C09CCB}");

            // null
            AZ::Uuid id;
            EXPECT_TRUE(id.IsNull());

            const char idStr1[] = "{B5700F2E-661B-4AC0-9335-817CB4C09CCB}";
            const char idStr2[] = "{B5700F2E661B4AC09335817CB4C09CCB}";
            const char idStr3[] = "B5700F2E-661B-4AC0-9335-817CB4C09CCB";
            const char idStr4[] = "B5700F2E661B4AC09335817CB4C09CCB";

            // create from string
            id = AZ::Uuid::CreateString(idStr1);
            EXPECT_EQ(defId, id);
            id = AZ::Uuid::CreateString(idStr2);
            EXPECT_EQ(defId, id);
            id = AZ::Uuid::CreateString(idStr3);
            EXPECT_EQ(defId, id);
            id = AZ::Uuid::CreateString(idStr4);
            EXPECT_EQ(defId, id);

            // variant
            EXPECT_EQ(AZ::Uuid::VAR_RFC_4122, id.GetVariant());
            // version
            EXPECT_EQ(AZ::Uuid::VER_RANDOM, id.GetVersion());

            // tostring
            char buffer[39];
            id = AZ::Uuid::CreateString(idStr1);
            EXPECT_EQ(39, id.ToString(buffer, 39, true, true));
            EXPECT_EQ(0, strcmp(buffer, idStr1));
            EXPECT_EQ(35, id.ToString(buffer, 35, true, false));
            EXPECT_EQ(0, strcmp(buffer, idStr2));
            EXPECT_EQ(37, id.ToString(buffer, 37, false, true));
            EXPECT_EQ(0, strcmp(buffer, idStr3));
            EXPECT_EQ(33, id.ToString(buffer, 33, false, false));
            EXPECT_EQ(0, strcmp(buffer, idStr4));

            EXPECT_EQ(AZStd::string(idStr1), id.ToString<AZStd::string>());
            EXPECT_EQ(AZStd::string(idStr2), id.ToString<AZStd::string>(true, false));
            EXPECT_EQ(AZStd::string(idStr3), id.ToString<AZStd::string>(false, true));
            EXPECT_EQ(AZStd::string(idStr4), id.ToString<AZStd::string>(false, false));

            AZStd::string str1;
            id.ToString(str1);
            EXPECT_EQ(AZStd::string(idStr1), str1);
            id.ToString(str1, true, false);
            EXPECT_EQ(AZStd::string(idStr2), str1);
            id.ToString(str1, false, true);
            EXPECT_EQ(AZStd::string(idStr3), str1);
            id.ToString(str1, false, false);
            EXPECT_EQ(AZStd::string(idStr4), str1);

            // operators
            AZ::Uuid idBigger("C5700F2E661B4ac09335817CB4C09CCB");
            EXPECT_LT(id, idBigger);
            EXPECT_NE(idBigger, id);
            EXPECT_GT(idBigger,id);

            // hash
            AZStd::hash<AZ::Uuid> hash;
            size_t hashVal = hash(id);
            EXPECT_NE(0, hashVal);

            // test the hashing and equal function in a unordered container
            using UuidSetType = AZStd::unordered_set<AZ::Uuid>;
            UuidSetType uuidSet;
            uuidSet.insert(id);
            EXPECT_NE(uuidSet.end(), uuidSet.find(id));

            // check uniqueness (very quick and basic)
            for (int i = 0; i < numUuid; ++i)
            {
                m_array[i] = AZ::Uuid::Create();
            }

            for (int i = 0; i < numUuid; ++i)
            {
                auto uniqueToTest = AZ::Uuid::Create();
                for (int j = 0; j < numUuid; ++j)
                {
                    EXPECT_NE(uniqueToTest, m_array[j]);
                }
            }

            // test the name function
            auto uuidName = AZ::Uuid::CreateName("BlaBla");
            // check variant
            EXPECT_EQ(AZ::Uuid::VAR_RFC_4122, uuidName.GetVariant());
            // check version
            EXPECT_EQ(AZ::Uuid::VER_NAME_SHA1, uuidName.GetVersion());
            // check id
            EXPECT_EQ(AZ::Uuid::CreateName("BlaBla"), uuidName);
        }
    };

    TEST_F(UuidTests, Test)
    {
        run();
    }

    TEST_F(UuidTests, GreaterThanOrEqualTo_LeftGreaterThanRight_ReturnsTrue)
    {
        const char leftStr[] = "{F418022E-DAFE-4450-BCB9-4B7727070178}";
        const char rightStr[] = "{B5700F2E-661B-4AC0-9335-817CB4C09CCB}";
        AZ::Uuid left = AZ::Uuid::CreateString(leftStr);
        AZ::Uuid right = AZ::Uuid::CreateString(rightStr);
        EXPECT_GE(left, right);
    }

    TEST_F(UuidTests, LessThan_LeftLessThanRight_ReturnsTrue)
    {
        const char leftStr[] = "{A418022E-DAFE-4450-BCB9-4B7727070178}";
        const char rightStr[] = "{B5700F2E-661B-4AC0-9335-817CB4C09CCB}";
        AZ::Uuid left = AZ::Uuid::CreateString(leftStr);
        AZ::Uuid right = AZ::Uuid::CreateString(rightStr);
        EXPECT_LT(left, right);
    }

    TEST_F(UuidTests, GreaterThanOrEqualTo_LeftEqualsRight_ReturnsTrue)
    {
        const char uuidStr[] = "{F418022E-DAFE-4450-BCB9-4B7727070178}";
        AZ::Uuid left = AZ::Uuid::CreateString(uuidStr);
        AZ::Uuid right = AZ::Uuid::CreateString(uuidStr);
        EXPECT_GE(left, right);
    }

    TEST_F(UuidTests, GreaterThan_LeftGreaterThanRight_ReturnsTrue)
    {
        const char leftStr[] = "{F418022E-DAFE-4450-BCB9-4B7727070178}";
        const char rightStr[] = "{B5700F2E-661B-4AC0-9335-817CB4C09CCB}";
        AZ::Uuid left = AZ::Uuid::CreateString(leftStr);
        AZ::Uuid right = AZ::Uuid::CreateString(rightStr);
        EXPECT_GT(left, right);
    }

    TEST_F(UuidTests, LessThanOrEqualTo_LeftLessThanRight_ReturnsTrue)
    {
        const char leftStr[] = "{A418022E-DAFE-4450-BCB9-4B7727070178}";
        const char rightStr[] = "{B5700F2E-661B-4AC0-9335-817CB4C09CCB}";
        AZ::Uuid left = AZ::Uuid::CreateString(leftStr);
        AZ::Uuid right = AZ::Uuid::CreateString(rightStr);
        EXPECT_LE(left, right);
    }

    TEST_F(UuidTests, LessThanOrEqualTo_LeftEqualsRight_ReturnsTrue)
    {
        const char uuidStr[] = "{F418022E-DAFE-4450-BCB9-4B7727070178}";
        AZ::Uuid left = AZ::Uuid::CreateString(uuidStr);
        AZ::Uuid right = AZ::Uuid::CreateString(uuidStr);
        EXPECT_LE(left, right);
    }

    TEST_F(UuidTests, CreateStringPermissive_HexAndSpacesGiven_Success)
    {
        const char uuidStr[] = "{34D44249-E599-4B30-811F-4215C2DEA269}";
        AZ::Uuid left = AZ::Uuid::CreateString(uuidStr);

        const char permissiveStr[] = "{ 0x34D44249 - 0xE5994B30 - 0x811F4215 - 0xC2DEA269 }";
        AZ::Uuid right = AZ::Uuid::CreateStringPermissive(permissiveStr);
        EXPECT_EQ(left, right);

        const char permissiveStr2[] = "{ 0x34D44249-0xE5994B30  0x811F4215 - C2DEA269 }";
        right = AZ::Uuid::CreateStringPermissive(permissiveStr2);
        EXPECT_EQ(left, right);

        const char permissiveStr3[] = "34D44249-0xE5994B30  0x811F4215 - C2DEA269 }";
        right = AZ::Uuid::CreateStringPermissive(permissiveStr3);
        EXPECT_EQ(left, right);

        const char permissiveStr4[] = "{ x34D44249 - xE5994B30 - x811F4215  xC2DEA269 }";
        right = AZ::Uuid::CreateStringPermissive(permissiveStr4);
        EXPECT_EQ(left, right);

        const char permissiveStr5[] = "{ 0X34D44249 - 0XE5994B30 - 0X811F4215  0XC2DEA269 }";
        right = AZ::Uuid::CreateStringPermissive(permissiveStr5);
        EXPECT_EQ(left, right);
    }

    TEST_F(UuidTests, CreateStringPermissive_InvalidHexAndSpacesGiven_Fails)
    {
        const char uuidStr[] = "{8FDDE7B1 - C332 - 4EBA - BD85 - 2898E7440E4C}";
        AZ::Uuid left = AZ::Uuid::CreateStringPermissive(uuidStr);

        const char permissiveStr1[] = "{ 8FDDE7B1 - 0 xC332 - 4EBA - BD85 - 2898E7440E4C}";
        AZ::Uuid right = AZ::Uuid::CreateStringPermissive(permissiveStr1);
        EXPECT_NE(left, right);
    }

    TEST_F(UuidTests, CreateStringPermissive_InvalidCharacterGiven_Fails)
    {
        AZ::Uuid left = AZ::Uuid::CreateNull();

        // The below check should just give an empty uuid due to the 'g' 
        const char permissiveStr1[] = "{CCF8AB1E- gA04A-43D1-AD8A-70725BC3392E}";
        AZ::Uuid right = AZ::Uuid::CreateStringPermissive(permissiveStr1);
        EXPECT_EQ(left, right);
    }

    TEST_F(UuidTests, CreateStringPermissive_StringWithExtraData_Succeeds)
    {
        const char uuidStr[] = "{34D44249-E599-4B30-811F-4215C2DEA269}";
        AZ::Uuid left = AZ::Uuid::CreateString(uuidStr);

        const char permissiveStr[] = "0x34D44249-0xE5994B30-0x811F4215-0xC2DEA269 Hello World";
        AZ::Uuid right = AZ::Uuid::CreateStringPermissive(permissiveStr);
        EXPECT_EQ(left, right);

    }

    TEST_F(UuidTests, CreateStringPermissive_StringWithLotsOfExtraData_Succeeds)
    {
        const char uuidStr[] = "{34D44249-E599-4B30-811F-4215C2DEA269}";
        AZ::Uuid left = AZ::Uuid::CreateString(uuidStr);

        const char permissiveStr[] = "0x34D44249-0xE5994B30-0x811F4215-0xC2DEA269 Hello World this is a really long string "
            "with lots of extra data to make sure we can parse a long string without failing as long as the uuid is in "
            "the beginning of the string then we should succeed";
        AZ::Uuid right = AZ::Uuid::CreateStringPermissive(permissiveStr);
        EXPECT_EQ(left, right);
    }

    TEST_F(UuidTests, ToFixedString_ResultIsAccurate_Succeeds)
    {
        {
            const char uuidStr[] = "{34D44249-E599-4B30-811F-4215C2DEA269}";
            const AZ::Uuid source = AZ::Uuid::CreateString(uuidStr);
            const AZStd::string dynamic = source.ToString<AZStd::string>();
            const AZ::Uuid::FixedString fixed = source.ToFixedString();
            EXPECT_STREQ(dynamic.c_str(), fixed.c_str());
        }

        {
            const char uuidStr[] = "{678EFGBA-E599-4B30-811F-77775555AAFF}";
            const AZ::Uuid source = AZ::Uuid::CreateString(uuidStr);
            const AZStd::string dynamic = source.ToString<AZStd::string>();
            const AZ::Uuid::FixedString fixed = source.ToFixedString();
            EXPECT_STREQ(dynamic.c_str(), fixed.c_str());
        }
    }

    TEST_F(UuidTests, ToFixedString_FormatSpecifier_Succeeds)
    {
        {
            const char uuidStr[] = "{34D44249-E599-4B30-811F-4215C2DEA269}";
            const AZ::Uuid source = AZ::Uuid::CreateString(uuidStr);
            const AZStd::string dynamic = AZStd::string::format("%s", source.ToString<AZStd::string>().c_str());
            const AZStd::string fixed = AZStd::string::format("%s", source.ToFixedString().c_str());
            EXPECT_EQ(dynamic, fixed);
        }

        {
            const char uuidStr[] = "{678EFGBA-E599-4B30-811F-77775555AAFF}";
            const AZ::Uuid source = AZ::Uuid::CreateString(uuidStr);
            const AZStd::string dynamic = AZStd::string::format("%s", source.ToString<AZStd::string>().c_str());
            const AZStd::string fixed = AZStd::string::format("%s", source.ToFixedString().c_str());
            EXPECT_EQ(dynamic, fixed);
        }
    }

    TEST_F(UuidTests, UuidIsConstexpr_Compiles)
    {
        constexpr AZ::Uuid defaultUuid;
        static_assert(defaultUuid.IsNull());
        static_assert(defaultUuid == AZ::Uuid{});
        static_assert(defaultUuid <= AZ::Uuid{});
        static_assert(defaultUuid >= AZ::Uuid{});
        static_assert(!(defaultUuid != AZ::Uuid{}));
        static_assert(!(defaultUuid < AZ::Uuid{}));
        static_assert(!(defaultUuid > AZ::Uuid{}));

        constexpr AZ::Uuid::FixedString uuidString = defaultUuid.ToFixedString();
        static_assert(uuidString == "{00000000-0000-0000-0000-000000000000}");
        EXPECT_EQ("{00000000-0000-0000-0000-000000000000}", uuidString);

        constexpr auto nullUuid = AZ::Uuid::CreateNull();
        static_assert(nullUuid.IsNull());

        // Uuid from uuid formatted string
        constexpr AZ::Uuid stringUuid{ "{610014D7-DC11-4305-83D8-D59F3AB224B4}" };
        static_assert(!stringUuid.IsNull());
        static_assert(stringUuid != defaultUuid);
        constexpr AZStd::string_view viewString{ "{610014D7-DC11-4305-83D8-D59F3AB224B4}" };
        constexpr auto string2Uuid = AZ::Uuid::CreateString(viewString.data(), viewString.size());
        static_assert(stringUuid == string2Uuid);


        // Uuid from text string using SHA-1 algorithm
        constexpr auto nameUuid = AZ::Uuid::CreateName("BlaBla");
        // check variant
        static_assert(nameUuid.GetVariant() == AZ::Uuid::VAR_RFC_4122);
        // check version
        static_assert(nameUuid.GetVersion() == AZ::Uuid::VER_NAME_SHA1);
        // check id
        static_assert(!nameUuid.IsNull());

        // Uuid from data blob
        constexpr AZStd::array dataArray{ 0x14, 0x56, 0x32, 0xFF, 0x42, 0x98, 0x76, 0x4d, 0x22, 0xFA };
        constexpr auto dataUuid = AZ::Uuid::CreateData(dataArray);
        static_assert(!dataUuid.IsNull());

        // constexpr hash function
        constexpr AZ::Uuid hashStringUuid{ "{1F02F63B-4527-4234-9D2A-AD11F4323019}" };
        // The Uuid::GetHash function returns the first 8 bytes of the Uuid as a size_t
        constexpr size_t expectedHashValue = 0x3442'2745'3BF6'021F;
        static_assert(hashStringUuid.GetHash() == expectedHashValue);
    }

    namespace UuidTestInternal
    {
        template<auto TypeId>
        constexpr const char* TypeConverter()
        {
            return "Primary Converter";
        }
        template<>
        constexpr const char* TypeConverter<O3DE_UUID_TO_NONTYPE_PARAM("{2232CEF3-A4EF-481D-97AC-90044DAD7FBE}")>()
        {
            return "My Custom Converter";
        }
    }

    TEST_F(UuidTests, UuidNonTypeSpecialization_IsCalled)
    {
        constexpr AZ::Uuid uuidNonSpecialized{ "{EE90E096-00FD-498F-9132-AD963C2AA65D}" };
        constexpr AZ::Uuid uuidSpecialized{ "{2232CEF3-A4EF-481D-97AC-90044DAD7FBE}" };
        EXPECT_STREQ("Primary Converter", UuidTestInternal::TypeConverter<O3DE_UUID_TO_NONTYPE_PARAM(uuidNonSpecialized)>());
        EXPECT_STREQ("My Custom Converter", UuidTestInternal::TypeConverter<O3DE_UUID_TO_NONTYPE_PARAM(uuidSpecialized)>());
    }

    // These static asserts test the UuidInternal::GetValue function
    // Expected inputs from a Uuid
    static_assert(AZ::UuidInternal::GetValue('0') == AZStd::byte(0));
    static_assert(AZ::UuidInternal::GetValue('1') == AZStd::byte(1));
    static_assert(AZ::UuidInternal::GetValue('2') == AZStd::byte(2));
    static_assert(AZ::UuidInternal::GetValue('3') == AZStd::byte(3));
    static_assert(AZ::UuidInternal::GetValue('4') == AZStd::byte(4));
    static_assert(AZ::UuidInternal::GetValue('5') == AZStd::byte(5));
    static_assert(AZ::UuidInternal::GetValue('6') == AZStd::byte(6));
    static_assert(AZ::UuidInternal::GetValue('7') == AZStd::byte(7));
    static_assert(AZ::UuidInternal::GetValue('8') == AZStd::byte(8));
    static_assert(AZ::UuidInternal::GetValue('9') == AZStd::byte(9));
    static_assert(AZ::UuidInternal::GetValue('a') == AZStd::byte(10));
    static_assert(AZ::UuidInternal::GetValue('b') == AZStd::byte(11));
    static_assert(AZ::UuidInternal::GetValue('c') == AZStd::byte(12));
    static_assert(AZ::UuidInternal::GetValue('d') == AZStd::byte(13));
    static_assert(AZ::UuidInternal::GetValue('e') == AZStd::byte(14));
    static_assert(AZ::UuidInternal::GetValue('f') == AZStd::byte(15));
    static_assert(AZ::UuidInternal::GetValue('A') == AZStd::byte(10));
    static_assert(AZ::UuidInternal::GetValue('B') == AZStd::byte(11));
    static_assert(AZ::UuidInternal::GetValue('C') == AZStd::byte(12));
    static_assert(AZ::UuidInternal::GetValue('D') == AZStd::byte(13));
    static_assert(AZ::UuidInternal::GetValue('E') == AZStd::byte(14));
    static_assert(AZ::UuidInternal::GetValue('F') == AZStd::byte(15));

    // Common characters in a Uuid
    static_assert(AZ::UuidInternal::GetValue('{') == AZ::UuidInternal::InvalidValue);
    static_assert(AZ::UuidInternal::GetValue('-') == AZ::UuidInternal::InvalidValue);
    static_assert(AZ::UuidInternal::GetValue('}') == AZ::UuidInternal::InvalidValue);
    static_assert(AZ::UuidInternal::GetValue(' ') == AZ::UuidInternal::InvalidValue);

    // Boundary conditions with invalid Uuid characters
    static_assert(AZ::UuidInternal::GetValue(0) == AZ::UuidInternal::InvalidValue);
    static_assert(AZ::UuidInternal::GetValue(AZStd::numeric_limits<char>::max()) == AZ::UuidInternal::InvalidValue);
    static_assert(AZ::UuidInternal::GetValue('0' - 1) == AZ::UuidInternal::InvalidValue);
    static_assert(AZ::UuidInternal::GetValue('9' + 1) == AZ::UuidInternal::InvalidValue);
    static_assert(AZ::UuidInternal::GetValue('a' - 1) == AZ::UuidInternal::InvalidValue);
    static_assert(AZ::UuidInternal::GetValue('f' + 1) == AZ::UuidInternal::InvalidValue);
    static_assert(AZ::UuidInternal::GetValue('A' - 1) == AZ::UuidInternal::InvalidValue);
    static_assert(AZ::UuidInternal::GetValue('F' + 1) == AZ::UuidInternal::InvalidValue);

    
    class UuidBenchmark : public AllocatorsBenchmarkFixture
    {
    public:
        void SetUp(::benchmark::State& st) override
        {
            AllocatorsBenchmarkFixture::SetUp(st);
            SetUpInternal();
        }

        void SetUp(const ::benchmark::State& st) override
        {
            AllocatorsBenchmarkFixture::SetUp(st);
            SetUpInternal();
        }

        void TearDown(::benchmark::State& st) override
        {
            TearDownInternal();
            AllocatorsBenchmarkFixture::TearDown(st);
        }

        void TearDown(const ::benchmark::State& st) override
        {
            TearDownInternal();
            AllocatorsBenchmarkFixture::TearDown(st);
        }

        void SetUpInternal()
        {
            std::mt19937 randomEngine(0);
            m_uuidStrings.resize(100);
            // Generate some random guid strings, using a random mix of guid formats
            for (AZStd::string& uuidString : m_uuidStrings)
            {
                bool useBrackets = std::uniform_int_distribution<>{ 0, 1 }(randomEngine);
                bool useDashes = std::uniform_int_distribution<>{ 0, 1 }(randomEngine);
                uuidString = AZ::Uuid::CreateRandom().ToString<AZStd::string>(useBrackets, useDashes);
            }
        }

        void TearDownInternal()
        {
            m_uuidStrings.clear();
            m_uuidStrings.shrink_to_fit();
        }

        AZStd::vector<AZStd::string> m_uuidStrings;
    };

    BENCHMARK_DEFINE_F(UuidBenchmark, CreateString_Benchmark)(::benchmark::State& st)
    {
        while (st.KeepRunning())
        {
            for (const AZStd::string& uuidString : m_uuidStrings)
            {
                AZ::Uuid::CreateString(uuidString);
            }
        }
    }
    BENCHMARK_REGISTER_F(UuidBenchmark, CreateString_Benchmark)->Unit(benchmark::kNanosecond);
}
