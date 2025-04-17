/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <ratio>

namespace AZStd
{
    // [ratio.ratio](http://eel.is/c++draft/ratio#ratio), class template ratio
    using std::ratio;

    // [ratio.arithmetic](http://eel.is/c++draft/ratio#arithmetic), ratio arithmetic
    using std::ratio_add;
    using std::ratio_subtract;
    using std::ratio_multiply;
    using std::ratio_divide;

    // [ratio.comparison](http://eel.is/c++draft/ratio#comparison), ratio comparison
    using std::ratio_equal;
    using std::ratio_equal_v;
    using std::ratio_not_equal;
    using std::ratio_not_equal_v;
    using std::ratio_less;
    using std::ratio_less_v;
    using std::ratio_less_equal;
    using std::ratio_less_equal_v;
    using std::ratio_greater;
    using std::ratio_greater_v;
    using std::ratio_greater_equal;
    using std::ratio_greater_equal_v;

    // [ratio.si](http://eel.is/c++draft/ratio#si), convenience SI type aliases
    using std::atto;
    using std::femto;
    using std::pico;
    using std::nano;
    using std::micro;
    using std::milli;
    using std::centi;
    using std::deci;
    using std::deca;
    using std::hecto;
    using std::kilo;
    using std::mega;
    using std::giga;
    using std::tera;
    using std::peta;
    using std::exa;
}
