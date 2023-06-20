/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/std/hash.h>
#include <AzCore/std/optional.h>

#include "UserTypes.h"

using AZStd::optional;
using AZStd::nullopt;
using AZStd::in_place;

namespace OptionalTestClasses
{
    class NonTriviallyDestructableClass
    {
    public:
        ~NonTriviallyDestructableClass() {}
    };

    class NonTriviallyConstructibleClass
    {
    public:
        NonTriviallyConstructibleClass() {}
    };

    class ConstructibleClass
    {
    public:
        enum Tag
        {
            TheTag
        };

        ConstructibleClass(Tag) {}
    };

    class ConstructibleWithInitializerListClass
    {
    public:
        ConstructibleWithInitializerListClass(std::initializer_list<const char*> il, int theInt)
            : m_char(*il.begin())
            , m_int(theInt)
        {
        }

        const char* m_char;
        int m_int;
    };
} // end namespace OptionalTestClasses

namespace UnitTest
{

    using namespace OptionalTestClasses;

    class OptionalFixture
        : public LeakDetectionFixture
    {
    };

    static_assert(!(optional<int>()), "optional constructed with no args should be false");
    static_assert(!(optional<int>(nullopt)), "optional constructed with nullopt should be false");
    TEST_F(OptionalFixture, ConstructorNonTrivial)
    {
        optional<NonTriviallyConstructibleClass> opt;
        EXPECT_FALSE(bool(opt)) << "optional constructed with no args should be false";
    }

    TEST_F(OptionalFixture, ConstructorInPlace)
    {
        const optional<int> opt(in_place, 5);
        EXPECT_TRUE(bool(opt)) << "optional constructed with args should be true";
        EXPECT_EQ(*opt, 5);
    }

    TEST_F(OptionalFixture, ConstructorInPlaceNonTriviallyDestructable)
    {
        const optional<NonTriviallyDestructableClass> opt(in_place);
        EXPECT_TRUE(bool(opt)) << "optional constructed with args should be true";
    }

    TEST_F(OptionalFixture, ConstructorInPlaceWithInitializerList)
    {
        const optional<ConstructibleWithInitializerListClass> opt(in_place, {"O3DE"}, 4);
        EXPECT_TRUE(bool(opt)) << "optional constructed with args should be true";
    }

    TEST_F(OptionalFixture, ConstructorCopyTrivial)
    {
        const optional<int> opt(in_place, 5);
        const optional<int> optCopy(opt);
        EXPECT_EQ(bool(opt), bool(optCopy)) << "Copying an optional should result in the same bool() value";
    }

    TEST_F(OptionalFixture, ConstructorMoveTrivial)
    {
        const optional<int> opt(in_place, 5);
        const optional<int> optMoved(AZStd::move(opt));
        EXPECT_EQ(bool(opt), bool(optMoved)) << "Moving an optional should result in the same bool() value";
    }

    TEST_F(OptionalFixture, ConstructorMoveNonTrivial)
    {
        AZStd::string s1 {"Hello"};
        AZStd::string s2(AZStd::move(s1));
        EXPECT_NE(s1, s2);

        optional<AZStd::string> opt {"Hello"};
        const optional<AZStd::string> optMoved(AZStd::move(opt));
        EXPECT_EQ(bool(opt), bool(optMoved)) << "Moving an optional should result in the same bool() value";
        EXPECT_NE(opt.value(), optMoved.value());
    }

    TEST_F(OptionalFixture, CanAssignFromEmptyOptional)
    {
        optional<int> opt1;
        opt1 = {};
        EXPECT_FALSE(bool(opt1)) << "Optional should still be empty";
    }
    TEST_F(OptionalFixture, AZStdHashActuallyCompiles)
    {
        constexpr optional<int> opt1{ 5 };
        constexpr size_t hashValue = AZStd::hash<optional<int>>{}(opt1);
        static_assert(hashValue != 0, "Hash of engaged optional of int within non-zero value should not be 0");
        EXPECT_NE(0, hashValue);
    }
} // end namespace UnitTest
