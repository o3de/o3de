/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/std/containers/bitset.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <type_traits>
#include <bitset> // Used for comparison tests.

#include "UserTypes.h"


namespace UnitTest
{
    // Or the bitset can be tested without respect to the unsigned long, but benefit from having multiple test cases exercised through the parameterized test
    class BitsetUnsignedLongTests
        : public ::testing::WithParamInterface <unsigned long>
        , public UnitTest::ScopedAllocatorSetupFixture
    {

    protected:
        void SetUp() override
        {
            m_unsignedLong = GetParam();
            m_bitset = AZStd::bitset<32>(m_unsignedLong);
        }

        // The ground truth
        AZ::u32 m_unsignedLong;
        // The unit under test
        AZStd::bitset<32> m_bitset;
    };

    TEST_P(BitsetUnsignedLongTests, UnsignedLongConstructor_MatchesUnsignedLong)
    {
        // Bit by bit comparison
        unsigned long currentBit = 1;
        for (size_t i = 0; i < m_bitset.size(); ++i)
        {
            // Expect each bit of the bitset which was constructed during the test fixture SetUp to match the bits in the unsigned long
            EXPECT_EQ(m_bitset[i], (m_unsignedLong & currentBit) != 0) << "The bit at index " << i << " did not match the corresponding bit in the unsigned long";
            currentBit <<= 1;
        }
    }

    TEST_P(BitsetUnsignedLongTests, ToUlong_MatchesUnsignedLong)
    {
        EXPECT_EQ(m_bitset.to_ulong(), m_unsignedLong);
    }

    TEST_P(BitsetUnsignedLongTests, BitwiseNot_MatchesUnsignedLong)
    {
        m_bitset = ~m_bitset;
        EXPECT_EQ(m_bitset.to_ulong(), ~m_unsignedLong);
    }

    TEST_P(BitsetUnsignedLongTests, Reset_EachBitReset_EachBitIsFalse)
    {
        // Iterate over the entire bitset and reset each bit individually
        for (size_t i = 0; i < m_bitset.size(); ++i)
        {
            m_bitset.reset(i);
            EXPECT_FALSE(m_bitset[i]) << "The bit at index " << i << " was not reset to 0";
        }
    }

    TEST_P(BitsetUnsignedLongTests, Flip_EachBitFlipped_MatchesFlippedUnsignedLong)
    {
        // Iterate over the entire bitset and flip each bit individually
        for (size_t i = 0; i < m_bitset.size(); ++i)
        {
            m_bitset.flip(i);
        }

        // Compare to the flipped unsigned long
        EXPECT_EQ(m_bitset.to_ulong(), ~m_unsignedLong);
    }

    TEST_P(BitsetUnsignedLongTests, Reference_EachBitFlipped_MatchesFlippedUnsignedLong)
    {
        // Iterate over the entire bitset and flip each bit individually via a bitset reference
        for (size_t i = 0; i < m_bitset.size(); ++i)
        {
            AZStd::bitset<32>::reference ref = m_bitset[i];
            ref.flip();
        }

        // Compare to the flipped unsigned long
        EXPECT_EQ(m_bitset.to_ulong(), ~m_unsignedLong);
    }

    TEST_P(BitsetUnsignedLongTests, Reference_BitwiseNotForEachBit_EachBitDoesNotEqualOriginalValue)
    {
        for (size_t i = 0; i < m_bitset.size(); ++i)
        {
            // Get a reference to a bit
            AZStd::bitset<32>::reference ref = m_bitset[i];
            // Compare the reference to the original value of the bit
            EXPECT_NE(m_bitset[i], ~ref) << "The ~ operator did not negate the reference to the bitset at index " << i << ".";
        }
    }

    // This fixture initializes two bitsets from two unsigned longs
    // Bitwise operations can be performed between the bitsets, then compared to the same bitwise operations preformed between the unsigned longs
    class BitsetUnsignedLongPairTests
        : public ::testing::WithParamInterface < AZStd::pair<unsigned long, unsigned long> >
        , public UnitTest::ScopedAllocatorSetupFixture
    {

    protected:

        void SetUp() override
        {
            m_unsignedLong1 = GetParam().first;
            m_bitset1 = AZStd::bitset<32>(m_unsignedLong1);

            m_unsignedLong2 = GetParam().second;
            m_bitset2 = AZStd::bitset<32>(m_unsignedLong2);
        }

        unsigned long m_unsignedLong1;
        unsigned long m_unsignedLong2;

        AZStd::bitset<32> m_bitset1;
        AZStd::bitset<32> m_bitset2;
    };

    TEST_P(BitsetUnsignedLongPairTests, BitwiseANDOperator_MatchesUnsignedLongAND)
    {
        EXPECT_EQ((m_bitset1 & m_bitset2).to_ulong(), m_unsignedLong1 & m_unsignedLong2);
    }

    TEST_P(BitsetUnsignedLongPairTests, BitwiseOROperator_MatchesUnsignedLongOR)
    {
        EXPECT_EQ((m_bitset1 | m_bitset2).to_ulong(), m_unsignedLong1 | m_unsignedLong2);
    }

    TEST_P(BitsetUnsignedLongPairTests, BitwiseXOROperator_MatchesUnsignedLongXOR)
    {
        EXPECT_EQ((m_bitset1 ^ m_bitset2).to_ulong(), m_unsignedLong1 ^ m_unsignedLong2);
    }

    TEST_P(BitsetUnsignedLongPairTests, BitwiseANDAssignmentOperator_MatchesUnsignedLongANDAssignment)
    {
        m_bitset1 &= m_bitset2;
        m_unsignedLong1 &= m_unsignedLong2;

        EXPECT_EQ(m_bitset1.to_ulong(), m_unsignedLong1);
    }

    TEST_P(BitsetUnsignedLongPairTests, BitwiseORAssignmentOperator_MatchesUnsignedLongORAssignment)
    {
        m_bitset1 |= m_bitset2;
        m_unsignedLong1 |= m_unsignedLong2;

        EXPECT_EQ(m_bitset1.to_ulong(), m_unsignedLong1);
    }

    TEST_P(BitsetUnsignedLongPairTests, BitwiseXORAssignmentOperator_MatchesUnsignedLongXORAssignment)
    {
        m_bitset1 ^= m_bitset2;
        m_unsignedLong1 ^= m_unsignedLong2;

        EXPECT_EQ(m_bitset1.to_ulong(), m_unsignedLong1);
    }

    TEST_P(BitsetUnsignedLongPairTests, EqualOperator_ComparedToOtherBitset_MatchesUnsignedLongComparison)
    {
        // The == comparison between the bitsets should have the same result as the == comparison between the unsigned longs
        EXPECT_EQ(m_bitset1 == m_bitset2, m_unsignedLong1 == m_unsignedLong2);
    }

    TEST_P(BitsetUnsignedLongPairTests, NotEqualOperator_ComparedToOtherBitset_MatchesUnsignedLongComparison)
    {
        // The != comparison between the bitsets should have the same result as the != comparison between the unsigned longs
        EXPECT_EQ(m_bitset1 != m_bitset2, m_unsignedLong1 != m_unsignedLong2);
    }

    TEST_P(BitsetUnsignedLongPairTests, CopyConstructor_CopyOtherBitset_EqualsOriginal)
    {
        m_bitset2 = m_bitset1;
        EXPECT_EQ(m_bitset1, m_bitset2);
    }

    TEST_P(BitsetUnsignedLongPairTests, ReferenceAssignmentOperator_AssignBoolToReferenceForEachBit_ReferencedBitsetMatchesOriginal)
    {
        // Iterate over the entire m_bitset1 and assign its values to m_bitset2 bit by bit
        for (size_t i = 0; i < m_bitset1.size(); ++i)
        {
            // Assign a bool value to the reference
            bool bitValue = m_bitset1[i];
            AZStd::bitset<32>::reference bitset2Ref = m_bitset2[i];
            bitset2Ref = bitValue;
            EXPECT_EQ(m_bitset1[i], m_bitset2[i]) << "The value of the bit at index " << i << " from m_bitset1 was not assigned to the bit at index " << i << " of m_bitset2.";
        }

        EXPECT_EQ(m_bitset1, m_bitset2);
    }

    TEST_P(BitsetUnsignedLongPairTests, ReferenceAssignmentOperator_AssignReferenceToReferenceForEachBit_ReferencedBitsetMatchesOriginal)
    {
        // Iterate over the entire m_bitset1 and assign its values to m_bitset2 bit by bit
        for (size_t i = 0; i < m_bitset1.size(); ++i)
        {
            // Assign a bitset reference value to the reference
            AZStd::bitset<32>::reference bitset1Ref = m_bitset1[i];
            AZStd::bitset<32>::reference bitset2Ref = m_bitset2[i];
            bitset2Ref = bitset1Ref;
            EXPECT_EQ(m_bitset1[i], m_bitset2[i]) << "The value of the bit at index " << i << " from m_bitset1 was not assigned to the bit at index " << i << " of m_bitset2.";
        }

        EXPECT_EQ(m_bitset1, m_bitset2);
    }

    // This class initializes an AZStd::bitset and an std::bitset with the same value to validate that the behavior of AZStd::bitset matches the behavior of std::bitset
    // It should only be used to test functions where we want AZStd::bitset to conform to the behavior of std::bitset
    // It is useful for testing functions where operations on the bitset behave differently than the same operations on an unsigned long,
    // such as shifting bits beyond the length of the bitset
    class BitsetStdComparisonTests
        : public ::testing::WithParamInterface <unsigned long>
        , public UnitTest::ScopedAllocatorSetupFixture
    {

    protected:
        void SetUp() override
        {
            // Initialize the bitsets from an unsigned long
            m_stdBitset = std::bitset<32>(GetParam());
            m_bitset = AZStd::bitset<32>(GetParam());
        }

        // The ground truth for the unit test
        std::bitset<32> m_stdBitset;
        // The unit under test
        AZStd::bitset<32> m_bitset;
        // An array of values to use when shifting bits
        AZStd::vector<AZStd::size_t> m_shiftValues = { 0,1,2,3,5,8,13,21,44 };
    };

    TEST_P(BitsetStdComparisonTests, RightShift_MatchesStd)
    {
        for (AZStd::size_t value : m_shiftValues)
        {
            EXPECT_EQ((m_bitset >> value).to_ulong(), (m_stdBitset >> value).to_ulong()) << "Right shift by " << value << " bits did not match std::bitset";
        }
    }

    TEST_P(BitsetStdComparisonTests, RightShiftAssignment_MatchesStd)
    {
        for (AZStd::size_t value : m_shiftValues)
        {
            m_bitset >>= value;
            m_stdBitset >>= value;
            EXPECT_EQ(m_bitset.to_ulong(), m_stdBitset.to_ulong()) << "Right shift assignment by " << value << " bits did not match std::bitset.";
        }
    }

    TEST_P(BitsetStdComparisonTests, LeftShift_MatchesStd)
    {
        for (AZStd::size_t value : m_shiftValues)
        {
            EXPECT_EQ((m_bitset << value).to_ulong(), (m_stdBitset << value).to_ulong()) << "Left shift by " << value << " bits did not match std::bitset";
        }
    }

    TEST_P(BitsetStdComparisonTests, LeftShiftAssignment_MatchesStd)
    {
        for (AZStd::size_t value : m_shiftValues)
        {
            m_bitset <<= value;
            m_stdBitset <<= value;
            EXPECT_EQ(m_bitset.to_ulong(), m_stdBitset.to_ulong()) << "Left shift assignment by " << value << " bits did not match std::bitset.";
        }
    }

    TEST_P(BitsetStdComparisonTests, ToString_MatchesStd)
    {
        AZStd::string bitsetString = m_bitset.to_string<char>();
        std::string stdBitsetString = m_stdBitset.to_string<char>();

        EXPECT_TRUE(azstricmp(bitsetString.c_str(), stdBitsetString.c_str()) == 0) << "Bitset string '" << bitsetString.c_str() << "' does not match expected output '" << stdBitsetString << "'";
    }

    // Helper to generate a set of n bitsets that can be re-used to generate either n test cases or nxn test cases
    std::vector<unsigned long> GenerateBitsetTestCases()
    {
        std::vector<unsigned long> testCases;

        testCases.push_back(0b0000'0000'0000'0000'0000'0000'0000'0000);
        testCases.push_back(0b1111'1111'1111'1111'1111'1111'1111'1111);
        testCases.push_back(0b1010'1010'1010'1010'1010'1010'1010'1010);
        testCases.push_back(0b0101'0101'0101'0101'0101'0101'0101'0101);
        testCases.push_back(0b0000'0000'0000'0000'1111'1111'1111'1111);
        testCases.push_back(0b1111'1111'1111'1111'0000'0000'0000'0000);

        // Asymmetrical test cases
        testCases.push_back(0b1100'1010'1000'1000'0011'1101'0100'1100);
        testCases.push_back(0b0111'1100'1000'1001'1000'0001'1001'1001);

        return testCases;
    };

    std::vector<unsigned long> GenerateBitsetUnsignedLongTestCases()
    {
        return GenerateBitsetTestCases();
    }

    std::vector<AZStd::pair<unsigned long, unsigned long>> GenerateBitsetUnsignedLongPairTestCases()
    {
        std::vector<AZStd::pair<unsigned long, unsigned long>> testCasePairs;
        std::vector<unsigned long> testCases = GenerateBitsetTestCases();
        for (unsigned long value1 : testCases)
        {
            for (unsigned long value2 : testCases)
            {
                testCasePairs.emplace_back(value1, value2);
            }
        }
        return testCasePairs;
    }

    std::string GenerateBitsetUnsignedLongTestCaseName(const ::testing::TestParamInfo<unsigned long>& info)
    {
        std::bitset<32> stdBitset(info.param);
        return stdBitset.to_string();
    }

    std::string GenerateBitsetUnsignedLongPairTestCaseName(const ::testing::TestParamInfo<AZStd::pair<unsigned long, unsigned long>>& info)
    {
        // Output a string in the style of 0000x1111 where 0000 is the first bitset and 1111 is the second bitset
        std::bitset<32> bitset1(info.param.first);
        std::bitset<32> bitset2(info.param.second);
        return bitset1.to_string() + 'x' + bitset2.to_string();
    }

    INSTANTIATE_TEST_CASE_P(Bitset, BitsetUnsignedLongTests, ::testing::ValuesIn(GenerateBitsetUnsignedLongTestCases()), GenerateBitsetUnsignedLongTestCaseName);

    INSTANTIATE_TEST_CASE_P(Bitset, BitsetUnsignedLongPairTests, ::testing::ValuesIn(GenerateBitsetUnsignedLongPairTestCases()), GenerateBitsetUnsignedLongPairTestCaseName);

    INSTANTIATE_TEST_CASE_P(Bitset, BitsetStdComparisonTests, ::testing::ValuesIn(GenerateBitsetUnsignedLongTestCases()), GenerateBitsetUnsignedLongTestCaseName);

    using namespace AZStd;

    class BitsetTests
        : public AllocatorsFixture
    {
    };

    TEST_F(BitsetTests, DefaultConstructor_IsZero)
    {
        bitset<8> bitset;
        ASSERT_EQ(bitset.to_ullong(), 0);
    }

    TEST_F(BitsetTests, Constructor64Bits_MatchesInput)
    {
        constexpr AZ::u64 initValue = std::numeric_limits<AZ::u64>::max();
        bitset<64> bitset(initValue);

        ASSERT_EQ(bitset.to_ullong(), initValue);
    }

    TEST_F(BitsetTests, Constructor64BitsInto32Bits_MatchesLeastSignificant32Bits)
    {
        constexpr AZ::u64 initValue = std::numeric_limits<AZ::u64>::max();
        bitset<32> bitset(initValue);

        constexpr AZ::u64 expectedValue(initValue & static_cast<AZ::u32>(-1));

        ASSERT_EQ(bitset.to_ullong(), expectedValue);
    }

    TEST_F(BitsetTests, Constructor32BitsInto64Bits_ZeroPadRemaining)
    {
        constexpr AZ::u32 initValue = std::numeric_limits<AZ::u32>::max();
        bitset<64> bitset(initValue);

        constexpr AZ::u64 expectedValue = static_cast<AZ::u64>(initValue);;

        ASSERT_EQ(bitset.to_ullong(), expectedValue);
    }

    TEST_F(BitsetTests, GeneralTesting)
    {
        // BitsetTest-Begin
        typedef bitset<25> bitset25_type;

        bitset25_type bs;
        AZ_TEST_ASSERT(bs.count() == 0);

        bitset25_type bs1((unsigned long)5);
        AZ_TEST_ASSERT(bs1.count() == 2);
        AZ_TEST_ASSERT(bs1[0] && bs1[2]);

        string str("10110");
        bitset25_type bs2(str, 0, str.length());
        AZ_TEST_ASSERT(bs2.count() == 3);
        AZ_TEST_ASSERT(bs2[1] && bs2[2] && bs2[4]);

        bitset25_type::reference bit0 = bs2[0], bit1 = bs2[1];
        AZ_TEST_ASSERT(bit0 == false);
        AZ_TEST_ASSERT(bit1 == true);

        bs &= bs1;
        AZ_TEST_ASSERT(bs.count() == 0);

        bs |= bs1;
        AZ_TEST_ASSERT(bs.count() == 2);
        AZ_TEST_ASSERT(bs[0] && bs[2]);

        bs ^= bs2;
        AZ_TEST_ASSERT(bs.count() == 3);
        AZ_TEST_ASSERT(bs[0] && bs[1] && bs[4]);

        bs <<= 4;
        AZ_TEST_ASSERT(bs.count() == 3);
        AZ_TEST_ASSERT(bs[4] && bs[5] && bs[8]);

        bs >>= 3;
        AZ_TEST_ASSERT(bs.count() == 3);
        AZ_TEST_ASSERT(bs[1] && bs[2] && bs[5]);

        bs.set(3);
        AZ_TEST_ASSERT(bs.count() == 4);
        AZ_TEST_ASSERT(bs[1] && bs[2] && bs[3] && bs[5]);

        bs.set(1, false);
        AZ_TEST_ASSERT(bs.count() == 3);
        AZ_TEST_ASSERT(!bs[1] && bs[2] && bs[3] && bs[5]);

        bs.set();
        AZ_TEST_ASSERT(bs.count() == 25);

        bs.reset();
        AZ_TEST_ASSERT(bs.count() == 0);

        bs.set(0);
        bs.set(1);
        AZ_TEST_ASSERT(bs.count() == 2);

        bs.flip();
        AZ_TEST_ASSERT(bs.count() == 23);

        bs.flip(0);
        AZ_TEST_ASSERT(bs.count() == 24);

        str = bs.to_string<char>();
        AZ_TEST_ASSERT(str.length() == 25);

        AZ_TEST_ASSERT(bs != bs1);
        bs2 = bs;
        AZ_TEST_ASSERT(bs == bs2);

        bs1.reset();
        AZ_TEST_ASSERT(bs.any());
        AZ_TEST_ASSERT(!bs1.any());
        AZ_TEST_ASSERT(!bs.none());
        AZ_TEST_ASSERT(bs1.none());

        bs1 = bs >> 1;
        AZ_TEST_ASSERT(bs1.count() == 23);

        bs1 = bs << 2;
        AZ_TEST_ASSERT(bs1.count() == 22);

        // extensions
        bitset25_type bs3(string("10110"));
        AZ_TEST_ASSERT(bs3.num_words() == 1); // check number of words
        bitset25_type::word_t tempWord = *bs3.data(); // access the bits data
        AZ_TEST_ASSERT((tempWord & 0x16) == 0x16); // check values
        bitset25_type bs4;
        *bs4.data() = tempWord; // modify the data directly
        AZ_TEST_ASSERT(bs3 == bs4);
    }
} // end namespace UnitTest
