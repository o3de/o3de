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
#include "CrySystem_precompiled.h"
#include <AzTest/AzTest.h>
#include <AzCore/Memory/AllocatorScope.h>
#include <CryCommon/stl/STLAlignedAlloc.h>

TEST(StringTests, CUT_Strings)
{
    bool bOk;
    char bf[4];

    // cry_strcpy()

    bOk = cry_strcpy(0, 0, 0);
    EXPECT_TRUE(!bOk);

    bOk = cry_strcpy(0, 0, 0, 0);
    EXPECT_TRUE(!bOk);

    bOk = cry_strcpy(0, 1, 0);
    EXPECT_TRUE(!bOk);

    bOk = cry_strcpy(0, 1, 0, 1);
    EXPECT_TRUE(!bOk);

    bOk = cry_strcpy(0, 1, "");
    EXPECT_TRUE(!bOk);

    bOk = cry_strcpy(0, 1, "", 1);
    EXPECT_TRUE(!bOk);

    memcpy(bf, "abcd", 4);
    bOk = cry_strcpy(bf, 0, "");
    EXPECT_TRUE(!bOk && !memcmp(bf, "abcd", 4));

    memcpy(bf, "abcd", 4);
    bOk = cry_strcpy(bf, 0, "", 1);
    EXPECT_TRUE(!bOk && !memcmp(bf, "abcd", 4));

    memcpy(bf, "abcd", 4);
    bOk = cry_strcpy(bf, 1, 0);
    EXPECT_TRUE(!bOk && !memcmp(bf, "\000bcd", 4));

    memcpy(bf, "abcd", 4);
    bOk = cry_strcpy(bf, 1, 0, 0);
    EXPECT_TRUE(!bOk && !memcmp(bf, "\000bcd", 4));

    memcpy(bf, "abcd", 4);
    bOk = cry_strcpy(bf, 0);
    EXPECT_TRUE(!bOk && !memcmp(bf, "\000bcd", 4));

    memcpy(bf, "abcd", 4);
    bOk = cry_strcpy(bf, 3, "qwerty");
    EXPECT_TRUE(!bOk && !memcmp(bf, "qw\000d", 4));

    memcpy(bf, "abcd", 4);
    bOk = cry_strcpy(bf, 3, "qwerty", 4);
    EXPECT_TRUE(!bOk && !memcmp(bf, "qw\000d", 4));

    memcpy(bf, "abcd", 4);
    bOk = cry_strcpy(bf, 3, "qwerty", 3);
    EXPECT_TRUE(!bOk && !memcmp(bf, "qw\000d", 4));

    memcpy(bf, "abcd", 4);
    bOk = cry_strcpy(bf, 3, "qwerty", 2);
    EXPECT_TRUE(bOk && !memcmp(bf, "qw\000d", 4));

    memcpy(bf, "abcd", 4);
    bOk = cry_strcpy(bf, 3, "qwerty", 1);
    EXPECT_TRUE(bOk && !memcmp(bf, "q\000cd", 4));

    memcpy(bf, "abcd", 4);
    bOk = cry_strcpy(bf, 3, "qwerty", 0);
    EXPECT_TRUE(bOk && !memcmp(bf, "\000bcd", 4));

    memcpy(bf, "abcd", 4);
    bOk = cry_strcpy(bf, "qwerty");
    EXPECT_TRUE(!bOk && !memcmp(bf, "qwe\000", 4));

    memcpy(bf, "abcd", 4);
    bOk = cry_strcpy(bf, "qwerty", 4);
    EXPECT_TRUE(!bOk && !memcmp(bf, "qwe\000", 4));

    memcpy(bf, "abcd", 4);
    bOk = cry_strcpy(bf, "qwerty", 3);
    EXPECT_TRUE(bOk && !memcmp(bf, "qwe\000", 4));

    memcpy(bf, "abcd", 4);
    bOk = cry_strcpy(bf, "qwerty", 2);
    EXPECT_TRUE(bOk && !memcmp(bf, "qw\000d", 4));

    memcpy(bf, "abcd", 4);
    bOk = cry_strcpy(bf, "qwe");
    EXPECT_TRUE(bOk && !memcmp(bf, "qwe\000", 4));

    memcpy(bf, "abcd", 4);
    bOk = cry_strcpy(bf, "qwe", 4);
    EXPECT_TRUE(bOk && !memcmp(bf, "qwe\000", 4));

    memcpy(bf, "abcd", 4);
    bOk = cry_strcpy(bf, "qw", 3);
    EXPECT_TRUE(bOk && !memcmp(bf, "qw\000d", 4));

    memcpy(bf, "abcd", 4);
    bOk = cry_strcpy(bf, sizeof(bf), "q");
    EXPECT_TRUE(bOk && !memcmp(bf, "q\000cd", 4));

    memcpy(bf, "abcd", 4);
    bOk = cry_strcpy(bf, sizeof(bf), "q", 2);
    EXPECT_TRUE(bOk && !memcmp(bf, "q\000cd", 4));

    // cry_strcat()

    bOk = cry_strcat(0, 0, 0);
    EXPECT_TRUE(!bOk);

    bOk = cry_strcat(0, 0, 0, 0);
    EXPECT_TRUE(!bOk);

    bOk = cry_strcat(0, 1, 0);
    EXPECT_TRUE(!bOk);

    bOk = cry_strcat(0, 1, 0, 0);
    EXPECT_TRUE(!bOk);

    bOk = cry_strcat(0, 1, "");
    EXPECT_TRUE(!bOk);

    bOk = cry_strcat(0, 1, "", 1);
    EXPECT_TRUE(!bOk);

    memcpy(bf, "abcd", 4);
    bOk = cry_strcat(bf, 0, "xy");
    EXPECT_TRUE(!bOk && !memcmp(bf, "abcd", 4));

    memcpy(bf, "abcd", 4);
    bOk = cry_strcat(bf, 0, "xy", 3);
    EXPECT_TRUE(!bOk && !memcmp(bf, "abcd", 4));

    memcpy(bf, "abcd", 4);
    bOk = cry_strcat(bf, 0, "xy", 0);
    EXPECT_TRUE(!bOk && !memcmp(bf, "abcd", 4));

    memcpy(bf, "abcd", 4);
    bOk = cry_strcat(bf, 1, "xyz");
    EXPECT_TRUE(!bOk && !memcmp(bf, "\000bcd", 4));

    memcpy(bf, "abcd", 4);
    bOk = cry_strcat(bf, 1, "xyz", 4);
    EXPECT_TRUE(!bOk && !memcmp(bf, "\000bcd", 4));

    memcpy(bf, "abcd", 4);
    bOk = cry_strcat(bf, 1, "xyz", 1);
    EXPECT_TRUE(!bOk && !memcmp(bf, "\000bcd", 4));

    memcpy(bf, "abcd", 4);
    bOk = cry_strcat(bf, 1, "xyz", 0);
    EXPECT_TRUE(bOk && !memcmp(bf, "\000bcd", 4));

    memcpy(bf, "abcd", 4);
    bOk = cry_strcat(bf, 1, 0, 0);
    EXPECT_TRUE(!bOk && !memcmp(bf, "\000bcd", 4));

    memcpy(bf, "a\000cd", 4);
    bOk = cry_strcat(bf, 3, "xyz");
    EXPECT_TRUE(!bOk && !memcmp(bf, "ax\000d", 4));

    memcpy(bf, "a\000cd", 4);
    bOk = cry_strcat(bf, 3, "xyz", 4);
    EXPECT_TRUE(!bOk && !memcmp(bf, "ax\000d", 4));

    memcpy(bf, "a\000cd", 4);
    bOk = cry_strcat(bf, 3, "xyz", 2);
    EXPECT_TRUE(!bOk && !memcmp(bf, "ax\000d", 4));

    memcpy(bf, "a\000cd", 4);
    bOk = cry_strcat(bf, 3, "xyz", 1);
    EXPECT_TRUE(bOk && !memcmp(bf, "ax\000d", 4));

    memcpy(bf, "abcd", 4);
    bOk = cry_strcat(bf, "xyz");
    EXPECT_TRUE(!bOk && !memcmp(bf, "abc\000", 4));

    memcpy(bf, "abcd", 4);
    bOk = cry_strcat(bf, "xyz", 4);
    EXPECT_TRUE(!bOk && !memcmp(bf, "abc\000", 4));

    memcpy(bf, "abcd", 4);
    bOk = cry_strcat(bf, "xyz", 1);
    EXPECT_TRUE(!bOk && !memcmp(bf, "abc\000", 4));

    memcpy(bf, "abcd", 4);
    bOk = cry_strcat(bf, "xyz", 0);
    EXPECT_TRUE(bOk && !memcmp(bf, "abc\000", 4));

    memcpy(bf, "ab\000d", 4);
    bOk = cry_strcat(bf, "xyz");
    EXPECT_TRUE(!bOk && !memcmp(bf, "abx\000", 4));

    memcpy(bf, "ab\000d", 4);
    bOk = cry_strcat(bf, "xyz", 4);
    EXPECT_TRUE(!bOk && !memcmp(bf, "abx\000", 4));

    memcpy(bf, "ab\000d", 4);
    bOk = cry_strcat(bf, "xyz", 1);
    EXPECT_TRUE(bOk && !memcmp(bf, "abx\000", 4));

    memcpy(bf, "ab\000d", 4);
    bOk = cry_strcat(bf, "xyz", 0);
    EXPECT_TRUE(bOk && !memcmp(bf, "ab\000d", 4));

    memcpy(bf, "ab\000d", 4);
    bOk = cry_strcat(bf, 0, 0);
    EXPECT_TRUE(!bOk && !memcmp(bf, "ab\000d", 4));

    memcpy(bf, "ab\000d", 4);
    bOk = cry_strcat(bf, 0, 1);
    EXPECT_TRUE(!bOk && !memcmp(bf, "ab\000d", 4));

    memcpy(bf, "a\000cd", 4);
    bOk = cry_strcat(bf, sizeof(bf), "xy");
    EXPECT_TRUE(bOk && !memcmp(bf, "axy\000", 4));

    memcpy(bf, "a\000cd", 4);
    bOk = cry_strcat(bf, sizeof(bf), "xy", 3);
    EXPECT_TRUE(bOk && !memcmp(bf, "axy\000", 4));

    memcpy(bf, "a\000cd", 4);
    bOk = cry_strcat(bf, sizeof(bf), "xy", 1);
    EXPECT_TRUE(bOk && !memcmp(bf, "ax\000d", 4));
}

using CryPrimitivesAllocatorScope = AZ::AllocatorScope<AZ::LegacyAllocator, CryStringAllocator>;

class CryPrimitives
    : public ::testing::Test
{
public:
    void SetUp() override
    {
        m_memory.ActivateAllocators();
    }

    void TearDown() override
    {
        m_memory.DeactivateAllocators();
    }

    CryPrimitivesAllocatorScope m_memory;
};

TEST_F(CryPrimitives, CUT_CryString)
{
    //////////////////////////////////////////////////////////////////////////
    // Based on MS documentation of find_last_of
    string strTestFindLastOfOverload1("abcd-1234-abcd-1234");
    string strTestFindLastOfOverload2("ABCD-1234-ABCD-1234");
    string strTestFindLastOfOverload3("456-EFG-456-EFG");
    string strTestFindLastOfOverload4("12-ab-12-ab");

    const char* cstr2 = "B1";
    const char* cstr2b = "D2";
    const char* cstr3a = "5E";
    string str4a("ba3");
    string str4b("a2");

    size_t nPosition(string::npos);

    nPosition = strTestFindLastOfOverload1.find_last_of('d', 14);
    EXPECT_TRUE(nPosition == 13);


    nPosition = strTestFindLastOfOverload2.find_last_of(cstr2, 12);
    EXPECT_TRUE(nPosition == 11);


    nPosition = strTestFindLastOfOverload2.find_last_of(cstr2b);
    EXPECT_TRUE(nPosition == 16);


    nPosition = strTestFindLastOfOverload3.find_last_of(cstr3a, 8, 2);
    EXPECT_TRUE(nPosition == 4);


    nPosition = strTestFindLastOfOverload4.find_last_of(str4a, 8);
    EXPECT_TRUE(nPosition == 4);


    nPosition = strTestFindLastOfOverload4.find_last_of(str4b);
    EXPECT_TRUE(nPosition == 9);

    //////////////////////////////////////////////////////////////////////////
    // Based on MS documentation of find_last_not_of
    string strTestFindLastNotOfOverload1("dddd-1dd4-abdd");
    string strTestFindLastNotOfOverload2("BBB-1111");
    string strTestFindLastNotOfOverload3("444-555-GGG");
    string strTestFindLastNotOfOverload4("12-ab-12-ab");

    const char* cstr2NF = "B1";
    const char* cstr3aNF = "45G";
    const char* cstr3bNF = "45G";

    string str4aNF("b-a");
    string str4bNF("12");

    nPosition = strTestFindLastNotOfOverload1.find_last_not_of('d', 7);
    EXPECT_TRUE(nPosition == 5);

    nPosition = strTestFindLastNotOfOverload1.find_last_not_of("d");
    EXPECT_TRUE(nPosition == 11);

    nPosition = strTestFindLastNotOfOverload2.find_last_not_of(cstr2NF, 6);
    EXPECT_TRUE(nPosition == 3);

    nPosition = strTestFindLastNotOfOverload3.find_last_not_of(cstr3aNF);
    EXPECT_TRUE(nPosition == 7);

    nPosition = strTestFindLastNotOfOverload3.find_last_not_of(cstr3bNF, 6, 3);//nPosition - 1 );
    EXPECT_TRUE(nPosition == 3);

    nPosition = strTestFindLastNotOfOverload4.find_last_not_of(str4aNF, 5);
    EXPECT_TRUE(nPosition == 1);

    nPosition = strTestFindLastNotOfOverload4.find_last_not_of(str4bNF);
    EXPECT_TRUE(nPosition == 10);
}


TEST_F(CryPrimitives, CUT_FixedString)
{
    CryStackStringT<char, 10> str1;
    CryStackStringT<char, 10> str2;
    CryStackStringT<char, 4> str3;
    CryStackStringT<char, 10> str4;
    CryStackStringT<char, 6> str5;
    CryStackStringT<wchar_t, 16> wstr1;
    CryStackStringT<wchar_t, 255> wstr2;
    CryFixedStringT<100> fixedString100;
    CryFixedStringT<200> fixedString200;

    typedef CryStackStringT<char, 10> T;
    T* pStr = new T;
    *pStr = "adads";
    delete pStr;

    str1 = "abcd";
    EXPECT_EQ(str1, "abcd");

    str2 = "efg";
    EXPECT_EQ(str2, "efg");

    str2 = str1;
    EXPECT_EQ(str2, "abcd");

    str1 += "XY";
    EXPECT_EQ(str1, "abcdXY");

    str2 += "efghijk";
    EXPECT_EQ(str2, "abcdefghijk");

    str1.replace("bc", "");
    EXPECT_EQ(str1, "adXY");

    str1.replace("XY", "1234");
    EXPECT_EQ(str1, "ad1234");

    str1.replace("1234", "1234567890");
    EXPECT_EQ(str1, "ad1234567890");

    str1.reserve(200);
    EXPECT_EQ(str1, "ad1234567890");
    EXPECT_TRUE(str1.capacity() == 200);

    str1.reserve(0);
    EXPECT_EQ(str1, "ad1234567890");
    EXPECT_TRUE(str1.capacity() == str1.length());

    str1.erase(7); // doesn't change capacity
    EXPECT_EQ(str1, "ad12345");

    str4.assign("abc");
    EXPECT_EQ(str4, "abc");
    str4.reserve(9);
    EXPECT_TRUE(str4.capacity() >= 9); // capacity is always >= MAX_SIZE-1
    str4.reserve(0);
    EXPECT_TRUE(str4.capacity() >= 9); // capacity is always >= MAX_SIZE-1

    size_t idx = str1.find("123");
    EXPECT_TRUE(idx == 2);

    idx = str1.find("123", 3);
    EXPECT_TRUE(idx == str1.npos);

    wstr1 = L"abc";
    EXPECT_EQ(wstr1, L"abc");
    EXPECT_TRUE(wstr1.compare(L"aBc") > 0);
    EXPECT_TRUE(wstr1.compare(L"babc") < 0);
    EXPECT_TRUE(wstr1.compareNoCase(L"aBc") == 0);

    str1.Format("This is a %s %ls with %d params", "mixed", L"string", 3);
    str2.Format("This is a %ls %s with %d params", L"mixed", "string", 3);
    EXPECT_EQ(str1, "This is a mixed string with 3 params");
    EXPECT_EQ(str1, str2);

    wstr1.Format(L"This is a %ls %hs with %d params", L"mixed", "string", 3);
    wstr2.Format(L"This is a %hs %ls with %d params", "mixed", L"string", 3);
    EXPECT_EQ(wstr1, L"This is a mixed string with 3 params");

    str5.FormatFast("%s", "12345");
    EXPECT_EQ("1234", str5);

    // we expect here that the string gets cut since it doesn't fit into the string buffer
    str5.FormatFast("%s", "012345");
    EXPECT_EQ("0123", str5);
}

//////////////////////////////////////////////////////////////////////////
// Unit Testing of aligned_vector
//////////////////////////////////////////////////////////////////////////
TEST_F(CryPrimitives, CUT_AlignedVector)
{
    stl::aligned_vector<int, 16> vec;

    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    EXPECT_TRUE(vec.size() == 3);
    EXPECT_TRUE(((INT_PTR)(&vec[0]) % 16) == 0);
}

TEST_F(CryPrimitives, CUT_DynArray)
{
    LegacyDynArray<int> a;
    a.push_back(3);
    a.insert(&a[0], 1, 1);
    a.insert(&a[1], 1, 2);
    a.insert(&a[0], 1, 0);

    for (int i = 0; i < 4; i++)
    {
        EXPECT_TRUE(a[i] == i);
    }

    const int nStrs = 11;
    string Strs[nStrs] = { "nought", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten" };
    LegacyDynArray<string> s;
    for (int i = 0; i < nStrs; i += 2)
    {
        s.push_back(Strs[i]);
    }
    for (int i = 1; i < nStrs; i += 2)
    {
        s.insert(i, Strs[i]);
    }
    for (int i = 0; i < nStrs; i++)
    {
        EXPECT_TRUE(s[i] == Strs[i]);
    }

    LegacyDynArray<string> s2 = s;
    s.erase(5, 2);
    EXPECT_TRUE(s.size() == nStrs - 2);

    s.insert(&s[3], &Strs[5], &Strs[8]);

    s2 = s2(3, 4);
    EXPECT_TRUE(s2.size() == 4);
}
