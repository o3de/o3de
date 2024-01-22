/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UserTypes.h"
#include <AzCore/std/hash_table.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/fixed_unordered_set.h>
#include <AzCore/std/containers/fixed_unordered_map.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/std/ranges/transform_view.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/typetraits/has_member_function.h>

#if defined(HAVE_BENCHMARK)
#include <benchmark/benchmark.h>
#endif // HAVE_BENCHMARK

namespace UnitTest
{
    using namespace AZStd;
    using namespace UnitTestInternal;

    AZ_HAS_MEMBER(HashValidate, validate, void, ());

    /**
     * Hash functions test.
     */
    class HashedContainers
        : public LeakDetectionFixture
    {
    public:
        template <class H, bool hasValidate>
        struct Validator
        {
            static void Validate(H& h)
            {
                EXPECT_TRUE(h.validate());
            }
        };

        template <class H>
        struct Validator<H, false>
        {
            static void Validate(H&) {}
        };

        template <class H>
        static void ValidateHash(H& h, size_t numElements = 0)
        {
            Validator<H, HasHashValidate_v<H>>::Validate(h);
            EXPECT_EQ(numElements, h.size());
            if (numElements > 0)
            {
                EXPECT_FALSE(h.empty());
                EXPECT_TRUE(h.begin() != h.end());
            }
            else
            {
                EXPECT_TRUE(h.empty());
                EXPECT_TRUE(h.begin() == h.end());
            }
        }
    };

    TEST_F(HashedContainers, Test)
    {
        EXPECT_EQ(1, hash<bool>()(true));
        EXPECT_EQ(0, hash<bool>()(false));

        char ch = 11;
        EXPECT_EQ(11, hash<char>()(ch));

        signed char sc = 127;
        EXPECT_EQ(127, hash<signed char>()(sc));

        unsigned char uc = 202;
        EXPECT_EQ(202, hash<unsigned char>()(uc));

        short sh = 16533;
        EXPECT_EQ(16533, hash<short>()(sh));

        signed short ss = 16333;
        EXPECT_EQ(16333, hash<signed short>()(ss));

        unsigned short us = 24533;
        EXPECT_EQ(24533, hash<unsigned short>()(us));

        int in = 5748538;
        EXPECT_EQ(5748538, hash<int>()(in));

        unsigned int ui = 0x0fffffff;
        EXPECT_EQ(0x0fffffff, hash<unsigned int>()(ui));

        AZ_TEST_ASSERT(hash<float>()(103.0f) != 0);
        EXPECT_EQ(hash<float>()(103.0f), hash<float>()(103.0f));
        AZ_TEST_ASSERT(hash<float>()(103.0f) != hash<float>()(103.1f));

        AZ_TEST_ASSERT(hash<double>()(103.0) != 0);
        EXPECT_EQ(hash<double>()(103.0), hash<double>()(103.0));
        AZ_TEST_ASSERT(hash<double>()(103.0) != hash<double>()(103.1));

        const char* string_literal = "Bla";
        const char* string_literal1 = "Cla";
        AZ_TEST_ASSERT(hash<const char*>()(string_literal) != 0);
        AZ_TEST_ASSERT(hash<const char*>()(string_literal) != hash<const char*>()(string_literal1));

        string str1(string_literal);
        string str2(string_literal1);
        AZ_TEST_ASSERT(hash<string>()(str1) != 0);
        AZ_TEST_ASSERT(hash<string>()(str1) != hash<string>()(str2));

        wstring wstr1(L"BLA");
        wstring wstr2(L"CLA");
        AZ_TEST_ASSERT(hash<wstring>()(wstr1) != 0);
        AZ_TEST_ASSERT(hash<wstring>()(wstr1) != hash<wstring>()(wstr2));
    }
    TEST_F(HashedContainers, HashRange_HashCombine_CompileWhenUsedInConstexprContext)
    {
        constexpr AZStd::array<int, 3> hashList{ { 4, 8, 3 } };
        static_assert(hash_range(hashList.begin(), hashList.end()) != 0);

        auto TestHashCombine = [](size_t hashValue) constexpr -> size_t
        {
            size_t seed = 0;
            hash_combine(seed, hashValue);
            return seed;
        };

        static_assert(TestHashCombine(42) != 0);
    }

    /**
     * HashTableSetTraits
     */
    // HashTableTest-Begin
    template<class T, bool isMultiSet, bool isDynamic>
    struct HashTableSetTestTraits
    {
        typedef T                               key_type;
        typedef typename AZStd::equal_to<T>     key_equal;
        typedef typename AZStd::hash<T>         hasher;
        typedef T                               value_type;
        typedef AZStd::allocator                allocator_type;
        enum
        {
            max_load_factor = 4,
            min_buckets = 7,
            has_multi_elements = isMultiSet,
            is_dynamic = isDynamic,
            fixed_num_buckets = 101, // a prime number between 1/3 - 1/4 of the number of elements.
            fixed_num_elements = 300,
        };
        static AZ_FORCE_INLINE const key_type& key_from_value(const value_type& value)  { return value; }
    };

    TEST_F(HashedContainers, HashTable_Dynamic)
    {
        array<int, 5> elements = {
            { 10, 100, 11, 2, 301 }
        };

        typedef HashTableSetTestTraits< int, false, true > hash_set_traits;
        typedef hash_table< hash_set_traits >   hash_key_table_type;
        hash_set_traits::hasher set_hasher;

        //
        hash_key_table_type hTable(set_hasher, hash_set_traits::key_equal(), hash_set_traits::allocator_type());
        ValidateHash(hTable);
        //
        hash_key_table_type hTable1(elements.begin(), elements.end(), set_hasher, hash_set_traits::key_equal(), hash_set_traits::allocator_type());
        ValidateHash(hTable1, elements.size());
        //
        hash_key_table_type hTable2(hTable1);
        ValidateHash(hTable1, hTable1.size());
        //
        hTable = hTable1;
        ValidateHash(hTable, hTable1.size());

        //AZ_TEST_ASSERT(hTable.bucket_count()==hash_set_traits::min_buckets);
        EXPECT_EQ((float)hash_set_traits::max_load_factor, hTable.max_load_factor());

        hTable.reserve(50);
        ValidateHash(hTable, hTable1.size());
        AZ_TEST_ASSERT(hTable.bucket_count() * hTable.max_load_factor() >= 50);
        AZ_TEST_ASSERT(hTable.bucket_count() < 100); // Make sure it doesn't interfere with the next test

        hTable.rehash(100);
        ValidateHash(hTable, hTable1.size());
        AZ_TEST_ASSERT(hTable.bucket_count() >= 100);

        hTable.insert(201);
        ValidateHash(hTable, hTable1.size() + 1);

        array<int, 5> toInsert = {
            { 17, 101, 27, 7, 501 }
        };
        hTable.insert(toInsert.begin(), toInsert.end());
        ValidateHash(hTable, hTable1.size() + 1 + toInsert.size());

        hTable.erase(hTable.begin());
        ValidateHash(hTable, hTable1.size() + toInsert.size());

        hTable.erase(hTable.begin(), next(hTable.begin(), (int)hTable1.size()));
        ValidateHash(hTable, toInsert.size());

        hTable.clear();
        ValidateHash(hTable);
        hTable.insert(toInsert.begin(), toInsert.end());

        hTable.erase(17);
        ValidateHash(hTable, toInsert.size() - 1);
        hTable.erase(18);
        ValidateHash(hTable, toInsert.size() - 1);

        hTable.clear();
        ValidateHash(hTable);
        hTable.insert(toInsert.begin(), toInsert.end());

        hash_key_table_type::iterator iter = hTable.find(7);
        AZ_TEST_ASSERT(iter != hTable.end());
        hash_key_table_type::const_iterator citer = hTable.find(7);
        AZ_TEST_ASSERT(citer != hTable.end());
        iter = hTable.find(57);
        AZ_TEST_ASSERT(iter == hTable.end());

        EXPECT_EQ(1, hTable.count(101));
        EXPECT_EQ(0, hTable.count(201));

        iter = hTable.lower_bound(17);
        AZ_TEST_ASSERT(iter != hTable.end());
        AZ_TEST_ASSERT(*prev(iter) != 17);
        citer = hTable.lower_bound(17);
        AZ_TEST_ASSERT(citer != hTable.end());
        AZ_TEST_ASSERT(*prev(citer) != 17);

        iter = hTable.lower_bound(57);
        AZ_TEST_ASSERT(iter == hTable.end());

        iter = hTable.upper_bound(101);
        AZ_TEST_ASSERT(iter == hTable.end() || *iter != 101);
        EXPECT_EQ(101, *prev(iter));
        citer = hTable.upper_bound(101);
        AZ_TEST_ASSERT(citer == hTable.end() || *citer != 101);
        EXPECT_EQ(101, *prev(citer));

        iter = hTable.upper_bound(57);
        AZ_TEST_ASSERT(iter == hTable.end());

        hash_key_table_type::pair_iter_iter rangeIt = hTable.equal_range(17);
        AZ_TEST_ASSERT(rangeIt.first != hTable.end());
        AZ_TEST_ASSERT(next(rangeIt.first) == rangeIt.second);

        hash_key_table_type::pair_citer_citer crangeIt = hTable.equal_range(17);
        AZ_TEST_ASSERT(crangeIt.first != hTable.end());
        AZ_TEST_ASSERT(next(crangeIt.first) == crangeIt.second);

        hTable1.clear();
        hTable.swap(hTable1);
        ValidateHash(hTable);
        ValidateHash(hTable1, toInsert.size());

        typedef HashTableSetTestTraits< int, true, true > hash_multiset_traits;
        typedef hash_table< hash_multiset_traits >  hash_key_multitable_type;

        hash_key_multitable_type hMultiTable(set_hasher, hash_set_traits::key_equal(), hash_set_traits::allocator_type());

        array<int, 7> toInsert2 = {
            { 17, 101, 27, 7, 501, 101, 101 }
        };
        hMultiTable.insert(toInsert2.begin(), toInsert2.end());
        ValidateHash(hMultiTable, toInsert2.size());

        EXPECT_EQ(3, hMultiTable.count(101));
        AZ_TEST_ASSERT(hMultiTable.lower_bound(101) != prev(hMultiTable.upper_bound(101)));
        AZ_TEST_ASSERT(hMultiTable.lower_bound(501) == prev(hMultiTable.upper_bound(501)));

        hash_key_multitable_type::pair_iter_iter rangeIt2 = hMultiTable.equal_range(101);
        AZ_TEST_ASSERT(rangeIt2.first != rangeIt2.second);
        EXPECT_EQ(3, distance(rangeIt2.first, rangeIt2.second));
        hash_key_multitable_type::iterator iter2 = rangeIt2.first;
        for (; iter2 != rangeIt2.second; ++iter2)
        {
            EXPECT_EQ(101, *iter2);
        }
    }

    TEST_F(HashedContainers, HashTable_InsertionDuplicateOnRehash)
    {
        struct TwoPtrs
        {
            void* m_ptr1;
            void* m_ptr2;

            bool operator==(const TwoPtrs& other) const
            {
                if (m_ptr1 == other.m_ptr1)
                {
                    return m_ptr2 == other.m_ptr2;
                }
                else if (m_ptr1 == other.m_ptr2)
                {
                    return m_ptr2 == other.m_ptr1;
                }
                return false;
            }
        };

        // This hashing function produces different hashes for two equal values,
        // which violates the requirement for hashing functions.
        // The test makes sure that this does not reproduce an issue that caused the insert() function to loop infinitely.
        struct TwoPtrsHasher
        {
            size_t operator()(const TwoPtrs& p) const
            {
                size_t hash{ 0 };
                AZStd::hash_combine(hash, p.m_ptr1, p.m_ptr2);
                return hash;
            }
        };
        using PairSet = AZStd::unordered_set<TwoPtrs, TwoPtrsHasher>;
        PairSet set;
        set.insert({ (void*)1, (void*)2 });
        set.insert({ (void*)3, (void*)4 });
        set.insert({ (void*)5, (void*)6 });
        set.insert({ (void*)7, (void*)8 });
        // Elements with different hashes, but equal
        set.insert({ (void*)0x000001ceddd9ca20, (void*)0x000001ceddd9cba0 }); // hash(148335135725641)
        set.insert({ (void*)0x000001ceddd9cba0, (void*)0x000001ceddd9ca20 }); // hash(148335135764189)
        AZ_TEST_START_TRACE_SUPPRESSION;
        // This will trigger the assertion of duplicated elements found
        // A bucket size of 23 since is where the collision between different hashes happens
        set.rehash(23);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1); // 1 assertion
    }

    TEST_F(HashedContainers, HashTable_Fixed)
    {
        array<int, 5> elements = {
            { 10, 100, 11, 2, 301 }
        };

        typedef HashTableSetTestTraits< int, false, false > hash_set_traits;
        typedef hash_table< hash_set_traits >   hash_key_table_type;
        hash_set_traits::hasher set_hasher;

        //
        hash_key_table_type hTable(set_hasher, hash_set_traits::key_equal(), hash_set_traits::allocator_type());
        ValidateHash(hTable);
        //
        hash_key_table_type hTable1(elements.begin(), elements.end(), set_hasher, hash_set_traits::key_equal(), hash_set_traits::allocator_type());
        ValidateHash(hTable1, elements.size());
        //
        hash_key_table_type hTable2(hTable1);
        ValidateHash(hTable1, hTable1.size());
        //
        hTable = hTable1;
        ValidateHash(hTable, hTable1.size());

        hTable.rehash(100);
        ValidateHash(hTable, hTable1.size());
        AZ_TEST_ASSERT(hTable.bucket_count() >= 100);

        hTable.insert(201);
        ValidateHash(hTable, hTable1.size() + 1);

        array<int, 5> toInsert = {
            { 17, 101, 27, 7, 501 }
        };
        hTable.insert(toInsert.begin(), toInsert.end());
        ValidateHash(hTable, hTable1.size() + 1 + toInsert.size());

        hTable.erase(hTable.begin());
        ValidateHash(hTable, hTable1.size() + toInsert.size());

        hTable.erase(hTable.begin(), next(hTable.begin(), (int)hTable1.size()));
        ValidateHash(hTable, toInsert.size());

        hTable.clear();
        ValidateHash(hTable);
        hTable.insert(toInsert.begin(), toInsert.end());

        hTable.erase(17);
        ValidateHash(hTable, toInsert.size() - 1);
        hTable.erase(18);
        ValidateHash(hTable, toInsert.size() - 1);

        hTable.clear();
        ValidateHash(hTable);
        hTable.insert(toInsert.begin(), toInsert.end());

        hash_key_table_type::iterator iter = hTable.find(7);
        AZ_TEST_ASSERT(iter != hTable.end());
        hash_key_table_type::const_iterator citer = hTable.find(7);
        AZ_TEST_ASSERT(citer != hTable.end());
        iter = hTable.find(57);
        AZ_TEST_ASSERT(iter == hTable.end());

        EXPECT_EQ(1, hTable.count(101));
        EXPECT_EQ(0, hTable.count(201));

        iter = hTable.lower_bound(17);
        AZ_TEST_ASSERT(iter != hTable.end());
        citer = hTable.lower_bound(17);
        AZ_TEST_ASSERT(citer != hTable.end());

        iter = hTable.lower_bound(57);
        AZ_TEST_ASSERT(iter == hTable.end());

        iter = hTable.upper_bound(101);
        AZ_TEST_ASSERT(iter != hTable.end());
        citer = hTable.upper_bound(101);
        AZ_TEST_ASSERT(citer != hTable.end());

        iter = hTable.upper_bound(57);
        AZ_TEST_ASSERT(iter == hTable.end());

        hash_key_table_type::pair_iter_iter rangeIt = hTable.equal_range(17);
        AZ_TEST_ASSERT(rangeIt.first != hTable.end());
        AZ_TEST_ASSERT(next(rangeIt.first) == rangeIt.second);

        hash_key_table_type::pair_citer_citer crangeIt = hTable.equal_range(17);
        AZ_TEST_ASSERT(crangeIt.first != hTable.end());
        AZ_TEST_ASSERT(next(crangeIt.first) == crangeIt.second);

        hTable1.clear();
        hTable.swap(hTable1);
        ValidateHash(hTable);
        ValidateHash(hTable1, toInsert.size());

        typedef HashTableSetTestTraits< int, true, false > hash_multiset_traits;
        typedef hash_table< hash_multiset_traits >  hash_key_multitable_type;

        hash_key_multitable_type hMultiTable(set_hasher, hash_set_traits::key_equal(), hash_set_traits::allocator_type());

        array<int, 7> toInsert2 = {
            { 17, 101, 27, 7, 501, 101, 101 }
        };
        hMultiTable.insert(toInsert2.begin(), toInsert2.end());
        ValidateHash(hMultiTable, toInsert2.size());

        EXPECT_EQ(3, hMultiTable.count(101));
        AZ_TEST_ASSERT(hMultiTable.lower_bound(101) != prev(hMultiTable.upper_bound(101)));
        AZ_TEST_ASSERT(hMultiTable.lower_bound(501) == prev(hMultiTable.upper_bound(501)));

        hash_key_multitable_type::pair_iter_iter rangeIt2 = hMultiTable.equal_range(101);
        AZ_TEST_ASSERT(rangeIt2.first != rangeIt2.second);
        EXPECT_EQ(3, distance(rangeIt2.first, rangeIt2.second));
        hash_key_multitable_type::iterator iter2 = rangeIt2.first;
        for (; iter2 != rangeIt2.second; ++iter2)
        {
            EXPECT_EQ(101, *iter2);
        }
    }

    TEST_F(HashedContainers, UnorderedSetTest)
    {
        typedef unordered_set<int> int_set_type;

        int_set_type::hasher myHasher;
        int_set_type::key_equal myKeyEqCmp;
        int_set_type::allocator_type myAllocator;

        int_set_type int_set;
        ValidateHash(int_set);

        int_set_type int_set1(100);
        ValidateHash(int_set1);
        AZ_TEST_ASSERT(int_set1.bucket_count() >= 100);

        int_set_type int_set2(100, myHasher, myKeyEqCmp);
        ValidateHash(int_set2);
        AZ_TEST_ASSERT(int_set2.bucket_count() >= 100);

        int_set_type int_set3(100, myHasher, myKeyEqCmp, myAllocator);
        ValidateHash(int_set3);
        AZ_TEST_ASSERT(int_set3.bucket_count() >= 100);

        array<int, 5> uniqueArray = {
            { 17, 101, 27, 7, 501 }
        };

        int_set_type int_set4(uniqueArray.begin(), uniqueArray.end());
        ValidateHash(int_set4, uniqueArray.size());

        int_set_type int_set5(uniqueArray.begin(), uniqueArray.end(), 100);
        ValidateHash(int_set5, uniqueArray.size());
        AZ_TEST_ASSERT(int_set5.bucket_count() >= 100);

        int_set_type int_set6(uniqueArray.begin(), uniqueArray.end(), 100, myHasher, myKeyEqCmp);
        ValidateHash(int_set6, uniqueArray.size());
        AZ_TEST_ASSERT(int_set6.bucket_count() >= 100);

        int_set_type int_set7(uniqueArray.begin(), uniqueArray.end(), 100, myHasher, myKeyEqCmp, myAllocator);
        ValidateHash(int_set7, uniqueArray.size());
        AZ_TEST_ASSERT(int_set7.bucket_count() >= 100);

        int_set_type int_set8({ 1, 2, 3, 4, 4, 5 });
        ValidateHash(int_set8, 5);
        EXPECT_EQ(1, int_set8.count(1));
        EXPECT_EQ(1, int_set8.count(2));
        EXPECT_EQ(1, int_set8.count(3));
        EXPECT_EQ(1, int_set8.count(4));
        EXPECT_EQ(1, int_set8.count(5));

        int_set_type int_set9 = int_set8;
        EXPECT_EQ(int_set9, int_set8);
        EXPECT_TRUE(int_set1 != int_set9);

        int_set_type int_set10;
        int_set10.insert({ 1, 2, 3, 4, 4, 5 });
        EXPECT_EQ(int_set8, int_set10);
    }

    TEST_F(HashedContainers, FixedUnorderedSetTest)
    {
        typedef fixed_unordered_set<int, 101, 300> int_set_type;

        int_set_type::hasher myHasher;
        int_set_type::key_equal myKeyEqCmp;

        int_set_type int_set;
        ValidateHash(int_set);
        EXPECT_EQ(101, int_set.bucket_count());

        int_set_type int_set1(myHasher, myKeyEqCmp);
        ValidateHash(int_set1);
        EXPECT_EQ(101, int_set1.bucket_count());

        array<int, 5> uniqueArray = {
            { 17, 101, 27, 7, 501 }
        };

        int_set_type int_set4(uniqueArray.begin(), uniqueArray.end());
        ValidateHash(int_set4, uniqueArray.size());

        int_set_type int_set6(uniqueArray.begin(), uniqueArray.end(), 0, myHasher, myKeyEqCmp);
        ValidateHash(int_set6, uniqueArray.size());
        EXPECT_EQ(101, int_set6.bucket_count());
    }

    TEST_F(HashedContainers, UnorderedMultiSet)
    {
        typedef unordered_multiset<int> int_multiset_type;

        int_multiset_type::hasher myHasher;
        int_multiset_type::key_equal myKeyEqCmp;
        int_multiset_type::allocator_type myAllocator;

        int_multiset_type int_set;
        ValidateHash(int_set);

        int_multiset_type int_set1(100);
        ValidateHash(int_set1);
        AZ_TEST_ASSERT(int_set1.bucket_count() >= 100);

        int_multiset_type int_set2(100, myHasher, myKeyEqCmp);
        ValidateHash(int_set2);
        AZ_TEST_ASSERT(int_set2.bucket_count() >= 100);

        int_multiset_type int_set3(100, myHasher, myKeyEqCmp, myAllocator);
        ValidateHash(int_set3);
        AZ_TEST_ASSERT(int_set3.bucket_count() >= 100);

        array<int, 7> uniqueArray = {
            { 17, 101, 27, 7, 501, 101, 101 }
        };

        int_multiset_type int_set4(uniqueArray.begin(), uniqueArray.end());
        ValidateHash(int_set4, uniqueArray.size());

        int_multiset_type int_set5(uniqueArray.begin(), uniqueArray.end(), 100);
        ValidateHash(int_set5, uniqueArray.size());
        AZ_TEST_ASSERT(int_set5.bucket_count() >= 100);

        int_multiset_type int_set6(uniqueArray.begin(), uniqueArray.end(), 100, myHasher, myKeyEqCmp);
        ValidateHash(int_set6, uniqueArray.size());
        AZ_TEST_ASSERT(int_set6.bucket_count() >= 100);

        int_multiset_type int_set7(uniqueArray.begin(), uniqueArray.end(), 100, myHasher, myKeyEqCmp, myAllocator);
        ValidateHash(int_set7, uniqueArray.size());
        AZ_TEST_ASSERT(int_set7.bucket_count() >= 100);

        int_multiset_type int_set8({ 1, 2, 3, 4, 4, 5 });
        ValidateHash(int_set8, 6);
        EXPECT_EQ(1, int_set8.count(1));
        EXPECT_EQ(1, int_set8.count(2));
        EXPECT_EQ(1, int_set8.count(3));
        EXPECT_EQ(2, int_set8.count(4));
        EXPECT_EQ(1, int_set8.count(5));

        int_multiset_type int_set9 = int_set8;
        AZ_TEST_ASSERT(int_set8 == int_set9);
        AZ_TEST_ASSERT(int_set1 != int_set9);

        int_multiset_type int_set10;
        int_set10.insert({ 1, 2, 3, 4, 4, 5 });
        EXPECT_EQ(int_set8, int_set10);
    }

    TEST_F(HashedContainers, FixedUnorderedMultiSet)
    {
        typedef fixed_unordered_multiset<int, 101, 300> int_multiset_type;

        int_multiset_type::hasher myHasher;
        int_multiset_type::key_equal myKeyEqCmp;

        int_multiset_type int_set;
        ValidateHash(int_set);
        EXPECT_EQ(101, int_set.bucket_count());

        int_multiset_type int_set2(myHasher, myKeyEqCmp);
        ValidateHash(int_set2);
        EXPECT_EQ(101, int_set2.bucket_count());

        array<int, 7> uniqueArray = {
            { 17, 101, 27, 7, 501, 101, 101 }
        };

        int_multiset_type int_set4(uniqueArray.begin(), uniqueArray.end());
        ValidateHash(int_set4, uniqueArray.size());
        EXPECT_EQ(101, int_set4.bucket_count());

        int_multiset_type int_set6(uniqueArray.begin(), uniqueArray.end(), 0, myHasher, myKeyEqCmp);
        ValidateHash(int_set6, uniqueArray.size());
        EXPECT_EQ(101, int_set6.bucket_count());
    }

    TEST_F(HashedContainers, UnorderedMapBasic)
    {
        typedef unordered_map<int, int> int_int_map_type;

        int_int_map_type::hasher myHasher;
        int_int_map_type::key_equal myKeyEqCmp;
        int_int_map_type::allocator_type myAllocator;

        int_int_map_type intint_map;
        ValidateHash(intint_map);

        int_int_map_type intint_map1(100);
        ValidateHash(intint_map1);
        AZ_TEST_ASSERT(intint_map1.bucket_count() >= 100);

        int_int_map_type intint_map2(100, myHasher, myKeyEqCmp);
        ValidateHash(intint_map2);
        AZ_TEST_ASSERT(intint_map2.bucket_count() >= 100);

        int_int_map_type intint_map3(100, myHasher, myKeyEqCmp, myAllocator);
        ValidateHash(intint_map3);
        AZ_TEST_ASSERT(intint_map3.bucket_count() >= 100);

        // insert with default value.
        intint_map.insert_key(16);
        ValidateHash(intint_map, 1);
        EXPECT_EQ(16, intint_map.begin()->first);

        intint_map.insert(AZStd::make_pair(22, 11));
        ValidateHash(intint_map, 2);

        // map look up
        int& mappedValue = intint_map[22];
        EXPECT_EQ(11, mappedValue);
        int& mappedValueNew = intint_map[33];   // insert a new element
        mappedValueNew = 100;
        EXPECT_EQ(100, mappedValueNew);
        EXPECT_EQ(100, intint_map.at(33));

        array<int_int_map_type::value_type, 5> uniqueArray;
        uniqueArray[0] = int_int_map_type::value_type(17, 1);
        uniqueArray[1] = int_int_map_type::value_type(101, 2);
        uniqueArray[2] = int_int_map_type::value_type(27, 3);
        uniqueArray[3] = int_int_map_type::value_type(7, 4);
        uniqueArray[4] = int_int_map_type::value_type(501, 5);

        int_int_map_type intint_map4(uniqueArray.begin(), uniqueArray.end());
        ValidateHash(intint_map4, uniqueArray.size());

        int_int_map_type intint_map5(uniqueArray.begin(), uniqueArray.end(), 100);
        ValidateHash(intint_map5, uniqueArray.size());
        AZ_TEST_ASSERT(intint_map5.bucket_count() >= 100);

        int_int_map_type intint_map6(uniqueArray.begin(), uniqueArray.end(), 100, myHasher, myKeyEqCmp);
        ValidateHash(intint_map6, uniqueArray.size());
        AZ_TEST_ASSERT(intint_map6.bucket_count() >= 100);

        int_int_map_type intint_map7(uniqueArray.begin(), uniqueArray.end(), 100, myHasher, myKeyEqCmp, myAllocator);
        ValidateHash(intint_map7, uniqueArray.size());
        AZ_TEST_ASSERT(intint_map7.bucket_count() >= 100);

        int_int_map_type intint_map8({
            { 1, 10 },{ 2, 200 },{ 3, 3000 },{ 4, 40000 },{ 4, 40001 },{ 5, 500000 }
        });
        ValidateHash(intint_map8, 5);
        EXPECT_EQ(10, intint_map8[1]);
        EXPECT_EQ(200, intint_map8[2]);
        EXPECT_EQ(3000, intint_map8[3]);
        EXPECT_EQ(40000, intint_map8[4]);
        EXPECT_EQ(500000, intint_map8[5]);
        EXPECT_EQ(1, intint_map8.find(1)->first);
        EXPECT_EQ(2, intint_map8.find(2)->first);
        EXPECT_EQ(3, intint_map8.find(3)->first);
        EXPECT_EQ(4, intint_map8.find(4)->first);
        EXPECT_EQ(5, intint_map8.find(5)->first);
        EXPECT_EQ(10, intint_map8.find(1)->second);
        EXPECT_EQ(200, intint_map8.find(2)->second);
        EXPECT_EQ(3000, intint_map8.find(3)->second);
        EXPECT_EQ(40000, intint_map8.find(4)->second);
        EXPECT_EQ(500000, intint_map8.find(5)->second);

        int_int_map_type intint_map9;
        intint_map9.insert({ { 1, 10 }, { 2, 200 }, { 3, 3000 }, { 4, 40000 }, { 4, 40001 }, { 5, 500000 } });
        EXPECT_EQ(intint_map8, intint_map9);
    }

    TEST_F(HashedContainers, UnorderedMapNoncopyableValue)
    {
        using nocopy_map = unordered_map<int, MyNoCopyClass>;

        nocopy_map ptr_map;
        nocopy_map::value_type test_pair;
        test_pair.second.m_bool = true;
        ptr_map.insert(AZStd::move(test_pair));

        for (const auto& pair : ptr_map)
        {
            AZ_TEST_ASSERT(pair.second.m_bool);
        }
    }

    TEST_F(HashedContainers, UnorderedMapNonTrivialValue)
    {
        typedef unordered_map<int, MyClass> int_myclass_map_type;

        int_myclass_map_type::hasher myHasher;
        int_myclass_map_type::key_equal myKeyEqCmp;
        int_myclass_map_type::allocator_type myAllocator;

        int_myclass_map_type intclass_map;
        ValidateHash(intclass_map);

        int_myclass_map_type intclass_map1(100);
        ValidateHash(intclass_map1);
        AZ_TEST_ASSERT(intclass_map1.bucket_count() >= 100);

        int_myclass_map_type intclass_map2(100, myHasher, myKeyEqCmp);
        ValidateHash(intclass_map2);
        AZ_TEST_ASSERT(intclass_map2.bucket_count() >= 100);

        int_myclass_map_type intclass_map3(100, myHasher, myKeyEqCmp, myAllocator);
        ValidateHash(intclass_map3);
        AZ_TEST_ASSERT(intclass_map3.bucket_count() >= 100);

        // insert with default value.
        intclass_map.insert_key(16);
        ValidateHash(intclass_map, 1);

        intclass_map.insert(AZStd::make_pair(22, MyClass(11)));
        ValidateHash(intclass_map, 2);

        // Emplace
        auto result = intclass_map.emplace(33, MyClass(37));
        AZ_TEST_ASSERT(result.second);
        EXPECT_EQ(37, result.first->second.m_data);
        ValidateHash(intclass_map, 3);

        // map look up
        MyClass& mappedValue = intclass_map[22];
        EXPECT_EQ(11, mappedValue.m_data);
        MyClass& mappedValueNew = intclass_map[33];     // insert a new element
        mappedValueNew.m_data = 100;
        EXPECT_EQ(100, mappedValueNew.m_data);
        EXPECT_EQ(100, intclass_map.at(33).m_data);

        array<int_myclass_map_type::value_type, 5> uniqueArray;
        uniqueArray[0] = AZStd::make_pair(17, MyClass(1));
        uniqueArray[1] = AZStd::make_pair(101, MyClass(2));
        uniqueArray[2] = AZStd::make_pair(27, MyClass(3));
        uniqueArray[3] = int_myclass_map_type::value_type(7, MyClass(4));
        uniqueArray[4] = int_myclass_map_type::value_type(501, MyClass(5));

        int_myclass_map_type intclass_map4(uniqueArray.begin(), uniqueArray.end());
        ValidateHash(intclass_map4, uniqueArray.size());

        int_myclass_map_type intclass_map5(uniqueArray.begin(), uniqueArray.end(), 100);
        ValidateHash(intclass_map5, uniqueArray.size());
        AZ_TEST_ASSERT(intclass_map5.bucket_count() >= 100);

        int_myclass_map_type intclass_map6(uniqueArray.begin(), uniqueArray.end(), 100, myHasher, myKeyEqCmp);
        ValidateHash(intclass_map6, uniqueArray.size());
        AZ_TEST_ASSERT(intclass_map6.bucket_count() >= 100);

        int_myclass_map_type intclass_map7(uniqueArray.begin(), uniqueArray.end(), 100, myHasher, myKeyEqCmp, myAllocator);
        ValidateHash(intclass_map7, uniqueArray.size());
        AZ_TEST_ASSERT(intclass_map7.bucket_count() >= 100);

        int_myclass_map_type intclass_map8({
            { 1, MyClass(10) },{ 2, MyClass(200) },{ 3, MyClass(3000) },{ 4, MyClass(40000) },{ 4, MyClass(40001) },{ 5, MyClass(500000) }
        });
        ValidateHash(intclass_map8, 5);
        AZ_TEST_ASSERT(intclass_map8[1] == MyClass(10));
        AZ_TEST_ASSERT(intclass_map8[2] == MyClass(200));
        AZ_TEST_ASSERT(intclass_map8[3] == MyClass(3000));
        AZ_TEST_ASSERT(intclass_map8[4] == MyClass(40000));
        AZ_TEST_ASSERT(intclass_map8[5] == MyClass(500000));
        EXPECT_EQ(1, intclass_map8.find(1)->first);
        EXPECT_EQ(2, intclass_map8.find(2)->first);
        EXPECT_EQ(3, intclass_map8.find(3)->first);
        EXPECT_EQ(4, intclass_map8.find(4)->first);
        EXPECT_EQ(5, intclass_map8.find(5)->first);
        AZ_TEST_ASSERT(intclass_map8.find(1)->second == MyClass(10));
        AZ_TEST_ASSERT(intclass_map8.find(2)->second == MyClass(200));
        AZ_TEST_ASSERT(intclass_map8.find(3)->second == MyClass(3000));
        AZ_TEST_ASSERT(intclass_map8.find(4)->second == MyClass(40000));
        AZ_TEST_ASSERT(intclass_map8.find(5)->second == MyClass(500000));

        int_myclass_map_type intclass_map9;
        intclass_map9.insert({ { 1, MyClass(10) },{ 2, MyClass(200) },{ 3, MyClass(3000) },{ 4, MyClass(40000) },{ 4, MyClass(40001) },{ 5, MyClass(500000) } });
        EXPECT_EQ(intclass_map8, intclass_map9);
    }

    TEST_F(HashedContainers, UnorderedMapNonTrivialKey)
    {
        typedef unordered_map<AZStd::string, MyClass> string_myclass_map_type;

        string_myclass_map_type stringclass_map;
        ValidateHash(stringclass_map);

        // insert with default value.
        stringclass_map.insert_key("Hello");
        ValidateHash(stringclass_map, 1);

        stringclass_map.insert(AZStd::make_pair("World", MyClass(11)));
        ValidateHash(stringclass_map, 2);

        // Emplace
        auto result = stringclass_map.emplace("Goodbye", MyClass(37));
        AZ_TEST_ASSERT(result.second);
        EXPECT_EQ(37, result.first->second.m_data);
        ValidateHash(stringclass_map, 3);

        // map look up
        MyClass& mappedValue = stringclass_map["World"];
        EXPECT_EQ(11, mappedValue.m_data);
        MyClass& mappedValueNew = stringclass_map["Goodbye"];     // insert a new element
        mappedValueNew.m_data = 100;
        EXPECT_EQ(100, mappedValueNew.m_data);
        EXPECT_EQ(100, stringclass_map.at("Goodbye").m_data);
    }

    TEST_F(HashedContainers, FixedUnorderedMapBasic)
    {
        typedef fixed_unordered_map<int, int, 101, 300> int_int_map_type;

        int_int_map_type::hasher myHasher;
        int_int_map_type::key_equal myKeyEqCmp;

        int_int_map_type intint_map;
        ValidateHash(intint_map);
        EXPECT_EQ(101, intint_map.bucket_count());

        int_int_map_type intint_map2(myHasher, myKeyEqCmp);
        ValidateHash(intint_map2);
        EXPECT_EQ(101, intint_map2.bucket_count());

        // insert with default value.
        intint_map.insert_key(16);
        ValidateHash(intint_map, 1);
        EXPECT_EQ(16, intint_map.begin()->first);

        intint_map.insert(AZStd::make_pair(22, 11));
        ValidateHash(intint_map, 2);

        // map look up
        int& mappedValue = intint_map[22];
        EXPECT_EQ(11, mappedValue);
        int& mappedValueNew = intint_map[33];   // insert a new element
        mappedValueNew = 100;
        EXPECT_EQ(100, mappedValueNew);
        EXPECT_EQ(100, intint_map.at(33));

        array<int_int_map_type::value_type, 5> uniqueArray;
        uniqueArray[0] = int_int_map_type::value_type(17, 1);
        uniqueArray[1] = int_int_map_type::value_type(101, 2);
        uniqueArray[2] = int_int_map_type::value_type(27, 3);
        uniqueArray[3] = int_int_map_type::value_type(7, 4);
        uniqueArray[4] = int_int_map_type::value_type(501, 5);

        int_int_map_type intint_map4(uniqueArray.begin(), uniqueArray.end());
        ValidateHash(intint_map4, uniqueArray.size());
        EXPECT_EQ(101, intint_map4.bucket_count());

        int_int_map_type intint_map6(uniqueArray.begin(), uniqueArray.end(), 0, myHasher, myKeyEqCmp);
        ValidateHash(intint_map6, uniqueArray.size());
        EXPECT_EQ(101, intint_map6.bucket_count());
    }

    TEST_F(HashedContainers, FixedUnorderedMapNonTrivialValue)
    {
        typedef fixed_unordered_map<int, MyClass, 101, 300> int_myclass_map_type;

        int_myclass_map_type::hasher myHasher;
        int_myclass_map_type::key_equal myKeyEqCmp;

        int_myclass_map_type intclass_map;
        ValidateHash(intclass_map);
        EXPECT_EQ(101, intclass_map.bucket_count());

        int_myclass_map_type intclass_map2(myHasher, myKeyEqCmp);
        ValidateHash(intclass_map2);
        EXPECT_EQ(101, intclass_map2.bucket_count());

        // insert with default value.
        intclass_map.insert_key(16);
        ValidateHash(intclass_map, 1);

        intclass_map.insert(AZStd::make_pair(22, MyClass(11)));
        ValidateHash(intclass_map, 2);

        // map look up
        MyClass& mappedValue = intclass_map[22];
        EXPECT_EQ(11, mappedValue.m_data);
        MyClass& mappedValueNew = intclass_map[33];     // insert a new element
        mappedValueNew.m_data = 100;
        EXPECT_EQ(100, mappedValueNew.m_data);
        EXPECT_EQ(100, intclass_map.at(33).m_data);

        array<int_myclass_map_type::value_type, 5> uniqueArray;
        uniqueArray[0] = AZStd::make_pair(17, MyClass(1));
        uniqueArray[1] = AZStd::make_pair(101, MyClass(2));
        uniqueArray[2] = AZStd::make_pair(27, MyClass(3));
        uniqueArray[3] = int_myclass_map_type::value_type(7, MyClass(4));
        uniqueArray[4] = int_myclass_map_type::value_type(501, MyClass(5));

        int_myclass_map_type intclass_map4(uniqueArray.begin(), uniqueArray.end());
        ValidateHash(intclass_map4, uniqueArray.size());
        EXPECT_EQ(101, intclass_map4.bucket_count());

        int_myclass_map_type intclass_map6(uniqueArray.begin(), uniqueArray.end(), 0, myHasher, myKeyEqCmp);
        ValidateHash(intclass_map6, uniqueArray.size());
        EXPECT_EQ(101, intclass_map6.bucket_count());
    }

    TEST_F(HashedContainers, UnorderedMultiMapBasic)
    {
        typedef unordered_multimap<int, int> int_int_multimap_type;

        int_int_multimap_type::hasher myHasher;
        int_int_multimap_type::key_equal myKeyEqCmp;
        int_int_multimap_type::allocator_type myAllocator;

        int_int_multimap_type intint_map;
        ValidateHash(intint_map);

        int_int_multimap_type intint_map1(100);
        ValidateHash(intint_map1);
        AZ_TEST_ASSERT(intint_map1.bucket_count() >= 100);

        int_int_multimap_type intint_map2(100, myHasher, myKeyEqCmp);
        ValidateHash(intint_map2);
        AZ_TEST_ASSERT(intint_map2.bucket_count() >= 100);

        int_int_multimap_type intint_map3(100, myHasher, myKeyEqCmp, myAllocator);
        ValidateHash(intint_map3);
        AZ_TEST_ASSERT(intint_map3.bucket_count() >= 100);

        // insert with default value.
        intint_map.insert_key(16);
        intint_map.insert_key(16);
        ValidateHash(intint_map, 2);

        intint_map.insert(AZStd::make_pair(16, 44));
        intint_map.insert(AZStd::make_pair(16, 55));
        ValidateHash(intint_map, 4);

        array<int_int_multimap_type::value_type, 7> multiArray;
        multiArray[0] = int_int_multimap_type::value_type(17, 1);
        multiArray[1] = int_int_multimap_type::value_type(0, 2);
        multiArray[2] = int_int_multimap_type::value_type(27, 3);
        multiArray[3] = int_int_multimap_type::value_type(7, 4);
        multiArray[4] = int_int_multimap_type::value_type(501, 5);
        multiArray[5] = int_int_multimap_type::value_type(0, 6);
        multiArray[6] = int_int_multimap_type::value_type(0, 7);

        int_int_multimap_type intint_map4(multiArray.begin(), multiArray.end());
        ValidateHash(intint_map4, multiArray.size());

        int_int_multimap_type intint_map5(multiArray.begin(), multiArray.end(), 100);
        ValidateHash(intint_map5, multiArray.size());
        AZ_TEST_ASSERT(intint_map5.bucket_count() >= 100);

        int_int_multimap_type intint_map6(multiArray.begin(), multiArray.end(), 100, myHasher, myKeyEqCmp);
        ValidateHash(intint_map6, multiArray.size());
        AZ_TEST_ASSERT(intint_map6.bucket_count() >= 100);

        int_int_multimap_type intint_map7(multiArray.begin(), multiArray.end(), 100, myHasher, myKeyEqCmp, myAllocator);
        ValidateHash(intint_map7, multiArray.size());
        AZ_TEST_ASSERT(intint_map7.bucket_count() >= 100);

        int_int_multimap_type intint_map8({
            { 1, 10 },{ 2, 200 },{ 3, 3000 },{ 4, 40000 },{ 4, 40001 },{ 5, 500000 }
        });
        ValidateHash(intint_map8, 6);
        EXPECT_EQ(1, intint_map8.count(1));
        EXPECT_EQ(1, intint_map8.count(2));
        EXPECT_EQ(1, intint_map8.count(3));
        EXPECT_EQ(2, intint_map8.count(4));
        EXPECT_EQ(1, intint_map8.count(5));
        EXPECT_EQ(10, intint_map8.lower_bound(1)->second);
        EXPECT_EQ(200, intint_map8.lower_bound(2)->second);
        EXPECT_EQ(3000, intint_map8.lower_bound(3)->second);
        AZ_TEST_ASSERT(intint_map8.lower_bound(4)->second == 40000 || intint_map8.lower_bound(4)->second == 40001);
        AZ_TEST_ASSERT((++intint_map8.lower_bound(4))->second == 40000 || (++intint_map8.lower_bound(4))->second == 40001);
        AZ_TEST_ASSERT(intint_map8.lower_bound(5)->second == 500000);

        int_int_multimap_type intint_map9;
        intint_map9.insert({ { 1, 10 },{ 2, 200 },{ 3, 3000 },{ 4, 40000 },{ 4, 40001 },{ 5, 500000 } });
        EXPECT_EQ(intint_map8, intint_map9);
    }

    TEST_F(HashedContainers, FixedUnorderedMultiMapBasic)
    {
        typedef fixed_unordered_multimap<int, int, 101, 300> int_int_multimap_type;

        int_int_multimap_type::hasher myHasher;
        int_int_multimap_type::key_equal myKeyEqCmp;

        int_int_multimap_type intint_map;
        ValidateHash(intint_map);
        EXPECT_EQ(101, intint_map.bucket_count());

        int_int_multimap_type intint_map2(myHasher, myKeyEqCmp);
        ValidateHash(intint_map2);
        EXPECT_EQ(101, intint_map2.bucket_count());

        // insert with default value.
        intint_map.insert_key(16);
        intint_map.insert_key(16);
        ValidateHash(intint_map, 2);

        intint_map.insert(AZStd::make_pair(16, 44));
        intint_map.insert(AZStd::make_pair(16, 55));
        ValidateHash(intint_map, 4);

        array<int_int_multimap_type::value_type, 7> multiArray;
        multiArray[0] = int_int_multimap_type::value_type(17, 1);
        multiArray[1] = int_int_multimap_type::value_type(0, 2);
        multiArray[2] = int_int_multimap_type::value_type(27, 3);
        multiArray[3] = int_int_multimap_type::value_type(7, 4);
        multiArray[4] = int_int_multimap_type::value_type(501, 5);
        multiArray[5] = int_int_multimap_type::value_type(0, 6);
        multiArray[6] = int_int_multimap_type::value_type(0, 7);

        int_int_multimap_type intint_map4(multiArray.begin(), multiArray.end());
        ValidateHash(intint_map4, multiArray.size());
        EXPECT_EQ(101, intint_map4.bucket_count());

        int_int_multimap_type intint_map6(multiArray.begin(), multiArray.end(), 0, myHasher, myKeyEqCmp);
        ValidateHash(intint_map6, multiArray.size());
        EXPECT_EQ(101, intint_map6.bucket_count());
    }

    struct MoveOnlyType
    {
        MoveOnlyType() = default;
        MoveOnlyType(const AZStd::string name)
            : m_name(name)
        {}
        MoveOnlyType(const MoveOnlyType&) = delete;
        MoveOnlyType(MoveOnlyType&& other)
            : m_name(AZStd::move(other.m_name))
        {
        }

        MoveOnlyType& operator=(const MoveOnlyType&) = delete;
        MoveOnlyType& operator=(MoveOnlyType&& other)
        {
            m_name = AZStd::move(other.m_name);
            return *this;
        }

        bool operator==(const MoveOnlyType& other) const
        {
            return m_name == other.m_name;
        }

        AZStd::string m_name;
    };

    struct MoveOnlyTypeHasher
    {
        size_t operator()(const MoveOnlyType& moveOnlyType) const
        {
            return AZStd::hash<AZStd::string>()(moveOnlyType.m_name);
        }
    };

    TEST_F(HashedContainers, UnorderedSetMoveOnlyKeyTest)
    {
        AZStd::unordered_set<MoveOnlyType, MoveOnlyTypeHasher> ownedStringSet;

        AZStd::string nonOwnedString1("Test String");
        ownedStringSet.emplace(nonOwnedString1);

        auto keyEqual = [](const AZStd::string& testString, const MoveOnlyType& key)
        {
            return testString == key.m_name;
        };

        auto entityIt = ownedStringSet.find_as(nonOwnedString1, AZStd::hash<AZStd::string>(), keyEqual);
        EXPECT_NE(ownedStringSet.end(), entityIt);
        EXPECT_EQ(nonOwnedString1, entityIt->m_name);

        AZStd::string nonOwnedString2("Hashed Value");
        entityIt = ownedStringSet.find_as(nonOwnedString2, AZStd::hash<AZStd::string>(), keyEqual);
        EXPECT_EQ(ownedStringSet.end(), entityIt);
    }

    TEST_F(HashedContainers, UnorderedSetEquals)
    {
        AZStd::unordered_set<AZStd::string> aset1 = { "PlayerBase", "DamageableBase", "PlacementObstructionBase", "PredatorBase" };
        AZStd::unordered_set<AZStd::string> aset2;
        aset2.insert("PredatorBase");
        aset2.insert("PlayerBase");
        aset2.insert("PlacementObstructionBase");
        aset2.insert("DamageableBase");
        EXPECT_EQ(aset1, aset2);
    }

    TEST_F(HashedContainers, UnorderedMapIterateEmpty)
    {
        AZStd::unordered_map<int, MoveOnlyType> map;
        for (auto& item : map)
        {
            AZ_UNUSED(item);
            EXPECT_TRUE(false) << "Iteration should never have occurred on an empty unordered_map";
        }
    }

    TEST_F(HashedContainers, UnorderedMapIterateItems)
    {
        AZStd::unordered_map<int, int> map{
            {1, 2},
            {3, 4},
            {5, 6},
            {7, 8}
        };

        unsigned idx = 0;
        for (auto& item : map)
        {
            EXPECT_EQ(item.second, item.first + 1);
            ++idx;
        }

        EXPECT_EQ(idx, map.size());
    }

    template<typename ContainerType>
    class HashedSetContainers
        : public LeakDetectionFixture
    {
    };


    struct MoveOnlyIntType
    {
        MoveOnlyIntType() = default;
        MoveOnlyIntType(int32_t value)
            : m_value(value)
        {}
        MoveOnlyIntType(const MoveOnlyIntType&) = delete;
        MoveOnlyIntType& operator=(const MoveOnlyIntType&) = delete;
        MoveOnlyIntType(MoveOnlyIntType&& other)
            : m_value(other.m_value)
        {
        }

        MoveOnlyIntType& operator=(MoveOnlyIntType&& other)
        {
            m_value = other.m_value;
            other.m_value = {};
            return *this;
        }

        explicit operator int32_t()
        {
            return m_value;
        }

        bool operator==(const MoveOnlyIntType& other) const
        {
            return m_value == other.m_value;
        }

        int32_t m_value;
    };

    struct MoveOnlyIntTypeHasher
    {
        size_t operator()(const MoveOnlyIntType& moveOnlyType) const
        {
            return AZStd::hash<int32_t>()(moveOnlyType.m_value);
        }
    };

    template<typename ContainerType>
    struct HashedSetConfig
    {
        using SetType = ContainerType;
        static SetType Create()
        {
            SetType testSet;

            testSet.emplace(1);
            testSet.emplace(2);
            testSet.emplace(84075);
            testSet.emplace(-73);
            testSet.emplace(534);
            testSet.emplace(-845920);
            testSet.emplace(-42);
            testSet.emplace(0b1111'0000);
            return testSet;
        }
    };

    using SetContainerConfigs = ::testing::Types<
        HashedSetConfig<AZStd::unordered_set<int32_t>>
        , HashedSetConfig<AZStd::unordered_multiset<int32_t>>
        , HashedSetConfig<AZStd::unordered_set<MoveOnlyIntType, MoveOnlyIntTypeHasher>>
        , HashedSetConfig<AZStd::unordered_multiset<MoveOnlyIntType, MoveOnlyIntTypeHasher>>
    >;
    TYPED_TEST_CASE(HashedSetContainers, SetContainerConfigs);

    TYPED_TEST(HashedSetContainers, ExtractNodeHandleByKeySucceeds)
    {
        using SetType = typename TypeParam::SetType;
        using node_type = typename SetType::node_type;

        SetType testContainer = TypeParam::Create();
        node_type extractedNode = testContainer.extract(84075);

        EXPECT_EQ(7, testContainer.size());
        EXPECT_FALSE(extractedNode.empty());
        EXPECT_EQ(84075, static_cast<int32_t>(extractedNode.value()));
    }

    TYPED_TEST(HashedSetContainers, ExtractNodeHandleByKeyFails)
    {
        using SetType = typename TypeParam::SetType;
        using node_type = typename SetType::node_type;

        SetType testContainer = TypeParam::Create();
        node_type extractedNode = testContainer.extract(10000001);

        EXPECT_EQ(8, testContainer.size());
        EXPECT_TRUE(extractedNode.empty());
    }

    TYPED_TEST(HashedSetContainers, ExtractNodeHandleByIteratorSucceeds)
    {
        using SetType = typename TypeParam::SetType;
        using node_type = typename SetType::node_type;

        SetType testContainer = TypeParam::Create();
        auto foundIter = testContainer.find(-73);
        node_type extractedNode = testContainer.extract(foundIter);

        EXPECT_EQ(7, testContainer.size());
        EXPECT_FALSE(extractedNode.empty());
        EXPECT_EQ(-73, static_cast<int32_t>(extractedNode.value()));
    }

    TYPED_TEST(HashedSetContainers, InsertNodeHandleSucceeds)
    {
        using SetType = typename TypeParam::SetType;
        using node_type = typename SetType::node_type;

        SetType testContainer = TypeParam::Create();
        node_type extractedNode = testContainer.extract(84075);

        EXPECT_EQ(7, testContainer.size());
        EXPECT_FALSE(extractedNode.empty());
        EXPECT_EQ(84075, static_cast<int32_t>(extractedNode.value()));

        extractedNode.value() = 0;
        testContainer.insert(AZStd::move(extractedNode));
        EXPECT_NE(0, testContainer.count(0));
        EXPECT_EQ(8, testContainer.size());
    }



    TEST_F(HashedContainers, UnorderedSetInsertNodeHandleSucceeds)
    {
        using SetType = AZStd::unordered_set<int32_t>;
        using node_type = typename SetType::node_type;
        using insert_return_type = typename SetType::insert_return_type;

        SetType testContainer = HashedSetConfig<SetType>::Create();
        node_type extractedNode = testContainer.extract(84075);
        EXPECT_EQ(7, testContainer.size());
        EXPECT_FALSE(extractedNode.empty());
        EXPECT_EQ(84075, static_cast<int32_t>(extractedNode.value()));

        extractedNode.value() = -60;
        insert_return_type insertResult = testContainer.insert(AZStd::move(extractedNode));
        EXPECT_NE(testContainer.end(), insertResult.position);
        EXPECT_TRUE(insertResult.inserted);
        EXPECT_TRUE(insertResult.node.empty());

        EXPECT_NE(0, testContainer.count(-60));
        EXPECT_EQ(8, testContainer.size());
    }

    TEST_F(HashedContainers, UnorderedSetInsertNodeHandleWithEmptyNodeHandleFails)
    {
        using SetType = AZStd::unordered_set<int32_t>;
        using node_type = typename SetType::node_type;
        using insert_return_type = typename SetType::insert_return_type;

        SetType testContainer = HashedSetConfig<SetType>::Create();

        node_type extractedNode;
        EXPECT_TRUE(extractedNode.empty());

        insert_return_type insertResult = testContainer.insert(AZStd::move(extractedNode));
        EXPECT_EQ(testContainer.end(), insertResult.position);
        EXPECT_FALSE(insertResult.inserted);
        EXPECT_TRUE(insertResult.node.empty());
        EXPECT_EQ(8, testContainer.size());
    }

    TEST_F(HashedContainers, UnorderedSetInsertNodeHandleWithDuplicateValueInNodeHandleFails)
    {
        using SetType = AZStd::unordered_set<int32_t>;
        using node_type = typename SetType::node_type;
        using insert_return_type = typename SetType::insert_return_type;

        SetType testContainer = HashedSetConfig<SetType>::Create();
        node_type extractedNode = testContainer.extract(2);

        EXPECT_EQ(7, testContainer.size());
        EXPECT_FALSE(extractedNode.empty());
        EXPECT_EQ(2, static_cast<int32_t>(extractedNode.value()));

        // Set node handle value to a key that is currently within the container
        extractedNode.value() = 534;
        insert_return_type insertResult = testContainer.insert(AZStd::move(extractedNode));
        EXPECT_NE(testContainer.end(), insertResult.position);
        EXPECT_FALSE(insertResult.inserted);
        EXPECT_FALSE(insertResult.node.empty());

        EXPECT_EQ(7, testContainer.size());
    }

    TEST_F(HashedContainers, UnorderedSetExtractedNodeCanBeInsertedIntoUnorderedMultiset)
    {
        using SetType = AZStd::unordered_set<int32_t>;
        using MultisetType = AZStd::unordered_multiset<int32_t>;

        SetType uniqueSet{ 1, 2, 3 };
        MultisetType multiSet{ 1, 4, 1 };

        auto extractedNode = uniqueSet.extract(1);
        EXPECT_EQ(2, uniqueSet.size());
        EXPECT_FALSE(extractedNode.empty());

        auto insertIt = multiSet.insert(AZStd::move(extractedNode));
        EXPECT_NE(multiSet.end(), insertIt);
        EXPECT_EQ(4, multiSet.size());
        EXPECT_EQ(3, multiSet.count(1));
    }

    TEST_F(HashedContainers, UnorderedMultisetExtractedNodeCanBeInsertedIntoUnorderedSet)
    {
        using SetType = AZStd::unordered_set<int32_t>;
        using MultisetType = AZStd::unordered_multiset<int32_t>;

        SetType uniqueSet{ 1, 2, 3 };
        MultisetType multiSet{ 1, 4, 1 };

        auto extractedNode = multiSet.extract(4);
        EXPECT_EQ(2, multiSet.size());
        EXPECT_FALSE(extractedNode.empty());

        auto insertResult = uniqueSet.insert(AZStd::move(extractedNode));
        EXPECT_TRUE(insertResult.inserted);
        EXPECT_EQ(4, uniqueSet.size());
        EXPECT_EQ(1, uniqueSet.count(4));
    }

    template <typename ContainerType>
    class HashedSetDifferentAllocatorFixture
        : public LeakDetectionFixture
    {
    };

    template<template <typename, typename, typename, typename> class ContainerTemplate>
    struct HashedSetWithCustomAllocatorConfig
    {
        using ContainerType = ContainerTemplate<int32_t, AZStd::hash<int32_t>, AZStd::equal_to<int32_t>, AZ::AZStdIAllocator>;

        static ContainerType Create(std::initializer_list<typename ContainerType::value_type> intList, AZ::IAllocator* allocatorInstance)
        {
            ContainerType allocatorSet(intList, 0, AZStd::hash<int32_t>{}, AZStd::equal_to<int32_t>{}, AZ::AZStdIAllocator{ allocatorInstance });
            return allocatorSet;
        }
    };

    using SetTemplateConfigs = ::testing::Types<
        HashedSetWithCustomAllocatorConfig<AZStd::unordered_set>
        , HashedSetWithCustomAllocatorConfig<AZStd::unordered_multiset>
    >;
    TYPED_TEST_CASE(HashedSetDifferentAllocatorFixture, SetTemplateConfigs);

#if GTEST_HAS_DEATH_TEST
#if AZ_TRAIT_DISABLE_FAILED_DEATH_TESTS
    TYPED_TEST(HashedSetDifferentAllocatorFixture, DISABLED_InsertNodeHandleWithDifferentAllocatorsLogsTraceMessages)
#else
    TYPED_TEST(HashedSetDifferentAllocatorFixture, InsertNodeHandleWithDifferentAllocatorsLogsTraceMessages)
#endif // AZ_TRAIT_DISABLE_FAILED_DEATH_TESTS
    {
        using ContainerType = typename TypeParam::ContainerType;

        ContainerType systemAllocatorSet = TypeParam::Create({ {1}, {2}, {3} }, &AZ::AllocatorInstance<AZ::SystemAllocator>::Get());
        auto extractedNode = systemAllocatorSet.extract(1);

        ContainerType osAllocatorSet = TypeParam::Create({ {2} }, &AZ::AllocatorInstance<AZ::OSAllocator>::Get());
        // Attempt to insert an extracted node from a container using the AZ::SystemAllocator into a container using the AZ::OSAllocator
        EXPECT_DEATH(
            {
                UnitTest::TestRunner::Instance().StartAssertTests();
                osAllocatorSet.insert(AZStd::move(extractedNode));
                if (UnitTest::TestRunner::Instance().StopAssertTests() > 0)
                {
                    // AZ_Assert does not cause the application to exit in profile_test configuration
                    // Therefore an exit with a non-zero error code is invoked to trigger the death condition
                    exit(1);
                }
            }, ".*");
    }
#endif // GTEST_HAS_DEATH_TEST

    template<typename ContainerType>
    class HashedMapContainers
        : public LeakDetectionFixture
    {
    };

    template<typename ContainerType>
    struct HashedMapConfig
    {
        using MapType = ContainerType;
        static MapType Create()
        {
            MapType testMap;

            testMap.emplace(8001, 1337);
            testMap.emplace(-200, 31337);
            testMap.emplace(-932, 0xbaddf00d);
            testMap.emplace(73, 0xfee1badd);
            testMap.emplace(1872, 0xCDCDCDCD);
            testMap.emplace(0xFF, 7000000);
            testMap.emplace(0777, 0b00110000010);
            testMap.emplace(0b11010110110000101, 0xDDDDDDDD);
            return testMap;
        }
    };
    using MapContainerConfigs = ::testing::Types<
        HashedMapConfig<AZStd::unordered_map<int32_t, int32_t>>
        , HashedMapConfig<AZStd::unordered_multimap<int32_t, int32_t>>
        , HashedMapConfig<AZStd::unordered_map<MoveOnlyIntType, int32_t, MoveOnlyIntTypeHasher>>
        , HashedMapConfig<AZStd::unordered_multimap<MoveOnlyIntType, int32_t, MoveOnlyIntTypeHasher>>
    >;
    TYPED_TEST_CASE(HashedMapContainers, MapContainerConfigs);

    TYPED_TEST(HashedMapContainers, ExtractNodeHandleByKeySucceeds)
    {
        using MapType = typename TypeParam::MapType;
        using node_type = typename MapType::node_type;

        MapType testContainer = TypeParam::Create();
        node_type extractedNode = testContainer.extract(0777);

        EXPECT_EQ(7, testContainer.size());
        EXPECT_FALSE(extractedNode.empty());
        EXPECT_EQ(0777, static_cast<int32_t>(extractedNode.key()));
        EXPECT_EQ(0b00110000010, extractedNode.mapped());
    }

    TYPED_TEST(HashedMapContainers, ExtractNodeHandleByKeyFails)
    {
        using MapType = typename TypeParam::MapType;
        using node_type = typename MapType::node_type;

        MapType testContainer = TypeParam::Create();
        node_type extractedNode = testContainer.extract(3);

        EXPECT_EQ(8, testContainer.size());
        EXPECT_TRUE(extractedNode.empty());
    }

    TYPED_TEST(HashedMapContainers, ExtractNodeHandleByIteratorSucceeds)
    {
        using MapType = typename TypeParam::MapType;
        using node_type = typename MapType::node_type;

        MapType testContainer = TypeParam::Create();
        auto foundIter = testContainer.find(73);
        node_type extractedNode = testContainer.extract(foundIter);

        EXPECT_EQ(7, testContainer.size());
        EXPECT_FALSE(extractedNode.empty());
        EXPECT_EQ(73, static_cast<int32_t>(extractedNode.key()));
        EXPECT_EQ(0xfee1badd, extractedNode.mapped());
    }

    TYPED_TEST(HashedMapContainers, InsertNodeHandleSucceeds)
    {
        using MapType = typename TypeParam::MapType;
        using node_type = typename MapType::node_type;

        MapType testContainer = TypeParam::Create();
        node_type extractedNode = testContainer.extract(0777);

        EXPECT_EQ(7, testContainer.size());
        EXPECT_FALSE(extractedNode.empty());
        EXPECT_EQ(0777, static_cast<int32_t>(extractedNode.key()));
        extractedNode.key() = 0644;
        extractedNode.mapped() = 1'212;

        testContainer.insert(AZStd::move(extractedNode));
        auto foundIt = testContainer.find(0644);
        EXPECT_NE(testContainer.end(), foundIt);
        EXPECT_EQ(0644, static_cast<int32_t>(foundIt->first));
        EXPECT_EQ(1'212, foundIt->second);
        EXPECT_EQ(8, testContainer.size());
    }

    TEST_F(HashedContainers, UnorderedMapInsertNodeHandleSucceeds)
    {
        using MapType = AZStd::unordered_map<int32_t, int32_t>;
        using node_type = typename MapType::node_type;
        using insert_return_type = typename MapType::insert_return_type;

        MapType testContainer = HashedMapConfig<MapType>::Create();
        node_type extractedNode = testContainer.extract(0b11010110110000101);

        EXPECT_EQ(7, testContainer.size());
        EXPECT_FALSE(extractedNode.empty());
        EXPECT_EQ(0b11010110110000101, extractedNode.key());

        extractedNode.key() = -60;
        extractedNode.mapped() = -1;

        insert_return_type insertResult = testContainer.insert(AZStd::move(extractedNode));
        EXPECT_NE(testContainer.end(), insertResult.position);
        EXPECT_TRUE(insertResult.inserted);
        EXPECT_TRUE(insertResult.node.empty());

        auto foundIt = testContainer.find(-60);
        EXPECT_NE(testContainer.end(), foundIt);
        EXPECT_EQ(-60, foundIt->first);
        EXPECT_EQ(-1, foundIt->second);
        EXPECT_EQ(8, testContainer.size());
    }

    TEST_F(HashedContainers, UnorderedMapInsertNodeHandleWithEmptyNodeHandleFails)
    {
        using MapType = AZStd::unordered_map<int32_t, int32_t>;
        using node_type = typename MapType::node_type;
        using insert_return_type = typename MapType::insert_return_type;

        MapType testContainer = HashedMapConfig<MapType>::Create();

        node_type extractedNode;
        EXPECT_TRUE(extractedNode.empty());

        EXPECT_EQ(8, testContainer.size());
        insert_return_type insertResult = testContainer.insert(AZStd::move(extractedNode));
        EXPECT_EQ(testContainer.end(), insertResult.position);
        EXPECT_FALSE(insertResult.inserted);
        EXPECT_TRUE(insertResult.node.empty());
        EXPECT_EQ(8, testContainer.size());
    }

    TEST_F(HashedContainers, UnorderedMapInsertNodeHandleWithDuplicateValueInNodeHandleFails)
    {
        using MapType = AZStd::unordered_map<int32_t, int32_t>;
        using node_type = typename MapType::node_type;
        using insert_return_type = typename MapType::insert_return_type;
        MapType testContainer = HashedMapConfig<MapType>::Create();

        node_type extractedNode = testContainer.extract(0xFF);
        EXPECT_EQ(7, testContainer.size());
        EXPECT_FALSE(extractedNode.empty());
        EXPECT_EQ(0xFF, static_cast<int32_t>(extractedNode.key()));
        // Update the extracted node to have the same key as one of the elements currently within the container
        extractedNode.key() = -200;

        insert_return_type insertResult = testContainer.insert(AZStd::move(extractedNode));
        EXPECT_NE(testContainer.end(), insertResult.position);
        EXPECT_FALSE(insertResult.inserted);
        EXPECT_FALSE(insertResult.node.empty());
        EXPECT_EQ(7, testContainer.size());
    }

    TEST_F(HashedContainers, UnorderedMapExtractedNodeCanBeInsertedIntoUnorderedMultimap)
    {
        using MapType = AZStd::unordered_map<int32_t, int32_t>;
        using MultimapType = AZStd::unordered_multimap<int32_t, int32_t>;

        MapType uniqueMap{ {1, 2}, {2, 4}, {3, 6} };
        MultimapType multiMap{ {1, 2}, { 4, 8}, {1, 3} };

        auto extractedNode = uniqueMap.extract(1);
        EXPECT_EQ(2, uniqueMap.size());
        EXPECT_FALSE(extractedNode.empty());

        auto insertIt = multiMap.insert(AZStd::move(extractedNode));
        EXPECT_NE(multiMap.end(), insertIt);
        EXPECT_EQ(4, multiMap.size());
        EXPECT_EQ(3, multiMap.count(1));
    }

    TEST_F(HashedContainers, UnorderedMultimapExtractedNodeCanBeInsertedIntoUnorderedMap)
    {
        using MapType = AZStd::unordered_map<int32_t, int32_t>;
        using MultimapType = AZStd::unordered_multimap<int32_t, int32_t>;

        MapType uniqueMap{ {1, 2}, {2, 4}, {3, 6} };
        MultimapType multiMap{ {1, 2}, { 4, 8}, {1, 3} };

        auto extractedNode = multiMap.extract(4);
        EXPECT_EQ(2, multiMap.size());
        EXPECT_FALSE(extractedNode.empty());

        auto insertResult = uniqueMap.insert(AZStd::move(extractedNode));
        EXPECT_TRUE(insertResult.inserted);
        EXPECT_EQ(4, uniqueMap.size());
        EXPECT_EQ(1, uniqueMap.count(4));
    }

    TEST_F(HashedContainers, CheckADL)
    {
        // ensure this compiles and not does fail due to ADL (Koenig Lookup)
        std::vector<AZStd::unordered_map<int, int>> test_adl;

        EXPECT_TRUE(test_adl.empty());
    }

    TEST_F(HashedContainers, UnorderedMapTryEmplace_DoesNotConstruct_OnExistingKey)
    {
        static int s_tryEmplaceConstructorCallCount;
        s_tryEmplaceConstructorCallCount = 0;
        struct TryEmplaceConstructorCalls
        {
            TryEmplaceConstructorCalls()
            {
                ++s_tryEmplaceConstructorCallCount;
            }
            TryEmplaceConstructorCalls(int value)
                : m_value{ value }
            {
                ++s_tryEmplaceConstructorCallCount;
            }
            TryEmplaceConstructorCalls(const TryEmplaceConstructorCalls&)
            {
                ++s_tryEmplaceConstructorCallCount;
            }

            int m_value{};
        };
        using TryEmplaceTestMap = AZStd::unordered_map<int, TryEmplaceConstructorCalls>;
        TryEmplaceTestMap testContainer;

        // try_emplace move key
        AZStd::pair<TryEmplaceTestMap::iterator, bool> emplacePairIter = testContainer.try_emplace(1, 5);
        EXPECT_EQ(1, s_tryEmplaceConstructorCallCount);
        EXPECT_TRUE(emplacePairIter.second);
        EXPECT_EQ(5, emplacePairIter.first->second.m_value);

        // try_emplace copy key
        int testKey = 3;
        emplacePairIter = testContainer.try_emplace(testKey, 72);
        EXPECT_EQ(2, s_tryEmplaceConstructorCallCount);
        EXPECT_TRUE(emplacePairIter.second);
        EXPECT_EQ(72, emplacePairIter.first->second.m_value);

        // invoke try_emplace with hint and move key
        TryEmplaceTestMap::iterator emplaceIter = testContainer.try_emplace(testContainer.end(), 5, 4092);
        EXPECT_EQ(3, s_tryEmplaceConstructorCallCount);
        EXPECT_EQ(4092, emplaceIter->second.m_value);

        // invoke try_emplace with hint and copy key
        testKey = 48;
        emplaceIter = testContainer.try_emplace(testContainer.end(), testKey, 824);
        EXPECT_EQ(4, s_tryEmplaceConstructorCallCount);
        EXPECT_EQ(824, emplaceIter->second.m_value);

        // Since the key of '1' exist, nothing should be constructed
        emplacePairIter = testContainer.try_emplace(1, -6354);
        EXPECT_EQ(4, s_tryEmplaceConstructorCallCount);
        EXPECT_FALSE(emplacePairIter.second);
        EXPECT_EQ(5, emplacePairIter.first->second.m_value);
    }

    TEST_F(HashedContainers, UnorderedMapTryEmplace_DoesNotMoveValue_OnExistingKey)
    {
        AZStd::unordered_map<int, AZStd::unique_ptr<int>> testMap;
        auto testPtr = AZStd::make_unique<int>(5);
        auto [emplaceIter, inserted] = testMap.try_emplace(1, AZStd::move(testPtr));
        EXPECT_TRUE(inserted);
        EXPECT_EQ(nullptr, testPtr);

        testPtr = AZStd::make_unique<int>(7000);
        auto [emplaceIter2, inserted2] = testMap.try_emplace(1, AZStd::move(testPtr));
        EXPECT_FALSE(inserted2);
        ASSERT_NE(nullptr, testPtr);
        EXPECT_EQ(7000, *testPtr);
    }

    TEST_F(HashedContainers, UnorderedMapInsertOrAssign_PerformsAssignment_OnExistingKey)
    {
        static int s_tryInsertOrAssignConstructorCalls;
        static int s_tryInsertOrAssignAssignmentCalls;
        s_tryInsertOrAssignConstructorCalls = 0;
        s_tryInsertOrAssignAssignmentCalls = 0;
        struct InsertOrAssignInitCalls
        {
            InsertOrAssignInitCalls()
            {
                ++s_tryInsertOrAssignConstructorCalls;
            }
            InsertOrAssignInitCalls(int value)
                : m_value{ value }
            {
                ++s_tryInsertOrAssignConstructorCalls;
            }
            InsertOrAssignInitCalls(const InsertOrAssignInitCalls& other)
                : m_value{ other.m_value }
            {
                ++s_tryInsertOrAssignConstructorCalls;
            }

            InsertOrAssignInitCalls& operator=(int value)
            {
                m_value = value;
                ++s_tryInsertOrAssignAssignmentCalls;
                return *this;
            }

            int m_value{};
        };
        using InsertOrAssignTestMap = AZStd::unordered_map<int, InsertOrAssignInitCalls>;
        InsertOrAssignTestMap testContainer;

        // insert_or_assign move key
        AZStd::pair<InsertOrAssignTestMap::iterator, bool> insertOrAssignPairIter = testContainer.insert_or_assign(1, 5);
        EXPECT_EQ(1, s_tryInsertOrAssignConstructorCalls);
        EXPECT_EQ(0, s_tryInsertOrAssignAssignmentCalls);
        EXPECT_TRUE(insertOrAssignPairIter.second);
        EXPECT_EQ(5, insertOrAssignPairIter.first->second.m_value);

        // insert_or_assign copy key
        int testKey = 3;
        insertOrAssignPairIter = testContainer.insert_or_assign(testKey, 72);
        EXPECT_EQ(2, s_tryInsertOrAssignConstructorCalls);
        EXPECT_EQ(0, s_tryInsertOrAssignAssignmentCalls);
        EXPECT_TRUE(insertOrAssignPairIter.second);
        EXPECT_EQ(72, insertOrAssignPairIter.first->second.m_value);

        // invoke insert_or_assign with hint and move key
        InsertOrAssignTestMap::iterator insertOrAssignIter = testContainer.insert_or_assign(testContainer.end(), 5, 4092);
        EXPECT_EQ(3, s_tryInsertOrAssignConstructorCalls);
        EXPECT_EQ(0, s_tryInsertOrAssignAssignmentCalls);
        EXPECT_EQ(4092, insertOrAssignIter->second.m_value);

        // invoke insert_or_assign with hint and copy key
        testKey = 48;
        insertOrAssignIter = testContainer.insert_or_assign(testContainer.end(), testKey, 824);
        EXPECT_EQ(4, s_tryInsertOrAssignConstructorCalls);
        EXPECT_EQ(0, s_tryInsertOrAssignAssignmentCalls);
        EXPECT_EQ(824, insertOrAssignIter->second.m_value);

        // Since the key of '1' exist, only an assignment should take place
        insertOrAssignPairIter = testContainer.insert_or_assign(1, -6354);
        EXPECT_EQ(4, s_tryInsertOrAssignConstructorCalls);
        EXPECT_EQ(1, s_tryInsertOrAssignAssignmentCalls);
        EXPECT_FALSE(insertOrAssignPairIter.second);
        EXPECT_EQ(-6354, insertOrAssignPairIter.first->second.m_value);
    }

    template<template<class...> class SetTemplate>
    static void HashSetRangeConstructorSucceeds()
    {
        constexpr AZStd::string_view testView = "abc";

        SetTemplate testSet(AZStd::from_range, testView);
        EXPECT_THAT(testSet, ::testing::UnorderedElementsAre('a', 'b', 'c'));

        testSet = SetTemplate(AZStd::from_range, AZStd::vector<char>{testView.begin(), testView.end()});
        EXPECT_THAT(testSet, ::testing::UnorderedElementsAre('a', 'b', 'c'));
        testSet = SetTemplate(AZStd::from_range, AZStd::list<char>{testView.begin(), testView.end()});
        EXPECT_THAT(testSet, ::testing::UnorderedElementsAre('a', 'b', 'c'));
        testSet = SetTemplate(AZStd::from_range, AZStd::deque<char>{testView.begin(), testView.end()});
        EXPECT_THAT(testSet, ::testing::UnorderedElementsAre('a', 'b', 'c'));
        testSet = SetTemplate(AZStd::from_range, AZStd::unordered_set<char>{testView.begin(), testView.end()});
        EXPECT_THAT(testSet, ::testing::UnorderedElementsAre('a', 'b', 'c'));
        testSet = SetTemplate(AZStd::from_range, AZStd::fixed_vector<char, 8>{testView.begin(), testView.end()});
        EXPECT_THAT(testSet, ::testing::UnorderedElementsAre('a', 'b', 'c'));
        testSet = SetTemplate(AZStd::from_range, AZStd::array{ 'a', 'b', 'c' });
        EXPECT_THAT(testSet, ::testing::UnorderedElementsAre('a', 'b', 'c'));
        testSet = SetTemplate(AZStd::from_range, AZStd::span(testView));
        EXPECT_THAT(testSet, ::testing::UnorderedElementsAre('a', 'b', 'c'));
        testSet = SetTemplate(AZStd::from_range, AZStd::span(testView));
        EXPECT_THAT(testSet, ::testing::UnorderedElementsAre('a', 'b', 'c'));

        AZStd::fixed_string<8> testValue(testView);
        testSet = SetTemplate(AZStd::from_range, testValue);
        EXPECT_THAT(testSet, ::testing::UnorderedElementsAre('a', 'b', 'c'));
        testSet = SetTemplate(AZStd::from_range, AZStd::string(testView));
        EXPECT_THAT(testSet, ::testing::UnorderedElementsAre('a', 'b', 'c'));

        // Test Range views
        testSet = SetTemplate(AZStd::from_range, testValue | AZStd::views::transform([](const char elem) -> char { return elem + 1; }));
        EXPECT_THAT(testSet, ::testing::UnorderedElementsAre('b', 'c', 'd'));
    }

    template<template<class...> class MapTemplate>
    static void HashMapRangeConstructorSucceeds()
    {
        using ValueType = AZStd::pair<char, int>;
        constexpr AZStd::array testArray{ ValueType{'a', 1}, ValueType{'b', 2}, ValueType{'c', 3} };

        MapTemplate testMap(AZStd::from_range, testArray);
        EXPECT_THAT(testMap, ::testing::UnorderedElementsAre(ValueType{ 'a', 1 }, ValueType{ 'b', 2 }, ValueType{ 'c', 3 }));

        testMap = MapTemplate(AZStd::from_range, AZStd::vector<ValueType>{testArray.begin(), testArray.end()});
        EXPECT_THAT(testMap, ::testing::UnorderedElementsAre(ValueType{ 'a', 1 }, ValueType{ 'b', 2 }, ValueType{ 'c', 3 }));
        testMap = MapTemplate(AZStd::from_range, AZStd::list<ValueType>{testArray.begin(), testArray.end()});
        EXPECT_THAT(testMap, ::testing::UnorderedElementsAre(ValueType{ 'a', 1 }, ValueType{ 'b', 2 }, ValueType{ 'c', 3 }));
        testMap = MapTemplate(AZStd::from_range, AZStd::deque<ValueType>{testArray.begin(), testArray.end()});
        EXPECT_THAT(testMap, ::testing::UnorderedElementsAre(ValueType{ 'a', 1 }, ValueType{ 'b', 2 }, ValueType{ 'c', 3 }));
        testMap = MapTemplate(AZStd::from_range, AZStd::unordered_set<ValueType>{testArray.begin(), testArray.end()});
        EXPECT_THAT(testMap, ::testing::UnorderedElementsAre(ValueType{ 'a', 1 }, ValueType{ 'b', 2 }, ValueType{ 'c', 3 }));
        testMap = MapTemplate(AZStd::from_range, AZStd::fixed_vector<ValueType, 8>{testArray.begin(), testArray.end()});
        EXPECT_THAT(testMap, ::testing::UnorderedElementsAre(ValueType{ 'a', 1 }, ValueType{ 'b', 2 }, ValueType{ 'c', 3 }));
        testMap = MapTemplate(AZStd::from_range, AZStd::span(testArray));
        EXPECT_THAT(testMap, ::testing::UnorderedElementsAre(ValueType{ 'a', 1 }, ValueType{ 'b', 2 }, ValueType{ 'c', 3 }));
    }

    TEST_F(HashedContainers, RangeConstructor_Succeeds)
    {
        HashSetRangeConstructorSucceeds<AZStd::unordered_set>();
        HashSetRangeConstructorSucceeds<AZStd::unordered_multiset>();
        HashMapRangeConstructorSucceeds<AZStd::unordered_map>();
        HashMapRangeConstructorSucceeds<AZStd::unordered_multimap>();
    }

    template<template<class...> class SetTemplate>
    static void HashSetInsertRangeSucceeds()
    {
        constexpr AZStd::string_view testView = "abc";
        SetTemplate testSet{ 'd', 'e', 'f' };
        testSet.insert_range(AZStd::vector<char>{testView.begin(), testView.end()});
        testSet.insert_range(testView | AZStd::views::transform([](const char elem) -> char { return elem + 6; }));
        EXPECT_THAT(testSet, ::testing::UnorderedElementsAre('a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i'));
    }

    template<template<class...> class MapTemplate>
    static void HashMapInsertRangeSucceeds()
    {
        using ValueType = AZStd::pair<char, int>;
        constexpr AZStd::array testArray{ ValueType{'a', 1}, ValueType{'b', 2}, ValueType{'c', 3} };

        MapTemplate testMap(AZStd::from_range, testArray);

        testMap.insert_range(AZStd::vector<ValueType>{ValueType{ 'd', 4 }, ValueType{ 'e', 5 }, ValueType{ 'f', 6 }});
        EXPECT_THAT(testMap, ::testing::UnorderedElementsAre(ValueType{ 'a', 1 }, ValueType{ 'b', 2 }, ValueType{ 'c', 3 },
            ValueType{ 'd', 4 }, ValueType{ 'e', 5 }, ValueType{ 'f', 6 }));
    }

    TEST_F(HashedContainers, InsertRange_Succeeds)
    {
        HashSetInsertRangeSucceeds<AZStd::unordered_set>();
        HashSetInsertRangeSucceeds<AZStd::unordered_multiset>();
        HashMapInsertRangeSucceeds<AZStd::unordered_map>();
        HashMapInsertRangeSucceeds<AZStd::unordered_multimap>();
    }

    template <typename ContainerType>
    class HashedMapDifferentAllocatorFixture
        : public LeakDetectionFixture
    {
    };

    template<template <typename, typename, typename, typename, typename> class ContainerTemplate>
    struct HashedMapWithCustomAllocatorConfig
    {
        using ContainerType = ContainerTemplate<int32_t, int32_t, AZStd::hash<int32_t>, AZStd::equal_to<int32_t>, AZ::AZStdIAllocator>;

        static ContainerType Create(std::initializer_list<typename ContainerType::value_type> intList, AZ::IAllocator* allocatorInstance)
        {
            ContainerType allocatorMap(intList, 0, AZStd::hash<int32_t>{}, AZStd::equal_to<int32_t>{}, AZ::AZStdIAllocator{ allocatorInstance });
            return allocatorMap;
        }
    };

    using MapTemplateConfigs = ::testing::Types<
        HashedMapWithCustomAllocatorConfig<AZStd::unordered_map>
        , HashedMapWithCustomAllocatorConfig<AZStd::unordered_multimap>
    >;
    TYPED_TEST_CASE(HashedMapDifferentAllocatorFixture, MapTemplateConfigs);

#if GTEST_HAS_DEATH_TEST
#if AZ_TRAIT_DISABLE_FAILED_DEATH_TESTS
    TYPED_TEST(HashedMapDifferentAllocatorFixture, DISABLED_InsertNodeHandleWithDifferentAllocatorsLogsTraceMessages)
#else
    TYPED_TEST(HashedMapDifferentAllocatorFixture, InsertNodeHandleWithDifferentAllocatorsLogsTraceMessages)
#endif // AZ_TRAIT_DISABLE_FAILED_DEATH_TESTS
    {
        using ContainerType = typename TypeParam::ContainerType;

        ContainerType systemAllocatorMap = TypeParam::Create({ {1, 2}, {2, 4}, {3, 6} }, &AZ::AllocatorInstance<AZ::SystemAllocator>::Get());
        auto extractedNode = systemAllocatorMap.extract(1);

        ContainerType osAllocatorMap = TypeParam::Create({ {2, 4} }, &AZ::AllocatorInstance<AZ::OSAllocator>::Get());
        // Attempt to insert an extracted node from a container using the AZ::SystemAllocator into a container using the AZ::OSAllocator
        EXPECT_DEATH(
        {
            UnitTest::TestRunner::Instance().StartAssertTests();
            osAllocatorMap.insert(AZStd::move(extractedNode));
            if (UnitTest::TestRunner::Instance().StopAssertTests() > 0)
            {
                // AZ_Assert does not cause the application to exit in profile_test configuration
                // Therefore an exit with a non-zero error code is invoked to trigger the death condition
                exit(1);
            }
        } , ".*");
    }
#endif // GTEST_HAS_DEATH_TEST

    namespace HashedContainerTransparentTestInternal
    {
        static int s_allConstructorCount;
        static int s_allAssignmentCount;

        struct TrackConstructorCalls
        {
            TrackConstructorCalls()
            {
                ++s_allConstructorCount;
            }
            TrackConstructorCalls(int value)
                : m_value(value)
            {
                ++s_allConstructorCount;
            }
            TrackConstructorCalls(const TrackConstructorCalls& other)
                : m_value(other.m_value)
            {
                ++s_allConstructorCount;
            };
            TrackConstructorCalls& operator=(const TrackConstructorCalls& other)
            {
                ++s_allAssignmentCount;
                m_value = other.m_value;
                return *this;
            }

            operator int() const
            {
                return m_value;
            }

            int m_value{}; // Used for comparison
        };

        bool operator==(const TrackConstructorCalls& lhs, const TrackConstructorCalls& rhs)
        {
            return lhs.m_value == rhs.m_value;
        }
        bool operator==(int lhs, const TrackConstructorCalls& rhs)
        {
            return lhs== rhs.m_value;
        }
        bool operator==(const TrackConstructorCalls& lhs, int rhs)
        {
            return lhs.m_value == rhs;
        }
    }
}

namespace AZStd
{
    using is_transparent = void;
    template<>
    struct hash<UnitTest::HashedContainerTransparentTestInternal::TrackConstructorCalls>
    {
        using is_transparent = void;
        template<typename ConvertableToKey>
        constexpr size_t operator()(const ConvertableToKey& key) const
        {
            return static_cast<int>(key);
        }
    };
}
namespace UnitTest
{
    template<typename Container>
    struct HashedContainerTransparentConfig
    {
        using ContainerType = Container;
    };
    template <typename ContainerType>
    class HashedContainerTransparentFixture
        : public LeakDetectionFixture
    {
    protected:
        void SetUp() override
        {
            HashedContainerTransparentTestInternal::s_allConstructorCount = 0;
            HashedContainerTransparentTestInternal::s_allAssignmentCount = 0;
        }
        void TearDown() override
        {
            HashedContainerTransparentTestInternal::s_allConstructorCount = 0;
            HashedContainerTransparentTestInternal::s_allAssignmentCount = 0;
        }
    };

    using HashedContainerConfigs = ::testing::Types <
        HashedContainerTransparentConfig<
            AZStd::unordered_set<HashedContainerTransparentTestInternal::TrackConstructorCalls, AZStd::hash<HashedContainerTransparentTestInternal::TrackConstructorCalls>, AZStd::equal_to<>>
        >
        , HashedContainerTransparentConfig<
            AZStd::unordered_multiset<HashedContainerTransparentTestInternal::TrackConstructorCalls, AZStd::hash<HashedContainerTransparentTestInternal::TrackConstructorCalls>, AZStd::equal_to<>>
        >
        , HashedContainerTransparentConfig<
            AZStd::unordered_map<HashedContainerTransparentTestInternal::TrackConstructorCalls, int, AZStd::hash<HashedContainerTransparentTestInternal::TrackConstructorCalls>, AZStd::equal_to<>>
        >
        , HashedContainerTransparentConfig<
            AZStd::unordered_multimap<HashedContainerTransparentTestInternal::TrackConstructorCalls, int, AZStd::hash<HashedContainerTransparentTestInternal::TrackConstructorCalls>, AZStd::equal_to<>>
        >
    >;

    TYPED_TEST_CASE(HashedContainerTransparentFixture, HashedContainerConfigs);

    TYPED_TEST(HashedContainerTransparentFixture, FindDoesNotConstructKeyForTransparentHashEqual_NoKeyConstructed_Succeeds)
    {
        typename TypeParam::ContainerType container;
        container.emplace(1);
        container.emplace(2);
        container.emplace(3);
        container.emplace(4);
        container.emplace(5);
        container.emplace(1);

        // Reset constructor count to zero
        HashedContainerTransparentTestInternal::s_allConstructorCount = 0;

        // The following call will construct a TrackConstructorCalls element
        auto foundIt = container.find(HashedContainerTransparentTestInternal::TrackConstructorCalls{ 2 });
        EXPECT_EQ(1, HashedContainerTransparentTestInternal::s_allConstructorCount);
        EXPECT_NE(container.end(), foundIt);

        // The transparent find function should be invoked and no constructor of TrackConstructorCallsElement should be invoked
        foundIt = container.find(1);
        // The ConstructorCount should still be the same
        EXPECT_EQ(1, HashedContainerTransparentTestInternal::s_allConstructorCount);
        EXPECT_NE(container.end(), foundIt);

        // When find fails to locate an element, TrackConstructorCallsElement shouldn't be construction
        foundIt = container.find(-1);
        EXPECT_EQ(1, HashedContainerTransparentTestInternal::s_allConstructorCount);
        EXPECT_EQ(container.end(), foundIt);

        // No assignments should have taken place from any of the container operations
        EXPECT_EQ(0, HashedContainerTransparentTestInternal::s_allAssignmentCount);
    }

    TYPED_TEST(HashedContainerTransparentFixture, ContainsDoesNotConstructKeyForTransparentHashEqual_NoKeyConstructed_Succeeds)
    {
        typename TypeParam::ContainerType container;
        container.emplace(1);
        container.emplace(2);
        container.emplace(3);
        container.emplace(4);
        container.emplace(5);
        container.emplace(1);

        // Reset constructor count to zero
        HashedContainerTransparentTestInternal::s_allConstructorCount = 0;

        // The following call will construct a TrackConstructorCalls element
        EXPECT_TRUE(container.contains(HashedContainerTransparentTestInternal::TrackConstructorCalls{ 2 }));
        EXPECT_EQ(1, HashedContainerTransparentTestInternal::s_allConstructorCount);


        // The transparent contain function should be invoked and no constructor of TrackConstructorCallsElement should be invoked
        EXPECT_TRUE(container.contains(2));
        // The ConstructorCount should still be the same
        EXPECT_EQ(1, HashedContainerTransparentTestInternal::s_allConstructorCount);

        // In the case when the element isn't found, no constructor shouldn't be invoked as well
        EXPECT_FALSE(container.contains(27));
        EXPECT_EQ(1, HashedContainerTransparentTestInternal::s_allConstructorCount);
        // No assignments should have taken place from any of the container operations
        EXPECT_EQ(0, HashedContainerTransparentTestInternal::s_allAssignmentCount);
    }

    TYPED_TEST(HashedContainerTransparentFixture, CountDoesNotConstructKeyForTransparentHashEqual_NoKeyConstructed_Succeeds)
    {
        typename TypeParam::ContainerType container;
        container.emplace(1);
        container.emplace(2);
        container.emplace(3);
        container.emplace(4);
        container.emplace(5);
        container.emplace(1);

        // Reset constructor count to zero
        HashedContainerTransparentTestInternal::s_allConstructorCount = 0;

        // The following call will construct a TrackConstructorCalls element
        EXPECT_EQ(1, container.count(HashedContainerTransparentTestInternal::TrackConstructorCalls{ 2 }));
        EXPECT_EQ(1, HashedContainerTransparentTestInternal::s_allConstructorCount);


        // The transparent count function should be invoked and no constructor of TrackConstructorCallsElement should be invoked
        EXPECT_EQ(1, container.count(2));
        // The ConstructorCount should still be the same
        EXPECT_EQ(1, HashedContainerTransparentTestInternal::s_allConstructorCount);


        // In the case when the element isn't found, no constructor shouldn't be invoked as well
        EXPECT_EQ(0, container.count(27));
        EXPECT_EQ(1, HashedContainerTransparentTestInternal::s_allConstructorCount);
        // No assignments should have taken place from any of the container operations
        EXPECT_EQ(0, HashedContainerTransparentTestInternal::s_allAssignmentCount);
    }

    TYPED_TEST(HashedContainerTransparentFixture, LowerBoundDoesNotConstructKeyForTransparentHashEqual_NoKeyConstructed_Succeeds)
    {
        typename TypeParam::ContainerType container;
        container.emplace(21);
        container.emplace(4);
        container.emplace(3);
        container.emplace(7);
        container.emplace(15);
        container.emplace(42);

        // Lower Bound and Upper Bound doesn't make any sense for an Unordered Container, because there is no ordering
        // So the values returned from those calls are non-nonsensical.
        // i.e invoking upper_bound(16) with current data could return an iterator to 332, an iterator 1, or even the end iterator()
        // depending on how the data is stored in the node based hash table
        // Reset constructor count to zero
        HashedContainerTransparentTestInternal::s_allConstructorCount = 0;

        // The following call will construct a TrackConstructorCalls element
        auto foundIt = container.lower_bound(HashedContainerTransparentTestInternal::TrackConstructorCalls{ 4 });
        EXPECT_EQ(1, HashedContainerTransparentTestInternal::s_allConstructorCount);

        // The transparent lower_bound function should be invoked and no constructor of TrackConstructorCallsElement should be invoked
        foundIt = container.lower_bound(7);
        // The ConstructorCount should still be the same
        EXPECT_EQ(1, HashedContainerTransparentTestInternal::s_allConstructorCount);

        // No assignments should have taken place from any of the container operations
        EXPECT_EQ(0, HashedContainerTransparentTestInternal::s_allAssignmentCount);
    }

    TYPED_TEST(HashedContainerTransparentFixture, UpperBoundDoesNotConstructKeyForTransparentHashEqual_NoKeyConstructed_Succeeds)
    {
        typename TypeParam::ContainerType container;
        container.emplace(2);
        container.emplace(4);
        container.emplace(15);
        container.emplace(332);
        container.emplace(5);
        container.emplace(1);

        // Lower Bound and Upper Bound doesn't make any sense for an Unordered Container, because there is no ordering
        // So the values returned from those calls are non-nonsensical.
        // i.e invoking upper_bound(16) with current data could return an iterator to 332, an iterator 1, or even the end iterator()
        // depending on how the data is stored in the node based hash table
        // Reset constructor count to zero
        HashedContainerTransparentTestInternal::s_allConstructorCount = 0;

        // The following call will construct a TrackConstructorCalls element
        auto foundIt = container.upper_bound(HashedContainerTransparentTestInternal::TrackConstructorCalls{ 15 });
        EXPECT_EQ(1, HashedContainerTransparentTestInternal::s_allConstructorCount);

        // The transparent upper_bound function should be invoked and no constructor of TrackConstructorCallsElement should be invoked
        foundIt = container.upper_bound(5);
        // The ConstructorCount should still be the same
        EXPECT_EQ(1, HashedContainerTransparentTestInternal::s_allConstructorCount);

        // No assignments should have taken place from any of the container operations
        EXPECT_EQ(0, HashedContainerTransparentTestInternal::s_allAssignmentCount);
    }

    TYPED_TEST(HashedContainerTransparentFixture, EqualRangeDoesNotConstructKeyForTransparentHashEqual_NoKeyConstructed_Succeeds)
    {
        typename TypeParam::ContainerType container;
        container.emplace(-2352);
        container.emplace(3534);
        container.emplace(535408957);
        container.emplace(1310556522);
        container.emplace(55546193);
        container.emplace(1582);

        // Reset constructor count to zero
        HashedContainerTransparentTestInternal::s_allConstructorCount = 0;

        // The following call will construct a TrackConstructorCalls element
        auto foundRange = container.equal_range(HashedContainerTransparentTestInternal::TrackConstructorCalls{ -2352 });
        EXPECT_EQ(1, HashedContainerTransparentTestInternal::s_allConstructorCount);
        EXPECT_NE(container.end(), foundRange.first);
        EXPECT_NE(foundRange.first, foundRange.second);

        // The transparent upper_bound function should be invoked and no constructor of TrackConstructorCallsElement should be invoked
        foundRange = container.equal_range(55546193);
        // The ConstructorCount should still be the same
        EXPECT_EQ(1, HashedContainerTransparentTestInternal::s_allConstructorCount);
        EXPECT_NE(foundRange.first, foundRange.second);

        // In the case when the element isn't found, no constructor shouldn't be invoked as well
        foundRange = container.equal_range(0x9034);
        EXPECT_EQ(1, HashedContainerTransparentTestInternal::s_allConstructorCount);
        EXPECT_EQ(foundRange.first, foundRange.second);

        // No assignments should have taken place from any of the container operations
        EXPECT_EQ(0, HashedContainerTransparentTestInternal::s_allAssignmentCount);
    }

#if defined(HAVE_BENCHMARK)
    template <template <typename...> class Hash>
    void Benchmark_Lookup(benchmark::State& state)
    {
        const int count = 1024;
        Hash<int, int, AZStd::hash<int>, AZStd::equal_to<int>, AZStd::allocator> map;
        for (int i = 0; i < count; ++i)
        {
            map.emplace(i, i);
        }
        int val = 0;
        while (state.KeepRunning())
        {
            val += map[rand() % count];
        }
    }

    template <template <typename...> class Hash>
    void Benchmark_Insert(benchmark::State& state)
    {
        const int count = 1024;
        Hash<int, int, AZStd::hash<int>, AZStd::equal_to<int>, AZStd::allocator> map;
        int i = 0;
        while (state.KeepRunning())
        {
            i = (i == count) ? 0 : i + 1;
            map[i] = i;
        }
    }

    template <template <typename ...> class Hash>
    void Benchmark_Erase(benchmark::State& state)
    {
        const int count = 1024;
        Hash<int, int, AZStd::hash<int>, AZStd::equal_to<int>, AZStd::allocator> map;
        for (int i = 0; i < count; ++i)
        {
            map.emplace(i, i);
        }
        while (state.KeepRunning())
        {
            int val = rand() % count;
            map.erase(val);
        }
    }

    template <template <typename ...> class Hash>
    void Benchmark_Thrash(benchmark::State& state)
    {
        const int choices = 100;
        Hash<int, int, AZStd::hash<int>, AZStd::equal_to<int>, AZStd::allocator> map;
        size_t ops = 0;
        while (state.KeepRunning())
        {
            int val = rand() % choices;
            switch (rand() % 3)
            {
                case 0:
                    ops += map.emplace(val, val).second;
                    break;
                case 1:
                    ops += map.erase(val);
                    break;
                case 2:
                    ops += map.find(val) == map.end();
                    break;
            }
        }
    }

    void Benchmark_UnorderedMapLookup(benchmark::State& state)
    {
        Benchmark_Lookup<AZStd::unordered_map>(state);
    }
    BENCHMARK(Benchmark_UnorderedMapLookup);

    void Benchmark_UnorderedMapInsert(benchmark::State& state)
    {
        Benchmark_Insert<AZStd::unordered_map>(state);
    }
    BENCHMARK(Benchmark_UnorderedMapInsert);

    void Benchmark_UnorderedMapErase(benchmark::State& state)
    {
        Benchmark_Erase<AZStd::unordered_map>(state);
    }
    BENCHMARK(Benchmark_UnorderedMapErase);

    void Benchmark_UnorderedMapThrash(benchmark::State& state)
    {
        Benchmark_Thrash<AZStd::unordered_map>(state);
    }
    BENCHMARK(Benchmark_UnorderedMapThrash);
#endif
} // namespace UnitTest

#if defined(HAVE_BENCHMARK)
namespace Benchmark
{
    const int kNumInsertions = 10000;
    const int kModuloForDuplicates = 10;

    struct A
    {
        int m_int = 0;
    };

    using UnorderedMap = AZStd::unordered_map<int, A>;
    using AZStd::make_pair;

    // BM_UnorderedMap_InsertUniqueViaXXX: benchmark filling a map with unique values
    static void BM_UnorderedMap_InsertUniqueViaInsert(::benchmark::State& state)
    {
        while (state.KeepRunning())
        {
            UnorderedMap map;
            for (int mapKey = 0; mapKey < kNumInsertions; ++mapKey)
            {
                A& a = map.insert(make_pair(mapKey, A{})).first->second;
                a.m_int += 1;
            }
        }
    }
    BENCHMARK(BM_UnorderedMap_InsertUniqueViaInsert);

    static void BM_UnorderedMap_InsertUniqueViaEmplace(::benchmark::State& state)
    {
        while (state.KeepRunning())
        {
            UnorderedMap map;
            for (int mapKey = 0; mapKey < kNumInsertions; ++mapKey)
            {
                A& a = map.emplace(mapKey, A()).first->second;
                a.m_int += 1;
            }
        }
    }
    BENCHMARK(BM_UnorderedMap_InsertUniqueViaEmplace);

    static void BM_UnorderedMap_InsertUniqueViaBracket(::benchmark::State& state)
    {
        while (state.KeepRunning())
        {
            UnorderedMap map;
            for (int mapKey = 0; mapKey < kNumInsertions; ++mapKey)
            {
                A& a = map[mapKey];
                a.m_int += 1;
            }
        }
    }
    BENCHMARK(BM_UnorderedMap_InsertUniqueViaBracket);

    // BM_UnorderedMap_InsertDuplicatesViaXXX: benchmark filling a map with keys that repeat
    static void BM_UnorderedMap_InsertDuplicatesViaInsert(::benchmark::State& state)
    {
        while (state.KeepRunning())
        {
            UnorderedMap map;
            for (int mapKey = 0; mapKey < kNumInsertions; ++mapKey)
            {
                A& a = map.insert(make_pair(mapKey % kModuloForDuplicates, A())).first->second;
                a.m_int += 1;
            }
        }
    }
    BENCHMARK(BM_UnorderedMap_InsertDuplicatesViaInsert);

    static void BM_UnorderedMap_InsertDuplicatesViaEmplace(::benchmark::State& state)
    {
        while (state.KeepRunning())
        {
            UnorderedMap map;
            for (int mapKey = 0; mapKey < kNumInsertions; ++mapKey)
            {
                A& a = map.emplace(mapKey % kModuloForDuplicates, A{}).first->second;
                a.m_int += 1;
            }
        }
    }
    BENCHMARK(BM_UnorderedMap_InsertDuplicatesViaEmplace);

    static void BM_UnorderedMap_InsertDuplicatesViaBracket(::benchmark::State& state)
    {
        while (state.KeepRunning())
        {
            UnorderedMap map;
            for (int mapKey = 0; mapKey < kNumInsertions; ++mapKey)
            {
                A& a = map[mapKey % kModuloForDuplicates];
                a.m_int += 1;
            }
        }
    }
    BENCHMARK(BM_UnorderedMap_InsertDuplicatesViaBracket);

} // namespace Benchmark
#endif // HAVE_BENCHMARK
