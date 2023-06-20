/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/concepts/concepts.h>
#include <AzCore/std/ranges/ranges_functional.h>

namespace UnitTest
{
    class ConceptsTestFixture
        : public LeakDetectionFixture
    {};

    TEST_F(ConceptsTestFixture, GeneralConcepts)
    {

        // concept same_as
        static_assert(AZStd::same_as<ConceptsTestFixture, ConceptsTestFixture>);
        static_assert(!AZStd::same_as<ConceptsTestFixture, LeakDetectionFixture>);

        // concept derived_from
        static_assert(AZStd::derived_from<ConceptsTestFixture, LeakDetectionFixture>);
        static_assert(!AZStd::derived_from<LeakDetectionFixture, ConceptsTestFixture>);

        // concept convertible_to
        static_assert(AZStd::convertible_to<ConceptsTestFixture&, LeakDetectionFixture&>);
        static_assert(!AZStd::convertible_to<LeakDetectionFixture&, ConceptsTestFixture&>);


        // Test structs to validate common_reference_with and common_with concepts
        struct Base {};
        struct TestBase : Base {};
        struct TestDerived : TestBase {};
        struct TestDerived2 : TestBase {};
        struct NoMove
        {
            NoMove(NoMove&&) = delete;
            NoMove& operator=(NoMove&&) = delete;
        };
        struct NoDestructible
        {
            ~NoDestructible() = delete;
        };
        struct NoDefaultInitializable
        {
            NoDefaultInitializable(bool);
        };

        struct CopyOnly
        {
            CopyOnly(const CopyOnly&) = default;
        };
        struct MoveOnly
        {
            MoveOnly(MoveOnly&&) = default;
        };

        struct MoveableButNotCopyable
        {
            MoveableButNotCopyable(MoveableButNotCopyable&&) = default;
            MoveableButNotCopyable& operator=(MoveableButNotCopyable&&) = default;
        };

        // concept common_reference_with
        static_assert(AZStd::common_reference_with<TestBase&, Base&>);
        static_assert(AZStd::same_as<AZStd::common_reference_t<const TestBase&, Base&>, const Base&>);
        static_assert(!AZStd::common_reference_with<TestDerived2, TestDerived>);

        // concept common_with
        static_assert(AZStd::common_with<TestBase, Base>);
        static_assert(!AZStd::common_with<LeakDetectionFixture, AllocatorsBenchmarkFixture>);

        // arithmetic concepts
        // concept integral
        static_assert(AZStd::integral<int>);
        static_assert(!AZStd::integral<float>);

        // concept signed_integral
        static_assert(AZStd::signed_integral<int>);
        static_assert(!AZStd::signed_integral<unsigned int>);
        static_assert(!AZStd::signed_integral<float>);

        // concept signed_integral
        static_assert(AZStd::unsigned_integral<unsigned int>);
        static_assert(!AZStd::unsigned_integral<int>);
        static_assert(!AZStd::unsigned_integral<float>);

        // concept floating_point
        static_assert(AZStd::floating_point<float>);
        static_assert(!AZStd::floating_point<int>);

        // concept assignable_from
        static_assert(AZStd::assignable_from<Base&, TestBase>);
        static_assert(!AZStd::assignable_from<TestBase&, Base>);

        // concept swappable
        static_assert(AZStd::swappable<Base>);
        static_assert(!AZStd::swappable<NoMove>);
        static_assert(AZStd::swappable_with<Base, Base>);
        static_assert(!AZStd::swappable_with<NoMove, Base>);

        // concept destructible
        static_assert(AZStd::destructible<Base>);
        static_assert(!AZStd::destructible<NoDestructible>);

        // concept constructible_from
        static_assert(AZStd::constructible_from<NoDefaultInitializable, bool>);
        static_assert(!AZStd::constructible_from<NoDefaultInitializable>);

        // concept default_initializable
        static_assert(AZStd::default_initializable<Base>);
        static_assert(!AZStd::default_initializable<NoDefaultInitializable>);

        // concept move_constructible
        static_assert(AZStd::move_constructible<MoveOnly>);
        static_assert(!AZStd::move_constructible<NoMove>);

        // concept copy_constructible
        static_assert(AZStd::copy_constructible<CopyOnly>);
        static_assert(!AZStd::copy_constructible<MoveOnly>);

        // concept equality_comparable
        static_assert(AZStd::equality_comparable<AZStd::string_view>);
        static_assert(!AZStd::equality_comparable<Base>);
        static_assert(AZStd::equality_comparable_with<AZStd::string_view, const char*>);
        static_assert(!AZStd::equality_comparable_with<Base, TestBase>);
        static_assert(!AZStd::equality_comparable_with<Base, const char*>);

        // concept totally_ordered
        static_assert(AZStd::totally_ordered<AZStd::string_view>);
        static_assert(!AZStd::totally_ordered<Base>);
        static_assert(AZStd::totally_ordered_with<AZStd::string_view, const char*>);
        static_assert(!AZStd::totally_ordered_with<Base, TestBase>);
        static_assert(!AZStd::totally_ordered_with<Base, const char*>);

        // concept movable
        static_assert(AZStd::movable<MoveableButNotCopyable>);
        static_assert(!AZStd::movable<NoMove>);

        // concept copyable
        static_assert(AZStd::copyable<Base>);
        static_assert(!AZStd::copyable<MoveableButNotCopyable>);

        // concept semiregular
        static_assert(AZStd::semiregular<Base>);
        static_assert(!AZStd::semiregular<MoveableButNotCopyable>);

        // concept regular
        static_assert(AZStd::regular<AZStd::string_view>);
        static_assert(!AZStd::regular<Base>);

        // concept invocable
        static_assert(AZStd::invocable<decltype(AZStd::ranges::swap), int&, int&>);
        static_assert(!AZStd::invocable<decltype(AZStd::ranges::swap), int&, float&>);

        // concept predicate
        auto BooleanPredicate = [](double) -> int
        {
            return 0;
        };
        auto BasePredicate = [](int) -> Base
        {
            return Base{};
        };

        static_assert(AZStd::predicate<decltype(BooleanPredicate), double>);
        static_assert(!AZStd::predicate<decltype(BooleanPredicate), Base>);
        static_assert(!AZStd::predicate<decltype(BasePredicate), int>);

        // concept relation
        struct RelationPredicate
        {
            bool operator()(AZStd::string_view, Base) const;
            bool operator()(Base, AZStd::string_view) const;
            bool operator()(AZStd::string_view, AZStd::string_view) const;
            bool operator()(Base, Base) const;

            // non-complete relation
            bool operator()(Base, int) const;
            bool operator()(int, Base) const;
        };

        static_assert(AZStd::relation<RelationPredicate, AZStd::string_view, Base>);
        static_assert(AZStd::relation<RelationPredicate, Base, AZStd::string_view>);
        static_assert(!AZStd::relation<RelationPredicate, int, Base>);
        static_assert(!AZStd::relation<RelationPredicate, Base, int>);

        //concept equivalence_relation
        static_assert(AZStd::equivalence_relation<RelationPredicate, AZStd::string_view, Base>);
        static_assert(AZStd::equivalence_relation<RelationPredicate, Base, AZStd::string_view>);
        static_assert(!AZStd::equivalence_relation<RelationPredicate, int, Base>);
        static_assert(!AZStd::equivalence_relation<RelationPredicate, Base, int>);

        //concept strict_weak_order
        static_assert(AZStd::strict_weak_order<RelationPredicate, AZStd::string_view, Base>);
        static_assert(AZStd::strict_weak_order<RelationPredicate, Base, AZStd::string_view>);
        static_assert(!AZStd::strict_weak_order<RelationPredicate, int, Base>);
        static_assert(!AZStd::strict_weak_order<RelationPredicate, Base, int>);
    }

    TEST_F(ConceptsTestFixture, IteratorInvocableConcepts)
    {
        // concept indirectly unary invocable
        // i.e Is deferencing an iterator like type and invoking
        // a unary callable a well formed expression
        auto CharUnaryCallable = [](const char) -> int { return {}; };
        auto IntUnaryCallable = [](int) -> int { return {}; };
        auto IntRefUnaryCallable = [](int&) -> int { return {}; };
        static_assert(AZStd::indirectly_unary_invocable<decltype(CharUnaryCallable), AZStd::string_view::iterator>);
        static_assert(AZStd::indirectly_unary_invocable<decltype(IntUnaryCallable), AZStd::string_view::iterator>);
        static_assert(!AZStd::indirectly_unary_invocable<decltype(IntRefUnaryCallable), AZStd::string_view::iterator>);

        // concept indirectly regular unary invocable
        // i.e Is deferencing an iterator like type and invoking with a unary callable
        // which will not modify the input arguments(hence the term "regular") a well formed expression
        static_assert(AZStd::indirectly_regular_unary_invocable<decltype(CharUnaryCallable), AZStd::string_view::iterator>);
        static_assert(AZStd::indirectly_regular_unary_invocable<decltype(IntUnaryCallable), AZStd::string_view::iterator>);
        static_assert(!AZStd::indirectly_regular_unary_invocable<decltype(IntRefUnaryCallable), AZStd::string_view::iterator>);

        // concept indirect unary predicate
        // i.e Is deferencing an iterator like type and invoking with a unary predicate
        // i.e which is a callable that accepts one argument and returns value testable in a boolean context
        auto CharUnaryPredicate = [](const char) -> bool { return {}; };
        auto IntUnaryPredicate = [](int) -> int { return {}; };// Return value is an int which is convertible to bool
        auto IntRefUnaryPredicate = [](int&) -> int { return {}; }; // string_view iterator value type(char) can't bind to an int&
        auto CharUnaryNonPredicate = [](const char) -> AZStd::string_view { return {}; }; // string_view is not convertible to bool

        static_assert(AZStd::indirect_unary_predicate<decltype(CharUnaryPredicate), AZStd::string_view::iterator>);
        static_assert(AZStd::indirect_unary_predicate<decltype(IntUnaryPredicate), AZStd::string_view::iterator>);
        static_assert(!AZStd::indirect_unary_predicate<decltype(IntRefUnaryPredicate), AZStd::string_view::iterator>);
        static_assert(!AZStd::indirect_unary_predicate<decltype(CharUnaryNonPredicate), AZStd::string_view::iterator>);

        // concept indirect binary predicate
        // i.e Is deferencing two iterator like types and invoking a binary predicate with those values
        // well formed and returns value testable in a boolean context.
        auto CharIntBinaryPredicate = [](const char, int) -> bool { return{}; };
        auto CharCharRefBinaryPredicate = [](const char, const char&) -> uint32_t { return{}; };
        auto UIntRefCharBinaryPredicate = [](uint32_t&, char) -> bool { return{}; };
        auto CharCharBinaryNonPredicate = [](const char, const char) -> AZStd::string_view { return {}; };

        static_assert(AZStd::indirect_binary_predicate<decltype(CharIntBinaryPredicate),
            AZStd::string_view::iterator, AZStd::string_view::iterator>);
        static_assert(AZStd::indirect_binary_predicate<decltype(CharCharRefBinaryPredicate),
            AZStd::string_view::iterator, AZStd::string_view::iterator>);
        // string_view iterator value type(char) cannot bind to int&
        static_assert(!AZStd::indirect_binary_predicate<decltype(UIntRefCharBinaryPredicate),
            AZStd::string_view::iterator, AZStd::string_view::iterator>);
        // string_view is not convertible to bool
        static_assert(!AZStd::indirect_binary_predicate<decltype(CharCharBinaryNonPredicate),
            AZStd::string_view::iterator, AZStd::string_view::iterator>);
        // Ok - iter_reference_t<uint32_t*> = uint32_t&
        static_assert(AZStd::indirect_binary_predicate<decltype(UIntRefCharBinaryPredicate),
            uint32_t*, AZStd::string_view::iterator>);

        // concept indirect equivalence relation
        // i.e Is deferencing two iterator like types and invoking a binary predicate with those values
        // well formed and returns value testable in a boolean context.
        // The dereferenced iterator types should be model an equivalence relationship
        // (a == b) && (b == c) == (a ==c)
        static_assert(AZStd::indirect_equivalence_relation<decltype(CharIntBinaryPredicate),
            AZStd::string_view::iterator, AZStd::string_view::iterator>);
        static_assert(AZStd::indirect_equivalence_relation<decltype(CharCharRefBinaryPredicate),
            AZStd::string_view::iterator, AZStd::string_view::iterator>);
        static_assert(!AZStd::indirect_equivalence_relation<decltype(UIntRefCharBinaryPredicate),
            AZStd::string_view::iterator, AZStd::string_view::iterator>);
        static_assert(!AZStd::indirect_equivalence_relation<decltype(CharCharBinaryNonPredicate),
            AZStd::string_view::iterator, AZStd::string_view::iterator>);
        // The "relation" concept requires that both both arguments can be bind to
        // either of the two binary parameters
        static_assert(!AZStd::indirect_equivalence_relation<decltype(UIntRefCharBinaryPredicate),
            uint32_t*, AZStd::string_view::iterator>);

        // concept indirect strict weak order
        // i.e Is deferencing two iterator like types and invoking a binary predicate with those values
        // well formed and returns value testable in a boolean context.
        // The dereferenced iterator types should be model a strict weak order relation
        // (a < b) && (b < c) == (a < c)
        static_assert(AZStd::indirect_strict_weak_order<decltype(CharIntBinaryPredicate),
            AZStd::string_view::iterator, AZStd::string_view::iterator>);
        static_assert(AZStd::indirect_strict_weak_order<decltype(CharCharRefBinaryPredicate),
            AZStd::string_view::iterator, AZStd::string_view::iterator>);
        static_assert(!AZStd::indirect_strict_weak_order<decltype(UIntRefCharBinaryPredicate),
            AZStd::string_view::iterator, AZStd::string_view::iterator>);
        static_assert(!AZStd::indirect_strict_weak_order<decltype(CharCharBinaryNonPredicate),
            AZStd::string_view::iterator, AZStd::string_view::iterator>);
        // The "relation" concept requires that both both arguments can be bind to
        // either of the two binary parameters
        static_assert(!AZStd::indirect_strict_weak_order<decltype(UIntRefCharBinaryPredicate),
            uint32_t*, AZStd::string_view::iterator>);

        // indirect_result_t type alias
        static_assert(AZStd::same_as<AZStd::indirect_result_t<decltype(CharCharRefBinaryPredicate),
            AZStd::string_view::iterator, const char*>, uint32_t>);

        // projected operator* returns indirect result of the projection function
        static_assert(AZStd::same_as<AZStd::iter_reference_t<AZStd::projected<int*, AZStd::identity>>, int&>);
    }

    TEST_F(ConceptsTestFixture, IteratorAlgorithmConcepts)
    {
        static_assert(AZStd::indirectly_swappable<int*, int*>);
        static_assert(!AZStd::indirectly_swappable<int*, const int*>);
        auto CharIntIndirectlyComparable = [](const char lhs, int rhs) -> bool { return lhs == rhs; };
        static_assert(AZStd::indirectly_comparable<const char*, int*, decltype(CharIntIndirectlyComparable)>);
        static_assert(!AZStd::indirectly_comparable<AZStd::string_view, int*, decltype(CharIntIndirectlyComparable)>);

        static_assert(AZStd::permutable<typename AZStd::vector<int>::iterator>);
        // const iterator isn't indirectlly swappable or indirectly movable
        static_assert(!AZStd::permutable<typename AZStd::vector<int>::const_iterator>);

        static_assert(AZStd::mergeable<AZStd::vector<int>::iterator, AZStd::string_view::iterator, AZStd::vector<int>::iterator>);
        static_assert(!AZStd::mergeable<AZStd::vector<int>::iterator, AZStd::string_view::iterator, AZStd::string_view::iterator>);

        static_assert(AZStd::sortable<int*>);
        // Not sortable becaue the iter_reference_t<const int*> = const int& which isn't swappable
        static_assert(!AZStd::sortable<const int*>);
    }
}
