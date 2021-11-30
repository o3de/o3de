/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <AzFramework/StringFunc/StringFunc.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace AzFramework;

    //! Unit Test for testing Base64 Encode/Decode functions
    class Base64Test
        : public AllocatorsFixture
    {
    public:
        Base64Test()
            : AllocatorsFixture()
        {
        }

        void SetUp() override
        {
            AllocatorsFixture::SetUp();
            AllocatorInstance<PoolAllocator>::Create();
            AllocatorInstance<ThreadPoolAllocator>::Create();
            ComponentApplication::Descriptor desc;
            desc.m_useExistingAllocator = true;
            desc.m_enableDrilling = false; // we already created a memory driller for the test (AllocatorsFixture)
            m_app.Create(desc);
        }

        void TearDown() override
        {
            m_app.Destroy();
            AllocatorInstance<PoolAllocator>::Destroy();
            AllocatorInstance<ThreadPoolAllocator>::Destroy();
            AllocatorsFixture::TearDown();
        }

        ~Base64Test() override
        {
            
        }

        ComponentApplication m_app;
    };

    TEST_F(Base64Test, EmptyStringEncodeTest)
    {
        AZStd::string value;
        AZStd::string encodedString = AzFramework::StringFunc::Base64::Encode(reinterpret_cast<AZ::u8*>(value.data()), value.size());
        EXPECT_EQ("", encodedString);
    }

    //! Test vectors from the Base-N encodings rfc https://tools.ietf.org/html/rfc4648#section-10
    TEST_F(Base64Test, Rfc4648EncodeTest)
    {
        AZStd::string value = "f";
        AZStd::string encodedString = AzFramework::StringFunc::Base64::Encode(reinterpret_cast<AZ::u8*>(value.data()), value.size());
        EXPECT_EQ("Zg==", encodedString);

        value = "fo";
        encodedString = AzFramework::StringFunc::Base64::Encode(reinterpret_cast<AZ::u8*>(value.data()), value.size());
        EXPECT_EQ("Zm8=", encodedString);

        value = "foo";
        encodedString = AzFramework::StringFunc::Base64::Encode(reinterpret_cast<AZ::u8*>(value.data()), value.size());
        EXPECT_EQ("Zm9v", encodedString);

        value = "foob";
        encodedString = AzFramework::StringFunc::Base64::Encode(reinterpret_cast<AZ::u8*>(value.data()), value.size());
        EXPECT_EQ("Zm9vYg==", encodedString);

        value = "fooba";
        encodedString = AzFramework::StringFunc::Base64::Encode(reinterpret_cast<AZ::u8*>(value.data()), value.size());
        EXPECT_EQ("Zm9vYmE=", encodedString);

        value = "foobar";
        encodedString = AzFramework::StringFunc::Base64::Encode(reinterpret_cast<AZ::u8*>(value.data()), value.size());
        EXPECT_EQ("Zm9vYmFy", encodedString);
    }

    //! Test vectors from the Base-N encodings rfc https://tools.ietf.org/html/rfc4648#section-10
    TEST_F(Base64Test, Rfc4648DecodeTest)
    {
        AZStd::vector<AZ::u8> decodedVector;
        AZStd::string value = "Zg==";
        EXPECT_TRUE(AzFramework::StringFunc::Base64::Decode(decodedVector, value.data(), value.size()));
        size_t strLen = AZStd::min(AZ_ARRAY_SIZE("f") - 1, decodedVector.size());
        EXPECT_EQ(0, memcmp("f", decodedVector.data(), strLen));

        value = "Zm8=";
        EXPECT_TRUE(AzFramework::StringFunc::Base64::Decode(decodedVector, value.data(), value.size()));
        strLen = AZStd::min(AZ_ARRAY_SIZE("fo") - 1, decodedVector.size());
        EXPECT_EQ(0, memcmp("fo", decodedVector.data(), strLen));

        value = "Zm9v";
        EXPECT_TRUE(AzFramework::StringFunc::Base64::Decode(decodedVector, value.data(), value.size()));
        strLen = AZStd::min(AZ_ARRAY_SIZE("foo") - 1, decodedVector.size());
        EXPECT_EQ(0, memcmp("foo", decodedVector.data(), strLen));

        value = "Zm9vYg==";
        EXPECT_TRUE(AzFramework::StringFunc::Base64::Decode(decodedVector, value.data(), value.size()));
        strLen = AZStd::min(AZ_ARRAY_SIZE("foob") - 1, decodedVector.size());
        EXPECT_EQ(0, memcmp("foob", decodedVector.data(), strLen));

        value = "Zm9vYmE=";
        EXPECT_TRUE(AzFramework::StringFunc::Base64::Decode(decodedVector, value.data(), value.size()));
        strLen = AZStd::min(AZ_ARRAY_SIZE("fooba") - 1, decodedVector.size());
        EXPECT_EQ(0, memcmp("fooba", decodedVector.data(), strLen));

        value = "Zm9vYmFy";
        EXPECT_TRUE(AzFramework::StringFunc::Base64::Decode(decodedVector, value.data(), value.size()));
        strLen = AZStd::min(AZ_ARRAY_SIZE("foobar") - 1, decodedVector.size());
        EXPECT_EQ(0, memcmp("foobar", decodedVector.data(), strLen));
    }

    //! Test RFC 4648 Binary https://tools.ietf.org/html/rfc4648#page-12
    TEST_F(Base64Test, Rfc4648BinaryEncodeTest)
    {
        const AZ::u8 binaryValue[] = { 0x14, 0xfb, 0x9c, 0x03, 0xd9, 0x7e };
        AZStd::string encodedString = AzFramework::StringFunc::Base64::Encode(binaryValue, AZ_ARRAY_SIZE(binaryValue));
        EXPECT_EQ("FPucA9l+", encodedString);

        const AZ::u8 binaryValue2[] = { 0x14, 0xfb, 0x9c, 0x03, 0xd9 };
        encodedString = AzFramework::StringFunc::Base64::Encode(binaryValue2, AZ_ARRAY_SIZE(binaryValue2));
        EXPECT_EQ("FPucA9k=", encodedString);

        const AZ::u8 binaryValue3[] = { 0x14, 0xfb, 0x9c, 0x03 };
        encodedString = AzFramework::StringFunc::Base64::Encode(binaryValue3, AZ_ARRAY_SIZE(binaryValue3));
        EXPECT_EQ("FPucAw==", encodedString);

        EXPECT_EQ("TlVMAEluU3RyaW5n", AzFramework::StringFunc::Base64::Encode(reinterpret_cast<const AZ::u8*>("NUL\0InString"), AZ_ARRAY_SIZE("NUL\0InString") - 1));
    }

    //! Test RFC 4648 Binary https://tools.ietf.org/html/rfc4648#page-12
    TEST_F(Base64Test, Rfc4648BinaryDecodeTest)
    {
        const AZ::u8 expectedBinaryValue[] = { 0x14, 0xfb, 0x9c, 0x03, 0xd9, 0x7e };
        const AZ::u8 expectedBinaryValue2[] = { 0x14, 0xfb, 0x9c, 0x03, 0xd9 };
        const AZ::u8 expectedBinaryValue3[] = { 0x14, 0xfb, 0x9c, 0x03 };
        const AZ::u8 expectedBinaryValue4[] = { 'N','U', 'L', '\0', 'I', 'n', 'S', 't', 'r', 'i', 'n', 'g' };

        AZStd::vector<AZ::u8> decodedVector;
        const char textValue[] = "FPucA9l+";
        EXPECT_TRUE(AzFramework::StringFunc::Base64::Decode(decodedVector, textValue, AZ_ARRAY_SIZE(textValue) - 1));
        size_t vecLen = AZStd::min(AZ_ARRAY_SIZE(expectedBinaryValue), decodedVector.size());
        EXPECT_EQ(0, memcmp(expectedBinaryValue, decodedVector.data(), vecLen));


        const char textValue2[] = "FPucA9k=";
        EXPECT_TRUE(AzFramework::StringFunc::Base64::Decode(decodedVector, textValue2, AZ_ARRAY_SIZE(textValue2) - 1));
        vecLen = AZStd::min(AZ_ARRAY_SIZE(expectedBinaryValue2), decodedVector.size());
        EXPECT_EQ(0, memcmp(expectedBinaryValue2, decodedVector.data(), vecLen));

        const char textValue3[] = "FPucAw==";
        EXPECT_TRUE(AzFramework::StringFunc::Base64::Decode(decodedVector, textValue3, AZ_ARRAY_SIZE(textValue3) - 1));
        vecLen = AZStd::min(AZ_ARRAY_SIZE(expectedBinaryValue3), decodedVector.size());
        EXPECT_EQ(0, memcmp(expectedBinaryValue3, decodedVector.data(), vecLen));

        EXPECT_TRUE(AzFramework::StringFunc::Base64::Decode(decodedVector, "TlVMAEluU3RyaW5n", strlen("TlVMAEluU3RyaW5n")));
        vecLen = AZStd::min(AZ_ARRAY_SIZE(expectedBinaryValue4), decodedVector.size());
        EXPECT_EQ(0, memcmp(expectedBinaryValue4, decodedVector.data(), vecLen));
    }

    TEST_F(Base64Test, EmptyStringDecodeTest)
    {
        AZStd::vector<AZ::u8> decodedString;
        const char value[] = "";
        EXPECT_TRUE(AzFramework::StringFunc::Base64::Decode(decodedString, value, AZ_ARRAY_SIZE(value) - 1));
        EXPECT_EQ(0, decodedString.size());
    }
    
    TEST_F(Base64Test, ErrorDecodeTest)
    {
        AZStd::vector<AZ::u8> expectedVector{ 'T', 'e', 'x', 't', 0xDF };
        AZStd::vector<AZ::u8> decodedString = expectedVector;
        const char value[] = "NotMultpleOf4";
        EXPECT_FALSE(AzFramework::StringFunc::Base64::Decode(decodedString, value, AZ_ARRAY_SIZE(value) - 1));
        EXPECT_EQ(expectedVector, decodedString);

        const char value2[] = "Bad" "\x00" "Data@^&=";
        EXPECT_FALSE(AzFramework::StringFunc::Base64::Decode(decodedString, value2, AZ_ARRAY_SIZE(value2) - 1));
        EXPECT_EQ(expectedVector, decodedString);
    }
}
