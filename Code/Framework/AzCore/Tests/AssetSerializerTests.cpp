/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AZTestShared/Utils/Utils.h>


class AssetSerializerTest
    : public UnitTest::LeakDetectionFixture
{
    public:
        void SetUp() override
        {
            UnitTest::LeakDetectionFixture::SetUp();

            // Fill in v0 and v1 of the test data with subsets of our v2 test asset.
            memcpy(m_testSerializedAssetVersion0, m_testSerializedAssetVersion2, AZ_ARRAY_SIZE(m_testSerializedAssetVersion0));
            memcpy(m_testSerializedAssetVersion1, m_testSerializedAssetVersion2, AZ_ARRAY_SIZE(m_testSerializedAssetVersion1));
            // Fill in v0 and v1 of the test strings with subsets of our v2 test string.
            m_testAssetStringVersion0 = m_testAssetStringVersion2.substr(0, m_testAssetStringVersion2.find(",hint="));
            m_testAssetStringVersion1 = m_testAssetStringVersion2.substr(0, m_testAssetStringVersion2.find(",loadBehavior="));
        }

        void TearDown() override
        {
            UnitTest::LeakDetectionFixture::TearDown();
        }

    protected:
        const AZ::Data::AssetId m_testAssetId{ AZ::Uuid("{01234567-89AB-CDEF-FEDC-BA9876543210}"), 0x33774488 };
        const AZ::Data::AssetType m_testAssetType{ AZ::Uuid("{00112233-4455-6677-8899-AABBCCDDEEFF}") };
        const AZStd::string m_testAssetHint{ "abcd" };
        // Pick a test value that's not a default value.
        const AZ::Data::AssetLoadBehavior m_testAssetLoadBehavior{ AZ::Data::AssetLoadBehavior::NoLoad };

        // Buffer to contain v0 of serialized data (AssetId + AssetType)
        uint8_t m_testSerializedAssetVersion0[0x30];
        // Buffer to contain v1 of serialized data (AssetId + AssetType + AssetHint)
        uint8_t m_testSerializedAssetVersion1[0x3C];
        // Buffer containing v2 of serialized data (AssetId + AssetType + AssetHint + AssetLoadBehavior)
        uint8_t m_testSerializedAssetVersion2[0x3D]
        {
            // AssetId - Uuid
            0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
            0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10,
            // AssetId - SubId (account for little endianness)
            0x88, 0x44, 0x77, 0x33,
            // AssetId - pad bytes
                                    0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            // Asset Type
            0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
            0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
            // Asset Hint Size (account for little endianness)
            0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            // Asset Hint
            'a', 'b', 'c', 'd',
            // Asset Load Behavior
            aznumeric_cast<uint8_t>(AZ::Data::AssetLoadBehavior::NoLoad)
        };

        AZStd::string m_testAssetStringVersion0;
        AZStd::string m_testAssetStringVersion1;
        const AZStd::string m_testAssetStringVersion2
        {
            "id={01234567-89AB-CDEF-FEDC-BA9876543210}:33774488,type={00112233-4455-6677-8899-AABBCCDDEEFF},hint={abcd},loadBehavior=2"
        };
};


TEST_F(AssetSerializerTest, Load_LoadEmptyStream_LoadUnsuccessful)
{
    // Create a memory stream that's too small.
    uint8_t memBuffer[] = { 0 };
    AZ::IO::MemoryStream memStream(memBuffer, AZ_ARRAY_SIZE(memBuffer));

    // Try to load the data from the stream into a new asset
    constexpr bool isDataBigEndian = false;
    constexpr unsigned int version = 0;
    AZ::Data::Asset<AZ::Data::AssetData> testAsset;
    bool success = AZ::AssetSerializer::s_serializer.Load(&testAsset, memStream, version, isDataBigEndian);

    // Verify that the Load was unsuccessful
    EXPECT_FALSE(success);
}


TEST_F(AssetSerializerTest, Load_LoadVersion0Data_LoadSuccessful)
{
    // Create a test asset with an arbitrary set of expected values. (v0 is AssetId + AssetType, default hint and loadBehavior)
    AZ::Data::Asset<AZ::Data::AssetData> expectedAsset(m_testAssetId, m_testAssetType, AZStd::string());

    // Create a stream buffer that should match our expected asset, using version 0 of serialized data.
    AZ::IO::MemoryStream memStream(m_testSerializedAssetVersion0, AZ_ARRAY_SIZE(m_testSerializedAssetVersion0));

    // Load the data from the stream into a new asset
    constexpr bool isDataBigEndian = false;
    constexpr unsigned int version = 0;
    AZ::Data::Asset<AZ::Data::AssetData> testAsset;
    bool success = AZ::AssetSerializer::s_serializer.Load(&testAsset, memStream, version, isDataBigEndian);

    // Verify that the data read back in via Load matches the expected asset.
    EXPECT_TRUE(success);
    EXPECT_EQ(testAsset, expectedAsset);
}

TEST_F(AssetSerializerTest, Load_LoadVersion1Data_LoadSuccessful)
{
    // Create a test asset with an arbitrary set of expected values. (v1 is AssetId + AssetType + AssetHint, default loadBehavior)
    AZ::Data::Asset<AZ::Data::AssetData> expectedAsset(m_testAssetId, m_testAssetType, m_testAssetHint);

    // Create a stream buffer that should match our expected asset, using version 1 of serialized data.
    AZ::IO::MemoryStream memStream(m_testSerializedAssetVersion1, AZ_ARRAY_SIZE(m_testSerializedAssetVersion1));

    // Load the data from the stream into a new asset
    constexpr bool isDataBigEndian = false;
    constexpr unsigned int version = 1;
    AZ::Data::Asset<AZ::Data::AssetData> testAsset;
    bool success = AZ::AssetSerializer::s_serializer.Load(&testAsset, memStream, version, isDataBigEndian);

    // Verify that the data read back in via Load matches the expected asset.
    EXPECT_TRUE(success);
    EXPECT_EQ(testAsset, expectedAsset);
}

TEST_F(AssetSerializerTest, Load_LoadVersion2Data_LoadSuccessful)
{
    // Create a test asset with an arbitrary set of expected values. (v2 is AssetId + AssetType + AssetHint + AssetLoadBehavior)
    AZ::Data::Asset<AZ::Data::AssetData> expectedAsset(m_testAssetId, m_testAssetType, m_testAssetHint);
    expectedAsset.SetAutoLoadBehavior(m_testAssetLoadBehavior);

    // Create a stream buffer that should match our expected asset, using version 2 of serialized data.
    AZ::IO::MemoryStream memStream(m_testSerializedAssetVersion2, AZ_ARRAY_SIZE(m_testSerializedAssetVersion2));

    // Load the data from the stream into a new asset
    constexpr bool isDataBigEndian = false;
    constexpr unsigned int version = 2;
    AZ::Data::Asset<AZ::Data::AssetData> testAsset;
    bool success = AZ::AssetSerializer::s_serializer.Load(&testAsset, memStream, version, isDataBigEndian);

    // Verify that the data read back in via Load matches the expected asset.
    EXPECT_TRUE(success);
    EXPECT_EQ(testAsset, expectedAsset);
}

TEST_F(AssetSerializerTest, Save_SaveValidAsset_SaveSuccessful)
{
    // Create a test asset with an arbitrary set of expected values.
    AZ::Data::Asset<AZ::Data::AssetData> testAsset(m_testAssetId, m_testAssetType, m_testAssetHint);
    testAsset.SetAutoLoadBehavior(m_testAssetLoadBehavior);

    // Create a memory stream to save into, and initialize it with bad data.
    // Also, make it larger than our expected size to verify that the Save doesn't try to write more than expected.
    char memBuffer[AZ_ARRAY_SIZE(m_testSerializedAssetVersion2) * 2];
    memset(memBuffer, 'X', AZ_ARRAY_SIZE(memBuffer));
    AZ::IO::MemoryStream memStream(memBuffer, AZ_ARRAY_SIZE(memBuffer), 0);

    // Save the test asset.
    constexpr bool isDataBigEndian = false;
    size_t bytesWritten = AZ::AssetSerializer::s_serializer.Save(&testAsset, memStream, isDataBigEndian);

    // Verify that the number of bytes returned is the same amount actually written into the mem stream.
    EXPECT_EQ(bytesWritten, memStream.GetLength());

    // Verify that the number of bytes written matches the expected size.
    EXPECT_EQ(bytesWritten, AZ_ARRAY_SIZE(m_testSerializedAssetVersion2));

    // Verify that the bytes written match the expected buffer
    EXPECT_EQ(0, memcmp(m_testSerializedAssetVersion2, memBuffer, AZ_ARRAY_SIZE(m_testSerializedAssetVersion2)));
}

TEST_F(AssetSerializerTest, DataToText_ConvertValidAsset_ConversionSuccessful)
{
    // Create a memory stream containing the test serialized asset data.
    AZ::IO::MemoryStream memStream(m_testSerializedAssetVersion2, AZ_ARRAY_SIZE(m_testSerializedAssetVersion2));

    // Create a string buffer initialized with bad values and convert the serialized asset into text via DataToText.
    char stringBuffer[0x1000];
    memset(stringBuffer, 'X', AZ_ARRAY_SIZE(stringBuffer));
    AZ::IO::MemoryStream stringStream(stringBuffer, AZ_ARRAY_SIZE(stringBuffer), 0);
    constexpr bool isDataBigEndian = false;
    size_t stringSize = AZ::AssetSerializer::s_serializer.DataToText(memStream, stringStream, isDataBigEndian);

    // Verify that the result matches expectations.
    AZStd::string resultString(stringBuffer, stringSize);
    EXPECT_EQ(resultString, m_testAssetStringVersion2);
}

TEST_F(AssetSerializerTest, TextToData_ConvertVersion0Data_ConversionSuccessful)
{
    // Create a test asset with an arbitrary set of expected values.  (v0 contains AssetId + AssetType, default hint and loadBehavior)
    AZ::Data::Asset<AZ::Data::AssetData> testAsset(m_testAssetId, m_testAssetType, AZStd::string());

    // Save the asset into a memory stream.
    // We can't just use any of our test asset buffers directly because we need one saved in the latest version format and size,
    // but only with version 0 asset data in it.
    char testAssetBuffer[0x1000];
    AZ::IO::MemoryStream testAssetStream(testAssetBuffer, AZ_ARRAY_SIZE(testAssetBuffer), 0);
    constexpr bool isDataBigEndian = false;
    size_t testAssetBytesWritten = AZ::AssetSerializer::s_serializer.Save(&testAsset, testAssetStream, isDataBigEndian);
    // Make sure the amount written matches the expected size
    // (AssetId + AsssetType + AssetHint size + 0-length AssetHint string + AssetLoadBehavior)
    EXPECT_EQ(testAssetBytesWritten, sizeof(AZ::Data::AssetId) + sizeof(AZ::Data::AssetType) + sizeof(uint64_t) + 0 +
                                     sizeof(AZ::Data::AssetLoadBehavior));

    // Create an arbitrarily large memory stream to hold our converted data, and initialize to bad values.
    char memBuffer[0x1000];
    memset(memBuffer, 'X', AZ_ARRAY_SIZE(memBuffer));
    AZ::IO::MemoryStream memStream(memBuffer, AZ_ARRAY_SIZE(memBuffer), 0);

    // Convert a version 0 text string to data
    constexpr unsigned int version = 0;
    size_t bytesWritten = AZ::AssetSerializer::s_serializer.TextToData(m_testAssetStringVersion0.c_str(),
                                                                       version, memStream, isDataBigEndian);

    // Make sure the result value matches the actual amount written into the stream
    EXPECT_EQ(bytesWritten, memStream.GetLength());
    // Make sure the amount written matches the expected size
    EXPECT_EQ(bytesWritten, testAssetBytesWritten);
    // Make sure the data written matches the expected data
    EXPECT_EQ(0, memcmp(testAssetBuffer, memBuffer, testAssetBytesWritten));
}

TEST_F(AssetSerializerTest, TextToData_ConvertVersion1Data_ConversionSuccessful)
{
    // Create a test asset with an arbitrary set of expected values.  (v1 contains AssetId + AssetType + AssetHint, default loadBehavior)
    AZ::Data::Asset<AZ::Data::AssetData> testAsset(m_testAssetId, m_testAssetType, m_testAssetHint);

    // Save the asset into a memory stream.
    // We can't just use any of our test asset buffers directly because we need one saved in the latest version format and size,
    // but only with version 1 asset data in it.
    char testAssetBuffer[0x1000];
    AZ::IO::MemoryStream testAssetStream(testAssetBuffer, AZ_ARRAY_SIZE(testAssetBuffer), 0);
    constexpr bool isDataBigEndian = false;
    size_t testAssetBytesWritten = AZ::AssetSerializer::s_serializer.Save(&testAsset, testAssetStream, isDataBigEndian);
    // Make sure the amount written matches the expected size (same *size* as our test v2 data, just different data in it)
    EXPECT_EQ(testAssetBytesWritten, AZ_ARRAY_SIZE(m_testSerializedAssetVersion2));

    // Create an arbitrarily large memory stream to hold our converted data, and initialize to bad values.
    char memBuffer[0x1000];
    memset(memBuffer, 'X', AZ_ARRAY_SIZE(memBuffer));
    AZ::IO::MemoryStream memStream(memBuffer, AZ_ARRAY_SIZE(memBuffer), 0);

    // Convert a version 1 text string to data
    constexpr unsigned int version = 1;
    size_t bytesWritten = AZ::AssetSerializer::s_serializer.TextToData(m_testAssetStringVersion1.c_str(),
                                                                       version, memStream, isDataBigEndian);

    // Make sure the result value matches the actual amount written into the stream
    EXPECT_EQ(bytesWritten, memStream.GetLength());
    // Make sure the amount written matches the expected size
    EXPECT_EQ(bytesWritten, testAssetBytesWritten);
    // Make sure the data written matches the expected data
    EXPECT_EQ(0, memcmp(testAssetBuffer, memBuffer, testAssetBytesWritten));
}

TEST_F(AssetSerializerTest, TextToData_ConvertVersion2Data_ConversionSuccessful)
{
    // Create an arbitrarily large memory stream to hold our converted data, and initialize to bad values.
    char memBuffer[0x1000];
    memset(memBuffer, 'X', AZ_ARRAY_SIZE(memBuffer));
    AZ::IO::MemoryStream memStream(memBuffer, AZ_ARRAY_SIZE(memBuffer), 0);

    // Convert a version 2 text string to data
    constexpr bool isDataBigEndian = false;
    constexpr unsigned int version = 2;
    size_t bytesWritten = AZ::AssetSerializer::s_serializer.TextToData(m_testAssetStringVersion2.c_str(),
        version, memStream, isDataBigEndian);

    // Make sure the result value matches the actual amount written into the stream
    EXPECT_EQ(bytesWritten, memStream.GetLength());
    // Make sure the amount written matches the expected size
    EXPECT_EQ(bytesWritten, AZ_ARRAY_SIZE(m_testSerializedAssetVersion2));
    // Make sure the data written matches the expected data
    EXPECT_EQ(0, memcmp(m_testSerializedAssetVersion2, memBuffer, AZ_ARRAY_SIZE(m_testSerializedAssetVersion2)));
}

TEST_F(AssetSerializerTest, Clone_CloneValidAsset_CloneSuccessful)
{
    // Create an expected asset with an arbitrary set of expected values.
    AZ::Data::Asset<AZ::Data::AssetData> expectedAsset(m_testAssetId, m_testAssetType, m_testAssetHint);
    expectedAsset.SetAutoLoadBehavior(m_testAssetLoadBehavior);

    // Create an empty test asset
    AZ::Data::Asset<AZ::Data::AssetData> testAsset;

    // Verify that the empty asset doesn't match before the Clone, and does match afterwards.
    EXPECT_NE(expectedAsset, testAsset);
    AZ::AssetSerializer::s_serializer.Clone(&expectedAsset, &testAsset);
    EXPECT_EQ(expectedAsset, testAsset);
}

TEST_F(AssetSerializerTest, CompareValueData_CompareDifferentAssets_CompareReturnsFalse)
{
    // Create an expected asset with an arbitrary set of expected values.
    AZ::Data::Asset<AZ::Data::AssetData> expectedAsset(m_testAssetId, m_testAssetType, m_testAssetHint);
    expectedAsset.SetAutoLoadBehavior(m_testAssetLoadBehavior);

    // Create an empty test asset
    AZ::Data::Asset<AZ::Data::AssetData> testAsset;

    EXPECT_FALSE(AZ::AssetSerializer::s_serializer.CompareValueData(&expectedAsset, &testAsset));
}

TEST_F(AssetSerializerTest, CompareValueData_CompareEquivalentAssets_CompareReturnsTrue)
{
    // Create an expected asset with an arbitrary set of expected values.
    AZ::Data::Asset<AZ::Data::AssetData> expectedAsset(m_testAssetId, m_testAssetType, m_testAssetHint);
    expectedAsset.SetAutoLoadBehavior(m_testAssetLoadBehavior);

    // Create a second asset with the same data
    AZ::Data::Asset<AZ::Data::AssetData> testAsset(m_testAssetId, m_testAssetType, m_testAssetHint);
    expectedAsset.SetAutoLoadBehavior(m_testAssetLoadBehavior);

    EXPECT_TRUE(AZ::AssetSerializer::s_serializer.CompareValueData(&expectedAsset, &testAsset));
}
