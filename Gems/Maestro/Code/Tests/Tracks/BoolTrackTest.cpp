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
#include "Maestro_precompiled.h"

#include <AzTest/AzTest.h>
#include <AnimKey.h>
#include <Maestro/Types/AssetBlendKey.h>
#include <Cinematics/BoolTrack.h>


const float KEY_TIME = 1.0f;

class CBoolTrackTest
    : public ::testing::Test
{
public:
    CBoolTrackTest() {}

    void SetUp()
    {
        m_complexBoolTrack.SetDefaultValue(false);
        int keyIndex = 0;

        //Single Key, track default TRUE
        m_singleKey.time = KEY_TIME;
        m_singleKeyBoolTrack.CreateKey(m_singleKey.time);
        m_singleKeyBoolTrack.SetKey(keyIndex, &m_singleKey);

        //Key 1, track default FALSE
        m_key1.time = KEY_TIME;
        m_complexBoolTrack.CreateKey(m_key1.time);
        m_complexBoolTrack.SetKey(keyIndex++, &m_key1);

        //Key 2, track default FALSE
        m_key2.time = KEY_TIME * 2.0f;
        m_complexBoolTrack.CreateKey(m_key2.time);
        m_complexBoolTrack.SetKey(keyIndex++, &m_key2);

        //Key 3, track default FALSE
        m_key3.time = KEY_TIME * 5.0f;
        m_complexBoolTrack.CreateKey(m_key3.time);
        m_complexBoolTrack.SetKey(keyIndex++, &m_key3);
    }

    CBoolTrack m_emptyBoolTrack;
    CBoolTrack m_singleKeyBoolTrack;
    CBoolTrack m_complexBoolTrack;

    AZ::IAssetBlendKey m_singleKey;
    AZ::IAssetBlendKey m_key1;
    AZ::IAssetBlendKey m_key2;
    AZ::IAssetBlendKey m_key3;
};

TEST_F(CBoolTrackTest, GetValue_NoKeys_ExpectDefault) 
{
    bool result = false;
    m_emptyBoolTrack.GetValue(KEY_TIME, result);

    //default value of m_emptyBoolTrack is true
    EXPECT_TRUE(result) << "The track is not at default value, even though there are no keys";
}

TEST_F(CBoolTrackTest, GetValue_OneKey_BeforeKey_ExpectDefault) 
{
    bool result = false;
    m_singleKeyBoolTrack.GetValue(KEY_TIME - 0.5f, result);

    //default value of m_singleKeyBoolTrack is true
    EXPECT_TRUE(result) << "The track is not at default value, even though no keys have been hit yet";
}

TEST_F(CBoolTrackTest, GetValue_OneKey_AfterKey_ExpectNotDefault) 
{
    bool result = false;
    m_singleKeyBoolTrack.GetValue(KEY_TIME, result);

    //default value of m_singleKeyBoolTrack is true
    EXPECT_FALSE(result) << "Hitting a Key did not change the default value";
}

TEST_F(CBoolTrackTest, GetValue_EvenKeys_ExpectDefault) 
{
    bool result = false;

    m_complexBoolTrack.GetValue(m_key1.time, result);
    EXPECT_TRUE(result);

    m_complexBoolTrack.GetValue(m_key2.time, result);
    //default value of m_complexBoolTrack is false
    EXPECT_FALSE(result);
}

TEST_F(CBoolTrackTest, GetValue_OddKeys_ExpectNotDefault) 
{
    bool result = false;
    m_complexBoolTrack.GetValue(m_key3.time, result);
    //default value of m_complexBoolTrack is false
    EXPECT_TRUE(result);
}

TEST_F(CBoolTrackTest, SetValue_SetDefault_ExpectChange)
{
    m_emptyBoolTrack.SetValue(0.0f, /*value*/ false, /*set default?*/ true);

    bool result = true;
    m_emptyBoolTrack.GetValue(0.0f, result);
    EXPECT_FALSE(result);
}

TEST_F(CBoolTrackTest, SetValue_DoNotSetDefault_NoChange) 
{
    m_singleKeyBoolTrack.SetValue(0.0f, /*value*/ false, /*set default?*/ false);

    bool result = false;
    m_singleKeyBoolTrack.GetValue(0.0f, result);
    EXPECT_TRUE(result);
}
