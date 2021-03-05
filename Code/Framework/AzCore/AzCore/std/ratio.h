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
#ifndef AZSTD_RATIO_H
#define AZSTD_RATIO_H

#include <AzCore/std/base.h>
#include <AzCore/std/typetraits/integral_constant.h>

#include <cstdlib>
#include <climits>
#include <limits>

// Fix for windows without NO_MIN_MAX define, or any other abuse of such basic keywords.
#if defined(AZ_COMPILER_MSVC)
#   ifdef min
#       pragma push_macro("min")
#       undef min
#       define AZ_WINDOWS_MIN_PUSHED
#   endif
#   ifdef max
#       pragma push_macro("max")
#       undef max
#       define AZ_WINDOWS_MAX_PUSHED
#   endif
#endif

namespace AZStd
{
    //----------------------------------------------------------------------------//
    //                                                                            //
    //              20.4 Compile-time rational arithmetic [ratio]                 //
    //                                                                            //
    //----------------------------------------------------------------------------//

    template <AZ::s64 N, AZ::s64 D = 1>
    class ratio;

    // ratio arithmetic
    template <class R1, class R2>
    struct ratio_add;
    template <class R1, class R2>
    struct ratio_subtract;
    template <class R1, class R2>
    struct ratio_multiply;
    template <class R1, class R2>
    struct ratio_divide;

    // ratio comparison
    template <class R1, class R2>
    struct ratio_equal;
    template <class R1, class R2>
    struct ratio_not_equal;
    template <class R1, class R2>
    struct ratio_less;
    template <class R1, class R2>
    struct ratio_less_equal;
    template <class R1, class R2>
    struct ratio_greater;
    template <class R1, class R2>
    struct ratio_greater_equal;

    // convenience SI typedefs
    typedef ratio<1LL, 1000000000000000000LL> atto;
    typedef ratio<1LL,    1000000000000000LL> femto;
    typedef ratio<1LL,       1000000000000LL> pico;
    typedef ratio<1LL,          1000000000LL> nano;
    typedef ratio<1LL,             1000000LL> micro;
    typedef ratio<1LL,                1000LL> milli;
    typedef ratio<1LL,                 100LL> centi;
    typedef ratio<1LL,                  10LL> deci;
    typedef ratio<                 10LL, 1LL> deca;
    typedef ratio<                100LL, 1LL> hecto;
    typedef ratio<               1000LL, 1LL> kilo;
    typedef ratio<            1000000LL, 1LL> mega;
    typedef ratio<         1000000000LL, 1LL> giga;
    typedef ratio<      1000000000000LL, 1LL> tera;
    typedef ratio<   1000000000000000LL, 1LL> peta;
    typedef ratio<1000000000000000000LL, 1LL> exa;

    //----------------------------------------------------------------------------//
    //                                 helpers                                    //
    //----------------------------------------------------------------------------//

    namespace Internal
    {
        // static_gcd

        template <AZ::s64 X, AZ::s64 Y>
        struct static_gcd
        {
            static const AZ::s64 value = static_gcd<Y, X % Y>::value;
        };

        template <AZ::s64 X>
        struct static_gcd<X, 0>
        {
            static const AZ::s64 value = X;
        };

        // static_lcm

        template <AZ::s64 X, AZ::s64 Y>
        struct static_lcm
        {
            static const AZ::s64 value = X / static_gcd<X, Y>::value * Y;
        };

        template <AZ::s64 X>
        struct static_abs
        {
            static const AZ::s64 value = X < 0 ? -X : X;
        };

        template <AZ::s64 X>
        struct static_sign
        {
            static const AZ::s64 value = X == 0 ? 0 : (X < 0 ? -1 : 1);
        };

        template <AZ::s64 X, AZ::s64 Y, AZ::s64 = static_sign<Y>::value>
        class ll_add;

        template <AZ::s64 X, AZ::s64 Y>
        class ll_add<X, Y, 1>
        {
            static const AZ::s64 min =
                (1LL << (sizeof(AZ::s64) * CHAR_BIT - 1)) + 1;
            static const AZ::s64 max = -min;

            static char test[X <= max - Y];
            //    static_assert(X <= max - Y, "overflow in ll_add");
        public:
            static const AZ::s64 value = X + Y;
        };

        template <AZ::s64 X, AZ::s64 Y>
        class ll_add<X, Y, 0>
        {
        public:
            static const AZ::s64 value = X;
        };

        template <AZ::s64 X, AZ::s64 Y>
        class ll_add<X, Y, -1>
        {
            static const AZ::s64 min =
                (1LL << (sizeof(AZ::s64) * CHAR_BIT - 1)) + 1;
            static const AZ::s64 max = -min;

            static char test[min - Y <= X];
            //    static_assert(min - Y <= X, "overflow in ll_add");
        public:
            static const AZ::s64 value = X + Y;
        };

        template <AZ::s64 X, AZ::s64 Y, AZ::s64 = static_sign<Y>::value>
        class ll_sub;

        template <AZ::s64 X, AZ::s64 Y>
        class ll_sub<X, Y, 1>
        {
            static const AZ::s64 min =
                (1LL << (sizeof(AZ::s64) * CHAR_BIT - 1)) + 1;
            static const AZ::s64 max = -min;

            static char test[min + Y <= X];
            //    static_assert(min + Y <= X, "overflow in ll_sub");
        public:
            static const AZ::s64 value = X - Y;
        };

        template <AZ::s64 X, AZ::s64 Y>
        class ll_sub<X, Y, 0>
        {
        public:
            static const AZ::s64 value = X;
        };

        template <AZ::s64 X, AZ::s64 Y>
        class ll_sub<X, Y, -1>
        {
            static const AZ::s64 min =
                (1LL << (sizeof(AZ::s64) * CHAR_BIT - 1)) + 1;
            static const AZ::s64 max = -min;

            static char test[X <= max + Y];
            //    static_assert(X <= max + Y, "overflow in ll_sub");
        public:
            static const AZ::s64 value = X - Y;
        };

        template <AZ::s64 X, AZ::s64 Y>
        class ll_mul
        {
            static const AZ::s64 nan =
                (1LL << (sizeof(AZ::s64) * CHAR_BIT - 1));
            static const AZ::s64 min = nan + 1;
            static const AZ::s64 max = -min;
            static const AZ::s64 a_x = static_abs<X>::value;
            static const AZ::s64 a_y = static_abs<Y>::value;

            static char test1[X != nan];
            static char test2[Y != nan];
            static char test[a_x <= max / a_y];
            //    static_assert(X != nan && Y != nan && a_x <= max / a_y, "overflow in ll_mul");
        public:
            static const AZ::s64 value = X * Y;
        };

        template <AZ::s64 Y>
        class ll_mul<0, Y>
        {
        public:
            static const AZ::s64 value = 0;
        };

        template <AZ::s64 X>
        class ll_mul<X, 0>
        {
        public:
            static const AZ::s64 value = 0;
        };

        template <>
        class ll_mul<0, 0>
        {
        public:
            static const AZ::s64 value = 0;
        };

        // Not actually used but left here in case needed in future maintenance
        template <AZ::s64 X, AZ::s64 Y>
        class ll_div
        {
            static const AZ::s64 nan = (1LL << (sizeof(AZ::s64) * CHAR_BIT - 1));
            static const AZ::s64 min = nan + 1;
            static const AZ::s64 max = -min;

            static char test1[X != nan];
            static char test2[Y != nan];
            static char test3[Y != 0];
            //    static_assert(X != nan && Y != nan && Y != 0, "overflow in ll_div");
        public:
            static const AZ::s64 value = X / Y;
        };

        //template <class T>
        //  struct is_ratio : public false_type {};
        //template <intmax_t N, intmax_t D>
        //  struct is_ratio<ratio<N, D> > : public true_type  {};
        //template <class T>
        //  struct is_ratio : is_ratio<typename remove_cv<T>::type> {};
    }  // namespace Internal

    //----------------------------------------------------------------------------//
    //                                                                            //
    //                20.4.1 Class template ratio [ratio.ratio]                   //
    //                                                                            //
    //----------------------------------------------------------------------------//

    template <AZ::s64 N, AZ::s64 D>
    class ratio
    {
        static char test1[Internal::static_abs < N > ::value >= 0];
        static char test2[Internal::static_abs < D > ::value >  0];
        //    static_assert(Internal::static_abs<N>::value >= 0, "ratio numerator is out of range");
        //    static_assert(D != 0, "ratio divide by 0");
        //    static_assert(Internal::static_abs<D>::value >  0, "ratio denominator is out of range");
        static const AZ::s64 m_na = Internal::static_abs<N>::value;
        static const AZ::s64 m_da = Internal::static_abs<D>::value;
        static const AZ::s64 m_s = Internal::static_sign<N>::value  * Internal::static_sign<D>::value;
        static const AZ::s64 m_gcd = Internal::static_gcd<m_na, m_da>::value;
    public:
        static const AZ::s64 num = m_s * m_na / m_gcd;
        static const AZ::s64 den = m_da / m_gcd;
    };

    //----------------------------------------------------------------------------//
    //                                                                            //
    //                             Implementation                                 //
    //                                                                            //
    //----------------------------------------------------------------------------//

    template <class R1, class R2>
    struct ratio_add
    {
    private:
        static const AZ::s64 gcd_n1_n2 = Internal::static_gcd<R1::num, R2::num>::value;
        static const AZ::s64 gcd_d1_d2 = Internal::static_gcd<R1::den, R2::den>::value;
    public:
        typedef typename ratio_multiply
            <
            ratio<gcd_n1_n2, R1::den / gcd_d1_d2>,
            ratio
            <
                Internal::ll_add
                <
                    Internal::ll_mul<R1::num / gcd_n1_n2, R2::den / gcd_d1_d2>::value,
                    Internal::ll_mul<R2::num / gcd_n1_n2, R1::den / gcd_d1_d2>::value
                >::value,
                R2::den
            >
            >::type type;
    };

    template <class R1, class R2>
    struct ratio_subtract
    {
    private:
        static const AZ::s64 gcd_n1_n2 = Internal::static_gcd<R1::num, R2::num>::value;
        static const AZ::s64 gcd_d1_d2 = Internal::static_gcd<R1::den, R2::den>::value;
    public:
        typedef typename ratio_multiply
            <
            ratio<gcd_n1_n2, R1::den / gcd_d1_d2>, ratio <
                Internal::ll_sub < Internal::ll_mul<R1::num / gcd_n1_n2, R2::den / gcd_d1_d2>::value, Internal::ll_mul<R2::num / gcd_n1_n2, R1::den / gcd_d1_d2>::value >::value,
                R2::den >
            >::type type;
    };

    template <class R1, class R2>
    struct ratio_multiply
    {
    private:
        static const AZ::s64 gcd_n1_d2 = Internal::static_gcd<R1::num, R2::den>::value;
        static const AZ::s64 gcd_d1_n2 = Internal::static_gcd<R1::den, R2::num>::value;
    public:
        typedef ratio
            < Internal::ll_mul<R1::num / gcd_n1_d2, R2::num / gcd_d1_n2>::value, Internal::ll_mul<R2::den / gcd_n1_d2, R1::den / gcd_d1_n2>::value > type;
    };

    template <class R1, class R2>
    struct ratio_divide
    {
    private:
        static const AZ::s64 gcd_n1_n2 = Internal::static_gcd<R1::num, R2::num>::value;
        static const AZ::s64 gcd_d1_d2 = Internal::static_gcd<R1::den, R2::den>::value;
    public:
        typedef ratio
            < Internal::ll_mul<R1::num / gcd_n1_n2, R2::den / gcd_d1_d2>::value, Internal::ll_mul<R2::num / gcd_n1_n2, R1::den / gcd_d1_d2>::value > type;
    };

    // ratio_equal
    template <class R1, class R2>
    struct ratio_equal
        : public AZStd::integral_constant<bool, (R1::num == R2::num&& R1::den == R2::den) > {};

    template <class R1, class R2>
    struct ratio_not_equal
        : public AZStd::integral_constant<bool, !ratio_equal<R1, R2>::value> {};
    // ratio_less

    //----------------------------------------------------------------------------//
    //                              more helpers                                  //
    //----------------------------------------------------------------------------//
    namespace Internal
    {
        // Protect against overflow, and still get the right answer as much as possible.
        //   This just demonstrates for fun how far you can push things without hitting
        //   overflow.  The obvious and simple implementation is conforming.

        template <class R1, class R2, bool ok1, bool ok2>
        struct ratio_less3 // true, true and false, false
        {
            static const bool value = ll_mul<R1::num, R2::den>::value < ll_mul<R2::num, R1::den>::value;
        };

        template <class R1, class R2>
        struct ratio_less3<R1, R2, true, false>
        {
            static const bool value = true;
        };

        template <class R1, class R2>
        struct ratio_less3<R1, R2, false, true>
        {
            static const bool value = false;
        };

        template < class R1, class R2, bool = (R1::num < R1::den == R2::num < R2::den) >
                    struct ratio_less2 // N1 < D1 == N2 < D2
        {
            static const AZ::s64 max = -((1LL << (sizeof(AZ::s64) * CHAR_BIT - 1)) + 1);
            static const bool ok1 = R1::num <= max / R2::den;
            static const bool ok2 = R2::num <= max / R1::den;
            static const bool value = ratio_less3<R1, R2, ok1, ok2>::value;
        };

        template <class R1, class R2>
        struct ratio_less2<R1, R2, false>  // N1 < D1 != N2 < D2
        {
            static const bool value = R1::num < R1::den;
        };

        template < class R1, class R2, bool = (R1::num < R1::den == R2::num < R2::den) >
                    struct ratio_less1 // N1 < D1 == N2 < D2
        {
            static const bool value = ratio_less2<ratio<R1::num, R2::num>,
                    ratio<R1::den, R2::den> >::value;
        };

        template <class R1, class R2>
        struct ratio_less1<R1, R2, false>  // N1 < D1 != N2 < D2
        {
            static const bool value = R1::num < R1::den;
        };

        template <class R1, class R2, AZ::s64 S1 = static_sign<R1::num>::value, AZ::s64 S2 = static_sign<R2::num>::value>
        struct ratio_less
        {
            static const bool value = S1 < S2;
        };

        template <class R1, class R2>
        struct ratio_less<R1, R2, 1LL, 1LL>
        {
            static const bool value = ratio_less1<R1, R2>::value;
        };

        template <class R1, class R2>
        struct ratio_less<R1, R2, -1LL, -1LL>
        {
            static const bool value = ratio_less1<ratio<-R2::num, R2::den>,
                    ratio<-R1::num, R1::den> >::value;
        };

        template <class R1, class R2>
        struct ratio_gcd
        {
            typedef ratio<Internal::static_gcd<R1::num, R2::num>::value,
                Internal::static_lcm<R1::den, R2::den>::value> type;
        };
    }  // namespace Internal

    //----------------------------------------------------------------------------//
    //                                                                            //
    //                           more implementation                              //
    //                                                                            //
    //----------------------------------------------------------------------------//

    template <class R1, class R2>
    struct ratio_less
        : public AZStd::integral_constant<bool, Internal::ratio_less<R1, R2>::value> {};

    template <class R1, class R2>
    struct ratio_less_equal
        : public AZStd::integral_constant<bool, !ratio_less<R2, R1>::value> {};

    template <class R1, class R2>
    struct ratio_greater
        : public AZStd::integral_constant<bool, ratio_less<R2, R1>::value> {};

    template <class R1, class R2>
    struct ratio_greater_equal
        : public AZStd::integral_constant<bool, !ratio_less<R1, R2>::value> {};
}

#ifdef AZ_WINDOWS_MIN_PUSHED
#   pragma pop_macro("min")
#   undef AZ_WINDOWS_MIN_PUSHED
#endif
#ifdef AZ_WINDOWS_MAX_PUSHED
#   pragma pop_macro("max")
#   undef AZ_WINDOWS_MAX_PUSHED
#endif

#endif // AZSTD_RATIO_H
#pragma once


