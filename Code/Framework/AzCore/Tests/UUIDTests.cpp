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

using namespace AZ;

namespace UnitTest
{
    class UuidTests
        : public UnitTest::AllocatorsTestFixture
    {
        static const int numUuid = 2000;
        Uuid* m_array;
    public:
        void SetUp() override
        {
            AllocatorsTestFixture::SetUp();

            m_array = (Uuid*)azmalloc(sizeof(Uuid) * numUuid, AZStd::alignment_of<Uuid>::value);
        }
        void TearDown() override
        {
            azfree(m_array);

            AllocatorsTestFixture::TearDown();
        }
        void run()
        {
            Uuid defId;
            defId.data[0] = 0xb5;
            defId.data[1] = 0x70;
            defId.data[2] = 0x0f;
            defId.data[3] = 0x2e;
            defId.data[4] = 0x66;
            defId.data[5] = 0x1b;
            defId.data[6] = 0x4a;
            defId.data[7] = 0xc0;
            defId.data[8] = 0x93;
            defId.data[9] = 0x35;
            defId.data[10] = 0x81;
            defId.data[11] = 0x7c;
            defId.data[12] = 0xb4;
            defId.data[13] = 0xc0;
            defId.data[14] = 0x9c;
            defId.data[15] = 0xcb;

            // null
            Uuid id = Uuid::CreateNull();
            AZ_TEST_ASSERT(id.IsNull());

            const char idStr1[] = "{B5700F2E-661B-4AC0-9335-817CB4C09CCB}";
            const char idStr2[] = "{B5700F2E661B4AC09335817CB4C09CCB}";
            const char idStr3[] = "B5700F2E-661B-4AC0-9335-817CB4C09CCB";
            const char idStr4[] = "B5700F2E661B4AC09335817CB4C09CCB";

            // create from string
            id = Uuid::CreateString(idStr1);
            AZ_TEST_ASSERT(id == defId);
            id = Uuid::CreateString(idStr2);
            AZ_TEST_ASSERT(id == defId);
            id = Uuid::CreateString(idStr3);
            AZ_TEST_ASSERT(id == defId);
            id = Uuid::CreateString(idStr4);
            AZ_TEST_ASSERT(id == defId);

            // variant
            AZ_TEST_ASSERT(id.GetVariant() == Uuid::VAR_RFC_4122);
            // version
            AZ_TEST_ASSERT(id.GetVersion() == Uuid::VER_RANDOM);

            // tostring
            char buffer[39];
            id = Uuid::CreateString(idStr1);
            AZ_TEST_ASSERT(id.ToString(buffer, 39, true, true) == 39);
            AZ_TEST_ASSERT(strcmp(buffer, idStr1) == 0);
            AZ_TEST_ASSERT(id.ToString(buffer, 35, true, false) == 35);
            AZ_TEST_ASSERT(strcmp(buffer, idStr2) == 0);
            AZ_TEST_ASSERT(id.ToString(buffer, 37, false, true) == 37);
            AZ_TEST_ASSERT(strcmp(buffer, idStr3) == 0);
            AZ_TEST_ASSERT(id.ToString(buffer, 33, false, false) == 33);
            AZ_TEST_ASSERT(strcmp(buffer, idStr4) == 0);

            AZ_TEST_ASSERT(id.ToString<AZStd::string>() == AZStd::string(idStr1));
            AZ_TEST_ASSERT(id.ToString<AZStd::string>(true, false) == AZStd::string(idStr2));
            AZ_TEST_ASSERT(id.ToString<AZStd::string>(false, true) == AZStd::string(idStr3));
            AZ_TEST_ASSERT(id.ToString<AZStd::string>(false, false) == AZStd::string(idStr4));

            AZStd::string str1;
            id.ToString(str1);
            AZ_TEST_ASSERT(str1 == AZStd::string(idStr1));
            id.ToString(str1, true, false);
            AZ_TEST_ASSERT(str1 == AZStd::string(idStr2));
            id.ToString(str1, false, true);
            AZ_TEST_ASSERT(str1 == AZStd::string(idStr3));
            id.ToString(str1, false, false);
            AZ_TEST_ASSERT(str1 == AZStd::string(idStr4));

            // operators
            Uuid idBigger("C5700F2E661B4ac09335817CB4C09CCB");
            AZ_TEST_ASSERT(id < idBigger);
            AZ_TEST_ASSERT(id != idBigger);
            AZ_TEST_ASSERT(idBigger > id);

            // hash
            AZStd::hash<AZ::Uuid> hash;
            size_t hashVal = hash(id);
            AZ_TEST_ASSERT(hashVal != 0);

            // test the hashing and equal function in a unordered container
            typedef AZStd::unordered_set<AZ::Uuid> UuidSetType;
            UuidSetType uuidSet;
            uuidSet.insert(id);
            AZ_TEST_ASSERT(uuidSet.find(id) != uuidSet.end());

            // check uniqueness (very quick and basic)
            for (int i = 0; i < numUuid; ++i)
            {
                m_array[i] = Uuid::Create();
            }

            for (int i = 0; i < numUuid; ++i)
            {
                Uuid uniqueToTest = Uuid::Create();
                for (int j = 0; j < numUuid; ++j)
                {
                    AZ_TEST_ASSERT(m_array[j] != uniqueToTest);
                }
            }

            // test the name function
            Uuid uuidName = Uuid::CreateName("BlaBla");
            // check variant
            AZ_TEST_ASSERT(uuidName.GetVariant() == Uuid::VAR_RFC_4122);
            // check version
            AZ_TEST_ASSERT(uuidName.GetVersion() == Uuid::VER_NAME_SHA1);
            // check id
            AZ_TEST_ASSERT(uuidName == Uuid::CreateName("BlaBla"));
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
        Uuid left = Uuid::CreateString(leftStr);
        Uuid right = Uuid::CreateString(rightStr);
        EXPECT_TRUE(left >= right);
    }

    TEST_F(UuidTests, GreaterThanOrEqualTo_LeftLessThanRight_ReturnsFalse)
    {
        const char leftStr[] = "{A418022E-DAFE-4450-BCB9-4B7727070178}";
        const char rightStr[] = "{B5700F2E-661B-4AC0-9335-817CB4C09CCB}";
        Uuid left = Uuid::CreateString(leftStr);
        Uuid right = Uuid::CreateString(rightStr);
        EXPECT_FALSE(left >= right);
    }

    TEST_F(UuidTests, GreaterThanOrEqualTo_LeftEqualsRight_ReturnsTrue)
    {
        const char uuidStr[] = "{F418022E-DAFE-4450-BCB9-4B7727070178}";
        Uuid left = Uuid::CreateString(uuidStr);
        Uuid right = Uuid::CreateString(uuidStr);
        EXPECT_TRUE(left >= right);
    }

    TEST_F(UuidTests, LessThanOrEqualTo_LeftGreaterThanRight_ReturnsFalse)
    {
        const char leftStr[] = "{F418022E-DAFE-4450-BCB9-4B7727070178}";
        const char rightStr[] = "{B5700F2E-661B-4AC0-9335-817CB4C09CCB}";
        Uuid left = Uuid::CreateString(leftStr);
        Uuid right = Uuid::CreateString(rightStr);
        EXPECT_FALSE(left <= right);
    }

    TEST_F(UuidTests, LessThanOrEqualTo_LeftLessThanRight_ReturnsTrue)
    {
        const char leftStr[] = "{A418022E-DAFE-4450-BCB9-4B7727070178}";
        const char rightStr[] = "{B5700F2E-661B-4AC0-9335-817CB4C09CCB}";
        Uuid left = Uuid::CreateString(leftStr);
        Uuid right = Uuid::CreateString(rightStr);
        EXPECT_TRUE(left <= right);
    }

    TEST_F(UuidTests, LessThanOrEqualTo_LeftEqualsRight_ReturnsTrue)
    {
        const char uuidStr[] = "{F418022E-DAFE-4450-BCB9-4B7727070178}";
        Uuid left = Uuid::CreateString(uuidStr);
        Uuid right = Uuid::CreateString(uuidStr);
        EXPECT_TRUE(left <= right);
    }

    TEST_F(UuidTests, CreateStringPermissive_HexAndSpacesGiven_Success)
    {
        const char uuidStr[] = "{34D44249-E599-4B30-811F-4215C2DEA269}";
        Uuid left = Uuid::CreateString(uuidStr);

        const char permissiveStr[] = "{ 0x34D44249 - 0xE5994B30 - 0x811F4215 - 0xC2DEA269 }";
        Uuid right = Uuid::CreateStringPermissive(permissiveStr);
        EXPECT_EQ(left, right);

        const char permissiveStr2[] = "{ 0x34D44249-0xE5994B30  0x811F4215 - C2DEA269 }";
        right = Uuid::CreateStringPermissive(permissiveStr2);
        EXPECT_EQ(left, right);

        const char permissiveStr3[] = "34D44249-0xE5994B30  0x811F4215 - C2DEA269 }";
        right = Uuid::CreateStringPermissive(permissiveStr3);
        EXPECT_EQ(left, right);

        const char permissiveStr4[] = "{ x34D44249 - xE5994B30 - x811F4215  xC2DEA269 }";
        right = Uuid::CreateStringPermissive(permissiveStr4);
        EXPECT_EQ(left, right);

        const char permissiveStr5[] = "{ 0X34D44249 - 0XE5994B30 - 0X811F4215  0XC2DEA269 }";
        right = Uuid::CreateStringPermissive(permissiveStr5);
        EXPECT_EQ(left, right);
    }

    TEST_F(UuidTests, CreateStringPermissive_InvalidHexAndSpacesGiven_Fails)
    {
        const char uuidStr[] = "{8FDDE7B1 - C332 - 4EBA - BD85 - 2898E7440E4C}";
        Uuid left = Uuid::CreateStringPermissive(uuidStr);

        const char permissiveStr1[] = "{ 8FDDE7B1 - 0 xC332 - 4EBA - BD85 - 2898E7440E4C}";
        Uuid right = Uuid::CreateStringPermissive(permissiveStr1);
        EXPECT_NE(left, right);
    }

    TEST_F(UuidTests, CreateStringPermissive_InvalidCharacterGiven_Fails)
    {
        Uuid left = Uuid::CreateNull();

        // The below check should just give an empty uuid due to the g
        const char permissiveStr1[] = "{CCF8AB1E- gA04A-43D1-AD8A-70725BC3392E}";
        Uuid right = Uuid::CreateStringPermissive(permissiveStr1);
        EXPECT_EQ(left, right);
    }

    TEST_F(UuidTests, CreateStringPermissive_StringWithExtraData_Succeeds)
    {
        const char uuidStr[] = "{34D44249-E599-4B30-811F-4215C2DEA269}";
        Uuid left = Uuid::CreateString(uuidStr);

        const char permissiveStr[] = "0x34D44249-0xE5994B30-0x811F4215-0xC2DEA269 Hello World";
        Uuid right = Uuid::CreateStringPermissive(permissiveStr);
        EXPECT_EQ(left, right);

    }

    TEST_F(UuidTests, CreateStringPermissive_StringWithLotsOfExtraData_Succeeds)
    {
        const char uuidStr[] = "{34D44249-E599-4B30-811F-4215C2DEA269}";
        Uuid left = Uuid::CreateString(uuidStr);

        const char permissiveStr[] = "0x34D44249-0xE5994B30-0x811F4215-0xC2DEA269 Hello World this is a really long string "
        "with lots of extra data to make sure we can parse a long string without failing as long as the uuid is in "
        "the beginning of the string then we should succeed";
        Uuid right = Uuid::CreateStringPermissive(permissiveStr);
        EXPECT_EQ(left, right);
    }
}
