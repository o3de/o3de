/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/any.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/tuple.h>
#include "UserTypes.h"

namespace UnitTest
{
    template<class T1, class T2>
    std::pair<T1, T2> to_std_pair(const AZStd::pair<T1, T2>& azPair)
    {
        return std::make_pair(azPair.first, azPair.second);
    }

    template<class T1, class T2>
    std::pair<T1, T2> to_std_pair(AZStd::pair<T1, T2>&& azPair)
    {
        return std::make_pair(AZStd::move(azPair.first), AZStd::move(azPair.second));
    }

    //////////////////////////////////////////////////////////////////////////
    // Fixtures

    // Fixture for non-typed tests
    class TupleTest
        : public LeakDetectionFixture
    {
    protected:
        struct MoveOnlyType
        {
            AZ_CLASS_ALLOCATOR(MoveOnlyType, AZ::SystemAllocator);
            MoveOnlyType() = delete;
            MoveOnlyType(AZStd::tuple<AZ::s32&&, AZStd::string_view&&> tuplePair)
                : m_valueInt(AZStd::move(AZStd::get<0>(tuplePair)))
                , m_constString(AZStd::move(AZStd::get<1>(tuplePair)))
            {
            }

            AZ::s32 m_valueInt;
            AZStd::string_view m_constString;
        };
    };

    // Fixture for typed tests
    template<typename Tuple>
    class TupleTypedTest
        : public LeakDetectionFixture
    {
    };
    using TupleTestTypes = ::testing::Types<
        AZStd::tuple<>,
        AZStd::tuple<AZStd::unordered_map<AZStd::string, AZStd::string_view>>,
        AZStd::tuple<int, float, AZStd::string>,
        AZStd::tuple<int, float, int>,
        AZStd::tuple<bool, bool, bool, bool>
    >;
    TYPED_TEST_CASE(TupleTypedTest, TupleTestTypes);


    //////////////////////////////////////////////////////////////////////////
    // Tests for constructors
    namespace Constructor
    {
        // Construct empty
        TEST_F(TupleTest, DefaultConstruct)
        {
            AZStd::tuple<AZStd::string, float, AZStd::vector<int>> testTuple;
            EXPECT_TRUE(AZStd::get<0>(testTuple).empty());
            EXPECT_FLOAT_EQ(0.0f, AZStd::get<1>(testTuple));
            EXPECT_TRUE(AZStd::get<2>(testTuple).empty());
        }

        TEST_F(TupleTest, InitializerConstruct)
        {
            AZStd::tuple<AZStd::string, float> testTuple{ AZStd::string("Test"), 4.0f };
            EXPECT_EQ("Test", AZStd::get<0>(testTuple));
            EXPECT_FLOAT_EQ(4.0f, AZStd::get<1>(testTuple));
        }

        TEST_F(TupleTest, ConvertingConstruct)
        {
            AZStd::tuple<AZStd::string, AZStd::array<AZStd::string_view, 2U>> testTuple("Test", { { "Arg1", "Arg2" } });
            EXPECT_EQ("Test", AZStd::get<0>(testTuple));
            EXPECT_EQ(2, AZStd::get<1>(testTuple).size());
            EXPECT_EQ("Arg1", AZStd::get<1>(testTuple)[0]);
            EXPECT_EQ("Arg2", AZStd::get<1>(testTuple)[1]);
        }

        TEST_F(TupleTest, PairCopyConstruct)
        {
            AZStd::vector<AZStd::string> testStringVector{ "Test1", "Test2" };
            auto testPair = std::make_pair(AZStd::string("Test"), AZStd::vector<AZStd::string>{ "Test1", "Test2" });
            AZStd::tuple<AZStd::string, AZStd::vector<AZStd::string>> testTuple(testPair);
            EXPECT_EQ(testPair.first, AZStd::get<0>(testTuple));
            EXPECT_EQ(testPair.second, AZStd::get<1>(testTuple));
        }

        TEST_F(TupleTest, PairMoveConstruct)
        {
            AZStd::vector<AZStd::string> testStringVector{ "Test1", "Test2" };
            auto testPair = std::make_pair(AZStd::string("Test"), testStringVector);
            AZStd::tuple<AZStd::string, AZStd::vector<AZStd::string>> testTuple(AZStd::move(testPair));
            EXPECT_EQ("Test", AZStd::get<0>(testTuple));
            EXPECT_EQ(testStringVector, AZStd::get<1>(testTuple));
        }

        TEST_F(TupleTest, TupleCopyConstruct)
        {
            AZStd::tuple<AZStd::string, double, AZStd::vector<int>> testTuple("Test", 16.0, AZStd::vector<int>{5});
            AZStd::tuple<AZStd::string, double, AZStd::vector<int>> copyTuple(testTuple);
            EXPECT_EQ(testTuple, copyTuple);
        }

        TEST_F(TupleTest, TupleMoveConstruct)
        {
            AZStd::tuple<AZStd::string, double, AZStd::vector<int>> testTuple("Test", 16.0, AZStd::vector<int>{5});
            AZStd::tuple<AZStd::string, double, AZStd::vector<int>> moveTuple(testTuple);
            EXPECT_EQ("Test", AZStd::get<0>(moveTuple));
            EXPECT_EQ(16.0, AZStd::get<1>(moveTuple));
            EXPECT_EQ(AZStd::vector<int>{5}, AZStd::get<2>(moveTuple));
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Tests for assignment operator
    namespace Assignment
    {
        // Test copy assign
        TEST_F(TupleTest, CopyAssignTest)
        {
            AZStd::tuple<AZStd::any> source(AZStd::make_any<AZStd::string>("Test"));
            AZStd::tuple<AZStd::any> dest;
            EXPECT_TRUE(AZStd::get<0>(source).is<AZStd::string>());
            EXPECT_EQ("Test", AZStd::any_cast<AZStd::string>(AZStd::get<0>(source)));
            EXPECT_TRUE(AZStd::get<0>(dest).empty());
            dest = source;

            EXPECT_TRUE(AZStd::get<0>(dest).is<AZStd::string>());
            EXPECT_EQ(AZStd::any_cast<AZStd::string>(AZStd::get<0>(source)), AZStd::any_cast<AZStd::string>(AZStd::get<0>(dest)));
        }
        // Test move assign
        TEST_F(TupleTest, MoveAssignTest)
        {
            AZStd::tuple<AZStd::any> source(AZStd::make_any<AZStd::string>("Test"));
            AZStd::tuple<AZStd::any> dest;
            EXPECT_TRUE(AZStd::get<0>(source).is<AZStd::string>());
            EXPECT_EQ("Test", AZStd::any_cast<AZStd::string>(AZStd::get<0>(source)));
            EXPECT_TRUE(AZStd::get<0>(dest).empty());
            dest = AZStd::move(source);

            EXPECT_TRUE(AZStd::get<0>(source).empty());
            EXPECT_FALSE(AZStd::get<0>(dest).empty());
        }
    }

    namespace FreeFunctions
    {
        TEST_F(TupleTest, MakeTupleTest)
        {
            auto testTuple = AZStd::make_tuple(AZStd::string("Test"), AZStd::make_unique<AZStd::string>("UniqueTest"));
            EXPECT_EQ("Test", AZStd::get<0>(testTuple));
            ASSERT_NE(nullptr, AZStd::get<1>(testTuple));
            EXPECT_EQ("UniqueTest", *AZStd::get<1>(testTuple));
        }

        TEST_F(TupleTest, TieTest)
        {
            int valueInt = 0;
            double valueDouble = 0.0;
            auto testTuple = AZStd::make_tuple(17, 42.52);
            AZStd::tie(valueInt, valueDouble) = testTuple;
            EXPECT_EQ(17, valueInt);
            EXPECT_DOUBLE_EQ(valueDouble, 42.52);

            auto ignoreTuple = AZStd::make_tuple(true, 1, 15.0f);
            bool valueBool = false;
            AZStd::tie(valueBool, AZStd::ignore, AZStd::ignore) = ignoreTuple;
            EXPECT_TRUE(valueBool);
        }

        TEST_F(TupleTest, ForwardAsTupleTest)
        {
            MoveOnlyType testResult(AZStd::forward_as_tuple(15, AZStd::string_view("LinkerLiteral")));
            EXPECT_EQ(15, testResult.m_valueInt);
            EXPECT_EQ("LinkerLiteral", testResult.m_constString);
        }

        TEST_F(TupleTest, TupleCatTest)
        {
            AZStd::tuple<int, float, double> intFloatDoubleTuple{ 1, 2.0f, 4.0 };
            AZStd::tuple<> emptyTuple;
            AZStd::tuple<AZStd::string, AZStd::string> stringStringTuple("First", "Second");
            auto resultTuple = AZStd::tuple_cat(intFloatDoubleTuple, emptyTuple, stringStringTuple);
            EXPECT_EQ(5, AZStd::tuple_size<decltype(resultTuple)>::value);
        }

        TEST_F(TupleTest, TupleGetTest)
        {
            AZStd::tuple<int, float, double> intFloatDoubleTuple{ 1, 2.0f, 4.0 };
            EXPECT_EQ(AZStd::get<0>(intFloatDoubleTuple), AZStd::get<int>(intFloatDoubleTuple));
            EXPECT_EQ(AZStd::get<1>(intFloatDoubleTuple), AZStd::get<float>(intFloatDoubleTuple));
            EXPECT_EQ(AZStd::get<2>(intFloatDoubleTuple), AZStd::get<double>(intFloatDoubleTuple));

            const AZStd::tuple<AZStd::string, float, int> stringFloatIntTuple("Test", 2.0f, 8);
            EXPECT_EQ(AZStd::get<0>(stringFloatIntTuple), AZStd::get<AZStd::string>(stringFloatIntTuple));
            EXPECT_EQ(AZStd::get<1>(stringFloatIntTuple), AZStd::get<float>(stringFloatIntTuple));
            EXPECT_EQ(AZStd::get<2>(stringFloatIntTuple), AZStd::get<int>(stringFloatIntTuple));

            AZStd::tuple<AZStd::string, bool, AZStd::string> rValueTuple("FirstTest", false, "SecondTest");
            EXPECT_EQ(AZStd::get<1>(AZStd::move(rValueTuple)), AZStd::get<bool>(AZStd::move(rValueTuple)));
            // Type template version of get is only supported for types that are in the template exactly once
            // It is a static assert to attempt to access an element using AZStd::get<AZStd::string> as there are two of those types in the tuple
            EXPECT_NE(AZStd::get<0>(AZStd::move(AZStd::move(rValueTuple))), AZStd::get<2>(AZStd::move(rValueTuple)));
        }

        TEST_F(TupleTest, TupleHashTest)
        {
            AZStd::string testString("Test1");
            AZStd::string superDuperString("SuperDuperString");
            AZStd::unordered_map<AZStd::tuple<AZStd::string, int, bool>, AZStd::string> tupleToBoolMap;
            tupleToBoolMap.emplace(AZStd::make_tuple(testString, 234, true), "MappedValue");
            tupleToBoolMap.insert({ AZStd::make_tuple(superDuperString, -8345, false), "SecondHallPass" });
            EXPECT_EQ(2, tupleToBoolMap.size());
            auto foundIt = tupleToBoolMap.find(AZStd::make_tuple(testString, 234, true));
            ASSERT_NE(tupleToBoolMap.end(), foundIt);
            EXPECT_EQ("MappedValue", foundIt->second);

            foundIt = tupleToBoolMap.find(AZStd::make_tuple(superDuperString, -8345, false));
            ASSERT_NE(tupleToBoolMap.end(), foundIt);
            EXPECT_EQ("SecondHallPass", foundIt->second);

            // Not found case
            foundIt = tupleToBoolMap.find(AZStd::make_tuple(superDuperString, -8345, true));
            EXPECT_EQ(tupleToBoolMap.end(), foundIt);
            foundIt = tupleToBoolMap.find(AZStd::make_tuple(superDuperString, -8344, false));
            EXPECT_EQ(tupleToBoolMap.end(), foundIt);
            foundIt = tupleToBoolMap.find(AZStd::make_tuple("SuperDuperString1", -8345, false));
            EXPECT_EQ(tupleToBoolMap.end(), foundIt);
        }
    }

    namespace AssignTest
    {
        struct BaseIntWrapper
        {
            explicit BaseIntWrapper(AZ::s32 num = 0) : m_num(num) {}
            virtual ~BaseIntWrapper() = default;
            AZ::s32 m_num;
        };

        struct DerivedIntWrapper
            : BaseIntWrapper
        {
            explicit DerivedIntWrapper(AZ::s32 num = 0) : BaseIntWrapper(num) {}
        };

        struct IntAssignWrapper
        {
            IntAssignWrapper() = default;
            IntAssignWrapper& operator=(int)
            {
                return *this;
            }
        };

        TEST_F(TupleTest, CopyPair_AssignTest)
        {
            AZStd::pair<AZ::s64, char> pair0(2U, 'a');
            AZStd::tuple<AZ::s64, AZ::s16> tuple0;
            tuple0 = to_std_pair(pair0);
            EXPECT_EQ(2U, AZStd::get<0>(tuple0));
            EXPECT_EQ(AZ::s16('a'), AZStd::get<1>(tuple0));
        }

        TEST_F(TupleTest, MovePair_AssignTest)
        {
            AZStd::pair<AZ::s64, AZStd::unique_ptr<DerivedIntWrapper>> pair0(2U, AZStd::make_unique<DerivedIntWrapper>(3));
            AZStd::tuple<AZ::s64, AZStd::unique_ptr<BaseIntWrapper>> tuple0;
            tuple0 = to_std_pair(AZStd::move(pair0));
            EXPECT_EQ(2U, AZStd::get<0>(tuple0));
            EXPECT_EQ(3, AZStd::get<1>(tuple0)->m_num);
        }

        TEST_F(TupleTest, ConversionCopy_AssignTest)
        {
            {
                AZStd::tuple<AZ::s32> tuple0(2U);
                AZStd::tuple<AZ::s64> tuple1;
                tuple1 = tuple0;
                EXPECT_EQ(2U, AZStd::get<0>(tuple1));
            }
            {
                AZStd::tuple<AZ::s32, char> tuple0(2U, 'a');
                AZStd::tuple<AZ::s64, AZ::s32> tuple1;
                tuple1 = tuple0;
                EXPECT_EQ(2U, AZStd::get<0>(tuple1));
                EXPECT_EQ(AZ::s32('a'), AZStd::get<1>(tuple1));
            }
            {
                AZStd::tuple<AZ::s32, char, DerivedIntWrapper> tuple0(2U, 'a', DerivedIntWrapper(3));
                AZStd::tuple<AZ::s64, AZ::s32, BaseIntWrapper> tuple1;
                tuple1 = tuple0;
                EXPECT_EQ(2U, AZStd::get<0>(tuple1));
                EXPECT_EQ(AZ::s32('a'), AZStd::get<1>(tuple1));
                EXPECT_EQ(3, AZStd::get<2>(tuple1).m_num);
            }
            {
                DerivedIntWrapper derived0(3);
                DerivedIntWrapper derived1(2);
                AZStd::tuple<AZ::s32, char, DerivedIntWrapper&> tuple0(2U, 'a', derived1);
                AZStd::tuple<AZ::s64, AZ::s32, BaseIntWrapper&> tuple1(1, 'b', derived0);
                tuple1 = tuple0;
                EXPECT_EQ(2U, AZStd::get<0>(tuple1));
                EXPECT_EQ(AZ::s32('a'), AZStd::get<1>(tuple1));
                EXPECT_EQ(2, AZStd::get<2>(tuple1).m_num);
            }
            {
                AZ::s32 x = 42;
                AZ::s32 y = 43;
                AZStd::tuple<AZ::s32&&> tuple0(AZStd::move(x));
                AZStd::tuple<AZ::s32&> tuple1(y);
                tuple0 = tuple1;
                EXPECT_EQ(43, AZStd::get<0>(tuple0));
                EXPECT_EQ(&x, &AZStd::get<0>(tuple0));
            }
        }

        TEST_F(TupleTest, ConversionMove_AssignTest)
        {
            {
                AZStd::tuple<AZ::s32> tuple0(2U);
                AZStd::tuple<AZ::s64> tuple1;
                tuple1 = AZStd::move(tuple0);
                EXPECT_EQ(2U, AZStd::get<0>(tuple1));
            }
            {
                AZStd::tuple<AZ::s32, char> tuple0(2U, 'a');
                AZStd::tuple<AZ::s64, AZ::s32> tuple1;
                tuple1 = AZStd::move(tuple0);
                EXPECT_EQ(2U, AZStd::get<0>(tuple1));
                EXPECT_EQ(AZ::s32('a'), AZStd::get<1>(tuple1));
            }
            {
                AZStd::tuple<AZ::s32, char, DerivedIntWrapper> tuple0(2U, 'a', DerivedIntWrapper(3));
                AZStd::tuple<AZ::s64, AZ::s32, BaseIntWrapper> tuple1;
                tuple1 = AZStd::move(tuple0);
                EXPECT_EQ(2U, AZStd::get<0>(tuple1));
                EXPECT_EQ(AZ::s32('a'), AZStd::get<1>(tuple1));
                EXPECT_EQ(3, AZStd::get<2>(tuple1).m_num);
            }
            {
                DerivedIntWrapper derived0(3);
                DerivedIntWrapper derived1(2);
                AZStd::tuple<AZ::s32, char, DerivedIntWrapper&> tuple0(2U, 'a', derived1);
                AZStd::tuple<AZ::s64, AZ::s32, BaseIntWrapper&> tuple1(1, 'b', derived0);
                tuple1 = AZStd::move(tuple0);
                EXPECT_EQ(2U, AZStd::get<0>(tuple1));
                EXPECT_EQ(AZ::s32('a'), AZStd::get<1>(tuple1));
                EXPECT_EQ(2, AZStd::get<2>(tuple1).m_num);
            }
            {
                AZStd::tuple<AZ::s32, char, AZStd::unique_ptr<DerivedIntWrapper>> tuple0(2U, 'a', AZStd::make_unique<DerivedIntWrapper>(3));
                AZStd::tuple<AZ::s64, AZ::s32, AZStd::unique_ptr<BaseIntWrapper>> tuple1;
                tuple1 = AZStd::move(tuple0);
                EXPECT_EQ(2U, AZStd::get<0>(tuple1));
                EXPECT_EQ(AZ::s32('a'), AZStd::get<1>(tuple1));
                EXPECT_EQ(3, AZStd::get<2>(tuple1)->m_num);
            }
            {
                AZ::s32 x = 42;
                AZ::s32 y = 43;
                AZStd::tuple<AZ::s32&&, IntAssignWrapper> tuple0(AZStd::move(x), IntAssignWrapper{});
                AZStd::tuple<AZ::s32&&, AZ::s32> tuple1(AZStd::move(y), 44);
                tuple0 = AZStd::move(tuple1);
                EXPECT_EQ(43, AZStd::get<0>(tuple0));
                EXPECT_EQ(&x, &AZStd::get<0>(tuple0));
            }
        }

        struct NonAssignable
        {
        private:
            NonAssignable& operator=(const NonAssignable&) = delete;
            NonAssignable& operator=(NonAssignable&&) = delete;
        };
        struct CopyAssignable
        {
            CopyAssignable& operator=(const CopyAssignable&) = default;
            CopyAssignable& operator=(CopyAssignable&&) = delete;
        };
        AZ_TEST_STATIC_ASSERT((AZStd::is_copy_assignable<CopyAssignable>::value));
        struct MoveAssignable
        {
            MoveAssignable& operator=(MoveAssignable&&) { return *this; }
        private:
            MoveAssignable& operator=(const MoveAssignable&) = delete;
        };


        TEST_F(TupleTest, Copy_AssignTest)
        {
            {
                AZStd::tuple<AZ::s32> t0(2);
                AZStd::tuple<AZ::s32> t;
                t = t0;
                EXPECT_EQ(2, AZStd::get<0>(t));
            }
            {
                AZStd::tuple<AZ::s32, char> t0(2, 'a');
                AZStd::tuple<AZ::s32, char> t;
                t = t0;
                EXPECT_EQ(2, AZStd::get<0>(t));
                EXPECT_EQ('a', AZStd::get<1>(t));
            }
            {
                using T = AZStd::tuple<AZ::s32, char, AZStd::string>;
                const T t0(2, 'a', "some text");
                T t;
                t = t0;
                EXPECT_EQ(2, AZStd::get<0>(t));
                EXPECT_EQ('a', AZStd::get<1>(t));
                EXPECT_EQ("some text", AZStd::get<2>(t));
            }
            {
                // test reference assignment.
                using T = AZStd::tuple<AZ::s32&, AZ::s32&&>;
                AZ::s32 x = 42;
                AZ::s32 y = 100;
                AZ::s32 x2 = -1;
                AZ::s32 y2 = 500;
                T t(x, AZStd::move(y));
                T t2(x2, AZStd::move(y2));
                t = t2;
                EXPECT_EQ(x2, AZStd::get<0>(t));
                EXPECT_EQ(&x, &AZStd::get<0>(t));
                EXPECT_EQ(y2, AZStd::get<1>(t));
                EXPECT_EQ(&y, &AZStd::get<1>(t));
            }
        }

        struct CountAssign
        {
            CountAssign() = default;
            CountAssign& operator=(const CountAssign &)
            {
                ++copied;
                return *this;
            }
            CountAssign& operator=(CountAssign&&)
            {
                ++moved;
                return *this;
            }
            static void reset()
            {
                copied = moved = 0;
            }
            static AZ::s32 copied;
            static AZ::s32 moved;
        };
        AZ::s32 CountAssign::copied = 0;
        AZ::s32 CountAssign::moved = 0;

        struct MoveOnly
        {
            MoveOnly() = default;
            MoveOnly(const MoveOnly&) = delete;
            MoveOnly(MoveOnly&& other) : m_num(AZStd::move(other.m_num)) {}
            MoveOnly& operator=(MoveOnly&& other)
            {
                m_num = other.m_num;
                return *this;
            }
            MoveOnly(AZ::s32 num) : m_num(num) {}
            friend bool operator==(AZ::s32 lhs, const MoveOnly& rhs) { return lhs == rhs.m_num; }
            friend bool operator==(const MoveOnly& lhs, AZ::s32 rhs) { return lhs.m_num == rhs; }
            AZ::s32 m_num;
        };


        TEST_F(TupleTest, Move_AssignTest)
        {
            {
                using T = AZStd::tuple<>;
                T t0;
                T t;
                t = AZStd::move(t0);
            }
            {
                using T = AZStd::tuple<MoveOnly>;
                T t0(MoveOnly(0));
                T t;
                t = AZStd::move(t0);
                EXPECT_EQ(0, AZStd::get<0>(t));
            }
            {
                using T = AZStd::tuple<MoveOnly, MoveOnly>;
                T t0(MoveOnly(0), MoveOnly(1));
                T t;
                t = AZStd::move(t0);
                EXPECT_EQ(0, AZStd::get<0>(t));
                EXPECT_EQ(1, AZStd::get<1>(t));
            }
            {
                using T = AZStd::tuple<MoveOnly, MoveOnly, MoveOnly>;
                T t0(MoveOnly(0), MoveOnly(1), MoveOnly(2));
                T t;
                t = AZStd::move(t0);
                EXPECT_EQ(0, AZStd::get<0>(t));
                EXPECT_EQ(1, AZStd::get<1>(t));
                EXPECT_EQ(2, AZStd::get<2>(t));
            }
            {
                // test reference assignment.
                using T = AZStd::tuple<AZ::s32&, AZ::s32&&>;
                AZ::s32 x = 42;
                AZ::s32 y = 100;
                AZ::s32 x2 = -1;
                AZ::s32 y2 = 500;
                T t(x, AZStd::move(y));
                T t2(x2, AZStd::move(y2));
                t = AZStd::move(t2);
                EXPECT_EQ(x2, AZStd::get<0>(t));
                EXPECT_EQ(&x, &AZStd::get<0>(t));
                EXPECT_EQ(y2, AZStd::get<1>(t));
                EXPECT_EQ(&y, &AZStd::get<1>(t));
            }
            {
                // test reference assignment.
                using T = AZStd::tuple<AZ::s32&, AZ::s32&&>;
                AZ::s32 x = 42;
                AZ::s32 y = 100;
                AZ::s32 x2 = -1;
                AZ::s32 y2 = 500;
                T t(x, AZStd::move(y));
                T t2(x2, AZStd::move(y2));
                t = AZStd::move(t2);
                EXPECT_EQ(x2, AZStd::get<0>(t));
                EXPECT_EQ(&x, &AZStd::get<0>(t));
                EXPECT_EQ(y2, AZStd::get<1>(t));
                EXPECT_EQ(&y, &AZStd::get<1>(t));
            }
        }
    }

    namespace ConstructTest
    {
        struct Empty {};
        struct Explicit
        {
            explicit Explicit(AZ::s32 x) : m_value(x) {}
            AZ::s32 m_value;
        };

        struct Implicit
        {
            Implicit(AZ::s32 x) : m_value(x) {}
            int m_value;
        };

        struct BaseIntWrapper
        {
            BaseIntWrapper(AZ::s32 num = 0) : m_num(num) {}
            virtual ~BaseIntWrapper() = default;
            AZ::s32 m_num;
        };

        struct DerivedIntWrapper
            : BaseIntWrapper
        {
            explicit DerivedIntWrapper(AZ::s32 num = 0) : BaseIntWrapper(num) {}
        };

        struct IntAssignWrapper
        {
            IntAssignWrapper() = default;
            IntAssignWrapper& operator=(int)
            {
                return *this;
            }
        };

        struct DefaultOnly
        {
            DefaultOnly() = default;
            friend bool operator==(const DefaultOnly&, const DefaultOnly&) { return true; }
        };

        struct NoDefault
        {
        private:
            NoDefault() = delete;
        public:
            explicit NoDefault(int) {}
        };


        struct IllFormedDefault
        {
            IllFormedDefault(AZ::s32 x) : value(x) {}
            template <bool Pred = false>
            IllFormedDefault()
            {
                AZ_TEST_STATIC_ASSERT(Pred);
            }
            AZ::s32 value;
        };

        struct ConstructsWithTupleLeaf
        {
            ConstructsWithTupleLeaf() {}

            ConstructsWithTupleLeaf(const ConstructsWithTupleLeaf &) { GTEST_NONFATAL_FAILURE_("Copy Constructor should not be invoked"); }
            ConstructsWithTupleLeaf(ConstructsWithTupleLeaf &&) {}

            template <class T>
            ConstructsWithTupleLeaf(T)
            {
                static_assert((!AZStd::is_same<T, T>::value), "Constructor instantiated for type other than int");
            }
        };

        struct MoveOnly
        {
            MoveOnly() = default;
            MoveOnly(const MoveOnly&) = delete;
            MoveOnly(MoveOnly&& other) : m_num(AZStd::move(other.m_num)) {}
            MoveOnly& operator=(MoveOnly&& other)
            {
                m_num = other.m_num;
                return *this;
            }
            MoveOnly(AZ::s32 num) : m_num(num) {}
            friend bool operator==(AZ::s32 lhs, const MoveOnly& rhs) { return lhs == rhs.m_num; }
            friend bool operator==(const MoveOnly& lhs, AZ::s32 rhs) { return lhs.m_num == rhs; }
            AZ::s32 m_num;
        };

        struct ImplicitConstexpr
        {
            constexpr ImplicitConstexpr(int i) : m_num(i) {}
            friend constexpr bool operator==(const ImplicitConstexpr& x, const ImplicitConstexpr& y) { return x.m_num == y.m_num; }

            int m_num;
        };

        struct ExplicitConstexpr
        {
            constexpr explicit ExplicitConstexpr(int i) : m_num(i) {}
            friend constexpr bool operator==(const ExplicitConstexpr& x, const ExplicitConstexpr& y) { return x.m_num == y.m_num; }

            int m_num;
        };

        TEST_F(TupleTest, Default_ConstructTest)
        {
            {
                AZStd::tuple<> t;
                (void)t;
            }
            {
                AZStd::tuple<AZ::s32> t;
                EXPECT_EQ(0, AZStd::get<0>(t));
            }
            {
                AZStd::tuple<AZ::s32, char*> t;
                EXPECT_EQ(0, AZStd::get<0>(t));
                EXPECT_EQ(nullptr, AZStd::get<1>(t));
            }
            {
                AZStd::tuple<AZ::s32, char*, AZStd::string> t;
                EXPECT_EQ(0, AZStd::get<0>(t));
                EXPECT_EQ(nullptr, AZStd::get<1>(t));
                EXPECT_EQ("", AZStd::get<2>(t));
            }
            {
                AZStd::tuple<AZ::s32, char*, AZStd::string, DefaultOnly> t;
                EXPECT_EQ(0, AZStd::get<0>(t));
                EXPECT_EQ(nullptr, AZStd::get<1>(t));
                EXPECT_EQ("", AZStd::get<2>(t));
                EXPECT_EQ(DefaultOnly(), AZStd::get<3>(t));
            }
            {
                AZ_TEST_STATIC_ASSERT((!std::is_default_constructible<AZStd::tuple<NoDefault>>::value));
                AZ_TEST_STATIC_ASSERT((!std::is_default_constructible<AZStd::tuple<DefaultOnly, NoDefault>>::value));
                AZ_TEST_STATIC_ASSERT((!std::is_default_constructible<AZStd::tuple<NoDefault, DefaultOnly, NoDefault>>::value));
            }
            {
                const AZStd::tuple<> t;
                (void)t;
            }
            {
                const AZStd::tuple<AZ::s32> t;
                EXPECT_EQ(0, AZStd::get<0>(t));
            }
            {
                const AZStd::tuple<AZ::s32, char*> t;
                EXPECT_EQ(0, AZStd::get<0>(t));
                EXPECT_EQ(nullptr, AZStd::get<1>(t));
            }
            {
                // Check that the SFINAE on the default constructor is not evaluated when
                // it isn't needed. If the default constructor is evaluated then this test
                // should fail to compile.
                IllFormedDefault v(0);
                AZStd::tuple<IllFormedDefault> t(v);
            }
        }

        TEST_F(TupleTest, IsDestructible_ConstructTest)
        {
            AZ_TEST_STATIC_ASSERT((std::is_trivially_destructible<AZStd::tuple<> >::value));
            AZ_TEST_STATIC_ASSERT((std::is_trivially_destructible<AZStd::tuple<void*> >::value));
            AZ_TEST_STATIC_ASSERT((std::is_trivially_destructible<AZStd::tuple<AZ::s32, float> >::value));
            AZ_TEST_STATIC_ASSERT((!std::is_trivially_destructible<AZStd::tuple<AZStd::string> >::value));
            AZ_TEST_STATIC_ASSERT((!std::is_trivially_destructible<AZStd::tuple<AZ::s32, AZStd::string> >::value));
        }

        TEST_F(TupleTest, CopyPair_ConstructTest)
        {
            {
                using T0 = AZStd::pair<AZ::s32, char>;
                using T1 = AZStd::tuple<AZ::s64, AZ::s16>;
                T0 t0(2, 'a');
                T1 t1 = to_std_pair(t0);
                EXPECT_EQ(2, AZStd::get<0>(t1));
                EXPECT_EQ(AZ::s16('a'), AZStd::get<1>(t1));
            }
            {
                using P0 = AZStd::pair<AZ::s32, char>;
                using T1 = AZStd::tuple<AZ::s64, AZ::s16>;
                const P0 p0(2, 'a');
                const T1 t1 = to_std_pair(p0);
                EXPECT_EQ(AZStd::get<0>(p0), AZStd::get<0>(t1));
                EXPECT_EQ(AZStd::get<1>(p0), AZStd::get<1>(t1));
                EXPECT_EQ(2, AZStd::get<0>(t1));
                EXPECT_EQ(AZ::s16('a'), AZStd::get<1>(t1));
            }
        }

        TEST_F(TupleTest, MovePair_ConstructTest)
        {
            {
                using T0 = AZStd::pair<long, AZStd::unique_ptr<DerivedIntWrapper>>;
                using T1 = AZStd::tuple<long long, AZStd::unique_ptr<BaseIntWrapper>>;
                T0 t0(2, AZStd::make_unique<DerivedIntWrapper>(3));
                T1 t1 = to_std_pair(AZStd::move(t0));
                EXPECT_EQ(2, AZStd::get<0>(t1));
                EXPECT_EQ(3, AZStd::get<1>(t1)->m_num);
            }
        }

        TEST_F(TupleTest, ConversionCopy_ConstructTest)
        {
            {
                using T0 = AZStd::tuple<AZ::s32>;
                using T1 = AZStd::tuple<AZ::s64>;
                T0 t0(2);
                T1 t1 = t0;
                EXPECT_EQ(2, AZStd::get<0>(t1));
            }
            {
                using T0 = AZStd::tuple<AZ::s32, char>;
                using T1 = AZStd::tuple<AZ::s64, AZ::s32>;
                T0 t0(2, 'a');
                T1 t1 = t0;
                EXPECT_EQ(2, AZStd::get<0>(t1));
                EXPECT_EQ(AZ::s32('a'), AZStd::get<1>(t1));
            }
            {
                using T0 = AZStd::tuple<AZ::s32, char, DerivedIntWrapper>;
                using T1 = AZStd::tuple<AZ::s64, AZ::s32, BaseIntWrapper>;
                T0 t0(2, 'a', DerivedIntWrapper(3));
                T1 t1(t0);
                EXPECT_EQ(2, AZStd::get<0>(t1));
                EXPECT_EQ(AZ::s32('a'), AZStd::get<1>(t1));
                EXPECT_EQ(3, AZStd::get<2>(t1).m_num);
            }
            {
                DerivedIntWrapper d(3);
                using T0 = AZStd::tuple<AZ::s32, char, DerivedIntWrapper&>;
                using T1 = AZStd::tuple<AZ::s64, AZ::s32, BaseIntWrapper&>;
                T0 t0(2, 'a', d);
                T1 t1(t0);
                d.m_num = 2;
                EXPECT_EQ(2, AZStd::get<0>(t1));
                EXPECT_EQ(AZ::s32('a'), AZStd::get<1>(t1));
                EXPECT_EQ(2, AZStd::get<2>(t1).m_num);
            }

            {
                using T0 = AZStd::tuple<AZ::s32, char, AZ::s32>;
                using T1 = AZStd::tuple<AZ::s64, AZ::s32, BaseIntWrapper>;
                T0 t0(2, 'a', 3);
                T1 t1(t0);
                EXPECT_EQ(2, AZStd::get<0>(t1));
                EXPECT_EQ(AZ::s32('a'), AZStd::get<1>(t1));
                EXPECT_EQ(3, AZStd::get<2>(t1).m_num);
            }
            {
                const AZStd::tuple<int> t1(42);
                AZStd::tuple<Explicit> t2(t1);
                EXPECT_EQ(42, AZStd::get<0>(t2).m_value);
            }
            {
                const AZStd::tuple<int> t1(42);
                AZStd::tuple<Implicit> t2 = t1;
                EXPECT_EQ(42, AZStd::get<0>(t2).m_value);
            }
        }

        TEST_F(TupleTest, ConversionMove_ConstructTest)
        {
            {
                using T0 =  AZStd::tuple<AZ::s32>;
                using T1 = AZStd::tuple<AZ::s64>;
                T0 t0(2);
                T1 t1 = AZStd::move(t0);
                EXPECT_EQ(2, AZStd::get<0>(t1));
            }
            {
                using T0 =  AZStd::tuple<AZ::s32, char>;
                using T1 = AZStd::tuple<AZ::s64, AZ::s32>;
                T0 t0(2, 'a');
                T1 t1 = AZStd::move(t0);
                EXPECT_EQ(2, AZStd::get<0>(t1));
                EXPECT_EQ(AZ::s32('a'), AZStd::get<1>(t1));
            }
            {
                using T0 = AZStd::tuple<AZ::s32, char, DerivedIntWrapper>;
                using T1 = AZStd::tuple<AZ::s64, AZ::s32, BaseIntWrapper>;
                T0 t0(2, 'a', DerivedIntWrapper(3));
                T1 t1 = AZStd::move(t0);
                EXPECT_EQ(2, AZStd::get<0>(t1));
                EXPECT_EQ(AZ::s32('a'), AZStd::get<1>(t1));
                EXPECT_EQ(3, AZStd::get<2>(t1).m_num);
            }
            {
                DerivedIntWrapper d(3);
                using T0 = AZStd::tuple<AZ::s32, char, DerivedIntWrapper&>;
                using T1 = AZStd::tuple<AZ::s64, AZ::s32, BaseIntWrapper&>;
                T0 t0(2, 'a', d);
                T1 t1 = AZStd::move(t0);
                d.m_num = 2;
                EXPECT_EQ(2, AZStd::get<0>(t1));
                EXPECT_EQ(AZ::s32('a'), AZStd::get<1>(t1));
                EXPECT_EQ(2, AZStd::get<2>(t1).m_num);
            }
            {
                using T0 = AZStd::tuple<AZ::s32, char, AZStd::unique_ptr<DerivedIntWrapper>>;
                using T1 = AZStd::tuple<AZ::s64, AZ::s32, AZStd::unique_ptr<BaseIntWrapper>>;
                T0 t0(2, 'a', AZStd::make_unique<DerivedIntWrapper>(3));
                T1 t1 = AZStd::move(t0);
                EXPECT_EQ(2, AZStd::get<0>(t1));
                EXPECT_EQ(AZ::s32('a'), AZStd::get<1>(t1));
                EXPECT_EQ(3, AZStd::get<2>(t1)->m_num);
            }
            {
                AZStd::tuple<int> t1(42);
                AZStd::tuple<Explicit> t2(AZStd::move(t1));
                EXPECT_EQ(42, AZStd::get<0>(t2).m_value);
            }
            {
                AZStd::tuple<int> t1(42);
                AZStd::tuple<Implicit> t2 = AZStd::move(t1);
                EXPECT_EQ(42, AZStd::get<0>(t2).m_value);
            }
        }

        TEST_F(TupleTest, Copy_ConstructTest)
        {
            {
                using T = AZStd::tuple<>;
                T t0;
                T t = t0;
                ((void)t); // Prevent unused warning
            }
            {
                using T = AZStd::tuple<AZ::s32>;
                T t0(2);
                T t = t0;
                EXPECT_EQ(2, AZStd::get<0>(t));
            }
            {
                using T = AZStd::tuple<AZ::s32, char>;
                T t0(2, 'a');
                T t = t0;
                EXPECT_EQ(2, AZStd::get<0>(t));
                EXPECT_EQ('a', AZStd::get<1>(t));
            }
            {
                using T = AZStd::tuple<AZ::s32, char, AZStd::string>;
                const T t0(2, 'a', "some text");
                T t = t0;
                EXPECT_EQ(2, AZStd::get<0>(t));
                EXPECT_EQ('a', AZStd::get<1>(t));
                EXPECT_EQ("some text", AZStd::get<2>(t));
            }
            {
                using T = AZStd::tuple<AZ::s32>;
                const T t0(2);
                const T t = t0;
                EXPECT_EQ(2, AZStd::get<0>(t));
            }
            {
                using T =  AZStd::tuple<Empty>;
                const T t0;
                const T t = t0;
                const Empty e = AZStd::get<0>(t);
                ((void)e); // Prevent unused warning
            }
        }

        TEST_F(TupleTest, Move_ConstructTest)
        {
            {
                using T = AZStd::tuple<>;
                T t0;
                T t = AZStd::move(t0);
                ((void)t); // Prevent unused warning
            }
            {
                using T =  AZStd::tuple<MoveOnly>;
                T t0(MoveOnly(0));
                T t = AZStd::move(t0);
                EXPECT_EQ(0, AZStd::get<0>(t));
            }
            {
                using T = AZStd::tuple<MoveOnly, MoveOnly>;
                T t0(MoveOnly(0), MoveOnly(1));
                T t = AZStd::move(t0);
                EXPECT_EQ(0, AZStd::get<0>(t));
                EXPECT_EQ(1, AZStd::get<1>(t));
            }
            {
                using T = AZStd::tuple<MoveOnly, MoveOnly, MoveOnly>;
                T t0(MoveOnly(0), MoveOnly(1), MoveOnly(2));
                T t = AZStd::move(t0);
                EXPECT_EQ(0, AZStd::get<0>(t));
                EXPECT_EQ(1, AZStd::get<1>(t));
                EXPECT_EQ(2, AZStd::get<2>(t));
            }
            {
                using d_t = AZStd::tuple<ConstructsWithTupleLeaf>;
                d_t d((ConstructsWithTupleLeaf()));
                d_t d2(static_cast<d_t &&>(d));
            }
        }

        TEST_F(TupleTest, TemplateDepth_ConstructTest)
        {
            AZStd::array<char, 1256> source;
            AZStd::tuple<AZStd::array<char, 1256>> resultTuple(source);
        }

    }

    namespace ApplyTest
    {
        static AZ::s32 const_sum_fn() { return 0; }

        template <class FirstInt, class ...Ints>
        static AZ::s32 const_sum_fn(FirstInt x1, Ints... rest) { return static_cast<AZ::s32>(x1 + const_sum_fn(rest...)); }

        struct ConstSumT
        {
            ConstSumT() = default;
            template <class ...Ints>
            int operator()(Ints... values) const
            {
                return const_sum_fn(values...);
            }
        };

        TEST_F(TupleTest, ConstEvaluation_ApplyTest)
        {
            ConstSumT sum_obj{};
            {
                using Tup = AZStd::tuple<>;
                using Fn = AZ::s32(&)();
                const Tup t;
                EXPECT_EQ(0, AZStd::apply(static_cast<Fn>(const_sum_fn), t));
                EXPECT_EQ(0, AZStd::apply(sum_obj, t));
            }
            {
                using Tup = AZStd::tuple<AZ::s32>;
                using Fn = AZ::s32(&)(AZ::s32);
                const Tup t(42);
                EXPECT_EQ(42, AZStd::apply(static_cast<Fn>(const_sum_fn<>), t));
                EXPECT_EQ(42, AZStd::apply(sum_obj, t));
            }
            {
                using Tup = AZStd::tuple<AZ::s32, AZ::s64>;
                using Fn = AZ::s32(&)(AZ::s32, AZ::s64);
                const Tup t(42, 101);
                EXPECT_EQ(143, AZStd::apply(static_cast<Fn>(const_sum_fn), t));
                EXPECT_EQ(143, AZStd::apply(sum_obj, t));
            }
            {
                using Tup = AZStd::pair<AZ::s32, AZ::s64>;
                using Fn = AZ::s32(&)(AZ::s32, AZ::s64);
                const Tup t(42, 101);
                EXPECT_EQ(143, AZStd::apply(static_cast<Fn>(const_sum_fn), t));
                EXPECT_EQ(143, AZStd::apply(sum_obj, t));
            }
            {
                using Tup = std::tuple<AZ::s32, AZ::s64, AZ::s32>;
                using Fn = AZ::s32(&)(AZ::s32, AZ::s64, AZ::s32);
                const Tup t(42, 101, -1);
                EXPECT_EQ(142, AZStd::apply(static_cast<Fn>(const_sum_fn), t));
                EXPECT_EQ(142, AZStd::apply(sum_obj, t));
            }
            {
                using Tup = AZStd::array<AZ::s32, 3>;
                using Fn = AZ::s32(&)(AZ::s32, AZ::s32, AZ::s32);
                const Tup t = { { 42, 101, -1 } };
                AZ_TEST_STATIC_ASSERT(AZStd::tuple_size<Tup>::value == 3);
                EXPECT_EQ(42, AZStd::get<0>(t));
                EXPECT_EQ(142, AZStd::apply(static_cast<Fn>(const_sum_fn), t));
                EXPECT_EQ(142, AZStd::apply(sum_obj, t));
            }
        }


        static int my_int = 42;

        template <int N> struct index {};

        void RawFunc(index<0>) {}

        int RawFunc(index<1>) { return 0; }

        int & RawFunc(index<2>) { return static_cast<int &>(my_int); }
        int const & RawFunc(index<3>) { return static_cast<int const &>(my_int); }
        int volatile & RawFunc(index<4>) { return static_cast<int volatile &>(my_int); }
        int const volatile & RawFunc(index<5>) { return static_cast<int const volatile &>(my_int); }

        int && RawFunc(index<6>) { return static_cast<int &&>(my_int); }
        int const && RawFunc(index<7>) { return static_cast<int const &&>(my_int); }
        int volatile && RawFunc(index<8>) { return static_cast<int volatile &&>(my_int); }
        int const volatile && RawFunc(index<9>) { return static_cast<int const volatile &&>(my_int); }

        int * RawFunc(index<10>) { return static_cast<int *>(&my_int); }
        int const * RawFunc(index<11>) { return static_cast<int const *>(&my_int); }
        int volatile * RawFunc(index<12>) { return static_cast<int volatile *>(&my_int); }
        int const volatile * RawFunc(index<13>) { return static_cast<int const volatile *>(&my_int); }

        template <int Func, class Expect>
        void ReturnTypeTest()
        {
            using RawInvokeResult = decltype(RawFunc(index<Func>{}));
            AZ_TEST_STATIC_ASSERT((AZStd::is_same<RawInvokeResult, Expect>::value));
            using FnType = RawInvokeResult(*) (index<Func>);
            FnType fn = RawFunc;
            (void)fn;
            AZStd::tuple<index<Func>> t;
            (void)t;
            using InvokeResult = decltype(AZStd::apply(fn, t));
            AZ_TEST_STATIC_ASSERT((AZStd::is_same<InvokeResult, Expect>::value));
        }

        TEST_F(TupleTest, ReturnType_ApplyTest)
        {
            ReturnTypeTest<0, void>();
            ReturnTypeTest<1, int>();
            ReturnTypeTest<2, int &>();
            ReturnTypeTest<3, int const &>();
            ReturnTypeTest<4, int volatile &>();
            ReturnTypeTest<5, int const volatile &>();
            ReturnTypeTest<6, int &&>();
            ReturnTypeTest<7, int const &&>();
            ReturnTypeTest<8, int volatile &&>();
            ReturnTypeTest<9, int const volatile &&>();
            ReturnTypeTest<10, int *>();
            ReturnTypeTest<11, int const *>();
            ReturnTypeTest<12, int volatile *>();
            ReturnTypeTest<13, int const volatile *>();
        }
    }

    namespace ExtendedApplyTest
    {
        class ExtendedTupleTest
            : public LeakDetectionFixture
        {
        protected:
            static void SetUpTestCase()
            {
                ExtendedTupleTest::m_count = 0;
            }

        public:
            static AZ::s32 m_count;
        };

        AZ::s32 ExtendedTupleTest::m_count = 0;

        struct A_int_0
        {
            A_int_0() : obj1(0) {}
            A_int_0(AZ::s32 x) : obj1(x) {}
            AZ::s32 mem1() { return ++ExtendedTupleTest::m_count; }
            AZ::s32 mem2() const { return ++ExtendedTupleTest::m_count; }
            const AZ::s32 obj1;
            A_int_0& operator=(const A_int_0&) = delete;
        };

        struct A_int_1
        {
            A_int_1() {}
            A_int_1(AZ::s32) {}
            AZ::s32 mem1(AZ::s32 x) { return ExtendedTupleTest::m_count += x; }
            AZ::s32 mem2(AZ::s32 x) const { return ExtendedTupleTest::m_count += x; }
        };

        struct A_int_2
        {
            A_int_2() {}
            A_int_2(AZ::s32) {}
            AZ::s32 mem1(AZ::s32 x, AZ::s32 y) { return ExtendedTupleTest::m_count += (x + y); }
            AZ::s32 mem2(AZ::s32 x, AZ::s32 y) const { return ExtendedTupleTest::m_count += (x + y); }
        };

        template <class A>
        struct A_wrap
        {
            A_wrap() {}
            A_wrap(AZ::s32 x) : m_a(x) {}
            A & operator*() { return m_a; }
            A const & operator*() const { return m_a; }
            A m_a;
        };

        using A_wrap_0 = A_wrap<A_int_0>;
        using A_wrap_1 = A_wrap<A_int_1>;
        using A_wrap_2 = A_wrap<A_int_2>;


        template <class A>
        struct A_base : A
        {
            A_base() : A() {}
            A_base(AZ::s32 x) : A(x) {}
        };

        using A_base_0 = A_base<A_int_0>;
        using A_base_1 = A_base<A_int_1>;
        using A_base_2 = A_base<A_int_2>;


        template <
            class Tuple, class ConstTuple
            , class TuplePtr, class ConstTuplePtr
            , class TupleWrap, class ConstTupleWrap
            , class TupleBase, class ConstTupleBase
        >
            void test_ext_int_0()
        {
            ExtendedTupleTest::m_count = 0;
            using T = A_int_0;
            using Wrap = A_wrap_0;
            using Base = A_base_0;

            using mem1_t =  AZ::s32(T::*)();
            mem1_t mem1 = &T::mem1;

            using mem2_t = AZ::s32(T::*)() const;
            mem2_t mem2 = &T::mem2;

            using obj1_t = AZ::s32 const T::*;
            obj1_t obj1 = &T::obj1;

            // member function w/ref
            {
                T a;
                Tuple t{ { a } };
                EXPECT_EQ(1, AZStd::apply(mem1, t));
                EXPECT_EQ(1, ExtendedTupleTest::m_count);
            }
            ExtendedTupleTest::m_count = 0;
            // member function w/pointer
            {
                T a;
                TuplePtr t{ { &a } };
                EXPECT_EQ(1, AZStd::apply(mem1, t));
                EXPECT_EQ(1, ExtendedTupleTest::m_count);
            }
            ExtendedTupleTest::m_count = 0;
            // member function w/base
            {
                Base a;
                TupleBase t{ { a } };
                EXPECT_EQ(1, AZStd::apply(mem1, t));
                EXPECT_EQ(1, ExtendedTupleTest::m_count);
            }
            ExtendedTupleTest::m_count = 0;
            // member function w/wrap
            {
                Wrap a;
                TupleWrap t{ { a } };
                EXPECT_EQ(1, AZStd::apply(mem1, t));
                EXPECT_EQ(1, ExtendedTupleTest::m_count);
            }
            ExtendedTupleTest::m_count = 0;
            // const member function w/ref
            {
                T const a;
                ConstTuple t{ { a } };
                EXPECT_EQ(1, AZStd::apply(mem2, t));
                EXPECT_EQ(1, ExtendedTupleTest::m_count);
            }
            ExtendedTupleTest::m_count = 0;
            // const member function w/pointer
            {
                T const a;
                ConstTuplePtr t{ { &a } };
                EXPECT_EQ(1, AZStd::apply(mem2, t));
                EXPECT_EQ(1, ExtendedTupleTest::m_count);
            }
            ExtendedTupleTest::m_count = 0;
            // const member function w/base
            {
                Base const a;
                ConstTupleBase t{ { a } };
                EXPECT_EQ(1, AZStd::apply(mem2, t));
                EXPECT_EQ(1, ExtendedTupleTest::m_count);
            }
            ExtendedTupleTest::m_count = 0;
            // const member function w/wrapper
            {
                Wrap const a;
                ConstTupleWrap t{ { a } };
                EXPECT_EQ(1, AZStd::apply(mem2, t));
                EXPECT_EQ(1, ExtendedTupleTest::m_count);
            }
            // member object w/ref
            {
                T a{ 42 };
                Tuple t{ { a } };
                EXPECT_EQ(42, AZStd::apply(obj1, t));
            }
            // member object w/pointer
            {
                T a{ 42 };
                TuplePtr t{ { &a } };
                EXPECT_EQ(42, AZStd::apply(obj1, t));
            }
            // member object w/base
            {
                Base a{ 42 };
                TupleBase t{ { a } };
                EXPECT_EQ(42, AZStd::apply(obj1, t));
            }
            // member object w/wrapper
            {
                Wrap a{ 42 };
                TupleWrap t{ { a } };
                EXPECT_EQ(42, AZStd::apply(obj1, t));
            }
        }


        template <
            class Tuple, class ConstTuple
            , class TuplePtr, class ConstTuplePtr
            , class TupleWrap, class ConstTupleWrap
            , class TupleBase, class ConstTupleBase
        >
            void test_ext_int_1()
        {
            ExtendedTupleTest::m_count = 0;
            typedef A_int_1 T;
            typedef A_wrap_1 Wrap;
            typedef A_base_1 Base;

            using mem1_t = AZ::s32(T::*)(AZ::s32);
            mem1_t mem1 = &T::mem1;

            using mem2_t = AZ::s32(T::*)(AZ::s32) const;
            mem2_t mem2 = &T::mem2;

            // member function w/ref
            {
                T a;
                Tuple t{ a, 2 };
                EXPECT_EQ(2, AZStd::apply(mem1, t));
                EXPECT_EQ(2, ExtendedTupleTest::m_count);
            }
            ExtendedTupleTest::m_count = 0;
            // member function w/pointer
            {
                T a;
                TuplePtr t{ &a, 3 };
                EXPECT_EQ(3, AZStd::apply(mem1, t));
                EXPECT_EQ(3, ExtendedTupleTest::m_count);
            }
            ExtendedTupleTest::m_count = 0;
            // member function w/base
            {
                Base a;
                TupleBase t{ a, 4 };
                EXPECT_EQ(4, AZStd::apply(mem1, t));
                EXPECT_EQ(4, ExtendedTupleTest::m_count);
            }
            ExtendedTupleTest::m_count = 0;
            // member function w/wrap
            {
                Wrap a;
                TupleWrap t{ a, 5 };
                EXPECT_EQ(5, AZStd::apply(mem1, t));
                EXPECT_EQ(5, ExtendedTupleTest::m_count);
            }
            ExtendedTupleTest::m_count = 0;
            // const member function w/ref
            {
                T const a;
                ConstTuple t{ a, 6 };
                EXPECT_EQ(6, AZStd::apply(mem2, t));
                EXPECT_EQ(6, ExtendedTupleTest::m_count);
            }
            ExtendedTupleTest::m_count = 0;
            // const member function w/pointer
            {
                T const a;
                ConstTuplePtr t{ &a, 7 };
                EXPECT_EQ(7, AZStd::apply(mem2, t));
                EXPECT_EQ(7, ExtendedTupleTest::m_count);
            }
            ExtendedTupleTest::m_count = 0;
            // const member function w/base
            {
                Base const a;
                ConstTupleBase t{ a, 8 };
                EXPECT_EQ(8, AZStd::apply(mem2, t));
                EXPECT_EQ(8, ExtendedTupleTest::m_count);
            }
            ExtendedTupleTest::m_count = 0;
            // const member function w/wrapper
            {
                Wrap const a;
                ConstTupleWrap t{ a, 9 };
                EXPECT_EQ(9, AZStd::apply(mem2, t));
                EXPECT_EQ(9, ExtendedTupleTest::m_count);
            }
        }


        template <
            class Tuple, class ConstTuple
            , class TuplePtr, class ConstTuplePtr
            , class TupleWrap, class ConstTupleWrap
            , class TupleBase, class ConstTupleBase
        >
            void test_ext_int_2()
        {
            ExtendedTupleTest::m_count = 0;
            typedef A_int_2 T;
            typedef A_wrap_2 Wrap;
            typedef A_base_2 Base;

            using mem1_t = AZ::s32(T::*)(AZ::s32, AZ::s32);
            mem1_t mem1 = &T::mem1;

            using mem2_t = AZ::s32(T::*)(AZ::s32, int) const;
            mem2_t mem2 = &T::mem2;

            // member function w/ref
            {
                T a;
                Tuple t{ a, 1, 1 };
                EXPECT_EQ(2, AZStd::apply(mem1, t));
                EXPECT_EQ(2, ExtendedTupleTest::m_count);
            }
            ExtendedTupleTest::m_count = 0;
            // member function w/pointer
            {
                T a;
                TuplePtr t{ &a, 1, 2 };
                EXPECT_EQ(3, AZStd::apply(mem1, t));
                EXPECT_EQ(3, ExtendedTupleTest::m_count);
            }
            ExtendedTupleTest::m_count = 0;
            // member function w/base
            {
                Base a;
                TupleBase t{ a, 2, 2 };
                EXPECT_EQ(4, AZStd::apply(mem1, t));
                EXPECT_EQ(4, ExtendedTupleTest::m_count);
            }
            ExtendedTupleTest::m_count = 0;
            // member function w/wrap
            {
                Wrap a;
                TupleWrap t{ a, 2, 3 };
                EXPECT_EQ(5, AZStd::apply(mem1, t));
                EXPECT_EQ(5, ExtendedTupleTest::m_count);
            }
            ExtendedTupleTest::m_count = 0;
            // const member function w/ref
            {
                T const a;
                ConstTuple t{ a, 3, 3 };
                EXPECT_EQ(6, AZStd::apply(mem2, t));
                EXPECT_EQ(6, ExtendedTupleTest::m_count);
            }
            ExtendedTupleTest::m_count = 0;
            // const member function w/pointer
            {
                T const a;
                ConstTuplePtr t{ &a, 3, 4 };
                EXPECT_EQ(7, AZStd::apply(mem2, t));
                EXPECT_EQ(7, ExtendedTupleTest::m_count);
            }
            ExtendedTupleTest::m_count = 0;
            // const member function w/base
            {
                Base const a;
                ConstTupleBase t{ a, 4, 4 };
                EXPECT_EQ(8, AZStd::apply(mem2, t));
                EXPECT_EQ(8, ExtendedTupleTest::m_count);
            }
            ExtendedTupleTest::m_count = 0;
            // const member function w/wrapper
            {
                Wrap const a;
                ConstTupleWrap t{ a, 4, 5 };
                EXPECT_EQ(9, AZStd::apply(mem2, t));
                EXPECT_EQ(9, ExtendedTupleTest::m_count);
            }
        }

        TEST_F(ExtendedTupleTest, Int0_ExtendedApplyTest)
        {
            test_ext_int_0<
                AZStd::tuple<A_int_0 &>, AZStd::tuple<A_int_0 const &>
                , AZStd::tuple<A_int_0 *>, AZStd::tuple<A_int_0 const *>
                , AZStd::tuple<A_wrap_0 &>, AZStd::tuple<A_wrap_0 const &>
                , AZStd::tuple<A_base_0 &>, AZStd::tuple<A_base_0 const &>
            >();
            test_ext_int_0<
                AZStd::tuple<A_int_0>, AZStd::tuple<A_int_0 const>
                , AZStd::tuple<A_int_0 *>, AZStd::tuple<A_int_0 const *>
                , AZStd::tuple<A_wrap_0>, AZStd::tuple<A_wrap_0 const>
                , AZStd::tuple<A_base_0>, AZStd::tuple<A_base_0 const>
            >();
            test_ext_int_0<
                AZStd::array<A_int_0, 1>, AZStd::array<A_int_0 const, 1>
                , AZStd::array<A_int_0*, 1>, AZStd::array<A_int_0 const*, 1>
                , AZStd::array<A_wrap_0, 1>, AZStd::array<A_wrap_0 const, 1>
                , AZStd::array<A_base_0, 1>, AZStd::array<A_base_0 const, 1>
            >();
        }

        TEST_F(ExtendedTupleTest, Int1_ExtendedApplyTest)
        {
            test_ext_int_1<
                AZStd::tuple<A_int_1 &, int>, AZStd::tuple<A_int_1 const &, int>
                , AZStd::tuple<A_int_1 *, int>, AZStd::tuple<A_int_1 const *, int>
                , AZStd::tuple<A_wrap_1 &, int>, AZStd::tuple<A_wrap_1 const &, int>
                , AZStd::tuple<A_base_1 &, int>, AZStd::tuple<A_base_1 const &, int>
            >();
            test_ext_int_1<
                AZStd::tuple<A_int_1, int>, AZStd::tuple<A_int_1 const, int>
                , AZStd::tuple<A_int_1 *, int>, AZStd::tuple<A_int_1 const *, int>
                , AZStd::tuple<A_wrap_1, int>, AZStd::tuple<A_wrap_1 const, int>
                , AZStd::tuple<A_base_1, int>, AZStd::tuple<A_base_1 const, int>
            >();
            test_ext_int_1<
                AZStd::pair<A_int_1 &, int>, AZStd::pair<A_int_1 const &, int>
                , AZStd::pair<A_int_1 *, int>, AZStd::pair<A_int_1 const *, int>
                , AZStd::pair<A_wrap_1 &, int>, AZStd::pair<A_wrap_1 const &, int>
                , AZStd::pair<A_base_1 &, int>, AZStd::pair<A_base_1 const &, int>
            >();
            test_ext_int_1<
                AZStd::pair<A_int_1, int>, AZStd::pair<A_int_1 const, int>
                , AZStd::pair<A_int_1 *, int>, AZStd::pair<A_int_1 const *, int>
                , AZStd::pair<A_wrap_1, int>, AZStd::pair<A_wrap_1 const, int>
                , AZStd::pair<A_base_1, int>, AZStd::pair<A_base_1 const, int>
            >();
        }

        TEST_F(ExtendedTupleTest, Int2_ExtendedApplyTest)
        {
            test_ext_int_2<
                AZStd::tuple<A_int_2 &, int, int>, AZStd::tuple<A_int_2 const &, int, int>
                , AZStd::tuple<A_int_2 *, int, int>, AZStd::tuple<A_int_2 const *, int, int>
                , AZStd::tuple<A_wrap_2 &, int, int>, AZStd::tuple<A_wrap_2 const &, int, int>
                , AZStd::tuple<A_base_2 &, int, int>, AZStd::tuple<A_base_2 const &, int, int>
            >();
            test_ext_int_2<
                AZStd::tuple<A_int_2, int, int>, AZStd::tuple<A_int_2 const, int, int>
                , AZStd::tuple<A_int_2 *, int, int>, AZStd::tuple<A_int_2 const *, int, int>
                , AZStd::tuple<A_wrap_2, int, int>, AZStd::tuple<A_wrap_2 const, int, int>
                , AZStd::tuple<A_base_2, int, int>, AZStd::tuple<A_base_2 const, int, int>
            >();
        }
    }
}
