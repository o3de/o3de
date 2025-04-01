/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "AzCore/Outcome/Outcome.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    //! Class that tracks how it was build and modified
    struct KnowThyself
    {
        enum class State
        {
            NormalConstructor,
            CopyConstructor,
            MoveConstructor,
            InitializerListConstructor,
            CopyAssignment,
            MoveAssignment,
            StolenFrom,
            Destroyed,
        };

        KnowThyself(int id)
            : m_state(State::NormalConstructor)
            , m_id(id)
        {}

        KnowThyself(const KnowThyself& rhs)
            : m_state(State::CopyConstructor)
            , m_id(rhs.m_id)
            , m_originalState(rhs.m_state)
            , m_lineage(rhs.m_lineage + 1)
        {
        }

        KnowThyself(KnowThyself&& rhs)
            : m_state(State::MoveConstructor)
            , m_id(rhs.m_id)
            , m_originalState(rhs.m_state)
            , m_lineage(rhs.m_lineage + 1)
        {
            rhs.m_state = State::StolenFrom;
        }

        KnowThyself(std::initializer_list<int> lastBecomesId)
            : m_state(State::InitializerListConstructor)
            , m_id(*(lastBecomesId.end() - 1))
        {}

        KnowThyself& operator=(const KnowThyself& rhs)
        {
            m_state = State::CopyAssignment;
            m_id = rhs.m_id;
            m_originalState = rhs.m_state;
            m_lineage = rhs.m_lineage + 1;
            return *this;
        }

        KnowThyself& operator=(KnowThyself&& rhs)
        {
            m_state = State::MoveAssignment;
            m_id = rhs.m_id;
            m_originalState = rhs.m_state;
            m_lineage = rhs.m_lineage + 1;
            rhs.m_state = State::StolenFrom;
            return *this;
        }

        ~KnowThyself()
        {
            m_state = State::Destroyed;
        }

        State m_state = State::Destroyed;
        int m_id = -1;
        State m_originalState = State::Destroyed;
        int m_lineage = 0;
    };

    struct Aligned16
    {
        alignas(16) char m_data;
    };

    //! Class that changes a value when it's created and destroyed.
    //! +1 on creation. -1 on destruction.
    struct RefCounter
    {
        RefCounter(int* ref)
        {
            intPtr = ref;
            (*intPtr)++;
        }

        RefCounter(const RefCounter& other)
        {
            intPtr = other.intPtr;
            (*intPtr)++;
        }

        RefCounter(RefCounter&& other)
        {
            intPtr = other.intPtr;
            other.intPtr = nullptr;
        }

        ~RefCounter()
        {
            if (intPtr)
            {
                (*intPtr)--;
            }
        }

        RefCounter& operator=(const RefCounter& other)
        {
            intPtr = other.intPtr;
            (*intPtr)++;
            return *this;
        }

        int* intPtr;
    };

    //! A class for testing that the appropriate constructor and operator=
    //! functions are used when copying Outcome's contained value.
    //! A raw copy of this class or its members will not suffice.
    struct ClassWithComplexMembers
    {
        //! unordered_set contains a list,
        //! The list contains a head node with next and prev pointers.
        //! In an empty list, the head node points at itself.
        //! A raw copy of this list results in a head node that
        //! points to the head node from the other list.
        AZStd::unordered_set<uint32_t> s1;
        AZStd::unordered_set<uint32_t> s2;
    };

    class OutcomeTest
        : public ::testing::Test
    {
    public:
        enum class Error
        {
            InvalidInput,
            KnuckleHeads,
        };

        AZ::Outcome<KnowThyself, Error> TestReturnsError()
        {
            return AZ::Failure(Error::InvalidInput);
        }

        AZ::Outcome<KnowThyself, Error> TestReturnsValue(int id)
        {
            return AZ::Success(KnowThyself(id));
        }

        void run()
        {
            { // test function returning error
                auto outcome = TestReturnsError();
                AZ_TEST_ASSERT(!outcome.IsSuccess());
                AZ_TEST_ASSERT(outcome.GetError() == Error::InvalidInput);
                if (outcome)
                {
                    AZ_TEST_ASSERT(false && "bool operator not working for errors");
                }

                // test const and non-const GetError()
                const auto& constOutcome = outcome;
                AZ_TEST_ASSERT(constOutcome.GetError() == Error::InvalidInput);

                outcome.GetError() = Error::KnuckleHeads;
                AZ_TEST_ASSERT(outcome.GetError() == Error::KnuckleHeads);
            }

            { // test function returning success
                auto outcome = TestReturnsValue(5);
                AZ_TEST_ASSERT(outcome.IsSuccess());
                AZ_TEST_ASSERT(outcome.GetValue().m_id == 5);
                if (!outcome)
                {
                    AZ_TEST_ASSERT(false && "bool operator not working for successes");
                }

                // test const and non-const GetValue()
                const auto& constOutcome = outcome;
                AZ_TEST_ASSERT(constOutcome.GetValue().m_id == 5);

                outcome.GetValue().m_id = 7;
                AZ_TEST_ASSERT(outcome.GetValue().m_id == 7);
            }

            { // test value move constructor
                AZ::Outcome<KnowThyself, Error> outcome = AZ::Success(KnowThyself(5));
                AZ_TEST_ASSERT(outcome.IsSuccess());
                AZ_TEST_ASSERT(outcome.GetValue().m_id == 5);
                AZ_TEST_ASSERT(outcome.GetValue().m_state == KnowThyself::State::MoveConstructor);
            }

            { // test value copy constructor
                KnowThyself lvalue(5);
                AZ::Outcome<KnowThyself, Error> outcome = AZ::Success(lvalue);
                AZ_TEST_ASSERT(outcome.IsSuccess());
                AZ_TEST_ASSERT(outcome.GetValue().m_id == 5);
                AZ_TEST_ASSERT(outcome.GetValue().m_state == KnowThyself::State::CopyConstructor);
                AZ_TEST_ASSERT(lvalue.m_state != KnowThyself::State::StolenFrom);
            }


            { // test error copy constructor
                KnowThyself lvalue(5);
                AZ::Outcome<int, KnowThyself> outcome = AZ::Failure(lvalue);
                AZ_TEST_ASSERT(!outcome.IsSuccess());
                AZ_TEST_ASSERT(outcome.GetError().m_id == 5);
                AZ_TEST_ASSERT(outcome.GetError().m_state == KnowThyself::State::CopyConstructor);
            }

            { // test error move constructor
                AZ::Outcome<int, KnowThyself> outcome = AZ::Failure(KnowThyself(5));
                AZ_TEST_ASSERT(!outcome.IsSuccess());
                AZ_TEST_ASSERT(outcome.GetError().m_id == 5);
                AZ_TEST_ASSERT(outcome.GetError().m_state == KnowThyself::State::MoveConstructor);
            }

            { // test copy constructor on successful outcome
                AZ::Outcome<KnowThyself, Error> outcomeSrc = AZ::Success(KnowThyself(5));
                AZ::Outcome<KnowThyself, Error> outcomeDst = outcomeSrc;
                AZ_TEST_ASSERT(outcomeDst.IsSuccess());
                AZ_TEST_ASSERT(outcomeDst.GetValue().m_id == 5);
                AZ_TEST_ASSERT(outcomeDst.GetValue().m_state == KnowThyself::State::CopyConstructor);
                AZ_TEST_ASSERT(outcomeSrc.GetValue().m_state != KnowThyself::State::StolenFrom);
            }

            { // test copy constructor on failed outcome
                AZ::Outcome<int, KnowThyself> outcomeSrc = AZ::Failure(KnowThyself(5));
                AZ::Outcome<int, KnowThyself> outcomeDst = outcomeSrc;
                AZ_TEST_ASSERT(!outcomeDst.IsSuccess());
                AZ_TEST_ASSERT(outcomeDst.GetError().m_id == 5);
                AZ_TEST_ASSERT(outcomeDst.GetError().m_state == KnowThyself::State::CopyConstructor);
                AZ_TEST_ASSERT(outcomeSrc.GetError().m_state != KnowThyself::State::StolenFrom);
            }

            { // test move constructor on successful outcome
                AZ::Outcome<KnowThyself, Error> outcome(AZ::Outcome<KnowThyself, Error>(AZ::Success(KnowThyself(5))));
                AZ_TEST_ASSERT(outcome.IsSuccess());
                AZ_TEST_ASSERT(outcome.GetValue().m_id == 5);
                AZ_TEST_ASSERT(outcome.GetValue().m_state == KnowThyself::State::MoveConstructor);
            }

            { // test move constructor on failed outcome
                AZ::Outcome<int, KnowThyself> outcome(AZ::Outcome<int, KnowThyself>(AZ::Failure(KnowThyself(5))));
                AZ_TEST_ASSERT(!outcome.IsSuccess());
                AZ_TEST_ASSERT(outcome.GetError().m_id == 5);
                AZ_TEST_ASSERT(outcome.GetError().m_state == KnowThyself::State::MoveConstructor);
            }

            { // test assignment between successful outcomes
                AZ::Outcome<KnowThyself, int> outcomeSrc(AZ::Success(KnowThyself(5)));
                AZ::Outcome<KnowThyself, int> outcomeDst(AZ::Success(KnowThyself(99)));

                outcomeDst = outcomeSrc; // copy from src
                AZ_TEST_ASSERT(outcomeDst.GetValue().m_id == 5);
                AZ_TEST_ASSERT(outcomeDst.GetValue().m_state == KnowThyself::State::CopyAssignment);
                AZ_TEST_ASSERT(outcomeSrc.GetValue().m_state != KnowThyself::State::StolenFrom);

                outcomeDst = AZStd::move(outcomeSrc); // move from src
                AZ_TEST_ASSERT(outcomeDst.GetValue().m_id == 5);
                AZ_TEST_ASSERT(outcomeDst.GetValue().m_state == KnowThyself::State::MoveAssignment);
                AZ_TEST_ASSERT(outcomeSrc.GetValue().m_state == KnowThyself::State::StolenFrom);
            }

            { // test assignment between failed outcomes
                AZ::Outcome<int, KnowThyself> outcomeSrc(AZ::Failure(KnowThyself(5)));
                AZ::Outcome<int, KnowThyself> outcomeDst(AZ::Failure(KnowThyself(99)));

                outcomeDst = outcomeSrc; // copy from src
                AZ_TEST_ASSERT(outcomeDst.GetError().m_id == 5);
                AZ_TEST_ASSERT(outcomeDst.GetError().m_state == KnowThyself::State::CopyAssignment);
                AZ_TEST_ASSERT(outcomeSrc.GetError().m_state != KnowThyself::State::StolenFrom);

                outcomeDst = AZStd::move(outcomeSrc); // move from src
                AZ_TEST_ASSERT(outcomeDst.GetError().m_id == 5);
                AZ_TEST_ASSERT(outcomeDst.GetError().m_state == KnowThyself::State::MoveAssignment);
                AZ_TEST_ASSERT(outcomeSrc.GetError().m_state == KnowThyself::State::StolenFrom);
            }

            { // test assignment operator going from successful to failed outcome
                int counter = 0;
                AZ::Outcome<RefCounter, KnowThyself> successToFailure = AZ::Success(RefCounter(&counter));
                AZ::Outcome<RefCounter, KnowThyself> failedOutcome = AZ::Failure(KnowThyself(5));

                AZ_TEST_ASSERT(counter == 1);
                successToFailure = failedOutcome;
                AZ_TEST_ASSERT(counter == 0); // ensure destructor ran for successToFailure.m_success
                AZ_TEST_ASSERT(!successToFailure.IsSuccess());
                AZ_TEST_ASSERT(successToFailure.GetError().m_id == 5);
                AZ_TEST_ASSERT(failedOutcome.GetError().m_state != KnowThyself::State::StolenFrom);

                successToFailure = AZ::Success(RefCounter(&counter)); // reset back to success
                AZ_TEST_ASSERT(counter == 1);
                successToFailure = AZStd::move(failedOutcome);
                AZ_TEST_ASSERT(counter == 0); // ensure destructor ran for successToFailure.m_success
                AZ_TEST_ASSERT(!successToFailure.IsSuccess());
                AZ_TEST_ASSERT(successToFailure.GetError().m_id == 5);
                AZ_TEST_ASSERT(failedOutcome.GetError().m_state == KnowThyself::State::StolenFrom);
            }

            { // test assignment operator going from failed to successful outcome
                int counter = 0;
                AZ::Outcome<KnowThyself, RefCounter> failureToSuccess = AZ::Failure(RefCounter(&counter));
                AZ::Outcome<KnowThyself, RefCounter> successfulOutcome = AZ::Success(KnowThyself(5));

                AZ_TEST_ASSERT(counter == 1);
                failureToSuccess = successfulOutcome;
                AZ_TEST_ASSERT(counter == 0); // ensure destructor ran for failureToSuccess.m_failure
                AZ_TEST_ASSERT(failureToSuccess.IsSuccess());
                AZ_TEST_ASSERT(failureToSuccess.GetValue().m_id == 5);
                AZ_TEST_ASSERT(successfulOutcome.GetValue().m_state != KnowThyself::State::StolenFrom);

                failureToSuccess = AZ::Failure(RefCounter(&counter)); // reset back to failure
                AZ_TEST_ASSERT(counter == 1);
                failureToSuccess = AZStd::move(successfulOutcome);
                AZ_TEST_ASSERT(counter == 0); // ensure destructor ran for failureToSuccess.m_failure
                AZ_TEST_ASSERT(failureToSuccess.IsSuccess());
                AZ_TEST_ASSERT(failureToSuccess.GetValue().m_id == 5);
                AZ_TEST_ASSERT(successfulOutcome.GetValue().m_state == KnowThyself::State::StolenFrom);
            }

            { // test value type with trivial destructor
                AZ::Outcome<int, KnowThyself> outcome = AZ::Success(5);
                AZ_TEST_ASSERT(outcome.IsSuccess());
            }

            { // test error type with trivial destructor
                AZ::Outcome<KnowThyself, int> outcome = AZ::Failure(5);
                AZ_TEST_ASSERT(!outcome.IsSuccess());
            }

            { // test TakeValue and TakeError
                AZ::Outcome<KnowThyself, Error> goodOutcome = AZ::Success(KnowThyself(5));
                KnowThyself fromGoodOutcome = goodOutcome.TakeValue();
                AZ_TEST_ASSERT(fromGoodOutcome.m_id == 5);
                AZ_TEST_ASSERT(fromGoodOutcome.m_state == KnowThyself::State::MoveConstructor);
                AZ_TEST_ASSERT(goodOutcome.GetValue().m_state == KnowThyself::State::StolenFrom);

                AZ::Outcome<int, KnowThyself> failedOutcome = AZ::Failure(KnowThyself(5));
                KnowThyself fromFailedOutcome = failedOutcome.TakeError();
                AZ_TEST_ASSERT(fromFailedOutcome.m_id == 5);
                AZ_TEST_ASSERT(fromFailedOutcome.m_state == KnowThyself::State::MoveConstructor);
                AZ_TEST_ASSERT(failedOutcome.GetError().m_state == KnowThyself::State::StolenFrom);
            }

            { // test GetValueOr()
                AZ::Outcome<KnowThyself, Error> goodOutcome = AZ::Success(KnowThyself(5));
                AZ::Outcome<KnowThyself, Error> failedOutcome = AZ::Failure(Error::InvalidInput);

                KnowThyself shouldBeFive = goodOutcome.GetValueOr(KnowThyself(7));
                AZ_TEST_ASSERT(shouldBeFive.m_id == 5);

                KnowThyself shouldBeSeven = failedOutcome.GetValueOr(KnowThyself(7));
                AZ_TEST_ASSERT(shouldBeSeven.m_id == 7);

                // test that GetValueOr properly handles rvalue parameters
                KnowThyself shouldBeStolenFrom(9);
                KnowThyself shouldBeNine = failedOutcome.GetValueOr(AZStd::move(shouldBeStolenFrom));
                AZ_TEST_ASSERT(shouldBeNine.m_id == 9);
                AZ_TEST_ASSERT(shouldBeStolenFrom.m_state == KnowThyself::State::StolenFrom);

                // test that GetValueOr properly handles lvalue parameters
                KnowThyself shouldEqualGood = failedOutcome.GetValueOr(goodOutcome.GetValue());
                AZ_TEST_ASSERT(shouldEqualGood.m_id == 5);
                AZ_TEST_ASSERT(goodOutcome.GetValue().m_state != KnowThyself::State::StolenFrom);
            }

            { // test alignment
                AZ_TEST_STATIC_ASSERT((AZStd::alignment_of<AZ::Outcome<Aligned16, char> >::value == 16));
                AZ_TEST_STATIC_ASSERT((AZStd::alignment_of<AZ::Outcome<char, Aligned16> >::value == 16));
                AZ::Outcome<Aligned16, char> outcomeA = AZ::Success(Aligned16());
                AZ::Outcome<char, Aligned16> outcomeB = AZ::Failure(Aligned16());
                AZ_TEST_ASSERT(reinterpret_cast<size_t>(&outcomeA.GetValue().m_data) % 16 == 0);
                AZ_TEST_ASSERT(reinterpret_cast<size_t>(&outcomeB.GetError().m_data) % 16 == 0);
            }

            { // test with void types
                AZ::Outcome<void, void> outcome1 = AZ::Success();
                AZ_TEST_ASSERT(outcome1.IsSuccess());
                AZ::Outcome<void, void> outcome2 = AZ::Failure();
                AZ_TEST_ASSERT(!outcome2.IsSuccess());

                AZ::Outcome<void, Error> outcome3 = AZ::Success();
                AZ_TEST_ASSERT(outcome3.IsSuccess());
                AZ::Outcome<void, Error> outcome4 = AZ::Failure(Error::KnuckleHeads);
                AZ_TEST_ASSERT(!outcome4.IsSuccess());
                AZ_TEST_ASSERT(outcome4.GetError() == Error::KnuckleHeads);

                AZ::Outcome<Error, void> outcome5 = AZ::Success(Error::KnuckleHeads);
                AZ_TEST_ASSERT(outcome5.IsSuccess());
                AZ_TEST_ASSERT(outcome5.GetValue() == Error::KnuckleHeads);
                AZ::Outcome<Error, void> outcome6 = AZ::Failure();
                AZ_TEST_ASSERT(!outcome6.IsSuccess());
            }

            { // test that not too many copies/moves are performed
              // Please edit these test cases when number of moves/copies changes
                AZ::Outcome<KnowThyself> outcome1 = AZ::Success(KnowThyself(2));
                AZ_TEST_ASSERT(outcome1.GetValue().m_state == KnowThyself::State::MoveConstructor);
                AZ_TEST_ASSERT(outcome1.GetValue().m_lineage == 2);
                KnowThyself val1 = outcome1.GetValue();
                AZ_TEST_ASSERT(val1.m_lineage == 3);

                AZ::Outcome<KnowThyself> outcome2 = AZStd::move(outcome1);
                AZ_TEST_ASSERT(outcome2.GetValue().m_state == KnowThyself::State::MoveConstructor);

                AZ::Outcome<KnowThyself> outcome3 = outcome2;
                AZ_TEST_ASSERT(outcome3.GetValue().m_state == KnowThyself::State::CopyConstructor);
            }

            {
                // A previous iteration of Outcome would fail due to
                // the SuccessValue<ClassWithComplexMembers> being copied improperly.
                ClassWithComplexMembers complex;
                AZ::Outcome<ClassWithComplexMembers> outcome = AZ::Success(AZStd::move(complex));
                AZ_TEST_ASSERT(outcome.GetValue().s1.empty());
            }
        }
    };

    TEST_F(OutcomeTest, Test)
    {
        run();
    }

    class OutcomeSerializationTest
        : public LeakDetectionFixture
    {
    public:
        // We must expose the class for serialization first.
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();

            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
        }

        void TearDown() override
        {
            m_serializeContext.reset();

            LeakDetectionFixture::TearDown();
        }

    protected:
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
    };


    TEST_F(OutcomeSerializationTest, ReflectingOutcomeTwiceDoesNotLogError)
    {
        using TestOutcome = AZ::Outcome<int, int>;
        AZ::GenericClassInfo* outcomeGenericInfo = AZ::SerializeGenericTypeInfo<TestOutcome>::GetGenericInfo();
        ASSERT_NE(nullptr, outcomeGenericInfo);
        outcomeGenericInfo->Reflect(m_serializeContext.get());
        AZ_TEST_START_TRACE_SUPPRESSION;
        outcomeGenericInfo->Reflect(m_serializeContext.get());
        AZ_TEST_STOP_TRACE_SUPPRESSION(0);
    }
} // namespace UnitTest
