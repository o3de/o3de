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

#include <AzTest/AzTest.h>
#include "ZipEncryptor.h"

TEST(ResourceCompilerZipEncryptorTest, ParseKey_InputTooShort_ReturnFalse_FT)
{
    uint32 key[4];
    EXPECT_FALSE(ZipEncryptor::ParseKey(key, "Not32HexCharacters"));
}

TEST(ResourceCompilerZipEncryptorTest, ParseKey_ValidKey_ReturnTrue_FT)
{
    uint32 key[4];
    EXPECT_TRUE(ZipEncryptor::ParseKey(key, "123456789012345678901234567890AB"));
    EXPECT_EQ(key[0], 1450741931);
    EXPECT_EQ(key[1], 2022707764);
    EXPECT_EQ(key[2], 2417112150);
    EXPECT_EQ(key[3], 305419896);
}

TEST(ResourceCompilerZipEncryptorTest, ParseKey_InvalidInputV1_ReturnFalse_FT)
{
    uint32 key[4];
    EXPECT_FALSE(ZipEncryptor::ParseKey(key, "1234567890Z1345678901234567890AB"));
    // Error text will be @Pos 11                       ^ HERE. Invalid Hex value
}

TEST(ResourceCompilerZipEncryptorTest, ParseKey_InvalidInputV2_ReturnFalse_FT)
{
    uint32 key[4];
    EXPECT_FALSE(ZipEncryptor::ParseKey(key, "12345678901Z345678901234567890AB"));
    // Error text will be @Pos 12                        ^ HERE. Invalid Hex value
}
