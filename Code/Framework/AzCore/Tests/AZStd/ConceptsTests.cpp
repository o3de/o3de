/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/concepts/concepts.h>

namespace UnitTest
{
    class ConceptsTestFixture
        : public ScopedAllocatorSetupFixture
    {};

    TEST_F(ConceptsTestFixture, GeneralConcepts)
    {

        // concept same_as
        static_assert(AZStd::same_as<ConceptsTestFixture, ConceptsTestFixture>);
        static_assert(!AZStd::same_as<ConceptsTestFixture, ScopedAllocatorSetupFixture>);

        // concept derived_from
        static_assert(AZStd::derived_from<ConceptsTestFixture, ScopedAllocatorSetupFixture>);
        static_assert(!AZStd::derived_from<ScopedAllocatorSetupFixture, ConceptsTestFixture>);

        // concept convertible_to
        static_assert(AZStd::convertible_to<ConceptsTestFixture&, ScopedAllocatorSetupFixture&>);
        static_assert(!AZStd::convertible_to<ScopedAllocatorSetupFixture&, ConceptsTestFixture&>);


        // Test structs to validate common_reference_with and common_with concepts
        struct Base {};
        struct TestBase : Base {};
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
        static_assert(!AZStd::common_reference_with<AllocatorsTestFixture, ScopedAllocatorSetupFixture>);

        // concept common_with
        static_assert(AZStd::common_with<TestBase, Base>);
        static_assert(!AZStd::common_with<AllocatorsTestFixture, AllocatorsBenchmarkFixture>);

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
}
