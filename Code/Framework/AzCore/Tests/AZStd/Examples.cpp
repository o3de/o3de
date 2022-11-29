/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "UserTypes.h"

#include <AzCore/std/typetraits/typetraits.h>

#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/semaphore.h>
#include <AzCore/std/parallel/lock.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/deque.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/containers/fixed_list.h>
#include <AzCore/std/containers/intrusive_list.h>
#include <AzCore/std/containers/queue.h>
#include <AzCore/std/containers/stack.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>

namespace UnitTest
{
    using namespace AZStd;
    using namespace UnitTestInternal;
    class TypeTraitExamples
    {
    public:
        void run()
        {
            // TypeTraitExample-Begin

            // Checks the alignment of different types.
            AZ_TEST_STATIC_ASSERT(alignment_of<int>::value == 4);
            AZ_TEST_STATIC_ASSERT(alignment_of<char>::value == 1);
            AZ_TEST_STATIC_ASSERT(alignment_of<MyClass>::value == 16);

            // aligned_storage example
            // Create an int type aligned on 32 bytes.
            typedef aligned_storage<sizeof(int), 32>::type int_aligned32_type;
            AZ_TEST_STATIC_ASSERT((alignment_of<int_aligned32_type>::value) == 32);

            // aligned_storage example
            // Declare a buffer of 100 bytes, aligned on 16 bytes. (don't use more than 16 bytes alignment on the stack. It doesn't work on all platforms.)
            typedef aligned_storage<100, 16>::type buffer100_16alinged_type;
            // Check that our type is aligned on 16 bytes
            AZ_TEST_STATIC_ASSERT((alignment_of< buffer100_16alinged_type>::value) == 16);
            buffer100_16alinged_type myAlignedBuffer;
            // Make sure the buffer pointer is aligned to 16 bytes.
            AZ_TEST_ASSERT((AZStd::size_t(&myAlignedBuffer) & 15) == 0);

            // POD
            // Checks if a type is POD (Plain Old Data).
            AZ_TEST_STATIC_ASSERT(is_pod<MyStruct>::value == true);
            AZ_TEST_STATIC_ASSERT(is_pod<MyClass>::value == false);

            // TypeTraitExample-End
        }
    };

    TEST(Allocator, Examples)
    {
        // AllocatorExamples-Begin

        // Sharing allocator between containers
        {
            // I will use a 16 KB static_buffer_allocator (on the stack) for this sample. You can use any of your own allocators
            // unless they don't already point to your memory manager (which is the common way to use STL allocators)
            typedef static_buffer_allocator<16*1024, 16> static_buffer_16KB_aligned16;
            static_buffer_16KB_aligned16 bufferAllocator;

            typedef allocator_ref<static_buffer_16KB_aligned16> static_buffer_16KB_aligned16_ref;

            static_buffer_16KB_aligned16_ref sharedAllocator(bufferAllocator);

            // All containers will allocator from the same buffer. Here it is not
            // important that we will never actually free the data because of the static_buffer_allocator.
            // But if we consider that fact this is great example for some temporary containers, when
            // we don't want to even involve any memory managers.
            vector<int, static_buffer_16KB_aligned16_ref>     int_vector(sharedAllocator);
            list<float, static_buffer_16KB_aligned16_ref>     float_list(sharedAllocator);
            deque<MyClass, static_buffer_16KB_aligned16_ref>  myclass_deque(sharedAllocator);
        }

        // AllocatorExamples-End
    }

    class ContainersExamples
        : public AllocatorsFixture
    {
    public:
        void Array()
        {
            // ArrayExamples-Begin

            // Array class is like a regular C array, but if gives it container functionality.
            // All elements are initialized, when the array is created.

            // Create an array of 5 ints. All elements will be uninitialized (they will call the default ctor)
            array<int, 5> int10_uninit_array;

            // Array of 5 ints initialized to some values.
            array<int, 5> int10_init_array = {
                {1, 2, 3, 4, 5}
            };
            // just check if the first element is 1 and last is 5
            AZ_TEST_ASSERT(int10_init_array[0] == 1);
            AZ_TEST_ASSERT(int10_init_array[4] == 5);

            // set all elements to 11
            int10_init_array.fill(11);
            AZ_TEST_ASSERT(int10_init_array[0] == 11);
            AZ_TEST_ASSERT(int10_init_array[4] == 11);

            // Create an array of my class with default init.
            array<MyClass, 5> myclass_array;
            // default value for MyClass::m_data is 10, verify this.
            AZ_TEST_ASSERT(myclass_array[0].m_data == 10);
            AZ_TEST_ASSERT(myclass_array[4].m_data == 10);
            // My class pointer should be aligned on 32 bytes so verify this too.
            AZ_TEST_ASSERT((((AZStd::size_t)&myclass_array[0]) & (alignment_of<MyClass>::value - 1)) == 0);

            // ArrayExamples-End

            (void)int10_uninit_array;
        }

        void Vector()
        {
            // VectorExamples-Begin

            // Int vector using the default allocator
            typedef vector<int> int_vector_type;

            // 100 constant elements.
            {
                // Bad way, lot's of allocations and slow
                int_vector_type int_vec1;
                for (int i = 0; i < 100; ++i)
                {
                    int_vec1.push_back(10);
                }

                // Correct ways...
                int_vector_type int_vec2(100, 10); // Best way
                int_vec1.resize(100, 10); // Similar with a few more function calls
                int_vec1.assign(100, 10); // Similar with even more function calls
            }

            // 100 random values (0 to 99 in this example)
            {
                // Bad way, lot's of allocation and slow
                int_vector_type int_vec1;
                for (int i = 0; i < 100; ++i)
                {
                    int_vec1.push_back(i);
                }

                // Bad way, one allocation but pointless copies
                int_vec1.resize(100, 0 /*reset to 0*/); // one allocation, but sets all the values to 0
                for (int i = 0; i < 100; ++i)
                {
                    int_vec1[i] = i;
                }

                // Bad because it's tricky (sometimes correct)
                int_vec1.resize(100); // AZStd extension. This will work fast only for POD data types (like this int is), otherwise it will call default ctor.
                for (int i = 0; i < 100; ++i)
                {
                    int_vec1[i] = i;
                }

                // Correct way
                int_vec1.reserve(100); // as part of the standard or you can use AZStd extension set_capacity(), this will trim is down if necessary
                for (int i = 0; i < 100; ++i)
                {
                    int_vec1.push_back(i);
                }
            }

            // Copy values from other containers
            {
                int_vector_type int_vec1(100, 10);
                int_vector_type int_vec2;

                // Bad ways

                // Slow with many allocations
                for (int_vector_type::size_type i = 0; i < int_vec1.size(); ++i)
                {
                    int_vec2.push_back(int_vec1[i]);
                }

                // Correct if it's the same type
                int_vector_type int_vec3(int_vec1);
                int_vector_type int_vec4 = int_vec1;

                // Correct from different types
                list<int> int_list(10, 10);
                array<int, 4> int_array = {
                    {1, 2, 3, 4}
                };
                // C array
                int  int_carray[] = {1, 2, 3, 4};

                int_vector_type int_vec5(int_list.begin(), int_list.end());
                int_vector_type int_vec6(int_array.begin(), int_array.end());
                int_vector_type int_vec7(&int_carray[0], &int_carray[4]);
            }

            //
            // As you know from STL avoid using insert and erase on a vector, since they are slow operations.
            //

            // Clearing a container
            {
                int_vector_type int_vec1(100, 55); // 100 elements, 55 value

                // Bad way
                while (!int_vec1.empty())
                {
                    int_vec1.pop_back();
                }

                // Correct ways
                int_vec1.clear();
                int_vec1.erase(int_vec1.begin(), int_vec1.end());  // a few more function calls than clear

                // If you want to clear and make sure we free the memory.
                int_vec1.set_capacity(0); // AZStd extension.
            }

            // Exchanging the content of 2 vectors
            {
                int_vector_type int_vec1(100, 10);
                int_vector_type int_vec2(10, 11);

                // Only one way is correct, everything else is BAD (even if the allocators are different, it will do the proper job as fast as possible).
                int_vec1.swap(int_vec2);
            }

            // Quick tear-down (leak_and_reset extension)
            {
                // Assuming you have your own temporary allocators, I will use static_buffer_allocator for this sample.
                // \note this allocator already instruct the vector the he doesn't need to delete it's memory
                typedef static_buffer_allocator<16*1024, 1> static_buffer_16KB;

                // Add 100 elements on the stack
                // and YES having fixed_vector<int, (16*1024)/sizeof(T) > is the same.
                vector<int, static_buffer_16KB> tempVector(100, 10);

                // .. do some crazy operations, sorting whatever...

                // clearing (when the can afford to NOT call the destructor - it will not leak or something), otherwise just use the regular functions.

                // Bad ways, although all the bad way will be work as fast for POD data types, we consider them tricky, because you rely on the vector value_type.
                tempVector.clear();  // even it will not free any memory, it will call  if the type is not POD.
                tempVector.erase(tempVector.begin(), tempVector.end());
                tempVector.set_capacity(0);

                // Correct way to NOT call the dtor.
                // IMPORTANT: Leak and reset can be used on normal vectors too for instance if you have garbage collector, you are
                // just exiting the process and rely on somebody else to clean after you.
                tempVector.leak_and_reset();
            }

            // Allocators
            {
                // I will use static_buffer_allocator for this sample.
                typedef static_buffer_allocator<16*1024, 1> static_buffer_16KB;

                static_buffer_16KB otherAllocator;

                // change allocator name
                // All of this depends if your allocator assignment is slow/expensive. Otherwise this is valid code
                vector<int, static_buffer_16KB> int_vec1(100, 10, static_buffer_16KB());

                // Changing the allocator, will force the vector to re-allocate itself, if it has any elements.
                int_vec1.set_allocator(otherAllocator);

                // As in the allocators sample, you can share an allocator if you the allocator_ref.
                typedef allocator_ref<static_buffer_16KB> static_buffer_16KB_ref;

                // Both int_vec2 and int_vec3 will allocate from the otherAllocator.
                static_buffer_16KB_ref sharedAlloc(otherAllocator);
                vector<int, static_buffer_16KB_ref> int_vec2(sharedAlloc);
                vector<int, static_buffer_16KB_ref> int_vec3(sharedAlloc);

                // using the  container allocator, for other purpose... allocate 100 bytes on 16 byte alignment
                void* myData = int_vec2.get_allocator().allocate(100, 16);

                // do something...

                // free if you should, in the static_buffer_allocator you should not care about this.
                int_vec2.get_allocator().deallocate(myData, 100, 16);
            }

            // VectorExamples-End
        }

        void List()
        {
            // ListExamples-Begin

            // Use the list node type to pre allocate memory pools.
            {
                // One of the futures of the AZStd containers, that we node allocation type for each container (not only the list).
                // This allows us to know at compile time the size of the allocations (vector class is exception).
                // This example if very similar to what this fixed_list container does.

                // Some let's create pool for int list nodes.
                typedef static_pool_allocator< list<int>::node_type, 1000 > int_list_pool_allocator_type;
                typedef allocator_ref<int_list_pool_allocator_type> int_pool_alloc_ref_type;
                int_list_pool_allocator_type myPool;
                int_pool_alloc_ref_type myPoolRef(myPool);

                // Now we want to share that pool in multiple containers.
                list<int, int_pool_alloc_ref_type>  int_list(myPoolRef);
                list<int, int_pool_alloc_ref_type>  int_list1(myPoolRef);

                // in addition we can use the pool to allocate nodes that a smaller than the int type.
                list<char, int_pool_alloc_ref_type> char_list(myPoolRef);
                list<short, int_pool_alloc_ref_type> short_list(myPoolRef);

                // Now all of the above containers will allocate from the list<int> pool.

                int_list.assign(10, 202);
                AZ_TEST_ASSERT(int_list.size() == 10);
                AZ_TEST_ASSERT(int_list.front() == 202);
                int_list1.assign(10, 302);
                AZ_TEST_ASSERT(int_list1.size() == 10);
                AZ_TEST_ASSERT(int_list1.front() == 302);
                char_list.assign(30, (char)120);
                AZ_TEST_ASSERT(char_list.size() == 30);
                AZ_TEST_ASSERT(char_list.front() == 120);
                short_list.assign(20, (short)32000);
                AZ_TEST_ASSERT(short_list.size() == 20);
                AZ_TEST_ASSERT(short_list.front() == 32000);

                // Now after we did some work with the containers, we can tear them down faster. Which is another example of the
                // use of leak_and_reset.
                // If you look at the static_pool_allocator allocator, you will notice that the deallocate function returns the allocated node
                // to the pool. At this moment we don't really want to do that since we will not use this pool anymore and the memory will be free once we
                // destroy the pool. On the other hand we use integral type (which is POD types) and we don't need to worry about the destructor at all... so to be fast
                // instead of deallocating each node on it's own.
                int_list.leak_and_reset();
                int_list1.leak_and_reset();
                char_list.leak_and_reset();
                short_list.leak_and_reset();

                myPool.leak_before_destroy(); // tell the pool it's ok that we have allocated nodes.
            }

            // ListExamples-End
        }

        void Deque()
        {
            // DequeExamples-Begin

            // Customize the deque so it fits better our allocation needs
            {
                // Specialize the deque so we do allocate 20 int at in a block. If you look at the default
                // settings you can see for 4 byte types we will allocate blocks with 4 elements. 20 can be a little wasteful,
                // but a lot lass allocations will happen.
                deque<int, AZStd::allocator, 20>  int_deque;

                int_deque.push_back(10);
                int_deque.push_front(11);
                AZ_TEST_ASSERT(int_deque.size() == 2);
                AZ_TEST_ASSERT(int_deque.front() == 11);
                AZ_TEST_ASSERT(int_deque.back() == 10);
            }

            // DequeExamples-End
        }

        void Hashed()
        {
#ifdef AZ_PLATFORM_WINDOWS // Just to make sure examples actually work
            // UnorderedExamples-Begin

            // Advanced examples. This examples may be a little hard to read or understand if you are familiar with
            // way Hashed containers work. Read about hash_table and check the papers references.
            // Keep in mind that this customizations and speed ups should be used if you really know what they do, and
            // you really need it!
            // You should try to be complaint with the standard wherever possible, to avoid problems
            // if you use other STL. In 99% of the cases copying keys and value types is very fast or doesn't happed as often at all.
            // So using this you will make the code look more complicated and not compatible with the standard for no any real benefit.
            // Of course if you use these containers in rendering code for example and you insert hundreds (or even thousands) entries every frame,
            // these examples might help a lot.

            //////////////////////////////////////////////////////////////////////////
            // Some example classes we use
            struct MyExpensiveKeyType
            {
                MyExpensiveKeyType()
                    : m_keyData(0)     { /* expensive operations */ }
                MyExpensiveKeyType(int data)
                    : m_keyData(data)      { /* expensive operations */ }
                AZStd::size_t   GetHashed() const   { return m_keyData; /* just some hashing function */ }
                bool            IsEqual(const MyExpensiveKeyType& rhs) const        { return m_keyData == rhs.m_keyData; }

                int m_keyData;
            };

            // KeyHasher
            struct MyExpensiveKeyHasher
            {
                AZStd::size_t operator()(const MyExpensiveKeyType& k) const
                {
                    return k.GetHashed();
                }
            };

            // KeyTypeCompare
            struct MyExpensiveKeyEqualTo
            {
                AZ_FORCE_INLINE bool operator()(const MyExpensiveKeyType& left, const MyExpensiveKeyType& right) const { return left.IsEqual(right); }
                // We use this class to compare the AZStd::size_t to our key type.
                AZ_FORCE_INLINE bool operator()(const AZStd::size_t leftKey, const MyExpensiveKeyType& right) const { return (int)leftKey == right.m_keyData; }
            };

            // Map expensive value type.
            class MyExpensiveValueType
            {
            public:
                MyExpensiveValueType()
                    : m_data(0) { /* expensive operations */ }
                MyExpensiveValueType(int data)
                    : m_data(data) { /* expensive operations */ }

            private:
                [[maybe_unused]] int m_data;
            };
            //////////////////////////////////////////////////////////////////////////

            // Customization for expensive value type.
            {
                // Let's say the MyExpensiveValueType is expensive to construct. Ex. Does allocations register itself in systems, etc.
                // so if we have the map with it.
                // - In many cases people try to avoid this by storing pointer in the container
                // so the value type is not expensive to move around, but even if this works. It's not the idea of the container.
                typedef unordered_map<int, MyExpensiveValueType> myclass_map_type;
                myclass_map_type myMap;
                int myNewClassKey = 100;

                // So to use the regular insert, we need to create temp pair. Even if the key exists, or we don't really have a source for
                // for MyClass. Then this insert can be a big overhead.
                // - People sometimes try to fix this problem by calling myMap.find(myNewClassKey) to see if it's there and then make
                // the pair, this is ok. But you do the find 2 times... if it's not there. Even time is constant time operation O(1) it's pointless.
                myclass_map_type::value_type tempPair = AZStd::make_pair(myNewClassKey, MyExpensiveValueType());
                myMap.insert(tempPair);

                // When we don't have MyClass source value, a call to the extension insert_key() will to the job.
                myMap.insert_key(myNewClassKey); // So if the key doesn't't exist it will insert pair with default value. Which is g).
            }


            // Customization for expensive key type
            {
                // Like the example with word counting... in the 14CrazyIdeas paper. Sometimes the key can be expensive.
                typedef unordered_set<MyExpensiveKeyType, MyExpensiveKeyHasher, MyExpensiveKeyEqualTo> myclass_set_type;
                myclass_set_type mySet;

                // Let say as in the above example MyClass is expensive to construct, copy, etc. But we know can know the search key (we use hash
                // values for the unordered set).
                AZStd::size_t myNewClassKey = 101; // So we know the key (that hash_function(MyClass) will return, a good practical example is when you have string literal and you can get the hash without making string object)

                // This way you don't need to make the expensive MyClass object at all. This the situation is the same for maps. We need
                // to provide functions how to compare AZStd::size_t (in this case) to MyExpensiveKeyType (in this case). Which MyExpensiveKeyEqualTo class does.
                mySet.find_as(myNewClassKey, AZStd::hash<AZStd::size_t>(), MyExpensiveKeyEqualTo());
            }

            // Customization for expensive key or value with non default ctor
            {
                // This is most complicated of advanced example. It shows how to customize the insert function for both complex key and complex value types.
                typedef unordered_map<MyExpensiveKeyType, MyExpensiveValueType, MyExpensiveKeyHasher, MyExpensiveKeyEqualTo> expensive_map_type;

                // So again for the sake of discussion let's say MyExpensiveValueType is expensive to construct and the key MyExpensiveKeyType is expensive too.
                // As we saw from prev examples we can compare they key quickly to AZStd::size_t if we know the comparable to key, without making the expensive key
                // class. But this time if they key is not found we you like to construct the expensive value type with an input (non default)... so we have the fallowinf
                // quick insert struct...
                struct QuickInsert
                {
                    AZStd::size_t m_comparebleToKey;
                    int           m_keyInput;
                    int           m_valueInput;
                };
                // Then we need a converter class that will convert form this struct to the map key and value
                struct Converter
                {
                    typedef AZStd::size_t key_type;     // required because we might use the Map::key_type or Comparable to Key type.

                    const key_type& to_key(const QuickInsert& qi) const { return qi.m_comparebleToKey; }
                    expensive_map_type::value_type to_value(const QuickInsert& qi) const
                    {
                        // This is the place where the expensive ctors will be called only of really necessary
                        return AZStd::make_pair(MyExpensiveKeyType(qi.m_keyInput), MyExpensiveValueType(qi.m_valueInput));
                    }
                };
                Converter convQuickInsertToMapType;


                expensive_map_type myMap;

                QuickInsert qi;
                qi.m_comparebleToKey = 10;
                qi.m_keyInput = 100;  ///< Input for ctors or whatever
                qi.m_valueInput = 200; ///< Input for ctors or whatever

                myMap.insert_from(qi, convQuickInsertToMapType, AZStd::hash<AZStd::size_t>() /*hasher for the comparable to key*/, MyExpensiveKeyEqualTo());

                // And that's about it, this way you will do a fast (find_as like) compare without constructing the key, if doesn't exist, the insert
                // structure (QuickInsert) will be converted to the map types (expensive). This way if they key is in the map, it will be lightning fast.

                // Other example of the same thing imagine you have a map unordered_map<string,ExpensiveObject> everytime when you want to insert an object
                // and name is not string but string literal, you will one or more copies of the string object. Just to compute the key. You can compute (hash)
                // the string literal 100% the same as the string. This way you can avoid creating string object. This example is similar to the word count example
                // in the lazy_insert sample.
            }

            // UnorderedExamples-End
#endif // AZ_PLATFORM_WINDOWS
        }
    };

    TEST_F(ContainersExamples, Array)
    {
        Array();
    }

    TEST_F(ContainersExamples, Vector)
    {
        Vector();
    }

    TEST_F(ContainersExamples, List)
    {
        List();
    }

    TEST_F(ContainersExamples, Deque)
    {
        Deque();
    }

    TEST_F(ContainersExamples, Hashed)
    {
        Hashed();
    }
}
