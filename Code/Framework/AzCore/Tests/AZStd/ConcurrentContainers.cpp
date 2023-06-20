/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "UserTypes.h"

#include <AzCore/std/parallel/containers/concurrent_fixed_unordered_set.h>
#include <AzCore/std/parallel/containers/concurrent_fixed_unordered_map.h>
#include <AzCore/std/parallel/containers/concurrent_unordered_map.h>
#include <AzCore/std/parallel/containers/concurrent_unordered_set.h>
#include <AzCore/std/parallel/containers/concurrent_vector.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace UnitTest
{
    using namespace AZStd;
    using namespace UnitTestInternal;

    template<typename Set>
    class ConcurrentUnorderedSetTestBase
        : public LeakDetectionFixture
    {
    public:
        void run()
        {
            Set set;

            //insert
            AZ_TEST_ASSERT(set.empty());
            AZ_TEST_ASSERT(set.size() == 0);
            AZ_TEST_ASSERT(set.insert(10));
            AZ_TEST_ASSERT(!set.empty());
            AZ_TEST_ASSERT(set.size() == 1);
            AZ_TEST_ASSERT(set.insert(20));
            AZ_TEST_ASSERT(set.size() == 2);
            AZ_TEST_ASSERT(set.insert(30));
            AZ_TEST_ASSERT(set.size() == 3);

            AZ_TEST_ASSERT(!set.insert(20)); //not multiset
            AZ_TEST_ASSERT(set.size() == 3);

            //find
            AZ_TEST_ASSERT(set.find(10));
            AZ_TEST_ASSERT(!set.find(40));

            //erase
            AZ_TEST_ASSERT(set.erase(10) == 1);
            AZ_TEST_ASSERT(set.size() == 2);

            AZ_TEST_ASSERT(set.erase(10) == 0);
            AZ_TEST_ASSERT(set.size() == 2);
            AZ_TEST_ASSERT(set.erase(100) == 0);
            AZ_TEST_ASSERT(set.size() == 2);

            //erase_one
            AZ_TEST_ASSERT(set.erase_one(20));
            AZ_TEST_ASSERT(set.size() == 1);
            AZ_TEST_ASSERT(set.erase_one(30));
            AZ_TEST_ASSERT(set.size() == 0);
            AZ_TEST_ASSERT(set.empty());

            //clear
            set.insert(10);
            AZ_TEST_ASSERT(!set.empty());
            set.clear();
            AZ_TEST_ASSERT(set.empty());
            AZ_TEST_ASSERT(set.erase(10) == 0);

            //assignment
            set.insert(10);
            set.insert(20);
            set.insert(30);
            Set set2(set);
            AZ_TEST_ASSERT(set2.size() == 3);
            AZ_TEST_ASSERT(set2.find(20));

            Set set3;
            set3 = set;
            AZ_TEST_ASSERT(set3.size() == 3);
            AZ_TEST_ASSERT(set3.find(20));

            set.erase(10);
            AZ_TEST_ASSERT(set.size() == 2);
            set.swap(set3);
            AZ_TEST_ASSERT(set.size() == 3);
            AZ_TEST_ASSERT(set3.size() == 2);

            {
                m_failures = 0;
                AZStd::thread thread0(AZStd::bind(&ConcurrentUnorderedSetTestBase::InsertErase, this, 0));
                AZStd::thread thread1(AZStd::bind(&ConcurrentUnorderedSetTestBase::InsertErase, this, 1));
                AZStd::thread thread2(AZStd::bind(&ConcurrentUnorderedSetTestBase::InsertErase, this, 2));
                AZStd::thread thread3(AZStd::bind(&ConcurrentUnorderedSetTestBase::InsertErase, this, 3));
                thread0.join();
                thread1.join();
                thread2.join();
                thread3.join();
                AZ_TEST_ASSERT(m_failures == 0);
                AZ_TEST_ASSERT(m_set.empty());
            }

            m_set = Set(); // clear memory
        }

    private:
#ifdef _DEBUG
        static const int NUM_ITERATIONS = 1;
#else
        static const int NUM_ITERATIONS = 200;
#endif
        static const int NUM_VALUES = 500;
        void InsertErase(int id)
        {
            for (int iter = 0; iter < NUM_ITERATIONS; ++iter)
            {
                //insert
                for (int i = 0; i < NUM_VALUES; ++i)
                {
                    if (!m_set.insert(id * NUM_VALUES + i))
                    {
                        ++m_failures;
                    }
                }
                //find
                for (int i = 0; i < NUM_VALUES; ++i)
                {
                    if (!m_set.find(id * NUM_VALUES + i))
                    {
                        ++m_failures;
                    }
                }
                //erase
                for (int i = 0; i < NUM_VALUES; ++i)
                {
                    if (m_set.erase(id * NUM_VALUES + i) != 1)
                    {
                        ++m_failures;
                    }
                }
            }
        }

        Set m_set;
        atomic<int> m_failures;
    };

    typedef ConcurrentUnorderedSetTestBase<concurrent_unordered_set<int> > ConcurrentUnorderedSetTest;
    typedef ConcurrentUnorderedSetTestBase<concurrent_fixed_unordered_set<int, 1543, 2100> > ConcurrentFixedUnorderedSetTest;

    TEST_F(ConcurrentUnorderedSetTest, Test)
    {
        run();
    }

    TEST_F(ConcurrentFixedUnorderedSetTest, Test)
    {
        run();
    }

    template<typename Set>
    class ConcurrentUnorderedMultiSetTestBase
        : public LeakDetectionFixture
    {
    public:
        void run()
        {
            Set set;

            //insert
            AZ_TEST_ASSERT(set.empty());
            AZ_TEST_ASSERT(set.size() == 0);
            AZ_TEST_ASSERT(set.insert(10));
            AZ_TEST_ASSERT(!set.empty());
            AZ_TEST_ASSERT(set.size() == 1);
            AZ_TEST_ASSERT(set.insert(20));
            AZ_TEST_ASSERT(set.size() == 2);

            AZ_TEST_ASSERT(set.insert(20)); //multiset
            AZ_TEST_ASSERT(set.size() == 3);
            AZ_TEST_ASSERT(set.insert(30));
            AZ_TEST_ASSERT(set.insert(30));
            AZ_TEST_ASSERT(set.insert(30));
            AZ_TEST_ASSERT(set.size() == 6);

            //find
            AZ_TEST_ASSERT(set.find(10));
            AZ_TEST_ASSERT(set.find(20));
            AZ_TEST_ASSERT(!set.find(40));

            //erase
            AZ_TEST_ASSERT(set.erase(10) == 1);
            AZ_TEST_ASSERT(set.size() == 5);

            AZ_TEST_ASSERT(set.erase(10) == 0);
            AZ_TEST_ASSERT(set.size() == 5);
            AZ_TEST_ASSERT(set.erase(100) == 0);
            AZ_TEST_ASSERT(set.size() == 5);

            AZ_TEST_ASSERT(set.erase(20) == 2);
            AZ_TEST_ASSERT(set.size() == 3);

            //erase_one
            AZ_TEST_ASSERT(set.erase_one(30));
            AZ_TEST_ASSERT(set.size() == 2);
            AZ_TEST_ASSERT(set.erase(30) == 2);
            AZ_TEST_ASSERT(set.size() == 0);
            AZ_TEST_ASSERT(set.empty());

            //clear
            set.insert(10);
            AZ_TEST_ASSERT(!set.empty());
            set.clear();
            AZ_TEST_ASSERT(set.empty());
            AZ_TEST_ASSERT(set.erase(10) == 0);

            //assignment
            set.insert(10);
            set.insert(20);
            set.insert(30);
            Set set2(set);
            AZ_TEST_ASSERT(set2.size() == 3);
            AZ_TEST_ASSERT(set2.find(20));

            Set set3;
            set3 = set;
            AZ_TEST_ASSERT(set3.size() == 3);
            AZ_TEST_ASSERT(set3.find(20));

            set.erase(10);
            AZ_TEST_ASSERT(set.size() == 2);
            set.swap(set3);
            AZ_TEST_ASSERT(set.size() == 3);
            AZ_TEST_ASSERT(set3.size() == 2);

            {
                m_failures = 0;
                AZStd::thread thread0(AZStd::bind(&ConcurrentUnorderedMultiSetTestBase::InsertErase, this));
                AZStd::thread thread1(AZStd::bind(&ConcurrentUnorderedMultiSetTestBase::InsertErase, this));
                AZStd::thread thread2(AZStd::bind(&ConcurrentUnorderedMultiSetTestBase::InsertErase, this));
                AZStd::thread thread3(AZStd::bind(&ConcurrentUnorderedMultiSetTestBase::InsertErase, this));
                thread0.join();
                thread1.join();
                thread2.join();
                thread3.join();
                AZ_TEST_ASSERT(m_failures == 0);
                AZ_TEST_ASSERT(m_set.empty());
            }

            m_set = Set(); // clear memory
        }

    private:
#ifdef _DEBUG
        static const int NUM_ITERATIONS = 1;
#else
        static const int NUM_ITERATIONS = 200;
#endif
        static const int NUM_VALUES = 500;
        void InsertErase()
        {
            for (int iter = 0; iter < NUM_ITERATIONS; ++iter)
            {
                //insert
                for (int i = 0; i < NUM_VALUES; ++i)
                {
                    if (!m_set.insert(i))
                    {
                        ++m_failures;
                    }
                }
                //find
                for (int i = 0; i < NUM_VALUES; ++i)
                {
                    if (!m_set.find(i))
                    {
                        ++m_failures;
                    }
                }
                //erase
                for (int i = 0; i < NUM_VALUES; ++i)
                {
                    if (!m_set.erase_one(i))
                    {
                        ++m_failures;
                    }
                }
            }
        }

        Set m_set;
        atomic<int> m_failures;
    };

    typedef ConcurrentUnorderedMultiSetTestBase<concurrent_unordered_multiset<int> > ConcurrentUnorderedMultiSetTest;
    typedef ConcurrentUnorderedMultiSetTestBase<concurrent_fixed_unordered_multiset<int, 1543, 2100> > ConcurrentFixedUnorderedMultiSetTest;

    TEST_F(ConcurrentUnorderedMultiSetTest, Test)
    {
        run();
    }

    TEST_F(ConcurrentFixedUnorderedMultiSetTest, Test)
    {
        run();
    }

    template<typename Map>
    class ConcurrentUnorderedMapTestBase
        : public LeakDetectionFixture
    {
    public:
        void run()
        {
            Map map;

            //insert
            AZ_TEST_ASSERT(map.empty());
            AZ_TEST_ASSERT(map.size() == 0);
            AZ_TEST_ASSERT(map.insert(AZStd::make_pair(10, 11)));
            AZ_TEST_ASSERT(!map.empty());
            AZ_TEST_ASSERT(map.size() == 1);
            AZ_TEST_ASSERT(map.insert(AZStd::make_pair(20, 21)));
            AZ_TEST_ASSERT(map.size() == 2);
            AZ_TEST_ASSERT(map.insert(AZStd::make_pair(30, 31)));
            AZ_TEST_ASSERT(map.size() == 3);

            AZ_TEST_ASSERT(!map.insert(AZStd::make_pair(20, 22))); //not multimap
            AZ_TEST_ASSERT(map.size() == 3);

            //find
            AZ_TEST_ASSERT(map.find(10));
            AZ_TEST_ASSERT(!map.find(40));
            int result = 0;
            AZ_TEST_ASSERT(map.find(10, &result));
            AZ_TEST_ASSERT(result == 11);

            //erase
            AZ_TEST_ASSERT(map.erase(10) == 1);
            AZ_TEST_ASSERT(map.size() == 2);

            AZ_TEST_ASSERT(map.erase(10) == 0);
            AZ_TEST_ASSERT(map.size() == 2);
            AZ_TEST_ASSERT(map.erase(100) == 0);
            AZ_TEST_ASSERT(map.size() == 2);

            //erase_one
            AZ_TEST_ASSERT(map.erase_one(20));
            AZ_TEST_ASSERT(map.size() == 1);
            AZ_TEST_ASSERT(map.erase_one(30));
            AZ_TEST_ASSERT(map.size() == 0);
            AZ_TEST_ASSERT(map.empty());

            //clear
            map.insert(10);
            AZ_TEST_ASSERT(!map.empty());
            map.clear();
            AZ_TEST_ASSERT(map.empty());
            AZ_TEST_ASSERT(map.erase(10) == 0);

            //assignment
            map.insert(AZStd::make_pair(10, 11));
            map.insert(AZStd::make_pair(20, 21));
            map.insert(AZStd::make_pair(30, 31));
            Map map2(map);
            AZ_TEST_ASSERT(map2.size() == 3);
            AZ_TEST_ASSERT(map2.find(20));

            Map map3;
            map3 = map;
            AZ_TEST_ASSERT(map3.size() == 3);
            AZ_TEST_ASSERT(map3.find(20));

            map.erase(10);
            AZ_TEST_ASSERT(map.size() == 2);
            map.swap(map3);
            AZ_TEST_ASSERT(map.size() == 3);
            AZ_TEST_ASSERT(map3.size() == 2);

            {
                m_failures = 0;
                AZStd::thread thread0(AZStd::bind(&ConcurrentUnorderedMapTestBase::InsertErase, this, 0));
                AZStd::thread thread1(AZStd::bind(&ConcurrentUnorderedMapTestBase::InsertErase, this, 1));
                AZStd::thread thread2(AZStd::bind(&ConcurrentUnorderedMapTestBase::InsertErase, this, 2));
                AZStd::thread thread3(AZStd::bind(&ConcurrentUnorderedMapTestBase::InsertErase, this, 3));
                thread0.join();
                thread1.join();
                thread2.join();
                thread3.join();
                AZ_TEST_ASSERT(m_failures == 0);
                AZ_TEST_ASSERT(m_map.empty());
            }

            m_map = Map(); // clear memory
        }

    private:
#ifdef _DEBUG
        static const int NUM_ITERATIONS = 1;
#else
        static const int NUM_ITERATIONS = 200;
#endif
        static const int NUM_VALUES = 500;
        void InsertErase(int id)
        {
            for (int iter = 0; iter < NUM_ITERATIONS; ++iter)
            {
                //insert
                for (int i = 0; i < NUM_VALUES; ++i)
                {
                    if (!m_map.insert(AZStd::make_pair(id * NUM_VALUES + i, id * NUM_VALUES + i + 1)))
                    {
                        ++m_failures;
                    }
                }
                //find
                for (int i = 0; i < NUM_VALUES; ++i)
                {
                    int result = 0;
                    if (!m_map.find(id * NUM_VALUES + i, &result))
                    {
                        ++m_failures;
                    }
                    if (result != id * NUM_VALUES + i + 1)
                    {
                        ++m_failures;
                    }
                }
                //erase
                for (int i = 0; i < NUM_VALUES; ++i)
                {
                    if (m_map.erase(id * NUM_VALUES + i) != 1)
                    {
                        ++m_failures;
                    }
                }
            }
        }

        Map m_map;
        atomic<int> m_failures;
    };

    typedef ConcurrentUnorderedMapTestBase<concurrent_unordered_map<int, int> > ConcurrentUnorderedMapTest;
    typedef ConcurrentUnorderedMapTestBase<concurrent_fixed_unordered_map<int, int, 1543, 2100> > ConcurrentFixedUnorderedMapTest;

    TEST_F(ConcurrentUnorderedMapTest, Test)
    {
        run();
    }

    TEST_F(ConcurrentFixedUnorderedMapTest, Test)
    {
        run();
    }

    template<typename Map>
    class ConcurrentUnorderedMultiMapTestBase
        : public LeakDetectionFixture
    {
    public:
        void run()
        {
            Map map;

            //insert
            AZ_TEST_ASSERT(map.empty());
            AZ_TEST_ASSERT(map.size() == 0);
            AZ_TEST_ASSERT(map.insert(AZStd::make_pair(10, 11)));
            AZ_TEST_ASSERT(!map.empty());
            AZ_TEST_ASSERT(map.size() == 1);
            AZ_TEST_ASSERT(map.insert(AZStd::make_pair(20, 21)));
            AZ_TEST_ASSERT(map.size() == 2);

            AZ_TEST_ASSERT(map.insert(AZStd::make_pair(20, 22))); //multimap
            AZ_TEST_ASSERT(map.size() == 3);
            AZ_TEST_ASSERT(map.insert(AZStd::make_pair(30, 31)));
            AZ_TEST_ASSERT(map.insert(AZStd::make_pair(30, 32)));
            AZ_TEST_ASSERT(map.insert(AZStd::make_pair(30, 33)));
            AZ_TEST_ASSERT(map.size() == 6);

            //find
            AZ_TEST_ASSERT(map.find(10));
            AZ_TEST_ASSERT(map.find(20));
            AZ_TEST_ASSERT(!map.find(40));
            int result = 0;
            AZ_TEST_ASSERT(map.find(10, &result));
            AZ_TEST_ASSERT(result == 11);

            //erase
            AZ_TEST_ASSERT(map.erase(10) == 1);
            AZ_TEST_ASSERT(map.size() == 5);

            AZ_TEST_ASSERT(map.erase(10) == 0);
            AZ_TEST_ASSERT(map.size() == 5);
            AZ_TEST_ASSERT(map.erase(100) == 0);
            AZ_TEST_ASSERT(map.size() == 5);

            AZ_TEST_ASSERT(map.erase(20) == 2);
            AZ_TEST_ASSERT(map.size() == 3);

            //erase_one
            AZ_TEST_ASSERT(map.erase_one(30));
            AZ_TEST_ASSERT(map.size() == 2);
            AZ_TEST_ASSERT(map.erase(30) == 2);
            AZ_TEST_ASSERT(map.size() == 0);
            AZ_TEST_ASSERT(map.empty());

            //clear
            map.insert(10);
            AZ_TEST_ASSERT(!map.empty());
            map.clear();
            AZ_TEST_ASSERT(map.empty());
            AZ_TEST_ASSERT(map.erase(10) == 0);

            //assignment
            map.insert(10);
            map.insert(20);
            map.insert(30);
            Map map2(map);
            AZ_TEST_ASSERT(map2.size() == 3);
            AZ_TEST_ASSERT(map2.find(20));

            Map map3;
            map3 = map;
            AZ_TEST_ASSERT(map3.size() == 3);
            AZ_TEST_ASSERT(map3.find(20));

            map.erase(10);
            AZ_TEST_ASSERT(map.size() == 2);
            map.swap(map3);
            AZ_TEST_ASSERT(map.size() == 3);
            AZ_TEST_ASSERT(map3.size() == 2);

            {
                m_failures = 0;
                AZStd::thread thread0(AZStd::bind(&ConcurrentUnorderedMultiMapTestBase::InsertErase, this));
                AZStd::thread thread1(AZStd::bind(&ConcurrentUnorderedMultiMapTestBase::InsertErase, this));
                AZStd::thread thread2(AZStd::bind(&ConcurrentUnorderedMultiMapTestBase::InsertErase, this));
                AZStd::thread thread3(AZStd::bind(&ConcurrentUnorderedMultiMapTestBase::InsertErase, this));
                thread0.join();
                thread1.join();
                thread2.join();
                thread3.join();
                AZ_TEST_ASSERT(m_failures == 0);
                AZ_TEST_ASSERT(m_map.empty());
            }

            Map dummy;
            m_map.swap(dummy); // clear memory
        }

    private:
#ifdef _DEBUG
        static const int NUM_ITERATIONS = 1;
#else
        static const int NUM_ITERATIONS = 200;
#endif
        static const int NUM_VALUES = 500;
        void InsertErase()
        {
            for (int iter = 0; iter < NUM_ITERATIONS; ++iter)
            {
                //insert
                for (int i = 0; i < NUM_VALUES; ++i)
                {
                    if (!m_map.insert(AZStd::make_pair(i, i + 1)))
                    {
                        ++m_failures;
                    }
                }
                //find
                for (int i = 0; i < NUM_VALUES; ++i)
                {
                    int result = 0;
                    if (!m_map.find(i, &result))
                    {
                        ++m_failures;
                    }
                    if (result != i + 1)
                    {
                        ++m_failures;
                    }
                }
                //erase
                for (int i = 0; i < NUM_VALUES; ++i)
                {
                    if (!m_map.erase_one(i))
                    {
                        ++m_failures;
                    }
                }
            }
        }

        Map m_map;
        atomic<int> m_failures;
    };

    typedef ConcurrentUnorderedMultiMapTestBase<concurrent_unordered_multimap<int, int> > ConcurrentUnorderedMultiMapTest;
    typedef ConcurrentUnorderedMultiMapTestBase<concurrent_fixed_unordered_multimap<int, int, 1543, 2100> > ConcurrentFixedUnorderedMultiMapTest;

    TEST_F(ConcurrentUnorderedMultiMapTest, Test)
    {
        run();
    }
    
    TEST_F(ConcurrentFixedUnorderedMultiMapTest, Test)
    {
        run();
    }

    class ConcurrentVectorTest
        : public LeakDetectionFixture
    {
    public:
        void run()
        {
            //
            //single threaded functionality tests
            //
            concurrent_vector<int> testVector;
            AZ_TEST_ASSERT(testVector.empty());
            AZ_TEST_ASSERT(testVector.size() == 0);
            testVector.push_back(10);
            AZ_TEST_ASSERT(!testVector.empty());
            AZ_TEST_ASSERT(testVector.size() == 1);
            AZ_TEST_ASSERT(testVector[0] == 10);
            testVector[0] = 20;
            AZ_TEST_ASSERT(testVector[0] == 20);
            testVector.clear();
            AZ_TEST_ASSERT(testVector.empty());
            AZ_TEST_ASSERT(testVector.size() == 0);
            for (int i = 0; i < 100; ++i)
            {
                testVector.push_back(i + 1000);
            }
            AZ_TEST_ASSERT(testVector.size() == 100);
            for (int i = 0; i < 100; ++i)
            {
                AZ_TEST_ASSERT(testVector[i] == i + 1000);
            }

            //
            //multithread tests
            //
            {
                AZStd::thread thread0(AZStd::bind(&ConcurrentVectorTest::Push, this, 0));
                AZStd::thread thread1(AZStd::bind(&ConcurrentVectorTest::Push, this, 1));
                AZStd::thread thread2(AZStd::bind(&ConcurrentVectorTest::Push, this, 2));
                AZStd::thread thread3(AZStd::bind(&ConcurrentVectorTest::Push, this, 3));
                thread0.join();
                thread1.join();
                thread2.join();
                thread3.join();

                //verify vector contains the right values in the expected order
                AZ_TEST_ASSERT(m_vector.size() == 4 * NUM_ITERATIONS);
                int nextValue[4];
                for (int i = 0; i < 4; ++i)
                {
                    nextValue[i] = i * NUM_ITERATIONS;
                }
                for (unsigned int vecIndex = 0; vecIndex < m_vector.size(); ++vecIndex)
                {
                    int value = m_vector[vecIndex];

                    bool isFound = false;
                    for (int i = 0; i < 4; ++i)
                    {
                        if (nextValue[i] == value)
                        {
                            isFound = true;
                            ++nextValue[i];
                            break;
                        }
                    }
                    AZ_TEST_ASSERT(isFound);
                }
            }
        }

    private:
#if defined(_DEBUG)
        static const int NUM_ITERATIONS = 10000;
#else
        static const int NUM_ITERATIONS = 500000;
#endif

        void Push(int threadIndex)
        {
            for (int i = 0; i < NUM_ITERATIONS; ++i)
            {
                m_vector.push_back(threadIndex * NUM_ITERATIONS + i);
            }
        }

        concurrent_vector<int> m_vector;
    };

    TEST_F(ConcurrentVectorTest, Test)
    {
        run();
    }
}
